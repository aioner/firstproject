#include "rtsp_session_impl.h"
#include "sdp_parser.h"
#include "compat.h"
extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);
namespace xt_media_client
{

    bool rtsp_session_library::init(uint16_t connect_timeout, uint16_t describe_timeout, uint16_t setup_timeout, uint16_t play_timeout, uint16_t pause_timeout, uint16_t teardown_timeout,xt_rtsp_client_log_cb_t cb)
    {
        if (NULL != rtsp_session_handle_)
        {
            return true;
        }

        xt_rtsp_client_status_t stat = xt_rtsp_client_init();
        if (RTSP_CLIENT_STATUS_OK != stat)
        {
            return false;
        }
        ::xt_rtsp_client_register_log(cb);
        rtsp_client_config_t config;
        config.max_connections = RTSP_CONNECTION_MAX;
        config.dns_ip_address[0] = 0;

        stat = xt_rtsp_create_client(&config, &rtsp_session_handle_);
        if (RTSP_CLIENT_STATUS_OK != stat)
        {
            return false;
        }

        connect_timeout_ = connect_timeout;
        describe_timeout_ = describe_timeout;
        setup_timeout_ = setup_timeout;
        play_timeout_ = play_timeout;
        pause_timeout_ = pause_timeout;
        teardown_timeout_ = teardown_timeout;

        return true;
    }

    void rtsp_session_library::term()
    {
        if (NULL != rtsp_session_handle_)
        {
            xt_rtsp_destroy_client(rtsp_session_handle_);
            rtsp_session_handle_ = NULL;

            xt_rtsp_client_term();
        }
    }

    namespace detail
    {
        template<typename RequestT, typename ResponseT>
        struct rtsp_client_param_t
        {
            RequestT request;
            ResponseT response;

            rtsp_client_param_t()
            {
                (void)memset(&request, 0, sizeof(RequestT));
                (void)memset(&response, 0, sizeof(ResponseT));
            }
        };

    }

    rtsp_session_impl::rtsp_session_impl()
        :connection_(NULL),
        media_infos_(),
        session_(NULL),
        uri_()
    {}

    rtsp_session_impl::~rtsp_session_impl()
    {
        close_session();
        close_connection();
    }

    bool rtsp_session_impl::connect(const char *uri,const char *localip)
    {
        if (NULL == rtsp_session_library::instance()->get_handle())
        {
            return false;
        }

        rtsp_client_connection_config_t config;
        config.describe_response_timeout = rtsp_session_library::instance()->get_describe_timeout();
        config.dns_max_results = RTSP_CONNECTION_DNS_MAX_RESULTS;
        config.max_headers_in_msg = RTSP_CONNECTION_MAX_HEADER_INMSG;
        config.max_sessions = RTSP_CONNECTION_MAX_SESSION;
        config.max_urls_in_msg = RTSP_CONNECTION_MAX_URLS_INMSG;
        config.transmit_queue_size = RTSP_CONNECTION_TRANSMIT_QUEUE_SIZE;
        config.waiting_describe_requests = RTSP_CONNECTION_WAIT_DESCRIBE_REQ;

        int8_t connected = 0;
        xt_rtsp_client_status_t stat = xt_rtsp_client_create_connection(rtsp_session_library::instance()->get_handle(), uri, localip, 1550, &config, &connected, &connection_);
        if (RTSP_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "rtsp session xt_rtsp_client_create_connection failed.-stat(%d)", stat);
            return false;
        }

        if (0 == connected)
        {
            detail::rtsp_client_param_t<rtsp_client_connect_request_t, rtsp_client_connect_response_t> connect_param;

            stat = xt_rtsp_client_connect(connection_, &connect_param.request, &connect_param.response, NULL, NULL, rtsp_session_library::instance()->get_connect_timeout());
            if ((RTSP_CLIENT_STATUS_OK != stat) || (0 == connect_param.response.success))
            {
                md_log(md_log_error, "rtsp session xt_rtsp_client_connect failed.-stat(%d)", stat);
                return false;
            }
        }

        uri_.assign(uri);

        return true;
    }

    xt_media_client_status_t rtsp_session_impl::get_server_info(xt_session_server_info_t &server_info)
    {
        if (NULL == connection_)
        {
            return MEDIA_CLIENT_STATUS_ENV_INIT_FAIL;
        }

        char ip[RTSP_CLIENT_IP_LEN];
        uint16_t port = 0;

        xt_rtsp_client_status_t stat = xt_rtsp_client_get_addr(connection_, ip, &port);
        if (RTSP_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "rtsp session xt_rtsp_client_get_addr stat(%d)", stat);
            return MEDIA_CLIENT_STATUS_BAD_ADDR;
        }

        (void)strncpy_s(server_info.ip, ip);
        server_info.port = port;
        server_info.channel = 0;

       md_log(md_log_info, "rtsp session get_server_info ok.ip(%s),port(%d),channel(%d)", server_info.ip, server_info.port, server_info.channel);

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t rtsp_session_impl::parse_sdp(const std::string &sdp, std::vector<xt_sdp_media_info_t> &sdp_media_infos)
    {
        if (NULL == connection_)
        {
            return MEDIA_CLIENT_STATUS_ENV_INIT_FAIL;
        }

        if (!sdp_parser_t::parse(uri_.c_str(), sdp.c_str(), sdp.length(), media_infos_))
        {
            return MEDIA_CLIENT_STATUS_BAD_SDP_CTX;
        }

        sdp_media_infos = media_infos_;

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t rtsp_session_impl::describe(std::string& sdp)
    {
        if (NULL == connection_)
        {
            return MEDIA_CLIENT_STATUS_ENV_INIT_FAIL;
        }

        detail::rtsp_client_param_t<rtsp_client_describe_request_t, rtsp_client_describe_response_t> describe_param;
        (void)strncpy_s(describe_param.request.uri, uri_.c_str(), uri_.length());

        xt_rtsp_client_status_t stat = xt_rtsp_client_describe(connection_, &describe_param.request, &describe_param.response, NULL, NULL, rtsp_session_library::instance()->get_describe_timeout());

        if ((RTSP_CLIENT_STATUS_OK != stat) || (0 == describe_param.response.content_length))
        {
            md_log(md_log_error, "rtsp session xt_rtsp_client_describe failed.stat(%d)", stat);
            return MEDIA_CLIENT_STATUS_DESCRIBE_FAIL;
        }

        sdp.assign((const char *)describe_param.response.body, describe_param.response.content_length);

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t rtsp_session_impl::setup(std::vector<xt_session_param_t>& params)
    {
        if (NULL == connection_)
        {
            return MEDIA_CLIENT_STATUS_ENV_INIT_FAIL;
        }

        if (1 != params.size())
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        rtsp_client_session_config_t config;
        config.response_timeout = RTSP_SESSION_RESPONSE_TIMEOUT;
        config.ping_transmission_timeout = RTSP_SESSION_PING_TIMEOUT;

        std::size_t index = 0;
        for (; index < media_infos_.size(); ++index)
        {
            if (media_infos_[index].media_type == params[0].stream_type)
            {
                break;
            }
        }

        if (media_infos_.size() == index)
        {
            md_log(md_log_error, "rtsp session setup failed.-no fit media stream,count(%d)", media_infos_.size());
            return MEDIA_CLIENT_STATUS_SETUP_FAIL;
        }

        xt_rtsp_client_status_t stat = RTSP_CLIENT_STATUS_UNKNOWN;

        if (NULL == session_)
        {
            stat = xt_rtsp_client_create_session(connection_, &config, &session_);
            if ((RTSP_CLIENT_STATUS_OK != stat) || (NULL == session_))
            {
                 md_log(md_log_error, "rtsp session xt_rtsp_client_create_session failed.-stat(%d)", stat);
                return MEDIA_CLIENT_STATUS_SETUP_FAIL;
            }
        }

        detail::rtsp_client_param_t<rtsp_client_setup_request_t, rtsp_client_setup_response_t> setup_param;

        (void)strncpy_s(setup_param.request.uri, media_infos_[index].uri);
        setup_param.request.client_rtp_port = params[0].client_ctx.rtp_port;
        setup_param.request.client_rtcp_port = params[0].client_ctx.rtcp_port;
		setup_param.request.client_demux = params[0].client_ctx.demux;
		setup_param.request.client_demuxid = params[0].client_ctx.demuxid;
        setup_param.request.destination[0] = 0;
        setup_param.request.server_rtp_port = 0;
        setup_param.request.server_rtcp_port = 0;
		setup_param.request.is_unicast = 1;

        md_log(md_log_debug, "rtsp session xt_rtsp_client_setup entry");

        stat = xt_rtsp_client_setup(session_, &setup_param.request, &setup_param.response, NULL, NULL, rtsp_session_library::instance()->get_setup_timeout());
        if (RTSP_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "rtsp session xt_rtsp_client_setup failed.-stat(%d)", stat);
            return MEDIA_CLIENT_STATUS_SETUP_FAIL;
        }

        md_log(md_log_debug, "rtsp session xt_rtsp_client_setup leave");

        params[0].server_ctx.rtp_port = setup_param.response.server_rtp_port;
        params[0].server_ctx.rtcp_port = setup_param.response.server_rtcp_port;
        params[0].server_ctx.mode = 0;
        params[0].server_ctx.demux = setup_param.response.server_demux;
        params[0].server_ctx.demuxid = setup_param.response.server_demuxid;

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t rtsp_session_impl::play(double npt, float scale, uint32_t* seq, uint32_t* timestamp)
    {
        if (NULL == connection_)
        {
            return MEDIA_CLIENT_STATUS_ENV_INIT_FAIL;
        }

        detail::rtsp_client_param_t<rtsp_client_play_request_t, rtsp_client_play_response_t> play_param;

        (void)strncpy_s(play_param.request.uri, uri_.c_str(), uri_.length());
        play_param.request.scale = scale;
        play_param.request.range.format = RTSP_NPT_FORMAT_SEC;
        play_param.request.range.seconds = npt;

        xt_rtsp_client_status_t stat = xt_rtsp_client_play(session_, &play_param.request, &play_param.response, NULL, NULL, rtsp_session_library::instance()->get_play_timeout());
        if (RTSP_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "rtsp session xt_rtsp_client_play failed.-stat(%d)", stat);
            return MEDIA_CLIENT_STATUS_PLAY_FAIL;
        }

        if (play_param.response.rtp_info_num >= 1)
        {
            if (NULL != seq)
            {
                *seq = play_param.response.rtp_infos[0].seq;
            }

            if (NULL != timestamp)
            {
                *timestamp = play_param.response.rtp_infos[0].timestamp;
            }
        }

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t rtsp_session_impl::pause()
    {
        if (NULL == connection_)
        {
            return MEDIA_CLIENT_STATUS_ENV_INIT_FAIL;
        }

        detail::rtsp_client_param_t<rtsp_client_pause_request_t, rtsp_client_pause_response_t> pause_param;

        (void)strncpy_s(pause_param.request.uri, uri_.c_str(), uri_.length());
        xt_rtsp_client_status_t stat = xt_rtsp_client_pause(session_, &pause_param.request, &pause_param.response, NULL, NULL, rtsp_session_library::instance()->get_pause_timeout());
        if (RTSP_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "rtsp session xt_rtsp_client_pause failed.-stat(%d)", stat);
            return MEDIA_CLIENT_STATUS_PAUSE_FAIL;
        }

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t rtsp_session_impl::teardown()
    {
        if (NULL == connection_)
        {
            return MEDIA_CLIENT_STATUS_ENV_INIT_FAIL;
        }

        detail::rtsp_client_param_t<rtsp_client_teardown_request_t, rtsp_client_teardown_response_t> teardown_param;

        (void)strncpy_s(teardown_param.request.uri, uri_.c_str(), uri_.length());
        xt_rtsp_client_status_t stat = xt_rtsp_client_teardown(session_, &teardown_param.request, &teardown_param.response, NULL, NULL, rtsp_session_library::instance()->get_teardown_timeout());
        if (RTSP_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "rtsp session xt_rtsp_client_teardown failed.-stat(%d)", stat);
            return MEDIA_CLIENT_STATUS_TEARDOWN_FAIL;
        }

        return MEDIA_CLIENT_STATUS_OK;
    }

    void rtsp_session_impl::close_connection()
    {
        if (NULL != connection_)
        {
            xt_rtsp_client_destroy_connection(connection_);
            connection_ = NULL;
        }
    }

    void rtsp_session_impl::close_session()
    {
        if (NULL != session_)
        {
            xt_rtsp_client_destroy_session(session_);
            session_ = NULL;
        }
    }
}
