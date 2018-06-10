#include "udp_session_impl.h"
#include "compat.h"
extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);
namespace xt_media_client
{
    udp_session_impl::udp_session_impl(const char *ip, uint16_t port, uint32_t channel, udp_session_handle session)
        :session_(session),
        client_ctx_(),
        ip_(ip),
        channel_(channel),
        port_(port)
    {}

    udp_session_impl::~udp_session_impl()
    {
        ip_.clear();
        port_ = 0;
        channel_ = 0;
        session_ = NULL;
    }

    xt_media_client_status_t udp_session_impl::get_server_info(xt_session_server_info_t& server_info)
    {
        (void)strncpy_s(server_info.ip, ip_.c_str(), ip_.length());
        server_info.port = port_;
        server_info.channel = channel_;

       md_log(md_log_info, "udp session get_server_info ok.ip(%s),port(%d),channel(%d)", server_info.ip, server_info.port, server_info.channel);

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t udp_session_impl::play(double npt, float scale, uint32_t *seq, uint32_t *timestamp)
    {
        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t udp_session_impl::teardown()
    {
        xt_stop_request_t request;
        xt_stop_response_t response;

        request.channel = channel_;
        request.rtp_port = client_ctx_.rtp_port;
        request.rtcp_port = client_ctx_.rtcp_port;
        request.mode = client_ctx_.mode;
        request.demux_flag = client_ctx_.demux;
        request.demux_id = client_ctx_.demuxid;

        uint32_t count = 0;;
        while (count < udp_session_factory::instance()->get_request_try_count())
        {
            int32_t ret = ::xt_udp_client_session_stop(session_, ip_.c_str(), port_, &request, &response, NULL, NULL, udp_session_factory::instance()->get_request_one_timeout());
            if (0 == ret)
            {
                break;
            }
            count++;

            md_log(md_log_debug, "udp session xt_udp_session_stop try.count(%d),ret(%d)", count, ret);
        }

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t udp_session_impl::describe_and_setup(std::vector<xt_session_param_t>& params, std::string& sdp)
    {
        if (1 != params.size())
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        //xt_get_sdp_and_play_request_t request;
        xt_get_sdp_and_play_request_v1_t request ={0};
        xt_get_sdp_and_play_response_t response = {0};

        request.channel = channel_;
        request.code = params[0].code;
        request.rtp_port = params[0].client_ctx.rtp_port;
        request.rtcp_port = params[0].client_ctx.rtcp_port;
        request.mode = params[0].client_ctx.mode;
        request.demux_flag = params[0].client_ctx.demux;
        request.demux_id = params[0].client_ctx.demuxid;

        uint32_t count = 0;;
        while (count < udp_session_factory::instance()->get_request_try_count())
        {
            //int32_t ret = ::xt_udp_client_session_fast_play(session_, ip_.c_str(), port_, &request, &response, NULL,  NULL, udp_session_factory::instance()->get_request_one_timeout());
            int32_t ret = ::xt_udp_client_session_fast_play_v1(session_, ip_.c_str(), port_, &request, &response, NULL,  NULL, udp_session_factory::instance()->get_request_one_timeout());
            if (0 == ret)
            {
                break;
            }
            count++;

            md_log(md_log_debug, "udp session xt_udp_session_fast_play try.count(%d),ret(%d)", count, ret);
        }

        if (udp_session_factory::instance()->get_request_try_count() == count)
        {
            return MEDIA_CLIENT_STATUS_SETUP_FAIL;
        }

        params[0].server_ctx.rtp_port = response.rtp_port;
        params[0].server_ctx.rtcp_port = response.rtcp_port;
        params[0].server_ctx.mode = response.mode;
        params[0].server_ctx.demux = response.demux_flag;
        params[0].server_ctx.demuxid = response.demux_id;

        client_ctx_ = params[0].client_ctx;

        sdp.assign((const char *)response.sdp, response.length);

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t udp_session_impl::describe(std::string& sdp)
    {
    	xt_get_sdp_request_t request;
        xt_get_sdp_response_t response = { 0 };

        request.channel = channel_;

        uint32_t count = 0;
        while (count < udp_session_factory::instance()->get_request_try_count())
        {
            int32_t ret = ::xt_udp_client_session_get_sdp(session_, ip_.c_str(), port_, &request, &response, NULL, NULL, udp_session_factory::instance()->get_request_one_timeout());
            if (0 == ret)
            {
                break;
            }
            count++;

            md_log(md_log_debug, "udp session xt_udp_client_session_get_sdp try.count(%d),ret(%d)", count, ret);
        }

        if (udp_session_factory::instance()->get_request_try_count() == count)
        {
            return MEDIA_CLIENT_STATUS_DESCRIBE_FAIL;
        }

        sdp.assign(reinterpret_cast<const char *>(response.sdp), response.length);

        return MEDIA_CLIENT_STATUS_OK;
    }

    void udp_session_impl::heartbit()
    {
        if (2 == client_ctx_.mode)
        {
            xt_udp_client_session_heartbit2(session_, ip_.c_str(), port_, client_ctx_.demuxid, client_ctx_.rtp_port, channel_);
        }
        else
        {
            xt_udp_client_session_heartbit(session_, ip_.c_str(), port_, channel_);
        }
    }

    xt_media_client_status_t udp_session_factory::create_session(const char *ip, uint16_t port, uint32_t channel, media_session_ptr& session)
    {
        udp_session_impl_ptr udp_session(new udp_session_impl(ip, port, channel, session_.handle));

        {
            spinlock_t::scoped_lock _lock(mutex_);
            sessions_.push_back(udp_session);
        }

        session = udp_session;
        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t udp_session_factory::destroy_session(const media_session_ptr& session)
    {
        spinlock_t::scoped_lock _lock(mutex_);
        for (sessions_container_t::iterator it = sessions_.begin(); sessions_.end() != it; ++it)
        {
            if ((*it).get() == session.get())
            {
                sessions_.erase(it);
                return MEDIA_CLIENT_STATUS_OK;
            }
        }

        return MEDIA_CLIENT_STATUS_UNKNOWN;
    }

    void udp_session_factory::on_timer()
    {
        spinlock_t::scoped_lock _lock(mutex_);
        for (sessions_container_t::iterator it = sessions_.begin(); sessions_.end() != it; ++it)
        {
            (*it)->heartbit();
        }
    }
}
