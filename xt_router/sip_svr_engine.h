#ifndef STD_SIP_ENGINE2_H__
#define STD_SIP_ENGINE2_H__

#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "common_type.h"
#include "xt_sip_api.h"
#include "xt_sip_api_types.h"
#ifdef _WIN32
#pragma comment(lib,"xt_sip.lib")
#endif //#ifdef _WIN32

#include "sdp.h"
#include "parse_buffer.h"
#include "Router_config.h"
using namespace std;
typedef enum
{
	SIP_FOR_NA=-1,
	SIP_FOR_TCP=0,
	SIP_FOR_UDP=1,
	SIP_FOR_TCP_UDP=2
} sip_for_trans_protocol_t;

typedef enum
{
	SIP_STATUS_OK=0,
	SIP_STATUS_FAIL,
	SIP_STATUS_PROTOCOL_ERR,
	SIP_STATUS_USRE_IS_EMPTY,
	SIP_STATUS_PWD_IS_EMPTY,
	SIP_STATUS_DOMAIM_IS_EMPTY,
	SIP_STATUS_CREATE_FAIL,
	SIP_STATUS_PORT_IS_EMPTY,
	SIP_STATUS_MALLOC_MEM_FAIL,
	SIP_STATUS_TARGET_IS_EMPTY,
}std_sip_status;


typedef struct __struct_node_info_type__
{
	string ids;
	string ip;
	string port;
}node_info_t,*ptr_node_info_t;

typedef struct __struct_sip_cfg_info_type__
{
	//create cfg
	uint32_t session_keep_alive_time_interval; //�Ự����ʱ�䣬��λ���� [90, 180]�� ����UPDATE
	uint32_t registration_retry_time_interval; //ע������ʱ��
	sip_for_trans_protocol_t protocol;   //����Э�� ��sip_trans_protocol_t ����"|"����������п���ֻʹ��tcp��udp�����ͬʱʹ��
	uint32_t transport;  //sip ����˿�
	uint16_t tls_port;  //�����tls�˿ڣ���0��ʾ������
	uint16_t dtls_port;  //�����dtls�˿ڣ���0��ʾ������

	//register cfg
	uint32_t  expires;    //��������˼ 1���������������ʱ�� 2��REGIST ˢ��ʱ�� [60, 3600]��
	uint32_t  link_keep_alive_time_interval; //��·����ʱ�� ��λ����  ��OPTION��ʱ�� [25000, 90000]ms
	uint32_t  regist_retry_time_interval; //REGIST ʧ������ʱ�� [60,3600]s

	uint32_t channle_type;                  //�ŵ����� ��ֵ��channel_t ������õ��п�����һ������Ҳ�����Ƕ�������
	uint32_t net_delay;                 //��ʱ
	uint32_t net_packetloss;            //������
	uint32_t net_mtu;                   //����䵥Ԫ Ĭ�ϣ�1500
	uint32_t net_bandwidth;             //����
	std::string main_domain;            //����

	typedef std::list<std::string> back_domain_t;
	typedef back_domain_t::iterator back_domain_handle_t;
	back_domain_t back_domains; //����
	std::string usre;
	std::string pwd;
}sip_cfg_info_t,*ptr_sip_cfg_info_t;

typedef enum
{
	SIP_INVITE=0,
	SIP_REINVITE=1,
}std_sip_session_type_t;

typedef enum
{
	SIP_INVITE_SERVER=0,
	SIP_INVITE_CLIENT=1,
	SIP_INVITE_CANCEL=2
}std_sip_invite_type_t;

typedef struct _struct_std_sip_session_type_
{
	std_sip_invite_type_t invite_type;//��ǻỰ��invite�����ǽ���
	string recv_src_callid;//���λỰ��������һ��Դ�����������'Dialog'�Ĺ�ϵ
	vector<string> trans_callid_container;//���λỰ��ΪԴ���ظ��㲥��Ŀ�ֻ꣬���ظ��㲥��ʹ��
	xt_sip_client_invite_handle_t hclient;
	xt_sip_server_invite_handle_t hserver;
	
	string router_sdp;//���������ü���sdp
	string ids_to;//���λỰ��Ϊת��Դʱ��ids

	std::string call_id;
	std::string from;//invite����ķ�����
	std::string to;  //invite����Ľ�����
	std::string sdp; //���յ�sdp
	uint32_t sdp_len;
	long srcno;
	long dev_handle;

	boost::posix_time::ptime creat_time;

	typedef _struct_std_sip_session_type_ my_t;
	my_t& operator=(const my_t& rf)
	{
		this->invite_type = rf.invite_type;
		this->router_sdp = rf.router_sdp;

		this->ids_to = rf.ids_to;
		this->recv_src_callid = rf.recv_src_callid;
		this->hserver = rf.hserver;
		this->hclient = rf.hclient;

		this->trans_callid_container = rf.trans_callid_container;
		this->srcno = rf.srcno;
		this->dev_handle = rf.dev_handle;

		this->call_id = rf.call_id;
		this->from = rf.from;
		this->to = rf.to;
		this->sdp = rf.sdp;
		this->sdp_len = rf.sdp_len;

		this->creat_time = rf.creat_time;

		return *this;
	}
}std_sip_session_t,*ptr_std_sip_session_t;

class sip_svr_engine : boost::noncopyable
{
public:
	static sip_svr_engine* instance(){ return &self_;};
	static sip_svr_engine* _(){return &self_;};
public:
	int std_sip_start();
	int std_sip_stop();

	sdp_direction_t parse_sdp_direction(const std::string& sdp);

	long parse_call_id_by_msg(const xt_sip_msg_handle_t msg,std::string& call_id);
	long parse_from_by_msg(const xt_sip_msg_handle_t msg,std::string& from);
	long parse_to_by_msg(const xt_sip_msg_handle_t msg,std::string& to);
	long parse_content_body_by_msg(const xt_sip_msg_handle_t msg,std::string& ctx);

	void response_optstatus_ok();
	void response_optstatus_fail(const std::string& fail_case);
	void response_chanstatus(char* ch_status);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//sip �����ӿڷ�װ
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void add_options(const char* target);
	void del_options(const char* target);
	//���ܵ��Ƚӿ�
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	long add_send_dst(const int srcno,const std::string& dst_sdp);
	long del_send_dst(const int srcno,const std::string& dst_sdp);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	std_sip_status load_cfg();
	std_sip_status start();
	void stop();
protected:
	sip_svr_engine();
	~sip_svr_engine();
	static sip_svr_engine self_;

	//���� server����������Ϣ
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
	//�յ�INVITE �� sdp
	static void on_sip_server_invite_with_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len);

	//�յ�INVITE ����SDP
	static void on_sip_server_invite_no_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);

	//�յ�ACK �� sdp
	static void on_sip_server_invite_ack_with_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len);

	//�յ�ACK ���� sdp
	static void on_sip_server_invite_ack_no_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);

	//�յ�BYE
	static void on_sip_server_invite_bye(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, xt_sip_invite_terminated_reason_t reason);

	//�յ��Ự�ڵ�INFO
	static void on_sip_server_invite_info(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);

	//���ͻỰ�ڵ�INFO�������
	static void on_sip_server_invite_info_response(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);

	//�յ��Ự�ڵ�MESSAGE
	static void on_sip_server_invite_message(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);

	//���ͻỰ�ڵ�MESSAGE���������Ӧ
	static void on_sip_server_invite_message_response(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);

	//�յ��Ự���MESSAGE
	static void on_sip_server_message_arrived(void *ctx, xt_sip_server_message_handle_t h, xt_sip_msg_handle_t msg);

	//OPTIONʧ����Ӧ
	static int8_t on_xt_sip_heartbeat_not_pong_cb(void *ctx, const char *target,uint32_t count);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//����client����������Ϣ
public:
	static void on_client_invite_failure(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg);
	static void on_client_invite_200ok(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len);
	static void on_client_invite_offer(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len);
	static void on_client_invite_bye(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, xt_sip_invite_terminated_reason_t reason);
	static void on_client_invite_info(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg);
	static void on_client_invite_info_response(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);
	static void on_client_invite_message(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg);
	static void on_client_invite_message_response(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);
	static void on_client_msg_prev_post(void *ctx, xt_sip_msg_handle_t msg);//Ԥ����invite��Ϣ����
	//ע����Ϣ����
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//ע����Ϣ��¼
private:
	//������Ϣ
	sip_cfg_info_t cfg_info_;
public:
	//sip ���
	xt_sip_handle_t sip_hanle_;
public:
	//1.����uri�б�ids��ip��port
	//2.��ѯuri������
	//3.���챾��uri���Ա���ids��Ϊ��Ե㣬Ѱ����һ��
	string get_local_uri()
	{
		return m_local_uri;
	}
	long find_next_ids(string from_ids,string local_ids,const string dst_ids,std::string &next_uri);
	long load_nodelist();
	long parse_uri(const string uri,node_info_t &node_i);
	list< vector<node_info_t> > m_node_list;
	string m_local_uri;
	//����ģ��sdpý��������������м�sdp������Ϣ
	long combine_c_to_media(const string sdp_m, const string sdp_c,string &dst_sdp);
	//����src sdp�ķ���ü�һ����sdp
	long one_part_sdp(const string src_sdp, string &part_sdp,bool remove_same_dir);
	long push_media_streams(std_sip_session_t src_session, std_sip_session_t dst_session);
	long parse_msg_info(xt_sip_msg_handle_t msg,string &callid,string &from,string &to);
	static xt_sip_client_invite_callback_t client_invite_cb;
	xt_sip_client_invite_callback_t get_client_invite_cb()
	{
		return client_invite_cb;
	};
public:
	//option����
	//�Ự����
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
	typedef std::map<std::string,std_sip_session_t>  sip_session_mgr_container_t;
	typedef std::map<std::string,std_sip_session_t>::iterator sip_session_mgr_container_handle_t;

	void add_session(const std_sip_session_t& session);
	void del_session(const std_sip_session_t& session);
	void del_session(const std::string& call_id);
	long end_session(std_sip_session_t session);
	long get_session(const std::string& call_id,std_sip_session_t& session);
	long get_session_ids(std::string ids,std_sip_session_t &session);
	long exist_session(const std_sip_session_t session,string &org_callid);
	long update_session(const std_sip_session_t& session);

	std_sip_session_type_t get_session_type(const std::string& call_id);
private:
	boost::shared_mutex session_mgr_mutex_;
	sip_session_mgr_container_t std_sip_session_mgr_lst;
public:
	std::map<long,std_sip_session_t> m_tmp_session_call;
	static long id_;
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
};
#endif //#ifndef STD_SIP_ENGINE_H__
