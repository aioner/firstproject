// Device_DH.h: longerface for the
// SDeviceInfo struct
// SChannelInfo struct
// CDevice_DH class
// CDeviceList class
// CChannelList class
// ʹ��ʱ��Ҫ���뾲̬��<ws2_32.lib>
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVICE_DH_H__8CB9885D_F5AD_4813_8C61_6C6016578886__INCLUDED_)
#define AFX_DEVICE_DH_H__8CB9885D_F5AD_4813_8C61_6C6016578886__INCLUDED_

#include "InfoMrg.h"

//////////////////////////////////////////////////////////////////////
// ϵͳ�ඨ��
class CDevice_DH  
{
public:
    CDeviceList_DH m_DeviceList;			//�����豸�б�
    CChannelList_DH m_ChannelList;			//����ͨ���б�
    CBackFileList_DH* m_BackFileList;		//�ط�¼���ļ���Ϣ�б�
    bool m_bNoInit; 						//ϵͳ��״̬(Ĭ��:true)
    fAudioDataCallBack m_pFunAudioCB;		//�������ݻص�����ָ��
    fCommAlarmCB m_pFunCommAlarmCB; 		//��ͨ������Ϣ�ص�ָ��
    fMotionDectionCB m_pFuncMotionDectionCB;//�ƶ������Ϣ�ص�ָ��
    fVideoLostCB m_pFuncVideoLostCB;		//��Ƶ��ʧ��Ϣ�ص�ָ��
    fShelterAlarmCB m_pFunShelterAlarmCB;	//��Ƶ�ڵ���Ϣ�ص�ָ��
    fDiskFullCB m_pFunDiskFullCB;			//Ӳ������Ϣ�ص�ָ��
    fDiskErrorCB m_pFunDiskErrorCB; 		//Ӳ�̴�����Ϣ�ص�ָ��
    fSoundDetectCB m_pFunSoundDetectCB; 	//��Ƶ�����Ϣ�ص�ָ��
    fDisConnectCB m_pFunDisConnectCB;		//���߻ص�ָ��
    fDrawWndCB m_pDrawWndCB;				//�ַ����ӻص�ָ��
    
    DHPlayBackPos m_pPlayBackPos;
    DWORD m_nPBUser;
    DHDownloadPos m_pDownloadPos;
    DWORD m_nDLUser;
    
    static DHDEV_CHANNEL_CFG m_ChannelCfg[DH_MAX_CHANNUM];
    static DHDEV_ALARM_SCHEDULE m_AlarmCfg;
    static CDevice_DH* m_pThis;
    static char g_szFlag[];
    
    void* m_hRelinkEvent;
    vector<SReOnlineInfo> m_lstOnline;
    vector<SReLinkInfo> m_lstRelink;
    
protected:
    /****************************************************************/
    /*�ڲ��ӿ� �� �ص�����											*/
    /****************************************************************/
    /*�豸�Ͽ�ʱ�ص�������������������Ͽ����豸�б�ĸ��¼��豸��ɾ������*/
    static void CALLBACK DisConnectFunc(long lLoginID, char *pchDVRIP, long nDVRPort, DWORD dwUser);
    /*�豸����ʱ�ص�������������������Ͽ����豸�б�ĸ��¼��豸��ɾ������*/
    static void CALLBACK ReOnlineFunc(long lLoginID, char *pchDVRIP, long nDVRPort, DWORD dwUser);
      /*��Ϣ�ص�������,�Ƕ�����sdkӦ�õĻص�*/
    static BOOL CALLBACK MessCallBack(long lCommand, long lLoginID, char *pBuf, DWORD dwBufLen, char *pchDVRIP, LONG nDVRPort, DWORD dwUser);

    static bool CALLBACK MessCallBackV26(long lCommand, long lLoginID, char *pBuf, DWORD dwBufLen,
                                       char *pchDVRIP, long nDVRPort, DWORD dwUser);
    /*���ݻص�������*/
    static void CALLBACK DataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize,LONG param, DWORD dwUser);
    /*�Խ���Ƶ�ص�������*/
    static void CALLBACK AudioCallBack(long nTalkHandle, char *pDataBuf, DWORD dwBufSize, BYTE byAudioFlag,
        DWORD dwUser);
    //�ַ����ӻص�
    static void CALLBACK DrawWndCallBack(long lLoginID, long lRealHandle, HDC hDC, DWORD dwUser);
    //Զ���ļ����š����ؽ���POS�ص�
    static void CALLBACK FileDownloadPosCallBack(long lPlayBackRet, DWORD dwTotalSize, DWORD dwDownLoadSize, DWORD dwUser);
    
    /*ָֹͣ���豸������������*/
    void EDvr_StopDeviceDataLink(char* strIP);	
    //�豸�صǲ���
    long EDvr_ReLogin(long nLoginID);
    //�豸������������
    long EDvr_ReLink(long nLoginID);
    long EDvr_ReLink(DWORD nDevIPv4, long nChID);
    
public:
    CDevice_DH();
    virtual ~CDevice_DH();
    
    //////////////////////////////////////////////////////////////////////////
    // ��ʼ���������˳�
    //////////////////////////////////////////////////////////////////////////
    /*��ʼ���ͻ���*/
    long EDvr_Init();//ʧ�ܷ���-1���ɹ�����0
    
    /*ֹͣ��ǰ�����в������˳��ͷ�������Դ*/
    void EDvr_Exit();
    
    //�豸���߻ص�����
    void EDvr_SetDisConnectCallBackFunc(fDisConnectCB lpFuncCB);
    
    //////////////////////////////////////////////////////////////////////////
    // DVR�豸��½��ע��
    //////////////////////////////////////////////////////////////////////////
    /*��¼��ָ�����豸��*/
    long EDvr_Login(
        char*pstrIP,		//DVR IP��ַ
        long nPort, 		//DVR �˿ں�
        char*pstrLogName,	//DVR ��½�˺�
        char *pstrLogPass,	//DVR ��½����
        bool bCheckUser = FALSE
        );//ʧ�ܷ���-1���ɹ������豸��ţ����Ժ�����Ĳ���

    //�����澯����
    long EDvr_Listen(SDeviceInfo_DH &rDevice);
    
    long EDvr_ReLogin(
        char*pstrIP 	//DVR IP��ַ
        );
    
    /*��ָ�����豸��ע��*/
    void EDvr_LogOut(
        char *pstrIP		//DVR IP��ַ
        );

	void EDvr_LogOut(
		long lLoginHandle	//dvr��¼���
		);
    
    /*�������ѵ�¼���豸��ע��*/
    void EDvr_LogOutAll();
    
    /*Զ�������豸*/
    void EDvr_Reboot(
        char *pstrIP		//DVR IP��ַ
        );
    
    //////////////////////////////////////////////////////////////////////////
    // �豸ͨ����ʱ��������
    //////////////////////////////////////////////////////////////////////////
    /*������ָ���豸��ָ��ͨ������������*/
    long EDvr_StartDataLink(
        char*pstrIP,		//DVR IP��ַ
        long nChID, 		//DVR �ɼ��ն�ͨ��
        long nLinkType, 	//���ӷ�ʽ TCP:0��UDP:1
        long nMediaType,    //������� 0-¼������1~
        HWND hWin			//���Ŵ������������
        );//ʧ�ܷ���-1���ɹ���������ͨ���ţ����Ժ�����Ĳ���
    
    /*ֹͣ��ָ���豸��ָ��ͨ������������*/
    void EDvr_StopDataLink(
        long nRDataLink 	//EDvr_StartDataLink ����ֵ
        );
    void EDvr_StopDataLink(
        char *pstrIP,		//DVR IP��ַ
        long nChID			//DVR �ɼ��ն�ͨ��
        );
    
    /*ֹͣ��ǰ��������������*/
    void EDvr_StopAllDataLink();
    
    /*��ȡ�ɼ�ͨ����Ӧ�������*/
    long EDvr_GetChannelHandle(char *pstrIP, long nChID);
    
    //////////////////////////////////////////////////////////////////////////
    // �豸ͨ����ʱ���ݲ���
    //////////////////////////////////////////////////////////////////////////
    
    /*����ָ���豸��ָ��ͨ�����������ӵ����ݲ���*/
    long EDvr_StartDataCapture(
        long nRDataLink 	//EDvr_StartDataLink ����ֵ
        );//ʧ�ܷ���-1���ɹ�����0
    long EDvr_StartDataCapture(
        char *pstrIP,		//DVR IP��ַ
        long nChID			//DVR �ɼ��ն�ͨ��
        );//ʧ�ܷ���-1���ɹ�����0
    
    /*ָֹͣ���豸��ָ��ͨ�����������ӵ����ݲ���*/
    void EDvr_StopDataCapture(
        long nRDataLink 	//EDvr_StartDataLink ����ֵ
        );
    void EDvr_StopDataCapture(
        char *pstrIP,		//DVR IP��ַ
        long nChID
        );
    
    //////////////////////////////////////////////////////////////////////////
    // �ɼ�¼��/��ͼ����
    //////////////////////////////////////////////////////////////////////////
    /*��ʼָ���豸ָ��ͨ�����������ӵ����ݱ����ļ�(¼��)*/
    long EDvr_StartDataRecorder(
        char *pstrIP,		//DVR IP��ַ
        long nChID, 		//DVR �ɼ��ն�ͨ��
        char *pstrFileName	//����¼���ļ���
        );//ʧ�ܷ���-1���ɹ�����0
    long EDvr_StartDataRecorder(
        long nRDataLink,	//EDvr_StartDataLink ����ֵ
        char *pstrFileName	//����¼���ļ���
        );//ʧ�ܷ���-1���ɹ�����0
    
    /*ָֹͣ���豸ָ��ͨ�����������ӵ����ݱ����ļ�(¼��)*/
    void EDvr_StopDataRecorder(
        char *pstrIP,		//DVR IP��ַ
        long nChID			//DVR �ɼ��ն�ͨ��
        );
    void EDvr_StopDataRecorder(
        long nRDataLink 	//EDvr_StartDataLink ����ֵ
        );
    
    /*��ָ���豸ָ��ͨ��������������н�ͼ*/
    long EDvr_ConvertPicture(
        char *pstrIP,		//DVR IP��ַ
        long nChID, 		//DVR �ɼ��ն�ͨ��
        char *pstrFileName	//����¼���ļ���
        );//ʧ�ܷ���-1���ɹ�����0
    long EDvr_ConvertPicture(
        long nRDataLink,	//EDvr_StartDataLink ����ֵ
        char *pstrFileName	//����¼���ļ���
        );//ʧ�ܷ���-1���ɹ�����0
    
    //////////////////////////////////////////////////////////////////////////
    // ��̨�������
    //////////////////////////////////////////////////////////////////////////
    /*��̨����*/
    long EDvr_PTZControl(
        char *pstrIP,		//DVR IP��ַ
        long nChID, 		//DVR �ɼ��ն�ͨ��
        long nPTZCommand,	//��̨��������
        bool bStop, 		//��ʼ/ֹͣ�˶�
        long nStep			//����/�ٶ� 1-8
        );//ʧ�ܷ���-1���ɹ�����0
    
    /*��̨������չ*/
    long EDvr_PTZControlEx(
        char *pstrIP,		//DVR IP��ַ
        long nChID, 		//DVR �ɼ��ն�ͨ��
        long nPTZCommandEx, //��̨��������
        bool bStop, 		//��ʼ/ֹͣ�˶�
        unsigned char param1, unsigned char param2, unsigned char param3
        );//ʧ�ܷ���-1���ɹ�����0
    
    //////////////////////////////////////////////////////////////////////////
    // �豸¼�����
    //////////////////////////////////////////////////////////////////////////
    
    /*��ȡԶ��¼������*/
    long EDvr_GetRecordBackState(
        char *pstrIP,		//DVR IP��ַ
        char *pRState,		//¼��״̬���飺0��¼��1�ֶ�¼��2�Զ�¼��
        long nMaxLen,		//״̬���鳤�����ֵ������С��ͨ����
        long *pnStateLen	//״̬���鳤�ȣ�ÿ��ͨ��һ���ֽ�
        );//ʧ�ܷ���-1���ɹ�����0
    /*����Զ��¼������*/
    long EDvr_SetRecordBackState(
        char *pstrIP,		//DVR IP��ַ
        char *pRState,		//¼��״̬���飺0��¼��1�ֶ�¼��2�Զ�¼��
        long nStateLen		//״̬���鳤�ȣ�ÿ��ͨ��һ���ֽ�
        );//ʧ�ܷ���-1���ɹ�����0
    
    /*�رղ�ѯ���*/
    void EDvr_FindClose(
        long lFileFindHandle
        );
    /*������һ����¼*/
    long EDvr_FindNextFile(
        long lFileFindHandle
        );
    
    /*��ʼ����¼���ļ�*/
    long EDvr_StartPlayBack(
        char *pstrIP,		//DVR IP��ַ
        long nFileHandle,	//�����ļ��������
        HWND hWnd			//���Ŵ�����
        );
    long EDvr_StartPlayBack(
        char *pstrIP,		//DVR IP��ַ
        long nChID, 		//DVR �ɼ��ն�ͨ��
        char *pstrFileName, //¼���ļ���
        HWND hWnd			//���Ŵ�����
        );
    
    /*ֹͣ����¼���ļ�*/
    void EDvr_StopPlayBack(
        char *pstrIP,		//DVR IP��ַ
        long nChID			//DVR �ɼ��ն�ͨ��
        );
    void EDvr_StopPlayBack(
        long nBDataLink 	//�ļ����Ų������
        );
    void EDvr_StopPlayBack(HWND hWnd);
    
    /*��ʼ����¼���ļ�*/
    long EDvr_StartFileDownload(
        char *pstrIP,		//DVR IP��ַ
        long nFileHandle,	//�����ļ��������
        char *sSavedFileName
        );
    long EDvr_StartFileDownload(
        char *pstrIP,		//DVR IP��ַ
        long nChID, 		//DVR �ɼ��ն�ͨ��
        char *pstrFileName, //¼���ļ���
        char *sSavedFileName//�����ļ���
        );
    
    /*ֹͣ����¼���ļ�*/
    void EDvr_StopFileDownload(
        long nFileHandle	//�����ļ��������
        );
    //ֹͣ�����ļ�
    // long nChID ͨ����
    // char *pstrFileName �������ļ���
    void EDvr_StopFileDownload(long nChID, char *pstrFileName);
    
    /*�������¼��ĵ�ǰλ��*/
    long EDvr_GetDownloadPos(
        long nDownHandle,	//�ļ����ؾ��
        long *nTotalSize,	//�������ؽ���
        long *nDownLoadSize //�����ؽ���
        );
    
    //////////////////////////////////////////////////////////////////////////
    // �����Խ�����
    //////////////////////////////////////////////////////////////////////////
    /*���������ص�����*/
    void EDvr_SetAudioDataCallBackFunc(fAudioDataCallBack CallBackFunc);
    
    /*��������*/
    long EDvr_OpenSound(char *pstrIP, long nChID);
    void EDvr_CloseSound();
    
    //�����������ݲ���
    long EDvr_StartAudioDataCapture();
    
    //ֹͣ�������ݲ���
    void EDvr_StopAudioDataCapture();
    
    /*�����������豸*/
    long EDvr_AudioDataSend(char *pstrIP, char *pSendBuff, long nBuffSize);
    
    /*������豸�õ�����������*/
    void EDvr_AudioDecode(char *pSendBuff, long nBuffSize);
    
    /*��������*/
    long EDvr_SetAudioVolume(char *pstrIP, unsigned short wVolume);
    
    //////////////////////////////////////////////////////////////////////////
    // ������Ϣ�ص�����
    //////////////////////////////////////////////////////////////////////////
    
    /*������ͨ������Ϣ�ص�*/
    void EDvr_SetCommAlarmCallBackFunc(fCommAlarmCB CallBackFunc);
    
    //�����ƶ���ⱨ���ص�
    void EDvr_SetMotionDectionCallBackFunc(fMotionDectionCB CallBackFunc);
    
    //������Ƶ��ʧ�����ص�
    void EDvr_SetVideoLostCallBackFunc(fVideoLostCB CallBackFunc);
    
    /*������Ƶ�ڵ���Ϣ�ص�*/
    void EDvr_SetShelterAlarmCallBackFunc(fShelterAlarmCB CallBackFunc);
    
    /*����Ӳ������Ϣ�ص�*/
    void EDvr_SetDiskFullCallBackFunc(fDiskFullCB CallBackFunc);
    
    /*����Ӳ�̴�����Ϣ�ص�*/
    void EDvr_SetDiskErrorCallBackFunc(fDiskErrorCB CallBackFunc);
    
    /*������Ƶ�����Ϣ�ص�*/
    void EDvr_SetSoundDetectCallBackFunc(fSoundDetectCB CallBackFunc);
    
    
    //////////////////////////////////////////////////////////////////////////
    // ��������
    //////////////////////////////////////////////////////////////////////////
    
    /*������������*/
    long EDvr_SetNormalAlarmCFG(
        char *pstrIP,						//DVR IP��ַ
        long nAlarmKind,					//��������:0-��������,1-�ƶ����,2-��Ƶ��ʧ,3-��Ƶ�ڵ�,4-���̱���
        long nAlarmChannel, 				//��������-��������ţ��ƶ����.��Ƶ��ʧ.��Ƶ�ڵ�-��Ƶͨ����ţ����̱���--1-�޴���,2-������,3-���̹���
        long nAlarmInfo,					//��������-0����,1���գ��ƶ����.��Ƶ��ʧ.��Ƶ�ڵ�-������(1~6)�����̱���--������,ʹ��ʱ��0
        long nAlarmEnable					//����״̬:0-����,1-����
        );									//����ֵ:�ɹ�0��ʧ��-1
    
    /*��ȡ�ⲿ��������״̬*/
    long EDvr_GetMsgAlarmInEnable(
        char *pstrIP,						//DVR IP��ַ
        bool *pIsEnable,					//����״̬���飬true������false����
        long *pAlarmInCount 				//���鳤��
        );
    /*�����ⲿ��������״̬*/
    long EDvr_SetMsgAlarmInEnable(
        char *pstrIP,						//DVR IP��ַ
        const bool *pIsEnable,				//����״̬���飬true������false����
        long nAlarmInCount = DH_MAX_ALARM_IN_NUM
        );
    
    /*��ȡ�������״̬*/
    long EDvr_GetMsgAlarmOutEnable(
        char *pstrIP,						//DVR IP��ַ
        bool *pIsEnable,					//����״̬���飬true������false����
        long *pAlarmOutCount				//���鳤��
        );
    /*���ñ������״̬*/
    long EDvr_SetMsgAlarmOutEnable(
        char *pstrIP,						//DVR IP��ַ
        const bool *pIsEnable,				//����״̬���飬true������false����
        long nAlarmOutCount = DH_MAX_ALARMOUT_NUM
        );
    
    /*��ȡ��̬��ⱨ������״̬*/
    long EDvr_GetMsgMotionDectionEnable(
        char *pstrIP,						//DVR IP��ַ
        bool *pIsEnable,					//����״̬���飬true������false����
        long *pVideoCount
        );
    /*���ö�̬��ⱨ������״̬*/
    long EDvr_SetMsgMotionDectionEnable(
        char *pstrIP,						//DVR IP��ַ
        const bool *pIsEnable,				//����״̬���飬true������false����
        long nVideoCount = DH_MAX_VIDEO_IN_NUM
        );
    
    /*��ȡ��Ƶ��ʧ����״̬*/
    long EDvr_GetMsgVedioLostEnable(
        char *pstrIP,						//DVR IP��ַ
        bool *pIsEnable,					//����״̬���飬true������false����
        long *pVideoCount
        );
    /*������Ƶ��ʧ����״̬*/
    long EDvr_SetMsgVedioLostEnable(
        char *pstrIP,						//DVR IP��ַ
        const bool *pIsEnable,				//����״̬���飬true������false����
        long nVideoCount = DH_MAX_VIDEO_IN_NUM
        );
    
    /*��ȡͼ���ڵ�����״̬*/
    long EDvr_GetMsgBlindEnable(
        char *pstrIP,						//DVR IP��ַ
        bool *pIsEnable,				//����״̬���飬true������false����
        long *pVideoCount
        );
    /*����ͼ���ڵ�����״̬*/
    long EDvr_SetMsgBlindEnable(
        char *pstrIP,						//DVR IP��ַ
        const bool *pIsEnable,				//����״̬���飬true������false����
        long nVideoCount = DH_MAX_VIDEO_IN_NUM
        );
    
    /*��ȡӲ����Ϣ����״̬*/
    long EDvr_GetMsgHardDiskEnable(
        char *pstrIP,						//DVR IP��ַ
        long nConfigType,					//�������ͣ�0��Ӳ�̱�����1Ӳ���������㱨����2Ӳ�̹��ϱ���
        bool *pIsEnable 					//����״̬��true������false����
        );
    /*����Ӳ����Ϣ����״̬*/
    long EDvr_SetMsgHardDiskEnable(
        char *pstrIP,						//DVR IP��ַ
        long nConfigType,					//�������ͣ�0��Ӳ�̱�����1Ӳ���������㱨����2Ӳ�̹��ϱ���
        bool bIsEnable						//����״̬��true������false����
        );
    
    //////////////////////////////////////////////////////////////////////////
    // ƴ����������
    
    long EDvr_StartMultiLink(
        char*pstrIP,						//DVR IP��ַ
        long nLinkType, 					//���ӷ�ʽ TCP:0��UDP:1
        HWND hWin							//���Ŵ������������
        );
    void EDvr_StopMultiLink(char *pstrIP);
    void EDvr_StopMultiLink(long nRDataLink);
    
    //////////////////////////////////////////////////////////////////////////
    // ��Ƶͨ������
    
    /*��ȡͨ������*/
    long EDvr_GetVideoEffect(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        BYTE *nBright,						//����(0-100)
        BYTE *nContrast,					//�Աȶ�(0-100)
        BYTE *nSaturation,					//���Ͷ�(0-100)
        BYTE *nHue, 						//ɫ��(0-100)
        bool *bGainEnable,					//���濪��
        BYTE *nGain 						//����(0-100)
        );
    /*����ͨ������*/
    long EDvr_SetVideoEffect(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        BYTE nBright,						//����(0-100)
        BYTE nContrast, 					//�Աȶ�(0-100)
        BYTE nSaturation,					//���Ͷ�(0-100)
        BYTE nHue,							//ɫ��(0-100)
        bool bGainEnable,					//���濪��
        BYTE nGain							//����(0-100)
        );
    /*��Ƶ��������*/
    long EDvr_SetImageEffect(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long nEffecType,					//����ѡ��:0-����,1-�Աȶ�,2-���Ͷ�,3-ɫ��,4-����
        long nEffectValue					//ѡ�����ֵ(0~100)
        );
    
    /*��ȡͨ����Ƶ����*/
    long EDvr_GetChannelVideoInfo(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
                                            long nEncOpt,						/*ͨ��ѡ��(1-Rec_Nomarl;2-Rec_Motion;3-RecAlarm;
                                            4-Assistant_1;5-Assistant_2;6-Assistant_3)*/
                                            char *pstrChannelName,				//ͨ����
                                            bool *bVideoEnable, 				//ͼ��ʹ��
                                            BYTE *nBitRate, 					//����(����)����(1-BRC_CBR;2-BRC_VBR)
                                            BYTE *nFPS, 						//֡��(1-30??)
                                            BYTE *nEncodeMode,					//����ģʽ:�μ�ö�� eEncodeMode
                                            BYTE *nImageSize,					//�ֱ���:�μ�ö��	eImageSize
                                            BYTE *nImageQlty					//����(1-6)
                                            );
    /*����ͨ����Ƶ����*/
    long EDvr_SetChannelVideoInfo(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long nEncOpt,						//ͨ��ѡ��(1-Rec_Nomarl;2-Rec_Motion;3-RecAlarm;
        //4-Assistant_1;5-Assistant_2;6-Assistant_3)
        char *pstrChannelName,				//ͨ����
        bool bVideoEnable,					//ͼ��ʹ��
        BYTE nBitRate,						//����(����)����(1-BRC_CBR;2-BRC_VBR)
        BYTE nFPS,							//֡��(1-30??)
        BYTE nEncodeMode,					//����ģʽ:�μ�ö�� eEncodeMode
        BYTE nImageSize,					//�ֱ���:�μ�ö��	eImageSize
        BYTE nImageQlty 					//����(1-6)
        ); 
    
    /*��Ƶ�ֱ�������*/
    long EDvr_SetImageZoom(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long nEncKind,						//ͨ��ѡ��(1-������-����;2-������-�ƶ����;3-������-��������;
                                            //4-������1;5-������2;6-������3)
        long nImageZoom,					//�ֱ���(0-D1,1-HD1,2-BCIF,3-CIF,4-QCIF,5-VGA,6-QVGA,7-SVCD)
        long nZoonWidth,					//���(����)
        long nZoomHeigh,					//�߶�(����)
        long nFPS							//֡��(0-1,1-2,2-4,3-6,4-12,5-25)
        );
    /*��������������*/
    long EDvr_SetImageDefine(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long nEncKind,						//ͨ��ѡ��(1-������-����;2-������-�ƶ����;3-������-��������;
        //4-������1;5-������2;6-������3)
        long nBitRateType,					//��������(0-������,1-������,2-������(����))
        long nBitRate,						//��������(����)
        long nImageQlty,					//����(1-6)
        long nEncodeMode					//����ģʽ(0-DIVX_MPEG4;1-MS_MPEG4;2-MPEG2;3-MPEG1;4-H263;
        //5-MJPG;6-FCC_MPEG4;7-H264)
        );
    /*����ͨ������*/
    long EDvr_SetChannelName(
        char *pstrIP,						//DVR IP��ַ
        long nChType,						//ͨ������(0-��Ƶͨ��,1-��������ͨ��(����),2-�������ͨ��(����))
        long nChID, 						//ͨ�����
        char *pstrName						//���������
        );
    
    /*��ȡͨ����Ƶ����*/
    long EDvr_GetChannelAudioInfo(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long nEncOpt,						///ͨ��ѡ��(1-Rec_Nomarl;2-Rec_Motion;3-RecAlarm;
                                            //4-Assistant_1;5-Assistant_2;6-Assistant_3)
        bool *bAudioEnable, 				//��Ƶʹ��
        BYTE *nFormatTag,					//����(����,��0)
        WORD *nTrackCount,					//������(����,��0)
        WORD *nBitsPerSample,				//�������(����,��0)
        DWORD *nSamplesPerSec				//������(����,��0)
        );
    
    /*��ȡͨ��OSD����*/
    long EDvr_GetChannelOsdInfo(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long OsdOpt,						//Osd����:1-ʱ����ʾ��2-ͨ������ʵ��3-�ڵ���ʾ
        DWORD *nFrontColor, 				//��ɫ:4BYTE �ֱ�ΪR��G��B��͸����
        DWORD *nBackColor,					//��ɫ:4BYTE �ֱ�ΪR��G��B��͸����
        RECT *rcRecr,						//λ��:4LONG �ֱ�Ϊ�ϡ��¡�����
        bool *bOsdShow						//Osdʹ��
        );
    /*����ͨ��OSD����*/
    long EDvr_SetChannelOsdInfo(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long OsdOpt,						//Osd����:1-ʱ����ʾ��2-ͨ������ʵ��3-�ڵ���ʾ
        DWORD nFrontColor,					//��ɫ:4BYTE �ֱ�ΪR��G��B��͸����
        DWORD nBackColor,					//��ɫ:4BYTE �ֱ�ΪR��G��B��͸����
        const RECT *rcRecr, 				//λ��:4LONG �ֱ�Ϊ�ϡ��¡�����
        bool bOsdShow						//Osdʹ��
        );
    
    //////////////////////////////////////////////////////////////////////////
    // ��������
    
    /*��ȡ485����Э���б�*/
    long EDvr_Get485PorList(
        char *pstrIP,						//DVR IP��ַ
        DWORD *n485PorCount,				//485Э������
        char *str485PorList 			//Э�������б�
        );
    
    /*��ȡ232���ڹ����б�*/
    long EDvr_Get232FuncList(
        char *pstrIP,						//DVR IP��ַ
        DWORD *n232FuncCount,				//232��������
        char *str232FuncList				//���������б�
        );
    
    /*��ȡ458��������*/
    long EDvr_Get485Info(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long *n485Por,						//485Э����ţ���ӦЭ���б����
        DWORD *nCOMInfo,					//ͨ�ô������ã�4BYTE �ֱ�Ϊ
                                            //����λ��  0-5��1-6��2-7��3-8
                                            //ֹͣλ��  0-1λ, 1-1.5λ, 2-2λ
                                            //У�飺	0-no, 1-odd, 2-even
                                            //�����ʣ�  {0-300,1-600,2-1200,3-2400,4-4800,5-9600,6-19200,7-38400,8-57600,9-115200}*/
        BYTE *n485Address					//485�ڵ�ַ(0-255)
        );
    /*����458��������*/
    long EDvr_Set485Info(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long n485Por,						//485Э����ţ���ӦЭ���б����
        DWORD nCOMInfo, 					//ͨ�ô������ã�4BYTE �ֱ�Ϊ
                                            //����λ��  0-5��1-6��2-7��3-8
                                            //ֹͣλ��  0-1λ, 1-1.5λ, 2-2λ
                                            //У�飺    0-no, 1-odd, 2-even
                                            //�����ʣ�  {0-300,1-600,2-1200,3-2400,4-4800,5-9600,6-19200,7-38400,8-57600,9-115200}
        BYTE n485Address					//485�ڵ�ַ(0-255)
    );
    
    /*��ȡ232��������*/
    long EDvr_Get232Info(
        char *pstrIP,						//DVR IP��ַ
        long n232Index, 					//232�������
        long *n232Func, 					//232������ţ���Ӧ�����б����
        DWORD *nCOMInfo 					//ͨ�ô������ã�4BYTE �ֱ�Ϊ
                                            //����λ��0-5��1-6��2-7��3-8
                                            //ֹͣλ��0-1λ, 1-1.5λ, 2-2λ
                                            //У�飺	0-no, 1-odd, 2-even
                                            //�����ʣ�{0-300,1-600,2-1200,3-2400,4-4800,5-9600,6-19200,7-38400,8-57600,9-115200}
        );
    /*����232��������*/
    long EDvr_Set232Info(
        char *pstrIP,						//DVR IP��ַ
        long n232Index, 					//232�������
        long n232Func,						//232������ţ���Ӧ�����б����
        DWORD nCOMInfo						//ͨ�ô������ã�4BYTE �ֱ�Ϊ
                                            //����λ��0-5��1-6��2-7��3-8
                                            //ֹͣλ��0-1λ, 1-1.5λ, 2-2λ
                                            //У�飺    0-no, 1-odd, 2-even
                                            //�����ʣ�{0-300,1-600,2-1200,3-2400,4-4800,5-9600,6-19200,7-38400,8-57600,9-115200}
        );
    
    //////////////////////////////////////////////////////////////////////////
    // ��������
    
    /*��ȡ����˿�����*/
    long EDvr_GetNetPortInfo(
        char *pstrIP,						//DVR IP��ַ
        char *pstrDeviceName,				//�豸����
        WORD *nTCPCount,					//TCP��������
        WORD *nTCPPort, 					//TCP�˿ں�
        WORD *nUDPPort, 					//UDP�˿ں�
        WORD *nHTTPPort,					//HTTP�˿ں�
        WORD *HTTPSPort,					//HTTPS�˿ں�
        WORD *nSSLPort						//SSL�˿ں�
        );
    /*��������˿�����*/
    long EDvr_SetNetPortInfo(
        char *pstrIP,						//DVR IP��ַ
        const char *pstrDeviceName, 		//�豸����
        WORD nTCPCount, 					//TCP��������
        WORD nTCPPort,						//TCP�˿ں�
        WORD nUDPPort,						//UDP�˿ں�
        WORD nHTTPPort, 					//HTTP�˿ں�
        WORD HTTPSPort, 					//HTTPS�˿ں�
        WORD nSSLPort						//SSL�˿ں�
        );
    
    /*��ȡ��̫��������*/
    long EDvr_GetEthernetInfo(
        char *pstrIP,						//DVR IP��ַ
        long nEthernetIndex,				//��̫�������
        char *pstrEthernetIP,				//����IP
        char *EthernetMask, 				//������������
        char *GatewayIP,					//��������
        long *nNetMode, 					//�ӿ����ͣ�1-10MBase-T;2-10MBase-Tȫ˫��;3-100MBase-TX;4-100Mȫ˫��;5-10M/100M����Ӧ
        char *pstrMAC						//����MAC���
        );
    /*������̫��������*/
    long EDvr_SetEthernetInfo(
        char *pstrIP,						//DVR IP��ַ
        long nEthernetIndex,				//��̫�������
        const char *pstrEthernetIP, 		//����IP
        const char *EthernetMask,			//������������
        const char *GatewayIP,				//��������
        long nNetMode						//�ӿ����ͣ�1-10MBase-T;2-10MBase-Tȫ˫��;3-100MBase-TX;4-100Mȫ˫��;5-10M/100M����Ӧ
        //const char *pstrMAC				//����MAC���
        );
    
    /*��ȡԶ�̷���������*/
    long EDvr_GetRemoteHostInfo(
        char *pstrIP,						//DVR IP��ַ
        long nHostType, 					//����������: 1-����������;2-��־������;3- SMTP������;4-�ಥ��;5-NFS������;
                                            //6-Զ��Ftp������;7-PPPoE������;8-DDNS������;9-DNS������
        char *pstrHostIP,					//������IP
        WORD *nHostPort,					//�������˿�
        bool *bIsEnable,					//�����Ƿ���
        char *pstrUserName, 				//�û���
        char *pstrPassWord, 				//����
        char *pstrHostName					//��������Ϣ��PPPoE=>PPPoEע�᷵�ص�IP��DDNS=>DDNS��������������Ч
        );
    /*����Զ�̷���������*/
    long EDvr_SetRemoteHostInfo(
        char *pstrIP,						//DVR IP��ַ
        long nHostType, 					//����������: 1-����������;2-��־������;3- SMTP������;4-�ಥ��;5-NFS������;
                                            //6-Զ��Ftp������;7-PPPoE������;8-DDNS������;9-DNS������
        const char *pstrHostIP, 			//������IP
        WORD nHostPort, 					//�������˿�
        bool bIsEnable, 					//�����Ƿ���
        const char *pstrUserName,			//�û���
        const char *pstrPassWord,			//����
        const char *pstrHostName			//��������Ϣ��PPPoE=>PPPoEע�᷵�ص�IP��DDNS=>DDNS��������������Ч
        );
    
    /*��ȡ�ʼ�����������*/
    long EDvr_GetMailHostInfo(
        char *pstrIP,						//DVR IP��ַ
        char *pstrHostIP,					//������IP
        WORD *nHostPort,					//�������˿�
        char *pstrUserName, 				//�û���
        char *pstrPassWord, 				//����
        char *pstrDestAddr, 				//�ʼ���ַ
        char *pstrCcAddr,					//���͵�ַ
        char *pstrBccAddr,					//������ַ
        char *pstrSubject					//�ʼ�����
        );
    /*�����ʼ�����������*/
    long EDvr_SetMailHostInfo(
        char *pstrIP,						//DVR IP��ַ
        char *pstrHostIP,					//������IP
        WORD nHostPort, 					//�������˿�
        const char *pstrUserName,			//�û���
        const char *pstrPassWord,			//����
        const char *pstrDestAddr,			//�ʼ���ַ
        const char *pstrCcAddr, 			//���͵�ַ
        const char *pstrBccAddr,			//������ַ
        const char *pstrSubject 			//�ʼ�����
        );
    
    //////////////////////////////////////////////////////////////////////////
    // ����ϵͳ����
    
    /*��ȡ�豸��������*/
    long EDvr_GetDeviceAttribute(
        char *pstrIP,						//DVR IP��ַ
        BYTE *pDeviceAttributes,			//��Ƶ��,��Ƶ��,��������,�������,�����,USB��,IDE��,����,����
        long *pnAttributeSize				//�������鳤��
        );
    
    /*����I֡*/
    long EDvr_CaptureIFrame(
        char *pstrIP,						//DVR IP��ַ
        long nChID							//��Ƶͨ����
        );
    
    //////////////////////////////////////////////////////////////////////////
    // �û���Ϣ����
    
    /*��ȡ�û�Ȩ����Ϣ�б�*/
    long EDvr_GetUserRightList(char *pstrIP, fUserRightCB lpFunCB);
    
    /*�û�����Ϣ����*/
    long EDvr_GetUserGroupList(char *pstrIP, fUserGroupCB lpFunCB);
    long EDvr_GetUserGroup(
        char *pstrIP,						//DVR IP��ַ
        char *pstrGroupName,				//�û�������(����)
        long *nGroupID, 					//�û�����
        long *nRightNum,					//Ȩ������
        long *pnRightList,					//Ȩ�ޱ������,���100����
        char *pstrGroupMemo 				//�û��鱸ע
        );
    
    /*�û���Ϣ����*/
    long EDvr_GetUserInfoList(char *pstrIP, fUserInfoCB lpFunCB);
    long EDvr_GetUserInfo(
        char *pstrIP,						//DVR IP��ַ
        char *pstrUserName, 				//�û���(����)
        long *nUserID,						//�û����
        long *nGroupID, 					//�û�����
        char *pstrPassword, 				//����(����)
        long *nRightNum,					//Ȩ������
        long *pnRightList,					//Ȩ�ޱ������,���100����
        char *pstrUserMemo					//�û���ע
        );
    
    /*�����û���*/
    long EDvr_AddUserGroup(
        char *pstrIP,						//DVR IP��ַ
        long nGroupID,						//�û�����
        char *pstrGroupName,				//�û�������
        long nRightNum, 					//Ȩ������
        long *pnRightList,					//Ȩ�ޱ������,���100����
        char *pstrGroupMemo 				//�û��鱸ע
        );
    
    /*�����û�*/
    long EDvr_AddUser(
        char *pstrIP,						//DVR IP��ַ
        long nUserID,						//�û����
        char *pstrUserName, 				//�û���
        long nGroupID,						//�û�����
        char *pstrPassword, 				//����
        long nRightNum, 					//Ȩ������
        long *pnRightList,					//Ȩ�ޱ������,���100����
        char *pstrUserMemo					//�û���ע
        );
    
    
    /*ɾ���û���*/
    long EDvr_DeletUserGroup(char *pstrIP, char *pstrGroupName);
    /*ɾ���û�*/
    long EDvr_DeleteUser(
        char *pstrIP,
        char *pstrUserName,
        char *pstrPassword					//(��ʱ����)
        );
    
    /*�޸��û���*/
    long EDvr_EditUserGroup(
        char *pstrIP,						//DVR IP��ַ
        char *pstrGroupName,				//�û�����(����)
        long nGroupID,						//�û�����
        char *pstrNewName,					//���û�����
        long nRightNum, 					//Ȩ������
        long *pnRightList,					//Ȩ�ޱ������,���100����
        char *pstrGroupMemo 				//�û��鱸ע
        );
    /*�޸��û�*/
    long EDvr_EditUser(
        char *pstrIP,						//DVR IP��ַ
        char *pstrUserName, 				//�û���(����)
        long nUserID,						//�û����
        char *pstrNewName,					//���û���
        long nGroupID,						//�û�����
        char *pstrPassword, 				//(��ʱ����)
        long nRightNum, 					//Ȩ������
        long *pnRightList,					//Ȩ�ޱ������,���100����
        char *pstrUserMemo					//�û���ע
        );
    
    /*�޸��û�����*/
    long EDvr_ModifyUserPassword(
        char *pstrIP,						//DVR IP��ַ
        char *pstrUserName, 				//�û���(����)
        char *pstrOldPassword,				//������
        char *pstrNewPassword				//������
        );
    
    //��ӳ����û� 20081206
    long New_AddPowerUser(char* szIP, char* szUser, char* szPwd = NULL);
    
    //////////////////////////////////////////////////////////////////////////
    // �����ӿ���ϸ�����װ
    
    //������������
    // pstrIP��DVR IP��ַ
    // nChID����Ӧ����ͨ����ţ�û�е���0
    // nAlarmType���������� 0�źű��� 1�ƶ���� 2��Ƶ��ʧ 3��Ƶ�ڵ� 4�޴��̱��� 5���������� 6���̹��ϱ���
    // nAlarmEnable����������״̬
    // nValue1�����ò���1>>�źű���:0���� 1���գ��ƶ����,��Ƶ�ڵ�:������1-6�����̱���,��Ƶ��ʧΪ��������
    // nValue2�����ò���2>>�ƶ����:������� BYTE[12][16]�����̱���,�źű���,��Ƶ�ڵ�,��Ƶ��ʧ:��������
    // nValue3�����ò���3>>�ƶ����:������鳤��12*16�����̱���,�źű���,��Ƶ�ڵ�,��Ƶ��ʧ:��������
    long EDvr_GetAlarmNormalInfo(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nAlarmEnable, BYTE *nValue1, BYTE *nValue2,long *nValue3);
        
    //���������������
    // pstrIP��DVR IP��ַ
    // nChID����Ӧ����ͨ����ţ�û�е���0
    // nAlarmType���������� 0�źű��� 1�ƶ���� 2��Ƶ��ʧ 3��Ƶ�ڵ� 4�޴��̱��� 5���������� 6���̹��ϱ���
    // nOutEnable���Ƿ񱨾��������
    // pAlarmOut���������ͨ�����飬BYTE[16]
    // nAlarmOutSize������������鳤�� 16
    long EDvr_GetAlarmOut(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nOutEnable, BYTE *pAlarmOut, long *nAlarmOutSize);
    long EDvr_SetAlarmOut(char *pstrIP, long nChID, BYTE nAlarmType, BYTE nOutEnable, BYTE *pAlarmOut, long nAlarmOutSize);
    
    //��������¼������
    // pstrIP��DVR IP��ַ
    // nChID����Ӧ����ͨ����ţ�û�е���0
    // nAlarmType���������� 0�źű��� 1�ƶ���� 2��Ƶ��ʧ 3��Ƶ�ڵ� 4�޴��̱��� 5���������� 6���̹��ϱ���
    // nRecordEnable���Ƿ񱨾�����¼��
    // nPreRecLen��Ԥ¼ʱ��(��)
    // pAlarmRecord��¼��ͨ�����飬BYTE[16]
    // nAlarmRecordSize��¼��ͨ�����鳤�� 16
    long EDvr_GetAlarmRecord(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nRecordEnable, long *nPreRecLen, BYTE *pAlarmRecord, long *nAlarmRecordSize);
    long EDvr_SetAlarmRecord(char *pstrIP, long nChID, BYTE nAlarmType, BYTE nRecordEnable, long nPreRecLen, BYTE *pAlarmRecord, long nAlarmRecordSize);
    
    //����������̨
    // pstrIP��DVR IP��ַ
    // nChID����Ӧ����ͨ����ţ�û�е���0
    // nAlarmType���������� 0�źű��� 1�ƶ���� 2��Ƶ��ʧ 3��Ƶ�ڵ� 4�޴��̱��� 5���������� 6���̹��ϱ���
    // nPTZEnable���Ƿ񱨾�������̨
    // nPTZType����̨�������� BYTE[16]��0������ 1ת��Ԥ�õ� 2Ѳ�� 3�켣 4��ɨ
    // nPTZNo����̨���������� BYTE[16]��Ԥ�õ����/Ѳ�����/�켣���/��ɨ���
    // nPTZSize����̨���鳤�� 16
    long EDvr_GetAlarmPTZ(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nPTZEnable, BYTE *nPTZType, BYTE *nPTZNo, long *nPTZSize);
    long EDvr_SetAlarmPTZ(char *pstrIP, long nChID, BYTE nAlarmType, BYTE nPTZEnable, BYTE *nPTZType, BYTE *nPTZNo, long nPTZSize);
    
    //���������������ÿ���
    long EDvr_GetAlarmOtherEn(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *pOtherEnable, long *nOtherSize);
    
    //�����ַ����ӻص�
    void EDvr_SetDrawWndCallBackFunc(fDrawWndCB CallBackFunc);
    //�����ַ�����
    void EDvr_OpenDrawWnd();
    
    //////////////////////////////////////////////////////////////////////
    // Զ���ļ�����
    //��ͣ�ط�
    long EDvr_PausePlayBack(HWND hWnd, long bPause);
    //��֡����
    long EDvr_OneFramePlayBack(HWND hWnd, long bOneFrame);
    //���ò���λ��
    long EDvr_SetPosPlayBack(HWND hWnd, long nPlayTime);
    //���
    long EDvr_FastPlayBack(HWND hWnd);
    //����
    long EDvr_SlowPlayBack(HWND hWnd);
    //ץͼ
    long EDvr_ConvertPicture(HWND hWnd, char *pstrFileName);
    
    //�����ط��ļ�
    long New_OpenPlayBack(char *pstrIP, long nFileHandle, HWND hWnd);
    //ֹͣ�ط��ļ�
    void New_StopPlayBack( HWND hWnd );
    //�رջط��ļ�
    void New_ClosePlayBack( HWND hWnd );
    //���ûطŽ���
    long New_SetPosPlayBack(HWND hWnd, double dbPos);
    //���Ž��Ȼص�
    void New_RegPlayBackPos(DHPlayBackPos CallBackFunc, DWORD dwUser);
    //���ؽ��Ȼص�
    void New_RegDownloadPos(DHDownloadPos CallBackFunc, DWORD dwUser);
    
    //���ò���ͼ���ɫ��
    long New_GetColor(HWND hWnd, long* nBright, long* nContrast, long* nSaturation, long* nHue);
    long New_SetColor(HWND hWnd, long nBright, long nContrast, long nSaturation, long nHue);
    
    /*��ȡͨ����Ƶ����*/
    long New_GetChannelVideoInfo(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long nEncOpt,						//ͨ��ѡ��(1-Rec_Nomarl;2-Rec_Motion;3-RecAlarm;
                                            //         4-Assistant_1;5-Assistant_2;6-Assistant_3)
        char *pstrChannelName,				//ͨ����
        bool *bVideoEnable, 				//ͼ��ʹ��
        BYTE *nBitRateType, 				//����(����)����(1-BRC_CBR;2-BRC_VBR)
        long *nBitRate,
        BYTE *nFPS, 						//֡��(1-30??)
        BYTE *nEncodeMode,					//����ģʽ:�μ�ö�� eEncodeMode
        BYTE *nImageSize,					//�ֱ���:�μ�ö��	eImageSize
        BYTE *nImageQlty					//����(1-6)
        );
    /*����ͨ����Ƶ����*/
    long New_SetChannelVideoInfo(
        char *pstrIP,						//DVR IP��ַ
        long nChID, 						//��Ƶͨ����
        long nEncOpt,						//ͨ��ѡ��(1-Rec_Nomarl;2-Rec_Motion;3-RecAlarm;
                                            //         4-Assistant_1;5-Assistant_2;6-Assistant_3)
        char *pstrChannelName,				//ͨ����
        bool bVideoEnable,					//ͼ��ʹ��
        BYTE nBitRateType,					//����(����)����(1-BRC_CBR;2-BRC_VBR)
        long nBitRate,
        BYTE nFPS,							//֡��(1-30??)
        BYTE nEncodeMode,					//����ģʽ:�μ�ö�� eEncodeMode
        BYTE nImageSize,					//�ֱ���:�μ�ö��	eImageSize
        BYTE nImageQlty 					//����(1-6)
        ); 
    
    long New_GetFPSList(DWORD* nCount, char* pstrNameList);
    
    static const WORD nVideoRateList[12];
    long New_GetVideoRateList(DWORD* nCount, char* pstrNameList);
    
    //////////////////////////////////////////////////////////////////////////
    // OV��׼���ӽӿ�
    static long m_nDeviceType;
    static long m_nDataType;
    
    long New_InitDevice(long nDeviceType, long nDataType);
    void New_ExitDevice();
    long New_LoginDevice(char* szDeviceIP, long nPort,char* szUserID, char* szPassword , long nModelType);
    void New_LogoutDevice(char* szDeviceIP);
	 void New_LogoutDevice(long index);//add pan
    long New_StartLinkDevice(char* szDeviceIP, long nChannel, long nLinkType, long nNetPort, long nMediaType, void* UserContext = NULL);
    void New_StopLinkDevice(long nLinkID);
    long New_StartLinkCapture(long nLinkID, OV_PRealDataCallback fCallBack, void* objUser);
    void New_StopLinkCapture(long nLinkID);
    long New_GetHeadData(long nLinkID, BYTE* pHeadData);    
    //͸������ӿ�
    void in_SaveComCfg(int nLoginID);
    long in_SerialSend(int nLoginID, int nSerialPort, void* pDataBuf, long nDataSize);
    
    /************************************************************************/
    /*	�����ýӿ�															*/
    /************************************************************************/
    long T_InsertDevice(
        char *pstrIP,		//DVR IP��ַ
        long nPort, 		//DVR �˿ں�
        char *pstrLogName,	//DVR ��½�˺�
        char *pstrLogPass,	//DVR ��½����
        long nIndex 		//�豸�������
        );
    void T_UserAlarm(char *pstrIP, long nDVRPort, long nChID);
};

#endif // !defined(AFX_DEVICE_DH_H__8CB9885D_F5AD_4813_8C61_6C6016578886__INCLUDED_)
