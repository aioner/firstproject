#include "push_link_impl.h"

namespace xt_media_client
{
    push_link_impl::push_link_impl(ports_mgr_t *ports_mgr)
        :media_link_impl(ports_mgr)
    {}

    push_link_impl::~push_link_impl()
    {}

    xt_media_client_status_t push_link_impl::create_link(int track_num, bool demux)
    {
       return media_link_impl::create_link(MEDIA_CLIENT_LKMODE_PUSH, track_num, demux);
    }

    xt_media_client_status_t push_link_impl::create_link(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports)
    {
        return media_link_impl::create_link(MEDIA_CLIENT_LKMODE_PUSH, track_num, demux,multicast,multicastip,multiports);
    }

    xt_media_client_status_t push_link_impl::set_sdp(const std::string& sdp)
    {
      xt_media_client_status_t stat;
      do 
      {
          stat = media_link_impl::set_sdp(sdp);
          if (MEDIA_CLIENT_STATUS_OK != stat) break;

          stat = set_packer(sdp);
          if (MEDIA_CLIENT_STATUS_OK != stat) break;

      } while (false);
      if (MEDIA_CLIENT_STATUS_SDP_EXIST == stat)
      {
          stat = MEDIA_CLIENT_STATUS_OK;
      }
       return stat;
    }
}

