/************************************************************************/
/* 设备操作单元                                                         */
/* 针对单个编码器设备的操作                                             */
/************************************************************************/
#ifndef _OV_DEVICECONTROL_H_
#define _OV_DEVICECONTROL_H_

#pragma once

#include "InfoManage.h"
#include "DllLoad.h"

#ifndef _WIN32
#define OUT
#endif

//////////////////////////////////////////////////////////////////////////
int LoadBasicLib(LPCSTR szFindPath);
void FreeBasicLib();
int LoadOneLib(SOV_CtrlPlug *pControlPlug, LPCSTR szIniPath, LPCSTR szFindPath);
int LoadPlugAPI(SOV_CtrlPlug *pControlPlug, LPCSTR szIniPath, LPCSTR szLibDirectory, LPCSTR szKeyFile);
void FreeOneLib(SOV_CtrlPlug *pControlPlug);

void Plug_RegistEvent(SOV_CtrlPlug *pControlPlug);
LPCSTR Plug_SelectByDNS(SOV_CtrlPlug *pControlPlug, LPCSTR szDnsAddress, LPCSTR szDeviceUrl);

//////////////////////////////////////////////////////////////////////////
long Device_Login(SDeviceCtrlInfo *pDevice);
long Device_Logout(SDeviceCtrlInfo *pDevice);
int Device_Listen(SDeviceCtrlInfo *pDevice);
int Device_GetDeviceAttrib(SDeviceCtrlInfo *pDevice);
int Device_SetCameraWholeEffect(SDeviceCtrlInfo *pDevice, int nChannel, SBaseEffects sEffects);
int Device_SetCameraOneEffect(SDeviceCtrlInfo *pDevice, int nChannel, int eEffect, int nValue);
int Device_DisposeAlarmWholeInput(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[]);
int Device_DisposeAlarmOneInput(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable);
int Device_GetAlarmStateInput(SDeviceCtrlInfo *pDevice);
int Device_DisposeAlarmWholeMotion(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[]);
int Device_DisposeAlarmOneMotion(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable);
int Device_GetAlarmStateMotion(SDeviceCtrlInfo *pDevice);
int Device_DisposeAlarmWholeBlind(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[]);
int Device_DisposeAlarmOneBlind(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable);
int Device_GetAlarmStateBlind(SDeviceCtrlInfo *pDevice);
int Device_DisposeAlarmWholeLost(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[]);
int Device_DisposeAlarmOneLost(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable);
int Device_GetAlarmStateLost(SDeviceCtrlInfo *pDevice);
int Device_DisposeAlarmWholeOutput(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[]);
int Device_DisposeAlarmOneOutput(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable);
int Device_GetAlarmStateOutput(SDeviceCtrlInfo *pDevice);
int Device_DisposeRecordWhole(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[]);
int Device_DisposeRecordOne(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable);
int Device_GetRecordState(SDeviceCtrlInfo *pDevice);
int Device_PtzControlBase(SDeviceCtrlInfo *pDevice, int nChannel, int ePtzAct, int nSpeed, int bStop);
int Device_PtzControlAdvice(SDeviceCtrlInfo *pDevice, int nChannel, char* szAdvCmd, int nCmdSize);
int Device_PtzPreset(SDeviceCtrlInfo *pDevice, int nChannel, int ePreserAct, int nPreset);
int Device_SetDateTime(SDeviceCtrlInfo *pDevice, SYSTEMTIME* pDateTime);
int Device_SetCameraName(SDeviceCtrlInfo *pDevice, int nChannel, const char* szCameraName);
int Device_SetCameraText(SDeviceCtrlInfo *pDevice, int nChannel, SCameraText* pCameraText);
int Device_SetCameraTextV2(SDeviceCtrlInfo *pDevice, int nChannel, int nTextIndex, SCameraText* pCameraText);
int Device_CaptureIFrame(SDeviceCtrlInfo *pDevice, int nChannel);
int Device_SerialOpen(SDeviceCtrlInfo *pDevice, int nSerialPort, dgSerialOutput pFunction, void* objUser);
int Device_SerialOpenParam(SDeviceCtrlInfo *pDevice, int nSerialPort, SSerialParam* pSerialParam, dgSerialOutput pFunction, void* objUser);
int Device_SerialClose(SDeviceCtrlInfo *pDevice, int nSerialPort);
int Device_SerialSend(SDeviceCtrlInfo *pDevice, int nSerialPort, int nChannel, int nDataLength, BYTE arrayData[]);
int Device_OpenMediaChannel(SDeviceCtrlInfo *pDevice, int nChannel, const char* szTargetAddress, int nTargetPort);
int Device_CloseMediaChannel(SDeviceCtrlInfo *pDevice, int nChannel, const char* szTargetAddress, int nTargetPort);
int Device_GetDeviceWorkstate(SDeviceCtrlInfo *pDevice);
int Device_GetMediaBitrate(SDeviceCtrlInfo *pDevice);

//////////////////////////////////////////////////////////////////////////
// inner background process
int Device_LoginNew(SDeviceCtrlInfo *pDevice);
int Device_LoginReonline(SDeviceCtrlInfo *pDevice);
int Device_SetEffectDefault(SDeviceCtrlInfo *pDevice, int nChannel);
int Device_GetAlarmStateAsync(SDeviceCtrlInfo *pDevice, int nAlarmType[], int nTypeLength);
int Device_MakeStateXML(SDeviceCtrlInfo *pDevice, char *szXMLBuffer);

//采集设备的特定布防状态
int Device_RequestAlarmState(SDeviceCtrlInfo *pDevice, int nAlarmType);
//得到设备的特定布防状态
int Device_GetAlarmState(SDeviceCtrlInfo *pDevice, int nAlarmType, int OUT *nAlarmSize, char** OUT pAlarmDispose);
int Device_GetValueState(SDeviceCtrlInfo *pDevice, int nAlarmType, int OUT *nValuesSize, int** OUT pValues1, int** OUT pValues2);
//以'0'/'1'字符串的形式输出设备的特定布防状态
int Device_FireAlarmState(SDeviceCtrlInfo *pDevice, int nAlarmType);

#endif	//_OV_DEVICECONTROL_H_
