#ifndef _ROUTER_TASK_H_INCLUDED
#define _ROUTER_TASK_H_INCLUDED

#include "framework/task.h"
#include "framework/event_context.h"
#include "common_type.h" 
#include "XTEngine.h"
#include "center_common_types.h"

//处理来自jk.ocx的任务，每种指令一个任务
class router_task_request_mgr
{
public:
    static void start_play(const std::string& ondb_sn,const std::string& ids, long chanid, long strmtype, 
		const std::string& localip,
		const std::string& db_url, long db_chanid, long db_type, long chanid2, 
        long link_type,const std::string login_name ="admin" , 
        const std::string login_password ="12345", const long login_port = -1);

    static void update_ids(const std::string& ids, long chanid, long strmtype, 
        const std::string& new_ids, long new_chaid);
    static void stop_play(const std::string& ondb_sn,const std::string& ids, long chanid, long strmtype);
    static void stop_play(const std::string& ids);
    static void stop_play(const long chanid);
    static void stop_all();
    static void device_logout_pro(const std::string& ids);

    //static void error_to_center(const std::string& ondb_sn,const std::string&ids, long chanid, long stream_type, long ctl_id, long err_id);
    static void reponse_play_fail_to_center(const std::string& ondb_sn,const std::string&ids, long chanid, long stream_type);
    static void reponse_stop_reuslt_to_center(const std::string& ondb_sn,const std::string&ids, long chanid, long stream_type);
    static void response_to_center(const std::string& ondb_sn,const dev_handle_t dev_handle,const int srcno,const std::string&ids, long chanid,long stream_type, 
        long server_id, const std::string&ip, long playid, long dvrtype, long link_type, long server_type,const long ctl_id = OP_PLAY);
    static void response_to_center_v1(const std::string& ondb_sn,const dev_handle_t dev_handle,const int srcno,const std::string&ids, long chanid,long stream_type, 
        long transmit_ch, const std::string&ip, long playid, long dvrtype, long link_type, long server_type);

    static int regist(const std::string ids, const std::string local_ip, unsigned short local_port,
        const std::string server_ip, unsigned short server_port);

    static void transparent_cmd_pro(const std::string& ids, const std::string& cmd);

    //日志入口
    static void set_log(void (*)(const char *));
    static void log(const char *fmt, ...);

    static void gw_join_sip_respond_fail_to_gateway(const long sessionid,const std::string& ids,const std::string &guid,int errocode,const std::string& errorreasion);
    static void gw_join_sip_respond_success_to_gateway(const long sessionid,const std::string& ids,const std::string &guid);

    static void gw_join_sip_respond_trans_sdp_to_gateway(const long sessionid,const std::string& gateway_ids,const std::string &guid,const std::string& sdp); 

};

//JK任务
/////////////////////////////////////////////////////////////////////////////////////////////////
class start_play_router_task : public framework::task_base, public framework::event_context
{
public:
    start_play_router_task(const std::string& ondb_sn,const std::string& ids, long chanid,
        long strmtype, 
		const std::string& localip,
		const std::string& db_url,long db_chanid,long db_type, long chanid2, long link_type,
        const std::string&login_name, const std::string&login_password, const long login_port)
        :ids_(ids),
        chanid_(chanid),
        strmtype_(strmtype),
		localip_(localip),
        db_url_(db_url),
        db_chanid_(db_chanid),
        db_type_(db_type),
        chanid2_(chanid2),
        link_type_(link_type),
        ondb_sn_(ondb_sn),
        login_name_(login_name),
        login_password_(login_password),
        login_port_(login_port)

    {}

    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

private:
    std::string ids_;
    long chanid_;
    long strmtype_;
	std::string localip_;
    std::string db_url_;
    long db_chanid_;
    long db_type_;
    long link_type_;
    long chanid2_;
    std::string ondb_sn_;

    std::string login_name_;
    std::string login_password_; 
    long login_port_;
};

class update_ids_router_task : public framework::task_base, public framework::event_context
{
public:
    update_ids_router_task(const std::string& ids, long chanid, long strmtype, const std::string& new_ids, long new_chaid)
        :ids_(ids),
        chanid_(chanid),
        strmtype_(strmtype),
        new_ids_(new_ids),
        new_chanid_(new_chaid)
    {}

    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

private:
    std::string ids_;
    long chanid_;
    long strmtype_;
    std::string new_ids_;
    long new_chanid_;
};

class stop_play_router_task : public framework::task_base, public framework::event_context
{
public:
    stop_play_router_task(const std::string& ondb_sn,const std::string& ids, long chanid, long type)
        :ondb_sn_(ondb_sn),
        ids_(ids),
        chanid_(chanid),
        stream_type_(type)
    {}

    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

private:
    std::string ondb_sn_;
    std::string ids_;
    long chanid_;
    long stream_type_;
};

class stop_play_router_task2 : public framework::task_base, public framework::event_context
{
public:
    stop_play_router_task2(const std::string& ids)
        :ids_(ids)
    {}

    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

private:
    std::string ids_;
};

class stop_play_router_task3 : public framework::task_base, public framework::event_context
{
public:
    stop_play_router_task3(long chanid)
        :chanid_(chanid)
    {}

    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

private:
    long chanid_;
};

class stop_play_router_by_strmid_task : public framework::task_base, public framework::event_context
{
public:
    stop_play_router_by_strmid_task(const long strmid)
        :strmid_(strmid)
    {}

    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

private:
    long strmid_;
};

class stop_all_router_task : public framework::task_base, public framework::event_context
{
public:
    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }
};

class create_transmit_src_task : public framework::task_base ,public framework::event_context
{
public:
    create_transmit_src_task(const std::string& ondb_sn,const device_info& device):
      ondb_sn_(ondb_sn),device_(device),count_(0){}

      uint32_t run();

      //并发反馈
      void process_event() { signal(deferred, arbitrarily_free_work_thread); }

private:
    device_info device_;
    std::string ondb_sn_;
    uint32_t count_;
};

class result_to_center_router_v1_task : public framework::task_base, public framework::event_context
{
public:
    result_to_center_router_v1_task(const std::string& ondb_sn,const dev_handle_t dev_handle,const int srcno,const std::string&ids, long chanid, long stream_type, long transmit_ch, long result_type, 
        const std::string& other_info, const std::string&ip, long playid, long dvrtype, long link_type, 
        long ctl_id, long err_id)
        :ondb_sn_(ondb_sn),
        dev_handle_(dev_handle),
        srcno_(srcno),
        ids_(ids),
        chanid_(chanid),
        stream_type_(stream_type),
        transmit_ch_(transmit_ch),
        result_type_(result_type),
        other_info_(other_info),
        ip_(ip),
        playid_(playid),
        dvrtype_(dvrtype),
        link_type_(link_type),
        ctl_id_(ctl_id),
        err_id_(err_id)
    {}

    uint32_t run();
    //并发反馈
    void process_event() { signal(deferred, arbitrarily_free_work_thread); }

private:
    std::string ondb_sn_;
    std::string ids_;
    std::string other_info_;
    std::string ip_;
    long chanid_;
    long transmit_ch_;
    long result_type_;
    long playid_;
    long dvrtype_;
    long link_type_;
    long err_id_;
    long ctl_id_;
    long stream_type_;
    dev_handle_t dev_handle_;
    int srcno_;
};

class result_to_center_router_task : public framework::task_base, public framework::event_context
{
public:
    result_to_center_router_task(const std::string& ondb_sn,const dev_handle_t dev_handle,const int srcno,const std::string&ids, long chanid, long stream_type, long server_id, long result_type, 
        const std::string& other_info, const std::string&ip, long playid, long dvrtype, long link_type, 
        long ctl_id, long err_id)
        :ondb_sn_(ondb_sn),
        dev_handle_(dev_handle),
        srcno_(srcno),
        ids_(ids),
        chanid_(chanid),
        stream_type_(stream_type),
        server_id_(server_id),
        result_type_(result_type),
        other_info_(other_info),
        ip_(ip),
        playid_(playid),
        dvrtype_(dvrtype),
        link_type_(link_type),
        ctl_id_(ctl_id),
        err_id_(err_id),
        count_(0)
    {}

    uint32_t run();
    //并发反馈
    void process_event() { signal(deferred, arbitrarily_free_work_thread); }

private:
    std::string ondb_sn_;
    std::string ids_;
    std::string other_info_;
    std::string ip_;
    long chanid_;
    long server_id_;
    long result_type_;
    long playid_;
    long dvrtype_;
    long link_type_;
    long err_id_;
    long ctl_id_;
    long stream_type_;
    dev_handle_t dev_handle_;
    int srcno_;
    uint32_t count_;
};

class regist_task : public framework::task_base, public framework::event_context
{
public:
    regist_task(const std::string ids_, const std::string local_ip_, unsigned short local_port_,
        const std::string server_ip_, unsigned short server_port_)
        :ids(ids_),
        local_ip(local_ip_),
        local_port(local_port_),
        server_ip(server_ip_),
        server_port(server_port_)
    {}

    uint32_t run();
    void process_event() { signal(deferred, arbitrarily_free_work_thread); }

private:
    std::string ids;
    std::string local_ip;
    unsigned short local_port;
    std::string server_ip;
    unsigned short server_port;
};

class dev_logout_pro_task : public framework::task_base, public framework::event_context
{
public:
    dev_logout_pro_task(const std::string& ids)
        :ids_(ids)
    {}

    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

private:
    std::string ids_;
};

///////////////////////////////////////////////////////////////////////////////////////////////// 


//网关 sip 对接  add by songlei 20150518
/////////////////////////////////////////////////////////////////////////////////////////////////
class gw_join_sip_parse_join_gateway_xml_cmd_task : public framework::task_base, public framework::event_context
{
public:
    gw_join_sip_parse_join_gateway_xml_cmd_task(const std::string& ids, const std::string& cmd)
        :ids_(ids),cmd_(cmd){}
    uint32_t run();
    void process_event() { signal(deferred, center_cmd_xml_parse_pro_thread); }
private:
    std::string ids_;
    std::string cmd_;

};
class gw_join_sip_play_pro_task : public framework::task_base, public framework::event_context
{
public:
    gw_join_sip_play_pro_task(const sip_play_msg& msg):
      msg_(msg),sdp_()
      {}
      uint32_t run();
      void process_event() { signal(deferred, recv_center_cmd_pro_thread); }
private:
    sip_play_msg msg_;
    std::string sdp_;//交换生成的sdp
};

class gw_join_sip_sdp_pro_task :public framework::task_base,public framework::event_context
{
public:
    gw_join_sip_sdp_pro_task(sip_sdp_msg& msg):
      msg_(msg)
      {}

      uint32_t run();
      void process_event() { signal(deferred, recv_center_cmd_pro_thread); }
private:
    sip_sdp_msg msg_;
};

class gw_join_sip_close_recv_task :public framework::task_base,public framework::event_context
{
public:
    gw_join_sip_close_recv_task(long sessionid,std::string& sendids):
      sessionid_(sessionid),sendids_(sendids)
      {}

      uint32_t run();
      void process_event() { signal(deferred, recv_center_cmd_pro_thread); }
private:
    long sessionid_;
    std::string sendids_;

};


class gw_join_sip_add_send_task :public framework::task_base,public framework::event_context
{
public:

    gw_join_sip_add_send_task(long sessionid,std::string& sendids,std::string &guid):
      sessionid_(sessionid),sendids_(sendids),guid_(guid)
      {}

      uint32_t run();
      void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

private:
    long sessionid_;
    std::string sendids_;
	std::string guid_;
};

class gw_join_sip_close_send_task :public framework::task_base,public framework::event_context
{
public:

    gw_join_sip_close_send_task(long sessionid,std::string& sendids):
      sessionid_(sessionid),sendids_(sendids)
      {}

      uint32_t run();
      void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

private:
    long sessionid_;
    std::string sendids_;
};

class gw_join_sip_clear_session_pro_task :public framework::task_base,public framework::event_context
{
public:
    gw_join_sip_clear_session_pro_task(long sessionid,std::string& sendids):
      sessionid_(sessionid),sendids_(sendids)
      {}

      uint32_t run();
      void process_event() { signal(deferred, recv_center_cmd_pro_thread); }
private:
    long sessionid_;
    std::string sendids_;
};

class gw_join_sip_clear_trans_ch_task :public framework::task_base,public framework::event_context
{
public:
    gw_join_sip_clear_trans_ch_task(long sessionid,std::string& sendids):
      sessionid_(sessionid),sendids_(sendids)
      {}

      uint32_t run();
      void process_event() { signal(deferred, recv_center_cmd_pro_thread); }
private:
    long sessionid_;
    std::string sendids_;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
#endif //_ROUTER_TASK_H_INCLUDED
