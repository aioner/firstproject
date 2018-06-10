#include "rtsp_link_impl.h"
#include "rtsp_session_impl.h"
extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);
namespace xt_media_client
{
    rtsp_link_impl::rtsp_link_impl()
        :media_link_ref_t()
    {}

    rtsp_link_impl::~rtsp_link_impl()
    {
        rtsp_link_ref_impl_factory_t::instance()->destory(impl_);
    }

    xt_media_client_status_t rtsp_link_impl::create_link(ports_mgr_t *ports_mgr, const char *uri, uint32_t media_type, bool demux,const char *localip)
    {
        media_link_ref_impl_t *link_impl = NULL;
        xt_media_client_status_t stat = rtsp_link_ref_impl_factory_t::instance()->create(ports_mgr, uri, media_type, demux, &link_impl,localip);
        if (MEDIA_CLIENT_STATUS_OK != stat)
        {
            return stat;
        }

        init(link_impl, media_type);

        return stat;
    }
	xt_media_client_status_t rtsp_link_impl:: create_link(ports_mgr_t *ports_mgr, const char *uri, uint32_t media_type,bool demux, const char *multicast_ip, uint16_t multicast_port)
	{
		media_link_ref_impl_t *link_impl = NULL;
		xt_media_client_status_t stat = rtsp_link_ref_impl_factory_t::instance()->create(ports_mgr, uri, media_type, demux,multicast_ip,multicast_port, &link_impl);
		if (MEDIA_CLIENT_STATUS_OK != stat)
		{
			return stat;
		}

		init(link_impl, media_type);

		return stat;
	}

    xt_media_client_status_t rtsp_link_ref_impl_factory_t::create(ports_mgr_t *ports_mgr, const char *uri, uint32_t stream_type, bool demux, media_link_ref_impl_t **link_impl,const char *localip)
    {
		std::string key = std::string(localip)+"|"+std::string(uri);

        scoped_lock _lock(mutex_);
        factory_map_t::iterator it = factory_.find(key);
        if (factory_.end() == it)
        {
            boost::shared_ptr<rtsp_session_impl> session(new rtsp_session_impl);
            if (!session->connect(uri,localip))
            {
                return MEDIA_CLIENT_STATUS_CONNECT_FAIL;
            }

            std::auto_ptr<media_link_ref_impl_t> link_impl(new (std::nothrow) media_link_ref_impl_t(ports_mgr));
            link_impl->register_session(session);

            xt_media_client_status_t stat = link_impl->create_link(MEDIA_CLIENT_LKMODE_STD, stream_type, demux,localip);
            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                return stat;
            }

           md_log(md_log_info, "rtsp_link_factory_create:link(%#p)", link_impl.get());

            it = factory_.insert(factory_map_t::value_type(key, link_impl.release())).first;
        }

        *link_impl = it->second;

        return MEDIA_CLIENT_STATUS_OK;
    }
	
	xt_media_client_status_t rtsp_link_ref_impl_factory_t::create(ports_mgr_t *ports_mgr, const char *uri, uint32_t stream_type, bool demux, const char *multicast_ip, uint16_t multicast_port, media_link_ref_impl_t **link_impl)
	{
		std::string key(uri);
		scoped_lock _lock(mutex_);
		factory_map_t::iterator it = factory_.find(key);
		if (factory_.end() == it)
		{
			boost::shared_ptr<rtsp_session_impl> session(new rtsp_session_impl);
			if (!session->connect(uri))
			{
				return MEDIA_CLIENT_STATUS_CONNECT_FAIL;
			}

			std::auto_ptr<media_link_ref_impl_t> link_impl(new (std::nothrow) media_link_ref_impl_t(ports_mgr));
			link_impl->register_session(session);

			xt_media_client_status_t stat = link_impl->create_link(MEDIA_CLIENT_LKMODE_STD_MULTI, stream_type, demux,multicast_ip,multicast_port);
			if (MEDIA_CLIENT_STATUS_OK != stat)
			{
				return stat;
			}

			md_log(md_log_info, "rtsp_link_factory_create:link(%#p)", link_impl.get());

			it = factory_.insert(factory_map_t::value_type(key, link_impl.release())).first;
		}

		*link_impl = it->second;

		return MEDIA_CLIENT_STATUS_OK;
	}

    void rtsp_link_ref_impl_factory_t::destory(media_link_ref_impl_t *link_impl)
    {
        bool link_need_close = false;

        if (link_impl)
        {
            if (link_impl->empty())
            {
                scoped_lock _lock(mutex_);
                for (factory_map_t::iterator it = factory_.begin(); factory_.end() != it; ++it)
                {
                    if (it->second == link_impl)
                    {
                        factory_.erase(it);
                        link_need_close = true;
                        break;
                    }
                }
            }
        }

        if (link_need_close)
        {
            md_log(md_log_info, "rtsp_link_factory_destroy:link(%#p)", link_impl);

            link_impl->close();
            delete link_impl;
        }
    }
}

