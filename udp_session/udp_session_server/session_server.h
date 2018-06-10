#ifndef _SESSION_SERVER_H_INCLUED
#define _SESSION_SERVER_H_INCLUED

#include "msg_serialization.h"
#include "udp_server.h"
#include "xt_udp_session_server.h"
#include "timer.h"

#include <map>
#include <vector>
#include <boost/date_time.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/shared_ptr.hpp>

namespace udp_session_server
{
    class session_sync_result_cache_t
    {
    public:
        struct sync_type
        {
            void *ctx;
            void *response;
            response_done_callback_t done;
            boost::posix_time::ptime expire_at;
        };

        typedef std::map<uint32_t, sync_type> result_map_type;

        bool push(uint32_t sequence, void *response, response_done_callback_t done, void *ctx, uint32_t timeout);
        bool pop(uint32_t sequence, sync_type& result);
        void check_overtime();

    private:
        result_map_type results_;
        boost::shared_mutex mutex_;

        typedef boost::shared_lock<boost::shared_mutex> read_lock;
        typedef boost::unique_lock<boost::shared_mutex> write_lock;
    };

    class session_server_t : public udp_server_t::events_type
    {
    public:
        session_server_t();
        ~session_server_t();

        bool init(const char *ip, uint16_t port, void *service);

        bool send_msg(const char *v4_ip, uint16_t port, udp_session::msg_request_t *request, void *response, response_done_callback_t done, void *ctx, uint32_t timeout);
        bool send_msg(const char *v4_ip, uint16_t port, udp_session::msg_request_t *request);
 
        class events_type
        {
        public:
            virtual bool on_receive_get_sdp_msg(const char *v4_ip, uint16_t port, const udp_session::get_sdp_request_msg_t *request, udp_session::get_sdp_response_msg_t *response) = 0;
            virtual bool on_receive_play_msg(const char *v4_ip, uint16_t port, const udp_session::play_request_msg_t *request, udp_session::play_response_msg_t *response) = 0;
            virtual bool on_receive_stop_msg(const char *v4_ip, uint16_t port, const udp_session::stop_request_msg_t *request, udp_session::stop_response_msg_t *response) = 0;
            virtual bool on_receive_heartbit_msg(const char *v4_ip, uint16_t port, const udp_session::heartbit_request_msg_t *request) = 0;
            virtual bool on_receive_get_sdp_and_play_msg(const char *v4_ip, uint16_t port, const udp_session::get_sdp_and_play_request_msg_t *request, udp_session::get_sdp_and_play_response_msg_t *response) = 0;
            virtual bool on_receive_get_sdp_and_play_msg_v1(const char *v4_ip, uint16_t port, const udp_session::get_sdp_and_play_request_msg_v1_t *request, udp_session::get_sdp_and_play_response_msg_t *response) = 0;
            virtual bool on_receive_send_data_msg(const char *v4_ip, uint16_t port, const udp_session::send_data_request_msg_t *request, udp_session::send_data_response_msg_t *response) = 0;

            virtual bool on_receive_send_regist_msg(const char *v4_ip, uint16_t port,xt_send_regist_response_t *response ) = 0;
            virtual bool on_receive_send_stop_regist_msg(const char *v4_ip, uint16_t port, xt_send_regist_response_t *response ) =0;
            virtual bool on_receive_heartbit2_msg(const char *v4_ip, uint16_t port, const udp_session::heartbit2_request_msg_t *request) = 0;
            virtual bool on_unknown_msg_response(const char *v4_ip, uint16_t port, const udp_session::msg_response_t *response, void *xresponse) = 0;
        protected:
            virtual ~events_type() {}
        };

        void register_callback(events_type *events);
    private:
        bool on_receive_msg(const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size);

        bool on_receive_request(const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size);

        bool on_receive_response(const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size);
        udp_server_t *server_;
        udp_session::msg_serialization_t *msg_serializer_;
        events_type *events_;
        session_sync_result_cache_t sync_result_cache_;
    };

    class clients_mgr_t
    {
    public:
        struct client_key_type
        {
            std::string ip;
            uint16_t port;

            bool operator <(const client_key_type&rhs) const
            {
                int cmp = strcmp(ip.c_str(), rhs.ip.c_str());
                if (0 != cmp)
                {
                    return (cmp < 0);
                }

                if (port != rhs.port)
                {
                    return (port < rhs.port);
                }

                return false;
            }
        };

        struct channel_info_type
        {
            boost::posix_time::ptime refresh_time;
            uint32_t demux_id;
            uint16_t rtp_port;
            uint16_t rtcp_port;
            uint16_t channel;
            uint8_t code;
            uint8_t demux_flag;
        };

        typedef std::vector<channel_info_type> channel_container_type;

        struct client_info_type
        {
            channel_container_type channels;
        };

        typedef std::map<client_key_type, client_info_type> client_info_map_type;

        clients_mgr_t()
            :client_infos_(),
            mutex_(),
            del_sink_cb_(NULL),
            add_sink_cb_(NULL)
        {}

        void set_callback(del_sink_callback_t del_cb, add_sink_callback_t add_cb)
        {
            del_sink_cb_ = del_cb;
            add_sink_cb_ = add_cb;
        }

        uint32_t add_client(const char *ip, uint16_t port, uint16_t channel,uint8_t code, uint16_t &rtp_port, uint16_t &rtcp_port, uint8_t &demux_flag, uint32_t &demux_id);
        uint32_t del_client(const char *ip, uint16_t port, uint16_t channel,uint8_t code, uint16_t rtp_port, uint16_t rtcp_port, uint8_t demux_flag, uint32_t demux_id);
        void heartbit_client(const char *ip, uint16_t port, uint16_t channel);
        void check_overtime(uint32_t millsec);

    private:
        uint32_t _del_client_sink(client_info_map_type::iterator it, uint32_t channel,uint8_t code);
        uint32_t _del_client_sink(client_info_map_type::iterator it, uint32_t channel,uint8_t code, uint16_t rtp_port, uint16_t rtcp_port, uint8_t demux_flag, uint32_t demux_id);
        bool _del_client_sink(client_info_map_type::iterator it, channel_container_type::iterator &it2);

        bool _find_channel(client_info_map_type::iterator it, uint16_t channel, channel_container_type::iterator& chan_it);

        typedef boost::detail::spinlock mutex_type;
        typedef boost::detail::spinlock::scoped_lock scoped_lock;

        client_info_map_type client_infos_;
        mutex_type mutex_;
        del_sink_callback_t del_sink_cb_;
        add_sink_callback_t add_sink_cb_;
    };

    class session_server_impl : public session_server_t, public session_server_t::events_type, public udp_session::timer_t
    {
    public:
        session_server_impl(const xt_udp_session_config_t *config);
        bool init();

        static void *create_service();
        static void run_service(void *service);
        static void destroy_service(void *service);

        int32_t send_regist_method( const char *ip, uint16_t port, const xt_send_regist_request_t *request, xt_send_regist_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout );
        int32_t send_stop_regist_method( const char *ip, uint16_t port, const xt_send_regist_request_t *request, xt_send_regist_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout );

        void new_a_request_msg(udp_session::msg_code code, udp_session::msg_request_t& msg);
    private:
        bool on_receive_get_sdp_msg(const char *v4_ip, uint16_t port, const udp_session::get_sdp_request_msg_t *request, udp_session::get_sdp_response_msg_t *response);
        bool on_receive_play_msg(const char *v4_ip, uint16_t port, const udp_session::play_request_msg_t *request, udp_session::play_response_msg_t *response);
        bool on_receive_stop_msg(const char *v4_ip, uint16_t port, const udp_session::stop_request_msg_t *request, udp_session::stop_response_msg_t *response);
        bool on_receive_heartbit_msg(const char *v4_ip, uint16_t port, const udp_session::heartbit_request_msg_t *request);
        bool on_receive_get_sdp_and_play_msg(const char *v4_ip, uint16_t port, const udp_session::get_sdp_and_play_request_msg_t *request, udp_session::get_sdp_and_play_response_msg_t *response);
        bool on_receive_get_sdp_and_play_msg_v1(const char *v4_ip, uint16_t port, const udp_session::get_sdp_and_play_request_msg_v1_t *request, udp_session::get_sdp_and_play_response_msg_t *response);
        bool on_receive_send_data_msg(const char *v4_ip, uint16_t port, const udp_session::send_data_request_msg_t *request, udp_session::send_data_response_msg_t *response);
        bool on_receive_send_regist_msg( const char *v4_ip, uint16_t port,xt_send_regist_response_t *response );
        bool on_receive_send_stop_regist_msg(const char *v4_ip, uint16_t port, xt_send_regist_response_t *response );
        bool on_receive_heartbit2_msg(const char *v4_ip, uint16_t port, const udp_session::heartbit2_request_msg_t *request);
        bool on_unknown_msg_response(const char *v4_ip, uint16_t port, const udp_session::msg_response_t *response, void *xresponse);

        void on_timer(bool operation_aborted);

        xt_udp_session_config_t config_;
        clients_mgr_t clients_mgr_;

        uint32_t request_sequence_;

        boost::shared_ptr<udp_server_t> server_;

        int32_t send_msg(const char *ip, uint16_t port, udp_session::msg_request_t *request, void *response, response_done_callback_t done, void *ctx, uint32_t timeout);
    };
}

#endif //_SESSION_SERVER_H_INCLUED
