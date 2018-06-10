#include "rtp_unpack_impl.h"

namespace xt_media_client
{
    rtp_unpack_direct_impl::rtp_unpack_direct_impl(uint32_t frame_type, uint32_t data_type)
        :cb_(NULL),
        frame_type_(frame_type),
        data_type_(data_type)
    {}

    bool rtp_unpack_direct_impl::pump_rtp_raw_data(uint8_t *data, uint32_t length, const rtp_fixed_header_t &params)
    {
        dump_rtp_frame_data(data, length, params);
        return true;
    }

    void rtp_unpack_direct_impl::register_frame_dump_callback(frame_data_dump_callback_t *cb)
    {
        cb_ = cb;
    }

    void rtp_unpack_direct_impl::dump_rtp_frame_data(uint8_t *data, uint32_t length, const rtp_fixed_header_t &params)
    {
        if (NULL != cb_)
        {
            cb_->on_frame_dump(data, length, frame_type_, data_type_, params.timestamp, params.ssrc);
        }
    }

    bool rtp_unpack_priv_impl::pump_rtp_raw_data(uint8_t *data, uint32_t length, const rtp_fixed_header_t &params)
    {
        rtp_unpack_scoped_update_seq update_seq(this, params.seq);
        return unpack_single_frame(data, length, params);
    }

    void rtp_unpack_priv_impl::on_lost_frame()
    {
        priv_chunk_count()++;
        unpack_state_ = unpack_stat_end;
    }

    bool rtp_unpack_priv_impl::unpack_frame_start(const rtp_fixed_header_t& params)
    {
        //遇到不连续的帧，按丢包处理，作为新的一帧开始并修复上一包序号
        if ((unpack_stat_uninit != unpack_state_) && !is_cont_pkt_seq(last_pkt_seq_, params.seq))
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

    bool rtp_unpack_priv_impl::unpack_frame_cont(const rtp_fixed_header_t& params)
    {
        //序号不连续或者同一个fu帧内部的包时间戳不一致，按丢包处理
        if (!is_cont_pkt_seq(last_pkt_seq_, params.seq) 
#ifdef _USE_RTP_TIMESTAMP_CHECK
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

    bool rtp_unpack_priv_impl::unpack_frame_end(const rtp_fixed_header_t& params)
    {
        //序号不连续或者同一个fu帧内部的包时间戳不一致，按丢包处理
        if (!is_cont_pkt_seq(last_pkt_seq_, params.seq)
#ifdef _USE_RTP_TIMESTAMP_CHECK
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

    bool rtp_unpack_priv_impl::unpack_single_frame(uint8_t *data, uint32_t length, const rtp_fixed_header_t& params)
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

    void rtp_unpack_priv_impl::dump_rtp_frame_data(const rtp_fixed_header_t& params)
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

    bool rtp_unpack_video_priv_impl::unpack_frame_start(uint8_t nal_type, const rtp_fixed_header_t& params)
    {
        if (!rtp_unpack_priv_impl::unpack_frame_start(params))
        {
            return false;
        }

        write_start_sequence(nal_type);

        if (is_priv_Iframe(nal_type))
        {
            set_frame_type(OV_VIDEO_I);
        }
        else
        {
            set_frame_type(OV_VIDEO_P);
        }

        return true;
    }

    bool rtp_unpack_video_priv_impl::unpack_single_frame(uint8_t nal_type, uint8_t *data, uint32_t length, const rtp_fixed_header_t& params)
    {
        if (!unpack_frame_start(nal_type, params))
        {
            return false;
        }

        write_rtp_raw_data(data, length);
        dump_rtp_frame_data(params);
        return true;
    }

    void rtp_unpack_video_priv_impl::write_start_sequence(uint8_t nal_type)
    {
        static const uint8_t _start_sequence1[] = { 0, 0, 1 };
        static const uint8_t _start_sequence2[] = { 0, 0, 0, 1 };

        if (is_Iframe(nal_type))
        {
            write_rtp_raw_data(_start_sequence1, sizeof(_start_sequence1));
        }
        else
        {
            write_rtp_raw_data(_start_sequence2, sizeof(_start_sequence2));
        }
    }

    void rtp_unpack_video_priv_impl::set_frame_type(uint32_t frame_type)
    {
        frame_type_ = frame_type;
        set_priv_frame_type(frame_type);
    }
}
