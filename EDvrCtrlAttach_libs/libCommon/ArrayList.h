
#ifndef _ARRAY_LIST_H__INCLUDE_
#define _ARRAY_LIST_H__INCLUDE_
#pragma once

#include "SyncLock.h"

// if the process free the memory of data return true,
// else the ArrayList must mamage the memory
#ifndef NODE_DATA_PROC
#define NODE_DATA_PROC
typedef int (*fFreeData)(void* pData);
typedef void *(*fCopyAllocData)(void* pData);
#endif

typedef struct _SListNode
{
    struct _SListNode *pPrevNode;
    struct _SListNode *pNextNode;
    size_t nIndex;
}SListNode;

typedef struct _SArrayListNode
{
    SListNode *pPrevNode;
    SListNode *pNextNode;
    size_t nIndex;
    void* pData;
    int bNoUsed;
}SArrayListNode, *HAtNode;

typedef struct _SArrayListRoot
{
    SListNode *pTail;
    SListNode *pHead;
    size_t nSize;
    size_t nCount;
    size_t nDataSize;
    LOCK_HANDLE lockCtrl;
    fCopyAllocData fpCopyAllocData;
    fFreeData fpFreeData;
    SListNode *pCursor;
}SArrayListRoot, *HArrayList;

#ifdef __cplusplus
extern "C" {
#endif

int ArrayList_Init(HArrayList hArrayList, size_t nDataSize, fFreeData fpFreeData, fCopyAllocData fpCopyAllocData);
HArrayList ArrayListCreateByProc(size_t nDataSize, fFreeData fpFreeData, fCopyAllocData fpAllocData);
#define ArrayListCreate(nDataSize, fpFreeData) ArrayListCreateByProc(nDataSize, fpFreeData, NULL)
void ArrayListDestroy(HArrayList hArrayList, BOOL bFreeRoot);
#define ArrayListRelease(hArrayList) ArrayListDestroy(hArrayList, TRUE)

size_t ArrayListGetAtIndex(HAtNode hAtNode);
void* ArrayListGetAtData(HAtNode hAtNode);
HAtNode ArrayListGetPrev(HAtNode hAtNode);
HAtNode ArrayListGetNext(HAtNode hAtNode);

size_t ArrayListGetCount(HArrayList hArrayList);
size_t ArrayListGetSize(HArrayList hArrayList);
size_t ArrayListGetNoUsedID(HArrayList hArrayList);
HAtNode ArrayListGetAt(HArrayList hArrayList, size_t nIndex);
void* ArrayListGetData(HArrayList hArrayList, size_t nIndex);

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

HAtNode ArrayListGetHead(HArrayList hArrayList);
HAtNode ArrayListGetTail(HArrayList hArrayList);

size_t ArrayListAddAt(HArrayList hArrayList, void* pData);
size_t ArrayListSetAt(HArrayList hArrayList, void* pData, size_t nIndex);

void ArrayListEraseAt(HArrayList hArrayList, size_t nIndex);
void ArrayListEraseAtSort(HArrayList hArrayList, size_t nIndex);
void ArrayListRemoveAll(HArrayList hArrayList);

typedef BOOL (*FPForEachNode)(HArrayList hArrayList, size_t nIndex, void* pData, void* objUser);
intptr_t ArrayListForEach(HArrayList hArrayList, FPForEachNode fpForEacnNode, void**ppOutput, void * objUser);

#ifdef __cplusplus
};
#endif//__cplusplus


//////////////////////////////////////////////////////////////////////////
// 类方式描述
#ifdef __cplusplus

template<typename TYPE>
inline void ZeroMemoryStruct(TYPE *pStruct)
{
    memset(pStruct, 0, sizeof(TYPE));
};

// 链表节点类模板
template<typename TYPE>
class CListNode : public _SArrayListNode
{
private:
    CListNode();
    ~CListNode();
public:
    size_t GetAtIndex();                        //得到节点序号
    TYPE* GetAtData();                          //得到节点数据
    CListNode* GetPrev();                       //得到上一节点
    CListNode* GetNext();                       //得到下一节点
    operator TYPE *();                          //类型运算符重载
};

// 链表类模板
template<typename TYPE>
class CArrayList : public _SArrayListRoot
{
public:
    CArrayList();
    CArrayList(fFreeData fpFreeData, fCopyAllocData fpCopyAllocData);
    ~CArrayList();

    void SetDataProc(fFreeData fpFreeData, fCopyAllocData fpCopyAllocData);
    size_t GetCount();                          //得到有效节点总数
    size_t GetSize();                           //得到链表长度

    size_t GetNoUsedID();                       //得到空闲节点
    TYPE* GetAtData(size_t nIndex);             //得到节点数据
    size_t SetAt(TYPE& rData, size_t nIndex);   //设置节点数据
    size_t AddAt(TYPE& rData);                  //增加节点
    void EraseAt(size_t nIndex);                //删除节点不排序
    void EraseAtSort(size_t nIndex);            //删除节点后重新排序
    void RemoveAll();                           //清空链表
    TYPE* operator[](size_t nIndex);            //索引

    CListNode<TYPE> *GetHead();
    CListNode<TYPE> *GetTail();
    CListNode<TYPE> *GetAt(size_t nIndex);
};

//////////////////////////////////////////////////////////////////////////
// class ListNode

template<typename TYPE>
size_t CListNode<TYPE>::GetAtIndex()
{
    return ArrayListGetAtIndex(this);
}

template<typename TYPE>
TYPE* CListNode<TYPE>::GetAtData()
{
    return (TYPE*)ArrayListGetAtData(this);
}

template<typename TYPE>
CListNode<TYPE>* CListNode<TYPE>::GetPrev()
{
    return (CListNode<TYPE>*)ArrayListGetPrev(this);
}

template<typename TYPE>
CListNode<TYPE>* CListNode<TYPE>::GetNext()
{
    return (CListNode<TYPE>*)ArrayListGetNext(this);
}

template<typename TYPE>
CListNode<TYPE>::operator TYPE *()
{
    return (TYPE*)ArrayListGetAtData(this);
}

//////////////////////////////////////////////////////////////////////////
//class CArrayList<TYPE>

template<typename TYPE>
CArrayList<TYPE>::CArrayList()
{
    ArrayList_Init(this, sizeof(TYPE), NULL, NULL);
}

template<typename TYPE>
CArrayList<TYPE>::CArrayList(fFreeData fpFreeData, fCopyAllocData fpCopyAllocData)
{
    ArrayList_Init(this, sizeof(TYPE), fpFreeData, fpCopyAllocData);
}

template<typename TYPE>
CArrayList<TYPE>::~CArrayList()
{
    ArrayListRemoveAll(this);
}

template<typename TYPE>
void CArrayList<TYPE>::SetDataProc(fFreeData fpFreeData, fCopyAllocData fpCopyAllocData)
{
    this->fpCopyAllocData = fpCopyAllocData;
    this->fpFreeData = fpFreeData;
}

template<typename TYPE>
size_t CArrayList<TYPE>::GetCount()
{
    return ArrayListGetCount(this);
}

template<typename TYPE>
size_t CArrayList<TYPE>::GetSize()
{
    return ArrayListGetSize(this);
}

template<typename TYPE>
size_t CArrayList<TYPE>::GetNoUsedID()
{
    return ArrayListGetNoUsedID(this);
}

template<typename TYPE>
TYPE *CArrayList<TYPE>::GetAtData(size_t nIndex)
{
    return (TYPE*)ArrayListGetData(this, nIndex);
}

template<typename TYPE>
size_t CArrayList<TYPE>::SetAt(TYPE& rData, size_t nIndex)
{
    return ArrayListSetAt(this, &rData, nIndex);
}

template<typename TYPE>
size_t CArrayList<TYPE>::AddAt(TYPE& rData)
{
    return ArrayListAddAt(this, &rData);
}

template<typename TYPE>
void CArrayList<TYPE>::EraseAt(size_t nIndex)
{
    ArrayListEraseAt(this, nIndex);
}

template<typename TYPE>
void CArrayList<TYPE>::EraseAtSort(size_t nIndex)
{
    ArrayListEraseAtSort(this, nIndex);
}

template<typename TYPE>
void CArrayList<TYPE>::RemoveAll()
{
    ArrayListRemoveAll(this);
}

template<typename TYPE>
TYPE* CArrayList<TYPE>::operator[](size_t nIndex)
{
    return (TYPE*)ArrayListGetData(this, nIndex);
}

template<typename TYPE>
CListNode<TYPE> *CArrayList<TYPE>::GetHead()
{
    return (CListNode<TYPE>*)ArrayListGetHead(this);
}

template<typename TYPE>
CListNode<TYPE> *CArrayList<TYPE>::GetTail()
{
    return (CListNode<TYPE>*)ArrayListGetTail(this);
}

template<typename TYPE>
CListNode<TYPE> *CArrayList<TYPE>::GetAt(size_t nIndex)
{
    return (CListNode<TYPE>*)ArrayListGetAt(this, nIndex);
}

#endif//__cplusplus

#define ARRAYLIST_SELECT_BEGIN_C(TYPE, pMgr)\
    size_t i;\
    HAtNode iterator;\
    TYPE * pNode;\
    for(i=0,iterator=ArrayListGetHead((HArrayList)pMgr);\
        i<ArrayListGetSize((HArrayList)pMgr);\
        i++,iterator=ArrayListGetNext(iterator)){\
        if(iterator==NULL) break;\
        pNode = (TYPE*)ArrayListGetAtData(iterator);\
        if(pNode==NULL) continue;

#ifdef __cplusplus
#define ARRAYLIST_SELECT_BEGIN_CPP(TYPE, rMgr)\
    size_t i;\
    CListNode<TYPE> * iterator;\
    TYPE * pNode;\
    for(i=0,iterator=rMgr.GetHead();\
        i<rMgr.GetSize();\
        i++,iterator=iterator->GetNext()){\
        if(iterator==NULL) break;\
        pNode = iterator->GetAtData();\
        if(pNode==NULL) continue;
#endif//__cplusplus

#define ARRAYLIST_SELECT_END()\
    }

#define ARRAYLIST_COMPARE_STRING_CONTINUE(NodeMember, Value)\
    if(strcmp(pNode->NodeMember, Value)) continue;

#define ARRAYLIST_COMPARE_STRING_COUNT_CONTINUE(NodeMember, Value, Count)\
    if(strncmp(pNode->NodeMember, Value, Count)) continue;

#define ARRAYLIST_COMPARE_NUMBER_CONTINUE(NodeMember, Value)\
    if(pNode->NodeMember != Value) continue;

#define ARRAYLIST_COMPARE_POINT_NULL_CONTINUE(NodeMember)\
    if(pNode->NodeMember == NULL) continue;

#endif//_ARRAY_LIST_H__INCLUDE_
