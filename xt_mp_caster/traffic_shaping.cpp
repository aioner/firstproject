#ifdef _USE_RTP_TRAFFIC_SHAPING
#include "traffic_shaping.h"
#include "mp.h"
#include "mp_caster.h"

#define _TRAFFIC_SHAPING_FLOW_OUT_MULTIPLE          1.2

traffic_shaping_t::traffic_shaping_t(i_flow_bucket_t *flow_bucket, i_speed_calc_t *speed_calc, i_flow_op_t *op)
:flow_bucket_(flow_bucket),
speed_calc_(speed_calc),
op_(op)
{
    if (speed_calc_)
    {
        speed_calc_->register_callback(this);
    }
}

bool traffic_shaping_t::flow_in(void *flow)
{
    if ((NULL == speed_calc_) || (NULL == op_) || (NULL == flow_bucket_))
    {
        return false;
    }

    speed_calc_->update_bytes(op_->size(flow));

    return flow_bucket_->flow_in(flow);
}

void traffic_shaping_t::register_flow_out_callback(flow_out_callback_t *cb)
{
    if (NULL != flow_bucket_)
    {
        flow_bucket_->register_callback(cb);
    }
}

void traffic_shaping_t::on_network_changed(uint32_t speed)
{
    //std::cout << "speed=" << speed << std::endl;
    if (NULL != flow_bucket_)
    {
        flow_bucket_->update_speed(speed);
    }
}

flow_bucket_impl::flow_bucket_impl(uint32_t init_speed, uint32_t max_flow_num, i_flow_op_t *op)
:flows_(max_flow_num),
op_(op),
cb_(NULL),
speed_(init_speed),
disabled_deadline_ms_(0)
{}

bool flow_bucket_impl::flow_in(void *flow)
{
    if (in_disabled_time())
    {
        return (0 != flow_out_directly(flow));
    }
    else
    {
        return flows_.bounded_push(flow);
    }
}

uint32_t flow_bucket_impl::flow_out_directly(void *flow)
{
    if ((NULL == op_) || (NULL == cb_))
    {
        return 0;
    }

    uint32_t size_of_flow = op_->size(flow);
    cb_->on_flow_out(flow);
    op_->release(flow);

    return size_of_flow;
}

bool flow_bucket_impl::in_disabled_time()
{
    if (0 != disabled_deadline_ms_)
    {
        if (disabled_deadline_ms_ >= get_tick_count())
        {
            return true;
        }
        else
        {
            disabled_deadline_ms_ = 0;
        }
    }
    return false;
}

void flow_bucket_impl::register_callback(flow_out_callback_t *cb)
{
    cb_ = cb;
}

void flow_bucket_impl::update_speed(uint32_t speed)
{
    speed_ = speed;
}

void flow_bucket_impl::disable_for_time(uint32_t ms)
{
    disabled_deadline_ms_ = get_tick_count() + ms;
}

uint32_t flow_bucket_impl::on_expires()
{
    uint32_t total_bytes = 0;

    while (true)
    {
        void *flow = NULL;
        if (!flows_.pop(flow))
        {
            break;
        }

        total_bytes += flow_out_directly(flow);

        uint32_t expires = 0;
        if (speed_ >= 0)
        {
            expires = static_cast<uint32_t>(total_bytes * 1000 / (_TRAFFIC_SHAPING_FLOW_OUT_MULTIPLE * speed_));
        }

        if (expires > 0)
        {
            return expires;
        }
    }

    return 0;
}

speed_calc_window_impl::speed_calc_window_impl(uint16_t num_of_windows, uint16_t calc_ms_priod)
:flows_in_windows_(num_of_windows),
flows_in_windows_mutex_(),
bytes_total_in_windows_(0),
priod_total_in_windows_(0),
bytes_in_one_priod_(0),
calc_ms_priod_(calc_ms_priod),
calc_ms_timestamp_(0),
prev_ms_timestamp_(0),
cb_(NULL)
{}

void speed_calc_window_impl::update_bytes(uint32_t bytes)
{
    tick_count_t now = get_tick_count();
    if (0 == calc_ms_timestamp_)
    {
        calc_ms_timestamp_ = now;
        prev_ms_timestamp_ = now;
    }

    //如果两次数据间隔超过4个周期，则认为是无效数据，从新计算该周期的速率
    if (static_cast<uint32_t>(now - prev_ms_timestamp_) >= calc_ms_priod_ * 4)
    {
        calc_ms_timestamp_ = now;
        bytes_in_one_priod_ = 0;
    }

    bytes_in_one_priod_ += bytes;

    uint32_t distance = static_cast<uint32_t>(now - calc_ms_timestamp_);
    if (distance >= static_cast<tick_count_t>(calc_ms_priod_))
    {
        bool ready = refresh_flows_windows(distance);

        bytes_total_in_windows_ += bytes_in_one_priod_;
        priod_total_in_windows_ += distance;

        if (ready && (NULL != cb_))
        {
            cb_->on_network_changed(static_cast<uint32_t>(bytes_total_in_windows_ * 1000.0f / (priod_total_in_windows_)));
        }

        calc_ms_timestamp_ = now;
        bytes_in_one_priod_ = 0;
    }

    prev_ms_timestamp_ = now;
}

void speed_calc_window_impl::register_callback(speed_calc_callback_t *cb)
{
    cb_ = cb;
}

bool speed_calc_window_impl::refresh_flows_windows(uint32_t distance)
{
    spinlock_t::scoped_lock _lock(flows_in_windows_mutex_);
    bool ready = flows_in_windows_.full();
    if (ready)
    {
        bytes_total_in_windows_ -= flows_in_windows_.front().bytes_;
        priod_total_in_windows_ -= flows_in_windows_.front().priod_;
    }

    flows_in_windows_.push_back(flow_in_window_t(bytes_in_one_priod_, distance));

    return ready;
}

class rtp_op_impl : public i_flow_op_t
{
public:
    void release(void *flow)
    {
        xt_mp_caster::rtp_block *rtp = static_cast<xt_mp_caster::rtp_block *>(flow);
        if (NULL != rtp)
        {
            rtp->release();
        }
    }

    size_type size(void *flow) const
    {
        xt_mp_caster::rtp_block *rtp = static_cast<xt_mp_caster::rtp_block *>(flow);
        if (NULL == rtp)
        {
            return 0;
        }

        return rtp->payload_totalsize();
    }

    uint32_t priority(void *flow) const
    {
        xt_mp_caster::rtp_block *rtp = static_cast<xt_mp_caster::rtp_block *>(flow);
        if (NULL == rtp)
        {
            return 0;
        }

        return rtp->m_priority;
    }

    static i_flow_op_t *create()
    {
        static rtp_op_impl s_rtp_op;
        return &s_rtp_op;
    }
};

traffic_shaping_wrapper_t::traffic_shaping_wrapper_t(uint32_t init_speed, uint32_t max_flow_num, uint16_t num_of_windows, uint16_t calc_ms_priod)
:flow_bucket_impl(init_speed, max_flow_num, rtp_op_impl::create()),
speed_calc_window_impl(num_of_windows, calc_ms_priod),
traffic_shaping_t(this, this, rtp_op_impl::create())
{
    xt_mp_caster::caster::self()->get_timer_mgr().add_timer(this);
}

#endif	//_USE_RTP_TRAFFIC_SHAPING

