#include "rtp_sink_impl.h"
#include "rtp_unpack.h"
#include "compat.h"
#include "xt_media_client.h"
#include <boost/thread.hpp>

extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);
namespace xt_media_client
{
    bool xt_mp_sink_library::init(uint16_t rtp_rv_thread_num, uint16_t rtp_sink_thread_num, uint16_t rtp_post_thread_num)
    {
        sink_init_descriptor descriptor;

        descriptor.rv_thread_num = rtp_rv_thread_num;
        descriptor.sink_thread_num = rtp_sink_thread_num;
        descriptor.post_thread_num = rtp_post_thread_num;

        long ret = ::mp_sink_init(&descriptor);
        return (ret >= 0);
    }

    void xt_mp_sink_library::term()
    {
        ::mp_sink_end();
    }

    bool xt_mp_sink_library::add_rtp_sink(rv_rtp rtpH, rtp_sink_impl *sink)
    {
        spinlock_t::scoped_lock _lock(rtp_to_sink_map_mutex_);
        return rtp_to_sink_map_.insert(rtp_to_sink_map_t::value_type(rtpH, sink)).second;
    }

    bool xt_mp_sink_library::del_rtp_sink(rv_rtp rtpH)
    {
        spinlock_t::scoped_lock _lock(rtp_to_sink_map_mutex_);
        return (0 != rtp_to_sink_map_.erase(rtpH));
    }

    rtp_sink_impl *xt_mp_sink_library::get_rtp_sink(rv_rtp rtpH)
    {
        rtp_to_sink_map_t::const_iterator it = rtp_to_sink_map_.find(rtpH);
        if (rtp_to_sink_map_.end() == it)
        {
            return NULL;
        }
        return it->second;
    }

    bool payload_to_unpacker_t::add_unpacker(int payload, const rtp_unpack_ptr& unpacker)
    {
        spinlock_t::scoped_lock _lock(mutex_);
        return unpackers_.insert(unpacker_map_t::value_type(payload, unpacker)).second;
    }

    rtp_unpack_ptr payload_to_unpacker_t::get_unpacker(int payload)
    {
        spinlock_t::scoped_lock _lock(mutex_);
        unpacker_map_t::iterator it = unpackers_.find(payload);
        return unpackers_.end() != it ? it->second : rtp_unpack_ptr();
    }

    bool payload_to_unpacker_t::empty() const
    {
        spinlock_t::scoped_lock _lock(mutex_);
        return unpackers_.empty();
    }

    void payload_to_unpacker_t::register_frame_dump_callback(frame_data_dump_callback_t *cb)
    {
        spinlock_t::scoped_lock _lock(mutex_);
        for (unpacker_map_t::iterator it = unpackers_.begin(); unpackers_.end() != it; ++it)
        {
            it->second->register_frame_dump_callback(cb);
        }
    }

    rtp_sink_impl::rtp_sink_impl(uint32_t rtp_frame_bytes, uint32_t rtp_frame_max_bytes)
        :handle_(),
        cb_(NULL),
        payload_to_unpacker_(),
        rtp_frame_buf_(new (std::nothrow) rtp_buf_t(rtp_frame_bytes, rtp_frame_max_bytes)),
        is_open_(false),
        is_multicast_(false),
        is_demux_(false),
        rtp_total_bytes_(0),
		rtcp_cb_(NULL)
    {}

    rtp_sink_impl::rtp_sink_impl(const rtp_unpack_ptr&packer)
        :handle_(),
        cb_(NULL),
        payload_to_unpacker_(),
        rtp_frame_buf_(),
        is_open_(false),
        is_multicast_(false),
        is_demux_(false),
        rtp_total_bytes_(0),
		rtcp_cb_(NULL)
    {
        add_unpacker(-1, packer);
    }

    rtp_sink_impl::~rtp_sink_impl()
    {
        close_rtp();
    }

    void rtp_sink_impl::add_unpacker(int payload, const rtp_unpack_ptr &packer)
    {
        payload_to_unpacker_.add_unpacker(payload, packer);
        if (packer)
        {
            if (cb_)
            {
                payload_to_unpacker_.register_frame_dump_callback(cb_);
            }
        }
    }

    long rtp_sink_impl::rtcp_send_fir()
    { 
        return ::mp_rtcp_send_fir(&handle_);
    }

    bool rtp_sink_impl::open_rtp(const char *ip, uint16_t rtp_port, uint16_t rtcp_port, open_rtp_mode open_mode, bool demux, uint32_t& demuxid, const multicast_param_t *multicast, rv_context demux_handler)
    {
        xt_mp_descriptor descriptor;
        (void)memset(&descriptor, 0, sizeof(xt_mp_descriptor));

        (void)strncpy_s((char *)descriptor.local_address.ip_address, sizeof(descriptor.local_address.ip_address), ip, MEDIA_CLIENT_IP_LEN);
        descriptor.local_address.port = rtp_port;

        //!!!notify: 有组帧器就说明需要直接输出rtp数据
        switch (open_mode)
        {
        case raw_rtp_output:
            descriptor.is_direct_output  = MP_TRUE;
            break;
        case frame_output:
            descriptor.is_direct_output  = MP_FALSE;
            break;
        case unknown_output:
            descriptor.is_direct_output  = payload_to_unpacker_.empty() ? MP_FALSE : MP_TRUE;
            break;
        default:
            assert(false);
            break;
        }

        descriptor.manual_rtcp = MP_FALSE;
        descriptor.mode = MP_MEMORY_MSINK;

        if (multicast)
        {
            descriptor.rtp_multi_cast_opt = MP_TRUE;
            descriptor.rtp_multi_cast_address.port = multicast->port;
            (void)strncpy_s((char *)descriptor.rtp_multi_cast_address.ip_address, sizeof(descriptor.rtp_multi_cast_address.ip_address), multicast->ip, MEDIA_CLIENT_IP_LEN);
            descriptor.rtp_multi_cast_ttl = multicast->ttl;

            descriptor.rtcp_multi_cast_opt = MP_TRUE;
            descriptor.rtcp_multi_cast_address.port = multicast->port + 1;
            (void)strncpy_s((char *)descriptor.rtcp_multi_cast_address.ip_address, sizeof(descriptor.rtcp_multi_cast_address.ip_address), multicast->ip, MEDIA_CLIENT_IP_LEN);
            descriptor.rtcp_multi_cast_ttl = multicast->ttl;
        }
        else
        {
            descriptor.rtp_multi_cast_opt = MP_FALSE;
            descriptor.rtcp_multi_cast_opt = MP_FALSE;
        }

        descriptor.context = this;
        descriptor.onReceiveDataEvent = &s_rtp_frame_handler;
        descriptor.onReceiveRtpEvent = &s_rtp_raw_handler;
        descriptor.onReportReceiveEvent = &s_report_receive_handler;
        descriptor.onReportSendEvent = &s_report_send_handler;

        is_demux_ = demux;
        is_multicast_ = (NULL != multicast);
        long ret = 0;

       md_log(md_log_info, "open sink:port[%d]", rtp_port);
        if (demux)
        {
            ret = ::mp_open_mult(&descriptor, &handle_, &demuxid);
            if (ret > -1)
            {
                ::mp_setdemux_handler(demux_handler);
            }
        }
        else
        {
            ret = ::mp_open(&descriptor, &handle_);
        }

        is_open_ = (ret >= 0);
        is_multicast_ = NULL != multicast ? true : false;
        is_demux_ = demux;

        // !notify: 必须要在open后立刻调用active
        if (is_open_)
        {
            ::mp_active(&handle_, 1);

            if (demux)
            {
                xt_mp_sink_library::instance()->add_rtp_sink(::mp_query_rtp_handle(&handle_), this);
            }
        }

       md_log(md_log_info, "open sink:ret[%d] port[%d]", ret, rtp_port);

        return is_open_;
    }

    void rtp_sink_impl::active_rtp(frame_data_dump_callback_t *cb)
    {
        if (!payload_to_unpacker_.empty())
        {
            payload_to_unpacker_.register_frame_dump_callback(cb);
        }
        else
        {
            cb_ = cb;
        }
    }

    void rtp_sink_impl::register_rtcp_callback(rtcp_report_callback_t *cb)
    {
    	rtcp_cb_ = cb;
    }
	
    void rtp_sink_impl::close_rtp()
    {
        if (is_open_)
        {
            if (is_demux_)
            {
                xt_mp_sink_library::instance()->del_rtp_sink(mp_query_rtp_handle(&handle_));
            }

           md_log(md_log_info, "close sink:handle_[%p]", handle_.h_mp);
            ::mp_active(&handle_, 0);
           md_log(md_log_info, "close sink(mp_active):handle_[%p]", handle_.h_mp);
            ::mp_close(&handle_);
           md_log(md_log_info, "close sink(mp_close):handle_[%p]", handle_.h_mp);
            active_rtp(NULL);
           md_log(md_log_info, "close sink(active_rtp):handle_[%p]", handle_.h_mp);
            is_open_ = false;
        }
    }

    bool rtp_sink_impl::add_remote_address(const char *ip, uint16_t rtp_port, uint16_t rtcp_port, bool demux, uint32_t demuxid)
    {
        if (is_multicast_)
        {
            return true;
        }

        return add_rtp_remote_address(ip, rtp_port, demux, demuxid) && add_rtcp_remote_address(ip, rtcp_port, demux, demuxid);
    }

    bool rtp_sink_impl::add_rtp_remote_address(const char *ip, uint16_t port, bool demux, uint32_t demuxid)
    {
        long result = 0;
        mp_address_descriptor descriptor;

        if (is_multicast_)
        {
            return true;
        }

        (void)memset(&descriptor, 0, sizeof(mp_address_descriptor));

        (void)strncpy_s((char *)descriptor.ip_address, sizeof(descriptor.ip_address), ip, MEDIA_CLIENT_IP_LEN);
        descriptor.port = port;

        if (demux)
        {
            result = ::mp_add_mult_rtp_remote_address(&handle_, &descriptor, demuxid);
        }
        else
        {
            result = ::mp_add_rtp_remote_address(&handle_, &descriptor);
        }

        return (result >= 0);
    }

    bool rtp_sink_impl::add_rtcp_remote_address(const char *ip, uint16_t port, bool demux, uint32_t demuxid)
    {
        long result = 0;
        mp_address_descriptor descriptor;

        if (is_multicast_)
        {
            return true;
        }

        (void)memset(&descriptor, 0, sizeof(mp_address_descriptor));

        (void)strncpy_s((char *)descriptor.ip_address, sizeof(descriptor.ip_address), ip, MEDIA_CLIENT_IP_LEN);
        descriptor.port = port;

        if (demux)
        {
            result = ::mp_add_mult_rtcp_remote_address(&handle_, &descriptor, demuxid);
        }
        else
        {
            result = ::mp_add_rtcp_remote_address(&handle_, &descriptor);
        }

        return (result >= 0);
    }

    //bool rtp_sink_impl::get_rtcp_rr(rtcp_recv_report_t *rr) const
    //{
    //    if (NULL == rr)
    //    {
    //        return false;
    //    }

    //    if (!is_open_)
    //    {
    //        return false;
    //    }

    //    rtcp_receive_report rtcp_rr;

    //    long ret = ::mp_query_rcv_rtcp(const_cast<p_msink_handle>(&handle_), &rtcp_rr);
    //    if (ret < 0)
    //    {
    //        return false;
    //    }

    //    rr->ssrc = rtcp_rr.ssrc;
    //    rr->cumulativeLost = rtcp_rr.cumulativeLost;
    //    rr->fractionLost = rtcp_rr.fractionLost;
    //    rr->jitter = rtcp_rr.jitter;
    //    rr->sequenceNumber = rtcp_rr.sequenceNumber;
    //    rr->lSR = rtcp_rr.lSR;
    //    rr->dlSR = rtcp_rr.dlSR;

    //    return true;
    //}

    bool rtp_sink_impl::get_rtp_prof_info(rtp_prof_info_t *rpi) const
    {
        if (NULL == rpi)
        {
            return false;
        }

        rpi->rtp_total_bytes = rtp_total_bytes_;

        return true;
    }

    uint32_t rtp_sink_impl::get_ssrc() const
    {
        return (handle_.rtp_ssrc<<16) + (handle_.rtcp_ssrc);
    }

    void rtp_sink_impl::rtp_frame_handler(mp_handle hmp)
    {
        block_params block = {0};
        XTFrameInfo frame = {0};

        while (true)
        {
            long ret = ::mp_read_out_data2(&handle_, rtp_frame_buf_->data(), rtp_frame_buf_->capacity(), &block, frame);
            //md_log(md_log_debug, "mp_read_out_data2:frame_type(%d), data_type(%d), length(%d)\n",frame.frametype,frame.datatype, rtp_frame_buf_->capacity());

            if (ret < 0)
            {
                break;
            }
            else if (ret > 0)
            {
                if (!rtp_frame_buf_->resize(ret))
                {
                    break;
                }

                continue;
            }

            rtp_total_bytes_ += block.size;

            if (NULL != cb_)
            {
                cb_->on_frame_dump(rtp_frame_buf_->data(), block.size, frame.frametype, frame.datatype, block.timestamp, block.ssrc);
            }
        }
    }

    void rtp_sink_impl::rtp_raw_handler(mp_handle hmp)
    {
		rv_rtp_param params={0};
		void *ptr_rtp_block = NULL;
		rtp_unpack_ptr unpacker = payload_to_unpacker_.get_unpacker(-1);
        while (unpacker)
        {
            long ret = ::mp_pump_out_rtp(&handle_, &ptr_rtp_block);
            if (ret < 0)
            {
                break;
            }
			unpacker->dump_rtp_frame_data((uint8_t *)ptr_rtp_block,params.len,params);
			if (ret == 1)
			{
				break;
			}
        }
		/*uint8_t buf[RTP_PACKAGE_MAX_SIZE];
		uint32_t len = sizeof(buf);

		rv_rtp_param params = { 0 };

		while (true)
		{
			long ret = ::mp_read_out_rtp(&handle_, buf, len, &params);
			if (ret < 0)
			{
				break;
			}

			rtp_total_bytes_ += params.len;

			rtp_unpack_ptr unpacker = payload_to_unpacker_.get_unpacker(-1);
			if (unpacker)
			{
				unpacker->pump_rtp_raw_data(buf, params.len - params.sByte, params);
			}
		}*/
    }

    void rtp_sink_impl::report_receive_handler(const rtcp_send_report *sr)
    {
        if (NULL != rtcp_cb_)
        {
            xt_media_client_rtcp_sr_t media_client_sr = { 0 };

            media_client_sr.lNTPtimestamp  = sr->lNTPtimestamp;
            media_client_sr.mNTPtimestamp = sr->mNTPtimestamp;
            media_client_sr.octets = sr->octets;
            media_client_sr.packets = sr->packets;
            media_client_sr.timestamp = sr->timestamp;

            rtcp_cb_->on_rtcp_receive(sr->ssrc, &media_client_sr);
            //XT_LOG(info, "report_receive_handler ssrc=%u,lNTPtimestamp=%u,mNTPtimestamp=%u,octets=%u,packets=%u,timestamp=%u",sr->ssrc,media_client_sr.lNTPtimestamp,media_client_sr.mNTPtimestamp,media_client_sr.octets,media_client_sr.packets,media_client_sr.timestamp);
        }
    }

    void rtp_sink_impl::report_send_handler(const rtcp_receive_report *rr)
    {
        if (NULL != rtcp_cb_)
        {
            xt_media_client_rtcp_rr_t media_client_rr = { 0 };

            media_client_rr.dlSR = rr->dlSR;
            media_client_rr.cumulativeLost = rr->cumulativeLost;
            media_client_rr.fractionLost = rr->fractionLost;
            media_client_rr.jitter = rr->jitter;
            media_client_rr.lSR = rr->lSR;
            media_client_rr.sequenceNumber = rr->sequenceNumber;

            rtcp_cb_->on_rtcp_send(rr->ssrc, &media_client_rr);
        }
    }

    void rtp_sink_impl::s_rtp_frame_handler(mp_handle hmp, void* ctx)
    {
        rtp_sink_impl *impl = static_cast<rtp_sink_impl *>(ctx);
        if (NULL != impl)
        {
            impl->rtp_frame_handler(hmp);
        }
    }
	
    void rtp_sink_impl::s_rtp_raw_handler(mp_handle hmp, void* ctx)
    {
        rtp_sink_impl *impl = static_cast<rtp_sink_impl *>(ctx);
        if (NULL != impl)
        {
			impl->rtp_raw_handler(hmp);
        }
    }

    void rtp_sink_impl::s_rtcp_report_handler(uint32_t, uint32_t, uint32_t, void *)
    {}

    bool rtp_sink_impl::is_demux()
    {
        return m_info.demux;
    }

    msink_handle* rtp_sink_impl::get_handle()
    {
        return &handle_;
    }

    //static 
    int rtp_sink_impl::pump_demux_rtp(rtp_sink_impl *sink, void *buf, uint32_t len, const rv_rtp_param &p, const rv_net_address &address)
    {
        int ret = -1;
        if (NULL != sink)
        {
            rtp_unpack_ptr unpacker = sink->payload_to_unpacker_.get_unpacker(p.payload);
            if (unpacker)
            {
                ret = 0;
                unpacker->pump_rtp_raw_data((uint8_t *)buf+p.sByte, len-p.sByte, p);
            }
        }

        return ret;
    }
	
	void rtp_sink_impl::s_report_receive_handler(const rtcp_send_report *sr, void *context)
    {
        rtp_sink_impl *impl = static_cast<rtp_sink_impl *>(context);
        if (NULL != impl)
        {
            impl->report_receive_handler(sr);
            //XT_LOG(info, "report_receive_handler ssrc=%u,lNTPtimestamp=%u,mNTPtimestamp=%u,octets=%u,packets=%u,timestamp=%u",sr->ssrc,sr->lNTPtimestamp,sr->mNTPtimestamp,sr->octets,sr->packets,sr->timestamp);
        }
    }

    //static
    void rtp_sink_impl::s_report_send_handler(const rtcp_receive_report *rr, void *context)
    {
        rtp_sink_impl *impl = static_cast<rtp_sink_impl *>(context);
        if (NULL != impl)
        {
            impl->report_send_handler(rr);
        }
    }
}
