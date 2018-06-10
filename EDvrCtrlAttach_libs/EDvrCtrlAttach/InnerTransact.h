/************************************************************************/
/* 接入层功能性逻辑和兼容性逻辑                                         */
/************************************************************************/

#pragma once
#include "CommonDef.h"

#define CTRL_ONLINE_TIMER       1
#define CTRL_GETSTATE_TIMER     2
#define CTRL_SENDSTATE_TIMER    3
#define CTRL_ALARMFILTER_TIMER  4
#define CTRL_DDNSCHECK_TIMER    5
#define CTRL_OFFLINECHECK_TIMER	6

//标记是否上传指定告警类型，记录256种告警，每种4种子类型
extern BYTE s_bSendEvent[0x400];
extern char *s_szEventMark[0x400];

void FireOnlineChangedTrance(SEDvrCtrlEvent* pCtrlEvent, LPCSTR szIDS, LPCSTR szAddress, int nDeviceType, int nOnline, int nRes, LPCSTR szRes);
void FireOnlineChanged(SEDvrCtrlEvent* pCtrlEvent, SDeviceCtrlInfo *pDevice, int nDeviceType, int nOnline, int nRes, LPCSTR szRes);
void FireAlarmState(SEDvrCtrlEvent* pCtrlEvent, SDeviceCtrlInfo *pDevice, LPCSTR szStateList, int nStateSize, int nAlarmType, LPCSTR szRes);
void FireSerialData(SEDvrCtrlEvent* pCtrlEvent, SDeviceCtrlInfo *pDevice, int nSerialPort, void* pBufData, int nBufSize);
void FireDeviceIPChanged(SEDvrCtrlEvent* pCtrlEvent, LPCSTR szSerialNum, LPCSTR szAddress, int nDeviceType);
void FireDvrStateInfo(SEDvrCtrlEvent* pCtrlEvent, SDeviceCtrlInfo *pDevice);
void FireDvrStateChanged(SEDvrCtrlEvent* pCtrlEvent, SDeviceCtrlInfo *pDevice, int eStateEvent,\
                         int nChannel, int nEventValue, void* pEventData, int nDataSize);

void OnlineCheck_Start(SEDvrCtrlEvent* pCtrlEvent);
void OnlineCheck_Stop(SEDvrCtrlEvent* pCtrlEvent);

void StateListen_Start(SEDvrCtrlEvent* pCtrlEvent);
void StateListen_Stop(SEDvrCtrlEvent* pCtrlEvent);

void AlarmFilter_Start(SEDvrCtrlEvent* pCtrlEvent);
void AlarmFilter_Stop(SEDvrCtrlEvent* pCtrlEvent);

int AlarmDispose_Check(SDeviceCtrlInfo *pDevice, int nAlarmType, int nChannel, int nAlarmValue, int nDuration);
//nDuration 持续时间
void AlarmSignla_Receive(SDeviceCtrlInfo *pDevice, int nAlarmType, int nChannel, int nAlarmValue, int nDuration);
void AlarmSignla_Clean(SDeviceCtrlInfo *pDevice, int nAlarmType, int nChannel);

void DDnsCheck_Start(SEDvrCtrlEvent *pCtrlEvent);
void DDnsCheck_Stop(SEDvrCtrlEvent *pCtrlEvent);

void OfflineCheck_Start(SEDvrCtrlEvent* pCtrlEvent);
void OfflineCheck_Stop(SEDvrCtrlEvent* pCtrlEvent);

void CALLBACK OfflineTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent);
void CALLBACK OnlineTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent);
void CALLBACK AlarmTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent);
void CALLBACK GetStateTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent);
void CALLBACK SendStateTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent);
void CALLBACK DDnsCheckTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent);

//////////////////////////////////////////////////////////////////////////
void CALLBACK OnDeviceAddressChanged(LPCSTR szUrl, LPCSTR szAddress, int nDeviceType, void* objUser);
void CALLBACK OnDeviceOnlineChanged(int nLoginID, LPCSTR szAddress, int nDeviceType, int nOnlineFlag, void* objUser);
void CALLBACK OnDeviceAlarmChanged(int nLoginID, LPCSTR szAddress, int nDeviceType, int nChannel, int eStateEvent,\
                                         int nEventValue, BYTE pEventData[], int nDataSize, void* objUser);
void CALLBACK OnSerialOutput(int nLoginID, LPCSTR szAddress, int lPortID, BYTE pDataBuf[], int lBufSize, void* objUser);
