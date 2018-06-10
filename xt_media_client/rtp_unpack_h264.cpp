#include "rtp_unpack_h264.h"
extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);

namespace xt_media_client
{
    namespace detail
    {
        inline uint16_t AV_RB16(uint8_t *n)
        {
            return ((n[0] << 8) | n[1]);
        }
    }

    bool rtp_unpack_h264_impl::pump_rtp_raw_data(uint8_t *data, uint32_t length, const rv_rtp_param& params)
    {
		//该对象析构的时候将调用p_->update_pkt_seq(seq_)更新sn号
        rtp_unpack_scoped_update_seq update_seq(this, params.sequenceNumber);

        uint8_t nal = data[0];
        uint8_t type = nal & 0x1f;

        //md_log(md_log_debug, "timestamp(%u),sequenceNumber(%u),extSequenceNumber(%u)", params.timestamp, params.sequenceNumber, params.extSequenceNumber);

        if ((type >= 0) && (type <= 23))
        {
            unpack_single_frame(type, data, length, params);
        }
        else if (24 == type) //单rtp包组成的一帧
        {
            data++;
            length--;

            while (length > 2)
            {
                uint16_t nal_size = detail::AV_RB16(data);

                data += 2;
                length -= 2;

                if (nal_size <= length)
                {
                    unpack_single_frame(type, data, length, params);
                }
                else
                {
                    on_lost_frame();
                    return false;
                }

                data += nal_size;
                length -= nal_size;
            }  //while (len > 2) 
        }
        else if (28 == type)//多rtp包组成的一帧
        {
            data++;
            length--;

            if (length > 1)
            {
                uint8_t fu_indicator  = nal;
                uint8_t fu_header = *data;
                uint8_t start_bit = fu_header >> 7;
                uint8_t end_bit = (fu_header & 0x40) >> 6;
                uint8_t nal_type = fu_header & 0x1f;
                uint8_t reconstructed_nal;

                reconstructed_nal = fu_indicator & 0xe0;
                reconstructed_nal |= nal_type;

                data++;
                length--;

                if (start_bit)
                {
                    unpack_frame_start(nal_type, params);
                    write_rtp_raw_data(&reconstructed_nal, 1);
                }
                else
                {
                    if (!unpack_frame_cont(params))
                    {
                        return false;
                    }
                }

                write_rtp_raw_data(data, length);

                if (end_bit)
                {
                    unpack_frame_end(params);
                }
            }
        }
        else
        {
            on_lost_frame();
            return false;
        }

        return true;
    }
}