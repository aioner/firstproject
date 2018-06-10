#include "ares.h"
#include "rvarescache.h"
#include "rvarescachep.h"
#include "rvtimestamp.h"
#include "rvtypes.h"
#include "rvtime.h"
#include "rvassert.h"
#include "rvares.h"
#include "rvmemory.h"
#include "rvaddress.h"
#include "rvthread.h"
#include "rvhashu.h"
#include "rvassert.h"
#include "rvehd.h"
#include "rvtimer.h"
#include "rvcondvar.h"
#include "rvpqueue.h"
#include <string.h>


#ifdef MULTIINSTANCE_TEST

typedef enum _State {
    Undefined = 0,
    Created,
    //Waited,
    Callbacked,
    //Cancelled,
    Deleted
} State;

typedef void (*HistoryAddCB)(void *ctx, State state, char *descr);

static HistoryAddCB HistoryAdd;

RVCOREAPI
void RvAresCacheSetHistoryCB(HistoryAddCB cb) {
    HistoryAdd = cb;
}

#else

#define HistoryAdd(ctx, state, descr)

#endif


/* Lint-make-happy macros */

/* Lint 818 warning suggests that some parameter may be declared as constant. On the other hand,
 *  this may be in contradiction with the usage of this parameter.  
 */
#define LINT_NOT_CONST(x) /*lint -esym(818, x)*/
#define LINT_CONST(x) /*lint +esym(818, x)*/

#if RV_DNS_USES_CACHING


#if RV_DNS_CACHE_FAKETIME

#if __GNUC__

#warning "You're compiling with faked notion of time for debugging purposes. To change unset RV_DNS_CACHE_FAKETIME"

#else

#pragma message(RvReminder "You're compiling with faked notion of time for debugging purposes. To change unset RV_DNS_CACHE_FAKETIME")

#endif


static RvAresTime gsTime = 0;
#define RvAresGetTime() (gsTime)

RVCOREAPI
void RvAresCacheDSetTime(RvUint32 t) {
    if(gsTime < t) {
        gsTime = t;
    }
}

RVCOREAPI
void RvAresCacheDAdvanceTime(RvUint32 t) {
    gsTime += t;
}

RVCOREAPI
RvUint32 RvAresCacheDGetTime() {
    return RvAresGetTime();
}


#else

/*
#define RvAresGetTime() ((RvAresTime)((RvTimestampGet(0) - gsStartTime) / RV_TIME_NSECPERSEC))
*/

#define RvAresGetTime() ((RvAresTime) (RvUint64ToRvInt32(RvUint64Div(RvUint64Sub(RvTimestampGet(0), gsStartTime), RV_TIME_NSECPERSEC))))

#endif



#define LOGSOURCE_NAME "RCACHE"

#ifndef RV_DNS_CACHE_NUM_CELLS
#define RV_DNS_CACHE_NUM_CELLS 30
#endif


#if !defined(RV_DNS_CACHE_MIN_TTL) || (RV_DNS_CACHE_MIN_TTL <= 0)
#undef RV_DNS_CACHE_MIN_TTL
#define RV_DNS_CACHE_MIN_TTL 1
#endif

typedef struct {
    RvAresTime  expirationTime;
    RvAresPage *head; /* points to the first page in the linked list of pages */
    RvAresPage *tail; /* points to the last page */
} RvAresCacheCell;

typedef struct {
    RvSize_t maxSize;
    RvSize_t curSize;
    RvAresTime minTtl;
    RvAresTime maxTtl;
    RvLogSource *logSrc;
    /* sorted according expiration time array of cells.
    * For the sake of simplicity, there are 2 fake cells:
    * first (with expiration time == current time) and last (with infinite expiration time)
    * Insertion sort is used to keep this array sorted
    */
    RvAresCacheCell *cells;
} RvAresCacheCells;


#define PageIsPoolPage(page) ((page)->pageSize == 0)
#define PageIsFullyExpired(page, curTime) ((page)->maxExpirationTime <= curTime)

#define DefaultPageSize(allocator) ((allocator)->pageSize)

typedef struct {
    RvAresHash        hash;
    RvAresCacheCells  cells;
    RvPQueue          poolExpired;       /* priority queue of expired pool pages */
    RvPQueue          npoolExpired;      /* priority queue of expired non-pool pages */
    RvAresPageAllocator pageAllocator;
    RvLogSource         *logSrc;
} RvAresCache;


/*lint -save -e750*/
#define CACHE_LOG_INFO_0(f)                     RvLogInfo(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME))
#define CACHE_LOG_INFO_1(f, p1)                 RvLogInfo(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1))
#define CACHE_LOG_INFO_2(f, p1, p2)             RvLogInfo(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2))
#define CACHE_LOG_INFO_3(f, p1, p2, p3)         RvLogInfo(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3))
#define CACHE_LOG_INFO_4(f, p1, p2, p3, p4)     RvLogInfo(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4))
#define CACHE_LOG_INFO_5(f, p1, p2, p3, p4, p5) RvLogInfo(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4, p5))
#define CACHE_LOG_INFO_6(f, p1, p2, p3, p4, p5, p6) RvLogInfo(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4, p5, p6))


#define CACHE_LOG_DEBUG_0(f)                     RvLogDebug(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME))
#define CACHE_LOG_DEBUG_1(f, p1)                 RvLogDebug(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1))
#define CACHE_LOG_DEBUG_2(f, p1, p2)             RvLogDebug(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2))
#define CACHE_LOG_DEBUG_3(f, p1, p2, p3)         RvLogDebug(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3))
#define CACHE_LOG_DEBUG_4(f, p1, p2, p3, p4)     RvLogDebug(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4))
#define CACHE_LOG_DEBUG_5(f, p1, p2, p3, p4, p5) RvLogDebug(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4, p5))


#define CACHE_LOG_ENTER_0(f)                RvLogEnter(LOGSRC, (LOGSRC, FUNCTION_NAME " " f))
#define CACHE_LOG_ENTER_1(f, p1)            RvLogEnter(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1))
#define CACHE_LOG_ENTER_2(f, p1, p2)        RvLogEnter(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1, p2))
#define CACHE_LOG_ENTER_3(f, p1, p2, p3)    RvLogEnter(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1, p2, p3))
#define CACHE_LOG_ENTER                     CACHE_LOG_ENTER_0("")


#define CACHE_LOG_LEAVE_0(f)                RvLogLeave(LOGSRC, (LOGSRC, FUNCTION_NAME " " f))
#define CACHE_LOG_LEAVE_1(f, p1)            RvLogLeave(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1))
#define CACHE_LOG_LEAVE_2(f, p1, p2)        RvLogLeave(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1, p2))
#define CACHE_LOG_LEAVE_3(f, p1, p2, p3)    RvLogLeave(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1, p2, p3))
#define CACHE_LOG_LEAVE                     CACHE_LOG_LEAVE_0("")


#define CACHE_LOG_ERROR_0(f)                RvLogError(LOGSRC, (LOGSRC, FUNCTION_NAME " " f))
#define CACHE_LOG_ERROR_1(f, p1)            RvLogError(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1))
#define CACHE_LOG_ERROR_2(f, p1, p2)        RvLogError(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1, p2))
#define CACHE_LOG_ERROR_3(f, p1, p2, p3)    RvLogError(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1, p2, p3))
#define CACHE_LOG_ERROR_4(f, p1, p2, p3, p4)     RvLogError(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4))
#define CACHE_LOG_ERROR_5(f, p1, p2, p3, p4, p5) RvLogError(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4, p5))

#define CACHE_LOG_EXCEP_0(f)                RvLogExcep(LOGSRC, (LOGSRC, FUNCTION_NAME " " f))
#define CACHE_LOG_EXCEP_1(f, p1)            RvLogExcep(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1))
#define CACHE_LOG_EXCEP_2(f, p1, p2)        RvLogExcep(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1, p2))
#define CACHE_LOG_EXCEP_3(f, p1, p2, p3)    RvLogExcep(LOGSRC, (LOGSRC, FUNCTION_NAME " " f, p1, p2, p3))
#define CACHE_LOG_EXCEP_4(f, p1, p2, p3, p4)     RvLogExcep(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4))
#define CACHE_LOG_EXCEP_5(f, p1, p2, p3, p4, p5) RvLogExcep(LOGSRC, (LOGSRC, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4, p5))


static RvUint64 gsStartTime;

static RvAresCacheParams gsCacheParams = {
    RV_DNS_CACHE_PAGE_SIZE,
    RV_DNS_CACHE_MINPAGES,
    RV_DNS_CACHE_MAXPAGES,
    RV_DNS_CACHE_MAXBIGPAGES_SIZE,
    RV_DNS_CACHE_DELTAPAGES,
    RV_DNS_CACHE_HASH_SIZE   
};


RVCOREAPI
void RvAresCacheParamsInit(RvAresCacheParams *params) {
    *params = gsCacheParams;
}

#define CONDSET(param) \
    if(params->param) gsCacheParams.param = params->param

RVCOREAPI
void RvAresCacheSetDefaultParams(RvAresCacheParams *params) {
    CONDSET(pageSize);
    CONDSET(minPages);
    CONDSET(maxPages);
    CONDSET(maxBigPagesSize);
    CONDSET(deltaPages);
    CONDSET(hashSize);
}

#define CACHE_TIME RvAresGetTime()


/* --------------------------- Utilities ----------------------------------*/
#include <ctype.h>


/*RVINLINE*/
static void RvStrToLower2(RvChar *dest, const RvChar *str, RvSize_t size) {
    while(size--) {
        *dest++ = (RvChar)tolower((RvInt)*str);
        str++;
    }

    *dest = 0;
}



/*--------------------------- CacheCells methods ---------------------------*/

static
RvStatus RvAresCacheCellsConstruct(RvAresCacheCells *self, RvLogSource *logSrc) {
    RvStatus s;
    RvLogMgr *logMgr = LogMgrFromLogSrc(logSrc);
    RvSize_t numCells = RV_DNS_CACHE_NUM_CELLS;
    RvAresTime minTTL = RV_DNS_CACHE_MIN_TTL;

    self->logSrc = logSrc;

    self->minTtl = minTTL;
    self->maxTtl = self->minTtl << (numCells - 1);

    numCells += 2;

    self->maxSize = numCells;
    self->curSize = 0;

    s = RvMemoryAlloc(0, numCells * sizeof(*self->cells), logMgr, (void **)&self->cells);
    if(s != RV_OK) {
        self->cells = 0;
    }
    return s;
}



static
void RvAresCacheCellsDestruct(const RvAresCacheCells *self) {
    RvLogMgr *logMgr = LOGMGR;

    if(self->cells) {
        (void)RvMemoryFree(self->cells, logMgr);
    }

}

static
void RvAresCacheCellConstruct(RvAresCacheCell *self, RvAresTime expirationTime) {
    self->expirationTime = expirationTime;
    self->head = 0;
    self->tail = 0;
}

#define RvAbs(x) (((RvInt32)(x) < 0) ? -((RvInt32)(x)) : (x))

static
RvAresTime RvAresCacheFindMedian(RvAresTime lttl, RvAresTime rttl, RvAresTime ttl) {
    RvAresTime newTtl;
    RvAresTime times[7];
    RvSize_t   nTimes = 0;
    RvAresTime foundTime;

    RvInt32 diff;
    RvSize_t i;

    if(lttl > ttl) {
        if(ttl <= (lttl >> 1)) {
            return ttl;
        }
        rttl = lttl;
        lttl = 1;
    } else if(rttl < ttl) {
        if(ttl >= (rttl << 1)) {
            return ttl;
        }
        lttl = rttl;
        while(rttl < ttl) {
            rttl <<= 1;
        }
    } else if( (ttl >= (lttl << 1)) && (ttl <= (rttl >> 1)) ) {
        return ttl;
    }


    times[nTimes++] = lttl;
    times[nTimes++] = rttl;

    newTtl = lttl;

    while(ttl > newTtl) {
        newTtl <<= 1;
    }

    if(newTtl <= (rttl >> 1) && newTtl >= (lttl << 1)) {
        times[nTimes++] = newTtl;
    }

    newTtl >>= 1;

    if(newTtl <= (rttl >> 1) && newTtl >= (lttl << 1)) {
        times[nTimes++] = newTtl;
    }

    newTtl = rttl;

    while(ttl < newTtl) {
        newTtl >>= 1;
    }

    if(newTtl <= (rttl >> 1) && newTtl >= (lttl << 1)) {
        times[nTimes++] = newTtl;
    }

    newTtl <<= 1;


    if(newTtl <= (rttl >> 1) && newTtl >= (lttl << 1)) {
        times[nTimes++] = newTtl;
    }

    diff = (RvInt32)(ttl - times[0]);
    diff = RvAbs(diff);
    foundTime = times[0];

    for(i = 1; i < nTimes; i++) {
        RvInt32 curDiff = (RvInt32)(ttl - times[i]);
        curDiff = RvAbs(curDiff);
        if(curDiff < diff) {
            diff = curDiff;
            foundTime = times[i];
        }
    }

    return foundTime;
}


static
RvAresCacheCell* RvAresCacheCellsFindCell(RvAresCacheCells *self, 
                                          RvAresTime curTime,
                                          RvAresTime ttl, 
                                          RvAresTime *pExpirationTime) {
    RvSize_t idx, lidx, ridx;
    RvSize_t dist;
    RvSize_t foundIdx = 0;
    RvAresTime expirationTime, lttl, rttl, newTtl;
    RvAresCacheCell *cell, *lcell, *rcell;
    RvAresCacheCell *cells;
    RvSize_t nCells;


    /* TTL is too low, return */
    if(ttl < self->minTtl) {
        return 0;
    }

    if(ttl > self->maxTtl) {
        ttl = self->maxTtl;
    }

    nCells = self->curSize;
    *pExpirationTime = expirationTime = curTime + ttl;
    cells = self->cells;


    if(nCells == 0) {
        self->curSize++;
        rcell = &cells[0];
        RvAresCacheCellConstruct(rcell, curTime + ttl);
        return rcell;
    }



    lidx = 0;
    ridx = self->curSize - 1;

    while((dist = (ridx - lidx)) > 1) {
        RvAresTime cellTime;

        idx = lidx + (dist >> 1);
        cellTime = cells[idx].expirationTime;
        if(cellTime < expirationTime) {
            lidx = idx;
            continue;
        }

        if(cellTime > expirationTime) {
            ridx = idx;
            continue;
        }

        foundIdx = idx;
        break;
    }

    if(foundIdx) {
        return &cells[foundIdx];
    }


    lcell = &cells[lidx];
    lttl =  lcell->expirationTime - curTime;
    rcell = &cells[ridx];
    rttl = rcell->expirationTime - curTime;

    newTtl = RvAresCacheFindMedian(lttl, rttl, ttl);

    if(newTtl == lttl) {
        return lcell;
    }

    if(newTtl == rttl) {
        return rcell;
    }

    /*lint -e(506)*/
    RvAssert(self->curSize < self->maxSize);

    if(newTtl < lttl) {
        idx = 0;
    } else if(newTtl < rttl) {
        idx = ridx;
    } else {
        idx = self->curSize;
    }


    cell = cells + idx;
    /* Move cells array right one cell */
    memmove((void *)(cells + idx + 1), (cells + idx), sizeof(*cell) * (self->curSize - idx));
    self->curSize++;
    RvAresCacheCellConstruct(cell, curTime + newTtl);
    return cell;
}

/* Remove first expired cell from the 'cells' array */
static
RvBool RvAresCacheCellsRemove(RvAresCacheCells *self,
                              RvAresTime expirationTime,
                              RvAresPage **ppage) {

    RvAresCacheCell *curCell;

    if(self->curSize == 0) {
        return RV_FALSE;
    }

    curCell = &self->cells[0];

    /* No expired cells */
    if(curCell->expirationTime > expirationTime) {
        return RV_FALSE;
    }

    *ppage = curCell->head;
    memmove(curCell, curCell + 1, (self->curSize - 1) * sizeof(*curCell));
    self->curSize--;
    return RV_TRUE;
}

/* Finds victim page to remove. Currently algorithm is very simple - just removes 
 *   the first small page (page from the pool). 
 */
static
RvAresPage* RvAresCacheCellsFindVictim(const RvAresCacheCells *self) {
    RvAresCacheCell *curCell;
    RvSize_t i;
    RvAresPage *victim = 0;
    RvAresPage *prevPage = 0;

    if(self->curSize == 0) {
        return 0;
    }

    for(i = 0, curCell = self->cells; i < self->curSize && victim == 0; i++, curCell++) {
        for(prevPage = 0, victim = curCell->head; victim != 0; prevPage = victim, victim = victim->next) {
            if(PageIsPoolPage(victim) && victim->lockCnt <= 1) {
                break;
            }
        }

        if(victim != 0) {
            break;
        }
    }

    if(victim == 0) {
        return 0;
    }

    if(prevPage != 0) {
        prevPage->next = victim->next;
    } else {
        curCell->head = victim->next;
    }

    if(victim->next == 0) {
        curCell->tail = prevPage;
    }

    return victim;
}

#define RvAresCacheCellGetPage(cell) ((cell)->tail)

static
void  RvAresCacheCellAddPage(RvAresCacheCell *cell, RvAresPage *page) {
    /* Pool pages are added at the tail, but non-pool pages are added at the head of list */
    if(PageIsPoolPage(page)) {
        if(cell->tail) {
            cell->tail->next = page;
        } else {
            cell->head = page;
        }

        cell->tail = page;
    } else {
        page->next = cell->head;
        cell->head = page;
        if(cell->tail == 0) {
            cell->tail = page;
        }
    }

    page->lockCnt = 1;
}


/*--------------------------- Page methods ---------------------------*/

static
void RvAresPageReset(RvAresPage *page) {
    RvUint8 *lastByte = page->freeSpace + page->spaceLeft;
    RvSize_t pageSize = (RvSize_t)(lastByte - (RvUint8 *)page);

    page->firstRecord = 0;
    page->next = 0;
    page->maxExpirationTime = 0;
    page->spaceLeft = pageSize - sizeof(*page);
    page->freeSpace = (RvUint8 *)page + sizeof(*page);
    page->lockCnt = 0;
}


static
RvStatus RvAresPageAddRdata(RvAresPage *page, RvRdata **pRdata) {
    RvUint8 *pdata;
    RvUint8 *pend;
    RvRdata *rdata = *pRdata;

    pdata = (RvUint8*)RvAlign(page->freeSpace);	
    pend = page->freeSpace + page->spaceLeft;

    if(pdata + rdata->size > pend) {
        return RV_ERROR_INSUFFICIENT_BUFFER;
    }

    memcpy(pdata, rdata, rdata->size);
    rdata = (RvRdata *)(void *)pdata;
    page->freeSpace = pdata + rdata->size;
    page->spaceLeft = (RvSize_t)(pend - page->freeSpace);
    rdata->nextPageEntry = page->firstRecord;
    rdata->page = page;
    if(rdata->expirationTime > page->maxExpirationTime) {
        page->maxExpirationTime = rdata->expirationTime;
    }
    *pRdata = page->firstRecord = rdata;
    return RV_OK;
}


/* RvAresCacheCtx functions */

static
void RvAresCacheCtxConstruct(RvAresCacheCtx *ctx, RvLogSource *logSrc) {
    ctx->logSrc = logSrc;
    ctx->rdata = 0;
    ctx->cur = ctx->start = ctx->u.buf;
    ctx->end   = ctx->u.buf + sizeof(ctx->u.buf);
    ctx->minTTL = 0xffffffff;
    ctx->type  = 0;
}

static
RvStatus RvAresCacheCtxEnlargeBuf(RvAresCacheCtx *self) {
    RvSize_t bufSize = self->end - self->start;
    RvSize_t newBufSize = bufSize << 1;
    RvSize_t dataSize;
    RvUint8 *newBuf;
    RvStatus s;
    RvLogMgr *logMgr = LOGMGR;

    s = RvMemoryAlloc(0, newBufSize, logMgr, (void **)&newBuf);
    if(s != RV_OK) {
        return s;
    }

    dataSize = self->cur - self->start;
    memcpy(newBuf, self->start, dataSize);
    if(self->start != self->u.buf) {
        RvMemoryFree(self->start, logMgr);
    }
    self->start = newBuf;
    self->cur = self->start + dataSize;
    self->end = self->start + newBufSize;
    self->rdata = (RvRdata *)self->start;
    return RV_OK;
}

static
void RvAresCacheCtxDestruct(RvAresCacheCtx *self) {
    if(self->start != self->u.buf) {
        RvMemoryFree(self->start, LOGMGR);
    }
}

#if 0
typedef struct {
    void  *(*memalloc)(RvSize_t size, void *data);  /* Allocate memory for heap. Return pointer to memory, NULL = failed. */
    void   (*memfree)(void *ptr, void *data);       /* Free memory allocated by memalloc. */
    RvBool (*itemcmp)(void *ptr1, void *ptr2);      /* Compare 2 items, return RV_TRUE if ptr1 higher priority, else RV_FALSE. */
    void   (*newindex)(void *item, RvSize_t index); /* Store new index for item, use this for removing items. */
    void *memallocdata;    /* User data parameter passed into memalloc */
    void *memfreedata;     /* User data parameter passed into memfree */
} RvPQueueFuncs;
#endif

static 
void* pqalloc(RvSize_t size, void *data) {
    RvStatus s;
    void *pmem;

    (void)data;

    s = RvMemoryAlloc(0, size, 0, &pmem);
    if(s != RV_OK) {
        return 0;
    }

    return pmem;
}

static
void pqfree(void *ptr, void *data) {
    (void)data;
    (void)RvMemoryFree(ptr, 0);
}

static 
RvBool pqcmp(void *ptr1, void *ptr2) {
    RvAresPage *p1 = (RvAresPage *)ptr1;
    RvAresPage *p2 = (RvAresPage *)ptr2;

    return p1->maxExpirationTime <= p2->maxExpirationTime;
}

static
void pqnewindex(void *item, RvSize_t idx) {
    (void)item, (void)idx;
}

static RvPQueueFuncs pqfuncs = {
    pqalloc,
    pqfree,
    pqcmp,
    pqnewindex,
    0,
    0
};

/*lint -save -e801 GOTO is OK here*/ 
static
RvStatus RvAresCacheConstruct(RvAresCache *self, RvLogSource *logSrc, const RvAresCacheParams *params) {
#define FUNCTION_NAME "RvAresCacheConstruct"
#define ALLOCATOR_CONSTRUCTED 1
#define HASH_CONSTRUCTED      2
#define CELLS_CONSTRUCTED     4
#define PE_CONSTRUCTED        8
#define NPE_CONSTRUCTED       16

    RvStatus s;
    RvUint32 stage = 0;
    RvSize_t pageSize;
    RvSize_t nMinPages;
    RvSize_t nMaxPages;
    RvSize_t nDeltaPages;
    RvSize_t hashSize;
    RvSize_t maxBigPagesSize;

    pageSize = params->pageSize;
    self->logSrc = logSrc;

    if(pageSize == 0) {
        pageSize = RV_DNS_CACHE_PAGE_SIZE;
    } else if(pageSize < RV_DNS_CACHE_MIN_PAGE_SIZE) {
        pageSize = RV_DNS_CACHE_MIN_PAGE_SIZE;
    }

    nMinPages = params->minPages;
    nMaxPages = params->maxPages;
    nDeltaPages = params->deltaPages;

    if(nMinPages == 0 && nMaxPages != 0) {
        nMinPages = nMaxPages;
    } else if(nMinPages != 0 && nMaxPages == 0) {
        nMaxPages = nMinPages;
    } else if(nMinPages == 0 && nMaxPages == 0) {
        nMinPages = RV_DNS_CACHE_MINPAGES;
        nMaxPages = RV_DNS_CACHE_MAXPAGES;
    }

    if(nMaxPages < nMinPages) {
        s = RV_ERROR_BADPARAM;
        goto failure;
    }

    if(nDeltaPages == 0) {
        nDeltaPages = RV_DNS_CACHE_DELTAPAGES;
    }

    if(nDeltaPages > nMinPages) {
        nDeltaPages = nMinPages;
    }

    hashSize = params->hashSize;
    if(hashSize == 0) {
        hashSize = RV_DNS_CACHE_HASH_SIZE;
    }
    hashSize = RvHashNextPrime(hashSize);

 
    s = RvAresCacheCellsConstruct(&self->cells, logSrc);
    if(s != RV_OK) goto failure;
    stage |= CELLS_CONSTRUCTED;

    maxBigPagesSize = params->maxBigPagesSize;
    if(maxBigPagesSize == 0) {
        maxBigPagesSize = RV_DNS_CACHE_MAXBIGPAGES_SIZE;
    }

    CACHE_LOG_INFO_6("Constructing cache with: minPages=%d, maxPages=%d, deltaPages=%d, pageSize=%d, hashSize=%d, maxBigPagesSize=%d",
        nMinPages, nMaxPages, nDeltaPages, pageSize, hashSize, maxBigPagesSize);

    s = RvAresPageAllocatorConstruct(&self->pageAllocator, 
        pageSize, nMinPages, nMaxPages, nDeltaPages, maxBigPagesSize, logSrc);
    if(s != RV_OK) goto failure;
    stage |= ALLOCATOR_CONSTRUCTED;
    s = RvAresHashConstruct(&self->hash, logSrc, hashSize);
    if(s != RV_OK) goto failure;
    stage |= HASH_CONSTRUCTED;

    if(RvPQueueConstruct(RV_PQUEUE_TYPE_DYNAMIC, 16, &pqfuncs, &self->poolExpired) == 0) goto failure;
    stage |= PE_CONSTRUCTED;

    if(RvPQueueConstruct(RV_PQUEUE_TYPE_DYNAMIC, 16, &pqfuncs, &self->npoolExpired) == 0) goto failure;
    stage |= NPE_CONSTRUCTED;

    return s;
failure:

    if(stage & CELLS_CONSTRUCTED) {
        RvAresCacheCellsDestruct(&self->cells);
    }

    if(stage & ALLOCATOR_CONSTRUCTED) {
        RvAresPageAllocatorDestruct(&self->pageAllocator);
    }

    if(stage & HASH_CONSTRUCTED) {
        RvAresHashDestruct(&self->hash);
    }

    if(stage & PE_CONSTRUCTED) {
        RvPQueueDestruct(&self->poolExpired);
    }

    if(stage & NPE_CONSTRUCTED) {
        RvPQueueDestruct(&self->npoolExpired);
    }


    return s;

#undef ALLOCATOR_CONSTRUCTED
#undef HASH_CONSTRUCTED
#undef CELLS_CONSTRUCTED
#undef FUNCTION_NAME
}

/*lint -restore*/

static
void RvAresCacheDestruct(RvAresCache *self) {
    RvAresHashDestruct(&self->hash);
    RvAresPageAllocatorDestruct(&self->pageAllocator);
    RvAresCacheCellsDestruct(&self->cells);
    RvPQueueDestruct(&self->poolExpired);
    RvPQueueDestruct(&self->npoolExpired);
}

static
void RvAresCacheClearCells(RvAresCache *self) {
#define FUNCTION_NAME "RvAresCacheClearCells"
    RvSize_t i;
    RvAresCacheCells *cells = &self->cells;


    for(i = 0; i < cells->curSize; i++) {
        RvAresPage *curPage;
        RvAresPage *nextPage;
        RvAresCacheCell *curCell = &self->cells.cells[i];

        for(curPage = curCell->head; curPage != 0; curPage = nextPage) {
            nextPage = curPage->next;
            curPage->lockCnt--;

            if(curPage->lockCnt == 0) {
                RvAresPageAllocatorFree(&self->pageAllocator, curPage);
                CACHE_LOG_DEBUG_1("page=%p freed", curPage);
            } else if(curPage->lockCnt > 0) {
                CACHE_LOG_DEBUG_1("page=%p is locked", curPage);
            } else {
                CACHE_LOG_EXCEP_2("page=%p has negative lockCnt=%d", curPage, curPage->lockCnt);
            }
        }
    }

    cells->curSize = 0;
#undef FUNCTION_NAME
}


static
void RvAresCacheClearExpiredPQueue(RvAresCache *self, RvPQueue *pq) {
#define FUNCTION_NAME "RvAresCacheClearExpiredPQueue"

    RvAresPage *page;


    while((page = (RvAresPage *)RvPQueueGet(pq)) != 0) {
        RvInt lockCnt = --page->lockCnt;

        if(lockCnt == 0) {
            RvAresPageAllocatorFree(&self->pageAllocator, page);
            CACHE_LOG_DEBUG_1("page=%p freed", page);
        } else if(lockCnt > 0) {
            CACHE_LOG_DEBUG_1("page=%p is locked", page);
        } else {
            CACHE_LOG_EXCEP_2("page=%p has negative lockCnt=%d", page, lockCnt);
        }
    }
#undef FUNCTION_NAME
}

static
void RvAresCacheClear(RvAresCache *self) {
    RvAresCacheClearExpiredPQueue(self, &self->poolExpired);
    RvAresCacheClearExpiredPQueue(self, &self->npoolExpired);
    RvAresHashClear(&self->hash);
    RvAresCacheClearCells(self);
}

static
RvBool RvAresCacheAddExpiredPages(RvAresCache *self, RvAresPage *expired) {
    while(expired) {
        RvPQueue *pq = PageIsPoolPage(expired) ? &self->poolExpired : &self->npoolExpired;
        if(!RvPQueuePut(pq, expired)) {
            return RV_FALSE;
        }
        expired = expired->next;
    }

    return RV_TRUE;
}

static 
void RvAresCachePurgeExpiredPagesPQ(RvAresCache *self, RvPQueue *pq, RvAresTime curTime) {
    RvAresPage *page;
    RvAresPage *locked = 0;

    while((page = (RvAresPage *)RvPQueuePeek(pq)) != 0) {
        if(!PageIsFullyExpired(page, curTime)) {
            break;
        }

        (void)RvPQueueGet(pq);
        if(page->lockCnt > 1) {
            /* locked pages shouldn't be purged, so we keep it on linked list and later 
             * return back to pq
             */
            page->next = locked;
            locked = page;
            continue;
        }
        RvAresHashRemovePage(&self->hash, page);
        RvAresPageAllocatorFree(&self->pageAllocator, page);
    }

    /* return back locked pages */
    while(locked != 0) {
        RvPQueuePut(pq, locked);
        locked = locked->next;
    }
}

static
RvAresPage* RvAresPQGetFirstUnLockedPage(RvPQueue *self) {
    RvAresPage *locked = 0;
    RvAresPage *page = 0;

    while((page = RvPQueueGet(self)) != 0 && page->lockCnt > 1) {
        page->next = locked;
        locked = page;
    }

    while(locked != 0) {
        RvPQueuePut(self, locked);
        locked = locked->next;
    }

    return page;
}

static
void RvAresCacheBookkeeping(RvAresCache *self, RvAresTime curTime) {
    RvAresPage *expired = 0;

    /* Add new expired pages to the queue of expired pages
    * This queue is organized according to expiration time
    */
    while(RvAresCacheCellsRemove(&self->cells, curTime, &expired)) {
        /* In case of OUT_OF_RESOURCES condition there may be
        * cells with no pages
        */
        if(expired == 0) {
            continue;
        }

        RvAresCacheAddExpiredPages(self, expired);
    }

    RvAresCachePurgeExpiredPagesPQ(self, &self->poolExpired, curTime);
    RvAresCachePurgeExpiredPagesPQ(self, &self->npoolExpired, curTime);
}

static
RvAresPage* RvAresCacheAllocPage(RvAresCache *self, RvSize_t pageSize) {
#define FUNCTION_NAME "RvAresCacheAllocPage"

    RvAresPage *page;

    /* First, try to allocate page using allocator */
    page = RvAresPageAllocatorAlloc(&self->pageAllocator, pageSize);
    if(page != 0) {
        CACHE_LOG_DEBUG_1("Fresh page=%p allocated", page);
        return page;
    }

    if(pageSize > DefaultPageSize(&self->pageAllocator)) {
        /* If we need a big page - quit */
        return 0;
    }

    /* Get partially expired page */
    page = RvAresPQGetFirstUnLockedPage(&self->poolExpired);

    if(page != 0) {
        CACHE_LOG_DEBUG_2("Partially expired page=%p allocated: expirationTime=%d", page, page->maxExpirationTime);
    } else {
        /* No expired pages left, find victim from cells */
        page = RvAresCacheCellsFindVictim(&self->cells);
        if(page == 0) {
            return 0;
        }

        CACHE_LOG_DEBUG_2("Victim page=%p allocated, lockCnt=%d", page, page->lockCnt);


    }

    /* Remove all entries on this page from hash */
    RvAresHashRemovePage(&self->hash, page);
    RvAresPageReset(page);
    return page;
#undef FUNCTION_NAME
}



/* Perform cache lookup for <inName, type> tuple
 * Returns:
 *  0 - if nothing appropriate exists in the cache
 *  pointer to appropriate cached record otherwise. This record may contain real data
 *  or negative caching information
 */
static
RvRdata* RvAresCacheFind(RvAresCache *self,
                         RvAresTime curAtime,
                         RvUint32 type,
                         const RvChar *inName,
                         RvSize_t nameSize,
                         RvAresHashInsertPoint *insertPoint) {

#define FUNCTION_NAME "RvAresCacheFind"

    RvRdata *curRec;
    RvAresHash *hash = &self->hash;
    RvChar nameBuf[RV_DNS_MAX_NAME_LEN + 1];
    RvChar *name = nameBuf;
    RvChar *last = name + nameSize - 1;

    RvStrToLower2(name, inName, nameSize);

    if(*last == '.') {
        *last = '\0';
        nameSize--;
    }

    *(last + 1) = 0;

    /* Lookup this name in hash according to type */
    CACHE_LOG_DEBUG_2("Trying <type=%d, name=%s>", type, nameBuf);

    curRec = RvAresHashFind(hash, curAtime, type, name, nameSize, insertPoint);

    if(curRec == 0) {
        /* If name doesn't exists, try to find appropriate NXDOMAIN record */

        CACHE_LOG_DEBUG_1("Doesn't exist, trying <NXDOMAIN, name=%s> ", nameBuf);
        curRec = RvAresHashFind(hash, curAtime, T_NXDOMAIN, name, nameSize, 0);
    }

    /* If we're looking for CNAME record, just return 0 */
    if(type == T_CNAME) {
        return curRec;
    }

    /* This <name, type> tuple doesn't exist, but 'name' still
     *  may be a start for chain of CNAME records that eventually
     *  will end up with appropriate record
     */
    while(curRec == 0) {
        RvChar *alias;
        RvUint8 *pCNAME;

        /* Try to find appropriate CNAME record */
        CACHE_LOG_DEBUG_1("Doesn't exist, trying <type=CNAME, name=%s>", name);
        curRec = RvAresHashFind(hash, curAtime, T_CNAME, name, nameSize, 0);

        if(curRec == 0) {
            CACHE_LOG_DEBUG_1("Doesn't exist <type=CNAME, name=%s>, quitting", name);
            return 0;
        }

        /* Check whether this is negative cache record */
        if(curRec->negative) {
            CACHE_LOG_DEBUG_1("Found negative cache record for <type=CNAME, name=%s>", name);
            return curRec;
        }


        pCNAME = &curRec->nameSize + curRec->nameSize + 2;
        nameSize = *pCNAME;
        alias = (RvChar *)(pCNAME + 1);
        /*
        RvStrToLower2(name, alias, nameSize);
        */
        name = alias;

        CACHE_LOG_DEBUG_2("Found CNAME record, trying <type=%d, name=%s>", type, name);
        curRec = RvAresHashFind(hash, curAtime, type, name, nameSize, 0);
        if(curRec == 0) {
            /* Try to find NXDOMAIN record */
            CACHE_LOG_DEBUG_1("Doesn't exist, trying <type=NXDOMAIN, name=%s>", nameBuf);
            curRec = RvAresHashFind(hash, curAtime, T_NXDOMAIN, name, nameSize, 0);
        }
    }

    CACHE_LOG_DEBUG_2("Found <type=%d, name=%s>", curRec->type, name);
    return curRec;

#undef FUNCTION_NAME
}



/* Serialization function
 *
 * We're keeping record-specific data (SRV, NAPTR, A, AAAA, CNAME) just after 'owner' name field
 *  of RvRdata structure
 *
 * Each field is individually aligned according to it's type
 * Strings are stored in the following format
 *
 * 1 - byte 'size' field'
 * 'size' - bytes of string value
 * 1 - byte '\0'
 *
 * So, string representation requires 'size' + 2 bytes.
 */

/* Format of serialized A extension record:
 * 4 - bytes wide IP value
 */
static
RvStatus RvAresInternA(RvAresCacheCtx *self, const RvDnsData *record) {
    const RvAddressIpv4 *paddr;
    RvUint8 *pRec; /* points to the current position in extension record */

    pRec = (RvUint8 *)RvAlign32(self->cur);
    if(pRec + 4 > self->end) {
        RvStatus s;
        s = RvAresCacheCtxEnlargeBuf(self);
        if(s != RV_OK) {
            return s;
        }

        pRec = (RvUint8 *)RvAlign32(self->cur);
    }

    paddr = RvAddressGetIpv4(&record->data.hostAddress);
    *(RvUint32 *)(void *)pRec = RvAddressIpv4GetIp(paddr);
    self->cur = pRec + 4;
    return RV_OK;
}

#if RV_NET_TYPE & RV_NET_IPV6

/* Format of serialized AAAA extension record
 * 16 - bytes wide IP
 */
RvStatus RvAresInternAAAA(RvAresCacheCtx *self, const RvDnsData *record) {

    const RvAddressIpv6 *paddr = RvAddressGetIpv6(&record->data.hostAddress);

    if(self->cur + 16 > self->end) {
        RvStatus s;

        s = RvAresCacheCtxEnlargeBuf(self);
        if(s != RV_OK) {
            return s;
        }
    }

    memcpy(self->cur, RvAddressIpv6GetIp(paddr), 16);
    self->cur += 16;
    return RV_OK;
}

#endif

/* Format of CNAME
 * 1 - byte wide size followed by
 * 'size' + 1 - bytes zero-terminated string
 */
static
RvStatus RvAresInternCNAME(RvAresCacheCtx *self, RvDnsData *record) {
    RvChar *alias = record->data.dnsCnameData.alias;
    RvSize_t aliasSize = strlen(alias);

    if(aliasSize >= 256) {
      return RV_ERROR_BADPARAM;
    }

    if(self->cur + aliasSize + 2 > self->end) {
        RvStatus s;

        s = RvAresCacheCtxEnlargeBuf(self);
        if(s != RV_OK) {
            return s;
        }
    }

    *self->cur++ = (RvUint8)aliasSize;
    memcpy(self->cur, alias, aliasSize);
    self->cur += aliasSize;
    *self->cur++ = 0;
    return RV_OK;
}

/* Format of SRV extension record
 * 2 - bytes port
 * 2 - bytes priority
 * 2 - bytes weight
 * 1 - byte  targetNameSize
 * targetNameSize - byte targetName
 *
 */

static
RvStatus RvAresInternSRV(RvAresCacheCtx *self, RvDnsData *record) {
    RvSize_t targetNameSize;
    RvDnsSrvData *srv = &record->data.dnsSrvData;
    RvUint8 *pRec;

    pRec = (RvUint8 *)RvAlign16(self->cur);
    targetNameSize = strlen(srv->targetName);
    if(targetNameSize >= 256) {
      return RV_ERROR_BADPARAM;
    }

    if((pRec + 6 + 1 + targetNameSize + 1) > self->end) {
        RvStatus s;

        s = RvAresCacheCtxEnlargeBuf(self);
        if(s != RV_OK) {
            return s;
        }
        pRec = (RvUint8 *)RvAlign16(self->cur);
    }

    *(RvUint16 *)(void *)pRec = srv->port;
    pRec += 2;
    *(RvUint16 *)(void *)pRec = srv->priority;
    pRec += 2;
    *(RvUint16 *)(void *)pRec = srv->weight;
    pRec += 2;

    *pRec++ = (RvUint8)targetNameSize;
    memcpy(pRec, srv->targetName, targetNameSize);
    pRec += targetNameSize;
    *pRec++ = 0;
    self->cur = pRec;

    return RV_OK;
}


/* Format of NAPTR extension record
    RvUint16 order;
    RvUint16 preference;
    RvUint8  flagsSize;
    RvChar   flags[flagsSize + 1];
    RvUint8  servicesSize;
    RvChar   services[servicesSize + 1];
    RvUint8  regexpSize;
    RvChar   regexp[regexpSize + 1];
    RvUint8  replacementSize;
    RvChar   replacement[replacementSize + 1];
*/


static
RvStatus RvAresInternNAPTR(RvAresCacheCtx *self, RvDnsData *record) {
    RvSize_t flagsSize;
    RvSize_t servicesSize;
    RvSize_t regexpSize;
    RvSize_t replacementSize;
    RvSize_t totalSize;
    RvDnsNaptrData *naptr;
    RvUint8 *pRec;

    naptr = &record->data.dnsNaptrData;

    flagsSize       = strlen(naptr->flags);
    servicesSize    = strlen(naptr->service);
    regexpSize      = strlen(naptr->regexp);
    replacementSize = strlen(naptr->replacement);

    /* String size shouldn't exceed 255 chars */
    if(flagsSize >= 256 || servicesSize >= 256 || regexpSize >= 256 || replacementSize >= 256) {
        return RV_ERROR_BADPARAM;
    }

    totalSize = flagsSize + 2 + servicesSize + 2 + regexpSize + 2 + replacementSize + 2 + 4;
    pRec = (RvUint8 *)RvAlign16(self->cur);

    if(pRec + totalSize > self->end) {
        RvStatus s;

        s = RvAresCacheCtxEnlargeBuf(self);
        if(s != RV_OK) {
            return s;
        }
        pRec = (RvUint8 *)RvAlign16(self->cur);
    }

    *(RvUint16 *)(void *)pRec = naptr->order;
    pRec += 2;

    *(RvUint16 *)(void *)pRec = naptr->preference;
    pRec += 2;

    *pRec++ = (RvUint8)flagsSize;
    strcpy((RvChar *)pRec, naptr->flags);
    pRec += flagsSize + 1;

    *pRec++ = (RvUint8)servicesSize;
    strcpy((RvChar *)pRec, naptr->service);
    pRec += servicesSize + 1;

    *pRec++ = (RvUint8)regexpSize;
    strcpy((RvChar *)pRec, naptr->regexp);
    pRec += regexpSize + 1;

    *pRec++ = (RvUint8)replacementSize;
    strcpy((RvChar *)pRec, naptr->replacement);
    pRec += replacementSize + 1;

    self->cur = pRec;
    return RV_OK;

}


static
RvStatus RvAresInternStart(RvAresCacheCtx *self, const RvDnsData *record) {
#define FUNCTION_NAME "RvAresInternStart"

    RvRdata *rdata;
    RvSize_t nameSize = strlen(record->ownerName);
    RvUint8 *pRec;
    RvInt    dataType;

    if(nameSize >= 256) {
        return RV_ERROR_BADPARAM;
    }

    pRec = self->cur;	

    if(pRec + sizeof(RvRdata) + nameSize > self->end) {
        RvStatus s;

        s = RvAresCacheCtxEnlargeBuf(self);
        if(s != RV_OK) {
            return s;
        }
        pRec = self->cur;	
    }


    self->minTTL = record->ttl;
    rdata = (RvRdata *)(void *)pRec;
    rdata->negative = RV_FALSE;

    dataType = (RvInt)record->dataType;
    if(dataType > 0 && dataType < (RvInt)RV_DNS_STATUS_TYPE) {
        /* Regular record, it's type is given by 'dataType' field */
       rdata->type = (RvUint16)dataType;
    } else if(dataType == (RvInt)RV_DNS_STATUS_TYPE) {
        /* Probably, negative caching */
        RvStatus rs = record->data.status;

        if(rs == ARES_ENOTFOUND) {
            /* NXDOMAIN response */
            rdata->type = T_NXDOMAIN;
        } else if(rs == ARES_ENODATA) {
            rdata->type = (RvUint16)record->queryType;
            rdata->negative = RV_TRUE;
        } else {
            CACHE_LOG_DEBUG_1("Unexpected status type: %d", rs);
            return RV_ERROR_BADPARAM;
        }

    } else {
        CACHE_LOG_DEBUG_1("Unexpected data type: %d", dataType);
    }

    self->type = rdata->type;

    rdata->nRecords = 0;
    rdata->nameSize = (RvUint8)nameSize;
    pRec = (RvUint8 *)&rdata->name[0];
    strcpy((RvChar *)pRec, record->ownerName);
    /*lint -e{662} */
    pRec += nameSize + 1;
    self->cur = pRec;
    self->rdata = rdata;
    return RV_OK;

#undef FUNCTION_NAME
}


static
RvStatus RvAresCacheFinishCaching(RvAresCache *self, RvAresTime curAtime, const RvAresCacheCtx *ctx) {
#undef FUNCTION_NAME
#define FUNCTION_NAME "RvAresCacheFinishCaching"

    RvAresHashInsertPoint insertPoint;
    RvRdata *rec = ctx->rdata;
    RvRdata *oldRec;
    /*RvAresHash *hash = &self->hash;*/
    RvStatus s;
    RvAresPage *page = 0;
    RvChar *name; 
    RvSize_t nameSize; 
    RvAresCacheCell *cell;
    RvUint32 ttl;

    if(rec == 0) {
        /* No record to cache */
        return RV_OK;
    }

    name = rec->name;
    nameSize = rec->nameSize;

    RvAresCacheBookkeeping(self, curAtime);
    ttl = ctx->minTTL;
    CACHE_LOG_DEBUG_4("%s caching recordset owner = %s, type = %d, ttl = %d", (rec->negative ? "Negatively" : "Positively"), rec->name, rec->type, ttl);

    /* TTL == 0 - just return. We still want to see DEBUG log line above */
    if(ttl == 0) {
        return RV_OK;
    }

    oldRec = RvAresCacheFind(self, curAtime, rec->type, name, nameSize, &insertPoint);

    /* Record with this <name,type> already exists in the data base */
    if(oldRec) {
        CACHE_LOG_DEBUG_0("Found old recordset - done");
        return RV_OK;
        /* Removing old recordset leads to patological behavior when multiple parallel queries
         * for the same name are performed. Then we constantly remove "old record" and due to 
         * the structure of memory allocation (e.g, we don't remove physically these records)
         * a lot of memory may be wasted
         
        RvAresHashRemove(hash, oldRec);
        */
    }

    rec->size = (RvUint16)(ctx->cur - (RvUint8 *)rec);

    cell = RvAresCacheCellsFindCell(&self->cells, curAtime, ctx->minTTL, &rec->expirationTime);
    if(cell == 0) {
        CACHE_LOG_ERROR_1("(ttl=%d) - failed to find cache cell", ttl);
        return RV_ERROR_UNKNOWN;
    }

    CACHE_LOG_DEBUG_2("for ttl = %d found cell with expiration = %d", ttl, cell->expirationTime);

    page = RvAresCacheCellGetPage(cell);

    if(page == 0) {
        /* no active page in this cell, try to allocate new page */
        page = RvAresCacheAllocPage(self, rec->size);
        if(page == 0) {
            CACHE_LOG_ERROR_0("no free pages was found, out of resources");
            return RV_ERROR_OUTOFRESOURCES;
        }
        RvAresCacheCellAddPage(cell, page);
    }

    /* After calling this function rec will point to the newly allocated
     *   record on the page
     */
    s = RvAresPageAddRdata(page, &rec);
    if(s == RV_ERROR_INSUFFICIENT_BUFFER) {
        page = RvAresCacheAllocPage(self, rec->size);
        if(page == 0) {
            CACHE_LOG_ERROR_0("no free pages was found, out of resources");
            return RV_ERROR_OUTOFRESOURCES;
        }

        s = RvAresPageAddRdata(page, &rec);
        if(s == RV_ERROR_INSUFFICIENT_BUFFER) {
            RvAresPageAllocatorFree(&self->pageAllocator, page);
            CACHE_LOG_ERROR_1("Record is too big (%d bytes), unable to cache", rec->size);
            return RV_DNS_ERROR_SMALLPAGE;
        }

        if(s != RV_OK) {
            return s;
        }

        RvAresCacheCellAddPage(cell, page);
    }

#if RV_DNS_CACHE_DEBUG
    rec->hmark = rec->pmark = RV_FALSE;
#endif

    RvAresHashAdd(&self->hash, insertPoint, rec);
    CACHE_LOG_DEBUG_4("Recordset=%p owner = %s, type = %d, ttl = %d cached", rec, rec->name, rec->type, ttl);

    return s;
#undef FUNCTION_NAME
}


static
RvStatus RvAresCacheRecord(RvAresCache *self,  RvAresCacheCtx *ctx, RvDnsData *record) {
    RvStatus s;
    RvAresCacheCtx  cnameCTX;

    (void)self;

    if(record->dataType == (RvDnsQueryType)T_CNAME) {
        RvAresCacheCtxConstruct(&cnameCTX, self->logSrc);
        ctx = &cnameCTX;
    }

    /* Interning first record */
    if(ctx->rdata == 0) {
        s = RvAresInternStart(ctx, record);
        if(s != RV_OK) {
            return s;
        }
    } else if(ctx->type != (RvInt)record->dataType) {
        return RV_ERROR_BADPARAM;
    }

    ctx->rdata->nRecords++;
    ctx->minTTL = RvMin(ctx->minTTL, record->ttl);
    if(ctx->rdata->negative) {
        return RV_OK;
    }

    switch(ctx->type) {
    case T_A:
        s = RvAresInternA(ctx, record);
        break;

#if RV_NET_TYPE & RV_NET_IPV6
    case T_AAAA:
        s = RvAresInternAAAA(ctx, record);
        break;
#endif

    case T_CNAME:
        s = RvAresInternCNAME(ctx, record);
        if(s == RV_OK) {
            RvAresTime curAtime = RvAresGetTime();
            s = RvAresCacheFinishCaching(self, curAtime, ctx);
            RvAresCacheCtxDestruct(ctx);
        }
        break;

    case T_SRV:
        s = RvAresInternSRV(ctx, record);
        break;

    case T_NAPTR:
        s = RvAresInternNAPTR(ctx, record);
        break;

    case T_NXDOMAIN:
        s = RV_OK;
        break;

    default:
        s = RV_DNS_ERROR_RTUNEXPECTED;
        break;

    }

    return s;
}



/*----------------------------------  Cache Daemon ---------------------------*/
#include "rvselect.h"
#include "rvsocket.h"
#include "rvares.h"

typedef void (*RvAresCacheDeserializer)(RvDnsData *data, RvUint8 **pBufStart);


static /* CBSUPPORT */
void RvAresCacheExternRdata(RvDnsData *data, RvUint8 **pBufStart) {
    RvRdata *rdata = (RvRdata *)(void *)*pBufStart;
    RvUint8 *pBuf;

    if(rdata->negative || rdata->type == T_NXDOMAIN) {
        data->dataType = RV_DNS_STATUS_TYPE;
        data->data.status = rdata->negative ? ARES_ENODATA : ARES_ENOTFOUND;
    } else {
        data->dataType = (RvDnsQueryType)(data->queryType = rdata->type);
    }

    data->ttl = 0;
    strcpy(data->ownerName, rdata->name);
    pBuf = &rdata->nameSize;
    pBuf += *pBuf + 2;
    *pBufStart = pBuf;
}


static /* CBSUPPORT */
void RvAresCacheExternA(RvDnsData *data, RvUint8 **pBufStart) {
    RvUint8 *pBuf = *pBufStart;
    RvUint32 ipAddr;
    RvAddress *pAddr = &data->data.hostAddress;

    pBuf = (RvUint8 *)RvAlign32(pBuf);	

    ipAddr = *(RvUint32 *)(void *)pBuf;

    (void)RvAddressConstructIpv4(pAddr, ipAddr, 0);
    *pBufStart = pBuf + 4;
}

#if RV_NET_TYPE & RV_NET_IPV6


static /* CBSUPPORT */
void RvAresCacheExternAAAA(RvDnsData *data, RvUint8 **pBufStart) {
    RvAddress *pAddr = &data->data.hostAddress;

    (void)RvAddressConstructIpv6(pAddr, *pBufStart, 0, 0);
    *pBufStart += 16;
}

#endif

static /* CBSUPPORT */
void RvAresCacheExternSRV(RvDnsData *data, RvUint8 **pBufStart) {
    RvUint8 *pBuf = *pBufStart;
    RvDnsSrvData *srv = &data->data.dnsSrvData;

    pBuf = (RvUint8*)RvAlign16(pBuf);
    srv->port = *(RvUint16 *)(void *)pBuf;
    pBuf += 2;
    srv->priority = *(RvUint16 *)(void *)pBuf;
    pBuf += 2;
    srv->weight = *(RvUint16 *)(void *)pBuf;
    pBuf += 2;
    strcpy(srv->targetName, (RvChar *)(pBuf + 1));
    pBuf += *pBuf + 2;
    *pBufStart = pBuf;
}


static /* CBSUPPORT */
void RvAresCacheExternNAPTR(RvDnsData *data, RvUint8 **pBufStart) {
    RvUint8 *pBuf = *pBufStart;
    RvDnsNaptrData *naptr = &data->data.dnsNaptrData;

    pBuf = (RvUint8 *)RvAlign16(pBuf);

    naptr->order = *(RvUint16 *)(void *)pBuf;
    pBuf += 2;
    naptr->preference = *(RvUint16 *)(void *)pBuf;
    pBuf += 2;
    strcpy(naptr->flags, (RvChar *)(pBuf + 1));
    pBuf += *pBuf + 2;
    strcpy(naptr->service, (RvChar *)(pBuf + 1));
    pBuf += *pBuf + 2;
    strcpy(naptr->regexp, (RvChar *)(pBuf + 1));
    pBuf += *pBuf + 2;
    strcpy(naptr->replacement, (RvChar *)(pBuf + 1));
    pBuf += *pBuf + 2;

    *pBufStart = pBuf;
}

static /* CBSUPPORT */
void RvAresCacheExternCNAME(RvDnsData *data, RvUint8 **pBufStart) {
    RvSize_t aliasSize;
    RvUint8 *pBuf = *pBufStart;
    RvDnsCNAMEData *cname = &data->data.dnsCnameData;

    aliasSize = *pBuf++;
    memcpy(cname->alias, pBuf, aliasSize + 1);
    pBuf += aliasSize + 1;
    *pBufStart = pBuf;
}


typedef struct _RvAresCacheDQuery RvAresCacheDQuery;

#define NODATA   0
#define RDATA    1
#define ADDRLIST 2

typedef struct _QueryData {
    RvInt discriminator;     /* choose one of the fields in union */
    union {
        RvRdata       *rdata;
        RvEHARecord   *eharec;
    } u;
} QueryData;

struct _RvAresCacheDQuery {
    RvObjPoolElement   poolElem;
    RvAresCacheDQuery *next;
    RvAresCacheClt    *clt;   
    RvUint32           qtype;
    RvUint32           qid;
    void              *ctx;
    RvDnsNewRecordCB   cb;
    QueryData          data;
};

typedef struct _QueryPool {
    RvObjPool pool;
} QueryPool;


struct _RvAresCacheD {
    RvLock             lock;
    RvBool             destructed;
    RvAresCache       *cache;
    RvTimer            timer;
    RvTimerQueue      *tquee;
    RvLogSource       *logSrc;
    RvAresCacheClt    *firstClt;
    RvUint32           userQid;       /* next qid. Will be always odd number */

    /* Debugging */
    RvSize_t           sentResults;
    RvSize_t           acceptedResults;
    RvEHD              *ehd;
    /* Active queries list */
    RvInt              nPostponed;     /* number of postponed queries for statistical purposes */
    RvInt              maxPostponed;   /* maximal number of postponed queries for statistical purposes */
    RvBool             needRaiseEvent; /* postponed event should be raised */
    QueryPool          queryPool;
    RvAresCacheDQuery *activeQuery;    /* points to the head of query list */
    RvAresCacheDQuery *lastActive;     /* points to the tail of query list */

    /* Support for cancelling queries */
    RvCondvar          inCallbackCond;  /* Conditional variable to support removing incallback queries */
    RvUint32           inCallbackQid;   /* query id of incallback query. when 0 - no queries in callback */
    RvAresCacheClt    *inCallbackClt;   /* pointer to cache client currently in callback */
    RvThreadId         inCallbackTid;   /* thread id of incallback thread */

};

static RvLock         gsGlobalInstanceLock;
static RvAresCacheD  *gsGlobalCacheD = 0;
static RvAresCache   *gsGlobalCacheP = 0;
static RvAresCache    gsGlobalCache;

static
RvStatus QueryPoolConstruct(QueryPool *self) {
    RvObjPoolFuncs vt;
    RvAresCacheDQuery tmpl;
    RvObjPool *pool;

	memset(&vt, 0, sizeof(RvObjPoolFuncs));
    vt.pagealloc = pageAlloc;
    vt.pagefree = pageFree;

    pool = RvObjPoolConstruct(&tmpl, &tmpl.poolElem, &vt, sizeof(tmpl), 32, 0, RV_OBJPOOL_TYPE_DYNAMIC, 
        RV_OBJPOOL_SALVAGE_ALLOWED, 0, 32, 50, &self->pool);

    return pool ? RV_OK : RV_ERROR_UNKNOWN;
}

static
void QueryPoolDestruct(QueryPool *self) {
    RvObjPoolDestruct(&self->pool);
}

static
RvAresCacheDQuery* QueryPoolAllocQuery(QueryPool *self) {
    RvAresCacheDQuery *q;
    
    q = (RvAresCacheDQuery *)RvObjPoolGetItem(&self->pool);
    return q;
}

static
void QueryPoolFreeQuery(QueryPool *self, RvAresCacheDQuery *q) {
    RvObjPoolReleaseItem(&self->pool, q);
}


/* 
 * Postponed queries handling 
 */

/* Removes query from active list */

static
RvStatus RvAresCacheDRemoveQueryAux(RvAresCacheD *self, RvAresCacheDQuery *prev, RvAresCacheDQuery *cur) {
#define FUNCTION_NAME "RvAresCacheDRemoveQueryAux"

#if RV_LOGMASK != RV_LOGLEVEL_NONE
    RvUint32 qid = cur->qid;
#endif

    RvAssert((prev == 0 && cur == self->activeQuery) || (prev->next == cur));

    if(cur->data.discriminator == RDATA){
        RvRdata *rdata = cur->data.u.rdata;
        RvAresPage *page = rdata->page;

        page->lockCnt--;
        if(page->lockCnt == 0) {
            /* Orphan page, release */
            RvAresPageAllocatorFree(&self->cache->pageAllocator, page);
        }
    } else {
        RvEHDRelease(self->ehd);
    }

   
    CACHE_LOG_DEBUG_1("canceling query qid=%x from cache", qid);
    if(prev == 0) {
        self->activeQuery = cur->next;
    } else {
        prev->next = cur->next;
    }

    if(self->lastActive == cur) {
        self->lastActive = prev;
    }

    QueryPoolFreeQuery(&self->queryPool, cur);
    return RV_OK;

#undef FUNCTION_NAME
}

static
RvStatus RvAresCacheDRemoveInCallbackQuery(RvAresCacheD *self, RvBool waitForCallbacks) {
#define FUNCTION_NAME "RvAresCacheDRemoveInCallbackQuery"
    RvThreadId tid = RvThreadCurrentId();
    RvUint32 qid = self->inCallbackQid;
    RV_UNUSED_ARG(qid);

    CACHE_LOG_DEBUG_1("canceling query qid=%x from cache", qid);

    if(tid == self->inCallbackTid) {
        CACHE_LOG_DEBUG_1("canceling incallback query from callback qid=%x", qid);
        return RV_OK;
    }

    if(!waitForCallbacks) {
        return RV_DNS_ERROR_INCALLBACK;
    }

    RV_CONDVAR_WAITL(self->inCallbackQid != qid, &self->inCallbackCond, &self->lock, 0);
    RvAssert(self->inCallbackQid != qid);

    return RV_OK;
#undef FUNCTION_NAME
}

static
RvStatus RvAresCacheDRemoveQuery(RvAresCacheD *self, RvUint32 qid, RvBool waitForCallbacks) {
#define FUNCTION_NAME "RvAresCacheDRemoveQuery"

    RvAresCacheDQuery *cur;
    RvAresCacheDQuery *prev;
    RvStatus s = RV_OK;


    for(cur = self->activeQuery, prev = 0; 
        cur != 0 && cur->qid != qid;
        prev = cur, cur = cur->next) {}

    if(cur != 0) {
        s = RvAresCacheDRemoveQueryAux(self, prev, cur);
    }

    if(self->inCallbackQid == qid) {
        RvAresCacheDRemoveInCallbackQuery(self, waitForCallbacks);
    }

    return s;
#undef FUNCTION_NAME
}


static
void RvAresCacheDAnalyzeAddressList(RvAresCacheD *self, RvAresCacheDQuery *query) {
#define FUNCTION_NAME
    RvAddressList *curAddr;
    RvDnsData data;
    RvStatus s;
    RvBool continueCallbacks = RV_TRUE;
    RvInt qAddrType;
    RvUint32 qid = query->qid;
    RvInt qtype = query->qtype;
    RvDnsNewRecordCB onNewRecordCB = query->cb;
    void *ctx = query->ctx;

    RvEHARecord *arec;
    RvSize_t ownerNameSize = sizeof(data.ownerName);

    RV_UNUSED_ARG(self);

    arec = query->data.u.eharec;


    data.recordNumber = 0;
    strncpy(data.ownerName, arec->base.name, ownerNameSize);
    data.ownerName[ownerNameSize - 1] = 0;
    data.queryType = qtype;
    data.dataType = (RvDnsQueryType)qtype;
    data.ttl = 0;
    
    qAddrType = (query->qtype == T_A) ? RV_ADDRESS_TYPE_IPV4 : RV_ADDRESS_TYPE_IPV6;

    for(curAddr = &arec->firstAddr; curAddr != 0 && continueCallbacks; curAddr = curAddr->next) {
        RvInt addrType = RvAddressGetType(&curAddr->addr);
        if(addrType != qAddrType) {
            continue;
        }

        data.recordNumber++;
        RvAddressCopy(&curAddr->addr, &data.data.hostAddress);
        s = onNewRecordCB(ctx, qid, &data);
        if(RvErrorGetCode(s) == RV_ERROR_DESTRUCTED) {
            CACHE_LOG_DEBUG_1("Query qid=%x served from ehd", qid);
            continueCallbacks = RV_FALSE;
        }
    }

    if(continueCallbacks) {
        data.dataType = RV_DNS_ENDOFLIST_TYPE;
        (void)onNewRecordCB(ctx, qid, &data);
        CACHE_LOG_DEBUG_1("Query qid=%x served from ehd", qid);
    }
#undef FUNCTION_NAME
}

/* Callback support function, is called in *unlocked* state of cached and yet
 * logMgr changes shouldn't happen 
 */
static /*CBSUPPORT*/
void RvAresCacheDAnalyzeRdata(RvAresCacheD *self, RvAresCacheDQuery *query) {
#define FUNCTION_NAME "RvAresCacheDAnalyzeRdata"
    RvSize_t nRecords;
    RvSize_t i;
    RvAresCacheDeserializer deserialize;
    RvUint8 *curPtr;
    RvDnsData data;
    RvDnsNewRecordCB onNewRecordCB;
    void *ctx;
    RvStatus s;
    RvBool continueCallbacks = RV_TRUE;
    RvUint32 qid = query->qid;
    RvRdata *rdata = query->data.u.rdata;

    RV_UNUSED_ARG(self);


    onNewRecordCB = query->cb;
    ctx = query->ctx;
    nRecords = rdata->nRecords;

    curPtr = (RvUint8 *)rdata;
    RvAresCacheExternRdata(&data, &curPtr);

    if(data.dataType == RV_DNS_STATUS_TYPE) {
        data.recordNumber = 0;
        HistoryAdd(ctx, Callbacked, "Callbacked S");
        (void)onNewRecordCB(ctx, qid, &data);
        CACHE_LOG_DEBUG_1("Query qid=%x served from cache", qid);
        return;
    } 

    HistoryAdd(ctx, Callbacked, "Callbacked D");

    switch((RvInt)data.dataType) {
        case T_A:
            deserialize = RvAresCacheExternA;
            break;

        case T_CNAME:
            deserialize = RvAresCacheExternCNAME;
            break;

#if RV_NET_TYPE & RV_NET_IPV6

        case T_AAAA:
            deserialize = RvAresCacheExternAAAA;
            break;

#endif

        case T_SRV:
            deserialize = RvAresCacheExternSRV;
            break;

        case T_NAPTR:
            deserialize = RvAresCacheExternNAPTR;
            break;

        default:
            /* Unexpected condition, shouldn't happen */

            CACHE_LOG_EXCEP_2("Unexpected data type (%d) for qid=%x, probably memory corruption", 
                (RvInt)data.dataType, qid);
            return;
    }

    data.recordNumber = 0;

    for(i = 0; i < nRecords && continueCallbacks; i++) {
        data.recordNumber++;
        deserialize(&data, &curPtr);
        /* if the callback returns error - don't continue with callbacks,
        *  probably, object that accepts callbacks is destructed
        */
        s = onNewRecordCB(ctx, qid, &data);
        if(RvErrorGetCode(s) == RV_ERROR_DESTRUCTED) {
            CACHE_LOG_DEBUG_1("Query qid=%x served from cache", qid);
            continueCallbacks = RV_FALSE;
        }
    }

    if(continueCallbacks) {
        data.dataType = RV_DNS_ENDOFLIST_TYPE;
        (void)onNewRecordCB(ctx, qid, &data);
        CACHE_LOG_DEBUG_1("Query qid=%x served from cache", qid);
    }
#undef FUNCTION_NAME
}

static
void RvAresCacheDAnalyzePacket(RvAresCacheD *self, RvAresCacheDQuery *query) {
#define FUNCTION_NAME "RvAresCacheDAnalyzePacket"
    RvLogMgr *logMgr;

    RV_CONDVAR_WAITL(self->inCallbackTid == 0, &self->inCallbackCond, &self->lock, 0);
    RvAssert(self->inCallbackQid == 0);

    if(query->clt->bInDestroy == RV_TRUE) {
        return;
    }

    logMgr = LOGMGR;
    self->inCallbackQid = query->qid;
    self->inCallbackClt = query->clt;
    self->inCallbackTid = RvThreadCurrentId();
    (void)RvLockRelease(&self->lock, 0);

    /* Now, we're officially in callback, no logMgr changes should happen */

    switch(query->data.discriminator) {
        case RDATA:
            RvAresCacheDAnalyzeRdata(self, query);
            break;
        case ADDRLIST:
            RvAresCacheDAnalyzeAddressList(self, query);
            break;
        default:
            CACHE_LOG_EXCEP_2("Unknown discriminator (%d), for qid=%x, shouldn't happen", 
                query->data.discriminator, query->qid);
            break;

    }

   
    /* This function assumes it's called in locked state of 'self' and should left it in 'locked' state */
    (void)RvLockGet(&self->lock, 0);

    self->inCallbackQid = 0;
    self->inCallbackTid = 0;
    self->inCallbackClt = 0;
    RvCondvarBroadcast(&self->inCallbackCond, logMgr);

    if(query->data.discriminator == RDATA) {
        RvAresPage *page = query->data.u.rdata->page;
        page->lockCnt--;
        CACHE_LOG_DEBUG_4("Serviced query qid=%x referencing rdata=%p referencing page=%p with lockCnt=%d",
            query->qid, query->data.u.rdata, page, page->lockCnt);
        if(page->lockCnt == 0) {
            CACHE_LOG_DEBUG_1("Orphan page=%p found, free it", page);
            RvAresPageAllocatorFree(&self->cache->pageAllocator, page);
        }
    } else {
        RvEHDRelease(self->ehd);
    }
#undef FUNCTION_NAME
}

LINT_NOT_CONST(timer)
static
RvBool RvAresCacheDOnTimer(void *ctx, RvTimer *timer) {
#define FUNCTION_NAME "RvAresCacheDOnTimer"

    RvAresCacheD *self = (RvAresCacheD *)ctx;
    RvInt maxQueries; /* Maximal number of queries to handle in this iteration */
    RvInt i;

    RV_UNUSED_ARG(timer);

    (void)RvLockGet(&self->lock, 0);


    self->needRaiseEvent = RV_TRUE;
    /* We will handle at most maxQueries in this iteration */
    maxQueries = self->nPostponed;
    self->nPostponed = 0;

    for(i = 0; i < maxQueries; i++) {
        RvAresCacheDQuery *q = self->activeQuery;

        if(q == 0) {
            break;
        }
        self->activeQuery = q->next;
        if(self->activeQuery == 0) {
            self->lastActive = 0;
        }
        CACHE_LOG_DEBUG_1("Serving query qid=%x from cache", q->qid);
        RvAresCacheDAnalyzePacket(self, q);
        QueryPoolFreeQuery(&self->queryPool, q);
        self->acceptedResults++;
    }

    (void)RvLockRelease(&self->lock, 0);
    return RV_FALSE;
#undef FUNCTION_NAME
}


static
RvStatus RvAresCacheDAddQuery(RvAresCacheD *self, RvAresCacheDQuery *q) {
#undef FUNCTION_NAME
#define FUNCTION_NAME "RvAresCacheDAddQuery"
    RvStatus s;

   
    /* Add query to the end of active query list */
    q->next = 0;
    if(self->lastActive) {
        /* If list isn't empty, make the last query in the list to point
         * to the new query
         */
        self->lastActive->next = q;
    } else {
        /* If list is empty, make activeQuery to point to the new entry
         */
        self->activeQuery = q;
    }
    self->lastActive = q;


    self->nPostponed++;
    self->maxPostponed = RvMax(self->nPostponed, self->maxPostponed);
    if(!self->needRaiseEvent) {
        CACHE_LOG_DEBUG_2("No need to trigger postponed events handling (nPostponed=%d, maxPostponed=%d)", 
            self->nPostponed, self->maxPostponed);
        return RV_OK;
    }

    RvTimerConstruct(&self->timer);
    s = RvTimerStartEx(&self->timer, self->tquee, RV_TIMER_TYPE_ONESHOT, RV_UINT64_ONE, RvAresCacheDOnTimer, self);
    if(s != RV_OK) {
        /* Exceptional condition: failed to start timer. 
         * The best we can do now is not to clear 'needRaiseEvent' and hope that next query to cache 
         * will succeed to start it.
         */
        CACHE_LOG_EXCEP_1("Failed to trigger postponed requests handling: %x", s);
        return s;
    }

    self->needRaiseEvent = RV_FALSE;
    CACHE_LOG_DEBUG_0("Postponed requests handling triggered");
    return RV_OK;
}

static 
void RvAresCacheSetLogSrc(RvAresCache *self, RvLogSource *logSrc);


/*lint -save -e801 GOTO is OK here*/
static
RvStatus RvAresCacheDConstruct(RvAresCacheD   *self,
                               RvAresCache    *pCache,
                               RvTimerQueue   *tqueue,
                               RvAresCacheClt *clt,
                               const RvAresCacheParams *params) {

#undef FUNCTION_NAME
#define FUNCTION_NAME "RvAresCacheDConstruct"
#define CACHE_CONSTRUCTED  1
#define LOCK_CONSTRUCTED   4
#define EHD_CONSTRUCTED    8
#define COND_CONSTRUCTED   16
#define QPOOL_CONSTRUCTED  32


    RvStatus s;
    RvUint32  stage = 0;
    RvAresCache *cache = 0;
    RvLogSource *logSrc;
    RvLogMgr *logMgr;

#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
    RV_UNUSED_ARG(params);
#endif

    logSrc = self->logSrc = clt->logSrc;
    logMgr = LogMgrFromLogSrc(logSrc);
    /* RvAresCacheSetLogInfo(self->logMgr, self->logSrc);*/

    CACHE_LOG_ENTER_3("(self = %p, tqueue = %p, clt = %p)", self, tqueue, clt);
    CACHE_LOG_INFO_6("Constructing DNS cache with pageSize=%d, minPages=%d, maxPages=%d,"
        "maxBigPagesSize=%d, deltaPages=%d, hashSize=%d", 
        (int)params->pageSize,
        (int)params->minPages,
        (int)params->maxPages,
        (int)params->maxBigPagesSize,
        (int)params->deltaPages,
        (int)params->hashSize);

    s = RvLockConstruct(logMgr, &self->lock);
    if(s != RV_OK) goto failure;
    s |= LOCK_CONSTRUCTED;

    s = RvCondvarConstruct(&self->inCallbackCond, logMgr);
    if(s != RV_OK) goto failure;
    s |= COND_CONSTRUCTED;

    self->inCallbackQid = 0;
    self->inCallbackTid = 0;
    self->userQid = 1;
    self->tquee = tqueue;
    self->nPostponed = 0;
    self->maxPostponed = 0;
    self->needRaiseEvent = RV_TRUE;

#ifndef RV_USE_LOCAL_CACHE
#define RV_USE_LOCAL_CACHE 0
#endif

#if RV_USE_LOCAL_CACHE
    cache = &self->cache;
    s = RvAresCacheConstruct(cache, logSrc, params);
    if(s != RV_OK) goto failure;
    stage |= CACHE_CONSTRUCTED;
#else
    self->cache = pCache;
    RvAresCacheSetLogSrc(self->cache, self->logSrc);
#endif

    s = RvEHDNew(&self->ehd, logMgr);
	if(s != RV_OK) {
		CACHE_LOG_ERROR_1("Error constructing END, HOSTS file will not be used (%d)", s);
	}
	else
		stage |= EHD_CONSTRUCTED;

    s = QueryPoolConstruct(&self->queryPool);
    if(s != RV_OK) {
        goto failure;
    }
    stage |= QPOOL_CONSTRUCTED;

    self->sentResults = 0;
    self->acceptedResults = 0;
    self->firstClt = 0;
    self->activeQuery = 0;
    self->lastActive = 0;
    self->destructed = RV_FALSE;
    RvTimerConstruct(&self->timer);

    CACHE_LOG_LEAVE;

    return s;

failure:
    if(stage & CACHE_CONSTRUCTED) {
        RvAresCacheDestruct(cache);
    }

    if(stage & LOCK_CONSTRUCTED) {
        (void)RvLockDestruct(&self->lock, logMgr);
    }

    if(stage & COND_CONSTRUCTED) {
        RvCondvarDestruct(&self->inCallbackCond, logMgr);
    }

    if(stage & EHD_CONSTRUCTED) {
        RvEHDDelete(self->ehd);
    }

    if(stage & QPOOL_CONSTRUCTED) {
        QueryPoolDestruct(&self->queryPool);
    }


    CACHE_LOG_ERROR_1("failed: %d", s);
    return s;

#undef CACHE_CONSTRUCTED
#undef LOCK_CONSTRUCTED
#undef FUNCTION_NAME
}

/*lint -restore*/

static
void RvAresCacheDAddClt(RvAresCacheD *self, RvAresCacheClt *env) {
    (void)RvLockGet(&self->lock, 0);

    /* First client is always added directly by CacheDConstruct, so here I don't treat
     * case when self->firstClt == 0
     */

    env->next = self->firstClt;
    self->firstClt = env;

    (void)RvLockRelease(&self->lock, 0);
}

static RvStatus RvAresCacheDDestruct(RvAresCacheD *self) {
#define FUNCTION_NAME "RvAresCacheDDestruct"
    RvLogMgr *logMgr = LOGMGR;
    RvStatus  s = RV_OK;

    RvLockGet(&self->lock, 0);
    if(self->destructed) {
        CACHE_LOG_DEBUG_1("cache=%p being destructed, exiting", self);
        RvLockRelease(&self->lock, 0);
        return RV_ERROR_DESTRUCTED;
    }
    self->destructed = RV_TRUE;
    RvAssert(self->activeQuery == 0 && self->inCallbackQid == 0);
    RvLockRelease(&self->lock, 0);

    if(self->needRaiseEvent == RV_FALSE) {
        /* Cancel the timer if it was started */
        RvTimerCancel(&self->timer, RV_TIMER_CANCEL_WAIT_FOR_CB);
    }


#if RV_USE_LOCAL_CACHE
    RvAresCacheClear(&self->cache);
    RvAresCacheDestruct(&self->cache);
#else
    RvAresCacheSetLogSrc(self->cache, 0);
#endif

    if(self->ehd) {
        (void)RvEHDDelete(self->ehd);
    }

    QueryPoolDestruct(&self->queryPool);

    RvCondvarDestruct(&self->inCallbackCond, logMgr);

    (void)RvLockDestruct(&self->lock, logMgr);
    return s;
#undef FUNCTION_NAME
}

static
RvStatus RvAresCacheDNew(RvAresCacheD **pSelf, RvAresCache *pCache, RvSelectEngine *seli, RvAresCacheClt *clt, const RvAresCacheParams *params) {
    RvStatus s = RV_OK;
    RvSize_t totalSize;
    RvTimerQueue *tqueue;


    totalSize = sizeof(**pSelf);

    s = RvMemoryAlloc(0, totalSize, clt->logMgr, (void **)pSelf);

    if(s != RV_OK) {
        return s;
    }

    (void)RvSelectGetTimeoutInfo(seli, 0, &tqueue);

    s = RvAresCacheDConstruct(*pSelf, pCache, tqueue, clt, params);
    if(s != RV_OK) {
        (void)RvMemoryFree(*pSelf, clt->logMgr);
    }

    return s;
}

/*RVCOREAPI*/
static void RvAresCacheDDelete(RvAresCacheD *self) {
    RvLogMgr *logMgr = LOGMGR;

    (void)RvAresCacheDDestruct(self);
    (void)RvMemoryFree(self, logMgr);
}


RvStatus RvAresCacheDRecord(RvAresCacheD *self, RvAresCacheCtx *ctx, RvDnsData *record) {
#define FUNCTION_NAME "RvAresCacheDRecord"

    RvStatus s;


    (void)RvLockGet(&self->lock, 0);

    CACHE_LOG_ENTER_3("(self=%p, owner=%s, type=%d)", self, record->ownerName, record->dataType);

    if(self->destructed) {
        CACHE_LOG_DEBUG_1("cache=%p being destructed, exiting", self);
        RvLockRelease(&self->lock, 0);
        return RV_ERROR_DESTRUCTED;
    }

    s = RvAresCacheRecord(self->cache, ctx, record);
    if(s != RV_OK) {
        CACHE_LOG_ERROR_4("(self=%p, owner=%s, type=%d) = %d, failed to cache record", self, record->ownerName, record->dataType, s);
    } else {
        CACHE_LOG_LEAVE_3("(self=%p, owner=%s, type=%d)", self, record->ownerName, record->dataType);
    }

    (void)RvLockRelease(&self->lock, 0);

    return s;
#undef FUNCTION_NAME
}


/*
 * RvStatus RvAresCacheDFind(RvAresCacheD *self,
 *                           RvUint32 queryType,
 *                           const RvChar *name,
 *                           RvSize_t nameSize,
 *                           RvChar  **domainSuffixes,
 *                           RvSize_t  nDomains,
 *                           RvUint32 *domainSuffixesMask,
 *                           RvDnsNewRecordCB cb,
 *                           void *cbCtx,
 *                           RvUint32 *pQid) 
 *
 * Description:
 *
 * Tries to find recordset given by <name, queryType> in the cache
 *
 * Arguments
 *
 * self        - points to the instance of cache
 * queryType   - type of query: one of T_A, T_AAAA, T_CNAME, T_SRV, T_NAPTR
 * name        - owner name
 * nameSize    - owner name size
 * domainSuffixes - array of possible suffixes to try
 * nDomains    - size of domainSuffixes array
 * domainSuffixesMask - if bit 'i' == 0 - appropriate record was found in negative cache
 * cb          - callback
 * cbCtx       - user context that will be passed to 'cb'
 * qid         - query id. This id may be used lately to cancel this query.
 
 *
 * Return value:
 *   RV_OK - record is found in positive cache positive cache and results will be reported
 *           using callback mechanism
 *
 *
 *
 *   RV_DNS_ERROR_CACHE_NOTFOUND - record wasn't found in cache.
 */
RvStatus RvAresCacheDFind(RvAresCacheClt *clt,
                          RvUint32 queryType,
                          const RvChar *name,
                          RvSize_t nameSize,
                          RvChar  **domainSuffixes,
                          RvSize_t  nDomains,
                          RvUint32 *domainSuffixesMask,
                          RvDnsNewRecordCB cb,
                          void *cbCtx,
                          RvUint32 *pQid) {

#define FUNCTION_NAME "RvAresCacheDFind"

    RvStatus s = RV_OK;
    RvAresCacheD *self = clt->cached;
    RvRdata  *rdata = 0;
    RvAresCacheDQuery *q;
    RvChar fullName[RV_DNS_MAX_NAME_LEN + 1];
    RvUint32 mask;
    RvUint32 outMask = 0;
    RvUint32 bit = 1;
    RvInt i;
    RvChar *suffixes[32];
    QueryData qdata = {NODATA, {0}};
    RvAresTime curAtime = RvAresGetTime();



    (void)RvLockGet(&self->lock, 0);

    CACHE_LOG_ENTER_3("(self=%p, qtype=%d, name=%s)", self, queryType, name);

    if(clt->bInDestroy || self->destructed) {
        CACHE_LOG_DEBUG_1("cache=%p being destructed, exiting", self);
        RvLockRelease(&self->lock, 0);
        return RV_ERROR_DESTRUCTED;
    }

    memcpy(fullName, name, nameSize);
    fullName[nameSize] = 0;

    memset(suffixes, 0, sizeof(suffixes));

    suffixes[0] = "";

    if(nDomains > 31) {
        CACHE_LOG_ERROR_1("Unexpected error: number of domains %d > 31", nDomains);
        nDomains = 31;
    }

    /* Name is fully qualified, domains search isn't needed */
    if(fullName[nameSize - 1] == '.') {
        nDomains = 0;
    }

    if(nDomains > 0 ) {
        memcpy(&suffixes[1], domainSuffixes, nDomains * sizeof(*domainSuffixes));
    }

    mask = (1 << (nDomains + 1)) - 1;

    for(bit = 1, i = 0; mask; bit <<= 1, mask >>= 1, i++) {
        RvSize_t suffixLen;
        RvChar *suffix;
        RvSize_t fullNameSize = nameSize;

        rdata = 0;
        if(!(mask & 1)) {
            continue;
        }

        suffix = suffixes[i];
        suffixLen = strlen(suffix);

        if(suffixLen) {
            fullNameSize = nameSize + suffixLen + 1;

            if(fullNameSize > RV_DNS_MAX_NAME_LEN) {
                continue;
            }

            fullName[nameSize] = '.';
            strcpy(fullName + nameSize + 1, suffix);
        }

        if(self->ehd != 0 && (queryType == T_A || queryType == T_AAAA)) {
            RvEHARecord *eharec = 0;
            s = RvEHDFind(self->ehd, fullName, (RvUint16)queryType, &eharec);
            if(s == RV_OK) {
                qdata.discriminator = ADDRLIST;
                qdata.u.eharec = eharec;
                break;
            }
        }


        rdata = RvAresCacheFind(self->cache,  curAtime, queryType, fullName, fullNameSize, 0);

        if(rdata != 0) {
            if((rdata->negative || rdata->type == T_NXDOMAIN)) {
                if(qdata.discriminator == NODATA) {
                    CACHE_LOG_DEBUG_2("(qtype=%d, name=%s), found in negative cache", queryType, fullName);
                    qdata.discriminator = RDATA;
                    qdata.u.rdata = rdata;
                }
                continue;
            } else {
                qdata.discriminator = RDATA;
                qdata.u.rdata = rdata;
                break;
            }
        }

        outMask |= bit;
        CACHE_LOG_DEBUG_2("(qtype=%d, name=%s), not found", queryType, fullName);
    }

    if((qdata.discriminator == NODATA || 
        ((qdata.discriminator == RDATA) && (qdata.u.rdata->negative || qdata.u.rdata->type == T_NXDOMAIN))) && 
        outMask != 0) {
        *domainSuffixesMask = outMask;
        (void)RvLockRelease(&self->lock, 0);
        return RV_DNS_ERROR_CACHE_NOTFOUND;
    }

    q = QueryPoolAllocQuery(&self->queryPool);
    if(q == 0) {
        CACHE_LOG_ERROR_0(" failed to allocate query");
        (void)RvLockRelease(&self->lock, 0);
        return RV_ERROR_OUTOFRESOURCES;  
    }
   
    q->clt = clt;
    q->qid = *pQid = self->userQid;
    q->data = qdata;
    q->qtype = queryType;
    if(qdata.discriminator == ADDRLIST) {
        RvEHDLock(self->ehd);
    } else {
        qdata.u.rdata->page->lockCnt++;
        CACHE_LOG_DEBUG_4("Query qid=%x references rdata=%p from page=%p with lockCnt=%d", 
            q->qid, qdata.u.rdata, qdata.u.rdata->page, qdata.u.rdata->page->lockCnt);
    }
    q->ctx = cbCtx;
    q->cb  = cb;

    /* User qid of cached queries will be always some odd number */
    self->userQid += 2;
    s = RvAresCacheDAddQuery(self, q);
    self->sentResults++;

    CACHE_LOG_LEAVE;

    (void)RvLockRelease(&self->lock, 0);

    return s;

#undef FUNCTION_NAME
}


/*RVCOREAPI*/
RvStatus RvAresCacheDCancelQuery(RvAresCacheD *self, RvUint32 qid, RvBool waitForCallbacks) {
#define FUNCTION_NAME "RvAresCacheDCancelQuery"

    RvStatus s;

    (void)RvLockGet(&self->lock, 0);
    if(self->destructed) {
        CACHE_LOG_DEBUG_1("cache=%p being destructed, exiting", self);
        RvLockRelease(&self->lock, 0);
        return RV_ERROR_DESTRUCTED;
    }

    s = RvAresCacheDRemoveQuery(self, qid, waitForCallbacks);
    
    (void)RvLockRelease(&self->lock, 0);

    return s;

#undef FUNCTION_NAME
}

static
RvStatus RvAresCacheDCancelQueries(RvAresCacheClt *clt) {
#define FUNCTION_NAME "RvAresCacheDCancelQueries"

    RvAresCacheD *self = clt->cached;
    RvAresCacheDQuery *prev, *cur, *next;

    prev = 0;
    cur = self->activeQuery;

    while(cur != 0) {
        if(cur->clt != clt) {
            prev = cur;
            cur = cur->next;
            continue;
        }

        next = cur->next;
        RvAresCacheDRemoveQueryAux(self, prev, cur);
        cur = next;
    }

    if(self->inCallbackClt == clt) {
        RvAresCacheDRemoveInCallbackQuery(self, RV_TRUE);
    }

    return RV_OK;

#undef FUNCTION_NAME
}

/*RVCOREAPI*/
LINT_NOT_CONST(self)
RvStatus RvAresCacheDStartCaching(RvAresCacheD *self, RvAresCacheCtx *ctx) {
#define FUNCTION_NAME "RvAresCacheDStartCaching"

    RvLockGet(&self->lock, 0);
    if(self->destructed) {
        CACHE_LOG_DEBUG_1("cache=%p being destructed, exiting", self);
        RvLockRelease(&self->lock, 0);
        return RV_ERROR_DESTRUCTED;
    }

    RvAresCacheCtxConstruct(ctx, self->logSrc);
    RvLockRelease(&self->lock, 0);
    return RV_OK;
#undef FUNCTION
}
LINT_CONST(self)

/*RVCOREAPI*/
RvStatus RvAresCacheDFinishCaching(RvAresCacheD *self, RvAresCacheCtx *ctx) {
    RvStatus s = RV_OK;
    RvAresTime curAtime = RvAresGetTime();

    (void)RvLockGet(&self->lock, 0);

    if(self->destructed) {
        CACHE_LOG_DEBUG_1("cache=%p being destructed, exiting", self);
        RvLockRelease(&self->lock, 0);
        return RV_ERROR_DESTRUCTED;
    }

    s = RvAresCacheFinishCaching(self->cache, curAtime, ctx);

    RvAresCacheCtxDestruct(ctx);
    (void)RvLockRelease(&self->lock, 0);

    return s;
}


void RvAresCacheDClear(RvAresCacheD *self) {

    (void)RvLockGet(&self->lock, 0);
    if(self->destructed) {
        CACHE_LOG_DEBUG_1("cache=%p being destructed, exiting", self);
        RvLockRelease(&self->lock, 0);
    }
    RvAresCacheClear(self->cache);
    (void)RvLockRelease(&self->lock, 0);
}

static 
void RvAresCacheSetLogSrc(RvAresCache *self, RvLogSource *logSrc) {
    self->logSrc = logSrc;
    self->hash.logSrc = logSrc;
    self->cells.logSrc = logSrc;
    self->pageAllocator.logSrc = logSrc;
}

static
void RvAresCacheDRemoveClt(RvAresCacheD *self, RvAresCacheClt *env) {
    /* RemoveClt is under global lock, so at most one RemoveClt may be in progress */

    RvAresCacheClt *cur, *prev;
    RvLogMgr *logMgr;
    RvStatus s;
    RvSelectEngine *seli = env->seli;
    RvTimerQueue *tqueue;
    RvTimer oldTimer;
    RvBool bTimerActive;
    RvTimerQueue *oldTQueue;

    (void)RvLockGet(&self->lock, 0);

    logMgr = LOGMGR;

    /* Wait while callback in progress on the cache */
    RV_CONDVAR_WAITL(self->inCallbackQid == 0, &self->inCallbackCond, &self->lock, 0);
    RvAssert(self->inCallbackQid == 0);
    /* Now, the cache is locked and no callbacks are in progress */


    env->bInDestroy = RV_TRUE;
    RvAresCacheDCancelQueries(env);

    for(prev = 0, cur = self->firstClt; cur != 0 && cur != env; prev = cur, cur = cur->next)
    {}

    if(cur == 0) {
        RvLockRelease(&self->lock, 0);
        return;
    }

    if(prev == 0) {
        self->firstClt = cur->next;
    } else {
        prev->next = cur->next;
    }

   
    if(self->firstClt == 0) {
        /* Removing last client, deleting cache */
        (void)RvLockRelease(&self->lock, 0);
        RvAresCacheDDelete(self);
        if(self == gsGlobalCacheD) {
            gsGlobalCacheD = 0;
        }
        return;
    } 

    seli = env->seli;

    if(self->logSrc == env->logSrc) {
        self->logSrc = self->firstClt->logSrc;
        RvAresCacheSetLogSrc(self->cache, self->logSrc);
        logMgr = LOGMGR;
    }

    RvSelectGetTimeoutInfo(seli, 0, &tqueue);

    if(self->tquee != tqueue) {
        (void)RvLockRelease(&self->lock, 0);
        return;
    }

    /* Cache changes it's seli */
    RvSelectGetTimeoutInfo(self->firstClt->seli, 0, &tqueue);
    oldTQueue = self->tquee;
    self->tquee = tqueue;
    bTimerActive = self->needRaiseEvent == RV_FALSE;
    oldTimer = self->timer;
    RvTimerConstruct(&self->timer);
    self->needRaiseEvent = RV_TRUE;

    if(self->activeQuery != 0) {
        RvTimerConstruct(&self->timer);
        s = RvTimerStartEx(&self->timer, self->tquee, RV_TIMER_TYPE_ONESHOT, RV_UINT64_ONE, RvAresCacheDOnTimer, self);
        if(s != RV_OK) {
            /* Exceptional condition: failed to start timer. 
            * The best we can do now is not to clear 'needRaiseEvent' and hope that next query to cache 
            * will succeed to start it.
            */
            CACHE_LOG_EXCEP_1("Failed to trigger postponed requests handling: %x", s);
            
        } else {
          self->needRaiseEvent = RV_FALSE;
          CACHE_LOG_DEBUG_0("Postponed requests handling triggered");
        }
    }

    RvLockRelease(&self->lock, 0);

    if(bTimerActive) {
        RvTimerCancel(&oldTimer, RV_TIMER_CANCEL_WAIT_FOR_CB);
    }
}


/* Cache client functions */

RvStatus RvAresCacheCltConstruct(RvAresCacheClt *self, RvSelectEngine *seli, const RvAresCacheParams *params, RvLogMgr *logMgr) {
    RvStatus s = RV_OK;

    self->logMgr = logMgr;
    self->logSrc = 0;
    self->seli   = seli;
    self->bInDestroy = RV_FALSE;
    if(logMgr != 0) {
        /* Construct log source */
        s = RvLogSourceConstruct(logMgr, &self->ilogSrc, LOGSOURCE_NAME, "DNS Caching log source");
        if(s != RV_OK) {
            return s;
        }
        self->logSrc = &self->ilogSrc;
    }

    (void)RvLockGet(&gsGlobalInstanceLock, 0);

    

    if(gsGlobalCacheD == 0) {
        if(gsGlobalCacheP == 0) {
            s = RvAresCacheConstruct(&gsGlobalCache, self->logSrc, params);
            if(s != RV_OK) goto failure;
            gsGlobalCacheP = &gsGlobalCache;
        } else {

        }

        s = RvAresCacheDNew(&gsGlobalCacheD, gsGlobalCacheP, seli, self, params);
    }

    if(s == RV_OK) {
        self->cached = gsGlobalCacheD;
        RvAresCacheDAddClt(gsGlobalCacheD, self);
    }

failure:
    (void)RvLockRelease(&gsGlobalInstanceLock, 0);

    return s;
}

#if 0
/* produces warning */
RVCOREAPI
RvAresCacheD* EnterCache() {
    RvAresCacheD *cached = gsGlobalCacheD;

    return cached;
}
#endif

LINT_NOT_CONST(self)
RvStatus RvAresCacheCltDestruct(RvAresCacheClt *self) {
    (void)RvLockGet(&gsGlobalInstanceLock, 0);
    RvAresCacheDRemoveClt(gsGlobalCacheD, self);
    (void)RvLockRelease(&gsGlobalInstanceLock, 0);

    if(self->logMgr) {
        (void)RvLogSourceDestruct(self->logSrc);
    }

    return RV_OK;
}
LINT_CONST(self)

/*RVCOREAPI*/
RvStatus RvAresCacheDInit() {
    RvStatus s;

    gsStartTime = RvTimestampGet(0);
    s = RvLockConstruct(0, &gsGlobalInstanceLock);
    return s;
}

RvStatus
RvAresCacheDEnd() {
    RvStatus s;

    if(gsGlobalCacheP) {
        RvAresCacheClear(gsGlobalCacheP);
        RvAresCacheDestruct(gsGlobalCacheP);
        gsGlobalCacheP = 0;
    }
    s = RvLockDestruct(&gsGlobalInstanceLock, 0);
    return s;
}


#if RV_DNS_CACHE_DEBUG

#include "rvstdio.h"


typedef struct {
    RvFILE *fp;
    RvChar *indent;
} DumpRecordCtx;

static
RvBool DumpRecordset(RvRdata *rdata, void *ctx) {
    DumpRecordCtx *dumpCtx = (DumpRecordCtx *)ctx;
    RvFILE *fp = dumpCtx->fp;
    RvChar *indent = dumpCtx->indent;
    RvDnsData d;
    RvUint8 *curPtr = (RvUint8 *)rdata;
    RvSize_t nRecords;
    const RvChar *typeName;
    RvSize_t i;
    RvAresCacheDeserializer deserialize;

    switch(rdata->type) {
    case T_A:
        typeName = "A";
        break;

    case T_AAAA:
        typeName = "AAAA";
        break;

    case T_SRV:
        typeName = "SRV";
        break;

    case T_CNAME:
        typeName = "CNAME";
        break;

    case T_NAPTR:
        typeName = "NAPTR";
        break;

    case T_NXDOMAIN:
        typeName = "NXDOMAIN";
        break;

    default:
        typeName = "";
        RvAssert(0);
    }


    nRecords = rdata->nRecords;
    RvAssert(nRecords > 0 || rdata->type == T_NXDOMAIN || rdata->negative);

    RvAresCacheExternRdata(&d, &curPtr);
    RvFprintf(fp, "%sR:%p E:%8.8d S:%3.3d N:%1.1d T:%8.8s O:%s\n", indent, rdata, rdata->expirationTime, rdata->size, rdata->negative, typeName, rdata->name);
    if(rdata->negative || rdata->type == T_NXDOMAIN) {
        return RV_TRUE;
    }

    for(i = 0; i < nRecords; i++) {
        switch(d.dataType) {
        case T_CNAME:
            deserialize = RvAresCacheExternCNAME;
            break;
        case T_A:
            deserialize = RvAresCacheExternA;
            break;

#if RV_NET_TYPE & RV_NET_IPV6

        case T_AAAA:
            deserialize = RvAresCacheExternAAAA;
            break;

#endif

        case T_SRV:
            deserialize = RvAresCacheExternSRV;
            break;

        case T_NAPTR:
            deserialize = RvAresCacheExternNAPTR;
            break;

        default:
            /* Shouldn't happen */
            RvAssert(0);
            return RV_FALSE;
        }

        deserialize(&d, &curPtr);

        switch(d.dataType) {
        case RV_DNS_CNAME_TYPE:
            {
                RvDnsCNAMEData *cname = &d.data.dnsCnameData;
                RvFprintf(fp, "%s\tC=%s\n", indent, cname->alias);
                break;
            }

        case RV_DNS_HOST_IPV4_TYPE:
        case RV_DNS_HOST_IPV6_TYPE:
            {
                RvAddress *addr;
                RvChar buf[256];

                addr = &d.data.hostAddress;
                RvAddressGetString(addr, sizeof(buf), buf);
                RvFprintf(fp, "%s\tA=%s\n", indent, buf);
                break;
            }

        case RV_DNS_SRV_TYPE:
            {
                RvDnsSrvData *srv = &d.data.dnsSrvData;
                RvFprintf(fp, "%s\tP=%d, Pri=%d, W=%d, T=%s\n", indent, srv->port, srv->priority, srv->weight, srv->targetName);
                break;
            }

        case RV_DNS_NAPTR_TYPE:
            {
                RvDnsNaptrData *naptr = &d.data.dnsNaptrData;
                RvFprintf(fp, "%s\tF=%s, O=%d, P=%d, R=%s, Rplc=%s, S=%s\n", indent, naptr->flags, naptr->order, naptr->preference, naptr->regexp, naptr->replacement, naptr->service);
                break;
            }

        default:
            RvAssert(0);
            break;
        }


    }

    RvFprintf(fp, "\n");

    return RV_TRUE;
}

typedef struct {
    RvSize_t nPages;
    RvSize_t nRecs;
    RvSize_t inHashRecs;
} PagesStats;

/* Traverses page list with head == start and tail == end */
static
void RvAresCacheTraversePages(RvFILE *fp, RvAresPage *start, RvAresPage *end, PagesStats *stats) {
    RvAresPage *curPage;
    RvAresPage *prevPage = 0;

    stats->nPages = 0;
    stats->nRecs = 0;
    stats->inHashRecs = 0;

    for(curPage = start; curPage; prevPage = curPage, curPage = curPage->next) {
        RvRdata *curRec;
        RvInt nRecords = 0;

        stats->nPages++;
        RvFprintf(fp, "\tPage: %p, Expiration time:%d, Free: %d\n", curPage, curPage->maxExpirationTime, (RvUint) curPage->spaceLeft);

        for(curRec = curPage->firstRecord; curRec; curRec = curRec->nextPageEntry) {
            DumpRecordCtx ctx;

            nRecords++;
            if(curRec->hmark) {
                /* This record already was traversed in hash */
                stats->inHashRecs++;
                RvFprintf(fp, "\t\tR:%p, in hash\n", curRec);
                continue;
            }

            RvFprintf(fp, "\t\tR:%p, not in hash:\n", curRec);
            ctx.fp = fp;
            ctx.indent = "\t\t";
            DumpRecordset(curRec, &ctx);
        }

        stats->nRecs += nRecords;
        RvFprintf(fp, "\t\tRecords: %d\n\n", nRecords);
    }


    RvAssert(prevPage == end);
}

typedef struct {
    RvInt nPages;
    RvInt nInHashRecs;
    RvInt nRecs;
} CellsStats;

static
RvBool RvAresCacheTraverseCells(RvFILE *fp, RvAresTime curTime, CellsStats *cstats, RvAresCacheCells *self) {
    RvSize_t i;
    RvAresTime prevTTL, curTTL;
    RvSize_t nCells = self->curSize;
    RvAresTime curE;

    RvAssert(self->curSize <= self->maxSize);

    cstats->nPages = 0;
    cstats->nInHashRecs = 0;
    cstats->nRecs = 0;

    if(self->curSize == 0) {
        return RV_TRUE;
    }

    RvFprintf(fp, "Cache has %d expiration cells:\n", (RvUint) nCells);
    curE = self->cells[0].expirationTime;
    curTTL = curE - curTime;

    RvFprintf(fp, "\tE: %d, T: %d\n", curE, curTTL);

    prevTTL = self->cells[0].expirationTime - curTime;
    for(i = 1; i < nCells; i++) {
        curE = self->cells[i].expirationTime;
        curTTL = curE - curTime;
        RvFprintf(fp, "\tE: %d, T: %d\n", curE, curTTL);
        if(curTTL < (prevTTL << 1)) {
            /* sanity check failed */
            RvAssert(0);
            RvFprintf(fp, "Sanity check on expiration cells failed\n");
            return RV_FALSE;
        }

        prevTTL = curTTL;
    }

    RvFprintf(fp, "\n");

    for(i = 0; i < nCells; i++) {
        RvAresCacheCell *curCell =  &self->cells[i];
        RvAresTime cellTTL;
        PagesStats stats;

        cellTTL = curCell->expirationTime - curTime;

        RvFprintf(fp, "Analyzing cell %d (E: %d, T: %d)\n", (RvUint)i, curCell->expirationTime, cellTTL);
        RvAresCacheTraversePages(fp, curCell->head, curCell->tail, &stats);
        cstats->nPages += stats.nPages;
        cstats->nRecs += stats.nRecs;
        cstats->nInHashRecs += stats.inHashRecs;

        RvFprintf(fp, "\tPages: %d, Records: %d\n\n\n", (RvUint)stats.nPages, (RvUint)stats.nRecs);
    }

    return RV_TRUE;
}

typedef struct {
    RvInt nRecords;
    RvInt hashSize;
    RvInt nBusyCells;
    RvInt avrgChainLength;
    RvInt maxChainLength;
    RvSize_t totalSize;
} HashStats;

typedef RvBool (*HashRecordTraverser)(RvRdata *rdata, void *ctx);


static
RvBool RvAresTraverseHash(RvAresHash *self, HashStats *stats, HashRecordTraverser traverser, void *ctx) {
    RvAresHashLink  *first;
    RvAresHashLink  *cur;
    RvRdata *curRec = 0;
    RvSize_t i;

    if(stats) {
        memset(stats, 0, sizeof(*stats));
        stats->hashSize = self->size;
    }

    for(i = 0; i < self->size; i++) {
        RvInt chainLen = 0;

        first = &self->buckets[i];

        for(cur = first->next; cur != first; cur = cur->next) {
            chainLen++;
            curRec = RvObjectFromField(RvRdata, hashLink, cur);

            if(traverser == 0) {
                /* Remove hmarks */
                curRec->hmark = RV_FALSE;
                continue;
            }

            /* Make sure that this record wasn't traversed already */
            RvAssert(curRec->hmark == RV_FALSE);
            curRec->hmark = RV_TRUE;
            stats->totalSize += curRec->size;
            traverser(curRec, ctx);
        }

        if(stats && chainLen > 0) {
            stats->nBusyCells++;
            stats->nRecords += chainLen;
            stats->maxChainLength = RvMax(stats->maxChainLength, chainLen);
        }

    }

    return RV_TRUE;
}

#define RvAresUntraverseHash(self) RvAresTraverseHash(self, 0, 0, 0)


static
void RvAresCacheDump(RvAresCacheD *self, RvChar *filename) {
    RvFILE *fp;
    HashStats hstats;
    DumpRecordCtx ctx;
#ifdef RV_DNS_CACHED_DEBUG_REVIVED
    PagesStats pstats;
#endif
    CellsStats cstats;
    RvAresTime curAtime = RvAresGetTime();

    fp = RvFopen(filename, "w");
    if(fp == 0) {
        return;
    }

    ctx.fp = fp;
    ctx.indent = "";
    (void)RvLockGet(&self->lock, 0);

    RvAresCacheBookkeeping(self->cache, curAtime);

    RvFprintf(fp, "\n--------------------------------------HASH-------------------------------------------\n");

    RvAresTraverseHash(&self->cache->hash, &hstats, DumpRecordset, &ctx);
    

    RvFprintf(fp, "\nHash statistics\n");
    RvFprintf(fp, "\tRecords: %d\n\tBusy cells: %d\n\tMax chain: %d\n\tTotal size: %d\n", hstats.nRecords, hstats.nBusyCells, hstats.maxChainLength, (RvUint)hstats.totalSize);

    RvFprintf(fp, "\n----------------------------------Expiration Cells------------------------------------\n");
    
    RvAresCacheTraverseCells(fp, curAtime, &cstats, &self->cache->cells);

    RvFprintf(fp, "\n\nExpiration cells totals:\nPages: %d\nTotal records:%d\nIn-hash records: %d\nOut-of-hash records:%d", 
        cstats.nPages, cstats.nRecs, cstats.nInHashRecs, cstats.nRecs - cstats.nInHashRecs);

    RvFprintf(fp, "\n-----------------------------------Expired Pages--------------------------------------\n");

#ifdef RV_DNS_CACHED_DEBUG_REVIVED
    RvAresCacheTraversePages(fp, self->cache.expiredPages.first, self->cache.expiredPages.last, &pstats);

    RvFprintf(fp, "\nExpired pages: %d\nTotal records: %d\nIn-hash records:%d\nOut-of-hash records: %d",
        (RvUint)pstats.nPages, (RvUint)pstats.nRecs, (RvUint)pstats.inHashRecs, (RvUint)(pstats.nRecs - pstats.inHashRecs));
#endif

    RvAresUntraverseHash(&self->cache->hash);


    (void)RvFclose(fp);

    (void)RvLockRelease(&self->lock, 0);
}

#ifndef RV_DNS_CACHE_DUMPFILE
#define RV_DNS_CACHE_DUMPFILE "dnscache.dump"
#endif

RVCOREAPI
void RvAresCacheDumpGlobal(RvChar *filename) {
    if(filename == 0 || filename[0] == 0) {
        filename = RV_DNS_CACHE_DUMPFILE;
    }
    RvAresCacheDump(gsGlobalCacheD, filename);
}

#endif

#endif

