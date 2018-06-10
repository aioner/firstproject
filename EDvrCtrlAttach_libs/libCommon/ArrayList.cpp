
#include "stdafx.h"
#include <windows.h>
#include <assert.h>
#include "ArrayList.h"

#define _ALGetAt _ALGetAtV1

#ifdef __cplusplus
extern "C" {
#endif

SArrayListNode *_DataNodeCreate(fCopyAllocData fpCopyAllocData, size_t nDataSize, void *pData)
{
    SArrayListNode *pNode = (SArrayListNode*)malloc(sizeof(SArrayListNode));
    assert(pNode!=NULL);
    memset(pNode, 0, sizeof(SArrayListNode));
    if(!fpCopyAllocData)
    {
        pNode->pData = malloc(nDataSize);
        assert(pNode->pData!=NULL);
        memcpy(pNode->pData, pData, nDataSize);
    }
    else
    {
        assert(fpCopyAllocData!=NULL);
        pNode->pData = fpCopyAllocData(pData);
    }
    return pNode;
}

void _DataNodeErase(SArrayListNode *pNode, fFreeData fpFreeData)
{
    int nCurrentFree = TRUE;
    assert(pNode!=NULL);
    if(fpFreeData && pNode->pData)
        nCurrentFree = fpFreeData(pNode->pData);

    if(nCurrentFree)
        pNode->pData = NULL;

    pNode->bNoUsed = TRUE;
}

void _DataNodeFree(SArrayListNode *pNode, fFreeData fpFreeData)
{
    _DataNodeErase(pNode, fpFreeData);
    if(pNode->pData)
        free(pNode->pData);
    free(pNode);
}

SArrayListNode *_ALGetAtV1(SArrayListRoot *pArray, size_t nIndex)
{
    SArrayListNode *pNode = (SArrayListNode*)pArray->pHead;
    assert(pArray!=NULL);

    if(nIndex > pArray->nSize || nIndex < 0)
        return NULL;

    while(nIndex--)
    {
        if(pNode == NULL)
            return NULL;
        pNode = (SArrayListNode*)pNode->pNextNode;
    }

    return pNode;
}

/*
void _AlGrow(SArrayListRoot *pArray, size_t nSize)
{
    size_t i;
    size_t nMemorySize = 0;
    SListNode *pNode = NULL;

    if(nSize <= pArray->nSize)
        return;

    if(!pArray->pNode || nSize > pArray->nLength)
    {
        SyncLock_Lock(&pArray->lockCtrl, -1);
        nMemorySize = (nSize+pArray->nGrowSize-1)/pArray->nGrowSize*pArray->nGrowSize;
        pNode = (SValArrayNode*)malloc(sizeof(SValArrayNode) * nMemorySize);

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
*/

//////////////////////////////////////////////////////////////////////////
SArrayListNode *_ALGetAtV2(SArrayListRoot *pArray, size_t nIndex)
{

    SArrayListNode *pNode = NULL;
    size_t i;
    size_t nDistance;
    BOOL bUseCursor = TRUE;
    BOOL bLeft = FALSE;

    SyncLock_Lock(&pArray->lockCtrl, -1);
    if (pArray->pCursor == pArray->pHead)           //判断游标是否有效
    {
        bUseCursor = FALSE;
    }
    if (pArray->pCursor->nIndex > nIndex)           //判断是否需要使用游标
    {
        nDistance = pArray->pCursor->nIndex - nIndex;
        if (nDistance > nIndex)
        {
            bUseCursor = FALSE;
        }
        else
        {
            bLeft = TRUE;
        }
    }

    if (bUseCursor)         //使用游标
    {
        pNode = (SArrayListNode*)pArray->pCursor;

        if(bLeft)           //从游标处向前遍历
        {
            for(i = pArray->pCursor->nIndex; i != nIndex; i --)
            {
                pNode = (SArrayListNode*)pNode->pPrevNode;
            }
        }
        else                //从游标处向后遍历
        {
            for(i = pArray->pCursor->nIndex; i != nIndex; i ++ )
            {
                pNode = (SArrayListNode*)pNode->pNextNode;
            }
        }
        pArray->pCursor = (SListNode*)pNode;
        SyncLock_Unlock(&pArray->lockCtrl);
        return pNode;

    }
    else                    //不使用游标
    {

        for(i=0, pNode = (SArrayListNode*)pArray->pHead;
            i<pArray->nSize;
            i++, pNode = (SArrayListNode*)pNode->pNextNode)
        {
            if(i == nIndex)
            {
                pArray->pCursor = (SListNode*)pNode;
                SyncLock_Unlock(&pArray->lockCtrl);
                return pNode;
            }
        }
    }

    SyncLock_Unlock(&pArray->lockCtrl);
    return NULL;
}

//////////////////////////////////////////////////////////////////////////

int ArrayList_Init(HArrayList hArrayList, size_t nDataSize, fFreeData fpFreeData, fCopyAllocData fpCopyAllocData)
{
    SArrayListRoot *pArray = (SArrayListRoot *)hArrayList;

    memset(pArray, 0, sizeof(SArrayListRoot));
    pArray->pHead = (SListNode *)pArray;
    pArray->pTail = (SListNode *)pArray;
	pArray->pCursor = pArray->pHead;
	pArray->nDataSize = nDataSize;
    pArray->fpCopyAllocData = fpCopyAllocData;
    pArray->fpFreeData = fpFreeData;
    SyncLock_Create(&pArray->lockCtrl);
    return 0;
}

HArrayList ArrayListCreateByProc(size_t nDataSize, fFreeData fpFreeData, fCopyAllocData fpCopyAllocData)
{
    SArrayListRoot *pArray = NULL;

    pArray = (SArrayListRoot*)malloc(sizeof(SArrayListRoot));
    ArrayList_Init(pArray, nDataSize, fpFreeData, fpCopyAllocData);

    return pArray;
}

void ArrayListDestroy(HArrayList hArrayList, BOOL bFreeRoot)
{
    SArrayListRoot *pArray = (SArrayListRoot *)hArrayList;
    assert(pArray != NULL);
	if (pArray == NULL)
	{
		return;
	}

    ArrayListRemoveAll(pArray);
    SyncLock_Destroy(&pArray->lockCtrl);
    if(bFreeRoot)
        free(pArray);
    else
        memset(pArray, 0, sizeof(SArrayListRoot));
}

size_t ArrayListGetAtIndex(HAtNode hAtNode)
{
    SArrayListNode *pNode = (SArrayListNode*)hAtNode;
    assert(pNode != NULL);
    return pNode->nIndex;
}

void* ArrayListGetAtData(HAtNode hAtNode)
{
    SArrayListNode *pNode = (SArrayListNode*)hAtNode;
    assert(pNode != NULL);
    if(pNode->bNoUsed)
        return NULL;
    return pNode->pData;
}

HAtNode ArrayListGetHead(HArrayList hArrayList)
{
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    assert(pArray != NULL);
    if(pArray->nSize == 0)
        return NULL;
    return (HAtNode)pArray->pHead;
}

HAtNode ArrayListGetTail(HArrayList hArrayList)
{
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    assert(pArray != NULL);
    if(pArray->nSize == 0)
        return NULL;
    return (HAtNode)pArray->pTail;
}

HAtNode ArrayListGetPrev(HAtNode hAtNode)
{
    SArrayListNode *pNode = (SArrayListNode*)hAtNode;
    assert(pNode != NULL);
    if(pNode->pPrevNode == (SListNode *)pNode)
        return NULL;
    return (HAtNode)pNode->pPrevNode;
}

HAtNode ArrayListGetNext(HAtNode hAtNode)
{
    SArrayListNode *pNode = (SArrayListNode*)hAtNode;
    assert(pNode != NULL);
    if((HAtNode)pNode->pNextNode == pNode)
        return NULL;
    return (HAtNode)pNode->pNextNode;
}

size_t ArrayListGetCount(HArrayList hArrayList)
{
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    assert(pArray != NULL);
    
    return pArray->nCount;
}

size_t ArrayListGetSize(HArrayList hArrayList)
{
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    assert(pArray != NULL);

    return pArray->nSize;
}

size_t ArrayListGetNoUsedID(HArrayList hArrayList)
{
    size_t i;
    SArrayListNode *pNode = NULL;
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    assert(pArray != NULL);

    SyncLock_Lock(&pArray->lockCtrl, -1);
    for(i=0, pNode = (SArrayListNode*)pArray->pHead;
        i<pArray->nSize;
        i++, pNode = (SArrayListNode*)pNode->pNextNode)
    {
        if(pNode == NULL)
            break;
        if(pNode->bNoUsed)
        {
            pNode->bNoUsed = FALSE;
            break;
        }
    }
    SyncLock_Unlock(&pArray->lockCtrl);
    return i;
}

HAtNode ArrayListGetAt(HArrayList hArrayList, size_t nIndex)
{
    SArrayListNode *pNode = NULL;
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    assert(pArray != NULL);
    if(pArray->nSize <= nIndex)
        return NULL;
    pNode = _ALGetAt(pArray, nIndex);
    if(pNode->bNoUsed)
        return NULL;
    else
        return (HAtNode)pNode;
}

void* ArrayListGetData(HArrayList hArrayList, size_t nIndex)
{
    HAtNode pNode = ArrayListGetAt(hArrayList, nIndex);
    if(pNode == NULL)
        return NULL;
    else
        return ArrayListGetAtData(pNode);
}

//////////////////////////////////////////////////////////////////////////

size_t ArrayListAddAt(HArrayList hArrayList, void* pData)
{
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    size_t nIndex;
    assert(pArray != NULL);
    pArray->nCount++;
    nIndex = ArrayListGetNoUsedID(hArrayList);
    return ArrayListSetAt(hArrayList, pData, nIndex);
}

size_t ArrayListSetAt(HArrayList hArrayList, void* pData, size_t nIndex)
{
    SArrayListNode *pNode = NULL;
    SListNode *pOldNode = NULL;
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    assert(pArray != NULL);
    if(pArray->nSize < nIndex)
        return -1;

    if(pArray->nSize == nIndex)
    {
        pNode = _DataNodeCreate(pArray->fpCopyAllocData, pArray->nDataSize, pData);
        pNode->nIndex = nIndex;

        // add tail
        pNode->pNextNode = pArray->pTail->pNextNode;
        pNode->pPrevNode = pArray->pTail;
        pArray->pTail->pNextNode = (SListNode *)pNode;
        pArray->pTail = (SListNode *)pNode;

        pArray->nSize++;
    }
    else
    {
		pNode = _ALGetAt(pArray, nIndex);
		SyncLock_Lock(&pArray->lockCtrl, -1);
        _DataNodeErase(pNode, pArray->fpFreeData);
        pNode->bNoUsed = 0;
        if(!pNode->pData)
        {
            if(!pArray->fpCopyAllocData)
            {
                pNode->pData = malloc(pArray->nDataSize);
                memcpy(pNode->pData, pData, pArray->nDataSize);
            }
            else
                pNode->pData = pArray->fpCopyAllocData(pData);
        }
        else
            memcpy(pNode->pData, pData, pArray->nDataSize);
        SyncLock_Unlock(&pArray->lockCtrl);
    }
    return nIndex;
}

void ArrayListEraseAt(HArrayList hArrayList, size_t nIndex)
{
    SArrayListNode *pNode = NULL;
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    assert(pArray != NULL);
    if(pArray->nSize <= nIndex)
        return;

    pArray->nCount--;
    pNode = _ALGetAt(pArray, nIndex);
    SyncLock_Lock(&pArray->lockCtrl, -1);
    _DataNodeErase(pNode, pArray->fpFreeData);
    SyncLock_Unlock(&pArray->lockCtrl);
}

void ArrayListEraseAtSort(HArrayList hArrayList, size_t nIndex)
{
    size_t i;
    SArrayListNode *pNode = NULL, *pNodeSort = NULL;
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    assert(pArray != NULL);
    if(pArray->nSize <= nIndex)
        return;

    //erase data
    pArray->nCount--;
    pNode = _ALGetAt(pArray, nIndex);
    SyncLock_Lock(&pArray->lockCtrl, -1);
    _DataNodeErase(pNode, pArray->fpFreeData);

    if(nIndex < pArray->nSize-1)
    {//need sort
        pNodeSort = (SArrayListNode*)pNode->pNextNode;
        // move the node to tail
        pNode->pPrevNode->pNextNode = pNode->pNextNode;
        pNode->pNextNode->pPrevNode = pNode->pPrevNode;

        pNode->pNextNode = pArray->pTail->pNextNode;
        pNode->pPrevNode = pArray->pTail;
        pArray->pTail->pNextNode = (SListNode *)pNode;
        pArray->pTail = (SListNode *)pNode;

        //sort index

        for(i=nIndex; i<pArray->nSize;
            i++, pNodeSort = (SArrayListNode*)pNodeSort->pNextNode)
        {
            if(pNodeSort == NULL)
                break;
            pNodeSort->nIndex = i;
        }
    }
    SyncLock_Unlock(&pArray->lockCtrl);
}

void ArrayListRemoveAll(HArrayList hArrayList)
{
    SArrayListNode *pNode = NULL;
    SArrayListRoot *pArray = (SArrayListRoot*)hArrayList;
    assert(pArray != NULL);

    SyncLock_Lock(&pArray->lockCtrl, -1);
    for(pNode = (SArrayListNode *)ArrayListGetTail(hArrayList);
        pArray->nSize > 0;
        pArray->nSize--, pNode = (SArrayListNode *)ArrayListGetTail(hArrayList))
    {
        pArray->pTail = pNode->pPrevNode;
        _DataNodeFree(pNode, pArray->fpFreeData);
    }
    pArray->pTail = (SListNode *)pArray;
    pArray->pHead = (SListNode *)pArray;
	pArray->pCursor = (SListNode *)pArray;
    pArray->nSize = pArray->nCount = 0;
    SyncLock_Unlock(&pArray->lockCtrl);
}

intptr_t ArrayListForEach(HArrayList hArrayList, FPForEachNode fpForEacnNode, void**ppOutput, void * objUser)
{
    size_t i;
    HAtNode iterator;
    void * pNode;
    if(fpForEacnNode == NULL)
        return ArrayListGetSize(hArrayList);

    for(i=0,iterator=ArrayListGetHead(hArrayList);
        i<ArrayListGetSize(hArrayList);
        i++,iterator=ArrayListGetNext(iterator))
    {
        if(iterator==NULL)
            break;
        pNode = ArrayListGetAtData(iterator);
        if(pNode==NULL)
            continue;

        if(!fpForEacnNode(hArrayList, i, pNode, objUser))
        {
            if(ppOutput != NULL)
                *ppOutput = pNode;
            return i;
        }
    }
    return -1;
}

#ifdef __cplusplus
}
#endif
