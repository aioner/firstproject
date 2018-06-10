#include "rvsdpprivate.h"
#include "rvalloc.h"
#include <stdlib.h>
#include <string.h>

#ifdef RV_SDP_USE_DEBUG_ALLOC_MODEL

/*#define RV_SDP_MONITOR_ALLOCATIONS*/


#ifdef RV_SDP_MONITOR_ALLOCATIONS
typedef struct _SdpAllocDebugStruct
{
    struct _SdpAllocDebugStruct *iNext;
    char iFileName[256];
    int iLineNum;
    void *iMemChunk;
    int iChunkSz;
} SdpAllocDebugStruct;
static SdpAllocDebugStruct *gMemListHead = NULL;
#endif /*RV_SDP_MONITOR_ALLOCATIONS*/

static RvInt32 gTotalMem = 0;
static RvInt32 gMaxUsedMem = 0;
static RvInt32 gTotalCnt = 0;
static RvMutex  gMutex;

void sdpRvAllocInitialize()
{
    gTotalMem = 0;
    gMaxUsedMem = 0;
    gTotalCnt = 0;
#ifdef RV_SDP_MONITOR_ALLOCATIONS
    gMemListHead = NULL;
#endif /*RV_SDP_MONITOR_ALLOCATIONS*/
    RvMutexConstruct(NULL,&gMutex);
}

void sdpRvAllocEnd()
{
    RvMutexDestruct(&gMutex,NULL);
}


void rvSdpGetMemoryCounters(int* memSize, int* memCnt)
{
    *memSize = gTotalMem;
    *memCnt = gTotalCnt;
}

RvUint32 rvSdpGetMemoryMaxUsage()
{
    return gMaxUsedMem;
}

void* sdpRvAllocAllocate(RvAlloc* a, RvSize_t s, char* fileN, int lineN) 
{
    void *p;
    RV_UNUSED_ARG(lineN);
    RV_UNUSED_ARG(fileN);
    
    if (s > a->maxSize)
        return NULL;
    p = a->alloc(a->pool, s);
//    p = malloc(s);
    if (!p)
        return NULL;
 
    /* update list of memcorey chunks */
    RvMutexLock(&gMutex,NULL);

#ifdef RV_SDP_MONITOR_ALLOCATIONS
    {
        SdpAllocDebugStruct *mm;
        mm = (SdpAllocDebugStruct*) malloc(sizeof(SdpAllocDebugStruct));
        strcpy(mm->iFileName,fileN);
        mm->iLineNum = lineN;
        mm->iMemChunk = p; 
        mm->iChunkSz = s;
        mm->iNext = NULL;
    
        if (!gMemListHead)
            gMemListHead = mm;
        else
        {
            mm->iNext = gMemListHead;
            gMemListHead = mm;
        }
    }
#endif /*RV_SDP_MONITOR_ALLOCATIONS*/
    
    gTotalCnt ++;
    gTotalMem += s;
    if (gTotalMem > gMaxUsedMem)
		gMaxUsedMem = gTotalMem;

    RvMutexUnlock(&gMutex,NULL);
        
    return p;
}


void sdpRvAllocDeallocate(RvAlloc* a, RvSize_t s, void* ptr) 
{

	RvMutexLock(&gMutex,NULL);
    gTotalCnt --;
    gTotalMem -= s;
    
#ifdef RV_SDP_MONITOR_ALLOCATIONS
    {
        SdpAllocDebugStruct *mm, *mmp;
        mmp = NULL;
        mm = gMemListHead;
        while (mm)
        {
            if (mm->iMemChunk == ptr)
            {
                if (!mmp)
                    gMemListHead = gMemListHead->iNext;
                else
                    mmp->iNext = mm->iNext;
                break;
            }
            mmp = mm;
            mm = mm->iNext;
        }
    }
#endif /*RV_SDP_MONITOR_ALLOCATIONS*/
    RvMutexUnlock(&gMutex,NULL);
//    free(ptr);
    a->dealloc(a->pool, s, ptr);
}

#endif /*RV_SDP_USE_DEBUG_ALLOC_MODEL*/
