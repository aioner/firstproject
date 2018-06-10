#include "udp_link_impl.h"
#include "udp_session_impl.h"

namespace xt_media_client
{
    udp_link_impl::udp_link_impl(ports_mgr_t *ports_mgr)
        :media_link_impl2(ports_mgr)
    {}

    udp_link_impl::~udp_link_impl()
    {
        udp_session_factory::instance()->destroy_session(get_session());
    }

    xt_media_client_status_t udp_link_impl::create_link(const char *ip, uint16_t port, uint32_t channel, uint32_t media_type, bool demux)
    {
        media_session_ptr session;
        xt_media_client_status_t stat = udp_session_factory::instance()->create_session(ip, port, channel, session);
        if (MEDIA_CLIENT_STATUS_OK != stat)
        {
            return stat;
        }

        register_session(session);

        return media_link_impl::create_link(MEDIA_CLIENT_LKMODE_PRIV_FAST, media_type, demux);
    }

    xt_media_client_status_t udp_link_impl::create_link(const char *ip, uint16_t port, uint32_t channel, uint32_t media_type, const char *multicast_ip, uint16_t multicast_port)
    {
        media_session_ptr session;
        xt_media_client_status_t stat = udp_session_factory::instance()->create_session(ip, port, channel, session);
        if (MEDIA_CLIENT_STATUS_OK != stat)
        {
            return stat;
        }

        register_session(session);

        return media_link_impl::create_link(multicast_ip, multicast_port);
    }
}