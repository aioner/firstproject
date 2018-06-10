#ifndef _RTSP_LINK_IMPL_H_INCLUDED
#define _RTSP_LINK_IMPL_H_INCLUDED


#include "media_link_impl.h"
#include "utility/singleton.h"

namespace xt_media_client
{
	//modified by lichao,20150811 增加rtsp纯音频流点播
    class rtsp_link_impl : public media_link_ref_t
    {
    public:
        rtsp_link_impl();
        ~rtsp_link_impl();

        xt_media_client_status_t create_link(ports_mgr_t *ports_mgr, const char *uri, uint32_t media_type, bool demux,const char *localip);
		xt_media_client_status_t create_link(ports_mgr_t *ports_mgr, const char *uri, uint32_t media_type,bool demux, const char *multicast_ip, uint16_t multicast_port);
    };

    class rtsp_link_ref_impl_factory_t : public xt_utility::singleton<rtsp_link_ref_impl_factory_t>
    {
    public:
        xt_media_client_status_t create(ports_mgr_t *ports_mgr, const char *uri, uint32_t stream_type, bool demux, media_link_ref_impl_t **link_impl,const char *localip);
		xt_media_client_status_t create(ports_mgr_t *ports_mgr, const char *uri, uint32_t stream_type, bool demux, const char *multicast_ip, uint16_t multicast_port,media_link_ref_impl_t **link_impl);
        void destory(media_link_ref_impl_t *link_impl);

    private:
        typedef std::map<std::string, media_link_ref_impl_t *> factory_map_t;
        factory_map_t factory_;
        spinlock_t mutex_;
        typedef spinlock_t::scoped_lock scoped_lock;
    };
}

#endif //_RTSP_LINK_IMPL_H_INCLUDED
