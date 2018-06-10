#ifndef _SEND_SIDE_CONTROLLER_H_INCLUDED
#define _SEND_SIDE_CONTROLLER_H_INCLUDED

#ifdef _USE_RTP_SEND_CONTROLLER
#include "msec_timer.h"
#include "rv_adapter/rv_api.h"
#include "boost/thread.hpp"
#include "boost/atomic.hpp"
#include "boost/thread/mutex.hpp"

#include<stdint.h>
#include <memory>
#include <map>


extern bool rv_net_address_equal(const rv_net_address& a, const rv_net_address& b);

class rtcp_observer_t
{
public:
    virtual void on_rtcp_receive(uint32_t ssrc, uint32_t remote_ssrc, const rv_rtcp_rrinfo& rr, uint32_t rtt, const rv_net_address& remote_rtcp_address) = 0;
    virtual ~rtcp_observer_t() {}
};

struct rv_net_address_wrapper_t : rv_net_address
{
public:
    rv_net_address_wrapper_t()
    {
        ::construct_rv_net_address(static_cast<rv_net_address *>(this));
    }

    rv_net_address_wrapper_t(const rv_net_address_wrapper_t& rhs)
    {
        assign(rhs);
    }

    rv_net_address_wrapper_t(const rv_net_address& rhs)
    {
        assign(rhs);
    }

    rv_net_address_wrapper_t& operator=(const rv_net_address_wrapper_t& rhs)
    {
        assign(rhs);
        return *this;
    }

    bool operator < (const rv_net_address_wrapper_t& rhs) const
    {
        rv_net_ipv4 a;
        construct_rv_net_ipv4(&a);
        convert_rvnet_to_ipv4(&a, const_cast<rv_net_address *>(static_cast<const rv_net_address*>(this)));

        rv_net_ipv4 b;
        construct_rv_net_ipv4(&b);
        convert_rvnet_to_ipv4(&b, const_cast<rv_net_address *>(static_cast<const rv_net_address*>(&rhs)));

        if (a.ip != b.ip)
        {
            return a.ip < b.ip;
        }

        return a.port < b.port;
    }

    void assign(const rv_net_address_wrapper_t& rhs)
    {
        memcpy(this, &rhs, sizeof(rv_net_address_wrapper_t));
    }

    void assign(const rv_net_address& rhs)
    {
        memcpy(address, rhs.address, sizeof(rv_net_address));
    }
};

struct remote_sink_prof_t
{
    int64_t ts_;
    uint32_t rtt_;
    uint8_t fraction_lost_;

    remote_sink_prof_t()
        :ts_(get_tick_count()),
        rtt_(0),
        fraction_lost_(0)
    {}

    void update(int64_t ts, uint8_t fraction_lost, uint32_t rtt)
    {
        ts_ = ts;

        //0��rtt����Чֵ
        if (0 != rtt)
        {
            rtt_ = rtt;
        }

        fraction_lost_ = fraction_lost;
    }
};

class bitrate_observer_t
{
public:
    virtual void on_network_change(uint32_t bitrate, uint8_t fraction_lost, uint32_t rtt) = 0;
    virtual ~bitrate_observer_t() {}
};

class bitrate_controller_t
{
public:
    bitrate_controller_t(uint32_t start_bitrate, uint32_t min_bitrate, uint32_t max_bitrate, uint32_t max_rtt_thr, uint32_t max_rtcp_priod_thr);
    ~bitrate_controller_t();

    void add_bitrate_observer(bitrate_observer_t *ob);
    void remove_bitrate_observer(bitrate_observer_t *ob);

    rtcp_observer_t *create_rtcp_observer();
    void add_sink(const rv_net_address& remote_rtcp_address);
    void del_sink(const rv_net_address& remote_rtcp_address);

    void on_rtcp_receive(uint32_t ssrc, uint32_t remote_ssrc, const rv_rtcp_rrinfo& rr, uint32_t rtt, const rv_net_address& remote_rtcp_address);
    void check_rtcp_priod();
private:
    void on_network_change(uint32_t bitrate, uint8_t fraction_lost, uint32_t rtt);
    void update_rtcp(const rv_net_address& remote_rtcp_address, uint8_t fraction_lost, uint32_t rtt);
    void update_prof();

    typedef std::map<rv_net_address_wrapper_t, remote_sink_prof_t> remote_sink_prof_map_t;
    remote_sink_prof_map_t remote_sink_prof_;

    bitrate_observer_t *bitrate_observer_;

    uint32_t max_rtcp_priod_thr_;
    uint32_t max_rtt_thr_;
    uint8_t max_fraction_lost_thr_;
    uint8_t min_fraction_lost_thr_;

    uint8_t max_fraction_lost_;
    uint32_t max_rtt_;

    uint32_t bitrate_;
    uint32_t max_bitrate_;
    uint32_t min_bitrate_;

    boost::mutex mutex_;
    typedef boost::mutex::scoped_lock scoped_lock;
};

class flow_controller_t : public bitrate_observer_t
{
public:
    flow_controller_t();

    bool update_flow(uint32_t bytes, uint32_t frame_type);

private:
    void on_network_change(uint32_t bitrate, uint8_t fraction_lost, uint32_t rtt);

    int64_t last_update_ts_;

    uint32_t limited_bitrate_;
    uint32_t limited_send_bytes_;
};

#endif //_USE_RTP_SEND_CONTROLLER
#endif //_SEND_SIDE_CONTROLLER_H_INCLUDED
