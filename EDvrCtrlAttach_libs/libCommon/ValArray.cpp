#include "stdafx.h"
#include "ValArray.h"

#ifdef __cplusplus
extern "C" {
#endif

int ValArrayNodeInit(SValArrayNode *pInput)
{
    memset(pInput, 0, sizeof(SValArrayNode));
    pInput->bNoUsed = TRUE;
    return 0;
}

void _ArrayNodeDataSet(SValArrayNode *pNode, fCopyAllocData fpCopyAllocData, size_t nDataSize, void *pData)
{
    if(!fpCopyAllocData)
    {
        pNode->pData = malloc(nDataSize);
        assert(pNode->pData!=NULL);
        memcpy(pNode->pData, pData, nDataSize);
    }
    else
        pNode->pData = fpCopyAllocData(pData);
}

void _ArrayNodeErase(SValArrayNode *pNode, fFreeData fpFreeData, BOOL bFree)
{
    int nCurrentFree = 1;
    if(fpFreeData != NULL && pNode->pData)
        nCurrentFree = fpFreeData(pNode->pData);

    if(bFree && !nCurrentFree && pNode->pData)
    {
        free(pNode->pData);
        pNode->pData = NULL;
    }

    pNode->bNoUsed = TRUE;
}

void _ValArrayGrow(SValArrayRoot *pArray, size_t nSize)
{
    size_t i;
    size_t nMemorySize = 0;
    SValArrayNode *pNode = NULL;

    if(nSize <= pArray->nSize)
        return;
    if(nSize > pArray->nSize && nSize <= pArray->nLength)
    {
        pArray->nSize = nSize;
        return;
    }
    if(!pArray->pNode || nSize > pArray->nLength)
    {
        SyncLock_Lock(&pArray->lockCtrl, -1);
        nMemorySize = (nSize+pArray->nGrowSize-1)/pArray->nGrowSize*pArray->nGrowSize;
        pNode = (SValArrayNode*)malloc(sizeof(SValArrayNode) * nMemorySize);
        assert(pNode!=NULL);

        if(pArray->pNode)
        {
            memcpy(pNode, pArray->pNode, sizeof(SValArrayNode) * pArray->nLength);
            free(pArray->pNode);
        }
        for(i=pArray->nLength; i<nMemorySize; i++)
            ValArrayNodeInit(pNode + i);

        pArray->pNode = pNode;
        pArray->nLength = nMemorySize;
        pArray->nSize = nSize;
        SyncLock_Unlock(&pArray->lockCtrl);
    }
}

//////////////////////////////////////////////////////////////////////////
int ValArray_Init(HValArray hValArray, size_t nDataSize, fFreeData fpFreeData, fCopyAllocData fpCopyAllocData)
{
    SValArrayRoot *pArray = (SValArrayRoot *)hValArray;

    memset(pArray, 0, sizeof(SValArrayRoot));
    pArray->nDataSize = nDataSize;
    pArray->fpCopyAllocData = fpCopyAllocData;
    pArray->fpFreeData = fpFreeData;
    pArray->nGrowSize = 64;
    SyncLock_Create(&pArray->lockCtrl);
    return 0;
}

HValArray ValArrayCreateByProc(size_t nDataSize, fFreeData fpFreeData, fCopyAllocData fpCopyAllocData)
{
    SValArrayRoot *pArray = NULL;

    pArray = (SValArrayRoot*)malloc(sizeof(SValArrayRoot));
    ValArray_Init(pArray, nDataSize, fpFreeData, fpCopyAllocData);

    return pArray;
}

void ValArrayRelease(HValArray hValArray)
{
    ValArrayRemoveAll(hValArray);
    SyncLock_Destroy(&hValArray->lockCtrl);
    free(hValArray);
}

size_t ValArrayGetAtIndex(HArrayAt hAtNode)
{
    SValArrayNode *pNode = (SValArrayNode*)hAtNode;
    assert(pNode != NULL);
    return pNode->nIndex;
}

void* ValArrayGetAtData(HArrayAt hAtNode)
{
    SValArrayNode *pNode = (SValArrayNode*)hAtNode;
    assert(pNode != NULL);
    if(pNode->bNoUsed)
        return NULL;
    return pNode->pData;
}

size_t ValArrayGetSize(HValArray hValArray)
{
    SValArrayRoot *pArray = (SValArrayRoot*)hValArray;
    assert(pArray != NULL);

    return pArray->nSize;
}

size_t ValArrayGetNoUsedID(HValArray hValArray)
{
    size_t i;
    SValArrayNode *pNode = NULL;
    SValArrayRoot *pArray = (SValArrayRoot*)hValArray;
    assert(pArray != NULL);

    SyncLock_Lock(&pArray->lockCtrl, -1);
    for(i=0; i<pArray->nSize; i++)
    {
        pNode = pArray->pNode + i;
        if(pNode->bNoUsed)
        {
            pNode->bNoUsed = FALSE;
            break;
        }
    }
    SyncLock_Unlock(&pArray->lockCtrl);
    return i;
}

HArrayAt ValArrayGetAt(HValArray hValArray, size_t nIndex)
{
    SValArrayNode *pNode = NULL;
    SValArrayRoot *pArray = (SValArrayRoot*)hValArray;
    assert(pArray != NULL);
    if(pArray->nSize <= nIndex)
        return NULL;

    SyncLock_Lock(&pArray->lockCtrl, -1);
    pNode = pArray->pNode + nIndex;
    SyncLock_Unlock(&pArray->lockCtrl);
    if(pNode->bNoUsed)
        return NULL;
    else
        return pNode;
}

void* ValArrayGetData(HValArray hValArray, size_t nIndex)
{
    HArrayAt pNode = ValArrayGetAt(hValArray, nIndex);
    if(pNode == NULL)
        return NULL;
    else
        return ValArrayGetAtData(pNode);
}

size_t ValArrayAddAt(HValArray hValArray, void* pData)
{
    SValArrayRoot *pArray = (SValArrayRoot*)hValArray;
    size_t nIndex;
    assert(pArray != NULL);
    nIndex = ValArrayGetNoUsedID(hValArray);
    return ValArraySetAt(hValArray, pData, nIndex);
}

size_t ValArraySetAt(HValArray hValArray, void* pData, size_t nIndex)
{
    SValArrayNode *pNode = NULL;
    SValArrayRoot *pArray = (SValArrayRoot*)hValArray;
    _ValArrayGrow(pArray, nIndex+1);

    SyncLock_Lock(&pArray->lockCtrl, -1);

    pNode = pArray->pNode + nIndex;
    if(!pNode->pData)
        pNode->pData = malloc(pArray->nDataSize);

    pNode->bNoUsed = FALSE;
    pNode->nIndex = nIndex;
    _ArrayNodeDataSet(pNode, pArray->fpCopyAllocData, pArray->nDataSize, pData);

    SyncLock_Unlock(&pArray->lockCtrl);

    return nIndex;
}

void ValArrayEraseAt(HValArray hValArray, size_t nIndex)
{
    SValArrayNode *pNode = NULL;
    SValArrayRoot *pArray = (SValArrayRoot*)hValArray;
    assert(pArray != NULL);
    if(pArray->nSize <= nIndex)
        return;

    pNode = (SValArrayNode *)ValArrayGetAt(pArray, nIndex);
    if(NULL == pNode)
        return;

    SyncLock_Lock(&pArray->lockCtrl, -1);
    _ArrayNodeErase(pNode, pArray->fpFreeData, FALSE);
    SyncLock_Unlock(&pArray->lockCtrl);
}

void ValArrayEraseAtSort(HValArray hValArray, size_t nIndex)
{
    size_t i, j;
    void *pData = NULL;
    SValArrayNode *pNode = NULL, *pNodeSort = NULL;
    SValArrayRoot *pArray = (SValArrayRoot*)hValArray;
    assert(pArray != NULL);
    if(pArray->nSize <= nIndex)
        return;

    //erase data
    pNode = (SValArrayNode *)ValArrayGetAt(pArray, nIndex);
    if(NULL == pNode)
        return;

    SyncLock_Lock(&pArray->lockCtrl, -1);
    _ArrayNodeErase(pNode, pArray->fpFreeData, FALSE);

    if(nIndex < pArray->nSize-1)
    {//need sort data
        for(i=nIndex+1, j=nIndex; i<pArray->nSize; i++)
        {
            pNodeSort = (SValArrayNode *)ValArrayGetAt(hValArray, i);
            if(pNodeSort == NULL)
                continue;
            pData = pNodeSort->pData;
            pNodeSort->pData = pNode->pData;
            pNodeSort->bNoUsed = TRUE;
            pNode->pData = pData;
            pNode->bNoUsed = FALSE;
            j++;
            pNode = pArray->pNode + j;
        }
        pArray->nSize = j;
    }
    if(nIndex == pArray->nSize-1)
        pArray->nSize = nIndex;

    SyncLock_Unlock(&pArray->lockCtrl);
}

void ValArrayRemoveAll(HValArray hValArray)
{
    size_t i;
    SValArrayNode *pNode = NULL;
    SValArrayRoot *pArray = (SValArrayRoot *)hValArray;

    SyncLock_Lock(&pArray->lockCtrl, -1);
    if(!pArray->pNode && pArray->nLength > 0)
    {
        for(i=0; i<pArray->nLength; i++)
        {
            pNode = (SValArrayNode *)ValArrayGetAt(pArray, i);
            if(NULL != pNode)
                _ArrayNodeErase(pNode, pArray->fpFreeData, TRUE);
        }
        free(pArray->pNode);
    }
    pArray->pNode = NULL;
    pArray->nLength = 0;
    pArray->nSize = 0;

    SyncLock_Unlock(&pArray->lockCtrl);
}
#ifdef __cplusplus
}
#endif
