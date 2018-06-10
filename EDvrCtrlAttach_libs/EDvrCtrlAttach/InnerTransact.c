#include "stdafx.h"
#include "InnerTransact.h"
#include "InfoManage.h"
#include "OV_DeviceControl.h"
#include "WriteLog.h"


#ifndef _WIN32
#include <unistd.h>
#endif


extern HPlugMrg g_hPlugMrg;
extern HDeviceMrg g_hDeviceMrg;
extern HReloginMrg g_hReloginMrg;
extern HAlarmMrg g_hAlarmMrg;
extern HDDnsDevice g_hDDnsMrg;
extern SEDvrCtrlEvent g_sCtrlEvent;

BYTE s_bSendEvent[0x400] = {0};
char *s_szEventMark[0x400] = {0};
//////////////////////////////////////////////////////////////////////////

void FireOnlineChangedTrance(SEDvrCtrlEvent* pCtrlEvent, LPCSTR szIDS, LPCSTR szAddress, int nDeviceType, int nOnline, int nRes, LPCSTR szRes)
{
    WriteLog(c_szFlag, "���ͷ�Ƕ��ʽ�豸����״̬ IDS:%s Address:%s DeviceType:%d OnlineFlag:%d",\
        szIDS, szAddress, nDeviceType, nOnline);
    if(pCtrlEvent->fpOnlineEvent)
        pCtrlEvent->fpOnlineEvent(szIDS, szAddress, nDeviceType, nOnline, nRes, szRes, pCtrlEvent->objOnline);
}

void FireOnlineChanged(SEDvrCtrlEvent* pCtrlEvent, SDeviceCtrlInfo *pDevice, int nDeviceType, int nOnline, int nRes, LPCSTR szRes)
{
    int nAlarmType[6] = {eStateAlarmIn, eStateMotionDetection, eStateVideoLost, eStateVideoBlind, eStateIllegalAccess, eStateMediaBitrate};
    pDevice->m_sWorkState.nOnlineState = nOnline;

    WriteLog(c_szFlag, "�����豸����״̬ IDS:%s Address:%s DeviceType:%d OnlineFlag:%d",\
        pDevice->m_szIDS, pDevice->m_szAddress, nDeviceType, nOnline);

    if(pCtrlEvent->fpOnlineEvent)
        pCtrlEvent->fpOnlineEvent(pDevice->m_szIDS, pDevice->m_szAddress, nDeviceType, nOnline, nRes, szRes, pCtrlEvent->objOnline);

    //������豸���ߣ���Ҫ�����豸����״̬
    if(nOnline)
    {
        WriteLog(c_szFlag, "�豸���� ͬ���豸״̬ IDS:%s Address:%s DeviceType:%d", pDevice->m_szIDS, pDevice->m_szAddress, nDeviceType);
        Device_GetDeviceAttrib(pDevice);
        Device_Listen(pDevice);
        Device_GetAlarmStateAsync(pDevice, nAlarmType, 6);
    }
}

void FireAlarmState(SEDvrCtrlEvent* pCtrlEvent, SDeviceCtrlInfo *pDevice, LPCSTR szStateList, int nStateSize, int nAlarmType, LPCSTR szRes)
{
    if(pCtrlEvent->fpAlarmStateEvent)
        pCtrlEvent->fpAlarmStateEvent(pDevice->m_szIDS, szStateList, nStateSize, nAlarmType, szRes, pCtrlEvent->objAlarm);
}

void FireSerialData(SEDvrCtrlEvent* pCtrlEvent, SDeviceCtrlInfo *pDevice, int nSerialPort, void* pBufData, int nBufSize)
{
    if(pCtrlEvent->fpSerialEvent)
        pCtrlEvent->fpSerialEvent(pDevice->m_szIDS, nSerialPort, pBufData, nBufSize, pCtrlEvent->objSerial);
}

void FireDeviceIPChanged(SEDvrCtrlEvent* pCtrlEvent, LPCSTR szSerialNum, LPCSTR szAddress, int nDeviceType)
{
    if(pCtrlEvent->fpIPChange)
        pCtrlEvent->fpIPChange(szSerialNum, szAddress, nDeviceType, pCtrlEvent->objIPChange);
}

void FireDvrStateInfo(SEDvrCtrlEvent* pCtrlEvent, SDeviceCtrlInfo *pDevice)
{
    if(pCtrlEvent->fpDvrStateInfo)
    {
        Device_MakeStateXML(pDevice, pCtrlEvent->szSendBuffer);
        pCtrlEvent->fpDvrStateInfo(pDevice->m_szIDS, pDevice->m_szAddress, g_sCtrlEvent.szSendBuffer,\
            lstrlen(g_sCtrlEvent.szSendBuffer)+1, pCtrlEvent->objStateInfo);
    }
}

void FireDvrStateChanged(SEDvrCtrlEvent* pCtrlEvent, SDeviceCtrlInfo *pDevice, int eStateEvent,\
                         int nChannel, int nEventValue, void* pEventData, int nDataSize)
{
    char *pDisposeState=NULL, *pAlarmState=NULL;
    if(eStateEvent < STATE_VALUE(0))
    {//������״̬����Ҫ���⴦��
        //��¼�澯״̬
        switch(eStateEvent)
        {
        case eStateAlarmIn:     //�ź�����
            pDisposeState = pDevice->m_sWorkState.nInputDispose;
            pAlarmState = pDevice->m_sWorkState.bInputAlarm;
            break;
        case eStateDiskFull:    //������
            pAlarmState = pDevice->m_sWorkState.bDiskFull;
            break;
        case eStateVideoLost:   //��Ƶ��ʧ
            pDisposeState = pDevice->m_sWorkState.nLostDispose;
            pAlarmState = pDevice->m_sWorkState.bVideoLost;
            break;
        case eStateMotionDetection://�ƶ����
            pDisposeState = pDevice->m_sWorkState.nMotionDispose;
            pAlarmState = pDevice->m_sWorkState.bMotionAlarm;
            break;
        case eStateNoForamt:    //����δ��ʽ��
        case eStateDiskError:   //������
            pAlarmState = pDevice->m_sWorkState.bDiskError;
            break;
        case eStateVideoBlind:  //��Ƶ�ڵ�
            pDisposeState = pDevice->m_sWorkState.nBlindDispose;
            pAlarmState = pDevice->m_sWorkState.bVideoBlind;
            break;
        default: return;
        }

        if(pDisposeState && !pDisposeState[nChannel])
            return;//�豸û�в��������Ը澯
        pAlarmState[nChannel] = nEventValue;
        if(!s_bSendEvent[eStateEvent])
            return;//������Ϣ�ϴ�
        if(s_szEventMark[eStateEvent])
        {
            pEventData = s_szEventMark[eStateEvent];
            nDataSize = strlen(s_szEventMark[eStateEvent])+1;
        }
    }
    else
    {
        //��ֵ״̬
    }

    if(pCtrlEvent->fpDvrStateChanged)
    {
    
#ifdef _WIN32
		__try
		{
			pCtrlEvent->fpDvrStateChanged(pDevice->m_szIDS, pDevice->m_szAddress, eStateEvent, nChannel, nEventValue, pEventData, nDataSize, pCtrlEvent->objStateChange);
		}
		__except(1)
		{
			WriteLog1(0, c_szFlag, "StateChanged App Error IDS:%s Address:%s Event:%d Channel:%d EventValue:%d EventData:%p DataSize:%d ObjUser:%p",
				pDevice->m_szIDS, pDevice->m_szAddress, eStateEvent, nChannel, nEventValue, pEventData, nDataSize, pCtrlEvent->objStateChange);
		}
#else
        pCtrlEvent->fpDvrStateChanged(pDevice->m_szIDS, pDevice->m_szAddress, eStateEvent, nChannel, nEventValue, pEventData, nDataSize, pCtrlEvent->objStateChange);
#endif
    }
}

//////////////////////////////////////////////////////////////////////////
void OnlineCheck_Start(SEDvrCtrlEvent* pCtrlEvent)
{
    ThreadPulse_Start(&pCtrlEvent->thrCheckOnline, CTRL_ONLINE_TIMER, NULL, (fThreadStateEvent)OnlineTimer, 1000, pCtrlEvent);
}

void OnlineCheck_Stop(SEDvrCtrlEvent* pCtrlEvent)
{
    ThreadPulse_Stop(&pCtrlEvent->thrCheckOnline);
}

//////////////////////////////////////////////////////////////////////////
void StateListen_Start(SEDvrCtrlEvent* pCtrlEvent)
{
    ThreadPulse_Start(&pCtrlEvent->thrGetWorkState, CTRL_GETSTATE_TIMER, NULL, (fThreadStateEvent)GetStateTimer, 1000, pCtrlEvent);
    ThreadPulse_Start(&pCtrlEvent->thrSendWorkState, CTRL_SENDSTATE_TIMER, NULL, (fThreadStateEvent)SendStateTimer, 1000, pCtrlEvent);
}

void StateListen_Stop(SEDvrCtrlEvent* pCtrlEvent)
{
    ThreadPulse_Stop(&pCtrlEvent->thrGetWorkState);
    ThreadPulse_Stop(&pCtrlEvent->thrSendWorkState);
}

//////////////////////////////////////////////////////////////////////////
// alarm Signla

void AlarmFilter_Start(SEDvrCtrlEvent* pCtrlEvent)
{
    ThreadPulse_Start(&pCtrlEvent->thrAlarmFilter, CTRL_ALARMFILTER_TIMER, NULL, (fThreadStateEvent)AlarmTimer, 1000, pCtrlEvent);
}

void AlarmFilter_Stop(SEDvrCtrlEvent* pCtrlEvent)
{
    ThreadPulse_Stop(&pCtrlEvent->thrAlarmFilter);
}

int AlarmDispose_Check(SDeviceCtrlInfo *pDevice, int nAlarmType, int nChannel, int nAlarmValue, int nDuration)
{
    char *pAlarmDispose = NULL;
    int nAlarmSize = 0;

    if(nAlarmValue == 0)
        return 0;

    //����ڲ�״̬Ϊ�����������յ��˸澯�źţ�˵����Ҫ���θ��ź�
    Device_GetAlarmState(pDevice, nAlarmType, &nAlarmSize, &pAlarmDispose);
    if(nChannel >= nAlarmSize)
        return -1;

    if(!pAlarmDispose[nChannel])
        return -1;
    else
        return 0;
}

void AlarmSignla_Receive(SDeviceCtrlInfo *pDevice, int nAlarmType, int nChannel, int nAlarmValue, int nDuration)
{
    int nAlarmID;
    SAlarmTimer sNewAlarm, *pAlarm = NULL;

    if(nAlarmValue == 0)
    {
        AlarmSignla_Clean(pDevice, nAlarmType, nChannel);
        return;
    }

    nAlarmID = AlarmMrg_SelectAlarm(g_hAlarmMrg, pDevice->m_nDeviceID, nAlarmType, nChannel, &pAlarm);
    if(nAlarmID < 0)
    {//�µĸ澯
        InitAlarmTimer(&sNewAlarm);
        sNewAlarm.m_nDeviceID = pDevice->m_nDeviceID;
        sNewAlarm.m_nAlarmType = nAlarmType;
        sNewAlarm.m_nChannel = nChannel;
        if(nDuration == 0)
            nDuration = 1;
        sNewAlarm.m_nTimer = sNewAlarm.m_nMarkTime = nDuration;
        nAlarmID = ArrayListAddAt((HArrayList)g_hAlarmMrg, &sNewAlarm);
        //����澯
        FireDvrStateChanged(&g_sCtrlEvent, pDevice, nAlarmType, nChannel, TRUE, NULL, 0);
    }
    else
    {//�����澯
        pAlarm->m_nTimer = pAlarm->m_nMarkTime;
    }
}

void AlarmSignla_Clean(SDeviceCtrlInfo *pDevice, int nAlarmType, int nChannel)
{
    int nAlarmID;
    SAlarmTimer *pAlarm = NULL;

    nAlarmID = AlarmMrg_SelectAlarm(g_hAlarmMrg, pDevice->m_nDeviceID, nAlarmType, nChannel, &pAlarm);
    if(nAlarmID < 0)
        return;

    ArrayListEraseAt((HArrayList)g_hAlarmMrg, nAlarmID);
}

//////////////////////////////////////////////////////////////////////////

void DDnsCheck_Start(SEDvrCtrlEvent *pCtrlEvent)
{
    ThreadPulse_Start(&pCtrlEvent->thrDDnsCheck, CTRL_DDNSCHECK_TIMER, NULL, (fThreadStateEvent)AlarmTimer, 10000, pCtrlEvent);
}

void DDnsCheck_Stop(SEDvrCtrlEvent *pCtrlEvent)
{
    ThreadPulse_Stop(&pCtrlEvent->thrDDnsCheck);
}

//////////////////////////////////////////////////////////////////////////

void OfflineCheck_Start( SEDvrCtrlEvent* pCtrlEvent )
{
	ThreadPulse_Start(&pCtrlEvent->thrCheckOffline, CTRL_OFFLINECHECK_TIMER, NULL, (fThreadStateEvent)OfflineTimer, 10000, pCtrlEvent);
}

void OfflineCheck_Stop( SEDvrCtrlEvent* pCtrlEvent )
{
	ThreadPulse_Stop(&pCtrlEvent->thrCheckOffline);
}
void CALLBACK OfflineTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent)
{
	size_t i;
	SDeviceCtrlInfo *pDevice;
	if(nStateID != PULSE_THREAD_WAITE_TIMEOUT)
		return;

	//��ʱ���ж�
	//if(g_sCtrlEvent.nQuestTimer++ < g_sCtrlEvent.nSendStateTime)
	//	return;

	//g_sCtrlEvent.nQuestTimer = 0;//��ʱ������
	for(i=0; i<ArrayListGetSize((HArrayList)g_hDeviceMrg); i++)
	{
		Sleep(0);
		pDevice = DeviceMrg_GetInfo(g_hDeviceMrg, i);
		if(!pDevice || pDevice->m_sWorkState.nOnlineState != 1 || pDevice->m_nLoginHandle == -1)
			continue;

		WriteLog(c_szFlag, "��ʱ����豸����״̬ IDS:%s Address:%s", pDevice->m_szIDS, pDevice->m_szAddress);
		Device_GetMediaBitrate(pDevice);
		if (pDevice->m_sWorkState.nPrimaryRate[0] == -1)
		{
			SOV_CtrlPlug *pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
			if (pControl)
			{
				SReloginDevice newRelogin;
				WriteLog(c_szFlag, "����豸���� IDS:%s Address:%s", pDevice->m_szIDS, pDevice->m_szAddress);
				FireOnlineChanged(&g_sCtrlEvent, pDevice, pControl->m_sType.m_nDeviceType, 0, 0, "");
				//�����豸Ϊ����״̬������ӵ��ֶ���������
				newRelogin.m_nDeviceID = pDevice->m_nDeviceID;
				newRelogin.m_nTimer = newRelogin.m_nMarkTime = pControl->m_nOnlineDelay/*10*/;
				ReloginMrg_AddInfoCheck(g_hReloginMrg, &newRelogin);
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CALLBACK OnlineTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent)
{
    size_t i;
    SReloginDevice *pRelogin;
    SDeviceCtrlInfo *pDevice;
    SOV_CtrlPlug *pControl;
    if(nStateID != PULSE_THREAD_WAITE_TIMEOUT)
        return;

    for(i=0; i<ArrayListGetSize((HArrayList)g_hReloginMrg); i++)
    {
        Sleep(0);
        //�õ�һ����Ҫ����������豸
        pRelogin = ReloginMrg_GetInfo(g_hReloginMrg, i);
        if(!pRelogin)
            continue;
        //����ʱ
        if(--pRelogin->m_nTimer <= 0)
        {//����ʱ��ɣ����Ե�¼�豸
            pDevice = DeviceMrg_GetInfo(g_hDeviceMrg, pRelogin->m_nDeviceID);
            if(!pDevice)
            {//�豸���Ƴ���������
                ArrayListEraseAt((HArrayList)g_hReloginMrg, i);
                continue;
            }

            if(0 > Device_Login(pDevice))
            {//��¼ʧ�ܣ��豸û������
                pRelogin->m_nTimer = pRelogin->m_nMarkTime;
                continue;
            }

            pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
            if(!pControl->m_bAutoOnline)
            {//���Զ��������豸��Ҫ�����豸״̬
                FireOnlineChanged(&g_sCtrlEvent, pDevice, pControl->m_sType.m_nDeviceType, 1, 0, "");
            }

            ArrayListEraseAt((HArrayList)g_hReloginMrg, i);
        }
    }
}

void CALLBACK AlarmTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent)
{
    size_t i;
    SAlarmTimer *pTimer;
    SDeviceCtrlInfo *pDevice;
    if(nStateID != PULSE_THREAD_WAITE_TIMEOUT)
        return;

    for(i=0; i<ArrayListGetSize((HArrayList)g_hAlarmMrg); i++)
    {
        Sleep(0);
        //�õ�һ����Ҫ����ĸ澯
        pTimer = AlarmMrg_GetInfo(g_hAlarmMrg, i);
        if(!pTimer)
            continue;
        //����ʱ
        if(--pTimer->m_nTimer == 0)
        {//����ʱ��ɣ��ϴ��澯��������������ʱ
            pDevice = DeviceMrg_GetInfo(g_hDeviceMrg, pTimer->m_nDeviceID);
            if(pDevice)
                FireDvrStateChanged(&g_sCtrlEvent, pDevice, pTimer->m_nAlarmType, pTimer->m_nChannel, 0, NULL, 0);
            ArrayListEraseAt((HArrayList)g_hAlarmMrg, i);
        }
    }
}

void CALLBACK GetStateTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent)
{
    size_t i;
    SDeviceCtrlInfo *pDevice;
    if(nStateID != PULSE_THREAD_WAITE_TIMEOUT)
        return;

    //��ʱ���ж�
    if(g_sCtrlEvent.nQuestTimer++ < g_sCtrlEvent.nSendStateTime)
        return;

    g_sCtrlEvent.nQuestTimer = 0;//��ʱ������
    for(i=0; g_sCtrlEvent.nSendStateTime && i<ArrayListGetSize((HArrayList)g_hDeviceMrg); i++)
    {
        Sleep(0);
        pDevice = DeviceMrg_GetInfo(g_hDeviceMrg, i);
        if(!pDevice || pDevice->m_sWorkState.nOnlineState != 1)
            continue;

        WriteLog(c_szFlag, "�ɼ��豸����״̬ IDS:%s Address:%s", pDevice->m_szIDS, pDevice->m_szAddress);
        Device_GetDeviceWorkstate(pDevice);
    }
}

void CALLBACK SendStateTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent)
{
    size_t i;
    SDeviceCtrlInfo *pDevice;
    if(nStateID != PULSE_THREAD_WAITE_TIMEOUT)
        return;

    //��ʱ���ж�
    if(g_sCtrlEvent.nSendTimer++ < g_sCtrlEvent.nSendStateTime)
        return;

    g_sCtrlEvent.nQuestTimer = 0;//��ʱ������
    for(i=0; g_sCtrlEvent.nSendStateTime && i<ArrayListGetSize((HArrayList)g_hDeviceMrg); i++)
    {
        Sleep(0);
        pDevice = DeviceMrg_GetInfo(g_hDeviceMrg, i);
        if(!pDevice || pDevice->m_sWorkState.nOnlineState != 1)
            continue;

        WriteLog(c_szFlag, "����豸����״̬ IDS:%s Address:%s", pDevice->m_szIDS, pDevice->m_szAddress);
        FireDvrStateInfo(&g_sCtrlEvent, pDevice);
    }
}

void CALLBACK DDnsCheckTimer(unsigned nThreadFlag, unsigned nStateID, SEDvrCtrlEvent* pCtrlEvent)
{
    size_t i;
    SOV_CtrlPlug *pControl;
    SDDnsDeviceInfo *pDDnsDevice;
    const char *szNewAddress;
    if(nStateID != PULSE_THREAD_WAITE_TIMEOUT)
        return;

    for(i=0; g_sCtrlEvent.nSendStateTime && i<ArrayListGetSize(g_hDDnsMrg); i++)
    {
        Sleep(0);
        pDDnsDevice = DDnsMrg_GetInfo(g_hDDnsMrg, i);
        if(!pDDnsDevice)
            continue;

        pControl = PlugMrg_GetInfo(g_hPlugMrg, pDDnsDevice->nDDnsTypeID);
        if(!pControl)
            continue;

        szNewAddress = Plug_SelectByDNS(pControl, g_sCtrlEvent.szDDnsAddress, pDDnsDevice->szSerialID);
        if(!szNewAddress || !szNewAddress[0]
           || lstrcmp(szNewAddress, "0.0.0.0")
           || !lstrcmp(szNewAddress, pDDnsDevice->szDeviceAddress))
        {
            continue;
        }
        lstrcpyn(pDDnsDevice->szDeviceAddress, szNewAddress, MAX_ADDRESS_LENGTH-1);
        FireDeviceIPChanged(&g_sCtrlEvent, pDDnsDevice->szSerialID, pDDnsDevice->szDeviceAddress, pDDnsDevice->nDeviceType);
    }
}

//////////////////////////////////////////////////////////////////////////

void CALLBACK OnDeviceAddressChanged(LPCSTR szUrl, LPCSTR szAddress, int nDeviceType, void* objUser)
{
    WriteLog(c_szFlag, "���յ��豸��ַ�ı� Url:%s Address:%s DeviceType:%d", szUrl, szAddress, nDeviceType);
    FireDeviceIPChanged(&g_sCtrlEvent, szUrl, szAddress, nDeviceType);
}

void CALLBACK OnDeviceOnlineChanged(int nLoginID, LPCSTR szAddress, int nDeviceType, int nOnlineFlag, void* objUser)
{
    int nDeviceID = -1;
    SDeviceCtrlInfo *pDevice = NULL;

    WriteLog(c_szFlag, "���յ��豸����״̬ Address:%s DeviceType:%d OnlineFlag:%d", szAddress, nDeviceType, nOnlineFlag);
    
    nDeviceID = DeviceMrg_SelectAddress(g_hDeviceMrg, szAddress, &pDevice);
    if(0 > nDeviceID)
        return;//�豸��ƥ��

    FireOnlineChanged(&g_sCtrlEvent, pDevice, nDeviceType, nOnlineFlag, 0, "");
}

void CALLBACK OnDeviceAlarmChanged(int nLoginID, LPCSTR szAddress, int nDeviceType, int nChannel, int eStateEvent,\
                                   int nEventValue, BYTE pEventData[], int nDataSize, void* objUser)
{
    int nDeviceID, nDuration = -1;
    SDeviceCtrlInfo *pDevice = NULL;
    nDeviceID = DeviceMrg_SelectAddress(g_hDeviceMrg, szAddress, &pDevice);

    if(0 > nDeviceID)
        return;//�豸��ƥ��

    if(pEventData == NULL || nDataSize == 0)
        nDuration = -1;

    if(nDataSize == sizeof(int))
        nDuration = *(int*)pEventData;
    if(nDuration == 0)
        nDuration = 3;//���ٳ���3��

    //�쿴����״̬
    if(0 > AlarmDispose_Check(pDevice, eStateEvent, nChannel, nEventValue, nDuration))
        return;

    //����ʽ�澯��������ʱ
    if(nEventValue && nDuration != -1)
    {
        AlarmSignla_Receive(pDevice, eStateEvent, nChannel, nEventValue, nDuration);
        return;
    }

    //������ʽ�澯
    WriteLog(c_szFlag, "�յ��豸�澯��Ϣ Address:%s DeviceType:%d Channel:%d Event:%d Value:%d",\
        szAddress, nDeviceType, nChannel, eStateEvent, nEventValue);
    FireDvrStateChanged(&g_sCtrlEvent, pDevice, eStateEvent, nChannel, nEventValue, pEventData, nDataSize);
}

void CALLBACK OnSerialOutput(int nLoginID, LPCSTR szAddress, int lPortID, BYTE pDataBuf[], int lBufSize, void* objUser)
{
#ifdef _WIN32
	int nDeviceID = (int)(INT_PTR)objUser;
#else
	int nDeviceID = (long)((void*)objUser);
#endif
    SDeviceCtrlInfo *pDevice = DeviceMrg_GetInfo(g_hDeviceMrg, nDeviceID);

    WriteLog(c_szFlag, "�յ��豸͸������ Address:%s PortID:%d Size:%d", szAddress, lPortID, lBufSize);

    if(!pDevice || strcmp(pDevice->m_szAddress, szAddress))
        return;//�豸��ƥ��

    FireSerialData(&g_sCtrlEvent, pDevice, lPortID, pDataBuf, lBufSize);
}
