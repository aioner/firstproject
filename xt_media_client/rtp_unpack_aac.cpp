#include "rtp_unpack_aac.h"

namespace xt_media_client
{
    bool rtp_unpack_aac_impl::pump_rtp_raw_data(uint8_t *data, uint32_t length, const rv_rtp_param &params)
    {
        rtp_unpack_scoped_update_seq update_seq(this, params.sequenceNumber);

        if (length > 4)
        {
            unpack_single_frame(data + 4, length - 4, params);
        }
        else
        {
            on_lost_frame();
        }

        return true;
    }
}
