#include "media_link.h"
#include "rtsp_link_impl.h"
#include "udp_link_impl.h"
#include "tcp_link_impl.h"
#include "ports_mgr_impl.h"
#include "custom_session_impl.h"
#include "rtsp_session_impl.h"
#include "udp_session_impl.h"
#include "tcp_session_impl.h"
#include "push_link_impl.h"

extern  void md_log(const media_client_log_level_t log_level,const char *fmt, ...);

namespace xt_media_client
{
    //static 
    xt_media_client_status_t media_link_factory::create_link(const char *uri, uint32_t media_type, 
		bool demux, xt_media_link_handle_t *phandle,const char *localip)
    {
        if (!rtsp_session_library::is_initialized())
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        std::auto_ptr<rtsp_link_impl> rtsp_link(new rtsp_link_impl);

        xt_media_client_status_t stat = rtsp_link->create_link(ports_mgr_impl::instance(), uri, media_type, demux,localip);
        if (MEDIA_CLIENT_STATUS_OK == stat)
        {
            *phandle = (xt_media_link_handle_t)rtsp_link.release();
        }

        md_log(md_log_info, "create rtsp link,uri(%s),media_type(%d),demux(%d),stat(%d)", uri, media_type, demux, stat);
        return stat;
    }

	//static mutlticast
	xt_media_client_status_t media_link_factory::create_link(const char *uri, uint32_t media_type, bool demux, const char *multicast_ip, uint16_t multicast_port,xt_media_link_handle_t *phandle)
	{
		if (!rtsp_session_library::is_initialized())
		{
			return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
		}

		std::auto_ptr<rtsp_link_impl> rtsp_link(new rtsp_link_impl);

		xt_media_client_status_t stat = rtsp_link->create_link(ports_mgr_impl::instance(), uri, media_type, demux,multicast_ip,multicast_port);
		if (MEDIA_CLIENT_STATUS_OK == stat)
		{
			*phandle = (xt_media_link_handle_t)rtsp_link.release();
		}

		md_log(md_log_info, "create rtsp link,uri(%s),media_type(%d),demux(%d),stat(%d)", uri, media_type, demux, stat);
		return stat;
	}
    //static
    xt_media_client_status_t media_link_factory::create_link(const char *ip, uint16_t port, uint32_t channel, uint32_t link_type, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle)
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;

        *phandle = NULL;
        switch (link_type)
        {
        case LT_UDP_RTP_PRI:
        case LT_UDP_RTP_DEMUX_PRI:
            {
                if (!udp_session_factory::is_initialized())
                {
                    return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
                }

                std::auto_ptr<udp_link_impl> udp_link(new udp_link_impl(ports_mgr_impl::instance()));

                stat = udp_link->create_link(ip, port, channel, media_type, demux);
                if (MEDIA_CLIENT_STATUS_OK == stat)
                {
                    *phandle = (xt_media_link_handle_t)udp_link.release();
                }
                break;
            }
        case LT_TCP_TCP_PRI: //私有tcp会话以及私有tcp流 以rtp流的方式处理
        case LT_TCP_UDP_PRI: //私有tcp会话以及私有udp流 以rtp流的方式处理
        case LT_TCP_RTP_PRI:
        case LT_TCP_RTP_DEMUX_PRI:
            {
                if (!tcp_session_factory::is_initialized())
                {
                    return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
                }

                std::auto_ptr<tcp_link_impl> tcp_link(new tcp_link_impl(ports_mgr_impl::instance()));

                stat = tcp_link->create_link(ip, port, channel, media_type, demux);
                if (MEDIA_CLIENT_STATUS_OK == stat)
                {
                    *phandle = (xt_media_link_handle_t)tcp_link.release();
                }
                break;
            }
        default:
            md_log(md_log_error, "bad link type.->link_type(%d)", link_type);
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
            break;
        }

        md_log(md_log_info, "create link,ip(%s),port(%d),channel(%d), link_type(%d),media_type(%d),demux(%d),stat(%d)", ip, port, channel, link_type, media_type, demux, stat);
        return stat;
    }

    //static
    xt_media_client_status_t media_link_factory::create_link(void *sock, uint32_t channel, uint32_t link_type, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle)
    {
        return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
    }

    //static
    xt_media_client_status_t media_link_factory::create_link(const xt_custom_session_callback_t *session, void *ctx, xt_media_client_link_mode mode, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle)
    {
        if ((NULL == session) || (NULL == phandle))
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        media_session_ptr custom_session(new custom_session_impl(session, ctx));

        std::auto_ptr<media_link_impl> custom_link(new media_link_impl(ports_mgr_impl::instance()));
        custom_link->register_session(custom_session);

        xt_media_client_status_t stat = custom_link->create_link(mode, media_type, demux);
        if (MEDIA_CLIENT_STATUS_OK == stat)
        {
            *phandle = (xt_media_link_handle_t)custom_link.release();
        }

        return stat;
    }

    //static
    xt_media_client_status_t media_link_factory::create_link(int track_num, bool demux, xt_media_link_handle_t *phandle)
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;

        if (track_num<=0 || (NULL==phandle))
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        *phandle = NULL;
        std::auto_ptr<push_link_impl> push_link(new push_link_impl(ports_mgr_impl::instance()));

        stat = push_link->create_link(track_num, demux);
        if (MEDIA_CLIENT_STATUS_OK == stat)
        {
            *phandle = (xt_media_link_handle_t)push_link.release();
        }
        return stat;
    }

    xt_media_client_status_t media_link_factory::create_link(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports,xt_media_link_handle_t *phandle)
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;

        if (track_num<=0 || (NULL==phandle))
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        *phandle = NULL;
        std::auto_ptr<push_link_impl> push_link(new push_link_impl(ports_mgr_impl::instance()));

        stat = push_link->create_link(track_num, demux,multicast,multicastip,multiports);
        if (MEDIA_CLIENT_STATUS_OK == stat)
        {
            *phandle = (xt_media_link_handle_t)push_link.release();
        }
        return stat;

    }

    //static
    xt_media_client_status_t media_link_factory::create_link(const char *ip, uint16_t port, uint32_t channel, uint32_t link_type, uint32_t media_type, const char *multicast_ip, uint16_t multicast_port, xt_media_link_handle_t *phandle)
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;

        *phandle = NULL;
        switch (link_type)
        {
        case LT_UDP_RTP_PRI:
        case LT_UDP_RTP_DEMUX_PRI:
            {
                if (!udp_session_factory::is_initialized())
                {
                    return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
                }

                std::auto_ptr<udp_link_impl> udp_link(new udp_link_impl(ports_mgr_impl::instance()));

                stat = udp_link->create_link(ip, port, channel, media_type, multicast_ip, multicast_port);
                if (MEDIA_CLIENT_STATUS_OK == stat)
                {
                    *phandle = (xt_media_link_handle_t)udp_link.release();
                }
                break;
            }

        default:
            md_log(md_log_error, "bad link type.->link_type(%d)", link_type);
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
            break;
        }

        md_log(md_log_info, "create link,ip(%s),port(%d),channel(%d), link_type(%d),media_type(%d),multicast(%s-%u),stat(%d)", ip, port, channel, link_type, media_type, multicast_ip, multicast_port, stat);
        return stat;
    }

    //static
    media_link_t *media_link_factory::query_link(xt_media_link_handle_t handle)
    {
        return static_cast<media_link_t *>(handle);
    }

    //static
    bool media_link_factory::destory_link(xt_media_link_handle_t handle)
    {
        media_link_t *link = query_link(handle);
        if (NULL != link)
        {
            link->play(NULL, NULL);
            instance()->push_close_link(link);
        }
        return true;
    }

    void media_link_factory::init()
    {
        repeat_thread::start_thread();
    }

    media_link_factory::~media_link_factory()
    {
        term();
    }

    void media_link_factory::push_close_link(media_link_t *link)
    {
        link_close_queue_.push(link);
    }

    void media_link_factory::term()
    {
        link_close_queue_.close();
        repeat_thread::close_thread();
    }

    void media_link_factory::on_repeat()
    {
        media_link_t *link = NULL;
        bool closed = true;
        link_close_queue_.pull(link, closed);
        if (!closed && (NULL != link))
        {
            link->close();
            delete link;
        }
    }
}
