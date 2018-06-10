#ifndef _RTP_UNPACK_H_INCLUDED
#define _RTP_UNPACK_H_INCLUDED

#ifdef _WIN32
#include <windows.h>
#endif

#include "xt_mp_sink_def.h"
#include "novtable.h"

#include <boost/shared_ptr.hpp>

namespace xt_media_client
{
    class frame_data_dump_callback_t;

    class MEDIA_CLIENT_NO_VTABLE rtp_unpack_t
    {
    public:
        virtual bool pump_rtp_raw_data(uint8_t *data, uint32_t length, const rv_rtp_param& params) = 0;
        virtual void register_frame_dump_callback(frame_data_dump_callback_t *cb) = 0;
		virtual void dump_rtp_frame_data(uint8_t *data, uint32_t length, const rv_rtp_param& params)=0;
    protected:
        virtual ~rtp_unpack_t() {}
    };

    typedef boost::shared_ptr<rtp_unpack_t> rtp_unpack_ptr;
}

#endif //_RTP_UNPACK_H_INCLUDED
