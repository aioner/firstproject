#include "rtp_unpack_h265.h"

#define RTP_HEVC_PAYLOAD_HEADER_SIZE  2
#define RTP_HEVC_FU_HEADER_SIZE       1
#define RTP_HEVC_DONL_FIELD_SIZE      2
#define HEVC_SPECIFIED_NAL_UNIT_TYPES 48

namespace xt_media_client
{
    bool rtp_unpack_h265_impl::pump_rtp_raw_data(uint8_t *data, uint32_t length, const rv_rtp_param& params)
    {
        rtp_unpack_scoped_update_seq update_seq(this, params.sequenceNumber);

        const uint8_t *rtp_pl = data;
        int tid = 0;
        int lid = 0;
        int nal_type = 0;
        int first_fragment = 0;
        int last_fragment = 0;
        int fu_type = 0;
        uint8_t new_nal_header[2] = { 0 };

        /* sanity check for size of input packet: 1 byte payload at least */
        if (length < RTP_HEVC_PAYLOAD_HEADER_SIZE + 1)
        {
            on_lost_frame();
            return false;
        }

        /*
          decode the HEVC payload header according to section 4 of draft version 6:

             0                   1
             0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |F|   Type    |  LayerId  | TID |
            +-------------+-----------------+

               Forbidden zero (F): 1 bit
               NAL unit type (Type): 6 bits
               NUH layer ID (LayerId): 6 bits
               NUH temporal ID plus 1 (TID): 3 bits
        */
        nal_type =  (data[0] >> 1) & 0x3f;
        lid  = ((data[0] << 5) & 0x20) | ((data[1] >> 3) & 0x1f);
        tid  =   data[1] & 0x07;

        /* sanity check for correct layer ID */
        if (lid) 
        {
            /* future scalable or 3D video coding extensions */
            on_lost_frame();
            return false;
        }

        /* sanity check for correct temporal ID */
        if (!tid) 
        {
           on_lost_frame();
           return false;
        }

        /* sanity check for correct NAL unit type */
        if (nal_type > 50) 
        {
            on_lost_frame();
            return false;
        }

        switch (nal_type) 
        {
        /* aggregated packets (AP) */
        case H265_NAL_AP:
            /* pass the HEVC payload header */
            data += RTP_HEVC_PAYLOAD_HEADER_SIZE;
            length -= RTP_HEVC_PAYLOAD_HEADER_SIZE;


            /* pass the HEVC DONL field */
            if (using_donl_field_) 
            {
                data += RTP_HEVC_DONL_FIELD_SIZE;
                length -= RTP_HEVC_DONL_FIELD_SIZE;
            }

            /* fall-through */
        /* video parameter set (VPS) */
        case H265_NAL_VPS:
        /* sequence parameter set (SPS) */
        case H265_NAL_SPS:
        /* picture parameter set (PPS) */
        case H265_NAL_PPS:
        /*  supplemental enhancement information (SEI) */
        case H265_NAL_SEI:
        /* single NAL unit packet */
        default:
            /* sanity check for size of input packet: 1 byte payload at least */
            if (length < 1)
            {
                on_lost_frame();
                return false;
            }

            /* A/V packet: copy start sequence */
            unpack_single_frame(nal_type, data, length, params);

            break;
        /* fragmentation unit (FU) */
        case 49:
            /* pass the HEVC payload header */
            data += RTP_HEVC_PAYLOAD_HEADER_SIZE;
            length -= RTP_HEVC_PAYLOAD_HEADER_SIZE;

            /*
                 decode the FU header

                  0 1 2 3 4 5 6 7
                 +-+-+-+-+-+-+-+-+
                 |S|E|  FuType   |
                 +---------------+

                    Start fragment (S): 1 bit
                    End fragment (E): 1 bit
                    FuType: 6 bits
            */
            first_fragment = data[0] & 0x80;
            last_fragment  = data[0] & 0x40;
            fu_type        = data[0] & 0x3f;

            /* pass the HEVC FU header */
            data += RTP_HEVC_FU_HEADER_SIZE;
            length -= RTP_HEVC_FU_HEADER_SIZE;

            /* pass the HEVC DONL field */
            if (using_donl_field_) 
            {
                data += RTP_HEVC_DONL_FIELD_SIZE;
                length -= RTP_HEVC_DONL_FIELD_SIZE;
            }

            /* sanity check for size of input packet: 1 byte payload at least */
            if (length < 1)
            {
                on_lost_frame();
                return false;
            }

            new_nal_header[0] = (rtp_pl[0] & 0x81) | (fu_type << 1);
            new_nal_header[1] = rtp_pl[1];

            /* start fragment vs. subsequent fragments */
            if (first_fragment) 
            {
                unpack_frame_start(fu_type, params);
                write_rtp_raw_data(new_nal_header, sizeof(new_nal_header));
            }
            else
            {
                if (!unpack_frame_cont(params))
                {
                    return false;
                }
            }

            write_rtp_raw_data(data, length);

            if (last_fragment)
            {
                unpack_frame_end(params);
            }
            break;
        /* PACI packet */
        case 50:
            /* Temporal scalability control information (TSCI) */
            on_lost_frame();
            return false;
            break;
        }

        return true;
    }
}
