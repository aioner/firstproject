#include "rvsmq.h"
#include "rvlock.h"
#include "rvsemaphore.h"
#include "rvmemory.h"
#include "rvassert.h"
#include "rvcbase.h"
#include "rvccorestrings.h"
#include "rvthread.h"
#include "rvsemaphore.h"

#if RV_USE_SMQ

/* Implementation of semaphore with TryWait functionality
 * On most OS such functionality already exists, but not on Symbian, for example
 * So, this code should be probably moved to the OS dependent semaphore implementations
 */

typedef struct {
    RvLock      lock;
    RvSemaphore sem;
    RvInt       nWaiters;
    RvInt       nNotify;
    RvInt       count;
} TWSemaphore;

RvStatus TWSemaphoreConstruct(TWSemaphore *self, RvUint count, RvLogMgr *log) {
    RvStatus s;
    
    s = RvLockConstruct(log, &self->lock);
    if(s != RV_OK) {
        return s;
    }
    
    s = RvSemaphoreConstruct(0, log,  &self->sem);
    if(s != RV_OK) {
        RvLockDestruct(&self->lock, log);
        return s;
    }
    
    self->count = count;
    self->nWaiters = 0;
    self->nNotify = 0;
    return RV_OK;
}

RvStatus TWSemaphoreWaitEx(TWSemaphore *self, RvBool wait, RvLogMgr *log) {
    RvStatus s;
    
    s = RvLockGet(&self->lock, log);
    if(s != RV_OK) {
        return s;
    }
    
    if(self->nWaiters >= self->count) {
        
        if(!wait) {
            RvLockRelease(&self->lock, log);
            return RV_ERROR_TRY_AGAIN;
        }
        
        self->nWaiters++;
        RvLockRelease(&self->lock, log);
        RvSemaphoreWait(&self->sem, log);
        RvLockGet(&self->lock, log);
        self->nWaiters--;
        self->nNotify--;
    }
    
    self->count--;
    RvLockRelease(&self->lock, log);
    return RV_OK;    
}

#define TWSemaphoreWait(sem, log) TWSemaphoreWaitEx(sem, RV_TRUE, log)
#define TWSemaphoreTryWait(sem, log) TWSemaphoreWaitEx(sem, RV_FALSE, log)

RvStatus TWSemaphorePost(TWSemaphore *self, RvLogMgr *log) {
    RvStatus s;
    
    s = RvLockGet(&self->lock, log);
    if(s != RV_OK) {
        return s;
    }
    
    self->count++;
    if(self->nWaiters > self->nNotify) {
        self->nNotify++;
        RvSemaphorePost(&self->sem, log);
    }
    
    s = RvLockRelease(&self->lock, log);
    return s;
}

RvStatus TWSemaphoreDestruct(TWSemaphore *self, RvLogMgr *log) {
    RvStatus s = RV_OK;
    
    RvLockDestruct(&self->lock, log);
    RvSemaphoreDestruct(&self->sem, log);
    return s;
}


typedef struct _RvSMQMessage {
	RvSMQMsgTargetId target;
	RvSMQMsgId       message;
	RvSMQMsgParam    param1;
	RvSMQMsgParam    param2;
} RvSMQMessage;

#include "rvselect.h"

/* Defines per-target structure */
/* When message arrives destined for this target, apporpiate callbeck will be called */
typedef struct _RvSMQMsgTarget {
	RvSMQMsgTargetId  id;             /* target id */
	void             *ctx;           /* context */
	RvSMQCb           cb;            /* callback */
	struct _RvSMQMsgTarget *next;   /* pointer to the next structure in the list */
} RvSMQMsgTarget;

typedef struct _RvSMQ {
    RvSMQNotifyCb  notifyCb;
    void        *notifyCtx;
	RvSMQMsgTarget *firstTarget;
	RvSMQMsgTargetId nextTargetId;
	RvInt        nextRead;
	RvInt        nextWrite;
	RvInt        size;
	RvLock       lock;
	TWSemaphore  full;
	RvInt        nmsgs;
    RvThreadId   ownerThreadId;
    RvUint32     dispatchLevel;
	RvSMQMessage  queue[1];
} RvSMQ;

static RvStatus rvSMQGet(HRvSMQ self, RvSMQMsgTargetId *t, RvSMQMsgId *msg, RvSMQMsgParam *p1, RvSMQMsgParam *p2, RvLogMgr *logMgr);

static RvBool rvSMQIsEmpty(HRvSMQ self) {
	return self->nmsgs == 0;
}


RvStatus
RvSMQConstruct(HRvSMQ *pSelf,  RvInt size, RvSMQNotifyCb notifyCB, void *notifyCtx, RvLogMgr *logMgr) {
	RvUint   needed;
	RvStatus s;
	RvInt    stage = 0;
	HRvSMQ    self;
	
#define STRUCT_ALLOCED   1
#define LOCK_CREATED     2
#define SFULL_CREATED    4
	
	
	*pSelf = 0;
	
	if(size <= 0) {
		return RV_ERROR_BADPARAM;
	}
	
	needed = sizeof(RvSMQ) + (size - 1) * sizeof(RvSMQMessage);
	s = RvMemoryAlloc(0, needed, 0, (void **)pSelf);
	if(s != RV_OK) {
		return s;
	}
	
	self = *pSelf;
	stage |= STRUCT_ALLOCED;

	s = RvLockConstruct(logMgr, &self->lock);
    if(s != RV_OK) goto failure;
	stage |= LOCK_CREATED;


	s = TWSemaphoreConstruct(&self->full, size, logMgr);
	if(s != RV_OK) goto failure;
	stage |= SFULL_CREATED;
	
	self->nextRead   = 0;
	self->nextWrite  = 0;
	self->size       = size;
	self->firstTarget      = 0;
	self->nextTargetId = 0;
	self->nmsgs      = 0;
    self->notifyCb   = notifyCB;
    self->notifyCtx  = notifyCtx;
    self->dispatchLevel = 0;
    self->ownerThreadId = RvThreadCurrentId();

	return RV_OK;
	
failure:
	
	if(stage & SFULL_CREATED)  {
		TWSemaphoreDestruct(&self->full, logMgr);
    }

    if(stage & LOCK_CREATED) {
        RvLockDestruct(&self->lock, logMgr);
    }


    if(stage & STRUCT_ALLOCED) {
        RvMemoryFree(self, logMgr);
    }

	return s;

#undef STRUCT_ALLOCED 
#undef LOCK_CREATED     
#undef SFULL_CREATED  

}

RvStatus 
RvSMQDestruct(HRvSMQ self, RvLogMgr *logMgr) {
  RvSMQMsgTarget *cur;
  RvSMQMsgTarget *next;

  RvLockDestruct(&self->lock, logMgr);
  TWSemaphoreDestruct(&self->full, logMgr);

#if USE_EMPTY_SEMAPHORE
  RvSemaphoreDestruct(&self->empty, logMgr);
#endif

  for(cur = self->firstTarget; cur != 0; cur = next) {
    next = cur->next;
    RvMemoryFree(cur, logMgr);
  }

  RvMemoryFree(self, logMgr);
  return RV_OK;
}

void 
RvSMQIndicateNotEmpty(HRvSMQ self, RvLogMgr *logMgr) {
  RV_UNUSED_ARG(logMgr);
  self->notifyCb(self, self->notifyCtx, RV_SMQ_NONEMPTY);
}

RvStatus
RvSMQPost(HRvSMQ self, RvSMQMsgTargetId t, RvSMQMsgId msg, RvSMQMsgParam p1, RvSMQMsgParam p2, RvLogMgr *logMgr) {
	RvStatus s;
	RvSMQMessage *cur;
    RvBool indicateNonEmpty = RV_FALSE;
    RvThreadId thisThreadId = RvThreadCurrentId();
    RvBool semwait = thisThreadId != self->ownerThreadId;

    /* If post was called from the same thread that owns this smq
     *  we shouldn't wait on semaphore, otherwise deadlock may occur. 
     */
   	s = TWSemaphoreWaitEx(&self->full, semwait, logMgr);
    if(s != RV_OK) {
        return (s == RV_ERROR_TRY_AGAIN) ? RV_SMQ_ERROR_FULL : s;
	}
	
	s = RvLockGet(&self->lock, logMgr);
    if(s != RV_OK) {
        return s;
    }

    RvAssert(self->nmsgs != self->size);

    cur = &self->queue[self->nextWrite];
	cur->message = msg;
	cur->param2  = p1;
	cur->param2  = p2;
	cur->target  = t;
	self->nextWrite++;
	self->nextWrite %= self->size;
	self->nmsgs++;
	if(self->nmsgs == 1) {
        indicateNonEmpty = RV_TRUE;
	}
	
	s = RvLockRelease(&self->lock, logMgr);
	if(s != RV_OK) {
		return s;
	}

  if(indicateNonEmpty && !self->dispatchLevel) {
    RvSMQIndicateNotEmpty(self, logMgr);
  }

#if USE_EMPTY_SEMAPHORE
	s = RvSemaphorePost(&self->empty, logMgr); 
#endif
	return s;
}


/* This function is called internally by RvSMQDispatch, that performs locking/unlocking 
 *  and checks that there is actually message to read
 */
static RvStatus rvSMQGet(HRvSMQ self, RvSMQMsgTargetId *t, RvSMQMsgId *msg, RvSMQMsgParam *p1, RvSMQMsgParam *p2, RvLogMgr *logMgr) {
	RvStatus s;
	RvSMQMessage *cur;

	
	cur  = &self->queue[self->nextRead];
	*t   = cur->target;
	*msg = cur->message;
	*p1  = cur->param1;
	*p2  = cur->param2;
	self->nextRead++;
	self->nextRead %= self->size;
	self->nmsgs--;
	
	
	s = TWSemaphorePost(&self->full, logMgr);
	return s;
}


RvStatus 
RvSMQDispatch(HRvSMQ self, RvLogMgr *logMgr) {
    RvStatus s;
    RvSMQMsgTargetId id;
    RvSMQMsgId     msg;
    RvSMQMsgParam  p1;
    RvSMQMsgParam  p2;
    RvSMQMsgTarget *cur;
    RvSMQCb        cb = 0;
    void           *ctx = 0;

    RvLockGet(&self->lock, logMgr);

    while(!rvSMQIsEmpty(self)) {


        s = rvSMQGet(self, &id, &msg, &p1, &p2, logMgr);


        for(cur = self->firstTarget; cur != 0; cur = cur->next) {
            if(cur->id == id) {
                cb = cur->cb;
                ctx = cur->ctx;
                break;
            }
        }

        self->dispatchLevel++;
        RvLockRelease(&self->lock, logMgr);

        if(cb != 0)	{
            cb(id, msg, p1, p2, ctx);
        }

        RvLockGet(&self->lock, logMgr);
        self->dispatchLevel--;
    }

    RvLockRelease(&self->lock, logMgr);

    return RV_OK;

}

RvStatus 
RvSMQRegisterTarget(HRvSMQ self, RvSMQCb cb, void *ctx, RvSMQMsgTargetId *t, RvLogMgr *logMgr) {
	RvSMQMsgTarget *clt;
	RvStatus s = RV_OK;

	
	RvLockGet(&self->lock, logMgr);

	s = RvMemoryAlloc(0, sizeof(*clt), logMgr, (void **)&clt);
	if(s != RV_OK) goto failure;

	clt->id = *t = self->nextTargetId;
	self->nextTargetId++;
	clt->cb = cb;
	clt->ctx = ctx;
	clt->next = self->firstTarget;
	self->firstTarget = clt;
failure:
	
	RvLockRelease(&self->lock, logMgr);
	return s;
}

RvStatus 
RvSMQUnregisterTarget(HRvSMQ self, RvSMQMsgTargetId t, RvLogMgr *logMgr) {
	RvSMQMsgTarget *clt = self->firstTarget;
	RvSMQMsgTarget *prev = 0;

	RvStatus s = RV_OK;

	RvLockGet(&self->lock, logMgr);

	for(; clt != 0 && clt->id != t; prev = clt, clt = clt->next)
    {}

	if(clt != 0) {
		if(prev == 0) {
			self->firstTarget = clt->next;
		} else {
			prev->next = clt->next;
		}
	}

	RvMemoryFree((void *)clt, logMgr);
	RvLockRelease(&self->lock, logMgr);
	return s;
}

#ifdef RV_MQ_TEST

#include <stdlib.h>
#include "rvstdio.h"
#include "rvtimestamp.h"
#include "rvthread.h"

#define DONT_DESTRUCT_THREADS 0

RvThread* RvCreateThread(RvThreadFunc f, void *data) {
	RvThread *th;
	RvStatus s;

	s = RvMemoryAlloc(0, sizeof(RvThread), 0, (void **)&th);
	if(s != RV_OK) {
		return 0;
	}

	s = RvThreadConstruct(f, data, 0, th);
	if(s != RV_OK) goto failure;

	s = RvThreadCreate(th);
	if(s != RV_OK) goto failure;

	s = RvThreadStart(th);
	if(s != RV_OK) goto failure;

		
	return th;
failure:
	RvMemoryFree(th, 0);
	return 0;
}

typedef struct {
	RvThread *self;
	RvInt id;
	RvInt nMsgs;
	HRvMQ mq;
} ProviderData;

void Provider(RvThread *th, void *data) {
	RvInt i;
	ProviderData *pd = (ProviderData *)data;

	RvThreadNanosleep(RV_TIME64_NSECPERSEC, 0);

	for(i = 0; i < pd->nMsgs; i++) {
		RvMQPost(pd->mq, pd->id, i, i * i, 0);
	}

	RvMQPost(pd->mq, 0, 0, 0, 0);
}

typedef struct {
	RvThread *self;
	RvInt id;
	HRvMQ mq;
	RvInt size;
	RvInt *count;
} ConsumerData;

void Consumer(RvThread *th, void *data) {
	ConsumerData *d = (ConsumerData *)data;

	RvThreadNanosleep(RV_TIME64_NSECPERSEC, 0);

	memset(d->count, 0, sizeof(*d->count) * d->size);

	while(1) {
		RvUint32 msgId;
		RvUint32 p1;
		RvUint32 p2;

		RvMQGet(d->mq, &msgId, &p1, &p2, 0);
		RvAssert(msgId <= (RvUint32)d->size);
		d->count[msgId]++;
		if(msgId == 0) break;
	}

}

#define MAX_TARGETS 5
#define MAX_MSG_IDS 5

typedef struct _SMQData {
	HRvSMQ q;
	RvSemaphore *start;
	int    nmsgs;
	int    msgid;
	int    target;
	
} SMQData;

typedef struct _TreatMsgCtx {
	int nmsgs[MAX_TARGETS][MAX_MSG_IDS];
} TreatMsgCtx;

RvInt done = 0;


void TreatMsg(RvSMQMsgTarget id, RvSMQMsgId msg, RvSMQMsgParam p1, RvSMQMsgParam p2, void *ctx) {
	TreatMsgCtx *mctx = (TreatMsgCtx *)ctx;

	if(msg == 0) {
		done++;
		return;
	}
	mctx->nmsgs[id][msg - 1]++;
}


void MsgSender(RvThread *th, void *data) {
	SMQData *d = (SMQData *)data;
	int i;
	int niter;

	RvSemaphoreWait(d->start, 0);
	niter = d->nmsgs / 5;

	for(i = 0; i < niter; i++) {
		int j;

		for(j = 1; j <= 5; j++) {
			RvSMQPost(d->q, d->target, j, 0, 0, 0);
		}
	}

	RvSMQPost(d->q, d->target, 0, 0, 0, 0);
#if DONT_DESTRUCT_THREADS
	while(1) {
	  RvThreadNanosleep(10000000000, 0);
	}
#endif
}

typedef struct _SeliData {
	RvSelectEngine *seli;
	RvSemaphore *start;
	RvInt32 nmsgs;
	RvInt32 target;
} SeliData;

void MsgSenderOld(RvThread *th, void *data) {
	SeliData *d = (SeliData *)data;
	int i;
	int niter;

	RvSemaphoreWait(d->start, 0);
	niter = d->nmsgs / 5;
	
	for(i = 0; i < niter; i++) {
		int j;
		
		for(j = 1; j <= 5; j++) {
			RvSelectStopWaiting(d->seli, (RvUint8)j, 0);
		}
	}
	
	RvSelectStopWaiting(d->seli, 255, 0);

}

typedef struct _PreemptionCtx {
	RvInt msgs[255];
} PreemptionCtx;

void TreatPreemption(RvSelectEngine *seli, RvUint8 msg, void *ctx) {
	PreemptionCtx *pctx = (PreemptionCtx *)ctx;
	static int j = 0;

	if(msg == 255) {
		done++;
		return;
	}

	pctx->msgs[msg]++;
	j++;
	if((j % 100) == 0) {
	  /*printf("Messages %d\n", j); */
	}

}

#define NTARGETS 5
#define NMSGS    7000

RvInt64 testSMQAux() {
	RvSelectEngine *seli;
	HRvSMQ q;
	SMQData tdata[NTARGETS];
	SeliData sdata[NTARGETS];
	TreatMsgCtx tctx;
	int i;
	RvThread *th[NTARGETS];
	RvSemaphore start;
	RvInt64 delta1, delta2;
	PreemptionCtx pctx;
	RvSMQMsgTarget t;
	int nmsgs;

	nmsgs = NMSGS * NTARGETS;

	RvSelectConstruct(10, 10, 0, &seli);
	RvSMQConstruct(&q, seli, 1000, 0);
	memset(tctx.nmsgs, 0, sizeof(tctx.nmsgs));
	RvSemaphoreConstruct(0, 0, &start);
	RvSMQRegisterTarget(q, TreatMsg, (void *)&tctx, &t, 0);
	done = 0;

	for(i = 0; i < NTARGETS; i++) {
		SMQData *cur = &tdata[i];
		cur->q = q;
		cur->nmsgs = NMSGS;
		cur->start = &start;
		cur->target = t;
		th[i] = RvCreateThread(MsgSender, cur);
	}


	delta1 = RvTimestampGet(0);

	for(i = 0; i < NTARGETS; i++) {
		RvSemaphorePost(&start, 0);
	}


	while(done < NTARGETS) {
		RvSelectWaitAndBlock(seli, 1000000000);
	}

	delta1 = RvTimestampGet(0) - delta1;

	for(i = 0; i < NTARGETS; i++) {
#if !DONT_DESTRUCT_THREADS
	  RvThreadDestruct(th[i]); 
#endif
	}

	RvSelectDestruct(seli, 10);

	RvSelectConstruct(10, 10, 0, &seli);

	for(i = 0; i < NTARGETS; i++) {
		SeliData *sur = &sdata[i];
		sur->seli = seli;
		sur->nmsgs = NMSGS;
		sur->start = &start;
		th[i] = RvCreateThread(MsgSenderOld, sur);
	}

	memset(&pctx, 0, sizeof(pctx));
	RvSelectSetPreemptionCb(seli, TreatPreemption, (void *)&pctx);

	delta2 = RvTimestampGet(0);
	done = 0;

	for(i = 0; i < NTARGETS; i++) {
		RvSemaphorePost(&start, 0);
	}
	
	while(done < NTARGETS) {
		RvSelectWaitAndBlock(seli, 1000000000);
	}

	delta2 = RvTimestampGet(0) - delta2;

	for(i = 0; i < NTARGETS; i++) {
#if !DONT_DESTRUCT_THREADS
		RvThreadDestruct(th[i]);
#endif
	}

	RvSelectDestruct(seli, 10);

	printf("Total times:\n");
	printf("1: %g\n2: %g\nd: %g\n", (double)delta1, (double)delta2, (double)(delta1 - delta2));
	printf("Per-message:\n");
	printf("1: %g\n2: %g\nd: %g\n", (double)delta1 / nmsgs, (double)delta2 / nmsgs, (double)(delta1 - delta2)/nmsgs);
	return delta1 - delta2;
}



void testSMQ() {
	RvInt64 res = 0;
	int i;
	double d;
	char *es;

	es = USE_EMPTY_SEMAPHORE ? "With 'Empty' semaphore" : "Without 'Empty' semaphore";
	
	printf("Lock %s, %s\n", RV_LOCK_STRING, es);
	for(i = 0; i < 20; i++) {
		res += testSMQAux();
	}

	d = ((double)res) / (NMSGS * NTARGETS * 20);
	printf("Amortized diff: %g\n\n", d);
}

#include "rvhost.h"

void getaddrs() {
	RvAddress addrs[10];
	RvSize_t naddrs = sizeof(addrs) / sizeof(addrs[0]);

	RvHostLocalGetAddress(0, &naddrs, addrs);

}

int main(int argc, char *argv[]) {
	HRvMQ mq;
	ProviderData *p;
	ConsumerData *c;
	int i;
	int j;
	int totalMsgs = 0;
	int err = 0;
	int nmsgs = 0;
	int ncp = 0;
	int mqsz = 0;

	RvCBaseInit();
	getaddrs();
	testSMQ();
	return 0;

	ncp = atol(argv[1]);
	nmsgs = atol(argv[2]);
	mqsz = atol(argv[3]);

	RvMemoryAlloc(0, ncp * sizeof(*p), 0, (void **)&p);
	RvMemoryAlloc(0, ncp * sizeof(*c), 0, (void **)&c);

	RvMQConstruct(&mq, mqsz, 0);

	for(i = 0; i < ncp; i++) {
		ProviderData *cp = &p[i];
		ConsumerData *cc = &c[i];

		cp->self = RvCreateThread(Provider, (void *)cp);
		cp->id = i + 1;
		cp->mq = mq;
		cp->nMsgs = nmsgs;

		cc->id = i + 1;
		cc->mq = mq;
		cc->self = RvCreateThread(Consumer, (void *)cc);
		cc->size = ncp + 1;
		RvMemoryAlloc(0, cc->size * sizeof(*(cc->count)), 0, (void **)&cc->count);
	}

	for(i = 0; i < ncp; i++) {
		RvThreadDestruct(p[i].self);
	}

	for(i = 0; i < ncp; i++) {
		RvThreadDestruct(c[i].self);
	}

	for(i = 0; i < ncp; i++) {
		for(j = 1; j <= ncp; j++) {
			totalMsgs += c[i].count[j];
		}
	}


	RvCBaseEnd();
	return 0;
}

#endif

#else
int prevent_warning_of_ranlib_has_no_symbols_rvsmq=0;
#endif /* RV_USE_SMQ */

