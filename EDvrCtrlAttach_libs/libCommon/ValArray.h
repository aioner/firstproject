#ifndef _VAL_ARRAY_H__INCLUDE_
#define _VAL_ARRAY_H__INCLUDE_
#pragma once

#include "SyncLock.h"

// if the process free the memory of data return true,
// else the ValArray must mamage the memory
#ifndef NODE_DATA_PROC
#define NODE_DATA_PROC
typedef int (*fFreeData)(void* pData);
typedef void *(*fCopyAllocData)(void* pData);
#endif//NODE_DATA_PROC

typedef struct _SValArrayNode
{
    size_t nIndex;
    void* pData;
    int bNoUsed;
}SValArrayNode, *HArrayAt;

typedef struct _SValArrayRoot
{
    size_t nSize;
    size_t nGrowSize;
    size_t nLength;
    size_t nDataSize;
    SValArrayNode *pNode;
    LOCK_HANDLE lockCtrl;
    fCopyAllocData fpCopyAllocData;
    fFreeData fpFreeData;
}SValArrayRoot, *HValArray;


#ifdef __cplusplus
extern "C" {
#endif

int ValArray_Init(HValArray hValArray, size_t nDataSize, fFreeData fpFreeData, fCopyAllocData fpCopyAllocData);
HValArray ValArrayCreateByProc(size_t nDataSize, fFreeData fpFreeData, fCopyAllocData fpAllocData);
#define ValArrayCreate(nDataSize, fpFreeData) ValArrayCreateByProc(nDataSize, fpFreeData, NULL)
void ValArrayRelease(HValArray hValArray);

size_t ValArrayGetAtIndex(HArrayAt hAtNode);
void* ValArrayGetAtData(HArrayAt hAtNode);

size_t ValArrayGetSize(HValArray hValArray);
size_t ValArrayGetNoUsedID(HValArray hValArray);
HArrayAt ValArrayGetAt(HValArray hValArray, size_t nIndex);
void* ValArrayGetData(HValArray hValArray, size_t nIndex);

size_t ValArrayAddAt(HValArray hValArray, void* pData);
size_t ValArraySetAt(HValArray hValArray, void* pData, size_t nIndex);

void ValArrayEraseAt(HValArray hValArray, size_t nIndex);
void ValArrayEraseAtSort(HValArray hValArray, size_t nIndex);

void ValArrayRemoveAll(HValArray hValArray);

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////
// �෽ʽ����
#ifdef __cplusplus

// �����ڵ���ģ��
template<typename TYPE>
class CValArrayNode : public _SValArrayNode
{
private:
    CValArrayNode();
    ~CValArrayNode();
public:
    size_t GetAtIndex();                        //�õ��ڵ����
    TYPE* GetAtData();                          //�õ��ڵ�����
    operator TYPE *();                          //�������������
};

// �������ģ��
template<typename TYPE>
class CValArray : public _SValArrayRoot
{
public:
    CValArray();
    CValArray(fFreeData fpFreeData, fCopyAllocData fpCopyAllocData);
    ~CValArray();

    void SetDataProc(fFreeData fpFreeData, fCopyAllocData fpCopyAllocData);
    size_t GetSize();                           //�õ����鳤��

    size_t GetNoUsedID();                       //�õ����нڵ�
    TYPE* GetAtData(size_t nIndex);             //�õ��ڵ�����
    size_t SetAt(TYPE& rData, size_t nIndex);   //���ýڵ�����
    size_t AddAt(TYPE& rData);                  //���ӽڵ�
    void EraseAt(size_t nIndex);                //ɾ���ڵ㲻����
    void EraseAtSort(size_t nIndex);            //ɾ���ڵ����������
    TYPE* operator[](size_t nIndex);            //����

    CValArrayNode<TYPE> *GetAt(size_t nIndex);
};

//////////////////////////////////////////////////////////////////////////
// class CValArrayNode

template<typename TYPE>
size_t CValArrayNode<TYPE>::GetAtIndex()
{
    return ValArrayGetAtIndex(this);
}

template<typename TYPE>
TYPE* CValArrayNode<TYPE>::GetAtData()
{
    return (TYPE*)ValArrayGetAtData(this);
}

template<typename TYPE>
CValArrayNode<TYPE>::operator TYPE *()
{
    return (TYPE*)ValArrayGetAtData(this);
}

//////////////////////////////////////////////////////////////////////////
// class CValArray

template<typename TYPE>
CValArray<TYPE>::CValArray()
{
    ValArray_Init(this, sizeof(TYPE), NULL, NULL);
}

template<typename TYPE>
CValArray<TYPE>::CValArray(fFreeData fpFreeData, fCopyAllocData fpCopyAllocData)
{
    ValArray_Init(this, sizeof(TYPE), fpFreeData, fpCopyAllocData);
}

template<typename TYPE>
CValArray<TYPE>::~CValArray()
{
    ValArrayRemoveAll(this);
}

template<typename TYPE>
void CValArray<TYPE>::SetDataProc(fFreeData fpFreeData, fCopyAllocData fpCopyAllocData)
{
    this->fpCopyAllocData = fpCopyAllocData;
    this->fpFreeData = fpFreeData;
}

template<typename TYPE>
size_t CValArray<TYPE>::GetSize()
{
    return ValArrayGetSize(this);
}

template<typename TYPE>
size_t CValArray<TYPE>::GetNoUsedID()
{
    return ValArrayGetNoUsedID(this);
}

template<typename TYPE>
TYPE* CValArray<TYPE>::GetAtData(size_t nIndex)
{
    return (TYPE*)ValArrayGetData(this, nIndex);
}

template<typename TYPE>
size_t CValArray<TYPE>::SetAt(TYPE& rData, size_t nIndex)
{
    return ValArraySetAt(this, *rData, nIndex);
}

template<typename TYPE>
size_t CValArray<TYPE>::AddAt(TYPE& rData)
{
    return ValArrayAddAt(this, *rData);
}

template<typename TYPE>
void CValArray<TYPE>::EraseAt(size_t nIndex)
{
    ValArrayEraseAt(this, nIndex);
}

template<typename TYPE>
void CValArray<TYPE>::EraseAtSort(size_t nIndex)
{
    ValArrayEraseAtSort(this, nIndex);
}

template<typename TYPE>
TYPE* CValArray<TYPE>::operator[](size_t nIndex)
{
    return (TYPE*)ValArrayGetData(this, nIndex);
}

template<typename TYPE>
CValArrayNode<TYPE> *CValArray<TYPE>::GetAt(size_t nIndex)
{
    return (CValArrayNode<TYPE> *)ValArrayGetAt(this, nIndex);
}

#endif

#endif//_VAL_ARRAY_H__INCLUDE_
