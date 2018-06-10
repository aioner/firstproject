//
//create by songlei 20160316
//
#ifndef DPS_STREAM_MONITOR_H__
#define DPS_STREAM_MONITOR_H__

#include "framework/task.h"
#include "framework/event_context.h"
#include "dps_ch_mgr.h"

class dps_replay_task;
class dps_replay_mgr : boost::noncopyable
{
    dps_replay_mgr():is_init_(false),ptr_task_(NULL),replay_time_interval_(100){}
public:
    typedef std::vector<dps_dev_s_handle_t> replay_queue_container_t;
    typedef replay_queue_container_t::iterator replay_queue_container_itr_t;
public:
    static dps_replay_mgr*_(){return &my_;}
    void init();
    void uninit();
    void replay();
    void post(const dps_dev_s_handle_t s_handle);
    const uint32_t& get_replay_time_interval()const 
    {return replay_time_interval_; }

protected:
    bool is_exist(const dps_dev_s_handle_t s_handle);
    void push_back(const dps_dev_s_handle_t s_handle);

private:
    static dps_replay_mgr my_;

    boost::atomic_bool is_init_;
    boost::detail::spinlock mutex_replay_queue_;
    replay_queue_container_t replay_queue_;
    dps_replay_task* ptr_task_;
    uint32_t replay_time_interval_;
};

class dps_replay_task : public framework::task_base_t
{
public:
    dps_replay_task(){}
    uint32_t run();
};

class dps_break_monitor_task;
class dps_break_monitor_mgr : boost::noncopyable
{
    dps_break_monitor_mgr():is_init_(false),ptr_task_(NULL),break_monitor_time_interval_(30000) {}
public:
    typedef std::vector<dps_dev_s_handle_t> monitor_queue_container_t;
    typedef monitor_queue_container_t::iterator monitor_queue_container_itr_t;
public:
    static dps_break_monitor_mgr* _(){return &my_;}
    void init();
    void uninit();
    void monitor();
    void post(const dps_dev_s_handle_t s_handle);
    const uint32_t &get_break_monitor_time_interval() const
    {  return break_monitor_time_interval_; }

protected:
    bool is_exist(const dps_dev_s_handle_t s_handle);
    void push_back(const dps_dev_s_handle_t s_handle);
private:
    static dps_break_monitor_mgr my_;
    boost::atomic_bool is_init_;
    dps_break_monitor_task* ptr_task_;
    boost::detail::spinlock mutex_monitor_queue_;
    monitor_queue_container_t monitor_queue_;
    uint32_t break_monitor_time_interval_;

};

class dps_break_monitor_task : public framework::task_base_t
{
public:
    dps_break_monitor_task(){}
    uint32_t run();
};


#endif // #ifndef DPS_STREAM_MONITOR_H__
