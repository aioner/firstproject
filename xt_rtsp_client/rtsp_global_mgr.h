#ifndef _RTSP_GLOBAL_MGR_H_INCLUDED
#define _RTSP_GLOBAL_MGR_H_INCLUDED

#include "rtsp_task.h"
#include "spinlock.h"
#include "thread_timer.h"
#include "utility/singleton.h"
#include "rv_rtsp_client_adapter.h"

#include <boost/pool/singleton_pool.hpp>

namespace xt_rtsp_client
{
    inline void XT_RTSP_CLIENT_STDCALL sync_op_cb(int32_t stat, void *ctx)
    {
        boost::promise<int32_t> *pr = static_cast<boost::promise<int32_t> *>(ctx);
        if (NULL != pr)
        {
            pr->set_value(stat);
        }
    }

    class rtsp_global_mgr : public thread_timer, public xt_utility::singleton<rtsp_global_mgr>, public rv_rtsp_client_adapter
    {
    public:
        rtsp_global_mgr()
            :connect_requests_mgr_()
        {}

        ~rtsp_global_mgr()
        {
            term();
        }

        void init(uint32_t check_timeout_priod);

        xt_rtsp_client_status_t create_client(const rtsp_client_config_t *config, rtsp_client_handle_t *pclient);
        void destroy_client(rtsp_client_handle_t client);

        xt_rtsp_client_status_t create_connection(rtsp_client_handle_t client, const char *uri, const char *local_ip, uint16_t local_port, const rtsp_client_connection_config_t *config, int8_t *connected, rtsp_connection_handle_t *pconnection);
        void destroy_connection(rtsp_connection_handle_t connection);

        xt_rtsp_client_status_t create_session(rtsp_connection_handle_t connection, const rtsp_client_session_config_t *config, rtsp_session_handle_t *psession);
        void destroy_session(rtsp_session_handle_t session);

        bool async_connect(rtsp_connection_handle_t connection, const rtsp_client_connect_request_t *request, rtsp_client_connect_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);
        bool async_describe_request(rtsp_connection_handle_t connection, const rtsp_client_describe_request_t *request, rtsp_client_describe_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);
        bool async_setup_request(rtsp_session_handle_t session, const rtsp_client_setup_request_t *request, rtsp_client_setup_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);
        bool async_play_request(rtsp_session_handle_t session, const rtsp_client_play_request_t *request, rtsp_client_play_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);
        bool async_pause_request(rtsp_session_handle_t session, const rtsp_client_pause_request_t *request, rtsp_client_pause_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);
        bool async_teardown_request(rtsp_session_handle_t session, const rtsp_client_teardown_request_t *request, rtsp_client_teardown_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);

        bool get_addr(rtsp_connection_handle_t connection, char ip[RTSP_CLIENT_IP_LEN], uint16_t *port);
        void disconnect(rtsp_connection_handle_t connection);

        void notify_connect(rtsp_connection_info_t * connection, void *response);
        void notify_rtsp_request(rtsp_connection_info_t * connection, RvUint16 cseq, void *response);
        void notify_rtsp_request(rtsp_session_info_t * connection, RvUint16 cseq, void *response);
        rtsp_connection_info_t* get_connection_by_uri(const std::string uri);


        template<typename AsyncFuncT, typename HandleT, typename RequestT, typename ResponseT>
        bool sync_op(AsyncFuncT async_func, HandleT h, const RequestT *request, ResponseT *response, uint32_t timeout)
        {
            boost::promise<int32_t> pr;
            boost::unique_future<int32_t> fut = pr.get_future();

            if (!(this->*async_func)(h, request, response, sync_op_cb, &pr, timeout))
            {
                return false;
            }

            return RTSP_CLIENT_STATUS_OK == fut.get();
        }

        template<typename AsyncFuncT, typename HandleT, typename RequestT, typename ResponseT>
        bool all_op(AsyncFuncT async_func, HandleT h, const RequestT *request, ResponseT *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
        {
            return (NULL != done) ? (this->*async_func)(h, request, response, done, ctx, timeout) : sync_op(async_func, h, request, response, timeout);
        }
    private:
        void term();
        void on_timer();

        void check_overtime_request();

        bool add_seq_request(rtsp_connection_info_t *connection, RvUint16 seq, async_task_t *task);
        bool cancel_seq_request(rtsp_connection_info_t *connection, RvUint16 seq);

        bool add_connect_request(const std::string uri,rtsp_connection_info_t * connection);
        bool delete_connect_request(rtsp_connection_info_t *connection);

        bool destroy_connection_info(rtsp_connection_info_t *connection);


        rtsp_global_mgr(const rtsp_global_mgr&);
        void operator=(const rtsp_global_mgr &);

        async_task_mgr_t<rtsp_connection_info_t *> connect_requests_mgr_;

        std::map<rtsp_connection_info_t *, async_task_mgr_t<RvUint16> > seq_requests_mgr_;

        std::map<std::string,rtsp_connection_info_t* >connect_request_mgr;

        spinlock_t seq_requests_mgr_mutex_;
    };
}





#endif //_RTSP_GLOBAL_MGR_H_INCLUDED
