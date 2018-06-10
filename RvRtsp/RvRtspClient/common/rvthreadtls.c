#include "rvthread.h"

#ifndef RV_THREAD_TLS_STATS
#define RV_THREAD_TLS_STATS 0
#endif

#include "rvthreadtls.h"
#include "rvlock.h"
#include "rvinterfacesdefs.h"

#if RV_THREAD_TLS_TYPE == RV_THREAD_TLS_MANUAL

#ifndef RV_THREAD_MAX_CONCURRENT_THREADS
#define RV_THREAD_MAX_CONCURRENT_THREADS 15
#endif

#define RV_THREAD_ERROR_NTHREADS  (-520)

typedef struct {
    RvThreadId id;
    RvThread   *th;
} RvThreadsHashEntry;

typedef struct {
    RvLock   lock;
    RvInt32  mod1;
    RvInt32  mod2;
    RvInt32  nMaxConcurrentThreads;
    RvInt32  nConcurrentThreads;
    RvThreadsHashEntry *hash;
#if RV_THREAD_TLS_STATS  /* gather statistics information */
    RvLock statsLock;
    RvInt32 nFindRequests;  /* number of find requests */
    RvInt32 nTotalIters;    /* total number of iterations */
#endif

} RvThreadsHash;

static RvThreadsHash sThreadsHash;

#define MCT RV_THREAD_MAX_CONCURRENT_THREADS

#if MCT >= 1024
#error Too many concurrent threads required, maximum of 1023 allowed
#endif

#define HASH_SIZE (MCT < 8 ? 19 : (MCT < 16 ? 41 : (MCT < 32 ? 71 : (MCT < 64 ? 137 : \
(MCT < 128 ? 263 : (MCT < 256 ? 523 : (MCT < 512 ? 1033 : (MCT < 1024 ? 2063 : 0))))))))

static RvThreadsHashEntry shashBuffer[HASH_SIZE];

#ifndef RvThreadsTLSAllocHashTable
#define RvThreadsTLSAllocHashTable(hashSize) (shashBuffer)
#endif

#ifndef RvThreadTLSFreeHashTable
#define RvThreadTLSFreeHashTable(hashTable)
#endif

const RvInt32 sPrimes[] =
{
	17,
	19,

	37,
	41,

	67,
	71,

	131,
	137,

	257,
	263,

	521,
	523,

	1031,
	1033,

	2053,
	2063,

	4099,
	4111,

	8209,
	8219,

	16411,
	16417,

	32771,
	32779,

	65537,
	65539,

	131101,
	131111,

	262147,
	262151,

	524309,
	524341,

	1048583,
	1048589,

	2097169,
	2097211,

	4194319,
	4194329,

	8388617,
	8388619,

	16777259,
	16777289,

	33554467,
	33554473,

	67108879,
	67108913,

	134217757,
	134217773,

	268435459,
	268435463,

	536870923,
	536870951,

	1073741827,
	1073741831,

/*
	2147483659,
	2147483693,
*/

    0,
    0
};

struct dummy {
    char a;
    char b;
};

static RvThread *CELL_EMPTY = 0;
static RvThread *CELL_DELETED = (RvThread *)&((struct dummy *)(0))->b;

#define CELL_IS_EMPTY(cell) (cell->th == CELL_EMPTY)
#define CELL_IS_DELETED(cell) (cell->th == CELL_DELETED)

static 
RvStatus RvThreadsHashConstruct(RvThreadsHash *self, RvInt32 nMaxThreads) {
    RvInt32 pow2 = 1;
    const RvInt32 *curPrime;
    RvSize_t hashSize;
    RvStatus s = RV_OK;
    RvLogMgr *logMgr = 0;

    for(; pow2 < nMaxThreads; pow2 <<= 1)
    ;
    /* Hash table size is > 2 * nMaxThreads */
    pow2 <<= 1;

    for(curPrime = sPrimes; *curPrime < pow2; curPrime += 2)
    ;

    self->mod2 = *curPrime++;
    self->mod1 = *curPrime;
    self->nMaxConcurrentThreads = nMaxThreads;

    hashSize = self->mod1;
    s = RvLockConstruct(logMgr, &self->lock);
    if(s != RV_OK) {
        return s;
    }

#if RV_THREAD_TLS_STATS
    s = RvLockConstruct(logMgr, &self->statsLock);
    if(s != RV_OK) {
        RvLockDestruct(&self->lock, logMgr);
        return s;
    }

    self->nTotalIters = 0;
    self->nFindRequests = 0;
#endif
    


    self->hash = RvThreadsTLSAllocHashTable(hashSize);
    if(self->hash == 0) {
        RvLockDestruct(&self->lock, logMgr);
    }

    memset((void *)self->hash, 0, hashSize);
    self->nConcurrentThreads = 0;
    return s;
}

static
void RvThreadsHashDestruct(RvThreadsHash *self) {
    RvLogMgr *logMgr = 0;

    RvLockDestruct(&self->lock, logMgr);
    RvThreadTLSFreeHashTable(self->hash);
#if RV_THREAD_TLS_STATS
    RvLockDestruct(&self->statsLock, logMgr);
#endif
}

RvStatus RvThreadTLSInit() {
    RvStatus s;

    s = RvThreadsHashConstruct(&sThreadsHash, RV_THREAD_MAX_CONCURRENT_THREADS);
    return s;
}

void RvThreadTLSEnd() {
    RvThreadsHashDestruct(&sThreadsHash);
}


static 
RvInt RvThreadsHashFind(RvThreadsHash *self, RvThreadId id) {
    RvThreadsHashEntry *hash = self->hash;
    RvInt32 step = id % self->mod2;
    RvInt32 hashSize = self->mod1;
    RvInt32 curIdx = id % hashSize;
    RvThreadsHashEntry *curCell; 
    RvInt32 foundIdx = -1;
    RvInt32 niters;

    for(niters = 0; niters < hashSize; niters++) {
        curCell = &hash[curIdx];

        if(CELL_IS_EMPTY(curCell)) {
            if(foundIdx == -1) {
                foundIdx = curIdx;
            }
            break;
        }

        if(CELL_IS_DELETED(curCell)) {
            if(foundIdx == -1) {
                foundIdx = curIdx;
            }
        } else if(curCell->id == id) {
            foundIdx = curIdx;
            break;
        }

        curIdx = (curIdx + step) % hashSize;
    }   
    
#if RV_THREAD_TLS_STATS
    RvLockGet(&self->statsLock, NULL/*logMgr*/);
            /*Don't use Thread's Log Manager in order to prevent recursive
            logging when this function is called from RvLogTextAny() ->
            RvThreadCurrent() -> RvThreadsHashFind() */
    self->nFindRequests++;
    self->nTotalIters += niters + 1;
    RvLockRelease(&self->statsLock, NULL/*logMgr*/);
#endif

    return foundIdx;
}

static RvStatus 
RvThreadsHashAdd(RvThreadsHash *self, RvThreadId id, RvThread *th) {
    RvStatus s = RV_OK;
    RvInt32 curIdx;
    RvLogMgr *logMgr = 0; /* Just in case I'll add logs in the future */

    s = RvLockGet(&self->lock, logMgr);
    if(s != RV_OK) {
        return s;
    }

    if(self->nConcurrentThreads > self->nMaxConcurrentThreads) {
        s = RV_THREAD_ERROR_NTHREADS;
        goto finish;
    }

    curIdx = RvThreadsHashFind(self, id);
    if(curIdx != -1) {
        RvThreadsHashEntry *cell = &self->hash[curIdx];
        cell->id = id;
        cell->th = th;
        self->nConcurrentThreads++;
    } else {
      /* Shouldn't happen - it means that there are more than 2 * RV_THREAD_MAX_CONCURRENT_THREADS
       * concurrent threads
       */
        s = RV_ERROR_UNKNOWN;
    }

finish:
    RvLockRelease(&self->lock, logMgr);
    return s;
}

static RvStatus 
RvThreadsHashRemove(RvThreadsHash *self, RvThreadId id) {
    RvStatus s = RV_OK;
    RvInt32 curIdx;
    RvLogMgr *logMgr = 0; 
 
    s = RvLockGet(&self->lock, logMgr);
    if(s != RV_OK) {
        return s;
    }

    curIdx = RvThreadsHashFind(self, id);

    /* Shouldn't happen - it means that there are more than 2 * RV_THREAD_MAX_CONCURRENT_THREADS
     * concurrent threads
     */
    if(curIdx == -1) {
        s = RV_ERROR_UNKNOWN;
    } else {
        RvThreadsHashEntry *cell = &self->hash[curIdx];
        /* cell wasn't found */
        if(CELL_IS_EMPTY(cell) || CELL_IS_DELETED(cell)) {
            s = RV_ERROR_UNKNOWN;
        } else {
            cell->th = CELL_DELETED;
            self->nConcurrentThreads--;
        }
    }

    RvLockRelease(&self->lock, logMgr);
    return s;
}

#if RV_THREAD_TLS_STATS

static 
void RvThreadsHashGetStats(RvThreadsHash *self, RvThreadTLSStats *stats) {
    RvLockGet(&self->statsLock, 0);
    stats->nFindRequests = self->nFindRequests;
    stats->nTotalIters = self->nTotalIters;
    RvLockRelease(&self->statsLock, 0);
}

RVCOREAPI
void RvThreadTLSGetStats(RvThreadTLSStats *stats) {
    RvThreadsHashGetStats(&sThreadsHash, stats);
}

#else

RVCOREAPI
void RvThreadTLSGetStats(RvThreadTLSStats *stats) {
    stats->nFindRequests = 1;
    stats->nTotalIters = 1;
}

#endif


RvStatus 
RvThreadTLSSetupThreadPtr(RvThreadId id, RvThread *th) {
    return RvThreadsHashAdd(&sThreadsHash, id, th);
}

RvStatus
RvThreadTLSRemoveThreadPtr(RvThreadId id) {
    return RvThreadsHashRemove(&sThreadsHash, id);
}

RvStatus
RvThreadTLSGetThreadPtr(RvThreadId id, RvThread **pth) {
    RvStatus s = RV_OK;
    RvInt32 curIdx;
    RvThreadsHashEntry *cell;

    curIdx = RvThreadsHashFind(&sThreadsHash, id);
    if(curIdx == -1) {
        return RV_ERROR_UNKNOWN;
    }

    cell = &sThreadsHash.hash[curIdx];
    if(CELL_IS_DELETED(cell) || CELL_IS_EMPTY(cell)) {
        return RV_ERROR_UNKNOWN;
    }

    *pth = cell->th;
    return s;
}


#else
int prevent_warning_of_ranlib_has_no_symbols_rvthreadtls=0;
#endif /* #if RV_THREAD_TLS_TYPE == RV_THREAD_TLS_MANUAL */

