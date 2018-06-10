#ifndef XTCHAN_H__INCLUDE__
#define XTCHAN_H__INCLUDE__

#include <map>
#include <boost/thread/recursive_mutex.hpp>
using namespace std;

// 通道信息
struct  xt_chan
{
    bool			active;		// 是否激活
    int				type;		// 0:私有 1:标准 2:单一转发
    unsigned int	sink;		// 转发数
};

class XTChan
{
private:
    XTChan(void);
    ~XTChan(void);
    static XTChan self;
public:
    static XTChan* instance(){return &self;}
    static XTChan* _(){return &self;}

    bool is_chan_used(const unsigned long chaid);

    // 增加通道
    int add_chan(unsigned long chanid, int num = 1);

    // 激活通道
    int active_chan(unsigned long chanid, bool active);

    // 增加/删除转发
    int add_sink(unsigned long chanid);
    int del_sink(unsigned long chanid);
    // 设置标准通道
    int set_stdchan(unsigned long chanid);

    // 设置备用通道
    int set_candchan(unsigned long chanid);
    // 获得空闲通道
    int get_freechan(unsigned long &chanid);
    int get_freestdchan(unsigned long &chanid);
    int get_freecandchan(unsigned long &chanid);

    // 获得通道信息
    int get_chan_info(unsigned long chanid, xt_chan &chan);

private:
    boost::recursive_mutex		m_mutex;		//mutex
    map<unsigned long, xt_chan>		m_chans;		//通道信息
};
#endif//XTCHAN_H__INCLUDE__
