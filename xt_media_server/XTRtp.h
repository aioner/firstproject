#ifndef XTRTP_H__INCLUDE___
#define XTRTP_H__INCLUDE___

#ifdef _OS_WINDOWS
#include <Windows.h>
#endif
#include <string>
#include <map>
#include "xt_mp_caster_def.h"
#include "xt_mp_caster_api.h"
#include "common.h"
#include <list>

#include <boost/thread/thread.hpp>
#include "h_xtmediaserver.h"

enum Rtp_Mode 
{
    M_RTP = 0,
    M_MUL = 1,
};

// rtp���͵�Ԫ��Ϣ
struct  Rtp_Handle
{
    mp_h_s			hmp;		// ���͵�Ԫ���
    mssrc_h_s		hmssrc;		// ���ݴ�����
    msink_h_s		hmsink;		// ���ݷ��;��
    int				payload_old;// ��������old
    int				payload;	// ��������new
    unsigned short  port;		// ���Ͷ˿�
    bool			multiplex;	// ����
    unsigned int	multid;		// ����ID

#ifdef _USE_RTP_SEND_CONTROLLER
    xt_network_changed_callback_t cb;
    void *ctx;
#endif
};

// ת���ڵ�
struct Rtp_Sink 
{
    common::Time_Type  createtime;//����ʱ��
    unsigned int	chanid;		// ͨ����
    msink_h_s		hmsink;		// �ڵ���
    std::string			ip;			// ip		
    unsigned short	port;		// port
    void*			linkid;		// ���Ӿ��
    short			mode;		// ����ģʽ(0,1: ������2���鲥)
    unsigned int	ssrc;		// ssrc
    bool			multiplex;	// ����
    unsigned int	multid;		// ����id

	bool local_multiplex;//���͸���
	unsigned int local_multid;//���͸���id

    rv_rtcp_info send;
    rv_rtcp_info recieve;
	uint32_t rr_rtt;

    Rtp_Sink& operator=(const Rtp_Sink& rf)
    {
        this->createtime = rf.createtime;
        this->chanid = rf.chanid;
        this->hmsink = rf.hmsink;
        this->ip = rf.ip;
        this->port = rf.port;
        this->linkid = rf.linkid;
        this->mode = rf.mode;
        this->ssrc = rf.ssrc;
        this->multiplex = rf.multiplex;
        this->multid = rf.multid;
		this->local_multiplex = rf.local_multiplex;
		this->local_multid = rf.local_multid;
        ::memcpy(&(this->send),&(rf.send),sizeof(rv_rtcp_info));
        ::memcpy(&(this->recieve),&(rf.recieve),sizeof(rv_rtcp_info));

        return *this;
    }
};

#define MAX_SINK_IP_LEN 256
typedef struct _struct_get_rtp_sink_condition_type_
{
    char ip[MAX_SINK_IP_LEN];  //ip
    unsigned long chanid;      //ͨ��
    unsigned short port;      //�˿�
    bool multiplex_flag;      //���ñ�ʶ
    unsigned int muxid;       //����id
}rtp_sink_comdition_t,*prtp_sink_comdition_t;

class XTRtp
{
private:
    XTRtp(void);
    ~XTRtp(void);

    static XTRtp self;

public:
    static XTRtp* instance(){return &self;}
    static XTRtp* _(){return &self;}

    // rtpԶ�˵�ַ����
    static void _raddr_cb(void *hmp, rv_net_address *addr);

    // rtcp�ص�
    static void sink_rtcp_cb(unsigned int ssrc,		// ssrc
        rv_rtcp_info &send,			// sr
        rv_rtcp_info &recieve,
		unsigned char *ip,
		unsigned short port,
		int multiplex,
		unsigned int multid);		// rr

    // rtcp app�ص�
    static rv_bool rtcpapp_msg_cb(unsigned int		chan_id,	// ͨ����
        unsigned char		subtype,	// ������	
        unsigned int		ssrc,		// ssrc
        unsigned char*		name,		// msg name
        unsigned char*		userData,	// user data
        unsigned int		userDataLen);// user data length

    // rtcp raw cb
    static rv_bool rtcp_rawdata_cb(
        mp_handle sink,
        uint8_t *buffer,
        uint32_t buffLen,
        rv_net_address *remoteAddress,
        rv_bool *pbDiscardBuffer);

    // �Ƿ���ͬsink
    static bool is_same_sink(Rtp_Sink &s1, Rtp_Sink &s2);

    // ��ʼ��
    int init(unsigned int num_chan,			// ͨ����
        std::string ip,						// ip
        unsigned short start_port,		// ��ʼ�˿�
        bool multiplex,					// ����
        bool sink_single,				// ͨ����ת��
        bool use_traffic_shapping = false);     //�Ƿ�����������

    // ����ʼ��
    int uninit();

    // ���ø�������
    void set_payload(unsigned long chanid, int payload, bool update);
    void reset_payload(unsigned long chanid);

	int get_payload(unsigned long chanid);

    //���¶����ش���־ add by songlei 20150708
    int update_resend_flag(const int flag);

	// �����ļ�����
	void set_file_patha(const char * file);

    // ����ת��
    int add_send(long chanid,			// ͨ����
        std::string ip,						// ip
        unsigned short port,			// port
        void* linkid,					// ���Ӿ��
        long mode,						// ����ģʽ(0,1: ������2���鲥)
        unsigned int ssrc = 0,			// ssrc
        bool multiplex = false,			// ����
        unsigned int multid = 0);		// ����id

    // ɾ��ת��
    int del_send(long chanid,			// ͨ����
        std::string ip,						// ip
        unsigned short port,			// port
        long mode,						// ����ģʽ(0,1: ������2���鲥)
        bool multiplex = false,			// ����
        unsigned int multid = 0);		// ����id

    // ɾ��ת��(linkid �Ự���)
    int del_send(void *linkid);

    // ɾ������ת��
    int del_send_all();

    // ɾ��ͨ��ת��(chanid ͨ����)
    int del_send_chan(unsigned long chanid);

    int send_data(unsigned long chanid, char *buff, unsigned long len, int frame_type, long device_type, bool is_std);
    int send_data(const xt_track_t& track, char *buff, unsigned long len, long device_type, bool is_std);

    // ���ݷ���
    int send_data_in_stamp(unsigned long chanid,			// ͨ����
        char *buff,						      // ��������
        unsigned long len,				// ���ݳ���
        int frame_type,					// ֡����
        long device_type,               // �豸����
        bool frame_ts_flg,    // �Ƿ��ⲿ����ʱ��
        uint32_t in_time_stamp,// �ⲿ����ʱ��
        uint8_t priority,
        bool is_std = false);

    // ֡���ݷ���
    int send_data_in_stamp(const xt_track_t& track,  // ת��track
        char *buff,                                  // ��������
        unsigned long len,                           // ���ݳ���
        long device_type,                            // �豸����
        bool frame_ts_flg,                           // �Ƿ��ⲿ����ʱ��
        uint32_t in_time_stamp,                      // �ⲿ����ʱ��
        uint8_t priority,                            // ���ȼ�
        bool is_std = false,
		bool use_ssrc =false,
		uint32_t ssrc = 0);                        // ��׼������

	// �����ݷ���
	int send_rtp_in_stamp(const xt_track_t& track,  // ת��track
		char *buff,                                  // ��������
		unsigned long len,                           // ���ݳ���
		long device_type,                            // �豸����
		bool frame_ts_flg,                           // �Ƿ��ⲿ����ʱ��
		uint32_t in_time_stamp,                      // �ⲿ����ʱ��
		uint8_t priority,                            // ���ȼ�
		bool is_std = false,
		bool use_ssrc =false,
		uint32_t ssrc = 0);                        // ��׼������

        //20160405 ���ӹ���PS��RTP���ͽӿ�
    int send_data_in_stamp_ps(const xt_track_t& track,    // ת��track
        char *buff,                               // ��������
        unsigned long len,                        // ���ݳ���
        long device_type,                         // �豸����
        bool frame_ts_flg,                        // �Ƿ��ⲿ����ʱ��
        uint32_t in_time_stamp,                   // �ⲿ����ʱ��
        uint8_t priority,                         // ���ȼ�
        bool is_std,                              // ��׼������
        bool usr_ssrc,
        uint32_t ssrc);


    void get_rtp_sink(std::list<Rtp_Sink>& lst_rtp_Sink);

    int get_cur_sink_num();

	int get_sink_sn(long chanid, unsigned short *sn);

    int get_rtp(unsigned long chanid, Rtp_Handle &rtp);

    //����RTCP����
    void set_rtcp_rport(unsigned int ssrc, const rv_rtcp_info &send,const rv_rtcp_info &recieve,
		char *ip, unsigned short port, bool multiplex, unsigned int multid);

    //��ȡsink
    int get_sink_by_comdition(Rtp_Sink& out_sink,const rtp_sink_comdition_t& comdition);

#ifdef _USE_RTP_SEND_CONTROLLER
    int register_network_changed_callback(int chan, xt_network_changed_callback_t cb, void *ctx);
#endif

private:
    // ���ת���ڵ�(sink in/out)
    int get_sink(Rtp_Sink &sink);
    int get_sink(void *linkid, Rtp_Sink &sink);
    int get_sink(unsigned long chanid, Rtp_Sink &sink);

    int get_rtp(void *hmp, unsigned long &chanid, Rtp_Handle &rtp);

    // ����ת���ڵ�
    int add_sink(Rtp_Sink &sink);

    // ɾ��ת���ڵ�
    int delete_sink(Rtp_Sink &sink);

    // ɾ��ת��(δ����)
    int _del_send(unsigned long chanid,			
        std::string ip,						
        unsigned short port,			
        short mode,					
        bool multiplex,		
        unsigned int multid); 
private:
    boost::shared_mutex			m_mutex;		//mutex
    std::map<unsigned int, Rtp_Handle>	m_rtpHandles;	// rtpת����Ԫ
    std::list<Rtp_Sink>					m_rtpSinks;		// rtpת���ڵ�

    bool							m_sink_single;	//ͨ����ת��
};
#endif//#ifndef XTRTP_H__INCLUDE___
