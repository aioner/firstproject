#ifndef _TCP_SESSION_IMPL_H_INCLUDED
#define _TCP_SESSION_IMPL_H_INCLUDED

#include "media_session.h"
#include "tcp_session_client/tcp_session_client_api.h"
#include "v4_addr.h"
#include "spinlock.h"
#include "utility/singleton.h"
#include "thread.h"

#include <vector>
#include <boost/smart_ptr/weak_ptr.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>

namespace xt_media_client
{
    class tcp_session_connection_t;
    typedef boost::shared_ptr<tcp_session_connection_t> tcp_session_connection_ptr;

    class tcp_session_impl : public media_session_t
    {
    public:
        tcp_session_impl(const tcp_session_connection_ptr& connection, uint16_t channel);
        ~tcp_session_impl();

        xt_media_client_status_t get_server_info(xt_session_server_info_t& server_info);
        xt_media_client_status_t play(double npt, float scale, uint32_t *seq, uint32_t *timestamp);
        xt_media_client_status_t describe_and_setup(std::vector<xt_session_param_t>& params, std::string& sdp);
        xt_media_client_status_t teardown();

    private:
        uint16_t audio_rtp_port_;
        uint16_t rtp_port_;
        uint16_t channel_;
        uint16_t mode_;
        uint32_t ssrc_;
        uint32_t multiplex_;
        uint32_t multiplexID_;

        boost::weak_ptr<tcp_session_connection_t> connection_;
    };

    typedef boost::shared_ptr<tcp_session_impl> tcp_session_impl_ptr;

    class tcp_session_connection_mgr_t;

    class tcp_session_connection_t : public boost::enable_shared_from_this<tcp_session_connection_t>
    {
    public:
        explicit tcp_session_connection_t(tcp_session_connection_mgr_t *connection_mgr);
        ~tcp_session_connection_t();

        tcp_session_client_status_t init(const char *bind_ip, uint16_t bind_port, tcp_session_client_service_t service,tcp_session_client_log_cb_t log_cb);
        tcp_session_client_status_t connect(const char *ip, uint16_t port, uint32_t timeout);
        tcp_session_client_status_t is_connected() const;
        tcp_session_client_status_t close();
        tcp_session_client_status_t login(uint32_t timeout);
        tcp_session_client_status_t play(const tcp_session_client_play_request_t& request, tcp_session_client_play_response_t& response, uint32_t timeout);
        tcp_session_client_status_t stop(const tcp_session_client_stop_request_t& request, tcp_session_client_stop_response_t& response, uint32_t timeout);
        tcp_session_client_status_t demux_play(const tcp_session_client_demux_play_request_t& request, tcp_session_client_demux_play_response_t& response, uint32_t timeout);
        tcp_session_client_status_t demux_stop(const tcp_session_client_demux_stop_request_t& request, tcp_session_client_demux_stop_response_t& response, uint32_t timeout);

        const v4_addr_t& get_server_addr() const { return server_addr_; }
        const tcp_session_client_login_response_t& get_multicast_info() const { return  login_response_; }
        xt_media_client_status_t create_session(uint16_t channel, media_session_ptr& session);
        uint32_t get_session_num() const
        {
            scoped_lock _lock(mutex_);
            return static_cast<uint32_t>(sessions_.size());
        }

        void on_session_destroy(tcp_session_impl *session);
    private:
        static void TCP_SESSION_CLIENT_STDCALL close_callback(void *ctx, tcp_session_client_status_t stat);
        void on_close(tcp_session_client_status_t stat);

        v4_addr_t server_addr_;
        tcp_session_client_login_response_t login_response_;
        tcp_session_client_handle_t handle_;

        mutable spinlock_t mutex_;
        typedef spinlock_t::scoped_lock scoped_lock;
        std::vector<tcp_session_impl_ptr> sessions_;

        tcp_session_connection_mgr_t *connection_mgr_;
    };

    class tcp_session_service_thread_t : public thread_base_t<tcp_session_service_thread_t>
    {
    public:
        typedef thread_base_t<tcp_session_service_thread_t> base_t;

        tcp_session_service_thread_t();
        ~tcp_session_service_thread_t();

        bool start_thread();
        void close_thread();
        operator tcp_session_client_service_t () { return service_; }

        void on_thread_run();
    private:
        bool create_service();
        void stop_service();
        void run_service();
        void destroy_service();

        tcp_session_client_service_t service_;
    };

    class tcp_session_connection_mgr_t
    {
    public:
        tcp_session_connection_mgr_t()
            :mutex_(),
            connections_(),
            bind_port_(0)
        {
            bind_ip_[0] = 0;
        }

        void init(const char *ip, uint16_t port,tcp_session_client_log_cb_t cb);

        xt_media_client_status_t create_session(const char *ip, uint16_t port, uint32_t channel, tcp_session_client_service_t service, uint16_t connect_timeout, uint16_t login_timeout, media_session_ptr& session);
        void clear_connections();

        void on_connection_close(tcp_session_connection_t *connection, tcp_session_client_status_t stat);
    private:

        boost::recursive_mutex mutex_;
        typedef boost::lock_guard<boost::recursive_mutex> scoped_lock;
        //typedef spinlock_t::scoped_lock scoped_lock;

        typedef std::vector<tcp_session_connection_ptr> tcp_session_connection_container_t;
        tcp_session_connection_container_t connections_;

        char bind_ip_[MEDIA_CLIENT_IP_LEN];
        uint16_t bind_port_;
        tcp_session_client_log_cb_t cb_;
    };

    class tcp_session_factory : public xt_utility::singleton<tcp_session_factory>, private tcp_session_connection_mgr_t
    {
    public:
        tcp_session_factory();
        ~tcp_session_factory();

        xt_media_client_status_t create_session(const char *ip, uint16_t port, uint32_t channel, media_session_ptr& session);
        xt_media_client_status_t destroy_session(const media_session_ptr& session);

        bool init(const char *ip, uint16_t port, uint16_t connect_timeout, uint16_t login_timeout, uint16_t play_timeout, uint16_t stop_timeout,tcp_session_client_log_cb_t cb);
        void term();

        uint16_t get_play_timeout() const { return play_timeout_; }
        uint16_t get_stop_timeout() const { return stop_timeout_; }
    private:
        tcp_session_service_thread_t service_;

        uint16_t connect_timeout_;
        uint16_t login_timeout_;
        uint16_t play_timeout_;
        uint16_t stop_timeout_;
    };
}

#endif //_TCP_SESSION_IMPL_H_INCLUDED
