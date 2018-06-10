#include "stdafx.h"
#include "InfoManage.h"

//////////////////////////////////////////////////////////////////////////

HPlugMrg PlugMrg_Create()
{
    return ArrayListCreate(sizeof(SOV_CtrlPlug), (fFreeData)InitCtrlPlug);
}

SOV_CtrlPlug *PlugMrg_GetInfo(HPlugMrg hManage, size_t nIndex)
{
    return (SOV_CtrlPlug*)(ArrayListGetData((HArrayList)hManage, nIndex));
}

size_t PlugMrg_SelectType(HPlugMrg hManage, int nDeviceType, SOV_CtrlPlug **ppInfo)
{
    size_t i;
    SOV_CtrlPlug *pNode = NULL;
    for(i=0; i<ArrayListGetSize((HArrayList)hManage); i++)
    {
        pNode = PlugMrg_GetInfo(hManage, i);
        if(pNode == NULL || (signed)(pNode->m_sType.m_nDeviceType) != nDeviceType)
            continue;
        if(ppInfo != NULL)
            *ppInfo = pNode;
        return i;
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////

HDeviceMrg DeviceMrg_Create()
{
    return ArrayListCreate(sizeof(SDeviceCtrlInfo), (fFreeData)InitDeviceInfo);
}

SDeviceCtrlInfo *DeviceMrg_GetInfo(HDeviceMrg hManage, size_t nIndex)
{
    return (SDeviceCtrlInfo*)ArrayListGetData((HArrayList)hManage, nIndex);
}

size_t DeviceMrg_SelectAddress(HDeviceMrg hManage, LPCSTR szAddress, SDeviceCtrlInfo **ppInfo)
{
    size_t i;
    SDeviceCtrlInfo *pNode = NULL;
    for(i=0; i<ArrayListGetSize((HArrayList)hManage); i++)
    {
        pNode = DeviceMrg_GetInfo(hManage, i);
        if(pNode == NULL || strcmp(pNode->m_szAddress, szAddress))
            continue;
        if(ppInfo != NULL)
            *ppInfo = pNode;
        return i;
    }
    return -1;
}

size_t DeviceMrg_SelectIDS(HDeviceMrg hManage, LPCSTR szIDS, SDeviceCtrlInfo **ppInfo)
{
    size_t i;
    SDeviceCtrlInfo *pNode = NULL;
    for(i=0; i<ArrayListGetSize((HArrayList)hManage); i++)
    {
        pNode = DeviceMrg_GetInfo(hManage, i);
        if(pNode == NULL || strcmp(pNode->m_szIDS, szIDS))
            continue;
        if(ppInfo != NULL)
            *ppInfo = pNode;
        return i;
    }
    return -1;
}

size_t DeviceMrg_SelectLoginID(HDeviceMrg hManage, int nTypeID, int nLoginHandle, SDeviceCtrlInfo **ppInfo)
{
    size_t i;
    SDeviceCtrlInfo *pNode = NULL;
    for(i=0; i<ArrayListGetSize((HArrayList)hManage); i++)
    {
        pNode = DeviceMrg_GetInfo(hManage, i);
        if(pNode == NULL || pNode->m_nTypeID != nTypeID || pNode->m_nLoginHandle != nLoginHandle)
            continue;
        if(ppInfo != NULL)
            *ppInfo = pNode;
        return i;
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////

HAlarmMrg AlarmMrg_Create()
{
    return ArrayListCreate(sizeof(SAlarmTimer), (fFreeData)InitAlarmTimer);
}

SAlarmTimer *AlarmMrg_GetInfo(HAlarmMrg hManage, size_t nIndex)
{
    return (SAlarmTimer*)ArrayListGetData((HArrayList)hManage, nIndex);
}

size_t AlarmMrg_SelectAlarm(HAlarmMrg hManage, int nDeviceID, int nAlarmType, int nChannel, SAlarmTimer **ppInfo)
{
    size_t i;
    SAlarmTimer *pNode = NULL;
    for(i=0; i<ArrayListGetSize((HArrayList)hManage); i++)
    {
        pNode = AlarmMrg_GetInfo(hManage, i);
        if(pNode == NULL
            || pNode->m_nDeviceID != nDeviceID
            || pNode->m_nAlarmType != nAlarmType
            || pNode->m_nChannel != nChannel)
            continue;
        if(ppInfo != NULL)
            *ppInfo = pNode;
        return i;
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////

HReloginMrg ReloginMrg_Create()
{
    return ArrayListCreate(sizeof(SReloginDevice), (fFreeData)InitRelogin);
}

SReloginDevice *ReloginMrg_GetInfo(HReloginMrg hManage, size_t nIndex)
{
    return (SReloginDevice*)ArrayListGetData((HArrayList)hManage, nIndex);
}

size_t ReloginMrg_SelectDevice(HReloginMrg hManage, int nDeviceID, SReloginDevice **ppInfo)
{
    size_t i;
    SReloginDevice *pNode = NULL;
    for(i=0; i<ArrayListGetSize((HArrayList)hManage); i++)
    {
        pNode = ReloginMrg_GetInfo(hManage, i);
        if(pNode == NULL
            || pNode->m_nDeviceID != nDeviceID)
            continue;
        if(ppInfo != NULL)
            *ppInfo = pNode;
        return i;
    }
    return -1;
}

size_t ReloginMrg_AddInfoCheck(HReloginMrg hManage, SReloginDevice *pInfo)
{
    int nReloginID = ReloginMrg_SelectDevice(hManage, pInfo->m_nDeviceID, NULL);
    if(nReloginID >= 0)
        return nReloginID;//重连队列已经存在

    return ArrayListAddAt((HArrayList)hManage, pInfo);
}

void ReloginMrg_DeleteInfoCheck(HReloginMrg hManage, int nDeviceID)
{
    int nReloginID = ReloginMrg_SelectDevice(hManage, nDeviceID, NULL);
    if(nReloginID < 0)
        return;//重连队列不存在

    ArrayListEraseAt((HArrayList)hManage, nDeviceID);
}

//////////////////////////////////////////////////////////////////////////
HDDnsDevice DDnsMrg_Create()
{
    return ArrayListCreate(sizeof(SDDnsDeviceInfo), (fFreeData)InitDDnsDevice);
}

SDDnsDeviceInfo *DDnsMrg_GetInfo(HDDnsDevice hManage, size_t nIndex)
{
    return (SDDnsDeviceInfo*)ArrayListGetData(hManage, nIndex);
}

size_t DDnsMrg_SelectDevice(HDDnsDevice hManage, LPCSTR szSerialID, SDDnsDeviceInfo **ppInfo)
{
    size_t i;
    SDDnsDeviceInfo *pNode = NULL;

    if(!szSerialID || !szSerialID[0])
        return -1;

    for(i=0; i<ArrayListGetSize(hManage); i++)
    {
        pNode = DDnsMrg_GetInfo(hManage, i);
        if(pNode == NULL
            || strcmp(pNode->szSerialID, szSerialID))
            continue;
        if(ppInfo != NULL)
            *ppInfo = pNode;
        return i;
    }
    return -1;
}
