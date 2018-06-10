#include "rtp_unpack_h264.h"

namespace xt_media_client
{
    namespace detail
    {
        inline uint16_t AV_RB16(uint8_t *n)
        {
            return ((n[0] << 8) | n[1]);
        }
    }

    bool rtp_unpack_h264_impl::pump_rtp_raw_data(uint8_t *data, uint32_t length, const rtp_fixed_header_t& params)
    {
        rtp_unpack_scoped_update_seq update_seq(this, params.seq);

        uint8_t nal = data[0];
        uint8_t type = nal & 0x1f;

        if ((type >= 0) && (type <= 23))
        {
            unpack_single_frame(type, data, length, params);
        }
        else if (24 == type)
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
        else if (28 == type)
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