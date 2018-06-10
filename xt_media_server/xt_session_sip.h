#ifndef _H_H_XT_SIP_
#define _H_H_XT_SIP_

#include "xt_sip_api.h"
#include "sdp.h"
#include "xtXml.h"
#include "parse_buffer.h"
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/noncopyable.hpp>
#include "h_xtmediaserver.h"
#include "common.h"
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/condition.hpp>

using namespace xt_sdp;

#define NUM_STREAM	16
#define LEN_SDP		2048
#define LEN_URL		512
#define LEN_NODE	128
#define MAX_ID_LEN 512
#define MAX_TIME_LEN 256

enum RES_STATE
{
    RS_NONE,
    RS_FAIL,
    RS_SUCCESS,
    RS_DOING
};

typedef struct _register_info_t 
{
    char target[LEN_URL];
    char sdp[LEN_SDP];
    uint32_t sdp_len;

    regist_timer_t timer;
    xt_sip_channel_category_t channel_type; //�ŵ�����
    xt_sip_client_register_handle_t handle;
}register_info_t;

typedef struct _track_info_t
{
    char trackname[LEN_NODE];
    char ip[LEN_NODE];
    int payload;
    unsigned short port;
    unsigned int demuxid;
    bool multicast;
    bool demux;

    _track_info_t& operator=(const _track_info_t& rf)
    {
        ::strncpy(this->trackname,rf.trackname,LEN_NODE);
        ::strncpy(this->ip,rf.ip, LEN_NODE);
        this->payload = rf.payload;
        this->port = rf.port;
        this->multicast = rf.multicast;
        this->demuxid = rf.demuxid;
        this->demux = rf.demux;
        return *this;
    }
}track_info_t;

typedef struct _struct_session_info_type_
{
    std::string call_id;
	std::string  res_pri;
    std::string username;
    std::string peerurl; //ͨ���Է���URI
    uint64_t sessionid;
    std::string ip;
    char sdp[LEN_SDP];
    uint32_t len_sdp;
    int srcno;
    std::vector<track_info_t> tracks;
    common::Time_Type creat_time;

    _struct_session_info_type_():
    sessionid(0),len_sdp(0),srcno(-1)
    {
        ::memset(sdp,0,LEN_SDP);
    }

    _struct_session_info_type_& operator=(const _struct_session_info_type_& rf)
    {
        this->call_id = rf.call_id;
        this->username = rf.username;
        this->peerurl = rf.peerurl;
        this->sessionid = rf.sessionid;
        this->ip = rf.ip;
        ::strncpy(this->sdp,rf.sdp,LEN_SDP);
        this->len_sdp = rf.len_sdp;
        this->srcno = rf.srcno;
        this->tracks = rf.tracks;
        this->creat_time = rf.creat_time;

        return *this;
    }
}session_info_t;

//�豸��־��Ϣ
typedef struct _struct_device_flag_type
{
    char devname[SIPMSG_NOEDE_LEN];
    char seq[SIPMSG_NOEDE_LEN];
    char devid[SIPMSG_NOEDE_LEN];
    char groupid[SIPMSG_NOEDE_LEN];
}dev_flag_t,*pdev_flag_t;

//�Ự��־��Ϣ
typedef struct _struct_session_flag_type
{
    char call_id[MAX_ID_LEN];
    char peerurl[LEN_URL];//ͨ���Է���URI

}session_flag_t,*psession_flag_t;

enum meida_type_t
{
    media_na   = -1,          //û����
    meida_video_and_adio = 0,  //����Ƶ��
    meida_audio = 1,
    meida_video = 2
};

class xt_session_sip :boost::noncopyable 
{
public:
    static xt_session_sip* inst(){return &self_;}

    int start(const start_sip_port_t& sip_port, const start_timer_t& timer, const char *domain, const char *username, const char *password);
    bool is_init()
    {
        return start_;
    }

    void stop();

    void regist(const regist_timer_t& timer, const sip_channel_t& chtype,const char *target,const char* sdp, uint32_t sdp_len);

    void unregist();

    void set_sdp(unsigned short index, const char *sdp, unsigned short len);

	void set_sdp_full(unsigned short index, const char *sdp, unsigned short len);

    void set_sipmsg_ptz_cb(sipmsg_ptz_cb cb);

    void set_sipmsg_picupdate_cb(sipmsg_picupdate_cb cb);

    void set_sipmsg_cb(sipmsg_cb cb);

    void set_sipinfo_cb(sipinfo_cb cb);

    void set_sipmsg_ifrmconfreq_cb(sipmsg_ifrmconfreq_cb cb);

    void set_sip_video_adjust_cb(sip_video_adjust_cb cb);

    void set_sip_get_dev_info_cb(sip_get_dev_info_cb cb);

    void set_sip_point_index_operation_cb(sip_point_index_operation_cb cb);

    void set_sip_register_srv_ret_info_cb(sip_register_srv_ret_info_cb cb);

    void set_sipmsg_bandwidth_cb(sipmsg_bandwidth_cb cb);

    void set_sipmsg_profile_cb(sipmsg_profile_cb cb);

    void modify_video_resolution(const session_info_t &session,const int strmid,const char *sdp, uint32_t len);

    void pro_re_inivte(const session_info_t &new_session,const int strmid,const char *sdp, uint32_t len, uint32_t mux_flag);

public:

    //! ע��ɹ�֮���ٴ�ˢ��ע����Ϣ
    //! \returns xt_sip_status_t
    //! ������ο�xt_sip_api_types.h   
    xt_sip_status_t xt_sip_client_register_request_refresh()
    {
        return ::xt_sip_client_register_request_refresh(register_info_.handle,register_info_.timer.expires);
    }

    //! ���ܶԶ˷��͹�����message/info��Ϣ������Ӧ2xx����
    //! \param [in] h ���лỰ���
    //! \param [in] content_type ��������
    //! \param [in] content      ����
    //! \param [in] content_length      �����ֽڳ���
    //! \returns xt_sip_status_t
    //!          ������ο�xt_sip_api_types.h
    xt_sip_status_t xt_sip_server_invite_accept_NIT(xt_sip_server_invite_handle_t h,
        uint32_t code, const char *content_type, const char *content, uint32_t content_length)
    {
        return ::xt_sip_server_invite_accept_NIT(h,code,content_type,content,content_length);
    }

    //! �ܾ��Զ˷��͹�����message/info��Ϣ������Ӧ��2xx���� ���̰߳�ȫ
    //! \param [in] h ���лỰ���
    //! \param [in] code ��Ӧ�� 4xx
    //! \returns xt_sip_status_t
    //!          ������ο�xt_sip_api_types.h
    xt_sip_status_t xt_sip_server_invite_reject_NIT(xt_sip_server_invite_handle_t h, uint32_t code)
    {
        return ::xt_sip_server_invite_reject_NIT(h,code);
    }

	//! ���ͻỰ��message��Ϣ
	//! \param [in] h ���лỰ���
	//! \param [in] content_type ��������
	//! \param [in] content      ����
	//! \param [in] content_length      �����ֽڳ���
	//! \returns xt_sip_status_t
	//!          ������ο�xt_sip_api_types.h
	xt_sip_status_t xt_sip_client_invite_message(xt_sip_client_invite_handle_t h, 
		const char *content_type, const char *content, uint32_t content_length)
	{
		return ::xt_sip_client_invite_message(h,content_type,content,content_length);
	}

    //! ����message����
    //! \param [in] h ���лỰ���
    //! \param [in] code ��Ӧ�� 2xx
    //! \returns xt_sip_status_t
    //!          ������ο�xt_sip_api_types.h
    xt_sip_status_t xt_sip_server_message_accept(xt_sip_server_message_handle_t h, int code=200)
    {
        return ::xt_sip_server_message_accept(h,code);
    }

    //! �ܾ�message���� ���̰߳�ȫ
    //! \param [in] h ���лỰ���
    //! \param [in] code ��Ӧ�� 4xx
    //! \returns xt_sip_status_t
    //!          ������ο�xt_sip_api_types.h
    xt_sip_status_t xt_sip_server_message_reject(xt_sip_server_message_handle_t h, int code=408)
    {
        return ::xt_sip_server_message_reject(h,408);
    }

    //! ����offer���в���Ӧanswer����
    //! \param [in] h ���лỰ���
    //! \param [in] sdp
    //! \param [in] sdp�ֽڳ���
    //! \returns xt_sip_status_t
    //!          ������ο�xt_sip_api_types.h
    xt_sip_status_t xt_sip_server_invite_provide_answer(xt_sip_server_invite_handle_t h, const char *sdp, uint32_t len)
    {
        return ::xt_sip_server_invite_provide_answer(h,sdp,len);
    }

	//! ����offer���в���Ӧanswer����
	//! \param [in] h ���лỰ���
	//! \param [in] sdp
	//! \param [in] sdp�ֽڳ���
	//! \returns xt_sip_status_t
	//!          ������ο�xt_sip_api_types.h
	xt_sip_status_t xt_sip_server_invite_provide_offer(xt_sip_server_invite_handle_t h, const char *sdp, uint32_t len)
	{
		return ::xt_sip_server_invite_provide_offer(h,sdp,len);
	}

    //! �ܾ����в��� ���̰߳�ȫ
    //! \param [in] h ���лỰ���
    //! \param [in] code ������ 4xx
    //! \returns xt_sip_status_t
    //!          ������ο�xt_sip_api_types.h
    xt_sip_status_t xt_sip_server_invite_reject(xt_sip_server_invite_handle_t h, uint32_t code=488)
    {
        return ::xt_sip_server_invite_reject(h,code);
    }

    //! ���ͻỰ��info��Ϣ ���̰߳�ȫ
    //! \param [in] h ���лỰ���
    //! \param [in] content_type ��������
    //! \param [in] content      ����
    //! \param [in] content_length      �����ֽڳ���
    //! \returns xt_sip_status_t
    //!          ������ο�xt_sip_api_types.h
    xt_sip_status_t xt_sip_server_invite_info(xt_sip_server_invite_handle_t h,const char *content_type, const char *content, uint32_t content_length)
    {
        return ::xt_sip_server_invite_info(h,content_type,content,content_length);
    } 

    //! ����message����֧��ͬ����ʱ��֧���첽���첽��ʱ�����ݲ�֧��
    //! \param [in] sip sip������
    //! \param [in] prof ���þ����Ŀǰ�����Ź��ܣ�NULL����
    //! \param [in] request message����
    //! \param [in,out] response message��Ӧ��ΪNULLʱ��ʾ�첽����
    //! \param [in] cb ͬ�����첽ģʽ�µĻص�����
    //! \param [in] ctx �ص������û�����
    //! \param [in] timeout ��ʱʱ�䣬��λ������
    //! \returns xt_sip_status_t
    //!          ������ο�xt_sip_api_types.h
    xt_sip_status_t xt_sip_make_client_message(xt_sip_handle_t sip, xt_sip_prof_handle_t prof,const xt_sip_client_message_request_t *request,xt_sip_client_message_response_t *response, xt_sip_client_message_callback_t *cb, void *ctx, uint32_t timeout=3000)
    { 
        return ::xt_sip_make_client_message(sip, prof, request,response, cb, ctx, timeout);
    }


    //! ͨ��sip��Ϣ�����ȡcontent body����Ҫ��xt_sip_buf_handle_delete�ͷ�
    //! \param [in] msg sip��Ϣ���
    //! \returns xt_sip_buf_handle_t
    //!                 content bodyֵ
    xt_sip_buf_handle_t xt_sip_msg_get_content_body(xt_sip_msg_handle_t msg)
    {
        return ::xt_sip_msg_get_content_body(msg);
    }

    //! ͨ��sip��Ϣ�����ȡCallID����Ҫ��xt_sip_buf_handle_delete�ͷ�
    //! \param [in] msg sip��Ϣ���
    //! \returns xt_sip_buf_handle_t
    //!                 CallIDֵ
    xt_sip_buf_handle_t xt_sip_msg_get_call_id(xt_sip_msg_handle_t msg)
    {
        return ::xt_sip_msg_get_call_id(msg);
    }

    //! ��ȡ�ڴ��ֽ�����
    //! \param [in] h �ڴ���
    //! \returns void *
    //!          �ֽ�����
    void *xt_sip_buf_get_data(xt_sip_buf_handle_t h)
    {
        return ::xt_sip_buf_get_data(h);
    }

    //! ��ȡ�ڴ泤��
    //! \param [in] h �ڴ���
    //! \returns uint32_t
    //!          �ֽڳ���
    uint32_t xt_sip_buf_get_len(xt_sip_buf_handle_t h)
    {
        return ::xt_sip_buf_get_len(h);
    }

    //! ɾ���ڴ���
    //! \param [in] h �ڴ���
    //! \returns 
    void xt_sip_buf_handle_delete(xt_sip_buf_handle_t h)
    {
        ::xt_sip_buf_handle_delete(h);
    }

    //! ͨ��sip��Ϣ�����ȡ�Զ�ipv4��ַ �̰߳�ȫ
    //! \param [in] msg sip��Ϣ���
    //! \param [out] ipv4 v4��ַ,ip 1.2.3.4 = ipv4 [0...3]=[1,2,3,4]
    //! \param [out] port v4�˿�
    //! \returns xt_sip_status_t
    //!          ������ο�xt_sip_api_types.h
    xt_sip_buf_handle_t xt_sip_msg_get_from(xt_sip_msg_handle_t msg)
    {
        return ::xt_sip_msg_get_from(msg);
    }

protected:
    xt_session_sip();

    ~xt_session_sip();

    static xt_session_sip self_;

    int get_strmid(const char *sdp, unsigned short len);

    int get_media_info(const char *sdp, unsigned short len, session_info_t &session);

    int construct_video_adjust_param(video_adjust_param_t& outparams,const session_info_t& insession,const char *insdp,const int insdp_len);

    bool is_multicast_ipv4(const std::string& ipv4);

    int get_dev_info(dev_info_t& out_devinfo);

    meida_type_t parse_media_type(const session_info_t& session);

    int get_packet_loss(double& out_loss,const session_info_t& session,const std::string& comdition="video");

    int get_delay(const session_info_t& session,const std::string& comdition="video");


    //�ص�
    //client
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    static void on_client_invite_terminated(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, xt_sip_invite_terminated_reason_t reason);
    static void on_client_register_response(void *ctx, xt_sip_client_register_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);
    static void on_client_register_removed(void *ctx, xt_sip_client_register_handle_t h, xt_sip_msg_handle_t msg);
    static int on_client_register_request_retry(void *ctx, xt_sip_client_register_handle_t h, int retrysec, xt_sip_msg_handle_t msg);

    //ע��
    void regist_impl();

    //��ȡregistʧ�ܺ���ע��ʱ��
    inline const uint16_t get_regist_retry_time() const
    {
        return register_info_.timer.regist_retry_time_interval;
    }

    void add_options();
    void del_options();
    void update_register_stat(const bool status);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //server
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    static void on_server_invite_offer(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len);
    static void on_server_invite_connected_confirmed(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);
    static void on_sip_server_invite_terminated(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, xt_sip_invite_terminated_reason_t reason);
    static void on_server_invite_info(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);
    static void on_server_invite_info_response(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);
    static void on_server_invite_message(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);//�Ự��
    static void on_server_invite_message_response(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);
    static void on_server_message_arrived(void *ctx, xt_sip_server_message_handle_t h, xt_sip_msg_handle_t msg); //�Ự��

	// ����sdp
	static void on_server_invite_offer_required(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);
	static void on_server_invite_answer(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len);

	static void on_client_message_response(void *ctx, xt_sip_client_message_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);
	
    //����
    static int8_t on_xt_sip_heartbeat_not_pong_cb(void *ctx, const char *target, uint32_t);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

public:
    void construct_devinfo_response(std::string& response,const dev_flag_t& dflg);
    void construct_session_info_response(std::string& response,const dev_flag_t& dflg);
    void construct_operator_response_ctx(std::string& out_ctx,int opr_ret_code,int response_code,const char* seq,const char* fail_case);
    void clear_send_data_all();
    void upate_client_register_handle(const xt_sip_client_register_handle_t handle)
    {
        if (XT_SIP_INVALID_HANDLE != register_info_.handle)
        {
            ::xt_sip_client_register_handle_delete(register_info_.handle);
           register_info_.handle = XT_SIP_INVALID_HANDLE;
        }

        register_info_.handle = ::xt_sip_client_register_handle_clone(handle);
    }

private:
    bool is_exist_sip_session(const session_info_t &session);
    void add_sip_session(const session_info_t &session);
    void del_sip_session(std::string call_id);
    int find_sip_session(std::string call_id, session_info_t &session);
    int find_sip_session_multicast(const track_info_t &track);
    int get_sip_session(session_info_t& out_session);
    int rtp_server_restart(void);
    int update_rtpport_mux(const char *sdp_rcv, uint32_t len_rcv, const char *sdp_snd, uint32_t len_snd, session_info_t &session);
    int sdp_parse(const char *sdp, uint32_t len, xt_sdp::sdp_session_t &xsdp);
    int video_bandwidth_set(xt_sdp::sdp_session_t &xsdp);
    int video_fmtp_set(xt_sdp::sdp_session_t &xsdp);
    int sdp_compare(const char *sdp_rcv, uint32_t len_rcv, const char *sdp_snd, uint32_t len_snd, std::string &sdp, session_info_t &session);
    bool medium_compare(sdp_session_t::medium_t &medium_recv, sdp_session_t::medium_t &medium_snd, int &payload);
    std::string get_medium_desc(std::string format, sdp_session_t::medium_t &medium);
    void update_medium_desc_recv(sdp_session_t::medium_t &medium, std::string format1, std::string format2);
    void update_medium_desc_snd(sdp_session_t::medium_t &medium, std::string format1, std::string format2);
    bool desc_compare(const std::string& desc1,const std::string& desc2);
    const std::string channel_type_to_str(const uint32_t& chtype) const;
	int check_pri(std::string res_pri);

	xt_sip_status_t ptzctrl(xtXml &xml, xtXmlNodePtr &root);
	xt_sip_status_t pointindexoperation(xtXml &xml, xtXmlNodePtr &root);
	xt_sip_status_t iframeconfreq(dev_flag_t *dev_flag, sipmsg_ifrmconfreq &freq, std::string &respones_ctx);
	xt_sip_status_t coderate(dev_flag_t *dev_flag, int strmid, const char *coderate, std::string &respones_ctx);
	xt_sip_status_t resolution(dev_flag_t *dev_flag, int strmid, const char *resolution, std::string &respones_ctx);
	xt_sip_status_t getdeviceinforeq(dev_flag_t *dev_flag, std::string &respones_ctx);
	xt_sip_status_t sessioninforeq(dev_flag_t *dev_flag, std::string &respones_ctx);

    typedef struct _strcut_rtpmap_value_type_
    {
        unsigned rtpmap_payload_format;
        char codec_name[128];
        unsigned rtp_timestamp_frequency;
        unsigned num_channels;
        _strcut_rtpmap_value_type_():
        rtpmap_payload_format(96),
            rtp_timestamp_frequency(0),
            num_channels(1)
        {
            ::memset(codec_name,0,128);
        }
    }rtpmap_value_t,*prtpmap_value_t;
   const bool parse_sdp_attribute_rtpmap_value (char const* sdp_rtpmap_value,rtpmap_value_t& out) const;
   const bool rtpmap_compare(sdp_session_t::medium_t &medium_recv, sdp_session_t::medium_t &medium_snd, int &payload) const;

private:
    boost::atomic_bool start_;

    //sip ���
    xt_sip_handle_t sip_;

    char sdp_[NUM_STREAM][LEN_SDP];
	char sdp_full_[NUM_STREAM][LEN_SDP];

    //ע����Ϣ��¼
    register_info_t register_info_;

    boost::shared_mutex	 session_info_mutex_; //session_infos_ ����Դ��
    std::map<std::string,session_info_t> session_infos_;

public:

    //֪ͨע�����
    void notify_regist_removed();

    //�ȴ�ע�����
    void wait_regist_removed(uint32_t outtime=10);
private:
    boost::atomic_bool regist_status_;
    boost::recursive_mutex regist_removed_mutex_;//ע������������
    boost::condition_variable_any regist_removed_condition_; //ע���ɹ�����֪ͨ

public:
    sipmsg_ptz_cb ptz_cb_;
    sipmsg_picupdate_cb picupdate_cb_;
    sipmsg_cb     message_cb_;
    sipinfo_cb     info_cb_;
    sipmsg_ifrmconfreq_cb igrmconfreq_cb_;
    sip_video_adjust_cb video_adjust_cb_;
    sip_get_dev_info_cb get_dev_info_cb_;  
    sip_point_index_operation_cb point_index_operation_cb_;
    sip_register_srv_ret_info_cb register_srv_ret_cb_;
    sipmsg_bandwidth_cb sipmsg_bandwidth_cb_;
    sipmsg_profile_cb sipmsg_profile_cb_;
};

#endif	//_H_H_XT_SIP_
