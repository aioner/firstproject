#ifndef _INFOMANAGE_H_
#define _INFOMANAGE_H_

#include "CommonDef.h"
#include "ArrayList.h"
//////////////////////////////////////////////////////////////////////////
typedef void *HPlugMrg;

HPlugMrg PlugMrg_Create();
SOV_CtrlPlug *PlugMrg_GetInfo(HPlugMrg hManage, size_t nIndex);

size_t PlugMrg_SelectType(HPlugMrg hManage, int nDeviceType, SOV_CtrlPlug **ppInfo);

//////////////////////////////////////////////////////////////////////////
typedef void *HDeviceMrg;

HDeviceMrg DeviceMrg_Create();
SDeviceCtrlInfo *DeviceMrg_GetInfo(HDeviceMrg hManage, size_t nIndex);

size_t DeviceMrg_SelectIDS(HDeviceMrg hManage, LPCSTR szIDS, SDeviceCtrlInfo **ppInfo);
size_t DeviceMrg_SelectAddress(HDeviceMrg hManage, LPCSTR szAddress, SDeviceCtrlInfo **ppInfo);
size_t DeviceMrg_SelectLoginHandle(HDeviceMrg hManage, int nTypeID, int nLoginHandle, SDeviceCtrlInfo **ppInfo);

//////////////////////////////////////////////////////////////////////////
typedef void *HAlarmMrg;

HAlarmMrg AlarmMrg_Create();
SAlarmTimer *AlarmMrg_GetInfo(HAlarmMrg hManage, size_t nIndex);

size_t AlarmMrg_SelectAlarm(HAlarmMrg hManage, int nDeviceID, int nAlarmType, int nChannel, SAlarmTimer **ppInfo);

//////////////////////////////////////////////////////////////////////////
typedef void *HReloginMrg;

HReloginMrg ReloginMrg_Create();
SReloginDevice *ReloginMrg_GetInfo(HReloginMrg hManage, size_t nIndex);

size_t ReloginMrg_SelectDevice(HReloginMrg hManage, int nDeviceID, SReloginDevice **ppInfo);
size_t ReloginMrg_AddInfoCheck(HReloginMrg hManage, SReloginDevice *pInfo);
void ReloginMrg_DeleteInfoCheck(HReloginMrg hManage, int nDeviceID);

//////////////////////////////////////////////////////////////////////////
typedef HArrayList HDDnsDevice;

HDDnsDevice DDnsMrg_Create();
SDDnsDeviceInfo *DDnsMrg_GetInfo(HDDnsDevice hManage, size_t nIndex);

size_t DDnsMrg_SelectDevice(HDDnsDevice hManage, LPCSTR szSerialID, SDDnsDeviceInfo **ppInfo);
#endif//_INFOMANAGE_H_
