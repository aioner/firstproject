#ifndef _RTP_UNPACK_H264_H_INCLUDED
#define _RTP_UNPACK_H264_H_INCLUDED

#include "rtp_unpack_impl.h"

#define RTP_H264_FRAME_NORMAL_SIZE             (256 * 1024)

namespace xt_media_client
{
    enum H264_nalu_type
    {
        NALU_Unspecified = 0,
        NALU_NON_IDR = 1,
        NALU_DataPartitionA,
        NALU_DataPartitionB,
        NALU_DataPartitionC,
        NALU_IDR = 5,
        NALU_SEI = 6,
        NALU_SPS = 7,
        NALU_PPS = 8,
        NALU_Delimiter = 9,
        NALU_SequenceEnd,
        NALU_StreamEnd,
        NALU_FillerData,
        NALU_SPSExetrnsion = 13,
        NALU_Prefix,
        NALU_Subset_SPS
    };

    class rtp_unpack_h264_impl : public rtp_unpack_video_priv_impl
    {
    public:
        rtp_unpack_h264_impl()
            :rtp_unpack_video_priv_impl(RTP_H264_FRAME_NORMAL_SIZE, RTP_VIDEO_FRAME_MAX_SIZE, OV_VIDEO_I, OV_VIDEO_I, DEVICE_VGA_DATA_TYPE)
        {}

        bool pump_rtp_raw_data(uint8_t *data, uint32_t length, const rtp_fixed_header_t& params);

    private:
        bool is_priv_Iframe(uint8_t nal_type)
        {
            return ((NALU_IDR == nal_type) || (NALU_SEI == nal_type) || (NALU_SPS == nal_type) || (NALU_PPS == nal_type));
        }

        bool is_Iframe(uint8_t nal_type)
        {
            return (NALU_IDR == nal_type);
        }
    };
}

#endif //_RTP_UNPACK_H264_H_INCLUDED
