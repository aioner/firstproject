#ifndef _RTP_UNPACK_H_INCLUDED
#define _RTP_UNPACK_H_INCLUDED

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtp.h"
#include "novtable.h"

#include <string>
#include <boost/shared_ptr.hpp>

//#define _USE_RTP_TIMESTAMP_CHECK

namespace xt_media_client
{
    class MEDIA_CLIENT_NO_VTABLE frame_data_dump_callback_t
    {
    public:
        virtual void on_frame_dump(void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc) = 0;
    protected:
        virtual ~frame_data_dump_callback_t() {}
    };

    class MEDIA_CLIENT_NO_VTABLE rtp_unpack_t
    {
    public:
        virtual bool pump_rtp_raw_data(uint8_t *data, uint32_t length, const rtp_fixed_header_t& params) = 0;
        virtual void register_frame_dump_callback(frame_data_dump_callback_t *cb) = 0;
        virtual ~rtp_unpack_t() {}

        static rtp_unpack_t *create(const std::string& format);
    };

    typedef boost::shared_ptr<rtp_unpack_t> rtp_unpack_ptr;
}

#endif //_RTP_UNPACK_H_INCLUDED
