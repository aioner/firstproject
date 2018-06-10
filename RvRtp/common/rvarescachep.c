#include "rvarescachep.h"
#include "rvmemory.h"
#include "rvstrutils.h"

/*---------------------------------- Page Allocator methods -----------------------------------*/

void RvAresPageAllocatorReset(RvAresPageAllocator *self) {
    RvObjPoolReset(&self->pagePool);
}

#if 0
	void *(*pagealloc)(RvSize_t size, void *data);   /* Allocate page of memory. Return pointer to page memory, NULL = failed. */
	void  (*pagefree)(void *ptr, void *data);        /* Free page of memory. */
#endif

void *pageAlloc(RvSize_t size, void *data) {
    void *p;
    RvStatus s;

    (void)data;
    s = RvMemoryAlloc(0, size, 0, (void **)&p);
    if(s != RV_OK) {
        return 0;
    }

    return p;
}

void pageFree(void *p, void *data) {
    (void)data;
    (void)RvMemoryFree(p, 0);
}


RvStatus RvAresPageAllocatorConstruct(RvAresPageAllocator *self,
                                      RvSize_t pageSize,
                                      RvSize_t minPages,
                                      RvSize_t maxPages,
                                      RvSize_t deltaPages,
                                      RvSize_t bigPagesMax,
                                      RvLogSource *logSrc) {

    RvStatus s = RV_OK;
    RvAresPage tmpl;
    RvObjPoolFuncs vt;
    RvObjPool *pool;
    RvSize_t  realPageSize;

    realPageSize = pageSize + sizeof(tmpl);

	memset(&vt, 0, sizeof(RvObjPoolFuncs));
    vt.pagealloc = pageAlloc;
    vt.pagefree = pageFree;

    pool = RvObjPoolConstruct(&tmpl, &tmpl.poolElem, &vt, realPageSize, deltaPages, 0, RV_OBJPOOL_TYPE_DYNAMIC,
            RV_OBJPOOL_SALVAGE_ALLOWED, maxPages, minPages, 50, &self->pagePool);

    if(pool == 0) {
        return RV_ERROR_UNKNOWN;
    }

    self->pageSize = pageSize;
    self->realPageSize = realPageSize;
    self->bigPagesMax = bigPagesMax;
    self->bigPagesCurrent = 0;
    self->logSrc = logSrc;
    return s;
}

void RvAresPageAllocatorDestruct(RvAresPageAllocator *self) {
    RvObjPool *pagePool = &self->pagePool;
    RvObjPoolReset(pagePool);
    (void)RvObjPoolDestruct(pagePool);
}

/* Allocates page for caching. If pageSize <= size of pool page, than page is allocated from the pool,
 * otherwise - page is allocated using RvMemoryAlloc.
 */
RvAresPage* RvAresPageAllocatorAlloc(RvAresPageAllocator *self, RvSize_t pageSize) {
    RvAresPage *page;
    RvLogMgr *logMgr = LogMgrFromLogSrc(self->logSrc);
    
    pageSize += sizeof(*page) + RV_ALIGN_SIZE;

    if(pageSize <= self->realPageSize) {
        pageSize = self->pageSize;
        page = (RvAresPage *)RvObjPoolGetItem(&self->pagePool);
        if(page == 0) {
            return 0;
        }
        page->pageSize = 0;
    } else {
        RvStatus s;

        if(self->bigPagesCurrent + pageSize > self->bigPagesMax) {
            return 0;
        }

        s = RvMemoryAlloc(0, pageSize, logMgr, (void **)&page);
        if(s != RV_OK) {
            return 0;
        }
        self->bigPagesCurrent += pageSize;
        page->pageSize = pageSize;
    }

    page->freeSpace = (RvUint8 *)page + sizeof(*page);
    page->spaceLeft = pageSize - sizeof(*page);
    page->firstRecord = 0;
    page->next = 0;
    page->maxExpirationTime = 0;
    page->lockCnt = 0;
    return page;
}


void RvAresPageAllocatorFree(RvAresPageAllocator *self, RvAresPage *page) {
    if(page->pageSize == 0) {
        (void)RvObjPoolReleaseItem(&self->pagePool, page);
    } else {
        self->bigPagesCurrent -= page->pageSize;
        RvMemoryFree(page, LogMgrFromLogSrc(self->logSrc));
    }
}

/*-------------------------------------- Hash Function --------------------------*/

typedef struct {
    RvUint32 curHash;
} RvHashVal;

#define RvHashValStart(self) ((self)->curHash = 5381)

static
void RvHashValAccumulate(RvHashVal *self, RvUint8 *buf, RvSize_t size) {
    RvUint32 hash = self->curHash;
    RvUint8  *pbuf;
    RvSize_t delta;

    if(size > 8) {
        delta = (size - 8) >> 1;
        size = 8;
        pbuf = buf + delta;
    } else {
        pbuf = buf;
    }

    switch(size) {
        case 8:
            hash = ((hash << 5) + hash) + pbuf[7];
        case 7:
            hash = ((hash << 5) + hash) + pbuf[6];
        case 6:
            hash = ((hash << 5) + hash) + pbuf[5];
        case 5:
            hash = ((hash << 5) + hash) + pbuf[4];
        case 4:
            hash = ((hash << 5) + hash) + pbuf[3];
        case 3:
            hash = ((hash << 5) + hash) + pbuf[2];
        case 2:
            hash = ((hash << 5) + hash) + pbuf[1];
        case 1:
            hash = ((hash << 5) + hash) + pbuf[0];
    }

    self->curHash = hash;
}


#define RvHashValGet(self) ((self)->curHash % 0x7fffffff)

void RvAresHashClear(RvAresHash *self) {
    RvAresHashLink *cur = self->buckets;      /* points to the first cell in the hash */
    RvAresHashLink *last = cur + self->size;  /* points to the cell after last cell in the hash */

    /* Initialize 'next' and 'prev' pointer of each cell to point to the cell itself */
    for(; cur < last; cur++) {
        cur->next = cur->prev = cur;
    }
} 



/*************************************************************************************
 * RvStatus RvAresHashConstruct(RvAresHash *self, RvLogSource *logSrc, RvSize_t hashSize)
 *************************************************************************************
 *
 * Constructs hash used for cache entries lookups
 *
 *
 *
 *  self     - points to the object being constructed
 *  logSrc   - log source
 *  hashSize - size of hash table
 */
RvStatus RvAresHashConstruct(RvAresHash *self, RvLogSource *logSrc, RvSize_t hashSize) {
    RvLogMgr *logMgr = LogMgrFromLogSrc(logSrc);
    RvStatus s;
    void *rawmem;

    RvSize_t totalSize = sizeof(self->buckets[0]) * hashSize;

    s = RvMemoryAlloc(0, totalSize, logMgr, &rawmem);
    if(s != RV_OK) {
        return s;
    }
    self->buckets = (RvAresHashLink *)rawmem;
    self->size    = hashSize;
    self->logSrc  = logSrc;

    RvAresHashClear(self);
    return RV_OK;
}

void RvAresHashDestruct(RvAresHash *self) {
    RvLogMgr *logMgr = LOGMGR;

    (void)RvMemoryFree(self->buckets, logMgr);
} 



/* Finds appropriate entry in the hash table. Expired records aren't removed */
static
RvRdata* RvAresHashFindAux(const RvAresHash *self, RvSize_t t, RvChar *name, RvSize_t nameSize, RvAresHashInsertPoint *insertPoint) {
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
        if(nameSize == curRec->nameSize && type == curRec->type && !RvStrncasecmp(name, curRec->name, nameSize)) {
            break;
        }
    }

    if(cur == first) {
        return 0;
    }

    return curRec;
}


void RvAresHashRemove(RvAresHash *self, RvRdata *data) {
    RvAresHashLink *entry = &data->hashLink;

    RV_UNUSED_ARG(self);
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->prev = entry->next = 0;
}


RvRdata* RvAresHashFind(RvAresHash *self, 
                        RvAresTime curTime,
                        RvSize_t t, 
                        RvChar *name, 
                        RvSize_t nameSize, 
                        RvAresHashInsertPoint *insertPoint) {

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

/*---------------------------- DLink methods -------------------------------*/

static
void RvDLinkLink(RvDLink *head, RvDLink *dl) {
    dl->prev = head;
    dl->next = head->next;
    if(head->next) {
        head->next->prev = dl;
    }
    head->next = dl;
}


void RvAresHashAdd(RvAresHash *self, RvAresHashInsertPoint insertionPoint, RvRdata *data) {
    RvAresHashLink *entry = &data->hashLink;
    RvAresHashLink *first = &self->buckets[insertionPoint];

    RvDLinkLink(first, entry);
}

/* Removes all entries currently on the page from hash */
void RvAresHashRemovePage(RvAresHash *self, const RvAresPage *page) {
    RvRdata *cur = page->firstRecord;

    while(cur) {
        if(RvRdataIsValid(cur)) {
            RvAresHashRemove(self, cur);
        }

        cur =  cur->nextPageEntry;
    }
}
