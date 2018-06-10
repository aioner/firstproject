#ifndef _RTP_UNPACK_IMPL_H_INCLUDED
#define _RTP_UNPACK_IMPL_H_INCLUDED

#include "rtp_unpack.h"
#include "xt_media_client_types.h"
#include "rtp_buf.h"

#include <string.h>

#define CHUNK_HEADER_FOURCC                 'HUHC' 
#define RTP_PACK_BUF_LEN                    (1024 * 1024)
#define DEVICE_VGA_DATA_TYPE                172
#define RTP_VIDEO_FRAME_NORMAL_SIZE         (256 * 1024)
#define RTP_AUDIO_FRAME_NORMAL_SIZE         (8 * 1024)
#define RTP_VIDEO_FRAME_MAX_SIZE            (4 * 1024 * 1024)
#define RTP_AUDIO_FRAME_MAX_SIZE            (64 * 1024)
#define RTP_FRAME_NORMAL_SIZE               RTP_VIDEO_FRAME_NORMAL_SIZE
#define RTP_FRAME_MAX_SIZE                  RTP_VIDEO_FRAME_MAX_SIZE

namespace xt_media_client
{
    // 公司私有头
    struct rtp_priv_header_t
    {
        unsigned uFourCC;       //识别标志 CHUNK_HEADER_FOURCC
        unsigned uHeaderSize;   //帧头长度 sizeof(SChunkHeader)
        unsigned uMediaType;    //帧类型
        unsigned uChunkCount;   //流水序号
        unsigned uDataSize;     //数据长度(不含头结构)
        unsigned uTimeStamp;    //帧时间戳
    };

    class rtp_unpack_direct_impl : public rtp_unpack_t
    {
    public:
        rtp_unpack_direct_impl(uint32_t frame_type, uint32_t data_type);

        bool pump_rtp_raw_data(uint8_t *data, uint32_t length, const rv_rtp_param& params);
        void register_frame_dump_callback(frame_data_dump_callback_t *cb);

    protected:
        void dump_rtp_frame_data(uint8_t *data, uint32_t length, const rv_rtp_param& params);
        frame_data_dump_callback_t *cb_;
        uint32_t frame_type_;
        uint32_t data_type_;
    };

    class rtp_priv_frame_data_t
    {
    public:
        rtp_priv_frame_data_t(uint32_t buf_capacity, uint32_t buf_max_bound, uint32_t priv_header_frame_type)
            :buf_(buf_capacity, buf_max_bound)
        {
            rewind();

            get_priv_header()->uHeaderSize = sizeof(rtp_priv_header_t);
            get_priv_header()->uFourCC = CHUNK_HEADER_FOURCC;
            get_priv_header()->uMediaType = priv_header_frame_type;
            get_priv_header()->uChunkCount = 0;
            get_priv_header()->uDataSize = 0;
            get_priv_header()->uTimeStamp = 0;
        }

        rtp_priv_header_t *get_priv_header()
        {
            return reinterpret_cast<rtp_priv_header_t *>(buf_.data());
        }

        void fit_priv_size()
        {
            uint32_t total = buf_.length();
            if (total > sizeof(rtp_priv_header_t))
            {
                get_priv_header()->uDataSize = total - sizeof(rtp_priv_header_t);
            }
            else
            {
                get_priv_header()->uDataSize = 0;
            }
        }

        uint32_t write_rtp_raw_data(const uint8_t *payload, uint32_t len)
        {
            return buf_.write(payload, len);
        }

        void rewind()
        {
            buf_.rewind();
            buf_.seek(sizeof(rtp_priv_header_t));
        }

        uint8_t *data()
        {
            return buf_.data();
        }

        uint32_t length() const
        {
            return buf_.length();
        }

    private:
        rtp_buf_t buf_;
    };

    class rtp_unpack_priv_impl : public rtp_unpack_direct_impl
    {
    public:
        enum unpack_state_t
        {
            unpack_stat_uninit = -1,
            unpack_stat_error,
            unpack_stat_start,
            unpack_stat_cont,
            unpack_stat_end
        };

        rtp_unpack_priv_impl(uint32_t buf_capacity, uint32_t buf_max_bound, uint32_t priv_header_frame_type, uint32_t frame_type,uint32_t data_type)
            :rtp_unpack_direct_impl(frame_type, data_type),
            rtp_priv_frame_(buf_capacity, buf_max_bound, priv_header_frame_type),
            unpack_state_(unpack_stat_uninit),
            last_pkt_seq_(0)
        {}

        bool pump_rtp_raw_data(uint8_t *data, uint32_t length, const rv_rtp_param& params);
    protected:
        void on_lost_frame();

        bool unpack_frame_start(const rv_rtp_param& params);
        bool unpack_frame_cont(const rv_rtp_param& params);
        bool unpack_frame_end(const rv_rtp_param& params);

        bool unpack_single_frame(uint8_t *data, uint32_t length, const rv_rtp_param& params);

        friend class rtp_unpack_scoped_update_seq;
        void update_pkt_seq(uint16_t rtp_pkt_seq);
        void set_priv_frame_type(uint32_t frame_type);

        void dump_rtp_frame_data(const rv_rtp_param& params);
        bool write_rtp_raw_data(const uint8_t *data, uint32_t length);

        uint32_t& priv_timestamp();
        uint32_t& priv_chunk_count();

        static bool is_cont_pkt_seq(uint16_t last_pkt_seq, uint16_t cur_pkt_seq)
        {
            return ((uint16_t)(last_pkt_seq + 1) == cur_pkt_seq);
        }
        static bool unpack_state_start_prepared(unpack_state_t stat)
        {
            return ((unpack_stat_uninit == stat) || (unpack_stat_end == stat));
        }
        static bool unpack_state_cont_prepared(unpack_state_t stat)
        {
            return ((unpack_stat_start == stat) || (unpack_stat_cont == stat));
        }
        static bool unpack_state_end_prepared(unpack_state_t stat)
        {
            return ((unpack_stat_start == stat) || (unpack_stat_cont == stat));
        }
    private:
        rtp_priv_frame_data_t rtp_priv_frame_;
        unpack_state_t unpack_state_;
        uint16_t last_pkt_seq_;
    };

    class rtp_unpack_scoped_update_seq
    {
        rtp_unpack_priv_impl *p_;
        uint16_t seq_;
    public:
        rtp_unpack_scoped_update_seq(rtp_unpack_priv_impl *p, uint16_t seq)
            :p_(p),
            seq_(seq)
        {}

        ~rtp_unpack_scoped_update_seq()
        {
            if (p_)
            {
                p_->update_pkt_seq(seq_);
            }
        }
    };

    class rtp_unpack_video_priv_impl : public rtp_unpack_priv_impl
    {
    public:
        rtp_unpack_video_priv_impl(uint32_t buf_capacity, uint32_t buf_max_bound, uint32_t priv_header_frame_type, uint32_t frame_type,  uint32_t data_type)
            :rtp_unpack_priv_impl(buf_capacity, buf_max_bound, priv_header_frame_type, frame_type, data_type)
        {}

    protected:
        bool unpack_frame_start(uint8_t nal_type, const rv_rtp_param& params);
        bool unpack_single_frame(uint8_t nal_type, uint8_t *data, uint32_t length, const rv_rtp_param& params);
        void write_start_sequence();
        void set_frame_type(uint32_t frame_type);

        virtual bool is_Iframe(uint8_t nal_type) = 0;
    };
}

#endif //_RTP_UNPACK_IMPL_H_INCLUDED
