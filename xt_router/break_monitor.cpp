#include "break_monitor.h"
#include <boost/thread/lock_guard.hpp>

break_monitor_mgr break_monitor_mgr::self_;

long break_monitor_mgr::init(const long max_monitor_stream_num)
{
    boost::lock_guard<boost::detail::spinlock>lock(bm_mgr_lock_);
    try
    {
        bm_mgr_.assign(max_monitor_stream_num,false);
    }
    catch(...)
    {
        return -1;
    }
    return 1;
}

void break_monitor_mgr::update_stream_state(const long stremid,const stream_state_t state /*= true*/)
{
    boost::lock_guard<boost::detail::spinlock>lock(bm_mgr_lock_);
    if (stremid < 0) return;
    try
    {
        bm_mgr_[stremid] = state;
    }
    catch(...)
    {
    }
}

break_monitor_mgr::stream_state_t break_monitor_mgr::get_stream_state(const long stremid)
{
    boost::lock_guard<boost::detail::spinlock>lock(bm_mgr_lock_);
    if (stremid < 0) return true;
    stream_state_t ret = false;
    try
    {
        ret = bm_mgr_[stremid];
    }
    catch (...)
    {
    }
    return ret;
}