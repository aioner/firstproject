#include "ares.h"
#include "rvarescache.h"
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
#include "rvobjpool.h"
#include "rvehd.h"
#include "rvtimer.h"
#include <string.h>


#if RV_DNS_USES_CACHING

typedef RvUint32 RvAresTime;

static RvLock        gsGlobalInstanceLock;
static RvAresCacheD  *gsGlobalCache = 0;


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


#define RvAresGetTime() ((RvAresTime)((RvTimestampGet(0) - gsStartTime) / RV_TIME_NSECPERSEC))

#endif



#define LOGSOURCE_NAME "RCACHE"

#ifndef RV_DNS_CACHE_NUM_CELLS
#define RV_DNS_CACHE_NUM_CELLS 30
#endif


#if !defined(RV_DNS_CACHE_MIN_TTL) || (RV_DNS_CACHE_MIN_TTL <= 0)
#undef RV_DNS_CACHE_MIN_TTL
#define RV_DNS_CACHE_MIN_TTL 1
#endif

#ifndef RV_DNS_CACHE_ASYNC_BUFFER_SIZE
#define RV_DNS_CACHE_ASYNC_BUFFER_SIZE 8192
#endif


/* casts some field 'f' in the object of type T to type T, given fields address faddr */
#define RvObjectFromField(T, f, faddr) ((T *)((RvChar *)(faddr) - RV_OFFSETOF(T, f)))

#if RV_LOGMASK != RV_LOGLEVEL_NONE

#define INCLUDE_LOG_SUPPORT 1
#else
#define INCLUDE_LOG_SUPPORT 0
#define RvStrnzcpy(dest, src, n)
#endif


typedef struct _RvAresPage RvAresPage;
typedef struct _RvRdata    RvRdata;
typedef struct _RvDLink    RvDLink;
typedef struct _RvSLink    RvSLink;

typedef RvDLink RvAresHashLink;
typedef RvSLink RvAresPageLink;

struct _RvDLink {
    RvDLink *next;
    RvDLink *prev;
};

struct _RvSLink {
    RvSLink *next;
};


typedef struct {
    RvAresTime  expirationTime;
    RvAresPage *head; /* points to the first page in the linked list of pages */
    RvAresPage *tail; /* points to the last page */
} RvAresCacheCell;

typedef struct {
    RvSize_t maxSize;
    RvSize_t curSize;
    RvAresTime startTime;
    RvAresTime minTtl;
    RvAresTime maxTtl;
    /* sorted according expiration time array of cells.
    * For the sake of simplicity, there are 2 fake cells:
    * first (with expiration time == current time) and last (with infinite expiration time)
    * Insertion sort is used to keep this array sorted
    */
    RvAresCacheCell *cells;
} RvAresCacheCells;


struct _RvAresPage {
    RvObjPoolElement poolElem;
    RvAresPage      *next;        /* points to the next page in the cell */
    RvAresTime       maxExpirationTime; /* maximal expiration time of records on this page */
    RvRdata         *firstRecord; /* points to the first record on the page */
    RvSize_t         spaceLeft;   /* left space in bytes */
    RvUint8         *freeSpace;   /* pointer to the first free byte */
};

typedef struct {
    RvObjPool pagePool;
    RvSize_t  pageSize;
} RvAresPageAllocator;

/* RvRdata is actually variable-length. Last field may be up to 256 chars length */
struct _RvRdata {
    RvRdata        *nextPageEntry;
    RvAresTime      expirationTime;
    RvUint16        type;

    /* RvRdata fields (derived class) */
    RvAresHashLink  hashLink;  /* links records in the same bucket */
    RvInt           idx;       /* used for cyclic ordering in A and AAAA responses */
    RvUint16        size;      /* total size */
    RvBool          negative;  /* RV_TRUE - negative caching */
    RvUint8         nRecords;  /* number of records in this data set */
#if RV_DNS_CACHE_DEBUG
    RvBool          hmark;
    RvBool          pmark;
#endif

    RvUint8         nameSize;  /* owner name size */
    RvChar          name[1];   /* owner name, variable length - should be last field */
/*
    No new fields after this point!
*/
};


#define RvRdataIsValid(self) ((self)->hashLink.next != 0)

typedef struct {
    RvAresHashLink  *buckets;
    RvBool           ownsMem;
    RvSize_t         size;
} RvAresHash;


typedef struct {
    RvAresHash        hash;
    RvAresCacheCells  cells;
    struct {
        RvAresPage        *first;
        RvAresPage        *last;
    } expiredPages;
    RvAresPageAllocator pageAllocator;
    RvAresCacheCtx      cacheCtx;
    RvUint8             internBuf[RV_DNS_CACHE_PAGE_SIZE];
} RvAresCache;


typedef RvSize_t RvAresHashInsertPoint;

typedef struct {
    RvUint32     time;
    RvLogSource *logSrc;
    RvLogMgr    *logMgr;
} RvAresCacheEnv;



/* Logging support */

#define CACHE_LOG_INFO_0(f)                     RvLogInfo(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME))
#define CACHE_LOG_INFO_1(f, p1)                 RvLogInfo(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1))
#define CACHE_LOG_INFO_2(f, p1, p2)             RvLogInfo(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2))
#define CACHE_LOG_INFO_3(f, p1, p2, p3)         RvLogInfo(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3))
#define CACHE_LOG_INFO_4(f, p1, p2, p3, p4)     RvLogInfo(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4))
#define CACHE_LOG_INFO_5(f, p1, p2, p3, p4, p5) RvLogInfo(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4, p5))

#define CACHE_LOG_DEBUG_0(f)                     RvLogDebug(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME))
#define CACHE_LOG_DEBUG_1(f, p1)                 RvLogDebug(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1))
#define CACHE_LOG_DEBUG_2(f, p1, p2)             RvLogDebug(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2))
#define CACHE_LOG_DEBUG_3(f, p1, p2, p3)         RvLogDebug(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3))
#define CACHE_LOG_DEBUG_4(f, p1, p2, p3, p4)     RvLogDebug(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4))
#define CACHE_LOG_DEBUG_5(f, p1, p2, p3, p4, p5) RvLogDebug(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4, p5))


#define CACHE_LOG_ENTER_0(f)                RvLogEnter(logSrc, (logSrc, FUNCTION_NAME " " f))
#define CACHE_LOG_ENTER_1(f, p1)            RvLogEnter(logSrc, (logSrc, FUNCTION_NAME " " f, p1))
#define CACHE_LOG_ENTER_2(f, p1, p2)        RvLogEnter(logSrc, (logSrc, FUNCTION_NAME " " f, p1, p2))
#define CACHE_LOG_ENTER_3(f, p1, p2, p3)    RvLogEnter(logSrc, (logSrc, FUNCTION_NAME " " f, p1, p2, p3))
#define CACHE_LOG_ENTER                     CACHE_LOG_ENTER_0("")


#define CACHE_LOG_LEAVE_0(f)                RvLogLeave(logSrc, (logSrc, FUNCTION_NAME " " f))
#define CACHE_LOG_LEAVE_1(f, p1)            RvLogLeave(logSrc, (logSrc, FUNCTION_NAME " " f, p1))
#define CACHE_LOG_LEAVE_2(f, p1, p2)        RvLogLeave(logSrc, (logSrc, FUNCTION_NAME " " f, p1, p2))
#define CACHE_LOG_LEAVE_3(f, p1, p2, p3)    RvLogLeave(logSrc, (logSrc, FUNCTION_NAME " " f, p1, p2, p3))
#define CACHE_LOG_LEAVE                     CACHE_LOG_LEAVE_0("")


#define CACHE_LOG_ERROR_0(f)                RvLogError(logSrc, (logSrc, FUNCTION_NAME " " f))
#define CACHE_LOG_ERROR_1(f, p1)            RvLogError(logSrc, (logSrc, FUNCTION_NAME " " f, p1))
#define CACHE_LOG_ERROR_2(f, p1, p2)        RvLogError(logSrc, (logSrc, FUNCTION_NAME " " f, p1, p2))
#define CACHE_LOG_ERROR_3(f, p1, p2, p3)    RvLogError(logSrc, (logSrc, FUNCTION_NAME " " f, p1, p2, p3))
#define CACHE_LOG_ERROR_4(f, p1, p2, p3, p4)     RvLogError(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4))
#define CACHE_LOG_ERROR_5(f, p1, p2, p3, p4, p5) RvLogError(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4, p5))

#define CACHE_LOG_EXCEP_0(f)                RvLogExcep(logSrc, (logSrc, FUNCTION_NAME " " f))
#define CACHE_LOG_EXCEP_1(f, p1)            RvLogExcep(logSrc, (logSrc, FUNCTION_NAME " " f, p1))
#define CACHE_LOG_EXCEP_2(f, p1, p2)        RvLogExcep(logSrc, (logSrc, FUNCTION_NAME " " f, p1, p2))
#define CACHE_LOG_EXCEP_3(f, p1, p2, p3)    RvLogExcep(logSrc, (logSrc, FUNCTION_NAME " " f, p1, p2, p3))
#define CACHE_LOG_EXCEP_4(f, p1, p2, p3, p4)     RvLogExcep(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4))
#define CACHE_LOG_EXCEP_5(f, p1, p2, p3, p4, p5) RvLogExcep(logSrc, (logSrc, "[%d]" FUNCTION_NAME " " f, CACHE_TIME, p1, p2, p3, p4, p5))



static RvUint32 gsCacheCtxIdx;
static RvUint64 gsStartTime;


/*RVCOREAPI*/
void RvAresCacheParamsInit(RvAresCacheParams *params) {
    memset(params, 0, sizeof(*params));
}

/*---------------------------- Cache environment methods -------------------------------*/

static
RvAresCacheEnv* RvAresCacheGetEnv() {
    RvAresCacheEnv *ctx = 0;
    RvStatus s;

    s = RvThreadGetVar(gsCacheCtxIdx, 0, (void **)&ctx);
    if(ctx != 0 || s != RV_OK) {
        return ctx;
    }


    s = RvMemoryAlloc(0, sizeof(*ctx), 0, (void **)&ctx);
    if(ctx == 0 || s != RV_OK) {
        return 0;
    }

    s = RvThreadSetVar(gsCacheCtxIdx, ctx, 0);
    if(s != RV_OK) {
        RvMemoryFree(ctx, 0);
        return 0;
    }

    return ctx;
}

static
void RvAresCacheEnvDestruct(RvThread *th, void *ctx, RvUint32 idx) {
    RV_UNUSED_ARG(th);
    RV_UNUSED_ARG(idx);

    RvMemoryFree(ctx, 0);
}

static
void RvAresCacheUpdateTime(void) {
    RvAresCacheEnv *ctx;

    ctx = RvAresCacheGetEnv();
    ctx->time = RvAresGetTime();
}

static
RvUint32 RvAresGetCacheTime(void) {
    RvAresCacheEnv *ctx = RvAresCacheGetEnv();
    return ctx->time;
}

#if RV_LOGMASK != RV_LOGLEVEL_NONE

static
RvLogSource* RvAresCacheGetLogSource(void) {
    RvAresCacheEnv *ctx = RvAresCacheGetEnv();
    if(ctx == 0) {
        return 0;
    }
    return ctx->logSrc;
}

static
void RvAresCacheSetLogInfo(RvLogMgr *logMgr, RvLogSource *logSrc) {
    RvAresCacheEnv *ctx = RvAresCacheGetEnv();
    ctx->logMgr = logMgr;
    ctx->logSrc = logSrc;
}

#define INIT_LOGS \
RvLogSource *logSrc = RvAresCacheGetLogSource()

#define INIT_LOGS1 \
RvLogSource *logSrc = self->logSrc

#else

#define RvAresCacheGetLogSource() (0)
#define RvAresCacheGetLogMgr() (0)
#define RvAresCacheSetLogInfo(logMgr, logSrc) (void)((void)logMgr, logSrc)
#define INIT_LOGS
#define INIT_LOGS1

#endif

#define CACHE_TIME RvAresGetCacheTime()


/* --------------------------- Utilities ----------------------------------*/
#include <ctype.h>

static
void RvStrToLower2(RvChar *dest, const RvChar *str, RvSize_t size) {
    while(size--) {
        *dest++ = (RvChar)tolower((RvInt)*str);
        str++;
    }

    *dest = 0;
}

static
RvInt RvStrnicmp(const RvChar *str1, const RvChar *str2, RvSize_t size) {
    RvSize_t i = 0;
    RvInt res = 0;

    while(!res && (i < size) && *str1 && *str2) {
        res = tolower((RvInt)*str1) - tolower((RvInt)*str2);
        i++;
        str1++;
        str2++;
    }

    return res;
}


/*---------------------------- DLink methods -------------------------------*/
void RvDLinkUnlink(RvDLink *dl) {
    if(dl->prev) {
        dl->prev->next = dl->next;
    }

    if(dl->next) {
        dl->next->prev = dl->prev;
    }
}

void RvDLinkLink(RvDLink *head, RvDLink *dl) {
    dl->prev = head;
    dl->next = head->next;
    if(head->next) {
        head->next->prev = dl;
    }
    head->next = dl;
}



/*--------------------------- CacheCells methods ---------------------------*/

static
RvStatus RvAresCacheCellsConstruct(RvAresCacheCells *self, RvSize_t numCells, RvAresTime minTTL) {
    RvStatus s;
    RvLogMgr *logMgr = 0;


    if(numCells == 0) {
        numCells = RV_DNS_CACHE_NUM_CELLS;
    }

    if(minTTL == 0) {
        self->minTtl = RV_DNS_CACHE_MIN_TTL;
    }


    self->maxTtl = self->minTtl << (numCells - 1);

    numCells += 2;



    self->maxSize = numCells;
    self->curSize = 0;
    self->startTime = RvAresGetCacheTime();

    s = RvMemoryAlloc(0, numCells * sizeof(*self->cells), logMgr, (void **)&self->cells);
    if(s != RV_OK) {
        self->cells = 0;
    }
    return s;
}

static
void RvAresCacheCellsClear(RvAresCacheCells *self) {
    self->curSize = 0;
    self->startTime = RvAresGetCacheTime();
}


static
void RvAresCacheCellsDestruct(RvAresCacheCells *self) {
    RvLogMgr *logMgr = 0;

    if(self->cells) {
        RvMemoryFree(self->cells, logMgr);
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
        RvInt32 curDiff = ttl - times[i];
        curDiff = RvAbs(curDiff);
        if(curDiff < diff) {
            diff = curDiff;
            foundTime = times[i];
        }
    }

    return foundTime;
}


static
RvAresCacheCell* RvAresCacheCellsFindCell(RvAresCacheCells *self, RvAresTime ttl, RvAresTime *pExpirationTime) {
    RvSize_t idx, lidx, ridx;
    RvSize_t dist;
    RvSize_t foundIdx = 0;
    RvAresTime curTime;
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

    curTime = RvAresGetCacheTime() - self->startTime;
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

static
RvBool RvAresCacheCellsRemove(RvAresCacheCells *self,
                              RvAresTime expirationTime,
                              RvAresPage **ppage) {

    RvAresCacheCell *curCell;

    if(self->curSize == 0) {
        return RV_FALSE;
    }

    /* expirationTime == 0 means current time */
    if(expirationTime == 0) {
        expirationTime = RvAresGetCacheTime();
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

#define RvAresCacheCellGetPage(cell) ((cell)->tail)

static
void  RvAresCacheCellAddPage(RvAresCacheCell *cell, RvAresPage *page) {
    if(cell->tail) {
        cell->tail->next = page;
    } else {
        cell->head = page;
    }

    cell->tail = page;
}




/*--------------------------- Page methods ---------------------------*/

static
void RvAresPageReset(RvAresPage *page) {
    RvUint8 *lastByte = page->freeSpace + page->spaceLeft;
    RvSize_t pageSize = lastByte - (RvUint8 *)page;

    page->firstRecord = 0;
    page->next = 0;
    page->maxExpirationTime = 0;
    page->spaceLeft = pageSize - sizeof(*page);
    page->freeSpace = (RvUint8 *)page + sizeof(*page);
}


static
RvStatus RvAresPageAddRdata(RvAresPage *page, RvRdata **pRdata) {
    RvUint8 *pdata;
    RvUint8 *pend;
    RvRdata *rdata = *pRdata;

    pdata = RvAlign(page->freeSpace);
    pend = page->freeSpace + page->spaceLeft;

    if(pdata + rdata->size > pend) {
        return RV_ERROR_INSUFFICIENT_BUFFER;
    }

    memcpy(pdata, rdata, rdata->size);
    rdata = (RvRdata *)pdata;
    page->freeSpace = pdata + rdata->size;
    page->spaceLeft = pend - page->freeSpace;
    rdata->nextPageEntry = page->firstRecord;
    if(rdata->expirationTime > page->maxExpirationTime) {
        page->maxExpirationTime = rdata->expirationTime;
    }
    *pRdata = page->firstRecord = rdata;
    return RV_OK;
}

/*---------------------------------- Page Allocator methods -----------------------------------*/

static
void RvAresPageAllocatorReset(RvAresPageAllocator *self) {
    RvObjPoolReset(&self->pagePool);
}

#if 0
	void *(*pagealloc)(RvSize_t size, void *data);   /* Allocate page of memory. Return pointer to page memory, NULL = failed. */
	void  (*pagefree)(void *ptr, void *data);        /* Free page of memory. */
#endif

static void *pageAlloc(RvSize_t size, void *data) {
    void *p;
    RvStatus s;

    (void)data;
    s = RvMemoryAlloc(0, size, 0, (void **)&p);
    if(s != RV_OK) {
        return 0;
    }

    return p;
}

static void pageFree(void *p, void *data) {
    (void)data;
    RvMemoryFree(p, 0);
}


static
RvStatus RvAresPageAllocatorConstruct(RvAresPageAllocator *self,
                                      RvSize_t pageSize,
                                      RvSize_t minPages,
                                      RvSize_t maxPages,
                                      RvSize_t deltaPages) {

    RvStatus s = RV_OK;
    RvAresPage tmpl;
    RvObjPoolFuncs vt;
    RvObjPool *pool;

    RvAssert(pageSize >= sizeof(RvAresPage));

    vt.objconstruct = 0;
    vt.objconstructdata = 0;
    vt.objdestruct = 0;
    vt.objdestructdata = 0;
    vt.pagealloc = pageAlloc;
    vt.pagefree = pageFree;

    self->pageSize = pageSize;
    pool = RvObjPoolConstruct(&tmpl, &tmpl.poolElem, &vt, pageSize, deltaPages, 0, RV_OBJPOOL_TYPE_DYNAMIC,
            RV_OBJPOOL_SALVAGE_ALLOWED, maxPages, minPages, 50, &self->pagePool);

    if(pool == 0) {
        return RV_ERROR_UNKNOWN;
    }

    return s;
}

static
void RvAresPageAllocatorDestruct(RvAresPageAllocator *self) {
    RvObjPoolDestruct(&self->pagePool);
}

static
RvAresPage* RvAresPageAllocatorAlloc(RvAresPageAllocator *self) {
    RvAresPage *page;
    page = (RvAresPage *)RvObjPoolGetItem(&self->pagePool);
    if(page == 0) {
        return 0;
    }
    page->freeSpace = (RvUint8 *)page + sizeof(*page);
    page->spaceLeft = self->pageSize - sizeof(*page);
    page->firstRecord = 0;
    page->next = 0;
    return page;
}


static
void RvAresPageAllocatorFree(RvAresPageAllocator *self, RvAresPage *page) {
    RvObjPoolReleaseItem(&self->pagePool, page);
}



/*-------------------------------------- Hash Function --------------------------*/

typedef struct {
    RvUint32 curHash;
} RvHashVal;

#define RvHashValStart(self) ((self)->curHash = 5381)

static
void RvHashValAccumulate(RvHashVal *self, RvUint8 *buf, RvSize_t size) {
    RvUint32 hash = self->curHash;
    RvUint8  *cur = buf + size;

    while(--cur >= buf) {
        hash = ((hash << 5) + hash) + (*cur);
    }

    self->curHash = hash;
}


#define RvHashValGet(self) ((self)->curHash % 0x7fffffff)

static
void RvAresHashClear(RvAresHash *self) {
  RvAresHashLink *cur = self->buckets;      /* points to the first cell in the hash */
  RvAresHashLink *last = cur + self->size;  /* points to the cell after last cell in the hash */

  /* Initialize 'next' and 'prev' pointer of each cell to point to the cell itself */
  for(; cur < last; cur++) {
      cur->next = cur->prev = cur;
  }

}

/*************************************************************************************
 * RvStatus RvAresHashConstruct(RvAresHash *self, RvUint8 *rawmem, RvSize_t hashSize)
 *************************************************************************************
 *
 * Constructs hash used for cache entries lookups
 *
 *
 *
 *  self     - points to the object being constructed
 *  rawmem   - if not 0, the hash will be constructed on this memory
 *             (in this case, size of this buffer should be at least hashSize * sizeof(void *)
 *             Otherwise, memory will be allocated using RvMemoryAlloc
 *  hashSize - size of hash table
 */
static
RvStatus RvAresHashConstruct(RvAresHash *self, RvUint8 *rawmem, RvSize_t hashSize) {
    RvLogMgr *logMgr = 0;
    RvStatus s;

    if(rawmem == 0) {
        RvSize_t totalSize = sizeof(self->buckets[0]) * hashSize;

        s = RvMemoryAlloc(0, totalSize, logMgr, (void **)&rawmem);
        if(s != RV_OK) {
            return s;
        }
        self->ownsMem = RV_TRUE;
    } else {
        self->ownsMem = RV_FALSE;
    }

    self->buckets = (RvAresHashLink *)rawmem;
    self->size    = hashSize;

    RvAresHashClear(self);
    return RV_OK;
}

static
void RvAresHashDestruct(RvAresHash *self) {
    RvLogMgr *logMgr = 0;

    if(self->ownsMem) {
        RvMemoryFree(self->buckets, logMgr);
    }
}



/* Finds appropriate entry in the hash table. Expired records aren't removed */
static
RvRdata* RvAresHashFindAux(RvAresHash *self, RvSize_t t, RvChar *name, RvSize_t nameSize, RvAresHashInsertPoint *insertPoint) {
    RvHashVal hv;
    RvSize_t  idx;
    RvAresHashLink  *cur;
    RvAresHashLink  *first;
    RvRdata *curRec = 0;
    RvUint8 type = (RvUint8)t;

    RvHashValStart(&hv);
    RvHashValAccumulate(&hv, (RvUint8 *)name, nameSize);
    RvHashValAccumulate(&hv, &type, 1);
    idx = RvHashValGet(&hv) % self->size;
    if(insertPoint != 0) {
        *insertPoint = idx;
    }

    first = &self->buckets[idx];
    for(cur = first->next; cur != first; cur = cur->next) {
        curRec = RvObjectFromField(RvRdata, hashLink, cur);
        if(nameSize == curRec->nameSize && type == curRec->type && !RvStrnicmp(name, curRec->name, nameSize)) {
            break;
        }
    }

    if(cur == first) {
        return 0;
    }

    return curRec;
}

static
void RvAresHashRemove(RvAresHash *self, RvRdata *data) {
    RvAresHashLink *entry = &data->hashLink;

    RV_UNUSED_ARG(self);
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->prev = entry->next = 0;
}


static
RvRdata* RvAresHashFind(RvAresHash *self, RvSize_t t, RvChar *name, RvSize_t nameSize, RvAresHashInsertPoint *insertPoint) {

    RvAresTime curTime = RvAresGetCacheTime();
    RvRdata *curRec;

    curRec = RvAresHashFindAux(self, t, name, nameSize, insertPoint);
    if(curRec == 0) {
        return 0;
    }

    if(curRec->expirationTime < curTime) {
        RvAresHashRemove(self, curRec);
        return 0;
    }

    return curRec;
}

static
void RvAresHashAdd(RvAresHash *self, RvAresHashInsertPoint insertionPoint, RvRdata *data) {
    RvAresHashLink *entry = &data->hashLink;
    RvAresHashLink *first = &self->buckets[insertionPoint];

    RvDLinkLink(first, entry);
}

/* Removes all entries currently on the page from hash */
static
void RvAresHashRemovePage(RvAresHash *self, RvAresPage *page) {
    RvRdata *cur = page->firstRecord;

    while(cur) {
        if(RvRdataIsValid(cur)) {
            RvAresHashRemove(self, cur);
        }

        cur =  cur->nextPageEntry;
    }
}


static
RvStatus RvAresCacheConstruct(RvAresCache *self, RvAresCacheParams *params) {
#define ALLOCATOR_CONSTRUCTED 1
#define HASH_CONSTRUCTED      2
#define CELLS_CONSTRUCTED     4

    RvStatus s;
    RvUint32 stage = 0;
    RvSize_t pageSize;
    RvSize_t nMinPages;
    RvSize_t nMaxPages;
    RvSize_t nDeltaPages;
    RvSize_t hashSize;

    RvAresCacheUpdateTime();

    pageSize = params->pageSize;

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
    } else if(nMaxPages != 0 && nMaxPages == 0) {
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


    hashSize = params->hashSize;
    if(hashSize == 0) {
        hashSize = RV_DNS_CACHE_HASH_SIZE;
    }
    hashSize = RvHashNextPrime(hashSize);


    s = RvAresCacheCellsConstruct(&self->cells, params->cellsNum, params->minTTL);
    if(s != RV_OK) goto failure;
    stage |= CELLS_CONSTRUCTED;


    s = RvAresPageAllocatorConstruct(&self->pageAllocator, pageSize, nMinPages, nMaxPages, nDeltaPages);
    if(s != RV_OK) goto failure;
    stage |= ALLOCATOR_CONSTRUCTED;
    RvAresHashConstruct(&self->hash, 0, hashSize);
    if(s != RV_OK) goto failure;
    stage |= HASH_CONSTRUCTED;
    self->expiredPages.first = 0;
    self->expiredPages.last  = 0;

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


    return s;

#undef ALLOCATOR_CONSTRUCTED
#undef HASH_CONSTRUCTED
#undef CELLS_CONSTRUCTED
}

static
void RvAresCacheDestruct(RvAresCache *self) {
    RvAresHashDestruct(&self->hash);
    RvAresPageAllocatorDestruct(&self->pageAllocator);
    RvAresCacheCellsDestruct(&self->cells);
}

static
void RvAresCacheClear(RvAresCache *self) {
    self->expiredPages.first = 0;
    self->expiredPages.last  = 0;
    RvAresHashClear(&self->hash);
    RvAresPageAllocatorReset(&self->pageAllocator);
    RvAresCacheCellsClear(&self->cells);

}

static
void RvAresCacheBookkeeping(RvAresCache *self) {
    RvAresTime curTime = RvAresGetCacheTime();
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
        if(self->expiredPages.last != 0) {
            self->expiredPages.last->next = expired;
        } else {
            self->expiredPages.first = expired;
        }

        while(expired->next) {
            expired = expired->next;
        }

        self->expiredPages.last = expired;
    }

}

static
RvAresPage* RvAresCacheAllocPage(RvAresCache *self) {
    RvAresPage *page;

    page = RvAresPageAllocatorAlloc(&self->pageAllocator);
    /* No free pages left */
    if(page != 0) {
        RvAresPageReset(page);
        return page;
    }

    page = self->expiredPages.first;

    /* No expired pages left */
    if(page == 0) {
        return 0;
    }

    self->expiredPages.first = page->next;
    /* last page */
    if(page == self->expiredPages.last) {
        self->expiredPages.last = 0;
    }

    /* Remove all entries on this page from hash */
    RvAresHashRemovePage(&self->hash, page);
    RvAresPageReset(page);
    return page;
}


/* Perform cache lookup for <inName, type> tuple
 * Returns:
 *  0 - if nothing appropriate exists in the cache
 *  pointer to appropriate cached record otherwise. This record may contain real data
 *  or negative caching information
 */
static
RvRdata* RvAresCacheFind(RvAresCache *self,
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
    INIT_LOGS;

    RvStrToLower2(name, inName, nameSize);

    if(*last == '.') {
        *last = '\0';
    }

    *(last + 1) = 0;

    /* Lookup this name in hash according to type */
    CACHE_LOG_DEBUG_2("Trying <type=%d, name=%s>", type, nameBuf);

    curRec = RvAresHashFind(hash, type, name, nameSize, insertPoint);

    if(curRec == 0) {
        /* If name doesn't exists, try to find appropriate NXDOMAIN record */

        CACHE_LOG_DEBUG_1("Doesn't exist, trying <NXDOMAIN, name=%s> ", nameBuf);
        curRec = RvAresHashFind(hash, T_NXDOMAIN, name, nameSize, 0);
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
        curRec = RvAresHashFind(hash, T_CNAME, name, nameSize, 0);

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
        RvStrToLower2(name, alias, nameSize);

        CACHE_LOG_DEBUG_2("Found CNAME record, trying <type=%d, name=%s>", type, name);
        curRec = RvAresHashFind(hash, type, name, nameSize, 0);
        if(curRec == 0) {
            /* Try to find NXDOMAIN record */
            CACHE_LOG_DEBUG_1("Doesn't exist, trying <type=NXDOMAIN, name=%s>", nameBuf);
            curRec = RvAresHashFind(hash, T_NXDOMAIN, name, nameSize, 0);
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
RvStatus RvAresInternA(RvAresCacheCtx *self, RvDnsData *record) {
    const RvAddressIpv4 *paddr;
    RvUint8 *pRec; /* points to the current position in extension record */

    pRec = (RvUint8 *)RvAlign32(self->cur);
    if(pRec + 4 > self->end) {
        return RV_ERROR_INSUFFICIENT_BUFFER;
    }

    paddr = RvAddressGetIpv4(&record->data.hostAddress);
    *(RvUint32 *)pRec = RvAddressIpv4GetIp(paddr);
    self->cur = pRec + 4;
    return RV_OK;
}

#if RV_NET_TYPE & RV_NET_IPV6

/* Format of serialized AAAA extension record
 * 16 - bytes wide IP
 */
RvStatus RvAresInternAAAA(RvAresCacheCtx *self, RvDnsData *record) {

    const RvAddressIpv6 *paddr = RvAddressGetIpv6(&record->data.hostAddress);

    if(self->cur + 16 > self->end) {
        return RV_ERROR_INSUFFICIENT_BUFFER;
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
        return RV_ERROR_INSUFFICIENT_BUFFER;
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
      return RV_ERROR_INSUFFICIENT_BUFFER;
    }

    *(RvUint16 *)pRec = srv->port;
    pRec += 2;
    *(RvUint16 *)pRec = srv->priority;
    pRec += 2;
    *(RvUint16 *)pRec = srv->weight;
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
        return RV_ERROR_INSUFFICIENT_BUFFER;
    }

    *(RvUint16 *)pRec = naptr->order;
    pRec += 2;

    *(RvUint16 *)pRec = naptr->preference;
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
RvStatus RvAresInternStart(RvAresCacheCtx *self, RvDnsData *record) {
#define FUNCTION_NAME "RvAresInternStart"

    RvRdata *rdata;
    RvSize_t nameSize = strlen(record->ownerName);
    RvUint8 *pRec;
    RvInt    dataType;
    INIT_LOGS;

    if(nameSize >= 256) {
        return RV_ERROR_BADPARAM;
    }

    pRec = RvAlign(self->cur);

    if(pRec + sizeof(RvRdata) + nameSize > self->end) {
        return RV_ERROR_INSUFFICIENT_BUFFER;
    }


    self->minTTL = record->ttl;
    rdata = (RvRdata *)pRec;
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

    if(rdata->type == T_A || rdata->type == T_AAAA){
        rdata->idx = 0;
    } else {
        rdata->idx = -1;
    }

    rdata->nRecords = 0;
    rdata->nameSize = (RvUint8)nameSize;
    pRec = (RvUint8 *)&rdata->name[0];
    strcpy((RvChar *)pRec, record->ownerName);
    pRec += nameSize + 1;
    self->cur = pRec;
    self->rdata = rdata;
    return RV_OK;

#undef FUNCTION_NAME
}

RvStatus RvAresCacheCtxInit(RvAresCacheCtx *ctx) {
    ctx->rdata = 0;
    ctx->cur = ctx->start = ctx->buf;
    ctx->end   = ctx->buf + sizeof(ctx->buf) - sizeof(RvAresPage);
    ctx->minTTL = 0xffffffff;
    ctx->type  = 0;
    return RV_OK;
}


static
RvStatus RvAresCacheFinishCaching(RvAresCache *self, RvAresCacheCtx *ctx) {
#undef FUNCTION_NAME
#define FUNCTION_NAME "RvAresCacheFinishCaching"

    RvAresHashInsertPoint insertPoint;
    RvRdata *rec = ctx->rdata;
    RvRdata *oldRec;
    RvAresHash *hash = &self->hash;
    RvStatus s;
    RvAresPage *page = 0;
    RvChar *name = rec->name;
    RvSize_t nameSize = rec->nameSize;
    RvAresCacheCell *cell;
    RvUint32 ttl;
    INIT_LOGS;

    ttl = ctx->minTTL;
    CACHE_LOG_DEBUG_4("%s caching recordset owner = %s, type = %d, ttl = %d", (rec->negative ? "Negatively" : "Positively"), rec->name, rec->type, ttl);

    oldRec = RvAresCacheFind(self, rec->type, name, nameSize, &insertPoint);

    /* Record with this <name,type> already exists in the data base */
    if(oldRec) {
        CACHE_LOG_DEBUG_0("Found old recordset, removing");
        RvAresHashRemove(hash, oldRec);
    }

    rec->size = (RvUint16)(ctx->cur - (RvUint8 *)rec);

    cell = RvAresCacheCellsFindCell(&self->cells, ctx->minTTL, &rec->expirationTime);
    if(cell == 0) {
        CACHE_LOG_ERROR_1("(ttl=%d) - failed to find cache cell", ttl);
        return RV_ERROR_UNKNOWN;
    }

    CACHE_LOG_DEBUG_2("for ttl = %d found cell with expiration = %d", ttl, cell->expirationTime);

    page = RvAresCacheCellGetPage(cell);

    if(page == 0) {
        /* no active page in this cell, try to allocate new page */
        page = RvAresCacheAllocPage(self);
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
        page = RvAresCacheAllocPage(self);
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
    CACHE_LOG_DEBUG_3("Recordset owner = %s, type = %d, ttl = %d cached", rec->name, rec->type, ttl);

    return s;

}


static
RvStatus RvAresCacheRecord(RvAresCache *self,  RvAresCacheCtx *ctx, RvDnsData *record) {
    RvStatus s;
    RvAresCacheCtx  cnameCTX;

    (void)self;

    if(record->dataType == T_CNAME) {
        RvAresCacheCtxInit(&cnameCTX);
        ctx = &cnameCTX;
    }

    /* Interning first record */
    if(ctx->rdata == 0) {
        s = RvAresInternStart(ctx, record);
        if(s != RV_OK) {
            return s;
        }
    } else if(ctx->type != record->dataType) {
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
          s = RvAresCacheFinishCaching(self, ctx);
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

    if(s != RV_OK) {
        return s;
    }

    return s;
}



/*----------------------------------  Cache Daemon ---------------------------*/
#include "rvselect.h"
#include "rvsocket.h"
#include "rvares.h"

typedef RvStatus (*RvAresCacheDeserializer)(RvDnsData *data, RvUint8 **pBufStart);


static
RvStatus RvAresCacheExternRdata(RvDnsData *data, RvUint8 **pBufStart) {
    RvRdata *rdata = (RvRdata *)*pBufStart;
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
    return RV_OK;
}


static
RvStatus RvAresCacheExternA(RvDnsData *data, RvUint8 **pBufStart) {
    RvUint8 *pBuf = *pBufStart;
    RvUint32 ipAddr;
    RvAddress *pAddr = &data->data.hostAddress;

    pBuf = RvAlign32(pBuf);

    ipAddr = *(RvUint32 *)pBuf;

    RvAddressConstructIpv4(pAddr, ipAddr, 0);
    *pBufStart = pBuf + 4;
    return RV_OK;
}

#if RV_NET_TYPE & RV_NET_IPV6


static
RvStatus RvAresCacheExternAAAA(RvDnsData *data, RvUint8 **pBufStart) {
    RvAddress *pAddr = &data->data.hostAddress;

    RvAddressConstructIpv6(pAddr, *pBufStart, 0, 0);
    *pBufStart += 16;
    return RV_OK;
}

#endif

static
RvStatus RvAresCacheExternSRV(RvDnsData *data, RvUint8 **pBufStart) {
    RvUint8 *pBuf = *pBufStart;
    RvDnsSrvData *srv = &data->data.dnsSrvData;

    pBuf = RvAlign16(pBuf);
    srv->port = *(RvUint16 *)pBuf;
    pBuf += 2;
    srv->priority = *(RvUint16 *)pBuf;
    pBuf += 2;
    srv->weight = *(RvUint16 *)pBuf;
    pBuf += 2;
    strcpy(srv->targetName, (RvChar *)(pBuf + 1));
    pBuf += *pBuf + 2;
    *pBufStart = pBuf;

    return RV_OK;

}


static
RvStatus RvAresCacheExternNAPTR(RvDnsData *data, RvUint8 **pBufStart) {
    RvUint8 *pBuf = *pBufStart;
    RvDnsNaptrData *naptr = &data->data.dnsNaptrData;

    pBuf = RvAlign16(pBuf);

    naptr->order = *(RvUint16 *)pBuf;
    pBuf += 2;
    naptr->preference = *(RvUint16 *)pBuf;
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
    return RV_OK;
}

static
RvStatus RvAresCacheExternCNAME(RvDnsData *data, RvUint8 **pBufStart) {
    RvSize_t aliasSize;
    RvUint8 *pBuf = *pBufStart;
    RvDnsCNAMEData *cname = &data->data.dnsCnameData;

    aliasSize = *pBuf++;
    memcpy(cname->alias, pBuf, aliasSize + 1);
    pBuf += aliasSize + 1;
    *pBufStart = pBuf;
    return RV_OK;
}




#if 0

typedef RvStatus (*RvDnsNewRecordCB) (
                                      IN  void*               context,
                                      IN  RvUint32            queryId,
                                      IN  RvDnsData*          dnsData);

#endif

typedef struct _RvAresCacheDQuery RvAresCacheDQuery;

struct _RvAresCacheDQuery {
    RvAresCacheDQuery *next;
    RvUint32           qid;
    void              *ctx;
    RvBool             inCallback;
    RvDnsNewRecordCB   cb;
    RvRdata            rdata;      /* Variable length field - should be last field in this structure */
};

struct _RvAresCacheD {
    RvLock             lock;
    RvAresCache        cache;
    RvTimer            timer;
    RvSize_t           nPostponed;     /* number of postponed queries for statistical purposes */
    RvSize_t           maxPostponed;   /* maximal number of postponed queries for statistical purposes */
    RvBool             needRaiseEvent; /* postponed event should be raised */
    RvTimerQueue      *tquee;
    RvLogMgr          *logMgr;
    RvLogSource       *logSrc;
    RvAresCacheClt    *firstClt;

    /* Debugging */
    RvSize_t           sentResults;
    RvSize_t           acceptedResults;
    RvEHD              *ehd;
    /* Active queries list */
    RvAresCacheDQuery *activeQuery;
};


/* Postponed queries handling */

static
RvStatus RvAresCacheDRemoveQuery(RvAresCacheD *self, RvUint32 qid, RvAresCacheDQuery **pq) {

    RvAresCacheDQuery *cur = self->activeQuery;
    RvAresCacheDQuery *prev = 0;

    while(cur && cur->qid != qid) {
        prev = cur;
        cur = cur->next;
    }

    if(cur == 0) {
        return RV_DNS_ERROR_CACHE_NOTFOUND;
    }

    if(cur->inCallback) {
        return RV_DNS_ERROR_INCALLBACK;
    }

    if(pq) {
        *pq = cur;
    }

    if(prev == 0) {
        self->activeQuery = cur->next;
    } else {
        prev->next = cur->next;
    }

    return RV_OK;
}

static
void RvAresCacheDAnalyzePacket(RvAresCacheD *self, RvAresCacheDQuery *query) {
    RvSize_t nRecords;
    RvSize_t i;
    RvAresCacheDeserializer deserialize;
    RvUint8 *curPtr;
    RvDnsData data;
    RvDnsNewRecordCB onNewRecordCB;
    void *ctx;
    RvLogMgr *logMgr = 0;
    RvStatus s;
    RvBool continueCallbacks = RV_TRUE;
    RvRdata *rdata = &query->rdata;
    RvUint32 qid = query->qid;


    nRecords = rdata->nRecords;
    if(nRecords < 1) {
        /* No packets, shouldn't happen */
        return;
    }

    onNewRecordCB = query->cb;
    ctx = query->ctx;
    query->inCallback = RV_TRUE;

    curPtr = (RvUint8 *)rdata;
    RvAresCacheExternRdata(&data, &curPtr);

    if(data.dataType == RV_DNS_STATUS_TYPE) {
        data.recordNumber = 0;
        RvLockRelease(&self->lock, logMgr);
        onNewRecordCB(ctx, qid, &data);
    } else {
        switch(data.dataType) {
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
            /* Shouldn't happen */
            return;
        }

        RvLockRelease(&self->lock, logMgr);

        data.recordNumber = 0;

        for(i = 0; i < nRecords && continueCallbacks; i++) {
            data.recordNumber++;
            deserialize(&data, &curPtr);
            /* if the callback returns error - don't continue with callbacks,
             *  probably, object that accepts callbacks is destructed
             */
            s = onNewRecordCB(ctx, qid, &data);
            if(s != RV_OK) {
                continueCallbacks = RV_FALSE;
            }
        }


        if(continueCallbacks) {
            data.dataType = RV_DNS_ENDOFLIST_TYPE;
            onNewRecordCB(ctx, qid, &data);
        }
    }

    /* This function assumes it's called in locked state of 'self' and should left it in 'locked' state */

    RvLockGet(&self->lock, logMgr);
    query->inCallback = RV_FALSE;
}

static
RvBool RvAresCacheDOnTimer(void *ctx, RvTimer *timer) {
    RvAresCacheD *self = (RvAresCacheD *)ctx;
    RvLogMgr *logMgr = 0;

    RV_UNUSED_ARG(timer);

    RvLockGet(&self->lock, logMgr);

    while(self->activeQuery) {
        RvAresCacheDQuery *q = self->activeQuery;
        self->activeQuery = q->next;
        self->nPostponed--;
        RvAresCacheDAnalyzePacket(self, q);
        self->acceptedResults++;
    }


    self->needRaiseEvent = RV_TRUE;
    RvLockRelease(&self->lock, logMgr);
    return RV_FALSE;
}

static
RvStatus RvAresCacheDAddQuery(RvAresCacheD *self, RvAresCacheDQuery *q,  RvUint32 qid, RvRdata *rdata, RvDnsNewRecordCB cb, void *cookie) {
#define logSrc self->logSrc
#undef FUNCTION_NAME
#define FUNCTION_NAME "RvAresCacheDAddQuery"
    RvStatus s;

    q->cb = cb;
    q->ctx = cookie;
    q->qid = qid;
    q->inCallback = RV_FALSE;
    q->next = self->activeQuery;
    self->activeQuery = q;
    self->nPostponed++;
    self->maxPostponed = RvMax(self->nPostponed, self->maxPostponed);
    memcpy(&q->rdata, rdata, rdata->size);
    if(!self->needRaiseEvent) {
        CACHE_LOG_DEBUG_2("No need to trigger postponed evants handling (nPostponed=%d, maxPostponed=%d)", 
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
#undef logSrc
}







static
RvStatus RvAresCacheDConstruct(RvAresCacheD   *self,
                               RvTimerQueue   *tqueue,
                               RvAresCacheClt *clt,
                               RvAresCacheParams *params) {

#undef FUNCTION_NAME
#define FUNCTION_NAME "RvAresCacheDConstruct"
#define CACHE_CONSTRUCTED  1
#define LOCK_CONSTRUCTED   4
#define EHD_CONSTRUCTED    8


    RvStatus s;
    RvUint32  stage = 0;
    RvAresCache *cache = 0;
    RvLogSource *logSrc;
    RvLogMgr    *logMgr;

    logMgr = self->logMgr = clt->logMgr;
    logSrc = self->logSrc = &clt->logSrc;
    RvAresCacheSetLogInfo(self->logMgr, self->logSrc);

    CACHE_LOG_ENTER_3("(self = %p, tqueue = %p, logMgr = %p)", self, tqueue, logMgr);

    s = RvLockConstruct(logMgr, &self->lock);
    if(s != RV_OK) goto failure;
    s |= LOCK_CONSTRUCTED;
    self->tquee = tqueue;
    self->nPostponed = 0;
    self->maxPostponed = 0;
    self->needRaiseEvent = RV_TRUE;
    cache = &self->cache;
    s = RvAresCacheConstruct(cache, params);
    if(s != RV_OK) goto failure;
    stage |= CACHE_CONSTRUCTED;

    s = RvEHDNew(&self->ehd, logMgr);
    if(s != RV_OK) {
        goto failure;
    }

    stage |= EHD_CONSTRUCTED;

    self->sentResults = 0;
    self->acceptedResults = 0;
    self->firstClt = 0;
    self->activeQuery = 0;

    CACHE_LOG_LEAVE;

    return s;

failure:
    if(stage & CACHE_CONSTRUCTED) {
        RvAresCacheDestruct(cache);
    }

    if(stage & LOCK_CONSTRUCTED) {
        RvLockDestruct(&self->lock, logMgr);
    }


    CACHE_LOG_ERROR_1("failed: %d", s);
    return s;

#undef CACHE_CONSTRUCTED
#undef LOCK_CONSTRUCTED
#undef FUNCTION_NAME
}

static
void RvAresCacheDAddClt(RvAresCacheD *self, RvAresCacheClt *env) {
    RvLockGet(&self->lock, self->logMgr);

    /* First client is always added directly by CacheDConstruct, so here I don't treat
     * case when self->firstClt == 0
     */

    env->next = self->firstClt;
    self->firstClt = env;

    RvLockRelease(&self->lock, self->logMgr);
}

RvStatus RvAresCacheDDestruct(RvAresCacheD *self) {
    RvLogMgr *logMgr = 0;
    RvStatus  s = RV_OK;

    RvAresCacheDestruct(&self->cache);

    if(self->ehd) {
        RvEHDDelete(self->ehd);
    }

    RvLockDestruct(&self->lock, logMgr);
    return s;
}

static
RvStatus RvAresCacheDNew(RvAresCacheD **pSelf, RvSelectEngine *seli, RvAresCacheClt *clt, RvAresCacheParams *params) {
    RvStatus s = RV_OK;
    RvSize_t totalSize;
    RvTimerQueue *tqueue;


    totalSize = sizeof(**pSelf);

    s = RvMemoryAlloc(0, totalSize, clt->logMgr, (void **)pSelf);

    if(s != RV_OK) {
        return s;
    }

    RvSelectGetTimeoutInfo(seli, 0, &tqueue);

    s = RvAresCacheDConstruct(*pSelf, tqueue, clt, params);
    if(s != RV_OK) {
        RvMemoryFree(*pSelf, clt->logMgr);
    }

    return s;
}

/*RVCOREAPI*/
void RvAresCacheDDelete(RvAresCacheD *self) {
    RvLogMgr *logMgr = 0;

    RvAresCacheDDestruct(self);
    RvMemoryFree(self, logMgr);
}


/* Actually, this function doesn't use at all inner state of 'self',
 *   all state information needed for this function is kept in the 'ctx',
 *   so it doesn't lock 'self' object.
 */
RvStatus RvAresCacheDRecord(RvAresCacheD *self, RvAresCacheCtx *ctx, RvDnsData *record) {
#define logSrc self->logSrc
#define FUNCTION_NAME "RvAresCacheDRecord"

    RvStatus s;

    CACHE_LOG_ENTER_3("(self=%p, owner=%s, type=%d)", self, record->ownerName, record->dataType);

    (void)RvLockGet(&self->lock, self->logMgr);

    RvAresCacheUpdateTime();
    RvAresCacheBookkeeping(&self->cache);

    s = RvAresCacheRecord(&self->cache, ctx, record);
    if(s != RV_OK) {
        CACHE_LOG_ERROR_4("(self=%p, owner=%s, type=%d) = %d, failed to cache record", self, record->ownerName, record->dataType, s);
    } else {
        CACHE_LOG_LEAVE_3("(self=%p, owner=%s, type=%d)", self, record->ownerName, record->dataType);
    }

    (void)RvLockRelease(&self->lock, self->logMgr);

    return s;
#undef FUNCTION_NAME
#undef logSrc
}


static
RvRdata* RvAresEhdFind(RvEHD *self, RvUint32 qtype, const RvChar *name, RvSize_t nameSize, RvUint8 *buf, RvSize_t bufSize) {
    RvSize_t rdataSize = bufSize;
    RvUint8 *pBuf;
    RvSize_t spaceLeft;
    RvStatus s;
    RvRdata *rdata = (RvRdata *)buf;
    RvInt nAns = 0;

    memset(rdata, 0, rdataSize);
    rdata->type = (RvUint16)qtype;
    rdata->nameSize = (RvUint8)nameSize;
    pBuf = (RvUint8 *)&rdata->name[0];
    memcpy(pBuf, name, nameSize);
    pBuf += nameSize;
    *pBuf++ = 0;

    spaceLeft = rdataSize - (pBuf - (RvUint8 *)rdata);
    s = RvEHDFind(self, rdata->name, rdata->type, pBuf, &spaceLeft, &nAns);
    if(s != RV_OK) {
        return 0;
    }

    rdata->size = (RvUint16)((pBuf + spaceLeft) - buf);
    rdata->nRecords = (RvUint8)nAns;
    return rdata;
}

#define RV_EHD_BUFSIZE 1024


/* RvStatus RvAresCacheDFind(RvAresCacheD *self, RvUint32 queryType, const RvChar *name,
 *                            RvSize_t nameSize, RvBool   asyncQuery, RvDnsNewRecordCB cb,
 *                            void *cbCtx, RvUint32 qid, RvUint8 *qbuf, RvSize_t *pQbufSize)
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
 * asyncQuery  - if true the query results will be delivered by callback mechanism (not inline)
 * cb          - callback
 * cbCtx       - user context that will be passed to 'cb'
 * qid         - query id. This id may be used lately to cancel this query.
 * qbuf        - buffer used for keeping information related to this query. Should be valid
 *               as long as the query is active, e.g. it shouldn't be released before query is canceled
 *               or callback is called.
 * pQbufSize   - input/output. As input parameter *pQbufSize holds the length of qbuf.
 *               As output parameter holds minimal size of qbuf needed
 *
 * Return value:
 *   RV_OK - record is found in positive cache positive cache and results will be reported
 *           using callback mechanism
 *
 *   RV_ERROR_INSUFFICIENT_BUFFER - record was found in positive cache, but qBuf is too
 *      small to keep query-related information. In this case *pQbufSize will hold minimal size
 *      of qbuf.
 *
 *   RV_DNS_ERROR_NOTFOUND - record was found in negative cache. No callbacks will be called in this case.
 *
 *   RV_DNS_ERROR_CACHE_NOTFOUND - record wasn't found in cache.
 */
RvStatus RvAresCacheDFind(RvAresCacheD *self,
                          RvUint32 queryType,
                          const RvChar *name,
                          RvSize_t nameSize,
                          RvChar  **domainSuffixes,
                          RvSize_t  nDomains,
                          RvUint32 *domainSuffixesMask,
                          RvDnsNewRecordCB cb,
                          void *cbCtx,
                          RvUint32 qid,
                          RvUint8 *qbuf,
                          RvSize_t *pQbufSize) {
#define FUNCTION_NAME "RvAresCacheDFind"
#define logSrc self->logSrc
#define logMgr self->logMgr

    RvStatus s = RV_OK;
    RvRdata  *rdata = 0;
    RvRdata  *negRdata = 0;
    RvUint8  *qBufEnd;
    RvSize_t  qBufSize = *pQbufSize;
    RvAresCacheDQuery *q;
    RvUint8 buf[RV_EHD_BUFSIZE + sizeof(RvAlignmentType)];
    RvChar fullName[RV_DNS_MAX_NAME_LEN + 1];
    RvUint32 mask;
    RvUint32 outMask = 0;
    RvUint32 bit = 1;
    RvInt i;
    RvChar *suffixes[32];
    RvSize_t querySize;


    CACHE_LOG_ENTER_3("(self=%p, qtype=%d, name=%s)", self, queryType, name);

    RvAresCacheSetLogInfo(logMgr, logSrc);

    RvAresCacheUpdateTime();

    RvLockGet(&self->lock, logMgr);

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

        if(self->ehd != 0) {
            rdata = (RvRdata *)RvAlign(buf);
            rdata = RvAresEhdFind(self->ehd, queryType, fullName, fullNameSize, (void *)rdata, RV_EHD_BUFSIZE);
        }

        if(rdata != 0) {
            break;
        }

        rdata = RvAresCacheFind(&self->cache, queryType, fullName, fullNameSize, 0);

        if(rdata != 0) {
            if(rdata->negative || rdata->type == T_NXDOMAIN) {
                CACHE_LOG_DEBUG_2("(qtype=%d, name=%s), found in negative cache", queryType, fullName);
                negRdata = rdata;
                continue;
            } else {
                break;
            }
        }

        outMask |= bit;
        CACHE_LOG_DEBUG_2("(qtype=%d, name=%s), not found", queryType, fullName);
    }

    if(rdata == 0) {
        rdata = negRdata;
    }

    if((rdata == 0 || rdata->negative || rdata->type == T_NXDOMAIN) && outMask != 0) {
        *domainSuffixesMask = outMask;
        RvLockRelease(&self->lock, logMgr);
        return RV_DNS_ERROR_CACHE_NOTFOUND;
    }


    qBufEnd = qbuf + qBufSize;
    querySize = RV_OFFSETOF(RvAresCacheDQuery, rdata) + rdata->size;
    q = (RvAresCacheDQuery *)RvAlign(qbuf);
    if((RvUint8 *)q + querySize > qBufEnd) {
        *pQbufSize = querySize + RV_ALIGN_SIZE;
        RvLockRelease(&self->lock, logMgr);
        return RV_ERROR_INSUFFICIENT_BUFFER;
    }

    
    s = RvAresCacheDAddQuery(self, q, qid, rdata, cb, cbCtx);
    self->sentResults++;

    RvLockRelease(&self->lock, logMgr);

    CACHE_LOG_LEAVE;
    return s;

#undef FUNCTION_NAME
#undef logSrc
#undef logMgr
}


/*RVCOREAPI*/
RvStatus RvAresCacheDCancelQuery(RvAresCacheD *self, RvUint32 qid) {
    RvLogMgr *logMgr = 0;
    RvStatus s;

    RvLockGet(&self->lock, logMgr);
    s = RvAresCacheDRemoveQuery(self, qid, 0);
    RvLockRelease(&self->lock, logMgr);

    return s;
}

/*RVCOREAPI*/
RvStatus RvAresCacheDStartCaching(RvAresCacheD *self, RvAresCacheCtx *ctx) {

    (void)self;
    RvAresCacheCtxInit(ctx);
    return RV_OK;
}

/*RVCOREAPI*/
RvStatus RvAresCacheDFinishCaching(RvAresCacheD *self, RvAresCacheCtx *ctx) {
    RvStatus s = RV_OK;

    (void)RvLockGet(&self->lock, self->logMgr);

    RvAresCacheUpdateTime();
    RvAresCacheBookkeeping(&self->cache);

    s = RvAresCacheFinishCaching(&self->cache, ctx);

    (void)RvLockRelease(&self->lock, self->logMgr);

    return s;
}

void RvAresCacheDClear(RvAresCacheD *self) {
    RvLockGet(&self->lock, self->logMgr);
    RvAresCacheClear(&self->cache);
    RvLockRelease(&self->lock, self->logMgr);
}


static
void RvAresCacheDRemoveClt(RvAresCacheD *self, RvAresCacheClt *env) {
    RvAresCacheClt *cur, *prev;

    RvLockGet(&self->lock, self->logMgr);

    for(prev = 0, cur = self->firstClt; cur != 0 && cur != env; prev = cur, cur = cur->next);

    if(cur != 0) {
        if(prev == 0) {
            self->firstClt = cur->next;
        } else {
            prev->next = cur->next;
        }

        if(self->firstClt == 0) {
            RvLockRelease(&self->lock, self->logMgr);
            RvAresCacheDDelete(self);
            if(self == gsGlobalCache) {
                gsGlobalCache = 0;
            }
            return;
        } else {
            if(self->logMgr == env->logMgr) {
                self->logMgr = self->firstClt->logMgr;
            }
        }
    }

    RvLockRelease(&self->lock, self->logMgr);
}


/* Cache client functions */

RvStatus RvAresCacheCltConstruct(RvAresCacheClt *self, RvSelectEngine *seli, RvAresCacheParams *params, RvLogMgr *logMgr) {
    RvStatus s = RV_OK;

    self->logMgr = logMgr;

    if(logMgr == 0) {
        /* If logMgr == 0, set to 0 logSource also */
        self->logSrc = 0;
    } else {
        /* Construct log source */
        s = RvLogSourceConstruct(logMgr, &self->logSrc, LOGSOURCE_NAME, "DNS Caching log source");
        if(s != RV_OK) {
            return s;
        }
    }

    RvLockGet(&gsGlobalInstanceLock, logMgr);

    if(gsGlobalCache == 0) {
        s = RvAresCacheDNew(&gsGlobalCache, seli, self, params);
    }

    if(s == RV_OK) {
        self->cached = gsGlobalCache;
        RvAresCacheDAddClt(gsGlobalCache, self);
    }

    RvLockRelease(&gsGlobalInstanceLock, logMgr);

    return s;
}


RvStatus RvAresCacheCltDestruct(RvAresCacheClt *self) {
    RvLockGet(&gsGlobalInstanceLock, self->logMgr);
    RvAresCacheDRemoveClt(gsGlobalCache, self);
    RvLockRelease(&gsGlobalInstanceLock, self->logMgr);

    if(self->logMgr) {
        RvLogSourceDestruct(&self->logSrc);
    }

    return RV_OK;
}


/*RVCOREAPI*/
RvStatus RvAresCacheDInit() {
    RvStatus s;

    gsStartTime = RvTimestampGet(0);
    s = RvThreadCreateVar(RvAresCacheEnvDestruct, "RvAresCacheEnv", 0, &gsCacheCtxIdx);
    if(s != RV_OK) {
        return s;
    }

    s = RvLockConstruct(0, &gsGlobalInstanceLock);
    return s;
}

RvStatus
RvAresCacheDEnd() {
    RvStatus s;

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
RvBool RvAresCacheTraverseCells(RvFILE *fp, CellsStats *cstats, RvAresCacheCells *self) {
    RvSize_t i;
    RvAresTime prevTTL, curTTL;
    RvAresTime curTime;
    RvSize_t nCells = self->curSize;
    RvAresTime curE;

    RvAssert(self->curSize <= self->maxSize);

    cstats->nPages = 0;
    cstats->nInHashRecs = 0;
    cstats->nRecs = 0;

    if(self->curSize == 0) {
        return RV_TRUE;
    }

    curTime = RvAresGetCacheTime();

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
    PagesStats pstats;
    CellsStats cstats;

    RvAresCacheUpdateTime();

    fp = RvFopen(filename, "w");
    if(fp == 0) {
        return;
    }

    ctx.fp = fp;
    ctx.indent = "";
    RvLockGet(&self->lock, 0);

    RvAresCacheBookkeeping(&self->cache);

    RvFprintf(fp, "\n--------------------------------------HASH-------------------------------------------\n");

    RvAresTraverseHash(&self->cache.hash, &hstats, DumpRecordset, &ctx);
    

    RvFprintf(fp, "\nHash statistics\n");
    RvFprintf(fp, "\tRecords: %d\n\tBusy cells: %d\n\tMax chain: %d\n\tTotal size: %d\n", hstats.nRecords, hstats.nBusyCells, hstats.maxChainLength, (RvUint)hstats.totalSize);

    RvFprintf(fp, "\n----------------------------------Expiration Cells------------------------------------\n");
    
    RvAresCacheTraverseCells(fp, &cstats, &self->cache.cells);

    RvFprintf(fp, "\n\nExpiration cells totals:\nPages: %d\nTotal records:%d\nIn-hash records: %d\nOut-of-hash records:%d", 
        cstats.nPages, cstats.nRecs, cstats.nInHashRecs, cstats.nRecs - cstats.nInHashRecs);

    RvFprintf(fp, "\n-----------------------------------Expired Pages--------------------------------------\n");

    RvAresCacheTraversePages(fp, self->cache.expiredPages.first, self->cache.expiredPages.last, &pstats);

    RvFprintf(fp, "\nExpired pages: %d\nTotal records: %d\nIn-hash records:%d\nOut-of-hash records: %d",
        (RvUint)pstats.nPages, (RvUint)pstats.nRecs, (RvUint)pstats.inHashRecs, (RvUint)(pstats.nRecs - pstats.inHashRecs));

    RvAresUntraverseHash(&self->cache.hash);


    (void)RvFclose(fp);

    RvLockRelease(&self->lock, 0);
}

#ifndef RV_DNS_CACHE_DUMPFILE
#define RV_DNS_CACHE_DUMPFILE "dnscache.dump"
#endif

RVCOREAPI
void RvAresCacheDumpGlobal(RvChar *filename) {
    if(filename == 0 || filename[0] == 0) {
        filename = RV_DNS_CACHE_DUMPFILE;
    }
    RvAresCacheDump(gsGlobalCache, filename);
}

#endif

#endif

