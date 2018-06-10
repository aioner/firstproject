#ifdef _USE_RTP_SEND_CONTROLLER
#include "send_side_controller.h"

#ifdef _WIN32
#include <wtypes.h>
#endif

bool rv_net_address_equal(const rv_net_address& a, const rv_net_address& b)
{
    const uint32_t *x = reinterpret_cast<const uint32_t *>(&a);
    const uint32_t *y = reinterpret_cast<const uint32_t *>(&b);
    return ((x[0] == y[0]) && (x[1] == y[1]) && (x[2] == y[2]) && (x[3] == y[3]));
}

enum { kAvgPacketSizeBytes = 1000 };

// Calculate the rate that TCP-Friendly Rate Control (TFRC) would apply.
// The formula in RFC 3448, Section 3.1, is used.
uint32_t CalcTfrcBps(int64_t rtt, uint8_t loss)
{
    if (rtt == 0 || loss == 0) 
    {
        // Input variables out of range.
        return 0;
    }
    double R = static_cast<double>(rtt) / 1000;  // RTT in seconds.
    int b = 1;  // Number of packets acknowledged by a single TCP acknowledgement:
    // recommended = 1.
    double t_RTO = 4.0 * R;  // TCP retransmission timeout value in seconds
    // recommended = 4*R.
    double p = static_cast<double>(loss) / 255;  // Packet loss rate in [0, 1).
    double s = static_cast<double>(kAvgPacketSizeBytes);

    // Calculate send rate in bytes/second.
    double X =
        s / (R * std::sqrt(2 * b * p / 3) +
        (t_RTO * (3 * std::sqrt(3 * b * p / 8) * p * (1 + 32 * p * p))));

    // Convert to bits/second.
    return (static_cast<uint32_t>(X * 8));
}

class rtcp_observer_impl_t : public rtcp_observer_t
{
public:
    explicit rtcp_observer_impl_t(bitrate_controller_t *owner)
        :owner_(owner)
    {}

    void on_rtcp_receive(uint32_t ssrc, uint32_t remote_ssrc, const rv_rtcp_rrinfo& rr, uint32_t rtt, const rv_net_address& remote_rtcp_address)
    {
        if (NULL != owner_)
        {
            //如果tLSR或tDLSR为0，则表示没有收到SR报告，rtt为不准确值
            if ((0 == rr.lSR) || (0 == rr.dlSR))
            {
                rtt = 0;
            }

            owner_->on_rtcp_receive(ssrc, remote_ssrc, rr, rtt, remote_rtcp_address);
        }
    }

    bitrate_controller_t *owner_;
};

bitrate_controller_t::bitrate_controller_t(uint32_t start_bitrate, uint32_t min_bitrate, uint32_t max_bitrate, uint32_t max_rtt_thr, uint32_t max_rtcp_priod_thr)
:remote_sink_prof_(),
bitrate_observer_(NULL),
max_rtcp_priod_thr_(max_rtcp_priod_thr),
max_rtt_thr_(max_rtt_thr),
max_fraction_lost_thr_(26),
min_fraction_lost_thr_(5),
max_fraction_lost_(0),
max_rtt_(0),
bitrate_(start_bitrate),
min_bitrate_(min_bitrate),
max_bitrate_(max_bitrate),
mutex_()
{}

bitrate_controller_t::~bitrate_controller_t()
{}

void bitrate_controller_t::add_bitrate_observer(bitrate_observer_t *ob)
{
    bitrate_observer_ = ob;
}

void bitrate_controller_t::remove_bitrate_observer(bitrate_observer_t *ob)
{
    bitrate_observer_ = NULL;
}

rtcp_observer_t *bitrate_controller_t::create_rtcp_observer()
{
    return new (std::nothrow) rtcp_observer_impl_t(this);
}

void bitrate_controller_t::add_sink(const rv_net_address& remote_rtcp_address)
{
    scoped_lock _lock(mutex_);
    remote_sink_prof_[remote_rtcp_address].update(get_tick_count(), 0, 0);
}

void bitrate_controller_t::del_sink(const rv_net_address& remote_rtcp_address)
{
    {
        scoped_lock _lock(mutex_);
        remote_sink_prof_.erase(remote_rtcp_address);
    }

    update_prof();
}

void bitrate_controller_t::on_rtcp_receive(uint32_t ssrc, uint32_t remote_ssrc, const rv_rtcp_rrinfo &rr, uint32_t rtt, const rv_net_address &remote_rtcp_address)
{
    update_rtcp(remote_rtcp_address, rr.fractionLost, rtt);
    update_prof();
}

void bitrate_controller_t::check_rtcp_priod()
{
    int64_t ts = get_tick_count();

    bool expires_flag = false;
    {
        scoped_lock _lock(mutex_);
        for (remote_sink_prof_map_t::iterator it = remote_sink_prof_.begin(); remote_sink_prof_.end() != it; ++it)
        {
            //rtcp��ʱ��
            if (ts - it->second.ts_ > max_rtcp_priod_thr_)
            {
                it->second.ts_ = ts;
                it->second.rtt_ = static_cast<uint32_t>(ts - it->second.ts_) * 2;
                expires_flag = true;
            }
        }
    }

    if (expires_flag)
    {
        update_prof();
    }
}

void bitrate_controller_t::update_rtcp(const rv_net_address& remote_rtcp_address, uint8_t fraction_lost, uint32_t rtt)
{
    scoped_lock _lock(mutex_);
    remote_sink_prof_map_t::iterator it = remote_sink_prof_.find(remote_rtcp_address);
    if (remote_sink_prof_.end() != it)
    {
        it->second.update(get_tick_count(), fraction_lost, rtt);
    }
}

void bitrate_controller_t::update_prof()
{
    scoped_lock _lock(mutex_);

    //ѡ�������ʺ�rtt���ֵ
    uint8_t max_fraction_lost = 0;
    uint32_t max_rtt = 0;
    for (remote_sink_prof_map_t::iterator it = remote_sink_prof_.begin(); remote_sink_prof_.end() != it; ++it)
    {
        if (max_fraction_lost < it->second.fraction_lost_)
        {
            max_fraction_lost = it->second.fraction_lost_;
        }

        if (max_rtt < it->second.rtt_)
        {
            max_rtt = it->second.rtt_;
        }
    }

    if (max_fraction_lost <= min_fraction_lost_thr_)
    {
        bitrate_ = static_cast<uint32_t>(bitrate_ * 1.08 + 0.5);
        bitrate_ += 1000 * 8;

        if (bitrate_ > max_bitrate_)
        {
            bitrate_ = max_bitrate_;
        }

        if (bitrate_ > (64U * 1024 * 1024 * 8))
        {
            bitrate_ = 64 * 1024 * 1024 * 8;
        }
    }
    else if (max_fraction_lost <= max_fraction_lost_thr_)
    {}
    else
    {
        bitrate_ = static_cast<uint32_t>((bitrate_ * static_cast<double>(512 - max_fraction_lost)) / 512.0);
        uint32_t tfrc_bps =  CalcTfrcBps(max_rtt, max_fraction_lost);
        if (bitrate_ < tfrc_bps)
        {
            bitrate_ = tfrc_bps;
        }

        if (bitrate_ < min_bitrate_)
        {
            bitrate_ = min_bitrate_;
        }

        if (bitrate_ < 10000)
        {
            bitrate_ = 10000;
        }
    }

    uint32_t bitrate = bitrate_;
    if ((0 != max_rtt_thr_) && (max_rtt >= max_rtt_thr_))
    {
        bitrate = min_bitrate_;

        //如果上一次rtt也高于阀值，则降低真实速率
        if (max_rtt_ >= max_rtt_thr_)
        {
            bitrate_ = bitrate;
        }
    }

    on_network_change(bitrate, max_fraction_lost, max_rtt);

    max_rtt_ = max_rtt;
    max_fraction_lost_ = max_fraction_lost;
}

void bitrate_controller_t::on_network_change(uint32_t bitrate, uint8_t fraction_lost, uint32_t rtt)
{
    if (NULL != bitrate_observer_)
    {
        bitrate_observer_->on_network_change(bitrate, fraction_lost, rtt);
    }
}

flow_controller_t::flow_controller_t()
:last_update_ts_(get_tick_count()),
limited_bitrate_(0),
limited_send_bytes_(0)
{}

bool flow_controller_t::update_flow(uint32_t bytes, uint32_t frame_type)
{
    if (0 == limited_bitrate_)
    {
        return true;
    }

    int64_t now = get_tick_count();
    if (now - last_update_ts_ > 1000)
    {
        limited_send_bytes_ = (limited_bitrate_ >>3);
        last_update_ts_ = now;
    }

    if (bytes > limited_send_bytes_)
    {
        if (0 == frame_type)
        {
            limited_send_bytes_ = 0;
            return true;
        }

        return false;
    }

    limited_send_bytes_ -= bytes;

    return true;
}

void flow_controller_t::on_network_change(uint32_t bitrate, uint8_t fraction_lost, uint32_t rtt)
{
    limited_bitrate_ = bitrate;
}

#endif //_USE_RTP_SEND_CONTROLLER

