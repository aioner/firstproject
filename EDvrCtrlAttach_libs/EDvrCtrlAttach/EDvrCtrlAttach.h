#ifndef _EDVR_CTRL_ATTACH_H__INCLUDE_
#define _EDVR_CTRL_ATTACH_H__INCLUDE_
#pragma once

#ifdef _WIN32
#define EDVR_API
#ifdef EDVRCTRLATTACH_EXPORTS
#define CONTROL_API EXTERN_C __declspec(dllexport)
#include "CommonDef.h"
#else
#include <Windows.h>

#define CONTROL_API EXTERN_C __declspec(dllimport)
#define MAX_TYPENAME_LENGTH 64
#define MAX_USERNAME_LENGTH 16
#define MAX_PASSWORD_LENGTH 32

//摄像机画面参数(0~255)
enum ECtrlEffect
{
    eCtrlBrightness = 1,
    eCtrlContrast = 2,
    eCtrlHue = 3,
    eCtrlSaturation = 4,
    eCtrlDefault = 6,
};

//云台基本动作
enum EPtzAct
{
    eIrisLarge = 0x100,
    eIrisSmall = 0x110,
    eZoomLarge = 0x200,
    eZoomSmall = 0x210,
    eFocusLarge = 0x300,
    eFocusSmall = 0x310,
    ePtzDown = 0x400,
    ePtzRight = 0x410,
    ePtzLeft = 0x420,
    ePtzUp = 0x430,
    ePtzAuto = 0x440,
    ePtzLeftUp = 0x450,
    ePtzRightUp = 0x460,
    ePtzLeftDown = 0x470,
    ePtzRightDown = 0x480,
    eCameraPower = 0x500,               //摄像机电源
    eCleanPower = 0x600,                //除尘
    eHeaterPower = 0x700,               //加热、除霜
    eWiperPower = 0x800,                //雨刮
    eLightPower = 0x900,                //灯光
};

//云台基本动作停止
enum EPtzActStop
{
    eIrisLargeStop = 0x101,
    eIrisSmallStop = 0x111,
    eZoomLargeStop = 0x201,
    eZoomSmallStop = 0x211,
    eFocusLargeStop = 0x301,
    eFocusSmallStop = 0x311,
    ePtzDownStop = 0x401,
    ePtzRightStop = 0x411,
    ePtzLeftStop = 0x421,
    ePtzUpStop = 0x431,
    ePtzAutoStop = 0x441,
    ePtzLeftUpStop = 0x451,
    ePtzRightUpStop = 0x461,
    ePtzLeftDownStop = 0x471,
    ePtzRightDownStop = 0x481,
    eCameraPowerStop = 0x501,               //摄像机电源
    eCleanPowerStop = 0x601,                //除尘
    eHeaterPowerStop = 0x701,               //加热、除霜
    eWiperPowerStop = 0x801,                //雨刮
    eLightPowerStop = 0x901,                //灯光
};

enum ECtrlPresetAct
{
    eCtrlPresetDelete = 0,
    eCtrlPresetAdd = 1,
    eCtrlPresetEdit = 2,
    eCtrlPresetMove = 3,
};

#define STATE_VALUE(x) 0x10000+x
enum EStateEvent
{
    //信号量类型
    eStateAlarmIn = 0,                  //报警输入
    eStateDiskFull = 1,                 //磁盘满
    eStateVideoLost = 2,                //视频丢失
    eStateMotionDetection = 3,          //移动侦测
    eStateNoForamt = 4,                 //没有格式化
    eStateDiskError = 5,                //磁盘错误
    eStateVideoBlind = 6,               //视频遮挡
    eStateNoCriterion = 7,              //制式不匹配
    eStateIllegalAccess = 8,            //信号输出
    eStateDataLost = 9,                 //数据丢失
    eStateDiskSize = 0x0A,              //磁盘容量
    eStateDiskSpace = 0x0B,             //磁盘剩余空间
    eStateDiskState = 0x0C,             //磁盘工作状态
    eStateDeviceState = 0x0D,           //设备工作状态
	eStateVideoAnalyzer = 0x0E,         //视频分析
	eStateRecord = 0x0F,				//录像状态

    //值类型
    eStateMediaBitrate = STATE_VALUE(9),//流媒体码率信息
};

//画面调节参数
typedef struct _SBaseEffects
{
    BYTE nBrightness;                   //亮度
    BYTE nContrast;                     //对比度
    BYTE nHue;                          //色度
    BYTE nSaturation;                   //饱和度
}SBaseEffects;

//设备类型
typedef struct _SDeviceType
{
    int m_nDeviceType;          //设备类型号，-1查询开始，-2查询结束，>=0对应设备类型
    int m_nControlPort;         //设备控制端口
    char m_szTypeName[32];      //设备类型描述
    char m_szUser[MAX_USERNAME_LENGTH];//登录用户名
    char m_szPassword[MAX_PASSWORD_LENGTH];//登录密码
}SDeviceType;

//串口参数
typedef struct _SSerialParam
{
    BYTE m_nDataBit;
    BYTE m_nStopBit;
    BYTE m_nParity;
    BYTE m_nFlowControl;
    int  m_nBaudRate;
}SSerialParam;

typedef int  (__stdcall* dgDeviceType)(int nCount, SDeviceType *pDeviceType, void *objUser);
typedef void (__stdcall* dgOnOnlineEvent)(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType, int nOnline, int nRes, LPCSTR szRes, void *objUser);
typedef void (__stdcall* dgOnAlarmStateEvent)(LPCSTR szIDS, LPCSTR szStateList, int nStateSize, int nAlarmType, LPCSTR szRes, void *objUser);
typedef void (__stdcall* dgOnSerialEvent)(LPCSTR szIDS, int nSerialPort, void* pBufData, int nBufSize, void *objUser);
typedef void (__stdcall* dgOnDeviceIPChange)(LPCSTR szSerialNum, LPCSTR szAddress, int nDeviceType, void *objUser);
typedef void (__stdcall* dgOnDvrStateInfo)(LPCSTR szIDS, LPCSTR szAddress, LPCSTR xmlStateInfo, int nInfoSize, void *objUser);
typedef void (__stdcall* dgOnDvrStateChanged)(LPCSTR szIDS, LPCSTR szAddress, int eStateEvent,\
                                             int nChannel, int nEventValue, void* pEventData, int nDataSize, void *objUser);

#endif//EDVR_CTRL_ATTACH_EXPORTS
#else
#include "CommonDef.h"
#define CONTROL_API extern "C" 
#define EDVR_API __attribute__((visibility("default")))
#endif//_WIN32
//////////////////////////////////////////////////////////////////////////
CONTROL_API int EDVR_API Ctrl_Startup(int nRes, char szRes[]);
CONTROL_API int EDVR_API Ctrl_Cleanup();
CONTROL_API int EDVR_API Ctrl_GetDeviceTypeList(dgDeviceType fpDeviceType, void *objUser);

CONTROL_API int EDVR_API Ctrl_RegistOnlineEvent(dgOnOnlineEvent pFunction, void *objUser);
CONTROL_API int EDVR_API Ctrl_RegistAlarmStateEvent(dgOnAlarmStateEvent pFunction, void *objUser);//报警布防状态
CONTROL_API int EDVR_API Ctrl_RegistSerialEvent(dgOnSerialEvent pFunction, void *objUser);		//透明串口收数据
CONTROL_API int EDVR_API Ctrl_RegistDeviceIPChange(dgOnDeviceIPChange pFunction, void *objUser);	//设备网络地址改变
CONTROL_API int EDVR_API Ctrl_RegistDvrStateInfo(dgOnDvrStateInfo pFunction, void *objUser);		//设备状态监听
CONTROL_API int EDVR_API Ctrl_RegistDvrStateChanged(dgOnDvrStateChanged pFunction, void *objUser);//设备状态改变（报警和其它属性发生变化）

CONTROL_API int EDVR_API Ctrl_ClientAddDvr(LPCSTR szSerialNum, int nDeviceType);
CONTROL_API int EDVR_API Ctrl_ClientJoin(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType);
CONTROL_API int EDVR_API Ctrl_ClientJoinV2(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType,\
                                  int nSubType, int nCtrlPort, LPCSTR szUserName, LPCSTR szPassword);
CONTROL_API int EDVR_API Ctrl_ClientLeave(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType);
CONTROL_API int EDVR_API Ctrl_ReflashDeviceList(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType, int nReflashState);

//布防开关
CONTROL_API int EDVR_API Ctrl_ClientAlarm(LPCSTR szIDS, int nChannel, int bEnable);//报警输入
CONTROL_API int EDVR_API Ctrl_ClientMotion(LPCSTR szIDS, int nChannel, int bEnable);//移动侦测
CONTROL_API int EDVR_API Ctrl_ClientRecord(LPCSTR szIDS, int nChannel, int bEnable);//设备录像
CONTROL_API int EDVR_API Ctrl_ClientOutput(LPCSTR szIDS, int nChannel, int bEnable);//报警输出
CONTROL_API int EDVR_API Ctrl_ClientLost(LPCSTR szIDS, int nChannel, int bEnable);//视频丢失
CONTROL_API int EDVR_API Ctrl_ClientBlind(LPCSTR szIDS, int nChannel, int bEnable);//视频遮挡

//布防状态（布防、撤防）
CONTROL_API int EDVR_API Ctrl_ClientAlarmState(LPCSTR szIDS);
CONTROL_API int EDVR_API Ctrl_ClientMotionState(LPCSTR szIDS);
CONTROL_API int EDVR_API Ctrl_ClientRecordState(LPCSTR szIDS);
CONTROL_API int EDVR_API Ctrl_ClientOutputState(LPCSTR szIDS);
CONTROL_API int EDVR_API Ctrl_ClientLostState(LPCSTR szIDS);
CONTROL_API int EDVR_API Ctrl_ClientBlindState(LPCSTR szIDS);
CONTROL_API int EDVR_API Ctrl_ClientMediaBitrate(LPCSTR szIDS);

CONTROL_API int EDVR_API Ctrl_SetDateTime(LPCSTR szIDS, int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec);

CONTROL_API int EDVR_API Ctrl_ClientImage(LPCSTR szIDS, int nChannel, int eCtrlEffect, int nValue);
CONTROL_API int EDVR_API Ctrl_ClientImageWhole(LPCSTR szIDS, int nChannel, SBaseEffects sEffects);
CONTROL_API int EDVR_API Ctrl_SetCameraName(LPCSTR szIDS, int nChannel, LPCSTR szName);
CONTROL_API int EDVR_API Ctrl_SetCameraText(LPCSTR szIDS, int nChannel, int bIsShow, LPCSTR szText,\
                       int nLeft, int nTop, int nWidth, int nHeight, int nRGBColor);
CONTROL_API int EDVR_API Ctrl_SetCameraTextV2(LPCSTR szIDS, int nChannel, int nTextIndex, int bIsShow,\
                       LPCSTR szText, int nLeft, int nTop, int nWidth, int nHeight, int nRGBColor);
CONTROL_API int EDVR_API Ctrl_CaptureIFrame(LPCSTR szIDS, int nChannel);

CONTROL_API int EDVR_API Ctrl_ClientPtz(LPCSTR szIDS, int nChannel, int ePtzAct);
CONTROL_API int EDVR_API Ctrl_ClientPtzSpeed(LPCSTR szIDS, int nChannel, int ePtzAct, int nSpeed);
CONTROL_API int EDVR_API Ctrl_ClientPtzAdvance(LPCSTR szIDS, int nChannel, char* szAdvCmd, int nCmdSize);
CONTROL_API int EDVR_API Ctrl_ClientPtzPreset(LPCSTR szIDS, int nChannel, int eCtrlPresetAct, int nPreset);

CONTROL_API int EDVR_API Ctrl_ClientSerialOpen(LPCSTR szIDS, int nSerialPort);
CONTROL_API int EDVR_API Ctrl_ClientSerialOpenParam(LPCSTR szIDS, int nSerialPort, SSerialParam *pSerialParam);
CONTROL_API int EDVR_API Ctrl_ClientSerialClose(LPCSTR szIDS, int nSerialPort);
CONTROL_API int EDVR_API Ctrl_ClientSerialSend(LPCSTR szIDS, int nSerialPort, int nChannel, void* pBufData, int nBufSize);

CONTROL_API int EDVR_API Ctrl_ClientStartTVOut(LPCSTR szIDS, int nChannel, LPCSTR szTargetIDS, int nTargetChannel);
CONTROL_API int EDVR_API Ctrl_ClientStopTVOut(LPCSTR szIDS, int nChannel, LPCSTR szTargetIDS, int nTargetChannel);

//////////////////////////////////////////////////////////////////////////

CONTROL_API int EDVR_API Ctrlv2_SetDateTime(LPCSTR szAddress, int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec);

CONTROL_API int EDVR_API Ctrlv2_ClientImage(LPCSTR szAddress, int nChannel, int eCtrlEffect, int nValue);
CONTROL_API int EDVR_API Ctrlv2_ClientImageWhole(LPCSTR szAddress, int nChannel, SBaseEffects sEffects);
CONTROL_API int EDVR_API Ctrlv2_CaptureIFrame(LPCSTR szAddress, int nChannel);

#endif//_EDVR_CTRL_ATTACH_H__INCLUDE_
