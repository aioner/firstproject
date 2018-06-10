#include "rvscache.h"
#include "rvmemory.h"

#if 0

typedef struct _RvSCache {
    RvUint8 *buf;
    RvUint8 *pCur;
    RvUint8 *pEnd;
} RvSCache;

#endif



RVAPI RvStatus RVCALLCONV RvSCacheConstruct(RvSCache *self, RvUint8 *buf, RvSize_t size) {
    self->buf = buf;
    self->pCur = buf;
    self->pEnd = buf + size;
    return RV_OK;
}

RVAPI RvStatus RVCALLCONV RvSCacheAlloc( RvSCache *self, RvSize_t size, RvSize_t alignment, void **pRes)
{
    RvUint8 *p = RvAlignTo(self->pCur, alignment);
    RvUint8 *newCur = p + size;
    RvStatus s = RV_OK;

    if(newCur < self->pEnd) {
        self->pCur = newCur;
        *pRes = p;
        return s;
    }

    s = RvMemoryAlloc(0, size, 0, pRes);
    return s;
}

RVAPI RvStatus RVCALLCONV RvSCacheFree(RvSCache *self, void *vp) {
    RvStatus s;
    RvUint8 *p = (RvUint8 *)vp;

    if(vp == 0) {
        return RV_OK;
    }

    if(p >= self->buf && p < self->pEnd) {
        return RV_OK;
    }

    s = RvMemoryFree(p, 0);
    return s;
}

RVAPI RvStatus RVCALLCONV RvSCacheStrDup(RvSCache *self, const RvChar *src, RvChar **pCopy) {
    RvSize_t l = strlen(src);
    RvChar *dest;
    RvStatus s = RV_OK;

    s = RvSCacheAlloc(self, l + 1, 1, (void **)&dest);
    if(s != RV_OK) {
        return s;
    }

    memcpy(dest, src, l + 1);
    *pCopy = dest;
    return s;
}

struct CAAlignment {
    char c;
    char *a[1];
};

RVAPI void RVCALLCONV RvSCacheFreeStringArray(RvSCache *self, RvChar **arr) {
    RvChar **cur;

    if(arr == 0) {
        return;
    }

    for(cur = arr; *cur != 0; cur++) {
        RvSCacheFree(self, *cur);
    }

    RvSCacheFree(self, arr);
}

RVAPI RvStatus RVCALLCONV RvSCacheCopyStringArray(RvSCache *self, RvChar **src, RvChar ***pCopy) {
    int i;
    int n;
    int alignment;
    RvChar **dest;
    RvStatus s = RV_OK;
    RvSize_t arrSize;
    
    alignment = RV_OFFSETOF(struct CAAlignment, a);
    for(n = 0; src[n] != 0; n++) {}

    arrSize = (n + 1) * sizeof(*dest);
    s = RvSCacheAlloc(self, arrSize, alignment, (void **)&dest);
    if(s != RV_OK) {
        return s;
    }
    
    memset(dest, 0, arrSize);

    for(i = 0; i < n; i++) {
        s = RvSCacheStrDup(self, src[i], &dest[i]);
        if(s != RV_OK) {
            RvSCacheFreeStringArray(self, dest);
        }
    }

    *pCopy = dest;
    return s;

}

