#ifndef _COMMON_ERROR_H_INCLUDED
#define _COMMON_ERROR_H_INCLUDED

namespace udp_session
{
    struct error
    {
        enum error_code
        {
            ok = 0,
            invalid_arg,
            msg_code_not_supported,
            channel_not_exists,
            timeout,
            send_msg_fail,
            channel_not_stop,
            get_sdp_cb_fail,
            add_sink_cb_fail,
            del_sink_cb_fail,
            send_data_cb_fail,
            send_data_content_too_long,
            bad_ip_addr
        };
    };
}

#endif //_COMMON_ERROR_H_INCLUDED
