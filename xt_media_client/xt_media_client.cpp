#include <boost/asio/ip/address_v4.hpp>
#include "xt_media_client.h"
#include "ports_mgr_impl.h"
#include "media_link.h"
#include "media_link_impl.h"
#include "udp_session_impl.h"
#include "rtsp_session_impl.h"
#include "rtsp_link_impl.h"      //added by lichao,20150811 增加rtsp纯音频流点播
#include "tcp_session_impl.h"
#include "rtp_sink_impl.h"
#include "sdp_parser.h"
#include <stdarg.h>
#include "msec_timer_mgr.h"
using namespace xt_media_client;
const char* XT_MEDIA_CLIENT_LIB_INFO = "XT_Lib_Version: V_XT_Media_Client_4.26.0105.0";
xt_media_client_cfg_t g_md_cfg;

void md_log(const media_client_log_level_t log_level,const char *fmt, ...)
{
    if (g_md_cfg.media_log_callback)
    {
        va_list args;
        va_start(args, fmt);
        char log_buf[2048]={0};
#ifdef _WIN32
        ::vsnprintf_s(log_buf, 2048, 2048-1, fmt, args);
#else
        ::vsnprintf(log_buf, 2048, fmt, args);
#endif
        va_end(args);
        g_md_cfg.media_log_callback("media_client",log_level,log_buf);
    }
}

void TCP_SESSION_CLIENT_STDCALL tcp_session_client_log_cb(const char* log_name,const tcp_sc_level_t log_level,const char* log_ctx)
{
    if (g_md_cfg.media_log_callback)
    {
        media_client_log_level_t level;
        switch (log_level)
        {
        case tcp_sc_log_info:
            {
                level = md_log_info;
                break;
            }
        case tcp_sc_log_error:
            {
                level = md_log_error;
                break;
            }
        case tcp_sc_log_warn:
            {
                level = md_log_warn;
                break;
            }
        case tcp_sc_log_debug:
            {
                level = md_log_debug;
                break;
            }
        default:
            {
                level = md_log_info;
                break;
            }
        }
        g_md_cfg.media_log_callback(log_name,level,log_ctx);
    }
}

void XT_RTSP_CLIENT_STDCALL rtsp_client_log_cb(const char* log_name,const rtsp_client_level_t log_level,const char* log_ctx)
{
    if (g_md_cfg.media_log_callback)
    {
        media_client_log_level_t level;
        switch (log_level)
        {
        case rtsp_c_log_info:
            {
                level = md_log_info;
                break;
            }
        case rtsp_c_log_error:
            {
                level = md_log_error;
                break;
            }
        case rtsp_c_log_warn:
            {
                level = md_log_warn;
                break;
            }
        case rtsp_c_log_debug:
            {
                level = md_log_debug;
                break;
            }
        default:
            {
                level = md_log_info;
                break;
            }
        }
        g_md_cfg.media_log_callback(log_name,level,log_ctx);
    }
}

extern "C"
{

    void init_cfg(const xt_media_client_cfg_t *config)
    {
        if (NULL != config)
        {
            ::strncpy(g_md_cfg.udp_session_bind_ip,config->udp_session_bind_ip,MEDIA_CLIENT_IP_LEN);
            g_md_cfg.udp_session_bind_port = config->udp_session_bind_port;
            g_md_cfg.udp_session_heartbit_proid = config->udp_session_heartbit_proid;
            g_md_cfg.udp_session_request_try_count = config->udp_session_request_try_count;
            g_md_cfg.udp_session_request_one_timeout = config->udp_session_request_one_timeout;
            ::strncpy(g_md_cfg.tcp_session_bind_ip,config->tcp_session_bind_ip,MEDIA_CLIENT_IP_LEN);
            g_md_cfg.tcp_session_bind_port = config->tcp_session_bind_port;
            g_md_cfg.tcp_session_connect_timeout = config->tcp_session_connect_timeout;
            g_md_cfg.tcp_session_login_timeout = config->tcp_session_login_timeout;
            g_md_cfg.tcp_session_play_timeout = config->tcp_session_play_timeout;
            g_md_cfg.tcp_session_stop_timeout = config->tcp_session_stop_timeout;
            g_md_cfg.rtsp_session_connect_timeout = config->rtsp_session_connect_timeout;
            g_md_cfg.rtsp_session_describe_timeout = config->rtsp_session_describe_timeout;
            g_md_cfg.rtsp_session_setup_timeout = config->rtsp_session_setup_timeout;
            g_md_cfg.rtsp_session_play_timeout = config->rtsp_session_play_timeout;
            g_md_cfg.rtsp_session_pause_timeout = config->rtsp_session_pause_timeout;
            g_md_cfg.rtsp_session_teardown_timeout = config->rtsp_session_teardown_timeout;
            g_md_cfg.rtp_rv_thread_num = config->rtp_rv_thread_num;
            g_md_cfg.rtp_sink_thread_num = config->rtp_sink_thread_num;
            g_md_cfg.rtp_post_thread_num = config->rtp_post_thread_num;
            g_md_cfg.ports_mgr_get_callback = config->ports_mgr_get_callback;
            g_md_cfg.ports_mgr_free_callback = config->ports_mgr_free_callback;
            g_md_cfg.media_log_callback = config->media_log_callback;
            g_md_cfg.ports_mgr_ctx = config->ports_mgr_ctx;
            g_md_cfg.session_enable_flags = config->session_enable_flags;
        }
    }
    xt_media_client_status_t xt_media_client_init(const xt_media_client_cfg_t *config)
    {
        if (NULL == config)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }
        init_cfg(config);

        //初始化rtp接收端口配置
		msec_timer_mgr_t::new_instance();
		msec_timer_mgr_t::instance()->start(30*1000);
        ports_mgr_impl::new_instance();
        ports_mgr_impl::instance()->register_callbacks(config->ports_mgr_get_callback, config->ports_mgr_free_callback, config->ports_mgr_ctx);
        ports_mgr_impl::instance()->init_demux_ports(); 
        md_log(md_log_info,"rtp port initialize ok.");

        //初始化udp协商配置
        if (MEDIA_CLIENT_USE_UDP_SESSION & config->session_enable_flags)
        {
            udp_session_factory::new_instance();
            if (!udp_session_factory::instance()->init(config->udp_session_bind_ip, 
                config->udp_session_bind_port,
                config->udp_session_heartbit_proid, 
                config->udp_session_request_try_count, config->udp_session_request_one_timeout))
            {
                md_log(md_log_error,"udp session module initialize failed.");
            }
            else
            {
                md_log(md_log_info,"udp session module initialize ok.bind_addr(%s,%d),heartbit_proid(%d),try_cnt(%d),timeout(%d)", config->udp_session_bind_ip, 
                    config->udp_session_bind_port, config->udp_session_heartbit_proid, config->udp_session_request_try_count, config->udp_session_request_one_timeout);
            }
        }

        //初始化rtsp协商配置
        if (MEDIA_CLIENT_USE_RTSP_SESSION & config->session_enable_flags)
        {
            //sdp parser解析库初始化
            sdp_parser_library::new_instance();
            if (!sdp_parser_library::instance()->init())
            {
                md_log(md_log_error, "sdp parser library initialize failed.");
                return MEDIA_CLIENT_STATUS_ENV_INIT_FAIL;
            }

            md_log(md_log_info, "sdp parser library initialize ok.");

            rtsp_session_library::new_instance();
            if (!rtsp_session_library::instance()->init(config->rtsp_session_connect_timeout, config->rtsp_session_describe_timeout, config->rtsp_session_setup_timeout, 
                config->rtsp_session_play_timeout, config->rtsp_session_pause_timeout, config->rtsp_session_teardown_timeout,rtsp_client_log_cb))
            {
                md_log(md_log_error, "rtsp session module initialize failed.");
            }
            else
            { 
                md_log(md_log_info, "rtsp session module initialize ok.connect_timeout(%d),describe_timeout(%d),setup_timeout(%d),play_timeout(%d),pause_timeout(%d),teardown_timeout(%d)",
                    config->rtsp_session_connect_timeout, config->rtsp_session_describe_timeout, config->rtsp_session_setup_timeout, 
                    config->rtsp_session_play_timeout, config->rtsp_session_pause_timeout, config->rtsp_session_teardown_timeout);

                rtsp_link_ref_impl_factory_t::new_instance();  //added by lichao,20150811 增加rtsp纯音频流点播
            }
        }

        //初始化tcp协商配置
        if (MEDIA_CLIENT_USE_TCP_SESSION & config->session_enable_flags)
        {
            tcp_session_factory::new_instance();
            if (!tcp_session_factory::instance()->init(config->tcp_session_bind_ip, config->tcp_session_bind_port, config->tcp_session_connect_timeout, 
                config->tcp_session_login_timeout, config->tcp_session_play_timeout, config->tcp_session_stop_timeout,tcp_session_client_log_cb))
            {
                md_log(md_log_error, "tcp session module initialize failed.");
            }
            else
            {
                md_log(md_log_info, "tcp session module initialize ok.bind_addr(%s,%d),connect_timeout(%d),login_timeout(%d),play_timeout(%d),stop_timeout(%d)",
                    config->tcp_session_bind_ip, config->tcp_session_bind_port, config->tcp_session_connect_timeout,
                    config->tcp_session_login_timeout, config->tcp_session_play_timeout, config->tcp_session_stop_timeout);
            }
        }

        //初始化mp_sink库配置
        xt_mp_sink_library::new_instance();
        if (!xt_mp_sink_library::instance()->init(config->rtp_rv_thread_num, config->rtp_sink_thread_num, config->rtp_post_thread_num))
        {
            md_log(md_log_error, "xt_mp_sink library initialize failed.");
            return MEDIA_CLIENT_STATUS_ENV_INIT_FAIL;
        }

        md_log(md_log_info, "xt_mp_sink library initialize ok.rtp_rv_thread_num(%d),rtp_sink_thread_num(%d),rtp_post_thread_num(%d)",
            config->rtp_rv_thread_num, config->rtp_sink_thread_num, config->rtp_post_thread_num);

        media_link_impl_base::init();

        md_log(md_log_info, "%s", "media_link_impl_base init");

        media_link_factory::new_instance();

        md_log(md_log_info, "%s", "media_link_factory new_instance");

        media_link_factory::instance()->init();

        md_log(md_log_info, "%s", "media_link_factory init");

        return MEDIA_CLIENT_STATUS_OK;
    }

    void xt_media_client_term()
    {
        media_link_factory::delete_instance();
        media_link_impl_base::uninit();
        xt_mp_sink_library::delete_instance();
        tcp_session_factory::delete_instance();
        rtsp_link_ref_impl_factory_t::delete_instance();   //added by lichao,20150811 增加rtsp纯音频流点播
        rtsp_session_library::delete_instance();
        udp_session_factory::delete_instance();
        sdp_parser_library::delete_instance();
        ports_mgr_impl::delete_instance();
        md_log(md_log_info, "xt_media_client_term ok.");
    }

    xt_media_client_status_t xt_media_client_rtsp_link(const char *uri, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle,const char *localip)
    {
        return media_link_factory::create_link(uri, media_type, demux, phandle,localip);
    }

    xt_media_client_status_t xt_media_client_priv_link(const char *ip, uint16_t port, uint32_t channel, uint32_t link_type, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle)
    {
        return media_link_factory::create_link(ip, port, channel, link_type, media_type, demux, phandle);
    }

    xt_media_client_status_t xt_media_client_socket_link(void *sock, uint32_t channel, uint32_t link_type, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle)
    {
        return media_link_factory::create_link(sock, channel, link_type, media_type, demux, phandle);
    }

    xt_media_client_status_t xt_media_client_custom_link(const xt_custom_session_callback_t *session, void *ctx, xt_media_client_link_mode mode, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle)
    {
        return media_link_factory::create_link(session, ctx, mode, media_type, demux, phandle);
    }

    void xt_media_client_close_link(xt_media_link_handle_t link)
    {
        media_link_factory::destory_link(link);
    }

    xt_media_client_status_t xt_media_client_query_prof(xt_media_link_handle_t handle, xt_rtp_prof_info_t *prof)
    {
        media_link_t *impl = media_link_factory::query_link(handle);
        if (NULL == impl)
        {
            return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
        }

        return impl->query_prof_info(prof);
    }

    xt_media_client_status_t xt_media_client_get_header(xt_media_link_handle_t handle, uint8_t *data, uint32_t *length)
    {
        media_link_t *impl = media_link_factory::query_link(handle);
        if (NULL == impl)
        {
            return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
        }

        return impl->get_header(data, length);
    }

    xt_media_client_status_t xt_media_client_play(xt_media_link_handle_t handle, xt_media_client_frame_callback_t cb, void *ctx)
    {
        media_link_t *impl = media_link_factory::query_link(handle);
        if (NULL == impl)
        {
            return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
        }

        return impl->play(cb, ctx);
    }

    xt_media_client_status_t xt_media_client_pause(xt_media_link_handle_t handle)
    {
        media_link_t *impl = media_link_factory::query_link(handle);
        if (NULL == impl)
        {
            return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
        }

        return impl->pause();
    }

    xt_media_client_status_t xt_media_client_seek(xt_media_link_handle_t handle, double npt, float scale, uint32_t *seq, uint32_t *timestamp)
    {
        media_link_t *impl = media_link_factory::query_link(handle);
        if (NULL == impl)
        {
            return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
        }

        return impl->seek(npt, scale, seq, timestamp);
    }

    xt_media_client_status_t xt_media_client_create_recv(int track_num, bool demux, xt_media_link_handle_t *phandle)
    {
        return media_link_factory::create_link(track_num, demux, phandle);
    }

    xt_media_client_status_t xt_media_client_create_recv_2(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports,xt_media_link_handle_t *phandle)
    {
        return media_link_factory::create_link(track_num,demux,multicast,multicastip,multiports,phandle);
    }

    xt_media_client_status_t xt_media_client_setsdp(xt_media_link_handle_t handle, const char *sdp, uint32_t len_sdp)
    {
        media_link_t *link = media_link_factory::query_link(handle);
        if (NULL == link)
        {
            return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
        } 

        return link->set_sdp(sdp);
    }

    xt_media_client_status_t xt_media_client_request_iframe(xt_media_link_handle_t handle)
    {
        media_link_t *link = media_link_factory::query_link(handle);
        if (NULL == link)
        {
            return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
        } 

        return link->rtcp_send_fir();
    }

	int xt_media_client_setresend(int resend,int wait_resend, int max_resend, int vga_order)
	{
		return ::mp_set_resend(resend,wait_resend,max_resend,vga_order);
	}

    xt_media_client_status_t xt_media_client_getport(xt_media_link_handle_t handle, xt_sink_info_t *sinks, int &num)
    {
        media_link_t *link = media_link_factory::query_link(handle);
        if (!link || !sinks)
        {
            return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
        }

        std::vector<xt_sink_info_t> infos;
        link->get_ports_info(infos);

        if (infos.size() == 0)
        {
            return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
        }

        if (num > infos.size())
        {
            num = infos.size();
        }

        for (std::size_t i=0;i<num&&i<infos.size();++i)
        {	 
            sinks[i].index     = infos[i].index;
            sinks[i].port_rtp	= infos[i].port_rtp;
            sinks[i].port_rtcp	= infos[i].port_rtcp;
            sinks[i].demux		= infos[i].demux;
            sinks[i].demuxid	= infos[i].demuxid;
        }

        return MEDIA_CLIENT_STATUS_OK;
    }

    void  xt_media_client_regist_callback(regist_call_back_t func)
    {
        udp_session_factory::instance()->set_regist_callback(func);
    }

    xt_media_client_status_t xt_media_client_register_rtcp_callback(xt_media_link_handle_t handle, xt_media_client_rtcp_report_callback_t cb, void *ctx)
    {
        media_link_t *impl = media_link_factory::query_link(handle);
        if (NULL == impl )
        {
            return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
        }

        return impl->register_rtcp_callback(cb, ctx);
    }
    xt_media_client_status_t xt_media_client_priv_multicast_link(const char *ip, uint16_t port, uint32_t channel, uint32_t link_type, uint32_t media_type, const char *multicast_ip, uint16_t multicast_port, xt_media_link_handle_t *phandle)
    {
        return media_link_factory::create_link(ip, port, channel, link_type, media_type, multicast_ip, multicast_port, phandle);
    }

    xt_media_client_status_t xt_media_client_register_no_frame_arrived_callback(xt_media_link_handle_t h, uint32_t priod, xt_media_client_no_frame_arrived_callback_t cb, void *ctx)
    {
    	 media_link_t *impl = media_link_factory::query_link(h);
    	 if (NULL == impl)
    	 {
    	    return MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS;
    	  }

    	  return impl->register_no_frame_arrived_callback(priod, cb, ctx);
    }
	
	xt_media_client_status_t xt_media_client_rtsp_multicat_link(const char *uri, uint32_t media_type, bool demux,  const char *multicast_ip, uint16_t multicast_port,xt_media_link_handle_t *phandle)
	{
		return media_link_factory::create_link(uri,media_type,demux,multicast_ip,multicast_port,phandle);
	}
}	
namespace xt_media_client
{
    bool ipv4_to_uint32(const char *ip, uint32_t &u)
    {
        try
        {
            u = boost::asio::ip::address_v4::from_string(ip).to_ulong();
        }
        catch (...)
        {
            return false;
        }

        return true;
    }
}

