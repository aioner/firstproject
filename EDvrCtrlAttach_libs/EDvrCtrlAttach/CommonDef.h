#ifndef _COMMON_H__INCLUDE_
#define _COMMON_H__INCLUDE_

#ifndef _WIN32
#include "xt_config.h"
#endif //#ifndef _WIN32

#include "ControlPlug.h"
#include "ThreadPulse.h"

#define MAX_IDS_LENGTH 24
#define MAX_DEVICE_TYPE 1024
#define MAX_ADDRESS_LENGTH 32
#define MAX_USERNAME_LENGTH 16
#define MAX_PASSWORD_LENGTH 32
#define MAX_TYPENAME_LENGTH 64

extern const char c_szCfg[];
extern const char c_szFlag[];
extern const char c_szLogPath[];
extern const char c_szAppName[];
extern HMODULE g_hModule;


//������������(0~255)
enum ECtrlEffect
{
    eCtrlBrightness = 1,
    eCtrlContrast = 2,
    eCtrlHue = 3,
    eCtrlSaturation = 4,
    eCtrlDefault = 6,
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

enum EReflashState
{
    eReflashStateStart = 0,
    eReflashStateDoing = 1,
    eReflashStateFinish = 2,
};

//�ź�����¼�����
typedef void (__stdcall* dgOnOnlineEvent)(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType, int nOnline, int nRes, LPCSTR szRes, void *objUser);
typedef void (__stdcall* dgOnAlarmStateEvent)(LPCSTR szIDS, LPCSTR szStateList, int nStateSize, int nAlarmType, LPCSTR szRes, void *objUser);
typedef void (__stdcall* dgOnSerialEvent)(LPCSTR szIDS, int nSerialPort, void* pBufData, int nBufSize, void *objUser);
typedef void (__stdcall* dgOnDeviceIPChange)(LPCSTR szSerialNum, LPCSTR szAddress, int nDeviceType, void *objUser);
typedef void (__stdcall* dgOnDvrStateInfo)(LPCSTR szIDS, LPCSTR szAddress, LPCSTR xmlStateInfo, int nInfoSize, void *objUser);
typedef void (__stdcall* dgOnDvrStateChanged)(LPCSTR szIDS, LPCSTR szAddress, int eStateEvent,\
                                             int nChannel, int nEventValue, void* pEventData, int nDataSize, void *objUser);

//�豸����
typedef struct _SDeviceType
{
    int m_nDeviceType;                      //�豸���ͺţ�-1��ѯ��ʼ��-2��ѯ������>=0��Ӧ�豸����
    int m_nControlPort;                     //�豸���ƶ˿�
    char m_szTypeName[32];                  //�豸��������
    char m_szUser[MAX_USERNAME_LENGTH];     //��¼�û���
    char m_szPassword[MAX_PASSWORD_LENGTH]; //��¼����
}SDeviceType;
typedef int(CALLBACK *dgDeviceType)(int nCount, SDeviceType *pDeviceType, void *objUser);

//�ײ����������Ϣ
typedef struct _SOV_CtrlPlug
{
    SDeviceType m_sType;
    int m_bAutoOnline;                      //�豸֧���Զ�����
    char m_szLibPath[MAX_PATH];             //ģ��·��

    SOV_CtrlAPI m_PlugAPI;
	int m_nOnlineDelay;						//������ʱ
}SOV_CtrlPlug;

//�豸����״̬
typedef struct _SDeviceState
{
    //����״̬(����)
    char nInputDispose[MAX_ALARM_INPUT];
    char nOutputDispose[MAX_ALARM_OUTPUT];
    char nMotionDispose[MAX_CAMERA_NUM];
    char nLostDispose[MAX_CAMERA_NUM];
	char nBlindDispose[MAX_CAMERA_NUM];
	char nRecordDispose[MAX_CAMERA_NUM];

    //��Ӧ״̬(�澯)
    int nOnlineState;                       //����״̬��0-���ߣ�1-���ߣ�2-�ȴ�
    int nHardwareAlarm;                     //Ӳ��״̬��0-������1-����Դռ�ã�2-Ӳ������
    char bInputAlarm[MAX_ALARM_INPUT];
    char bDiskFull[MAX_HARDDISK_NUM];
    char bVideoLost[MAX_CAMERA_NUM];
    char bMotionAlarm[MAX_CAMERA_NUM];
    char bDiskError[MAX_HARDDISK_NUM];
    char bVideoBlind[MAX_CAMERA_NUM];
    char bDiskState[MAX_HARDDISK_NUM];
    int nPrimaryRate[MAX_CAMERA_NUM];       //ͨ������������ ��λKB
    int nSecondaryRate[MAX_CAMERA_NUM];     //ͨ������������ ��λKB
    unsigned nDiskSize[MAX_HARDDISK_NUM];   //�������� ��λKB
    unsigned nDiskSpace[MAX_HARDDISK_NUM];  //���̿ռ� ��λKB
}SDeviceState;

//�豸��Ϣ
typedef struct _SDeviceCtrlInfo
{
    int m_nDeviceID;
    LOGINTYPE m_nLoginHandle;               // server login control handle (default -1, free)
    //short m_bAutoOnline;                  // is auto online       (default 0, cust)
    int m_nTypeID;                          // the device type id mapping to control plugs (default -1)
    unsigned short m_nSubType;              // server subtype       (default 0)
    unsigned short m_nCtrlPort;	            // the control port     (default 0)
    char m_szIDS[MAX_IDS_LENGTH];           // IDS by XingTu system
    char m_szAddress[MAX_ADDRESS_LENGTH];	// server address    (default string_empty)
    char m_szUser[MAX_USERNAME_LENGTH];	    // user name    (default string_empty)
    char m_szPass[MAX_PASSWORD_LENGTH];	    // password     (default string_empty)

    SDeviceAttribute m_sAttribs;            // server device attribs(default 0)
    SDeviceState m_sWorkState;
}SDeviceCtrlInfo;

typedef struct _SEDvrCtrlEvent
{
    int nSendStateTime; //���Ͷ�ʱ״̬��ʱ��(��)��Ĭ��0 ������
    int nQuestTimer;
    int nSendTimer;
    char *szSendBuffer;
    char szDDnsAddress[MAX_ADDRESS_LENGTH];

    dgOnOnlineEvent fpOnlineEvent;
    void *objOnline;
    dgOnAlarmStateEvent fpAlarmStateEvent;
    void *objAlarm;
    dgOnSerialEvent fpSerialEvent;
    void *objSerial;
    dgOnDeviceIPChange fpIPChange;
    void *objIPChange;
    dgOnDvrStateInfo fpDvrStateInfo;
    void *objStateInfo;
    dgOnDvrStateChanged fpDvrStateChanged;
    void *objStateChange;
    SThreadPulse thrCheckOnline;
    SThreadPulse thrGetWorkState;
    SThreadPulse thrSendWorkState;
    SThreadPulse thrAlarmFilter;
    SThreadPulse thrDDnsCheck;
	SThreadPulse thrCheckOffline;
}SEDvrCtrlEvent;

typedef struct _SAlarmTimer
{
    int m_nDeviceID;    //�豸���
    int m_nTimer;       //ʱ�����
    short m_nMarkTime;  //ʱ���׼
    short m_nAlarmType; //��������
    int m_nChannel;     //����ͨ��
}SAlarmTimer;

typedef struct _SReloginDevice
{
    int m_nDeviceID;    //�豸���
    int m_nTimer;       //ʱ�����
    int m_nMarkTime;    //ʱ���׼
    int m_nState;
}SReloginDevice;

typedef struct _SGetAlarmState
{
#ifdef _WIN32
	HANDLE hThread;
#else
	pthread_t hThread;
#endif
    DWORD nThreadID;
    SDeviceCtrlInfo *pDevice;
    int nAlarmType[64];
    int nTypeLength;
}SGetAlarmState;

typedef struct _SDDnsDeviceInfo
{
    int nDDnsTypeID;
    int nDeviceType;
    char szSerialID[MAX_PATH];
    char szDeviceAddress[MAX_ADDRESS_LENGTH];
}SDDnsDeviceInfo;

int InitCtrlPlug(SOV_CtrlPlug *pInput);
int InitDeviceInfo(SDeviceCtrlInfo *pInput);
int InitCtrlEvent(SEDvrCtrlEvent *pInput);
int InitAlarmTimer(SAlarmTimer *pInput);
int InitRelogin(SReloginDevice *pInput);
int InitDDnsDevice(SDDnsDeviceInfo *pInput);

int MakeBoolArrayString(char *szBuffer, int nBufferSize, BYTE *pBoolArray, int nArraySize);
int MakeUInt32ArrayString(char *szBuffer, int nBufferSize, UINT32 *pUint32Array, int nArraySize);

void AddParamInt(char *strXml, const char* ParamName, int ParamValue);
void AddParamString(char *strXml, const char* ParamName, const char* ParamValue);

unsigned ConvertVersion(char *szVersion);

#endif//_COMMON_H__INCLUDE_
