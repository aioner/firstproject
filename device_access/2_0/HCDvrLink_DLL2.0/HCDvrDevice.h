#ifndef HCDVRDEVICE_H__
#define HCDVRDEVICE_H__
#include "BaseDevice.h"

#ifdef _WIN32
#include <Windows.h>
#ifndef  HCDVRDEVICE_EXPORTS
#define  HCDVRDEVICEAPI __declspec(dllimport)
#else
#define  HCDVRDEVICEAPI __declspec(dllexport)
#endif

#else
#include "xt_config.h"
#include <pthread.h>
#define HCDVRDEVICEAPI __attribute__((visibility("default")))
#endif

#include "HCNetSDK.h"

class CriticalSection
{
public:
    CriticalSection()
    {
#ifdef _WIN32
        ::InitializeCriticalSection( &m_cs );
#else
        ::pthread_mutex_init(&m_cs, NULL);
#endif //#ifdef _WIN32
    }

    ~CriticalSection()
    {
#ifdef _WIN32
        ::DeleteCriticalSection( &m_cs );
#else
        ::pthread_mutex_destroy(&m_cs);
#endif //#ifdef _WIN32
    }

    void lock()
    {
#ifdef _WIN32
        ::EnterCriticalSection( &m_cs );
#else
        ::pthread_mutex_lock(&m_cs);
#endif //#ifdef _WIN32
    }

    void unlock()
    {
#ifdef _WIN32
        ::LeaveCriticalSection( &m_cs );
#else
        ::pthread_mutex_unlock(&m_cs);
#endif //#ifdef _WIN32
    }

#ifdef _WIN32
    CRITICAL_SECTION &getLock()
    {
        return m_cs;
    }
#else
    pthread_mutex_t &getLock()
    {
        return m_cs;
    }
#endif //#ifdef _WIN32

#if(_WIN32_WINNT >= 0x0400)
    bool trylock()
    {
        return ( TryEnterCriticalSection( &m_cs ) != 0 );
    }
#endif /* _WIN32_WINNT >= 0x0400 */

private:

#ifdef _WIN32
    CRITICAL_SECTION m_cs;
#else
    pthread_mutex_t m_cs;
#endif //#ifdef _WIN32
};

struct DEV_LINKINFO
{
    int mediaType;
    int port;
    int channel;
    int linkType;
    void* hAnalyzerHandle;
    OV_PRealDataCallback pRealDatacallback;

    DEV_LINKINFO()
    {
        hAnalyzerHandle = NULL;
        pRealDatacallback = NULL;
    }
};

struct DEV_LINKMAPS
{
    DEV_LINKINFO devLinkinfo;
    CriticalSection* cs;
	char headData[2048];
	int    headLen;
	DEV_LINKMAPS()
	{
		headData[0]='\0';
		headLen = 0;
	}
};
/*************************************************************************
����DVR�豸��
**************************************************************************/
class CHCDvrDevice : public CBaseDevice
{
public:
    CHCDvrDevice(void);
    ~CHCDvrDevice(void);

    //��ʼ���豸
    virtual long InitDevice(void* pParam);

    //����ʼ���豸
    virtual long UnInitDevice(void* pParam);

    //�豸��¼,��Ҫ��¼���豸��д�ú���������Ҫ��¼���豸������д�ú���
    virtual long LoginDevice(const char* szDeviceIP, long  nPort, const char* szUserID, const char* szPassword);

    //�豸�ǳ�
    virtual bool LogoutDevice(long lLoginHandle);

    //�����豸���ӣ�ȷ����������
    virtual long StartLinkDevice(char *szDeviceIP,long nNetPort ,long nChannel,long nLinkType,long nMediaType, long sockethandle = 0, const char *szMulticastIp = NULL, unsigned short nMulticastPort = 0);

    //�ر��豸����
    virtual void StopLinkDevice(long lDeviceLinkHandle);

    //�����ɼ�
    virtual long StartLinkCapture(long lDeviceLinkHandle, OV_PRealDataCallback lpRealDatacallback,void* UserContext);

    //�رղɼ�
    virtual long  StopLinkCapture(long lDeviceLinkHandle);

	//��ȡSDP
    virtual long GetSDP(long lDeviceLinkHandle, unsigned char *msg, long& length);

	///////�ļ�����//////////////////////////////////////////////////////////////
    virtual long TcpPlayCtrl(long lDeviceLinkHandle, double npt, float scale, unsigned long *rtp_pkt_timestamp);

    virtual long TcpPauseCtrl(long lDeviceLinkHandle);
    //////////////////////////////////////////////////////////////////////////////

    //map<long, CriticalSection*> m_LinkMap;
    map<long, DEV_LINKMAPS> m_LinkMap;


private:
    //�������У������ж��Ǳ�����ͨ������IPC
    map<long,NET_DVR_DEVICEINFO_V30> m_loginMap;

};

#ifdef __cplusplus
extern "C"{
#endif
    HCDVRDEVICEAPI CBaseDevice* AddMe(CDeviceCollect* pDeviceCollect, long nDeviceLinkType, long nDataType);
#ifdef __cplusplus
};
#endif

#endif //#ifndef HCDVRDEVICE_H__
