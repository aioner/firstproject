#include "tcp_link_impl.h"
#include "tcp_session_impl.h"

namespace xt_media_client
{
    tcp_link_impl::tcp_link_impl(ports_mgr_t *ports_mgr)
        :media_link_impl(ports_mgr)
    {}

    tcp_link_impl::~tcp_link_impl()
    {
        tcp_session_factory::instance()->destroy_session(get_session());
    }

    xt_media_client_status_t tcp_link_impl::create_link(const char *ip, uint16_t port, uint32_t channel, uint32_t media_type, bool demux)
    {
        media_session_ptr session;
        xt_media_client_status_t stat = tcp_session_factory::instance()->create_session(ip, port, channel, session);
        if (MEDIA_CLIENT_STATUS_OK != stat)
        {
            return stat;
        }

        register_session(session);

        return media_link_impl::create_link(MEDIA_CLIENT_LKMODE_PRIV_FAST, media_type, demux);
    }

}