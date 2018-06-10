#include "session_client.h"
#include "udp_client_impl.h"
#include "msg_serialization_impl.h"
#include "error.h"

#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/pool/singleton_pool.hpp>

namespace udp_session_client
{
    struct async_wait_context_t
    {
    public:
        static void UDP_SESSION_CLIENT_CALLBACK async_wait_handler(void *ctx, uint32_t e)
        {
            async_wait_context_t *context = (async_wait_context_t *)ctx;

            if (NULL != context)
            {
                context->notify(e);
                intrusive_ptr_release(context);
            }
        }

        static async_wait_context_t *allocate();
        static void deallocate(async_wait_context_t *ctx);

        uint32_t wait(uint32_t millisec)
        {
            boost::unique_lock<boost::mutex> _lock(mutex_);
            if (!condition_.timed_wait(_lock, boost::posix_time::millisec(millisec)))
            {
                return udp_session::error::timeout;
            }

            return ec_;
        }

        void notify(uint32_t ec)
        {
            boost::unique_lock<boost::mutex> _lock(mutex_);
            ec_ = ec;
            condition_.notify_all();
        }

        friend void intrusive_ptr_add_ref(async_wait_context_t *context)
        {
            context->add_ref();
        }

        friend void intrusive_ptr_release(async_wait_context_t *context)
        {
            context->release();
        }

    private:
        void add_ref()
        {
            ref_cnt_++;
        }

        void release()
        {
            if (--ref_cnt_ <= 0)
            {
                async_wait_context_t::deallocate(this);
            }
        }

        async_wait_context_t()
            :mutex_(),
            condition_(),
            ref_cnt_(0),
            ec_(0)
        {}

        ~async_wait_context_t()
        {}

        boost::mutex mutex_;
        boost::condition_variable condition_;
        boost::atomic_int32_t ref_cnt_;
        uint32_t ec_;
    };

    typedef boost::singleton_pool<async_wait_context_t, sizeof(async_wait_context_t) > async_wait_context_pool_t;

    async_wait_context_t *async_wait_context_t::allocate()
    {
        void *p = (async_wait_context_pool_t::malloc)();
        return new (p) async_wait_context_t;
    }

    void async_wait_context_t::deallocate(async_wait_context_t *ctx)
    {
        ctx->~async_wait_context_t();
        (async_wait_context_pool_t::free)(ctx);
    }

    bool session_sync_result_cache_t::push(uint32_t sequence, void *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        write_lock _lock(mutex_);

        result_map_type::iterator it = results_.find(sequence);
        if (results_.end() != it)
        {
            return false;
        }

        sync_type result;
        result.response = response;
        result.done = done;
        result.expire_at = boost::posix_time::microsec_clock::local_time() + boost::posix_time::milliseconds(timeout);
        result.ctx = ctx;

        results_.insert(result_map_type::value_type(sequence, result));

        return true;
    }

    bool session_sync_result_cache_t::pop(uint32_t sequence, sync_type &result)
    {
        write_lock _lock(mutex_);

        result_map_type::iterator it = results_.find(sequence);
        if (results_.end() == it)
        {
            return false;
        }

        result = it->second;

        results_.erase(it);
        return true;
    }

    void session_sync_result_cache_t::check_overtime()
    {
        read_lock _lock(mutex_);

        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        for (result_map_type::iterator it = results_.begin(); results_.end() != it;)
        {
            if (it->second.expire_at < now)
            {
                it->second.done(it->second.ctx, udp_session::error::timeout);
                results_.erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }

    typedef boost::asio::io_service udp_service_impl_t;

    session_client_t::session_client_t()
        :client_(),
        msg_serializer_(NULL),
        events_(NULL)
    {}

    session_client_t::~session_client_t()
    {
        events_ = NULL;

        timer_t::cancel();

        if (NULL != msg_serializer_)
        {
            udp_session::msg_serialization_impl *serialization_impl = (udp_session::msg_serialization_impl *)msg_serializer_;
            msg_serializer_ = NULL;
            delete serialization_impl;
        }
    }

    bool session_client_t::init(const char *ip, uint16_t port, void *service)
    {
        udp_service_impl_t *service_impl = (udp_service_impl_t *)service;
        if (NULL == service_impl)
        {
            return false;
        }

        try
        {
            client_.reset(new udp_client_impl(*service_impl));

            if (!client_->init(ip, port))
            {
                return false;
            }
        }
        catch (const std::exception& e)
        {
            return false;
        }

        client_->register_events(this);

        if (NULL == msg_serializer_)
        {
            msg_serializer_ = new udp_session::msg_serialization_impl;
        }

        timer_t::init(*service_impl, SESSION_CHECK_OVERTIME_PRIOD);

        return true;
    }

    void session_client_t::register_callback(events_type *events)
    {
        events_ = events;
    }

    bool session_client_t::send_msg(const char *v4_ip, uint16_t port, udp_session::msg_request_t *request, void *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        uint8_t request_buffer[2048];
        uint32_t request_buffer_len = sizeof(request_buffer);
        if (!msg_serializer_->serialize_request(request, request_buffer, request_buffer_len))
        {
            return false;
        }

        if (!sync_result_cache_.push(request->sequence, response, done, ctx, timeout))
        {
            return false;
        }

        return client_->send_msg(v4_ip, port, request_buffer, request_buffer_len);
    }

    bool session_client_t::send_msg(const char *v4_ip, uint16_t port, udp_session::msg_request_t *request)
    {
        uint8_t request_buffer[2048];
        uint32_t request_buffer_len = sizeof(request_buffer);
        if (!msg_serializer_->serialize_request(request, request_buffer, request_buffer_len))
        {
            return false;
        }

        return client_->send_msg(v4_ip, port, request_buffer, request_buffer_len);
    }

    bool session_client_t::on_receive_msg(const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size)
    {
        udp_session::msg_header_t head;

        memcpy(&head,buf,sizeof(udp_session::msg_header_t));
        if (head.code == udp_session::send_regist)
        {
            return  on_receive_request(v4_ip,port,buf,size);
        }
        else
        {
            return  on_receive_response(v4_ip,port,buf,size);
        }

    }

    void session_client_t::on_timer(bool operation_aborted)
    {
        if (!operation_aborted)
        {
            sync_result_cache_.check_overtime();
        }
    }

    bool session_client_t::on_receive_request( const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size )
    {
        uint8_t request_parser_buffer[1024];
        udp_session::msg_request_t *request = (udp_session::msg_request_t *)request_parser_buffer;

        uint32_t request_parser_buffer_len = sizeof(request_parser_buffer);
        if (!msg_serializer_->deserialize_request(buf, size, request, request_parser_buffer_len))
        {
            return false;
        }

        uint8_t response_parser_buffer[2048];
        udp_session::msg_response_t *response = (udp_session::msg_response_t *)response_parser_buffer;

        response->code = request->code;
        response->version = request->version;
        response->sequence = request->sequence;

        bool response_need_send = true;
        switch (request->code)
        {
        case udp_session::send_regist:
            events_->on_receive_send_regist_msg(v4_ip, port, (const udp_session::send_regist_request_msg_t *)request, (udp_session::send_regist_reponse_msg_t *)response);
            break;
        default:
            response_need_send = false;
            break;
        }

        if (response_need_send)
        {
            uint8_t response_buffer[2048];
            uint32_t response_buffer_len = 2048;
            if (!msg_serializer_->serialize_response(response, response_buffer, response_buffer_len))
            {
                return false;
            }

            client_->send_msg(v4_ip, port, response_buffer, response_buffer_len);
        }

        return true;
    }

    bool session_client_t::on_receive_response( const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size )
    {
        uint8_t response_parser_buffer[2048];
        udp_session::msg_response_t *response = (udp_session::msg_response_t *)response_parser_buffer;

        uint32_t response_parser_buffer_len = sizeof(response_parser_buffer);
        if (!msg_serializer_->deserialize_response(buf, size, response, response_parser_buffer_len))
        {
            return false;
        }

        session_sync_result_cache_t::sync_type sync_result;
        if (!sync_result_cache_.pop(response->sequence, sync_result))
        {
            return false;
        }

        if (udp_session::error::ok == response->error_code)
        {
            switch (response->code)
            {
            case udp_session::get_sdp:
                events_->on_get_sdp_response(v4_ip, port, (udp_session::get_sdp_response_msg_t *)response, (xt_get_sdp_response_t *)sync_result.response);
                break;
            case udp_session::play:
                events_->on_play_response(v4_ip, port, (udp_session::play_response_msg_t *)response, (xt_play_response_t *)sync_result.response);
                break;
            case udp_session::stop:
                events_->on_stop_response(v4_ip, port, (udp_session::stop_response_msg_t *)response, (xt_stop_response_t *)sync_result.response);
                break;
            case udp_session::get_sdp_and_play:
                events_->on_get_sdp_and_play_response(v4_ip, port, (udp_session::get_sdp_and_play_response_msg_t *)response, (xt_get_sdp_and_play_response_t *)sync_result.response);
                break;
            case udp_session::send_data:
                events_->on_send_data_response(v4_ip, port, (udp_session::send_data_response_msg_t *)response, (xt_send_data_response_t *)sync_result.response);
                break;
            default:
                events_->on_unknown_msg_response(v4_ip, port, response, sync_result.response);
                break;
            }
        }

        sync_result.done(sync_result.ctx, response->error_code);
        return true;
    }

    session_client_impl::session_client_impl()
        :request_sequence_(0)
    {}

    bool session_client_impl::init(const char *ip, uint16_t port, void *service)
    {
        session_client_t::register_callback(this);
        return session_client_t::init(ip, port, service);
    }

    bool session_client_impl::on_get_sdp_response(const char *v4_ip, uint16_t port, const udp_session::get_sdp_response_msg_t *response, xt_get_sdp_response_t *xresponse)
    {
        (void)memcpy(xresponse->sdp, response->sdp, response->length);
        xresponse->length = response->length;

        return true;
    }

    bool session_client_impl::on_play_response(const char *v4_ip, uint16_t port, const udp_session::play_response_msg_t *response, xt_play_response_t *xresponse)
    {
        xresponse->mode = response->proto;
        xresponse->rtp_port = response->server_rtp_port;
        xresponse->rtcp_port = response->server_rtcp_port;
        xresponse->demux_flag = response->demux_flag;
        xresponse->demux_id = response->demux_id;

        return true;
    }

    bool session_client_impl::on_stop_response(const char *v4_ip, uint16_t port, const udp_session::stop_response_msg_t *response, xt_stop_response_t *xresponse)
    {
        return true;
    }

    bool session_client_impl::on_get_sdp_and_play_response(const char *v4_ip, uint16_t port, const udp_session::get_sdp_and_play_response_msg_t *response, xt_get_sdp_and_play_response_t *xresponse)
    {
        xresponse->mode = response->proto;
        xresponse->rtp_port = response->server_rtp_port;
        xresponse->rtcp_port = response->server_rtcp_port;
        xresponse->demux_flag = response->demux_flag;
        xresponse->demux_id = response->demux_id;

        (void)memcpy(xresponse->sdp, response->sdp, response->length);
        xresponse->length = response->length;

        return true;
    }

    bool session_client_impl::on_send_data_response(const char *v4_ip, uint16_t port, const udp_session::send_data_response_msg_t *response, xt_send_data_response_t *xresponse)
    {
        xresponse->length = response->length;
        if (xresponse->length > XT_UDP_SESSION_RECV_DATA_MAX)
        {
            xresponse->length = XT_UDP_SESSION_RECV_DATA_MAX;
        }

        (void)memcpy(xresponse->data, response->data, xresponse->length);

        return true;
    }

    bool session_client_impl::on_unknown_msg_response(const char *v4_ip, uint16_t port, const udp_session::msg_response_t *response, void *xresponse)
    {
        return true;
    }

    int32_t session_client_impl::get_sdp_method(const char *ip, uint16_t port, xt_get_sdp_request_t *request, xt_get_sdp_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session::get_sdp_request_msg_t msg;

        new_a_request_msg(udp_session::get_sdp, msg);
        msg.channel = request->channel;

        return send_msg(ip, port, &msg, response, done, ctx, timeout);
    }

    int32_t session_client_impl::play_method(const char *ip, uint16_t port, xt_play_request_t *request, xt_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session::play_request_msg_t msg;

        new_a_request_msg(udp_session::play, msg);
        msg.channel = request->channel;
        msg.proto = request->mode;
        msg.client_rtp_port = request->rtp_port;
        msg.client_rtcp_port = request->rtcp_port;
        msg.demux_flag = request->demux_flag;
        msg.demux_id = request->demux_id;

        return send_msg(ip, port, &msg, response, done, ctx, timeout);
    }

    int32_t session_client_impl::stop_method(const char *ip, uint16_t port, xt_stop_request_t *request, xt_stop_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session::play_request_msg_t msg;

        new_a_request_msg(udp_session::stop, msg);
        msg.channel = request->channel;
        msg.proto = request->mode;
        msg.client_rtp_port = request->rtp_port;
        msg.client_rtcp_port = request->rtcp_port;
        msg.demux_flag = request->demux_flag;
        msg.demux_id = request->demux_id;

        return send_msg(ip, port, &msg, response, done, ctx, timeout);
    }

    void session_client_impl::heartbit_method(const char *ip, uint16_t port, uint16_t channel)
    {
        udp_session::heartbit_request_msg_t msg;
        msg.code = udp_session::heartbit;
        msg.sequence = 0;
        msg.version = UDP_SESSION_MSG_VERSION;
        msg.channel = channel;

        session_client_t::send_msg(ip, port, &msg);
    }

    void session_client_impl::heartbit2_method(const char *ip, uint16_t port, uint32_t sink_ip, uint16_t sink_port, uint16_t channel)
    {
        udp_session::heartbit2_request_msg_t msg;
        msg.code = udp_session::heartbit2;
        msg.sequence = 0;
        msg.version = UDP_SESSION_MSG_VERSION;
        msg.ip = sink_ip;
        msg.port = sink_port;
        msg.channel = channel;

        session_client_t::send_msg(ip, port, &msg);
    }

    int32_t session_client_impl::get_sdp_and_play_method(const char *ip, uint16_t port, xt_get_sdp_and_play_request_t *request, xt_get_sdp_and_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session::get_sdp_and_play_request_msg_t msg;

        new_a_request_msg(udp_session::get_sdp_and_play, msg);
        msg.channel = request->channel;
        msg.proto = request->mode;
        msg.client_rtp_port = request->rtp_port;
        msg.client_rtcp_port = request->rtcp_port;
        msg.demux_flag = request->demux_flag;
        msg.demux_id = request->demux_id;
        return send_msg(ip, port, &msg, response, done, ctx, timeout);
    }

    int32_t session_client_impl::get_sdp_and_play_method_v1(const char *ip, uint16_t port, xt_get_sdp_and_play_request_v1_t *request, xt_get_sdp_and_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session::get_sdp_and_play_request_msg_v1_t msg;

        new_a_request_msg(udp_session::get_sdp_and_play_v1, msg);
        msg.channel = request->channel;
        msg.code = request->code;
        msg.proto = request->mode;
        msg.client_rtp_port = request->rtp_port;
        msg.client_rtcp_port = request->rtcp_port;
        msg.demux_flag = request->demux_flag;
        msg.demux_id = request->demux_id;
        return send_msg(ip, port, &msg, response, done, ctx, timeout);
    }

    int32_t session_client_impl::send_data_method(const char *ip, uint16_t port, xt_send_data_request_t *request, xt_send_data_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        if (request->length > XT_UDP_SESSION_SEND_DATA_MAX)
        {
            return udp_session::error::send_data_content_too_long;
        }

        uint8_t send_data_buffer[sizeof(udp_session::send_data_request_msg_t) + XT_UDP_SESSION_SEND_DATA_MAX];
        udp_session::send_data_request_msg_t *p_msg = (udp_session::send_data_request_msg_t *)send_data_buffer;

        new_a_request_msg(udp_session::send_data, *p_msg);
        p_msg->channel = request->channel;
        p_msg->length = request->length;
        (void)memcpy(p_msg->content, request->content, request->length);

        return send_msg(ip, port, p_msg, response, done, ctx, timeout);
    }

    void session_client_impl::new_a_request_msg(udp_session::msg_code code, udp_session::msg_request_t& msg)
    {
        msg.code = code;
        msg.version = UDP_SESSION_MSG_VERSION;
        msg.sequence = request_sequence_++;
    }

    int32_t session_client_impl::send_msg(const char *ip, uint16_t port, udp_session::msg_request_t *request, void *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        //sync send operation
        if (NULL != done)
        {
            return session_client_t::send_msg(ip, port, request, response, done, ctx, timeout) ? udp_session::error::ok : udp_session::error::send_msg_fail;
        }

        // else ...
        // async operation
        boost::intrusive_ptr<async_wait_context_t> context(async_wait_context_t::allocate());

        done = async_wait_context_t::async_wait_handler;
        ctx = context.get();

        intrusive_ptr_add_ref(context.get());

        if (!session_client_t::send_msg(ip, port, request, response, done, ctx, timeout))
        {
            return udp_session::error::send_msg_fail;
        }

        return context->wait(timeout);
    }

    void *session_client_impl::create_service()
    {
        return new udp_service_impl_t;
    }

    void session_client_impl::run_service(void *service)
    {
        udp_service_impl_t *impl = (udp_service_impl_t *)service;
        if (NULL != impl)
        {
            impl->poll();
        }
    }

    void session_client_impl::destroy_service(void *service)
    {
        udp_service_impl_t *impl = (udp_service_impl_t *)service;
        if (NULL != impl)
        {
            impl->stop();
            delete impl;
        }
    }

    bool session_client_impl::on_receive_send_regist_msg( const char *v4_ip, uint16_t port, const udp_session::send_regist_request_msg_t *request, udp_session::send_regist_reponse_msg_t *response )
    {
        if (NULL == udp_session_regist_cb_)
        {
            response->error_code = udp_session::error::send_data_cb_fail;
            return false;
        }
        response->error_code =  udp_session::error::ok;
        udp_session_regist_cb_(v4_ip, port,request->ids, request->length);
        return true;
    }

    int32_t session_client_impl::set_regist_callback( udp_session_regist_call_back_t func )
    {
        udp_session_regist_cb_ = func;
        return  udp_session::error::ok;
    }
}
