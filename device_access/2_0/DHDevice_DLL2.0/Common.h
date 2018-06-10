// Common.h: interface for the CCommon class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMON_H__7C21641F_7142_4EE3_9BE5_07DA963CCEBF__INCLUDED_)
#define AFX_COMMON_H__7C21641F_7142_4EE3_9BE5_07DA963CCEBF__INCLUDED_

#ifdef _WIN32
#include <Windows.h>
#include <WinSock2.h>
#pragma comment (lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "toolsInsteadMFC.h"
#include "xt_config.h"
#endif//#ifdef _WIN32
#include "dhnetsdk.h"
#include "DHDvrDevice.h"


//////////////////////////////////////////////////////////////////////
// ϵͳ�궨��
#define MAX_LOGON_AMOUNT			4069	//����½�豸����
#define MAX_CHANNEL_AMOUNT			128 	//�������ͨ������
#define MAX_USERNAME_LENGTH 		32		//����û�������
#define MAX_PASSWORD_LENGTH 		64		//������볤��
#define MAX_BACKFILE_INFO_LENGTH	4096	//���ط��ļ���Ϣ����

#define REONLINE_WAIT_TIME			10		//�����ߵȴ�ʱ��
#define DATA_TIME_OUT				9		//�����ݳ�ʱ
#define REONLINE_TIMER				1		//����ͳ�Ƽ�ʱ����
#define RATE_CHECK_TIMER			2		//����ͳ�Ƽ�ʱ����


const BYTE DH_FileHead[8] = {0x44,0x41,0x48,0x55,0x41,0x11,0x04,0x19};
const int c_nBaudRate[10] = {300,600,1200,2400,4800,9600,19200,38400,57600,115200};
const int c_nDataBit[4] = {5, 6, 7, 8};

//typedef long (__stdcall *OV_PRealDataCallback)(long nLinkHandle, long nFrameType, unsigned char*	pDataBuf, long	nDataLength, long nDataType, void* objUser, long nTimeStam);

//////////////////////////////////////////////////////////////////////
/*�������ݴ���ص�ԭ��*/
typedef void(__stdcall *fAudioDataCallBack)(char *pstrIP, char *pDataBuf, DWORD dwBufSize, BYTE byAudioFlag);
/*��ͨ������Ϣ�ص�*/
typedef long(__stdcall *fCommAlarmCB)(char *pstrIP, long nDVRPort, long alarminputcount, BYTE *pAlarm);
//�ƶ���ⱨ����Ϣ�ص�
typedef long(__stdcall *fMotionDectionCB)(char *pstrIP, long nDVRPort, long nChannelCount, BYTE *pMotionDection);
//��Ƶ��ʧ������Ϣ�ص�
typedef long(__stdcall *fVideoLostCB)(char *pstrIP, long nDVRPort, long nChannelCount, BYTE *pVideoLost);
/*��Ƶ�ڵ���Ϣ�ص�*/
typedef long(__stdcall *fShelterAlarmCB)(char *pstrIP, long nDVRPort, BYTE *ShelterAlarm, long nArraySize);
/*Ӳ������Ϣ�ص�*/
typedef long(__stdcall *fDiskFullCB)(char *pstrIP, long nDVRPort, BYTE IsFull);
/*Ӳ�̹�����Ϣ�ص�*/
typedef long(__stdcall *fDiskErrorCB)(char *pstrIP, long nDVRPort, BYTE *DiskError, long nArraySize);
/*��Ƶ�����Ϣ�ص�*/
typedef long(__stdcall *fSoundDetectCB)(char *pstrIP, long nDVRPort, BYTE *SoundDetect, long nArraySize);
/*�豸���߻ص�*/
typedef void(__stdcall *fDisConnectCB)(char *pstIP, long nDVRPort);
//ͼ����ӻص�
typedef void(__stdcall *fDrawWndCB)(char *pstrIP, long nChID, HDC hDC,DWORD dwUser);
//�»طŽ�����Ϣ
typedef void(__stdcall *DHPlayBackPos)(char *pstrIP, long nChID, long hWnd, double dbPos, DWORD dwUser);
//�����ؽ�����Ϣ
typedef void(__stdcall *DHDownloadPos)(char *pstrIP, long nChID, DWORD dwTotalSize, DWORD dwDownLoadSize, double dbPos, DWORD dwUser);
//������Ϣ�ص�
//nDiskNum��������, nDiskID ������ţ�nFullSpace ������nFreeSpace ʣ��������nDiskState ״̬0-����,1-�,2-���ϵ�
typedef void(__stdcall *fDHDiskInfo)(char *szIP, long nDiskNum, long nDiskID, long nFullSpace, long nFreeSpace, long nDiskState);

typedef long (__stdcall *pOV_RealDataCallback)(DWORD nDeviceType, DWORD nLinkID,
                                               BYTE* pDataBuf, DWORD nDataLength, DWORD nImageType, DWORD nWidth, DWORD nHeight,
                                               long nFrameType, void* UserContext);

/*��ȡ�û�Ȩ���б�*/
typedef void (__stdcall *fUserRightCB)(
                                       char *pstrIP,                    //DVR IP��ַ
                                       long nRightID,                   //Ȩ�ޱ��
                                       char *pstrRightName,				//Ȩ������,32�ֽ�
                                       char *pstrRightMemo 				//Ȩ�ޱ�ע,32�ַ�
                                       );
/*��ȡ�û����б�*/
typedef void (__stdcall *fUserGroupCB)(
                                       char *pstrIP,					//DVR IP��ַ
                                       long nGroupID,					//�û�����
                                       char *pstrGroupName,				//�û�������
                                       long nRightNum, 					//Ȩ������
                                       long *pnRightList,				//Ȩ�ޱ������,���100����
                                       char *pstrGroupMemo 				//�û��鱸ע
                                       );
/*��ȡ�û��б�*/
typedef void (__stdcall *fUserInfoCB)(
                                      char *pstrIP,						//DVR IP��ַ
                                      long nUserID,						//�û����
                                      long nGroupID,					//�û�����
                                      char *pstrUserName, 				//�û���
                                      char *pstrPassword, 				//����(����)
                                      long nRightNum, 					//Ȩ������
                                      long *pnRightList,				//Ȩ�ޱ������,���100����
                                      char *pstrUserMemo				//�û���ע
                                      );

//////////////////////////////////////////////////////////////////////
// ϵͳ�ṹ����

//���ڻ�������
struct SComCfg
{
    BYTE nDataBit;
    BYTE nStopBit;
    BYTE nParity;
    BYTE nBaudRate;
};

//��½�豸��Ϣ
struct SDeviceInfo_DH
{
    DWORD m_dwIP;							//	�豸IP��ַ		(Ĭ��0	�յ�ַ)
    WORD m_nID; 							//	�豸ID���		(Ĭ��-1 ���豸)
    WORD m_wPost;							//	�豸Post�˿ں�	(Ĭ��0	�ն˿�)
    char m_strUserName[MAX_USERNAME_LENGTH];//	��½�û���		(Ĭ��NULL)
    char m_strPassWord[MAX_PASSWORD_LENGTH];//	��½����		(Ĭ��NULL)
    long m_nHandle; 						//	�豸���		(Ĭ��ֵ0,��)
    long m_nDevVer;                         //  �豸�汾������ѡ�����API�汾
	long lLoginHandle;						//	��¼��� add pan 8/11
    
    //�����Խ����
    long m_nAudioHandle;					//	�����Խ����	(Ĭ��ֵ0,��)
    void* m_hARecLock[2];                  //  �Խ�¼����      ()
    void* m_hSpeakRec[2];                  //  �Խ�¼�����    (Ĭ��ֵ0,��)
    
    //��������
    void* m_objSerial;
    long m_hSerialTran[2];                  //  ͸�����        (0-232, 1-485)
    long m_nSerial485;                      //  ��ǰʹ�õ�ͨ����(Ĭ��ֵ0)
    SComCfg m_cfg232[DH_MAX_232_NUM];
    SComCfg m_cfg485[DH_MAX_CHANNUM];
};

/*����ͨ����Ϣ*/
struct SChannelInfo_DH
{
    long m_nID; 							//	�豸���		(Ĭ��ֵ-1,����)
    DWORD m_dwIP;							//	�豸IP��ַ		(Ĭ��ֵ0,�յ�ַ)
    short m_nChannelType;					//	����ͨ��״̬	(Ĭ��ֵ0,�ޣ�1��ʱ���ݣ�2�ط����ݣ�3�ط�����)
    short m_nMediaType;                     //  �������        (Ĭ��ֵ0,������)
    bool m_aPlayState[2];					//	����״̬		(0���ݲ���״̬,1���ݱ���״̬)
    long m_nHandle; 						//	���ž��		(Ĭ��ֵ0,��)
    HWND m_hWnd;							//	��ʾ���		(Ĭ��ֵNULL)
    long m_nData;							//	�û�����		(Ĭ��ֵ-1)
	pOV_RealDataCallback m_pOVCallback; 	//	OV��׼�������	(Ĭ��ֵNULL)	
	OV_PRealDataCallback m_pNewCallBack;
    NET_RECORDFILE_INFO* m_pRPFile; 		//	�ط��ļ���Ϣ	(Ĭ��ֵNULL)
    DWORD m_nDataRate;						//	����ͳ��
    long m_nOutTime;						//	��ʱͳ��
    char* m_szSavePath;
};

struct SReOnlineInfo
{
    char szIP[16];
    long nWaitTime; 	//����ʱ(��)
};

struct SReLinkInfo
{
    DWORD nIPv4;    //�豸IP
    long nChID;     //ͼ��ͨ��
};

//ʱ���ʽת��
void BitSet(DWORD &nInput, long nBitCount, BYTE bFlag);
void FixComCfgStruct(SComCfg& rOutput, const DH_COMM_PROP& rInput);
void FixComCfgStruct(DH_COMM_PROP& rOutput, const SComCfg& rInput);

DH_RealPlayType FixMediaType(int nMediaType);

//�ṹ��ʼ��
void StructInit(SDeviceInfo_DH& Struct);
void StructInit(SChannelInfo_DH& Struct);
void StructInit(SReOnlineInfo& SInput);
void StructInit(SReLinkInfo& SInput);

void WriteLog(const char* szFlag, const char* szLogFmt, ...);
void EnableLog(bool bEnable);

#endif // !defined(AFX_COMMON_H__7C21641F_7142_4EE3_9BE5_07DA963CCEBF__INCLUDED_)
