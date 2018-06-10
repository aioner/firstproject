#ifndef _RTP_UNPACK_ACC_H_INCLUDED
#define _RTP_UNPACK_ACC_H_INCLUDED

#include "rtp_unpack_impl.h"

#define RTP_AAC_FRAME_NORMAL_SIZE       (8 * 1024)

namespace xt_media_client
{
    class rtp_unpack_aac_impl : public rtp_unpack_priv_impl
    {
    public:
        rtp_unpack_aac_impl()
            :rtp_unpack_priv_impl(RTP_AAC_FRAME_NORMAL_SIZE, RTP_AUDIO_FRAME_MAX_SIZE, OV_AUDIO, OV_AAC, DEVICE_VGA_DATA_TYPE)
        {}
        bool pump_rtp_raw_data(uint8_t *data, uint32_t length, const rv_rtp_param &params);
    };
}

#endif //_RTP_UNPACK_ACC_H_INCLUDED
