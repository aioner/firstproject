#ifndef _CONTROLPLUG_H_
#define _CONTROLPLUG_H_
#pragma once

//#ifdef WIN32
//typedef int LOGINTYPE;
//#else
//typedef long long LOGINTYPE;
//#endif
typedef int LOGINTYPE;

#define MAX_CAMERA_NUM 48               //��������
#define MAX_ALARM_INPUT 32              //���澯�����
#define MAX_ALARM_OUTPUT 16             //���澯�����
#define MAX_HARDDISK_NUM 48             //������

//������������(0~255)
enum EEffect
{
    eBrightness = 0,
    eContrast = 1,
    eSaturation = 2,
    eHue = 3,
};

//��̨��������
enum EPtzAct
{
    eIrisLarge = 0x100,                 //��Ȧ
    eIrisSmall = 0x110,                 
    eZoomLarge = 0x200,                 //��ͷԶ��
    eZoomSmall = 0x210,                 
    eFocusLarge = 0x300,                //�۽�
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

//��̨Ԥ�õ㶯��
enum EPresetAct
{
    ePresetDelete = 0,                  //���
    ePresetSet = 1,                     //����
    ePresetMove = 2,                    //ת��
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

//�豸������Ϣ
typedef struct _SDeviceAttribute
{
    BYTE m_nVideoCount;                 //��Ƶ��
    BYTE m_nAudioCount;                 //��Ƶ��
    BYTE m_nAlarmInCount;               //��������
    BYTE m_nAlarmOutCount;              //�������
    BYTE m_nEthernetCount;              //����
    BYTE m_nUsbCount;                   //USB�ӿ�
    BYTE m_nRomCount;                   //�ⲿ�洢���ӿ�
    BYTE m_nSerialCount;                //���п�
    BYTE m_nLPTCount;                   //���п�
    BYTE m_nRes[3];
}SDeviceAttribute;

//������ڲ���
typedef struct _SBaseEffects
{
    BYTE nBrightness;                   //����
    BYTE nContrast;                     //�Աȶ�
    BYTE nHue;                          //ɫ��
    BYTE nSaturation;                   //���Ͷ�
}SBaseEffects;

//���ڲ���
typedef struct _SSerialParam
{
    BYTE m_nDataBit;		//����λ[4-8]
    BYTE m_nStopBit;		//ֹͣλ[0-2](1��1.5��2)
    BYTE m_nParity;			//У��λ[0-2](�ޡ��桢ż)
    BYTE m_nFlowControl;	//���ط�ʽ[0-2](�ޡ���Ӳ)
    int  m_nBaudRate;		//������[���Ͳ���]
}SSerialParam;

//�ı���������
typedef struct _SCameraText
{
    short nLeft;
    short nTop;
    short nWidth;
    short nHeight;
    short bShow;                        //�Ƿ���ʾ
    short nTextLength;                  //
    int nRGBColor;                      //RGB32λɫ��
    char* szText;
}SCameraText;

//�豸����״̬
typedef struct _SDeviceWrokstate
{
    int nDeviceState;                   //������״̬0-������1-�߸��ɣ�2-����
    int nDiskNum;
    BYTE nDiskState[MAX_HARDDISK_NUM];  //0-������1-�ռ�����2-�쳣��3-����
    DWORD nDiskSize[MAX_HARDDISK_NUM];  //����������
    DWORD nDiskSpace[MAX_HARDDISK_NUM]; //���̿ռ�
}SDeviceWrokstate;


// �����豸����
typedef void (__stdcall* dgAddressChanged)(LPCSTR szUrl, LPCSTR szAddress, int nDeviceType, void* objUser);
typedef void (__stdcall* dgOnlineChanged)(int nLoginID, LPCSTR szAddress, int nDeviceType, int nOnlineFlag, void* objUser);
typedef void (__stdcall* dgAlarmChanged)(int nLoginID, LPCSTR szAddress, int nDeviceType, int nChannel, int eStateEvent,\
                                         int nEventValue, BYTE pEventData[], int nDataSize, void* objUser);
typedef void (__stdcall* dgSerialOutput)(int nLoginID, LPCSTR szAddress, int lPortID, BYTE pDataBuf[], int lBufSize, void* objUser);

// �����豸����
typedef int (__stdcall* dgStartup               )(int nDeviceType, char szRes[]);
typedef int (__stdcall* dgCleanup               )();
typedef int (__stdcall* dgRegistAddressChanged  )(dgAddressChanged pFunction, void* objUser);
typedef int (__stdcall* dgRegistOnlineChanged   )(dgOnlineChanged pFunction, void* objUser);
typedef int (__stdcall* dgRegistAlarmChanged    )(dgAlarmChanged pFunction, void* objUser);
typedef int (__stdcall* dgGetLogin              )(LPCSTR szAddress, int nCtlPort);
typedef LOGINTYPE (__stdcall* dgLogin           )(LPCSTR szAddress, int nCtlPort, LPCSTR szUser,LPCSTR szPassword, int nSubType);
typedef int (__stdcall* dgLoginLe               )(LPCSTR szAddress);
typedef int (__stdcall* dgLogout                )(LOGINTYPE nLoginID);
typedef int (__stdcall* dgListen                )(LOGINTYPE nLoginID);
typedef int (__stdcall* dgGetDeviceAttrib       )(LOGINTYPE nLoginID, SDeviceAttribute* pAttribs);
typedef int (__stdcall* dgSetCameraWholeEffect  )(LOGINTYPE nLoginID, int nChannel, SBaseEffects sEffects);
typedef int (__stdcall* dgSetCameraOneEffect    )(LOGINTYPE nLoginID, int nChannel, int eEffect, int nValue);
typedef int (__stdcall* dgDisposeAlarmWholeInput)(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeAlarmOneInput  )(LOGINTYPE nLoginID, int nChannel, int bEnable);
typedef int (__stdcall* dgGetAlarmStateInput    )(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeAlarmWholeMotion)(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeAlarmOneMotion )(LOGINTYPE nLoginID, int nChannel, int bEnable);
typedef int (__stdcall* dgGetAlarmStateMotion   )(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeAlarmWholeBlind)(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeAlarmOneBlind  )(LOGINTYPE nLoginID, int nChannel, int bEnable);
typedef int (__stdcall* dgGetAlarmStateBlind    )(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeAlarmWholeLost )(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeAlarmOneLost   )(LOGINTYPE nLoginID, int nChannel, int bEnable);
typedef int (__stdcall* dgGetAlarmStateLost     )(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeAlarmWholeOutput)(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeAlarmOneOutput )(LOGINTYPE nLoginID, int nChannel, int bEnable);
typedef int (__stdcall* dgGetAlarmStateOutput   )(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeRecordWhole    )(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgDisposeRecordOne      )(LOGINTYPE nLoginID, int nChannel, int bEnable);
typedef int (__stdcall* dgGetRecordState        )(LOGINTYPE nLoginID, int nArraySize, BYTE arrayEnable[]);
typedef int (__stdcall* dgPtzControlBase        )(LOGINTYPE nLoginID, int nChannel, int ePtzAct, int nSpeed, int bStop);
typedef int (__stdcall* dgPtzControlAdvice      )(LOGINTYPE nLoginID, int nChannel, char* szAdvCmd, int nCmdSize);
typedef int (__stdcall* dgPtzPreset             )(LOGINTYPE nLoginID, int nChannel, int ePreserAct, int nPreset);
typedef int (__stdcall* dgSetDateTime           )(LOGINTYPE nLoginID, SYSTEMTIME* pDateTime);
typedef int (__stdcall* dgSetCameraName         )(LOGINTYPE nLoginID, int nChannel, LPCSTR szCameraName);
typedef int (__stdcall* dgSetCameraText         )(LOGINTYPE nLoginID, int nChannel, SCameraText* pCameraText);
typedef int (__stdcall* dgSetCameraTextV2       )(LOGINTYPE nLoginID, int nChannel, int nTextIndex, SCameraText* pCameraText);
typedef int (__stdcall* dgCaptureIFrame         )(LOGINTYPE nLoginID, int nChannel);
typedef int (__stdcall* dgSerialOpen            )(LOGINTYPE nLoginID, int nSerialPort, dgSerialOutput pFunction, void* objUser);
typedef int (__stdcall* dgSerialOpenParam       )(LOGINTYPE nLoginID, int nSerialPort, SSerialParam* pSerialParam, dgSerialOutput pFunction, void* objUser);
typedef int (__stdcall* dgSerialClose           )(LOGINTYPE nLoginID, int nSerialPort);
typedef int (__stdcall* dgSerialSend            )(LOGINTYPE nLoginID, int nSerialPort, int nChannel, int nDataLength, BYTE arrayData[]);
typedef int (__stdcall* dgOpenMediaChannel      )(LOGINTYPE nLoginID, int nChannel, LPCSTR szTargetAddress, int nTargetPort);
typedef int (__stdcall* dgCloseMediaChannel     )(LOGINTYPE nLoginID, int nChannel, LPCSTR szTargetAddress, int nTargetPort);
typedef int (__stdcall* dgGetDeviceWorkstate    )(LOGINTYPE nLoginID, SDeviceWrokstate* pWorkstate);
typedef LPCSTR (__stdcall* dgSelectByDNS        )(LPCSTR szDnsAddress, LPCSTR szDeviceUrl);
typedef int (__stdcall* dgGetMediaBitrate       )(LOGINTYPE nLoginID, int nArraySize, int arrayPrimaryRate[], int arraySecondaryRate[]);

//�ײ�����API��ַָ��
#define DECLARE_DELEGATE(name) dg##name m_dg##name
typedef struct _SOV_CtrlAPI
{
    HINSTANCE m_hInstance;
    DECLARE_DELEGATE(Startup                );
    DECLARE_DELEGATE(Cleanup                );
    DECLARE_DELEGATE(RegistAddressChanged   );
    DECLARE_DELEGATE(RegistOnlineChanged    );
    DECLARE_DELEGATE(RegistAlarmChanged     );
    DECLARE_DELEGATE(GetLogin               );
    DECLARE_DELEGATE(Login                  );
    DECLARE_DELEGATE(LoginLe                );
    DECLARE_DELEGATE(Logout                 );
    DECLARE_DELEGATE(Listen                 );
    DECLARE_DELEGATE(GetDeviceAttrib        );
    DECLARE_DELEGATE(SetCameraWholeEffect   );
    DECLARE_DELEGATE(SetCameraOneEffect     );
    DECLARE_DELEGATE(DisposeAlarmWholeInput );
    DECLARE_DELEGATE(DisposeAlarmOneInput   );
    DECLARE_DELEGATE(GetAlarmStateInput     );
    DECLARE_DELEGATE(DisposeAlarmWholeMotion);
    DECLARE_DELEGATE(DisposeAlarmOneMotion  );
    DECLARE_DELEGATE(GetAlarmStateMotion    );
    DECLARE_DELEGATE(DisposeAlarmWholeBlind );
    DECLARE_DELEGATE(DisposeAlarmOneBlind   );
    DECLARE_DELEGATE(GetAlarmStateBlind     );
    DECLARE_DELEGATE(DisposeAlarmWholeLost  );
    DECLARE_DELEGATE(DisposeAlarmOneLost    );
    DECLARE_DELEGATE(GetAlarmStateLost      );
    DECLARE_DELEGATE(DisposeAlarmWholeOutput);
    DECLARE_DELEGATE(DisposeAlarmOneOutput  );
    DECLARE_DELEGATE(GetAlarmStateOutput    );
    DECLARE_DELEGATE(DisposeRecordWhole     );
    DECLARE_DELEGATE(DisposeRecordOne       );
    DECLARE_DELEGATE(GetRecordState         );
    DECLARE_DELEGATE(PtzControlBase         );
    DECLARE_DELEGATE(PtzControlAdvice       );
    DECLARE_DELEGATE(PtzPreset              );
    DECLARE_DELEGATE(SetDateTime            );
    DECLARE_DELEGATE(SetCameraName          );
    DECLARE_DELEGATE(SetCameraText          );
    DECLARE_DELEGATE(SetCameraTextV2        );
    DECLARE_DELEGATE(CaptureIFrame          );
    DECLARE_DELEGATE(SerialOpen             );
    DECLARE_DELEGATE(SerialOpenParam        );
    DECLARE_DELEGATE(SerialClose            );
    DECLARE_DELEGATE(SerialSend             );
    DECLARE_DELEGATE(OpenMediaChannel       );
    DECLARE_DELEGATE(CloseMediaChannel      );
    DECLARE_DELEGATE(GetDeviceWorkstate     );
    DECLARE_DELEGATE(SelectByDNS            );
    DECLARE_DELEGATE(GetMediaBitrate        );
}SOV_CtrlAPI;

static const char c_szStartup                [] = "OVCtrl_Startup";
static const char c_szCleanup                [] = "OVCtrl_Cleanup";
static const char c_szRegistAddressChanged   [] = "OVCtrl_RegistAddressChanged";
static const char c_szRegistOnlineChanged    [] = "OVCtrl_RegistOnlineChanged";
static const char c_szRegistAlarmChanged     [] = "OVCtrl_RegistAlarmChanged";
static const char c_szGetLogin               [] = "OVCtrl_GetLogin";
static const char c_szLogin                  [] = "OVCtrl_Login";
static const char c_szLoginLe                [] = "OVCtrl_LoginLe";
static const char c_szLogout                 [] = "OVCtrl_Logout";
static const char c_szListen                 [] = "OVCtrl_Listen";
static const char c_szGetDeviceAttrib        [] = "OVCtrl_GetDeviceAttrib";
static const char c_szSetCameraWholeEffect   [] = "OVCtrl_SetCameraWholeEffect";
static const char c_szSetCameraOneEffect     [] = "OVCtrl_SetCameraOneEffect";
static const char c_szDisposeAlarmWholeInput [] = "OVCtrl_DisposeAlarmWholeInput";
static const char c_szDisposeAlarmOneInput   [] = "OVCtrl_DisposeAlarmOneInput";
static const char c_szGetAlarmStateInput     [] = "OVCtrl_GetAlarmStateInput";
static const char c_szDisposeAlarmWholeMotion[] = "OVCtrl_DisposeAlarmWholeMotion";
static const char c_szDisposeAlarmOneMotion  [] = "OVCtrl_DisposeAlarmOneMotion";
static const char c_szGetAlarmStateMotion    [] = "OVCtrl_GetAlarmStateMotion";
static const char c_szDisposeAlarmWholeBlind [] = "OVCtrl_DisposeAlarmWholeBlind";
static const char c_szDisposeAlarmOneBlind   [] = "OVCtrl_DisposeAlarmOneBlind";
static const char c_szGetAlarmStateBlind     [] = "OVCtrl_GetAlarmStateBlind";
static const char c_szDisposeAlarmWholeLost  [] = "OVCtrl_DisposeAlarmWholeLost";
static const char c_szDisposeAlarmOneLost    [] = "OVCtrl_DisposeAlarmOneLost";
static const char c_szGetAlarmStateLost      [] = "OVCtrl_GetAlarmStateLost";
static const char c_szDisposeAlarmWholeOutput[] = "OVCtrl_DisposeAlarmWholeOutput";
static const char c_szDisposeAlarmOneOutput  [] = "OVCtrl_DisposeAlarmOneOutput";
static const char c_szGetAlarmStateOutput    [] = "OVCtrl_GetAlarmStateOutput";
static const char c_szDisposeRecordWhole     [] = "OVCtrl_DisposeRecordWhole";
static const char c_szDisposeRecordOne       [] = "OVCtrl_DisposeRecordOne";
static const char c_szGetRecordState         [] = "OVCtrl_GetRecordState";
static const char c_szPtzControlBase         [] = "OVCtrl_PtzControlBase";
static const char c_szPtzControlAdvice       [] = "OVCtrl_PtzControlAdvice";
static const char c_szPtzPreset              [] = "OVCtrl_PtzPreset";
static const char c_szSetDateTime            [] = "OVCtrl_SetDateTime";
static const char c_szSetCameraName          [] = "OVCtrl_SetCameraName";
static const char c_szSetCameraText          [] = "OVCtrl_SetCameraText";
static const char c_szSetCameraTextV2        [] = "OVCtrl_SetCameraTextV2";
static const char c_szCaptureIFrame          [] = "OVCtrl_CaptureIFrame";
static const char c_szSerialOpen             [] = "OVCtrl_SerialOpen";
static const char c_szSerialOpenParam        [] = "OVCtrl_SerialOpenParam";
static const char c_szSerialClose            [] = "OVCtrl_SerialClose";
static const char c_szSerialSend             [] = "OVCtrl_SerialSend";
static const char c_szOpenMediaChannel       [] = "OVCtrl_OpenMediaChannel";
static const char c_szCloseMediaChannel      [] = "OVCtrl_CloseMediaChannel";
static const char c_szGetDeviceWorkstate     [] = "OVCtrl_GetDeviceWorkstate";
static const char c_szSelectByDNS            [] = "OVCtrl_SelectByDNS";
static const char c_szGetMediaBitrate        [] = "OVCtrl_GetMediaBitrate";


#endif//_CONTROLPLUG_H_

