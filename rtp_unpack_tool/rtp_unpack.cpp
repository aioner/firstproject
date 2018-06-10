#include "rtp_unpack.h"

#include "rtp_unpack_h264.h"
#include "rtp_unpack_h265.h"
#include "rtp_unpack_aac.h"

using namespace xt_media_client;

rtp_unpack_t *rtp_unpack_t::create(const std::string &format)
{
    if ("g711" == format)
    {
        return new rtp_unpack_priv_impl(RTP_AUDIO_FRAME_NORMAL_SIZE, RTP_AUDIO_FRAME_MAX_SIZE, OV_AUDIO, OV_AUDIO, DEVICE_VGA_DATA_TYPE);
    }

    if ("aac" == format)
    {
        return new rtp_unpack_aac_impl;
    }

    if ("h264" == format)
    {
        return new rtp_unpack_h264_impl;
    }

    if ("h265" == format)
    {
        return new rtp_unpack_h265_impl;
    }

    return NULL;
}

