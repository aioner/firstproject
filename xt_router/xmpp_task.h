#ifndef __XMPP_TASK_H__
#define __XMPP_TASK_H__
#include "framework/task.h"
#include "framework/event_context.h"
#include "xmpp_type_def.h"
#include "XTEngine.h"

//XMPP调度部分
/////////////////////////////////////////////////////////////////////////////////////////////////
class xmpp_play_task : public framework::task_base, public framework::event_context
{
public:
    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

public:
    xmpp_play_task(const iq_type<play_requst_body>& iq_msg):
      iq_msg_(iq_msg),play_ip_(""),dev_ids_(""),play_ch_(-1),play_type_(-1),dev_chanid_(-1),
          link_type_(-1),dev_strmtype_(-1),transmit_channel_(-1),start_play_ret_(-1),get_sdp_count_(0),get_sdp_try_time_(10){}
private:
    //点播IQ_MSG
    iq_type<play_requst_body> iq_msg_;
    std::string play_ip_;
    std::string dev_ids_;
    long play_ch_;
    long play_type_;
    long dev_chanid_;
    long dev_strmtype_;
    long link_type_;
    long transmit_channel_;
    int start_play_ret_;
    uint32_t get_sdp_count_;
    uint32_t get_sdp_try_time_;
    src_info src_;
};

class xmpp_stop_play_task : public framework::task_base, public framework::event_context
{
public:	
    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }

public:
    xmpp_stop_play_task(const iq_type<stop_requst_body>& iq_msg):iq_msg_(iq_msg){}

private:

    //停点IQ_MSG
    iq_type<stop_requst_body> iq_msg_;
};

//反向注册
class xmpp_play_inform_request_task : public framework::task_base, public framework::event_context
{
public:
    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }
public:
    xmpp_play_inform_request_task(const iq_type<play_inform_request_body>& iq):iq_(iq){}
private:

    iq_type<play_inform_request_body> iq_;
};

class xmpp_play_stop_inform_request_task : public framework::task_base, public framework::event_context
{
public:
    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }
public:
    xmpp_play_stop_inform_request_task(const iq_type<stop_inform_request_body>& iq):iq_(iq){}
private:

    iq_type<stop_inform_request_body> iq_;
};

class xmpp_stop_all_task : public framework::task_base, public framework::event_context
{
public:
    xmpp_stop_all_task(){};
public:
    uint32_t run();
    void process_event() { signal(deferred, recv_center_cmd_pro_thread); }
};
/////////////////////////////////////////////////////////////////////////////////////////////////


#endif //#ifndef __XMPP_TASK_H__
