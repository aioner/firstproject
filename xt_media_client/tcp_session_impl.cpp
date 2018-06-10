#include "tcp_session_impl.h"
#include <boost/make_shared.hpp>
extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);
namespace xt_media_client
{
    tcp_session_impl::tcp_session_impl(const tcp_session_connection_ptr& connection, uint16_t channel)
        :audio_rtp_port_(0),
        rtp_port_(0),
        channel_(channel),
        mode_(0),
        ssrc_(0),
        multiplex_(0),
        multiplexID_(0),
        connection_(connection)
    {}

    tcp_session_impl::~tcp_session_impl()
    {}

    xt_media_client_status_t tcp_session_impl::get_server_info(xt_session_server_info_t& server_info)
    {
        boost::shared_ptr<tcp_session_connection_t> sp = connection_.lock();
        if (!sp)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        sp->get_server_addr().get_addr(server_info.ip, server_info.port);
        server_info.channel = channel_;

        md_log(md_log_info, "tcp session get server info,ip(%s),port(%d),channel(%d)", server_info.ip, server_info.port, server_info.channel);

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t tcp_session_impl::play(double npt, float scale, uint32_t *seq, uint32_t *timestamp)
    {
        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t tcp_session_impl::describe_and_setup(std::vector<xt_session_param_t>& params, std::string& sdp)
    {
        if (1 != params.size())
        {
            return MEDIA_CLIENT_STATUS_SETUP_FAIL;
        }

        boost::shared_ptr<tcp_session_connection_t> sp = connection_.lock();
        if (!sp)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        tcp_session_client_status_t stat = TCP_SESSION_CLIENT_STATUS_OK;
        if (0 != params[0].client_ctx.demux)
        {
            tcp_session_client_demux_play_request_t request = {0};
            tcp_session_client_demux_play_response_t response = {0};

            request.audio_rtp_port = 0;
            request.channel = channel_;
            request.mode = 4;
            request.multiplex = params[0].client_ctx.demux;
            request.multiplexID = params[0].client_ctx.demuxid;
            request.rtp_port = params[0].client_ctx.rtp_port;
            request.ssrc = params[0].client_ctx.ssrc;

            stat = sp->demux_play(request, response, tcp_session_factory::instance()->get_play_timeout());
            if (TCP_SESSION_CLIENT_STATUS_OK != stat)
            {
                md_log(md_log_error, "tcp session demux_play failed.->channel(%d),mode(%d),rtp_port(%d),multiplex(%d),multiplexID(%d),ssrc(%d),stat(%d)", 
                    request.channel, request.mode, request.rtp_port, request.multiplex, request.multiplexID, request.ssrc, stat);
                return MEDIA_CLIENT_STATUS_SETUP_FAIL;
            }

            params[0].server_ctx.ssrc = 0;
            params[0].server_ctx.demux = response.multiplex;
            params[0].server_ctx.demuxid = response.multiplexID;
            params[0].server_ctx.rtp_port = sp->get_multicast_info().rtp_start_port;
            params[0].server_ctx.rtcp_port = params[0].server_ctx.rtp_port + 1;
            params[0].server_ctx.mode = static_cast<uint8_t>(request.mode);

            sdp.assign(response.sdp, response.length);

            audio_rtp_port_ = 0;
            rtp_port_ = request.rtp_port;
            mode_ = request.mode;
            ssrc_ = request.ssrc;
            multiplex_ = request.multiplex;
            multiplexID_ = request.multiplexID;
        }
        else
        {
            tcp_session_client_play_request_t request = {0};
            tcp_session_client_play_response_t response = {0};
            request.audio_rtp_port = 0;
            request.channel = channel_;
            request.mode = 4;
            request.rtp_port = params[0].client_ctx.rtp_port;
            request.ssrc = params[0].client_ctx.ssrc;

            stat = sp->play(request, response, tcp_session_factory::instance()->get_play_timeout());
            if (TCP_SESSION_CLIENT_STATUS_OK != stat)
            {
                md_log(md_log_error, "tcp session play failed.->channel(%d),mode(%d),rtp_port(%d),ssrc(%d),stat(%d)", request.channel, request.mode, request.rtp_port, request.ssrc, stat);
                return MEDIA_CLIENT_STATUS_SETUP_FAIL;
            }

            params[0].server_ctx.ssrc = 0;
            params[0].server_ctx.demux = 0;
            params[0].server_ctx.demuxid = 0;
            //modiefied by lichao, 20150511 当response.rtp_port为0时从多播信息中获取
            params[0].server_ctx.rtp_port = response.rtp_port;
            if (0 == response.rtp_port)
            {
                if (params[0].server_ctx.demux)
                {
                    params[0].server_ctx.rtp_port = sp->get_multicast_info().rtp_start_port;
                }
                else
                {
                    params[0].server_ctx.rtp_port = sp->get_multicast_info().rtp_start_port + 2*channel_;
                }                
            }
            params[0].server_ctx.rtcp_port = params[0].server_ctx.rtp_port + 1;
            params[0].server_ctx.mode = static_cast<uint8_t>(request.mode);

            sdp.assign(response.sdp, response.length);

            audio_rtp_port_ = 0;
            rtp_port_ = request.rtp_port;
            mode_ = request.mode;
            ssrc_ = request.ssrc;
            multiplex_ = 0;
            multiplexID_ = 0;
        }

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t tcp_session_impl::teardown()
    {
        boost::shared_ptr<tcp_session_connection_t> sp = connection_.lock();
        if (!sp)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        sp->on_session_destroy(this);

        tcp_session_client_status_t stat = TCP_SESSION_CLIENT_STATUS_OK;
        if (0 != multiplex_)
        {
            tcp_session_client_demux_stop_request_t request = {0};
            tcp_session_client_demux_stop_response_t response = {0};

            request.audio_rtp_port = audio_rtp_port_;
            request.rtp_port = rtp_port_;
            request.channel = channel_;
            request.mode = mode_;
            request.multiplex = multiplex_;
            request.multiplexID = multiplexID_;
            request.ssrc = ssrc_;

            stat = sp->demux_stop(request, response, tcp_session_factory::instance()->get_stop_timeout());
            if (TCP_SESSION_CLIENT_STATUS_OK != stat)
            {
                md_log(md_log_error, "tcp session demux_stop failed.->rtp_port(%d,%d),channel(%d),mode(%d),multiplex(%d),multiplexID(%d),ssrc(%d)", 
                    audio_rtp_port_, rtp_port_, channel_, mode_, multiplex_, multiplexID_, ssrc_, stat);
                return MEDIA_CLIENT_STATUS_TEARDOWN_FAIL;
            }
        }
        else
        {
            tcp_session_client_stop_request_t request = {0};
            tcp_session_client_stop_response_t response = {0};

            request.channel = channel_;
            request.mode = mode_;
            request.rtp_port = rtp_port_;

            stat = sp->stop(request, response, tcp_session_factory::instance()->get_stop_timeout());
            if (TCP_SESSION_CLIENT_STATUS_OK != stat)
            {
                md_log(md_log_error, "tcp session stop failed.->channel(%d),mode(%d),rtp_port(%d),stat(%d)", channel_, mode_, rtp_port_, stat);
                return MEDIA_CLIENT_STATUS_TEARDOWN_FAIL;
            }
        }

        return MEDIA_CLIENT_STATUS_OK;
    }

    tcp_session_connection_t::tcp_session_connection_t(tcp_session_connection_mgr_t *connection_mgr)
        :server_addr_(),
        login_response_(),
        handle_(),
        mutex_(),
        sessions_(),
        connection_mgr_(connection_mgr)
    {}

    tcp_session_connection_t::~tcp_session_connection_t()
    {
        connection_mgr_ = NULL;
        this->close();
    }

    tcp_session_client_status_t tcp_session_connection_t::init(const char *bind_ip, uint16_t bind_port, tcp_session_client_service_t service,tcp_session_client_log_cb_t log_cb)
    {
        tcp_session_client_connect_callbacks_t callbacks = { &tcp_session_connection_t::close_callback, this };
        ::xt_tcp_client_register_log(log_cb);
        return tcp_session_client_new(bind_ip, bind_port, service, &callbacks, &handle_);
    }

    tcp_session_client_status_t tcp_session_connection_t::connect(const char *ip, uint16_t port, uint32_t timeout)
    {
        tcp_session_client_status_t stat = tcp_session_client_connect(handle_, ip, port, NULL, NULL, timeout);
        if (TCP_SESSION_CLIENT_STATUS_OK == stat)
        {
            server_addr_.set_addr(ip, port);
        }
        return stat;
    }

    tcp_session_client_status_t tcp_session_connection_t::is_connected() const
    {
        return tcp_session_client_is_connected(handle_);
    }

    tcp_session_client_status_t tcp_session_connection_t::close()
    {
        if (NULL == handle_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        tcp_session_client_handle_t h = handle_;
        handle_ = NULL;
        return tcp_session_client_close(h);
    }

    tcp_session_client_status_t tcp_session_connection_t::login(uint32_t timeout)
    {
        tcp_session_client_login_request_t request = {0};
        return tcp_session_client_login(handle_, &request, &login_response_, NULL, NULL, timeout);
    }

    tcp_session_client_status_t tcp_session_connection_t::play(const tcp_session_client_play_request_t& request, tcp_session_client_play_response_t& response, uint32_t timeout)
    {
        return tcp_session_client_play(handle_, &request, &response, NULL, NULL, timeout);
    }

    tcp_session_client_status_t tcp_session_connection_t::stop(const tcp_session_client_stop_request_t& request, tcp_session_client_stop_response_t& response, uint32_t timeout)
    {
        return tcp_session_client_stop(handle_, &request, &response, NULL, NULL, timeout);
    }

    tcp_session_client_status_t tcp_session_connection_t::demux_play(const tcp_session_client_demux_play_request_t& request, tcp_session_client_demux_play_response_t& response, uint32_t timeout)
    {
        return tcp_session_client_demux_play(handle_, &request, &response, NULL, NULL, timeout);
    }

    tcp_session_client_status_t tcp_session_connection_t::demux_stop(const tcp_session_client_demux_stop_request_t& request, tcp_session_client_demux_stop_response_t& response, uint32_t timeout)
    {
        return tcp_session_client_demux_stop(handle_, &request, &response, NULL, NULL, timeout);
    }

    xt_media_client_status_t tcp_session_connection_t::create_session(uint16_t channel, media_session_ptr& session)
    {
        tcp_session_impl_ptr tcp_session = boost::make_shared<tcp_session_impl>(shared_from_this(), channel);

        {
            scoped_lock _lock(mutex_);
            sessions_.push_back(tcp_session);
        }

        session = tcp_session;
        return MEDIA_CLIENT_STATUS_OK;
    }

    void tcp_session_connection_t::on_session_destroy(tcp_session_impl *session)
    {
        scoped_lock _lock(mutex_);

        for (std::vector<tcp_session_impl_ptr>::iterator it = sessions_.begin(); sessions_.end() != it; ++it)
        {
            if (it->get() == session)
            {
                sessions_.erase(it);
                break;
            }
        }
    }

    //static 
    void tcp_session_connection_t::close_callback(void *ctx, tcp_session_client_status_t stat)
    {
        tcp_session_connection_t *impl = static_cast<tcp_session_connection_t *>(ctx);
        if (impl)
        {
            impl->on_close(stat);
        }
    }

    void tcp_session_connection_t::on_close(tcp_session_client_status_t stat)
    {
        if (NULL != connection_mgr_)
        {
            connection_mgr_->on_connection_close(this, stat);
        }
    }

    tcp_session_service_thread_t::tcp_session_service_thread_t()
        :base_t(),
        service_()
    {}

    tcp_session_service_thread_t::~tcp_session_service_thread_t()
    {
        close_thread();
    }

    bool tcp_session_service_thread_t::start_thread()
    {
        if (!create_service())
        {
            return false;
        }

        base_t::start_thread();
        return true;
    }

    void tcp_session_service_thread_t::close_thread()
    {
        stop_service();
        base_t::close_thread();
        destroy_service();
    }

    bool tcp_session_service_thread_t::create_service()
    {
        return (NULL == service_) && (TCP_SESSION_CLIENT_STATUS_OK == tcp_session_client_create_service(&service_));
    }

    void tcp_session_service_thread_t::stop_service()
    {
        if (NULL != service_)
        {
            tcp_session_client_service_stop(service_);
        }
    }

    void tcp_session_service_thread_t::run_service()
    {
        if (NULL != service_)
        {
            tcp_session_client_service_run(service_);
        }
    }

    void tcp_session_service_thread_t::destroy_service()
    {
        if (NULL != service_)
        {
            tcp_session_client_service_t service = service_;
            service_ = NULL;
            tcp_session_client_destroy_service(service);
        }
    }

    void tcp_session_service_thread_t::on_thread_run()
    {
        run_service();
    }

    void tcp_session_connection_mgr_t::init(const char *ip, uint16_t port,tcp_session_client_log_cb_t cb)
    {
        strncpy_s(bind_ip_, ip, MEDIA_CLIENT_IP_LEN);
        bind_port_ = port;
        cb_ = cb;
    }

    xt_media_client_status_t tcp_session_connection_mgr_t::create_session(const char *ip, uint16_t port, uint32_t channel, tcp_session_client_service_t service, uint16_t connect_timeout, uint16_t login_timeout, media_session_ptr& session)
    {
        v4_addr_t addr(ip, port);
        tcp_session_connection_ptr connection;

        {
            scoped_lock _lock(mutex_);
            for (tcp_session_connection_container_t::iterator it = connections_.begin(); connections_.end() != it; ++it)
            {
                const tcp_session_connection_ptr& con = (*it);
                if ((con->get_server_addr() == addr) && (0 == con->is_connected()))
                {
                    connection = *it;
                    break;
                }
            }

            if (connection)
            {
                md_log(md_log_info, "tcp session connection found", ip, port);
                return connection->create_session(channel, session);
            }
        }

        connection.reset(new (std::nothrow) tcp_session_connection_t(this));

        if (TCP_SESSION_CLIENT_STATUS_OK != connection->init(bind_ip_, bind_port_, service,cb_))
        {
            md_log(md_log_error, "tcp session connection init failed.->bind addr ip(%s),port(%d)", bind_ip_, bind_port_);
            return MEDIA_CLIENT_STATUS_BAD_ADDR;
        }

        if (TCP_SESSION_CLIENT_STATUS_OK != connection->connect(ip, port, connect_timeout))
        {
            md_log(md_log_error, "tcp session connect failed.->server addr ip(%s),port(%d),timeout(%d)", ip, port, connect_timeout);
            return MEDIA_CLIENT_STATUS_CONNECT_FAIL;
        }

        if (TCP_SESSION_CLIENT_STATUS_OK != connection->login(login_timeout))
        {
            md_log(md_log_error, "tcp session login failed.->server addr ip(%s),port(%d),timeout(%d)", ip, port, login_timeout);
            return MEDIA_CLIENT_STATUS_LOGIN_FAIL;
        }

        md_log(md_log_info, "tcp session new connection");

        {
            scoped_lock _lock(mutex_);

            connections_.push_back(connection);
            return connection->create_session(channel, session);
        }
    }

    void tcp_session_connection_mgr_t::clear_connections()
    {
        scoped_lock _lock(mutex_);
        connections_.clear();
    }

    void tcp_session_connection_mgr_t::on_connection_close(tcp_session_connection_t *connection, tcp_session_client_status_t stat)
    {
        tcp_session_connection_ptr ptr;

        {
            scoped_lock _lock(mutex_);
            for (tcp_session_connection_container_t::iterator it = connections_.begin(); connections_.end() != it; ++it)
            {
                ptr = (*it);
                if (connection == ptr.get())
                {
                    connections_.erase(it);
                    break;
                }
            }
        }
    }

    tcp_session_factory::tcp_session_factory()
        :service_(),
        connect_timeout_(0),
        login_timeout_(0),
        play_timeout_(0),
        stop_timeout_(0)
    {}

    tcp_session_factory::~tcp_session_factory()
    {
        term();
    }

    xt_media_client_status_t tcp_session_factory::create_session(const char *ip, uint16_t port, uint32_t channel, media_session_ptr& session)
    {
        return tcp_session_connection_mgr_t::create_session(ip, port, channel, service_, connect_timeout_, login_timeout_, session);
    }

    xt_media_client_status_t tcp_session_factory::destroy_session(const media_session_ptr& session)
    {
        return MEDIA_CLIENT_STATUS_OK;
    }

    bool tcp_session_factory::init(const char *ip, uint16_t port, uint16_t connect_timeout, uint16_t login_timeout, uint16_t play_timeout, uint16_t stop_timeout,tcp_session_client_log_cb_t cb)
    {
        tcp_session_connection_mgr_t::init(ip, port,cb);

        connect_timeout_ = connect_timeout;
        login_timeout_ = login_timeout;
        play_timeout_ = play_timeout;
        stop_timeout_ = stop_timeout;
        return service_.start_thread();
    }

    void tcp_session_factory::term()
    {
        tcp_session_connection_mgr_t::clear_connections();
        service_.close_thread();
    }
}
