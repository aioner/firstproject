//
//create by songlei 20160316
//
#include "dps_stream_monitor.h"
#include "dps_core_dispatch.h"
#include "dps_cfg_mgr.h"

dps_replay_mgr dps_replay_mgr::my_;

void dps_replay_mgr::replay()
{
    if (!is_init_) return;
    boost::lock_guard<boost::detail::spinlock> lock(mutex_replay_queue_);
    replay_queue_container_itr_t itr = replay_queue_.begin();
    while(replay_queue_.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        itr = replay_queue_.erase(itr);
        dps_core_dispatch::_()->play_asyn(s_handle);
    }
}

bool dps_replay_mgr::is_exist(const dps_dev_s_handle_t s_handle)
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_replay_queue_);
    replay_queue_container_itr_t itr = replay_queue_.begin();
    for (;replay_queue_.end() != itr; ++itr)
    {
        if (s_handle == *itr)
        {
            return true;
        }
    }
    return false;
}

void dps_replay_mgr::push_back(const dps_dev_s_handle_t s_handle)
{
    boost::lock_guard<boost::detail::spinlock>  lock(mutex_replay_queue_);
    replay_queue_.push_back(s_handle);
}

void dps_replay_mgr::post(const dps_dev_s_handle_t s_handle)
{
    if (!is_init_) return;
    if (!is_exist(s_handle))
    {
        push_back(s_handle);
    }
}

void dps_replay_mgr::init()
{
    if (is_init_) return;
    replay_time_interval_ = dps_cfg_mgr::_()->replay_time_interval(100);
    ptr_task_ = new dps_replay_task();
    if (NULL != ptr_task_)
    {
        replay_queue_.clear();
        ptr_task_->signal();
        is_init_ = true;
    }
}

void dps_replay_mgr::uninit()
{
    if (!is_init_) return;
    if (NULL != ptr_task_)
    {
        ptr_task_->cancel();
    }
}

uint32_t dps_replay_task::run()
{
    dps_replay_mgr::_()->replay();

    return dps_replay_mgr::_()->get_replay_time_interval();
}

dps_break_monitor_mgr dps_break_monitor_mgr::my_;

void dps_break_monitor_mgr::init()
{
    if (is_init_) return;

    break_monitor_time_interval_ = dps_cfg_mgr::_()->break_monitor_time_interval(30000);

    ptr_task_ = new dps_break_monitor_task();
    if (NULL != ptr_task_)
    {
        monitor_queue_.clear();
        ptr_task_->signal();
        is_init_ = true;
    }

}

void dps_break_monitor_mgr::uninit()
{
    if (!is_init_) return;
    if (NULL != ptr_task_)
    {
        ptr_task_->cancel();
    }
}

void dps_break_monitor_mgr::monitor()
{
    if (!is_init_) return;

    boost::lock_guard<boost::detail::spinlock> lock(mutex_monitor_queue_);
    monitor_queue_container_itr_t itr = monitor_queue_.begin();
    while (monitor_queue_.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        if ( s_handle->is_open() && !s_handle->get_recv_data_refurbish_state())
        {
            s_handle->stop_capture();
            dps_replay_mgr::_()->post(s_handle);
        }
        else
        {
            s_handle->recv_data_refurbish(false);
        }
        ++itr;
    }
}

bool dps_break_monitor_mgr::is_exist(const dps_dev_s_handle_t s_handle)
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_monitor_queue_);
    monitor_queue_container_itr_t itr = monitor_queue_.begin();
    for (;monitor_queue_.end() != itr; ++itr)
    {
        if (s_handle == *itr)
        {
            return true;
        }
    }
    return false;
}

void dps_break_monitor_mgr::post(const dps_dev_s_handle_t s_handle)
{
    if (!is_init_) return;
    if (!is_exist(s_handle))
    {
        push_back(s_handle);
    }
}

void dps_break_monitor_mgr::push_back(const dps_dev_s_handle_t s_handle)
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_monitor_queue_);
    monitor_queue_.push_back(s_handle);
}

uint32_t dps_break_monitor_task::run()
{
    dps_break_monitor_mgr::_()->monitor();
    return dps_break_monitor_mgr::_()->get_break_monitor_time_interval();
}

