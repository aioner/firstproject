#ifndef _PUSH_LINK_IMPL_H_INCLUDED
#define _PUSH_LINK_IMPL_H_INCLUDED


#include "media_link_impl.h"

namespace xt_media_client
{
    class push_link_impl : public media_link_impl
    {
    public:
        explicit push_link_impl(ports_mgr_t *ports_mgr);
        ~push_link_impl();

        xt_media_client_status_t create_link(int track_num, bool demux);
        xt_media_client_status_t create_link(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports); 
        virtual xt_media_client_status_t set_sdp(const std::string& sdp);
    };
}

#endif //_PUSH_LINK_IMPL_H_INCLUDED
