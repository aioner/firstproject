#ifndef _TRAFFIC_SHAPING_H_INCLUDED
#define _TRAFFIC_SHAPING_H_INCLUDED

#ifdef _USE_RTP_TRAFFIC_SHAPING
#include "msec_timer.h"

#include "boost/lockfree/queue.hpp"
#include "boost/atomic.hpp"
#include "boost/circular_buffer.hpp"

class i_flow_op_t
{
public:
    typedef uint32_t size_type;

    virtual void release(void *flow) = 0;
    virtual size_type size(void *flow) const = 0;
    virtual uint32_t priority(void *flow) const = 0;
protected:
    virtual ~i_flow_op_t() {}
};

class flow_out_callback_t
{
public:
    virtual void on_flow_out(void *flow) = 0;

protected:
    virtual ~flow_out_callback_t() {}
};

class i_flow_bucket_t
{
public:
    virtual bool flow_in(void *flow) = 0;
    virtual void register_callback(flow_out_callback_t *cb) = 0;
    virtual void update_speed(uint32_t speed) = 0;
    virtual void disable_for_time(uint32_t ms) = 0;

protected:
    virtual ~i_flow_bucket_t() {}
};

class speed_calc_callback_t
{
public:
    virtual void on_network_changed(uint32_t speed) = 0;

protected:
    virtual ~speed_calc_callback_t() {}
};

class i_speed_calc_t
{
public:
    virtual void update_bytes(uint32_t bytes) = 0;
    virtual void register_callback(speed_calc_callback_t *cb) = 0;

protected:
    virtual ~i_speed_calc_t() {}
};

class traffic_shaping_t : public speed_calc_callback_t
{
public:
    traffic_shaping_t(i_flow_bucket_t *flow_bucket, i_speed_calc_t *speed_calc, i_flow_op_t *op);
    bool flow_in(void *flow);
    void register_flow_out_callback(flow_out_callback_t *cb);
private:
    void on_network_changed(uint32_t speed);

    i_flow_bucket_t *flow_bucket_;
    i_speed_calc_t *speed_calc_;
    i_flow_op_t *op_;
};

class flow_bucket_impl : public i_flow_bucket_t, public deadline_timer_callback_t
{
public:
    flow_bucket_impl(uint32_t init_speed, uint32_t max_flow_num, i_flow_op_t *op);

    bool flow_in(void *flow);
    void register_callback(flow_out_callback_t *cb);
    void update_speed(uint32_t speed);
    void disable_for_time(uint32_t ms);
private:
    uint32_t on_expires();
    uint32_t flow_out_directly(void *flow);
    bool in_disabled_time();

    boost::lockfree::queue<void *> flows_;

    i_flow_op_t *op_;
    flow_out_callback_t *cb_;
    boost::atomic_uint32_t speed_;

    volatile int64_t disabled_deadline_ms_;
};

class speed_calc_window_impl : public i_speed_calc_t
{
public:
    struct flow_in_window_t
    {
        flow_in_window_t(uint32_t bytes, uint32_t priod)
            :bytes_(bytes),
            priod_(priod)
        {}

        uint32_t bytes_;
        uint32_t priod_;
    };

    speed_calc_window_impl(uint16_t num_of_windows, uint16_t calc_ms_priod);
    void update_bytes(uint32_t bytes);
    void register_callback(speed_calc_callback_t *cb);
public:
    boost::circular_buffer<flow_in_window_t> flows_in_windows_;
    spinlock_t flows_in_windows_mutex_;
    uint32_t bytes_total_in_windows_;
    uint32_t priod_total_in_windows_;

    uint32_t bytes_in_one_priod_;
    uint32_t calc_ms_priod_;
    tick_count_t calc_ms_timestamp_;
    tick_count_t prev_ms_timestamp_;

    speed_calc_callback_t *cb_;

private:
    bool refresh_flows_windows(uint32_t distance);
};


class traffic_shaping_wrapper_t : private flow_bucket_impl, private speed_calc_window_impl, public traffic_shaping_t
{
public:
    traffic_shaping_wrapper_t(uint32_t init_speed, uint32_t max_flow_num, uint16_t num_of_windows, uint16_t calc_ms_priod);

    using traffic_shaping_t::flow_in;
};

#endif //_USE_RTP_TRAFFIC_SHAPING

#endif //_TRAFFIC_SHAPING_H_INCLUDED
