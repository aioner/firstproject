#ifndef _DATAFILTER_H_
#define _DATAFILTER_H_

#ifdef _WIN32
#ifndef  LINKCOMM_EXPORTS
#define  LINKCOMMAPI	__declspec(dllimport)
#else
#define  LINKCOMMAPI	__declspec(dllexport)
#endif

#else
#include "xt_config.h"
#define LINKCOMMAPI __attribute__((visibility("default")))
#endif

#include <list>
#include <map>
using namespace std;

#define MAX_CHANNELS  32
#define MAX_HEAD_SIZE 2048
#define MAX_LINKS     1024

//�ص�����  
typedef long (__stdcall *OV_PRealDataCallback)(long nLinkHandle, long nFrameType, unsigned char*	pDataBuf, long	nDataLength, long nDataType, void* objUser, long nTimeStamp, unsigned long nSSRC);

#ifndef POUTPUTREALDATA__
#define POUTPUTREALDATA__

struct xt_client_cfg_t
{
	char udp_session_bind_ip[32];
	unsigned short udp_session_bind_port;
	unsigned short udp_session_heartbit_proid; //udp�Ự������������ ��λ������ Ĭ������Ϊ:20
	unsigned short udp_session_request_try_count;  //udp�Ự����ʧ�ܺ����Դ��� Ĭ������Ϊ:4
	unsigned short udp_session_request_one_timeout;  //udp�Ự�����ȴ�ʱ�� Ĭ������Ϊ:5000

	char tcp_session_bind_ip[32];
	unsigned short tcp_session_bind_port;
	unsigned short tcp_session_connect_timeout; 	//tcp�Ự���ӳ�ʱʱ�䣬��λ������ Ĭ������Ϊ:10000
	unsigned short tcp_session_login_timeout;	   	//tcp�Ự��¼��ʱʱ�䣬��λ������ Ĭ������Ϊ:10000
	unsigned short tcp_session_play_timeout;		//tcp�Ự�㲥��ʱʱ�䣬��λ������ Ĭ������Ϊ:2000
	unsigned short tcp_session_stop_timeout;		//tcp�Ựͣ�㳬ʱʱ�䣬��λ������ Ĭ������Ϊ:2000

	unsigned short rtsp_session_connect_timeout;		//rtsp�Ự���ӳ�ʱʱ�䣬��λ������ Ĭ������Ϊ:10000
	unsigned short rtsp_session_describe_timeout;		//rtsp�Ựdescribe��ʱʱ�䣬��λ������ Ĭ������Ϊ:10000
	unsigned short rtsp_session_setup_timeout;		//rtsp�Ựsetup��ʱʱ�䣬��λ������ Ĭ������Ϊ:10000
	unsigned short rtsp_session_play_timeout;         //rtsp�Ựplay��ʱʱ�䣬��λ������ Ĭ������Ϊ:10000
	unsigned short rtsp_session_pause_timeout;        //rtsp�Ựpause��ʱʱ�䣬��λ������ Ĭ������Ϊ:10000
	unsigned short rtsp_session_teardown_timeout;     //rtsp�Ựteardown��ʱʱ�䣬��λ������ Ĭ������Ϊ:1000
} ;

struct RTCP_SR
{
	unsigned int    mNTPtimestamp;  /* Most significant 32bit of NTP timestamp */
	unsigned int    lNTPtimestamp;  /* Least significant 32bit of NTP timestamp */
	unsigned int    timestamp;      /* RTP timestamp */
	unsigned int    packets;        /* Total number of RTP data packets transmitted by the sender since transmission started and up until the time this SR packetwas generated. */
	unsigned int    octets;     /* The total number of payload octets (not including header or padding */
};

struct RTCP_RR
{
	unsigned int    fractionLost;   /* The fraction of RTP data packets from source specified by SSRC that were lost since previous SR/RR packet was sent. */
	unsigned int    cumulativeLost; /* Total number of RTP data packets from source specified by SSRC that have been lost since the beginning of reception. */
	unsigned int    sequenceNumber; /* Sequence number that was received from the source specified by SSRC. */
	unsigned int    jitter;         /* Estimate of the statistical variance of the RTP data packet inter arrival time. */
	unsigned int    lSR;            /* The middle 32 bits of the NTP timestamp received. */
	unsigned int    dlSR;         /* Delay since the last SR. */
};

//�������׻ص�����
typedef long (__stdcall *POUTPUTREALDATA)(long nLinkHandle, unsigned char* pDataBuf, long nDataLength, long nFrameType, long nDataType,void* objUser, long nTimeStamp, unsigned long nSSRC);

//����ע��ص�����
typedef long (__stdcall *regist_call_back_t)(const char *ip, unsigned short port, const unsigned char *data, unsigned length);

//SSRC����ص�����
typedef void (__stdcall *rtcp_report_callback_t)(void *ctx, unsigned int ssrc, const RTCP_SR *sr, const RTCP_RR *rr);
#endif//POUTPUTREALDATA__


struct LINKINFO;
//typedef map<long, LINKINFO*> LINKMAP;

struct PORTINFO
{
	int nDevType;
	int nNetLinkType;
	int nPort;
};

struct RCVINFO 
{
	int index;
	unsigned short port_rtp;
	unsigned short port_rtcp;
	bool demux;
	unsigned int demuxid;
};


//��ʶΨһ��һ·��������Ϣ

/************************************************************************************
                                     �豸�������
*************************************************************************************/
class CDeviceCollect;

class LINKCOMMAPI CBaseDevice
{
public:
	CBaseDevice(void);
	virtual ~CBaseDevice(void);

	//��ʼ���豸
	virtual long InitDevice(void* pParam){return -1;}

	//����ʼ���豸
	virtual long UnInitDevice(void* pParam){return -1;}

	//�豸��¼
	virtual long LoginDevice(const char* szDeviceIP, long  nPort, const char* szUserID, const char* szPassword){return 0xefffffff;}

	//�豸�ǳ�
	virtual bool LogoutDevice(long lLoginHandle){return 0;}

	//�����豸���ӣ�ȷ����������
	virtual long StartLinkDevice(char *szDeviceIP, long nNetPort , long nChannel, long nLinkType, long nMediaType, long sockethandle, 
		const char *szMulticastIp, unsigned short nMulticastPort,const char *szLocalIP){return 0;}

	//�����豸���ӣ�ȷ���������� XMPP
	virtual long StartLinkDevice(const char *ip, const char* sdp, long sdp_len, long channel, long link_type,  long media_type){return 0;}

	//�����豸���� -URL
	virtual long StartLinkDevice(const char *szURL){return 0;}

	//�ر��豸����
	virtual void StopLinkDevice(long lDeviceLinkHandle){}

	//�����ɼ�
	virtual long StartLinkCapture(long lDeviceLinkHandle, OV_PRealDataCallback lpRealDatacallback,void* UserContext){return 0;}

	//�رղɼ�
	virtual long  StopLinkCapture(long lDeviceLinkHandle){return 0;}

	//ȡ���豸״̬
	virtual long  GetDeviceStatus(long lDeviceLinkHandle){return 0;}

	virtual long GetSDP(long lDeviceLinkHandle, unsigned char *msg, long& length){return 0;}

	//�ļ������ӿ�
	virtual long TcpPlayCtrl(long lDeviceLinkHandle, double npt, float scale, unsigned long *rtp_pkt_timestamp){return 0;}
	virtual long TcpPauseCtrl(long lDeviceLinkHandle){return 0;}

	//RTSP�ӿ�
	virtual long RtspPlayCtrl(long lDeviceLinkHandle, double npt, float scale, unsigned long *rtp_pkt_timestamp){return 0;}
	virtual long RtspPauseCtrl(long lDeviceLinkHandle){return 0;}

	//
	virtual long GetClientInfo(long lDeviceLinkHandle, long& rtp_recv_port, long& rtcp_recv_port, bool& multiplex_r, long & multid_r){return 0;}

	//����SSRC����
	virtual	long SetSSRCReport(long lDeviceLinkHandle, rtcp_report_callback_t reportFunc, void* pContext){return 0;}

	//���ö����ش�����
	virtual long SetResend(int resend,int wait_resend, int max_resend, int vga_order){return 0;}

	//////////////////////////////////////////////////////////////////////////
	virtual long create_recv(int track_num, bool demux){return 0;}

	virtual long create_recvinfo(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports){return 0;}

	virtual long get_rcvinfo(long link, RCVINFO *infos, int &num){return 0;}

	virtual long set_sdp(long link, const char *sdp, unsigned int len_sdp){return 0;}

	virtual long request_iframe(long link_hadle){return 0;}
    virtual long set_regist_callback(regist_call_back_t func){return 0;}
	///////////////////////////////////////////////////////////////////////////////

	/********************************************�����ɹ�����ͳ�����,���಻��ʵ��**********************************************/
	//֪ͨ����ͷ
	long NotifyHeadData(long lDeviceLinkHandle, unsigned char* szHead, long nSize);

	//���ݲ������ָ��
	void  SetDeviceCollect(CDeviceCollect *pDeviceCollect);

	//ͨ��IP��õ�¼���
	long  GetLoginHandle(char* szIp);

	//��ȡ���еĶ˿�
	bool  GetPorts(long portnum, long *pPorts,  bool bIsSort = false);

	//�ͷŶ˿�
	void FreePorts(long *pPorts, long portnum);

	//��ȡý������
	long  GetMediaType(long lLinkHandle);

	//��������
	void SetLinkType(long nLinkType, long nDataType);

	//�������
	void* AddLinkInfo(LINKINFO *pLinkinfo);

    //ɾ������
    void DelLinkInfo(long linkhandle);

	//��ȡ������Ϣ
	LINKINFO*   GetLinkMap();

	int GetLinkInfo(long hanlde, LINKINFO **link);

    //��ȡ������
    long   GetLinkCount();

private:
	//AddLink Lock
	//CRITICAL_SECTION   m_lockAddLink;

	//��������,��Ӧ�����DLL
	long  m_nLinkType;

	//������������(TCP/UDP/�鲥)
	//int  m_nNetLinkType;

	CDeviceCollect *m_pDeviceCollect;

	//�����б�
	//LINKMAP  m_mapLink; 
    LINKINFO *m_Links;
	
public:
	//��Ӧ����Ľ���DLL
	long  m_nDataType;
};


struct DEVINFO
{
	long nDevLinkType; //��������
	long nNetLinkType; //������������ tcp/udp/rtp
};

/************************************************************************************
                                �豸������
*************************************************************************************/
class LINKCOMMAPI CDeviceCollect : public map<long,CBaseDevice*>
{
public:
	CDeviceCollect(void);
	~CDeviceCollect(void);

	//�����豸�б�
	bool   LoadDevices(char* szWorkPath);

	//ע���豸
	bool   UnLoadDevices();

	//ͨ���豸���ͻ�ȡ�豸����
	CBaseDevice* GetDevice(long nDeviceType);

	//��ȡ���ص����豸������
	long   GetDeviceCount();

	//��ȡ������
	long   GetLinkCount();

	//��ȡ��������
	long GetNetlinkType(int devType);

	//�ж��Ƿ��ѵ�¼
	long   GetLoginHandle(char* szIP, long nDeviceType);

	//�ж��Ƿ��ѵ�¼
	long   GetLoginHandle(char* szIP, CBaseDevice *pDevice);

	//�ж��Ƿ�������
	long  GetLinkHandle(char* szIP, long nChannel, long nMediaType, CBaseDevice *pDevice);

	//��ȡ������Ϣ
	int GetLinkInfo(long nLinkHandle, CBaseDevice *pDevice, LINKINFO **link);

	//�Ƿ���ָ����¼������豸����������
	bool     IsDeviceLink(long lLoginHandle, CBaseDevice *pDevice);

	void     AddDevice(CBaseDevice *pDevice, long nDeviceLinkType);

	long SetPortRange(long startPort, long portNum);

	//����˿�
	bool  GetPorts(long portnum, long *pPorts,  bool bIsSort = false);

	//�ͷŶ˿�
	void FreePorts(long *pPorts, long portnum);

	//���ö˿�
	bool QueryPort(long nDevType, long nNetLinkType, long& nPort);

private:

	//�豸�������������͵�ӳ��
	map<long, DEVINFO> m_mapLinkType;

	//��ʼ�˿ں�
	long   m_nStartPort;
	long   m_nEndPort;

	//�˿�����
	list<PORTINFO> m_PortInfo;

	//�˿�״̬����
	unsigned char* m_pPortStatus;
	
};

#ifdef __cplusplus
extern "C" {
#endif
//�������
CBaseDevice LINKCOMMAPI * AddA(CBaseDevice* pDevice,  CDeviceCollect *pDeviceCollect, long nDeviceLinkType, long nDataType);


#ifdef __cplusplus
};
#endif

#ifdef _WIN32
#ifndef  LINKCOMM_EXPORTS
#pragma comment(lib, "LinkComm2.0.lib")
#pragma message("Auto Link LinkComm2.0.lib")
#endif//#ifndef  LINKCOMM_EXPORTS
#endif//#ifdef _WIN32

#endif //_DATAFILTER_H_
