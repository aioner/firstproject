#ifndef _SESSION_CLIENT_H_INCLUED
#define _SESSION_CLIENT_H_INCLUED

#include "msg_serialization.h"
#include "udp_client.h"
#include "xt_udp_session_client.h"
#include "timer.h"

#include <map>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/shared_ptr.hpp>

#define SESSION_CHECK_OVERTIME_PRIOD  500

namespace udp_session_client
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

    class session_client_t : public udp_client_t::events_type, public udp_session::timer_t, private boost::noncopyable
    {
    public:
        session_client_t();
        ~session_client_t();

        bool init(const char *ip, uint16_t port, void *service);
        bool send_msg(const char *v4_ip, uint16_t port, udp_session::msg_request_t *request, void *response, response_done_callback_t done, void *ctx, uint32_t timeout);
        bool send_msg(const char *v4_ip, uint16_t port, udp_session::msg_request_t *request);

        class events_type
        {
        public:
            virtual bool on_get_sdp_response(const char *v4_ip, uint16_t port, const udp_session::get_sdp_response_msg_t *response, xt_get_sdp_response_t *xresponse) = 0;
            virtual bool on_play_response(const char *v4_ip, uint16_t port, const udp_session::play_response_msg_t *response, xt_play_response_t *xresponse) = 0;
            virtual bool on_stop_response(const char *v4_ip, uint16_t port, const udp_session::stop_response_msg_t *response, xt_stop_response_t *xresponse) = 0;
            virtual bool on_get_sdp_and_play_response(const char *v4_ip, uint16_t port, const udp_session::get_sdp_and_play_response_msg_t *response, xt_get_sdp_and_play_response_t *xresponse) = 0;
            virtual bool on_send_data_response(const char *v4_ip, uint16_t port, const udp_session::send_data_response_msg_t *response, xt_send_data_response_t *xresponse) = 0;
            virtual bool on_unknown_msg_response(const char *v4_ip, uint16_t port, const udp_session::msg_response_t *response, void *xresponse) = 0;
            virtual bool on_receive_send_regist_msg(const char *v4_ip, uint16_t port, const udp_session::send_regist_request_msg_t *request, udp_session::send_regist_reponse_msg_t *response ) =0;

        };

        void register_callback(events_type *events);
    private:
        bool on_receive_msg(const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size);
        bool on_receive_request(const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size);
        bool on_receive_response(const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size);
        void on_timer(bool operation_aborted);

        boost::shared_ptr<udp_client_t> client_;
        udp_session::msg_serialization_t *msg_serializer_;
        events_type *events_;

        session_sync_result_cache_t sync_result_cache_;
    };

    class session_client_impl : public session_client_t, public session_client_t::events_type
    {
    public:
        static void *create_service();
        static void run_service(void *service);
        static void destroy_service(void *service);

        session_client_impl();
        bool init(const char *ip, uint16_t port, void *service);

        int32_t get_sdp_method(const char *ip, uint16_t port, xt_get_sdp_request_t *request, xt_get_sdp_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
        int32_t play_method(const char *ip, uint16_t port, xt_play_request_t *request, xt_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
        int32_t stop_method(const char *ip, uint16_t port, xt_stop_request_t *request, xt_stop_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
        void heartbit_method(const char *ip, uint16_t port, uint16_t channel);
        void heartbit2_method(const char *ip, uint16_t port, uint32_t sink_ip, uint16_t sink_port, uint16_t channel);
        int32_t get_sdp_and_play_method(const char *ip, uint16_t port, xt_get_sdp_and_play_request_t *request, xt_get_sdp_and_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
        int32_t get_sdp_and_play_method_v1(const char *ip, uint16_t port, xt_get_sdp_and_play_request_v1_t *request, xt_get_sdp_and_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
        int32_t send_data_method(const char *ip, uint16_t port, xt_send_data_request_t *request, xt_send_data_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
    private:
        bool on_get_sdp_response(const char *v4_ip, uint16_t port, const udp_session::get_sdp_response_msg_t *response, xt_get_sdp_response_t *xresponse);
        bool on_play_response(const char *v4_ip, uint16_t port, const udp_session::play_response_msg_t *response, xt_play_response_t *xresponse);
        bool on_stop_response(const char *v4_ip, uint16_t port, const udp_session::stop_response_msg_t *response, xt_stop_response_t *xresponse);
        bool on_get_sdp_and_play_response(const char *v4_ip, uint16_t port, const udp_session::get_sdp_and_play_response_msg_t *response, xt_get_sdp_and_play_response_t *xresponse);
        bool on_send_data_response(const char *v4_ip, uint16_t port, const udp_session::send_data_response_msg_t *response, xt_send_data_response_t *xresponse);
        bool on_unknown_msg_response(const char *v4_ip, uint16_t port, const udp_session::msg_response_t *response, void *xresponse);

        void new_a_request_msg(udp_session::msg_code code, udp_session::msg_request_t& msg);
        int32_t send_msg(const char *ip, uint16_t port, udp_session::msg_request_t *request, void *response, response_done_callback_t done, void *ctx, uint32_t timeout);

        bool on_receive_send_regist_msg(const char *v4_ip, uint16_t port, const udp_session::send_regist_request_msg_t *request, udp_session::send_regist_reponse_msg_t *response );

        uint32_t request_sequence_;

        udp_session_regist_call_back_t udp_session_regist_cb_;

    public:
        int32_t set_regist_callback(udp_session_regist_call_back_t func);
    };
}

#endif //_SESSION_CLIENT_H_INCLUED
