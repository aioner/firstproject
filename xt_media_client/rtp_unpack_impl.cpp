#include "rtp_unpack_impl.h"
#include "xt_media_client_types.h"
#include "rtp_sink.h"

//RTP组帧需要验证时间戳的开关宏
//#define _RTP_UNPACK_TIMESTAMP_CHECK

//added by lichao for test
#include <iostream>

extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);
namespace xt_media_client
{
    rtp_unpack_direct_impl::rtp_unpack_direct_impl(uint32_t frame_type, uint32_t data_type)
        :cb_(NULL),
        frame_type_(frame_type),
        data_type_(data_type)
    {}

    bool rtp_unpack_direct_impl::pump_rtp_raw_data(uint8_t *data, uint32_t length, const rv_rtp_param &params)
    {
        dump_rtp_frame_data(data, length, params);
        return true;
    }

    void rtp_unpack_direct_impl::register_frame_dump_callback(frame_data_dump_callback_t *cb)
    {
        cb_ = cb;
    }

    void rtp_unpack_direct_impl::dump_rtp_frame_data(uint8_t *data, uint32_t length, const rv_rtp_param &params)
    {
        if (NULL != cb_)
        {
            cb_->on_frame_dump(data, length, frame_type_, data_type_, params.timestamp, params.sSrc);
        }
    }

    bool rtp_unpack_priv_impl::pump_rtp_raw_data(uint8_t *data, uint32_t length, const rv_rtp_param &params)
    {
        rtp_unpack_scoped_update_seq update_seq(this, params.sequenceNumber);
        return unpack_single_frame(data, length, params);
    }

    void rtp_unpack_priv_impl::on_lost_frame()
    {
        priv_chunk_count()++;
        unpack_state_ = unpack_stat_end;
        //md_log(md_log_debug, "on_lost_frame:frame_type=%u,unpack_state_=%u,last_pkt_seq_=%u", frame_type_, unpack_state_, last_pkt_seq_);
    }

    bool rtp_unpack_priv_impl::unpack_frame_start(const rv_rtp_param& params)
    {
        //遇到不连续的帧，按丢包处理，作为新的一帧开始并修复上一包序号
        if ((unpack_stat_uninit != unpack_state_) && !is_cont_pkt_seq(last_pkt_seq_, params.sequenceNumber))
        {
            on_lost_frame();
        }

        //遇到错误格式的包，按丢包处理
        if (!unpack_state_start_prepared(unpack_state_))
        {
            on_lost_frame();
        }

        unpack_state_ = unpack_stat_start;
        rtp_priv_frame_.rewind();
        priv_timestamp() = params.timestamp;

        return true;
    }

    bool rtp_unpack_priv_impl::unpack_frame_cont(const rv_rtp_param& params)
    {
        //序号不连续或者同一个fu帧内部的包时间戳不一致，按丢包处理
        if (!is_cont_pkt_seq(last_pkt_seq_, params.sequenceNumber)
#ifdef _RTP_UNPACK_TIMESTAMP_CHECK
            || (priv_timestamp() != params.timestamp)
#endif
            )
        {
            on_lost_frame();
            return false;
        }

        //持续状态之前必须为开始状态，否则按丢包处理
        if (!unpack_state_cont_prepared(unpack_state_))
        {
            on_lost_frame();
            return false;
        }

        unpack_state_ = unpack_stat_cont;

        return true;
    }

    bool rtp_unpack_priv_impl::unpack_frame_end(const rv_rtp_param& params)
    {
        //序号不连续或者同一个fu帧内部的包时间戳不一致，按丢包处理
        if (!is_cont_pkt_seq(last_pkt_seq_, params.sequenceNumber) 
#ifdef _RTP_UNPACK_TIMESTAMP_CHECK
            || (priv_timestamp() != params.timestamp)
#endif
            )
        {
            on_lost_frame();
            return false;
        }

        //结束状态之前必须为开始或持续状态，否则按丢包处理
        if (!unpack_state_end_prepared(unpack_state_))
        {
            on_lost_frame();
            return false;
        }

        dump_rtp_frame_data(params);
        return true;
    }

    bool rtp_unpack_priv_impl::unpack_single_frame(uint8_t *data, uint32_t length, const rv_rtp_param& params)
    {
        if (!unpack_frame_start(params))
        {
            return false;
        }

        write_rtp_raw_data(data, length);
        dump_rtp_frame_data(params);
        return true;
    }

    void rtp_unpack_priv_impl::update_pkt_seq(uint16_t rtp_pkt_seq)
    {
        last_pkt_seq_ = rtp_pkt_seq;
    }

    void rtp_unpack_priv_impl::set_priv_frame_type(uint32_t frame_type)
    {
        rtp_priv_frame_.get_priv_header()->uMediaType = frame_type;
    }

    void rtp_unpack_priv_impl::dump_rtp_frame_data(const rv_rtp_param& params)
    {
        rtp_priv_frame_.fit_priv_size();
        priv_chunk_count()++;
        rtp_unpack_direct_impl::dump_rtp_frame_data(rtp_priv_frame_.data(), rtp_priv_frame_.length(), params);
        unpack_state_ = unpack_stat_end;
    }

    bool rtp_unpack_priv_impl::write_rtp_raw_data(const uint8_t *data, uint32_t length)
    {
        return (0 != rtp_priv_frame_.write_rtp_raw_data(data, length));
    }

    uint32_t& rtp_unpack_priv_impl::priv_timestamp()
    {
        return rtp_priv_frame_.get_priv_header()->uTimeStamp;
    }

    uint32_t& rtp_unpack_priv_impl::priv_chunk_count()
    {
        return rtp_priv_frame_.get_priv_header()->uChunkCount;
    }

    bool rtp_unpack_video_priv_impl::unpack_frame_start(uint8_t nal_type, const rv_rtp_param& params)
    {
		//帧数据必然是私有的
        if (!rtp_unpack_priv_impl::unpack_frame_start(params))
        {
            return false;
        }

        write_start_sequence();

        if (is_Iframe(nal_type))
        {
            set_frame_type(OV_VIDEO_I);
        }
        else
        {
            set_frame_type(OV_VIDEO_P);
        }

        return true;
    }

    bool rtp_unpack_video_priv_impl::unpack_single_frame(uint8_t nal_type, uint8_t *data, uint32_t length, const rv_rtp_param& params)
    {
        if (!unpack_frame_start(nal_type, params))
        {
            return false;
        }

        write_rtp_raw_data(data, length);
        dump_rtp_frame_data(params);
        return true;
    }

    void rtp_unpack_video_priv_impl::write_start_sequence()
    {
        static const uint8_t _start_sequence[] = { 0, 0, 0, 1 };
        write_rtp_raw_data(_start_sequence, sizeof(_start_sequence));
    }

    void rtp_unpack_video_priv_impl::set_frame_type(uint32_t frame_type)
    {
        //frame_type_ = frame_type;
        set_priv_frame_type(frame_type);
    }
}
