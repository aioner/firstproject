#ifndef _UDP_SESSION_IMPL_H_INCLUDED
#define _UDP_SESSION_IMPL_H_INCLUDED

#include "media_session.h"
#include "xt_udp_session_client.h"
#include "thread_timer.h"
#include "spinlock.h"
#include "utility/singleton.h"

#include <string>
#include <boost/shared_ptr.hpp>

namespace xt_media_client
{
    class udp_session_impl : public media_session_t
    {
    public:
        udp_session_impl(const char *ip, uint16_t port, uint32_t channel, udp_session_handle session);
        ~udp_session_impl();

        xt_media_client_status_t get_server_info(xt_session_server_info_t& server_info);
        xt_media_client_status_t play(double npt, float scale, uint32_t *seq, uint32_t *timestamp);
        xt_media_client_status_t teardown();
        xt_media_client_status_t describe_and_setup(std::vector<xt_session_param_t>& params, std::string& sdp);
        xt_media_client_status_t describe(std::string& sdp);

        void heartbit();

    private:
        udp_session_handle session_;
        xt_session_context_t client_ctx_;
        std::string ip_;
        uint32_t channel_;
        uint16_t port_;
    };

    typedef boost::shared_ptr<udp_session_impl> udp_session_impl_ptr;

    class udp_session_service_t : public repeat_thread
    {
    public:
        udp_session_service_t()
            :service_(NULL)
        {
            init_service();
        }

        ~udp_session_service_t()
        {
            close_thread();
            term_service();
        }

        udp_service_handle native_handle() const
        {
            return service_;
        }

        void on_repeat()
        {
            if (NULL != service_)
            {
                ::xt_udp_client_run_service(service_);
            }
        }
    private:
        void init_service()
        {
            if (NULL == service_)
            {
                service_ = ::xt_udp_client_create_service();
            }
        }

        void term_service()
        {
            if (NULL != service_)
            {
                ::xt_udp_client_destroy_service(service_);
                service_ = NULL;
            }
        }

        udp_service_handle service_;
    };

    class udp_session_factory : public thread_timer, public xt_utility::singleton<udp_session_factory>
    {
    public:
        xt_media_client_status_t create_session(const char *ip, uint16_t port, uint32_t channel, media_session_ptr& session);
        xt_media_client_status_t destroy_session(const media_session_ptr& session);

        bool init(const char *ip, uint16_t port, uint16_t heartbit_proid, uint16_t request_try_count, uint16_t request_one_timeout)
        {
            if (NULL == ip)
            {
                return false;
            }

            session_.handle = ::xt_udp_client_session_init(ip, port, service_.native_handle());
            if (NULL == session_.handle)
            {
                return false;
            }

            service_.start_thread();
            thread_timer::set_interval(heartbit_proid);

            request_try_count_ = request_try_count;
            request_one_timeout_ = request_one_timeout;

            xt_udp_client_session_regist_callback( session_.handle,udp_session_regist_call_back);
            return true;
        }

        udp_session_factory()
            :service_(),
            session_(),
            sessions_(),
            mutex_(),
            request_try_count_(0),
            request_one_timeout_(0),
            regist_cb(NULL)
        {}

        ~udp_session_factory()
        {
            service_.close_thread();
            thread_timer::close();
        }

        uint16_t get_request_try_count() const { return request_try_count_; }
        uint16_t get_request_one_timeout() const { return request_one_timeout_; }

        void set_regist_callback(regist_call_back_t func) 
        {
            regist_cb = func;
        }

       static void UDP_SESSION_CLIENT_CALLBACK udp_session_regist_call_back(const char *ip, uint16_t port, const uint8_t *data, uint32_t length)
       {
           if (instance()->regist_cb != NULL)
           {
              instance()->regist_cb(ip,port,data,length);
           }
       }

    private:
        void on_timer();

        udp_session_service_t service_;
        regist_call_back_t regist_cb;

        struct auto_udp_session_handle
        {
            auto_udp_session_handle()
                :handle(NULL)
            {}

            ~auto_udp_session_handle()
            {
                if (NULL != handle)
                {
                    ::xt_udp_client_session_term(handle);
                    handle = NULL;
                }
            }

            udp_session_handle handle;
        };

        auto_udp_session_handle session_;

        typedef std::vector<udp_session_impl_ptr> sessions_container_t;
        sessions_container_t sessions_;

        spinlock_t mutex_;

        uint16_t request_try_count_;
        uint16_t request_one_timeout_;
    };
}

#endif //_UDP_SESSION_IMPL_H_INCLUDED
