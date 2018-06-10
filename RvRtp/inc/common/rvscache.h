#ifndef _rvscache_h
#define _rvscache_h

#include "rvcbase.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _RvSCache {
    RvUint8 *buf;
    RvUint8 *pCur;
    RvUint8 *pEnd;
} RvSCache;

RVAPI RvStatus RVCALLCONV RvSCacheConstruct(RvSCache *self, RvUint8 *buf, RvSize_t size);

RVAPI RvStatus RVCALLCONV RvSCacheAlloc(RvSCache *self, RvSize_t size, RvSize_t alignment, void **pRes);

RVAPI RvStatus RVCALLCONV RvSCacheFree(RvSCache *self, void *p);

RVAPI RvStatus RVCALLCONV RvSCacheStrDup(RvSCache *self, const RvChar *s, RvChar **pCopy);

RVAPI RvStatus RVCALLCONV RvSCacheCopyStringArray(RvSCache *self, RvChar **src, RvChar ***pCopy);

RVAPI void RVCALLCONV RvSCacheFreeStringArray(RvSCache *self, RvChar **arr);




#ifdef _cplusplus
}
#endif

#endif
