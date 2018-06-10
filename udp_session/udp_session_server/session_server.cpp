#include "session_server.h"
#include "udp_server_impl.h"
#include "msg_serialization_impl.h"
#include "error.h"

#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include "log.h"
#include "../../src/include/xt_log_def.h"
#include <stdio.h>
#include <stdarg.h>
extern xt_print_cb udp_svr_print_;
void UDP_SVR_PRINT(const xt_log_level ll, const char* format,...)
{
    if (udp_svr_print_)
    {
        char context[4096] = {0};
        va_list arg;
        va_start(arg, format);
        vsnprintf(context, sizeof(context)-1, format, arg);
        va_end(arg);
        udp_svr_print_("udp_svr", ll, context);
    }
}
namespace udp_session_server
{
    struct async_wait_context_t
    {
    public:
        static void UDP_SESSION_SERVER_CALLBACK async_wait_handler(void *ctx, uint32_t e)
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

    uint32_t clients_mgr_t::add_client(const char *ip, uint16_t port, uint16_t channel, uint8_t code, uint16_t &rtp_port, uint16_t &rtcp_port, uint8_t &demux_flag, uint32_t &demux_id)
    {
        client_key_type key;
        key.ip.assign(ip);
        key.port = port;

        uint32_t result = udp_session::error::ok;

        scoped_lock _lock(mutex_);

        client_info_map_type::iterator it = client_infos_.find(key);

        if (client_infos_.end() != it)
        {
            (void)_del_client_sink(it, channel,code);
        }
        else
        {
            it = client_infos_.insert(client_info_map_type::value_type(key, client_info_type())).first;
        }

        channel_info_type chan;
        chan.channel = channel;
        chan.code = code;
        chan.rtp_port = rtp_port;
        chan.rtcp_port = rtcp_port;
        chan.demux_flag = demux_flag;
        chan.demux_id = demux_id;
        chan.refresh_time = boost::posix_time::microsec_clock::local_time();

        it->second.channels.push_back(chan);

        int32_t result2 = add_sink_cb_(channel,code, 0, ip, &rtp_port, &rtcp_port, &demux_flag, &demux_id);
        if (0 != result2)
        {
            it->second.channels.pop_back();
            result = udp_session::error::add_sink_cb_fail;
        }
        UDP_SVR_PRINT(level_info,"client mgr add client callback|result(%d)", result2);

        return result;
    }

    uint32_t clients_mgr_t::del_client(const char *ip, uint16_t port, uint16_t channel, uint8_t code, uint16_t rtp_port, uint16_t rtcp_port, uint8_t demux_flag, uint32_t demux_id)
    {
        client_key_type key;
        key.ip.assign(ip);
        key.port = port;

        scoped_lock _lock(mutex_);

        client_info_map_type::iterator it = client_infos_.find(key);
        if (client_infos_.end() == it)
        {
            UDP_SVR_PRINT(level_error,"client mgr del client fail|ip(%s0,port(%d) not found.client num(%d)", ip, port, client_infos_.size());
            return udp_session::error::channel_not_exists;
        }

        return _del_client_sink(it, channel,code,rtp_port, rtcp_port, demux_flag, demux_id);
    }

    void clients_mgr_t::heartbit_client(const char *ip, uint16_t port, uint16_t channel)
    {
        client_key_type key;
        key.ip.assign(ip);
        key.port = port;

        scoped_lock _lock(mutex_);

        client_info_map_type::iterator it = client_infos_.find(key);
        if (client_infos_.end() != it)
        {
            channel_container_type::iterator chan_it;

            if (_find_channel(it, channel, chan_it))
            {
                chan_it->refresh_time = boost::posix_time::microsec_clock::local_time();
            }
        }
    }

    void clients_mgr_t::check_overtime(uint32_t millsec)
    {
        scoped_lock _lock(mutex_);

        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::ptime deadline = now - boost::posix_time::milliseconds(millsec);
        for (client_info_map_type::iterator it = client_infos_.begin(); client_infos_.end() != it;)
        {
            channel_container_type& channels = it->second.channels;
            for (channel_container_type::iterator it2 = channels.begin(); channels.end() != it2;)
            {
                if (deadline >= it2->refresh_time)
                {
                    std::string t1 = boost::posix_time::to_iso_string(now);
                    std::string t2 = boost::posix_time::to_iso_string(it2->refresh_time);
                    udp_session::logger::debug("client mgr check overtime|ip(%s),port(%d),channel(%d),tm(%s-%s),channel num(%d)",
                        it->first.ip.c_str(), it->first.port, it2->channel, t1.c_str(), t2.c_str(), channels.size());

                    (void)_del_client_sink(it, it2);
                }
                else
                {
                    ++it2;
                }
            }

            if (channels.empty())
            {
                // it=client_infos_.erase(it++);
                client_infos_.erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }

    uint32_t clients_mgr_t::_del_client_sink(client_info_map_type::iterator it, uint32_t channel,uint8_t code)
    {
        for (channel_container_type::iterator it2 = it->second.channels.begin(); it->second.channels.end() != it2; ++it2)
        {
            if (channel == it2->channel
                && code == it2->code)
            {
                return _del_client_sink(it, it2) ? udp_session::error::ok : udp_session::error::del_sink_cb_fail;
            }
        }
        return udp_session::error::channel_not_exists;
    }

    uint32_t clients_mgr_t::_del_client_sink(client_info_map_type::iterator it, uint32_t channel,uint8_t code, uint16_t rtp_port, uint16_t rtcp_port, uint8_t demux_flag, uint32_t demux_id)
    {
        for (channel_container_type::iterator it2 = it->second.channels.begin(); it->second.channels.end() != it2; ++it2)
        {
            if ((channel == it2->channel)
                && (code == it2->code)
                && (rtp_port == it2->rtp_port)
                && (rtcp_port == it2->rtcp_port)
                && (demux_flag == it2->demux_flag)
                && ((0 == demux_flag) || (demux_id == it2->demux_id)))
            {
                return _del_client_sink(it, it2) ? udp_session::error::ok : udp_session::error::del_sink_cb_fail;
            }
        }
        return udp_session::error::channel_not_exists;
    }

    bool clients_mgr_t::_del_client_sink(client_info_map_type::iterator it, channel_container_type::iterator &it2)
    {
        int32_t result = del_sink_cb_(it2->channel, it2->code,0, it->first.ip.c_str(), it2->rtp_port, it2->rtcp_port, it2->demux_flag, it2->demux_id);

        UDP_SVR_PRINT(level_info,"client mgr del sink1|ip(%s),port(%d),channel(%d),rtp_port(%d-%d),demux(%d-%d),result(%d)",
            it->first.ip.c_str(), it->first.port, it2->channel, it2->rtp_port, it2->rtcp_port, it2->demux_flag, it2->demux_id, result);
        if (0 != result)
        {
            return false;
        }
        it2 = it->second.channels.erase(it2);
        return true;
    }

    bool clients_mgr_t::_find_channel(client_info_map_type::iterator it, uint16_t channel, channel_container_type::iterator& chan_it)
    {
        if (client_infos_.end() == it)
        {
            return false;
        }

        for (chan_it = it->second.channels.begin(); it->second.channels.end() != chan_it; ++chan_it)
        {
            if (channel == chan_it->channel)
            {
                return true;
            }
        }
        return false;
    }

    bool ipv4_to_string(uint32_t u, std::string& ip)
    {
        try
        {
            ip = boost::asio::ip::address_v4(u).to_string();
        }
        catch (...)
        {
            return false;
        }

        return true;
    }

    typedef boost::asio::io_service udp_service_impl_t;

    session_server_t::session_server_t()
        :server_(NULL),
        msg_serializer_(NULL),
        events_(NULL)
    {}

    session_server_t::~session_server_t()
    {
        events_ = NULL;

        if (NULL != server_)
        {
            delete (udp_server_impl *)server_;
            server_ = NULL;
        }

        if (NULL != msg_serializer_)
        {
            delete (udp_session::msg_serialization_impl *)msg_serializer_;
            msg_serializer_ = NULL;
        }
        events_ = NULL;
    }

    bool session_server_t::init(const char *ip, uint16_t port, void *service)
    {
        udp_service_impl_t *service_impl = (udp_service_impl_t *)service;
        if (NULL == service_impl)
        {
            return false;
        }

        try
        {
            if (NULL == server_)
            {
                server_ = new udp_server_impl(*service_impl);
            }

            if (!server_->init(ip, port))
            {
                return false;
            }
        }
        catch (const std::exception& e)
        {
            return false;
        }

        server_->register_events(this);

        if (NULL == msg_serializer_)
        {
            msg_serializer_ = new udp_session::msg_serialization_impl;
        }

        return true;
    }

    bool session_server_t::on_receive_msg(const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size)
    {
        udp_session::msg_header_t head;
        memset(&head,0,sizeof(head));

        memcpy(&head,buf,sizeof(udp_session::msg_header_t));
        if (head.code != udp_session::send_regist)
        {
            return  on_receive_request(v4_ip,port,buf,size);
        }
        else
        {
            return  on_receive_response(v4_ip,port,buf,size);
        }
    }

    void session_server_t::register_callback(events_type *events)
    {
        events_ = events;
    }

    bool session_server_t::send_msg( const char *v4_ip, uint16_t port, udp_session::msg_request_t *request, void *response, response_done_callback_t done, void *ctx, uint32_t timeout )
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

        return server_->send_msg(v4_ip, port, request_buffer, request_buffer_len);
    }

    bool session_server_t::on_receive_request( const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size )
    {
        uint8_t request_parser_buffer[1024];
        udp_session::msg_request_t *request = (udp_session::msg_request_t *)request_parser_buffer;

        uint32_t request_parser_buffer_len = sizeof(request_parser_buffer);
        if (!msg_serializer_->deserialize_request(buf, size, request, request_parser_buffer_len))
        {
            UDP_SVR_PRINT(level_error,"deserialize_request fail|size(%d),buf_size(%d)", size, request_parser_buffer_len);
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
        case udp_session::get_sdp:
            events_->on_receive_get_sdp_msg(v4_ip, port, (const udp_session::get_sdp_request_msg_t *)request, (udp_session::get_sdp_response_msg_t *)response);
            break;
        case udp_session::play:
            events_->on_receive_play_msg(v4_ip, port, (const udp_session::play_request_msg_t *)request, (udp_session::play_response_msg_t *)response);
            break;
        case udp_session::stop:
            events_->on_receive_stop_msg(v4_ip, port, (const udp_session::stop_request_msg_t *)request, (udp_session::stop_response_msg_t *)response);
            break;
        case udp_session::heartbit:
            events_->on_receive_heartbit_msg(v4_ip, port, (const udp_session::heartbit_request_msg_t *)request);
            response_need_send = false;
            break;
        case udp_session::get_sdp_and_play:
            events_->on_receive_get_sdp_and_play_msg(v4_ip, port, (const udp_session::get_sdp_and_play_request_msg_t *)request, (udp_session::get_sdp_and_play_response_msg_t *)response);
            break;
        case udp_session::get_sdp_and_play_v1:
            events_->on_receive_get_sdp_and_play_msg_v1(v4_ip, port, (const udp_session::get_sdp_and_play_request_msg_v1_t *)request, (udp_session::get_sdp_and_play_response_msg_t *)response);
            break;
        case udp_session::send_data:
            events_->on_receive_send_data_msg(v4_ip, port, (const udp_session::send_data_request_msg_t *)request, (udp_session::send_data_response_msg_t *)response);
            break;
        case udp_session::heartbit2:
            events_->on_receive_heartbit2_msg(v4_ip, port, (const udp_session::heartbit2_request_msg_t *)request);
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
                UDP_SVR_PRINT(level_error,"serialize_response fail|size(%d)", response_buffer_len);
                return false;
            }

            server_->send_msg(v4_ip, port, response_buffer, response_buffer_len);
        }

        return true;
    }

    bool session_server_t::on_receive_response( const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size )
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
            case udp_session::send_regist:
                events_->on_receive_send_regist_msg(v4_ip, port,(xt_send_regist_response_t *)sync_result.response);
                break;
            default:
                events_->on_unknown_msg_response(v4_ip, port, response, sync_result.response);
                break;
            }
        }

        sync_result.done(sync_result.ctx, response->error_code);
        return true;
    }

    session_server_impl::session_server_impl(const xt_udp_session_config_t *config)
    {
        (void)memset(&config_, 0, sizeof(xt_udp_session_config_t));

        (void)strncpy(config_.ip, config->ip, sizeof(config_.ip));
        config_.port = config->port;

        config_.service = config->service;

        config_.heartbit_check_millsec = config->heartbit_check_millsec;
        config_.client_overtime_millsec = config->client_overtime_millsec;

        config_.query_sdp_callback = config->query_sdp_callback;
        config_.add_sink_callback = config->add_sink_callback;
        config_.del_sink_callback = config->del_sink_callback;
        config_.send_data_callback = config->send_data_callback;
        config_.send_regist_data_callback = config->send_regist_data_callback;
        request_sequence_ = 0;
    }

    bool session_server_impl::init()
    {
        UDP_SVR_PRINT(level_info,"session server init|ip(%s),port(%d),heartbit_check(%d),client_overtime(%d)", config_.ip, config_.port, config_.heartbit_check_millsec, config_.client_overtime_millsec);

        session_server_t::register_callback(this);

        if (!session_server_t::init(config_.ip, config_.port, config_.service))
        {
            UDP_SVR_PRINT(level_error,"session server init fail|sock init error(maybe port bind fail)");
            return false;
        }

        if (config_.heartbit_check_millsec > 0)
        {
            timer_t::init(*(udp_service_impl_t *)config_.service, config_.heartbit_check_millsec);
        }

        clients_mgr_.set_callback(config_.del_sink_callback, config_.add_sink_callback);

        return true;
    }

    bool session_server_impl::on_receive_get_sdp_msg(const char *v4_ip, uint16_t port, const udp_session::get_sdp_request_msg_t *request, udp_session::get_sdp_response_msg_t *response)
    {
        UDP_SVR_PRINT(level_error,"receive get sdp msg begin|ip(%s),port(%d),channel(%d)", v4_ip, port, request->channel);

        uint32_t len = MAX_SDP_LENGTH;
        int32_t result = config_.query_sdp_callback(request->channel,request->code, response->sdp, &len);
        if (0 != result)
        {
            UDP_SVR_PRINT(level_error,"receive get sdp msg fail|result(%d)", result);
            response->error_code = udp_session::error::get_sdp_cb_fail;
            return false;
        }

        response->error_code = udp_session::error::ok;
        response->length = len;

        UDP_SVR_PRINT(level_info,"receive get sdp msg ok|sdp_len(%d)", len);

        return true;
    }

    bool session_server_impl::on_receive_play_msg(const char *v4_ip, uint16_t port, const udp_session::play_request_msg_t *request, udp_session::play_response_msg_t *response)
    {
        UDP_SVR_PRINT(level_info,"receive play msg begin|ip(%s),port(%d),channel(%d),rtp_port(%d-%d),demux(%d-%d)",
            v4_ip, port, request->channel, request->client_rtp_port, request->client_rtcp_port, request->demux_flag, request->demux_id);

        uint16_t rtp_port = request->client_rtp_port;
        uint16_t rtcp_port = request->client_rtcp_port;
        uint8_t demux_flag = request->demux_flag;
        uint32_t demux_id = request->demux_id;


        uint32_t result = clients_mgr_.add_client(v4_ip, port, request->channel,request->code,rtp_port, rtcp_port, demux_flag, demux_id);
        if (udp_session::error::ok != result)
        {
            UDP_SVR_PRINT(level_error,"receive play msg fail|result(%d)", result);
            response->error_code = result;
            return false;
        }

        response->error_code = udp_session::error::ok;
        response->proto = request->proto;
        response->server_rtp_port = rtp_port;
        response->server_rtcp_port = rtcp_port;
        response->demux_flag = demux_flag;
        response->demux_id = demux_id;

        printf("server_rtp_port:%d server_rtcp_port:%d\n",response->server_rtp_port,
            response->server_rtcp_port);

        UDP_SVR_PRINT(level_info,"receive play msg ok|ip(%s),port(%d),channel(%d),s_rtp_port(%d-%d),s_demux(%d-%d)",
            v4_ip, port, request->channel, response->server_rtp_port, response->server_rtcp_port, response->demux_flag, response->demux_id);

        return true;
    }

    bool session_server_impl::on_receive_stop_msg(const char *v4_ip, uint16_t port, const udp_session::stop_request_msg_t *request, udp_session::stop_response_msg_t *response)
    {
        UDP_SVR_PRINT(level_info,"receive stop msg begin|ip(%s),port(%d),channel(%d),rtp_port(%d-%d),demux(%d-%d)",
            v4_ip, port, request->channel, request->client_rtp_port, request->client_rtcp_port, request->demux_flag, request->demux_id);

        uint32_t result = 0;
        if (2 == request->proto)
        {
            std::string ip;
            if (!ipv4_to_string(request->demux_id, ip))
            {
                UDP_SVR_PRINT(level_error,"receive stop msg fail|bad multicast ip(%x)", request->demux_id);
                response->error_code = udp_session::error::bad_ip_addr;
                return false;
            }

            result = clients_mgr_.del_client(ip.c_str(), port, request->channel, request->code,request->client_rtp_port, request->client_rtcp_port, request->demux_flag, request->demux_id);
        }
        else
        {
            result = clients_mgr_.del_client(v4_ip, port, request->channel, request->code, request->client_rtp_port, request->client_rtcp_port, request->demux_flag, request->demux_id);
        }

        if (udp_session::error::ok != result)
        {
            UDP_SVR_PRINT(level_error,"receive stop msg fail|result(%d)", result);
            response->error_code = result;
            return false;
        }

        response->error_code = udp_session::error::ok;

        UDP_SVR_PRINT(level_info,"receive stop msg ok|ip(%s),port(%d),channel(%d),rtp_port(%d-%d),demux(%d-%d)",
            v4_ip, port, request->channel, request->client_rtp_port, request->client_rtcp_port, request->demux_flag, request->demux_id);

        return true;
    }

    bool session_server_impl::on_receive_heartbit_msg(const char *v4_ip, uint16_t port, const udp_session::heartbit_request_msg_t *request)
    {
        clients_mgr_.heartbit_client(v4_ip, port, request->channel);
        return true;
    }

    bool session_server_impl::on_receive_get_sdp_and_play_msg(const char *v4_ip, uint16_t port, const udp_session::get_sdp_and_play_request_msg_t *request, udp_session::get_sdp_and_play_response_msg_t *response)
    {
        UDP_SVR_PRINT(level_info,"receive get sdp play msg begin|ip(%s),port(%d),channel(%d)", v4_ip, port, request->channel);

        uint32_t len = MAX_SDP_LENGTH;
        int32_t result = config_.query_sdp_callback(request->channel,request->code,response->sdp, &len);

        UDP_SVR_PRINT(level_info,"config_.query_sdp_callback channel:%d sdp:%s len:%d", 
            request->channel, response->sdp, len);
        if (0 != result)
        {
            UDP_SVR_PRINT(level_error,"receive get sdp play msg fail|result(%d)", result);
            response->error_code = udp_session::error::get_sdp_cb_fail;
            return false;
        }

        uint16_t rtp_port = request->client_rtp_port;
        uint16_t rtcp_port = request->client_rtcp_port;
        uint8_t demux_flag = request->demux_flag;
        uint32_t demux_id = request->demux_id;

        uint32_t result2 = 0;
        if (2 == request->proto)
        {
            std::string ip;
            if (!ipv4_to_string(demux_id, ip))
            {
                UDP_SVR_PRINT(level_error,"receive get sdp play msg fail|bad multicast ip(%x)", demux_id);
                response->error_code = udp_session::error::bad_ip_addr;
                return false;
            }

            result2 = clients_mgr_.add_client(ip.c_str(), port, request->channel, request->code, rtp_port, rtcp_port, demux_flag, demux_id);
        }
        else
        {
            result2 = clients_mgr_.add_client(v4_ip, port, request->channel, request->code, rtp_port, rtcp_port, demux_flag, demux_id);
        }


        if (udp_session::error::ok != result2)
        {
            UDP_SVR_PRINT(level_error,"receive get sdp play msg fail|result2(%d)", result2);
            response->error_code = result2;
            return false;
        }

        response->length = len;
        response->proto = request->proto;
        response->server_rtp_port = rtp_port;
        response->server_rtcp_port = rtcp_port;
        response->demux_flag = demux_flag;
        response->demux_id = demux_id;
        response->error_code = udp_session::error::ok;

        UDP_SVR_PRINT(level_info,"receive get sdp play msg ok|ip(%s),port(%d),channel(%d),s_rtp_port(%d-%d),s_demux(%d-%d), sdp_len(%d)",
            v4_ip, port, request->channel, response->server_rtp_port, response->server_rtcp_port, response->demux_flag, response->demux_id, len);

        return true;
    }

    bool session_server_impl::on_receive_get_sdp_and_play_msg_v1(const char *v4_ip, uint16_t port, const udp_session::get_sdp_and_play_request_msg_v1_t *request, udp_session::get_sdp_and_play_response_msg_t *response)
    {
        UDP_SVR_PRINT(level_info,"receive get sdp play msg begin|ip(%s),port(%d),channel(%d) code(%d)", v4_ip, port, request->channel,request->code);

        uint32_t len = MAX_SDP_LENGTH;
        int32_t result = config_.query_sdp_callback(request->channel,request->code, response->sdp, &len);

        UDP_SVR_PRINT(level_info,"config_.query_sdp_callback channel:%d sdp:%s len:%d", 
            request->channel, response->sdp, len);
        if (0 != result)
        {
            UDP_SVR_PRINT(level_error,"receive get sdp play msg fail|result(%d)", result);
            response->error_code = udp_session::error::get_sdp_cb_fail;
            return false;
        }

        uint16_t rtp_port = request->client_rtp_port;
        uint16_t rtcp_port = request->client_rtcp_port;
        uint8_t demux_flag = request->demux_flag;
        uint32_t demux_id = request->demux_id;

        uint32_t result2 = 0;
        if (2 == request->proto)
        {
            std::string ip;
            if (!ipv4_to_string(demux_id, ip))
            {
                UDP_SVR_PRINT(level_error,"receive get sdp play msg fail|bad multicast ip(%x)", demux_id);
                response->error_code = udp_session::error::bad_ip_addr;
                return false;
            }

            result2 = clients_mgr_.add_client(ip.c_str(), port, request->channel, request->code, rtp_port, rtcp_port, demux_flag, demux_id);
        }
        else
        {
            result2 = clients_mgr_.add_client(v4_ip, port, request->channel, request->code, rtp_port, rtcp_port, demux_flag, demux_id);
        }

        if (udp_session::error::ok != result2)
        {
            UDP_SVR_PRINT(level_error,"receive get sdp play msg fail|result2(%d)", result2);
            response->error_code = result2;
            return false;
        }
        response->length = len;
        response->proto = request->proto;
        response->server_rtp_port = rtp_port;
        response->server_rtcp_port = rtcp_port;
        response->demux_flag = demux_flag;
        response->demux_id = demux_id;
        response->error_code = udp_session::error::ok;
        UDP_SVR_PRINT(level_info,"receive get sdp play msg ok|ip(%s),port(%d),channel(%d),code(%d),s_rtp_port(%d-%d),s_demux(%d-%d), sdp_len(%d)",
            v4_ip, port, request->channel, request->code,response->server_rtp_port, response->server_rtcp_port, response->demux_flag, response->demux_id, len);
        return true;

    }

    bool session_server_impl::on_receive_send_data_msg(const char *v4_ip, uint16_t port, const udp_session::send_data_request_msg_t *request, udp_session::send_data_response_msg_t *response)
    {
        if (NULL == config_.send_data_callback)
        {
            response->error_code = udp_session::error::send_data_cb_fail;
            return false;
        }
        response->error_code = config_.send_data_callback(v4_ip, port, request->channel, request->code, request->content, request->length);
        return true;
    }

    bool session_server_impl::on_receive_send_regist_msg( const char *v4_ip, uint16_t port, xt_send_regist_response_t *response )
    {
        if (NULL == config_.send_regist_data_callback)
        {
            response->status = 1;
            return false;
        }
        response->status = 0;
        config_.send_regist_data_callback(v4_ip, port, response->status);
        return true;
    }

    void session_server_impl::on_timer(bool operation_aborted)
    {
        if (!operation_aborted)
        {
            clients_mgr_.check_overtime(config_.client_overtime_millsec);
        }
    }

    void *session_server_impl::create_service()
    {
        return new udp_service_impl_t;
    }

    void session_server_impl::run_service(void *service)
    {
        udp_service_impl_t * impl = (udp_service_impl_t *)service;
        if (NULL != impl)
        {
            impl->poll();
        }
    }

    void session_server_impl::destroy_service(void *service)
    {
        udp_service_impl_t * impl = (udp_service_impl_t *)service;
        if (NULL != impl)
        {
            impl->stop();
            delete impl;
        }
    }

    int32_t session_server_impl::send_regist_method( const char *ip, uint16_t port, const xt_send_regist_request_t *request, xt_send_regist_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout )
    {
        if (request->length > XT_UDP_SESSION_SEND_DATA_MAX)
        {
            return udp_session::error::send_data_content_too_long;
        }

        uint8_t send_data_buffer[sizeof(udp_session::send_regist_request_msg_t) + XT_UDP_SESSION_SEND_DATA_MAX]={0};
        udp_session::send_regist_request_msg_t *p_msg = (udp_session::send_regist_request_msg_t *)send_data_buffer;

        new_a_request_msg(udp_session::send_regist, *p_msg);
        (void)memcpy(p_msg->ids, request->ids, request->length);
        p_msg->length = request->length;
        return send_msg(ip, port, p_msg, response, done, ctx, timeout);
    }

    int32_t session_server_impl::send_msg( const char *ip, uint16_t port, udp_session::msg_request_t *request, void *response, response_done_callback_t done, void *ctx, uint32_t timeout )
    {
        //sync send operation
        if (NULL != done)
        {
            return session_server_t::send_msg(ip, port, request, response, done, ctx, timeout) ? udp_session::error::ok : udp_session::error::send_msg_fail;
        }

        // else ...
        // async operation
        boost::intrusive_ptr<async_wait_context_t> context(async_wait_context_t::allocate());

        done = async_wait_context_t::async_wait_handler;
        ctx = context.get();

        intrusive_ptr_add_ref(context.get());

        if (!session_server_t::send_msg(ip, port, request, response, done, ctx, timeout))
        {
            return udp_session::error::send_msg_fail;
        }

        return context->wait(timeout);
    }

    void session_server_impl::new_a_request_msg(udp_session::msg_code code, udp_session::msg_request_t& msg)
    {
        msg.code = code;
        msg.version = UDP_SESSION_MSG_VERSION;
        msg.sequence = request_sequence_++;
    }

    int32_t session_server_impl::send_stop_regist_method( const char *ip, uint16_t port, const xt_send_regist_request_t *request, xt_send_regist_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout )
    {
        if (request->length > XT_UDP_SESSION_SEND_DATA_MAX)
        {
            return udp_session::error::send_data_content_too_long;
        }

        uint8_t send_data_buffer[sizeof(udp_session::send_regist_request_msg_t) + XT_UDP_SESSION_SEND_DATA_MAX];
        udp_session::send_regist_request_msg_t *p_msg = (udp_session::send_regist_request_msg_t *)send_data_buffer;

        new_a_request_msg(udp_session::send_stop_regist, *p_msg);
        return send_msg(ip, port, p_msg, response, done, ctx, timeout);
    }

    bool session_server_impl::on_receive_heartbit2_msg(const char *v4_ip, uint16_t port, const udp_session::heartbit2_request_msg_t *request)
    {
        std::string ip;
        if (ipv4_to_string(request->ip, ip))
        {
            clients_mgr_.heartbit_client(ip.c_str(), request->port, request->channel);
        }
        return true;
    }

    bool session_server_impl::on_unknown_msg_response( const char *v4_ip, uint16_t port, const udp_session::msg_response_t *response, void *xresponse )
    {
        return true;
    }

    bool session_server_impl::on_receive_send_stop_regist_msg(const char *v4_ip, uint16_t port, xt_send_regist_response_t *response )
    {
        if (NULL == config_.send_regist_data_callback)
        {
            response->status = 1;
            return false;
        }
        response->status = 0;
        config_.send_regist_data_callback(v4_ip, port, response->status);
        return true;
    }
}

