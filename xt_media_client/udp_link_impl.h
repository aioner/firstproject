#ifndef _UDP_LINK_IMPL_H_INCLUDED
#define _UDP_LINK_IMPL_H_INCLUDED

#include "media_link_impl.h"

namespace xt_media_client
{
    class udp_link_impl : public media_link_impl2
    {
    public:
        explicit udp_link_impl(ports_mgr_t *ports_mgr);
        ~udp_link_impl();

        xt_media_client_status_t create_link(const char *ip, uint16_t port, uint32_t channel, uint32_t media_type, bool demux);
        xt_media_client_status_t create_link(const char *ip, uint16_t port, uint32_t channel, uint32_t media_type, const char *multicast_ip, uint16_t multicast_port);
    };
}

#endif //_UDP_LINK_IMPL_H_INCLUDED
