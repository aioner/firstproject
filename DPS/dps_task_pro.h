#ifndef DPS_TASK_PRO_H__
#define DPS_TASK_PRO_H__
#include "framework/task.h"
#include "framework/event_context.h"
#include "dps_ch_mgr.h"
#include "dps_device_access_mgr.h"
#include "dps_data_send_mgr.h"
#include "dps_common_type_def.h"

class dev_play_task : public framework::task_base_t, public framework::event_context_t
{
public:
    dev_play_task(const dps_dev_s_handle_t s_handle,const access_data_output_cb_t recv_data_cb,const send_data_cb_t send_data_cb)
        :s_handle_(s_handle),recv_data_cb_(recv_data_cb),send_data_cb_(send_data_cb),dev_handle_(DEV_HANDLE_NA){}
    uint32_t run();
    void process_event() {signal(deferred, dev_access_operator_thread);}

private:
    dps_dev_s_handle_t s_handle_;
    dev_handle_t dev_handle_;
    access_data_output_cb_t recv_data_cb_;
    send_data_cb_t send_data_cb_;
};

class create_src_task : public framework::task_base_t, public framework::event_context_t
{
public:
    create_src_task(const dps_dev_s_handle_t s_handle,const send_data_cb_t send_data_cb)
        :s_handle_(s_handle),try_count_(0),send_data_cb_(send_data_cb){}
    uint32_t run();
    void process_event() {signal(deferred,arbitrarily_free_work_thread);}
private:
    dps_dev_s_handle_t s_handle_;
    send_data_cb_t send_data_cb_;
    uint16_t try_count_;
};

class dev_stop_task : public framework::task_base_t, public framework::event_context_t
{
public:
    dev_stop_task(const dps_dev_s_handle_t s_handle):
      s_handle_(s_handle){}
    uint32_t run();
    void process_event(){signal(deferred,dev_access_operator_thread);}
private:
    dps_dev_s_handle_t s_handle_;
};


#endif //end #ifndef DPS_TASK_PRO_H__