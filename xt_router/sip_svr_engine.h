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
	uint32_t session_keep_alive_time_interval; //会话保活时间，单位：秒 [90, 180]秒 发送UPDATE
	uint32_t registration_retry_time_interval; //注册重试时间
	sip_for_trans_protocol_t protocol;   //传输协议 由sip_trans_protocol_t 进行"|"或运算得来有可能只使用tcp或udp与可能同时使用
	uint32_t transport;  //sip 传输端口
	uint16_t tls_port;  //传输层tls端口，填0表示不开启
	uint16_t dtls_port;  //传输层dtls端口，填0表示不开启

	//register cfg
	uint32_t  expires;    //有两个意思 1、代理服务器过期时间 2、REGIST 刷新时间 [60, 3600]秒
	uint32_t  link_keep_alive_time_interval; //链路保活时间 单位毫秒  发OPTION的时间 [25000, 90000]ms
	uint32_t  regist_retry_time_interval; //REGIST 失败重试时间 [60,3600]s

	uint32_t channle_type;                  //信道类型 此值由channel_t 或运算得到有可能是一种类型也可能是多种类型
	uint32_t net_delay;                 //延时
	uint32_t net_packetloss;            //丢包率
	uint32_t net_mtu;                   //最大传输单元 默认：1500
	uint32_t net_bandwidth;             //带宽
	std::string main_domain;            //主域

	typedef std::list<std::string> back_domain_t;
	typedef back_domain_t::iterator back_domain_handle_t;
	back_domain_t back_domains; //备域
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
	std_sip_invite_type_t invite_type;//标记会话是invite发起还是接受
	string recv_src_callid;//本次会话关联的另一个源，常规情况是'Dialog'的关系
	vector<string> trans_callid_container;//本次会话作为源，重复点播的目标，只有重复点播才使用
	xt_sip_client_invite_handle_t hclient;
	xt_sip_server_invite_handle_t hserver;
	
	string router_sdp;//交换产生裁剪后sdp
	string ids_to;//本次会话作为转发源时的ids

	std::string call_id;
	std::string from;//invite请求的发起者
	std::string to;  //invite请求的接收者
	std::string sdp; //接收到sdp
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
	//sip 操作接口封装
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void add_options(const char* target);
	void del_options(const char* target);
	//功能调度接口
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

	//处理 server发过到的消息
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
	//收到INVITE 带 sdp
	static void on_sip_server_invite_with_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len);

	//收到INVITE 不带SDP
	static void on_sip_server_invite_no_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);

	//收到ACK 带 sdp
	static void on_sip_server_invite_ack_with_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len);

	//收到ACK 不带 sdp
	static void on_sip_server_invite_ack_no_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);

	//收到BYE
	static void on_sip_server_invite_bye(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, xt_sip_invite_terminated_reason_t reason);

	//收到会话内的INFO
	static void on_sip_server_invite_info(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);

	//发送会话内的INFO操作结果
	static void on_sip_server_invite_info_response(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);

	//收到会话内的MESSAGE
	static void on_sip_server_invite_message(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg);

	//发送会话内的MESSAGE操作结果响应
	static void on_sip_server_invite_message_response(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);

	//收到会话外的MESSAGE
	static void on_sip_server_message_arrived(void *ctx, xt_sip_server_message_handle_t h, xt_sip_msg_handle_t msg);

	//OPTION失败响应
	static int8_t on_xt_sip_heartbeat_not_pong_cb(void *ctx, const char *target,uint32_t count);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//处理client发过来的消息
public:
	static void on_client_invite_failure(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg);
	static void on_client_invite_200ok(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len);
	static void on_client_invite_offer(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len);
	static void on_client_invite_bye(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, xt_sip_invite_terminated_reason_t reason);
	static void on_client_invite_info(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg);
	static void on_client_invite_info_response(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);
	static void on_client_invite_message(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg);
	static void on_client_invite_message_response(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success);
	static void on_client_msg_prev_post(void *ctx, xt_sip_msg_handle_t msg);//预发送invite消息处理
	//注册信息管理
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//注册信息记录
private:
	//配置信息
	sip_cfg_info_t cfg_info_;
public:
	//sip 句柄
	xt_sip_handle_t sip_hanle_;
public:
	//1.加载uri列表，ids，ip，port
	//2.查询uri数据项
	//3.构造本机uri，以本机ids作为相对点，寻找下一个
	string get_local_uri()
	{
		return m_local_uri;
	}
	long find_next_ids(string from_ids,string local_ids,const string dst_ids,std::string &next_uri);
	long load_nodelist();
	long parse_uri(const string uri,node_info_t &node_i);
	list< vector<node_info_t> > m_node_list;
	string m_local_uri;
	//保留模板sdp媒体能力级，组合中间sdp连接信息
	long combine_c_to_media(const string sdp_m, const string sdp_c,string &dst_sdp);
	//根据src sdp的方向裁剪一部分sdp
	long one_part_sdp(const string src_sdp, string &part_sdp,bool remove_same_dir);
	long push_media_streams(std_sip_session_t src_session, std_sip_session_t dst_session);
	long parse_msg_info(xt_sip_msg_handle_t msg,string &callid,string &from,string &to);
	static xt_sip_client_invite_callback_t client_invite_cb;
	xt_sip_client_invite_callback_t get_client_invite_cb()
	{
		return client_invite_cb;
	};
public:
	//option心跳
	//会话管理
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
