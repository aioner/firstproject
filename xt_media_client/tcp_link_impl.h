#ifndef _TCP_LINK_IMPL_H_INCLUDED
#define _TCP_LINK_IMPL_H_INCLUDED

#include "media_link_impl.h"

namespace xt_media_client
{
    class tcp_link_impl : public media_link_impl
    {
    public:
        explicit tcp_link_impl(ports_mgr_t *ports_mgr);
        ~tcp_link_impl();

        xt_media_client_status_t create_link(const char *ip, uint16_t port, uint32_t channel, uint32_t media_type, bool demux);
    };
}

#endif //_TCP_LINK_IMPL_H_INCLUDED
