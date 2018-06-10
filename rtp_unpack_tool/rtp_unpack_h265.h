#ifndef _RTP_UNPACK_H265_H_INCLUDED
#define _RTP_UNPACK_H265_H_INCLUDED

#include "rtp_unpack_impl.h"

#define RTP_H265_FRAME_NORMAL_SIZE              (256 * 1024)

namespace xt_media_client
{
    enum H265_nal_type
    {
        H265_NAL_IDR_W_RADL = 19,
        H265_NAL_IDR_N_LP = 20,
        H265_NAL_VPS = 32,
        H265_NAL_SPS = 33,
        H265_NAL_PPS = 34,
        H265_NAL_SEI = 39,
        H265_NAL_AP = 48,
        H265_NAL_FU = 49,
        H265_NAL_PACI = 50
    };

    class rtp_unpack_h265_impl : public rtp_unpack_video_priv_impl
    {
    public:
        explicit rtp_unpack_h265_impl(bool using_donl_field = false)
            :rtp_unpack_video_priv_impl(RTP_H265_FRAME_NORMAL_SIZE, RTP_VIDEO_FRAME_MAX_SIZE, OV_VIDEO_I, OV_VIDEO_I, DEVICE_VGA_DATA_TYPE),
            using_donl_field_(using_donl_field)
        {}

        bool pump_rtp_raw_data(uint8_t *data, uint32_t length, const rtp_fixed_header_t& params);

    private:
        bool is_priv_Iframe(uint8_t nal_type)
        {
            return ((H265_NAL_VPS == nal_type) 
                || (H265_NAL_SPS == nal_type) 
                || (H265_NAL_PPS == nal_type) 
                || (H265_NAL_SEI == nal_type)
                || (H265_NAL_IDR_W_RADL == nal_type)
                || (H265_NAL_IDR_N_LP == nal_type));
        }

        bool is_Iframe(uint8_t nal_type)
        {
            return ((H265_NAL_IDR_W_RADL == nal_type) || (H265_NAL_IDR_N_LP == nal_type));
        }

        bool using_donl_field_;
    };
}

#endif //_RTP_UNPACK_H265_H_INCLUDED
