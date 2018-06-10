#ifndef BREAK_MONITOR_H__
#define BREAK_MONITOR_H__
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <vector>

class break_monitor_mgr : boost::noncopyable
{
protected:
    break_monitor_mgr():bm_mgr_(){}
public:
    typedef bool stream_state_t;
    static break_monitor_mgr*_(){return &self_;}
    long init(const long max_monitor_stream_num);
    void update_stream_state(const long stremid,const stream_state_t state = true);
    stream_state_t get_stream_state(const long stremid);
private:
    boost::detail::spinlock bm_mgr_lock_;
    std::vector<stream_state_t> bm_mgr_;
    static break_monitor_mgr self_;
};
#endif //#ifndef BREAK_MONITOR_H__
