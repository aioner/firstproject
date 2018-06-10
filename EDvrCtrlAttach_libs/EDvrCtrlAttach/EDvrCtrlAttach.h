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

//������������(0~255)
enum ECtrlEffect
{
    eCtrlBrightness = 1,
    eCtrlContrast = 2,
    eCtrlHue = 3,
    eCtrlSaturation = 4,
    eCtrlDefault = 6,
};

//��̨��������
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
    eCameraPower = 0x500,               //�������Դ
    eCleanPower = 0x600,                //����
    eHeaterPower = 0x700,               //���ȡ���˪
    eWiperPower = 0x800,                //���
    eLightPower = 0x900,                //�ƹ�
};

//��̨��������ֹͣ
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
    eCameraPowerStop = 0x501,               //�������Դ
    eCleanPowerStop = 0x601,                //����
    eHeaterPowerStop = 0x701,               //���ȡ���˪
    eWiperPowerStop = 0x801,                //���
    eLightPowerStop = 0x901,                //�ƹ�
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
    //�ź�������
    eStateAlarmIn = 0,                  //��������
    eStateDiskFull = 1,                 //������
    eStateVideoLost = 2,                //��Ƶ��ʧ
    eStateMotionDetection = 3,          //�ƶ����
    eStateNoForamt = 4,                 //û�и�ʽ��
    eStateDiskError = 5,                //���̴���
    eStateVideoBlind = 6,               //��Ƶ�ڵ�
    eStateNoCriterion = 7,              //��ʽ��ƥ��
    eStateIllegalAccess = 8,            //�ź����
    eStateDataLost = 9,                 //���ݶ�ʧ
    eStateDiskSize = 0x0A,              //��������
    eStateDiskSpace = 0x0B,             //����ʣ��ռ�
    eStateDiskState = 0x0C,             //���̹���״̬
    eStateDeviceState = 0x0D,           //�豸����״̬
	eStateVideoAnalyzer = 0x0E,         //��Ƶ����
	eStateRecord = 0x0F,				//¼��״̬

    //ֵ����
    eStateMediaBitrate = STATE_VALUE(9),//��ý��������Ϣ
};

//������ڲ���
typedef struct _SBaseEffects
{
    BYTE nBrightness;                   //����
    BYTE nContrast;                     //�Աȶ�
    BYTE nHue;                          //ɫ��
    BYTE nSaturation;                   //���Ͷ�
}SBaseEffects;

//�豸����
typedef struct _SDeviceType
{
    int m_nDeviceType;          //�豸���ͺţ�-1��ѯ��ʼ��-2��ѯ������>=0��Ӧ�豸����
    int m_nControlPort;         //�豸���ƶ˿�
    char m_szTypeName[32];      //�豸��������
    char m_szUser[MAX_USERNAME_LENGTH];//��¼�û���
    char m_szPassword[MAX_PASSWORD_LENGTH];//��¼����
}SDeviceType;

//���ڲ���
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
CONTROL_API int EDVR_API Ctrl_RegistAlarmStateEvent(dgOnAlarmStateEvent pFunction, void *objUser);//��������״̬
CONTROL_API int EDVR_API Ctrl_RegistSerialEvent(dgOnSerialEvent pFunction, void *objUser);		//͸������������
CONTROL_API int EDVR_API Ctrl_RegistDeviceIPChange(dgOnDeviceIPChange pFunction, void *objUser);	//�豸�����ַ�ı�
CONTROL_API int EDVR_API Ctrl_RegistDvrStateInfo(dgOnDvrStateInfo pFunction, void *objUser);		//�豸״̬����
CONTROL_API int EDVR_API Ctrl_RegistDvrStateChanged(dgOnDvrStateChanged pFunction, void *objUser);//�豸״̬�ı䣨�������������Է����仯��

CONTROL_API int EDVR_API Ctrl_ClientAddDvr(LPCSTR szSerialNum, int nDeviceType);
CONTROL_API int EDVR_API Ctrl_ClientJoin(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType);
CONTROL_API int EDVR_API Ctrl_ClientJoinV2(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType,\
                                  int nSubType, int nCtrlPort, LPCSTR szUserName, LPCSTR szPassword);
CONTROL_API int EDVR_API Ctrl_ClientLeave(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType);
CONTROL_API int EDVR_API Ctrl_ReflashDeviceList(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType, int nReflashState);

//��������
CONTROL_API int EDVR_API Ctrl_ClientAlarm(LPCSTR szIDS, int nChannel, int bEnable);//��������
CONTROL_API int EDVR_API Ctrl_ClientMotion(LPCSTR szIDS, int nChannel, int bEnable);//�ƶ����
CONTROL_API int EDVR_API Ctrl_ClientRecord(LPCSTR szIDS, int nChannel, int bEnable);//�豸¼��
CONTROL_API int EDVR_API Ctrl_ClientOutput(LPCSTR szIDS, int nChannel, int bEnable);//�������
CONTROL_API int EDVR_API Ctrl_ClientLost(LPCSTR szIDS, int nChannel, int bEnable);//��Ƶ��ʧ
CONTROL_API int EDVR_API Ctrl_ClientBlind(LPCSTR szIDS, int nChannel, int bEnable);//��Ƶ�ڵ�

//����״̬��������������
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
