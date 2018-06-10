#include "rtsp_global_mgr.h"
#include "RvRtspMsg.h"

#include <boost/typeof/typeof.hpp>
#include <boost/bind.hpp>

namespace xt_rtsp_client
{
    bool rv_msg_header_add_fields(RvRtspHandle hRtsp, RvRtspMsgMessageHandle hRtspMsgMessage, RvRtspMsgHeaderHandle& additionalFields, 
        const std::vector<std::string>& fields, RvBool bIsRequest /* = RV_TRUE */, RvChar delimiter /* = */ )
    {
        for (std::size_t count = 0; count < fields.size(); ++count)
        {
            RvRtspMsgAppHeader msgHeader;
            RvChar *field = const_cast<RvChar *>(fields[count].c_str());

            msgHeader.bIsRequest = bIsRequest;
            msgHeader.hRtspMsgMessage = hRtspMsgMessage;
            msgHeader.delimiter = delimiter;

            msgHeader.headerFields = &field;
            msgHeader.headerFieldsSize = 1;
            msgHeader.headerFieldStrLen = (RvUint32)fields[count].length();

            RvStatus status = RV_OK;
            if (NULL == additionalFields)
            {
                status = ::RvRtspMsgAddHeaderFields(hRtsp, &msgHeader, &additionalFields);
            }
            else
            {
                status = ::RvRtspMsgAddGenericHeaderFields(hRtsp, additionalFields, &msgHeader);
            }
            if (RV_OK != status)
            {
                return false;
            }
        }
        return true;
    }

    void rtsp_global_mgr::init(uint32_t check_timeout_priod)
    {
        thread_timer::set_interval(check_timeout_priod);
    }

    void rtsp_global_mgr::term()
    {
        thread_timer::close();
    }

    xt_rtsp_client_status_t rtsp_global_mgr::create_client(const rtsp_client_config_t *config, rtsp_client_handle_t *pclient)
    {
        rtsp_client_info_t *client_impl = NULL;
        RvRtspConfiguration rv_config;

        rv_config.maxConnections = config->max_connections;
#ifdef _WIN32
        (void)strncpy_s(rv_config.strDnsAddress, config->dns_ip_address, RV_RTSP_IP_ADDRESS_MAX_LENGTH);
#else
        (void)strncpy(rv_config.strDnsAddress, config->dns_ip_address, RV_RTSP_IP_ADDRESS_MAX_LENGTH);
#endif

        rv_config.maxRtspMsgHeadersInMessage = 128;
        rv_config.memoryElementsNumber = 1024;
        rv_config.memoryElementsSize = 1024;
        rv_config.msgRequestElementsNumber = 1024;
        rv_config.msgResponseElementsNumber = 1024;

        RvStatus stat = this->client_init(&rv_config, client_impl);
        if (RV_OK != stat)
        {
            return RTSP_CLIENT_STATUS_NETWORK_PROBLEM;
        }

        *pclient = client_impl;

        return RTSP_CLIENT_STATUS_OK;
    }

    void rtsp_global_mgr::destroy_client(rtsp_client_handle_t client)
    {
        if (NULL != client)
        {
            rtsp_client_info_t *client_impl = (rtsp_client_info_t *)client;
            this->client_end(client_impl);
        }
    }

    xt_rtsp_client_status_t rtsp_global_mgr::create_connection(rtsp_client_handle_t client, const char *uri, const char *local_ip, uint16_t local_port, const rtsp_client_connection_config_t *config, int8_t *connected, rtsp_connection_handle_t *pconnection)
    {

        if (NULL == client)
        {
            return RTSP_CLIENT_STATUS_NULLPTR;
        }

        RvRtspConnectionConfiguration rv_config;
        rv_config.describeResponseTimeOut = config->describe_response_timeout;
        rv_config.dnsMaxResults = config->dns_max_results;
        rv_config.maxHeadersInMessage = config->max_headers_in_msg;
        rv_config.maxSessions = config->max_sessions;
        rv_config.maxUrlsInMessage = config->max_urls_in_msg;
        rv_config.maxWaitingDescribeRequests = config->waiting_describe_requests;
        rv_config.transmitQueueSize = config->transmit_queue_size;

        rtsp_connection_info_t *connection_impl = NULL;
        rtsp_client_info_t *client_impl = (rtsp_client_info_t *)client;

        boost::unique_future<RvStatus> fut = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::connection_init, this, client_impl, uri, local_ip, local_port, &rv_config, connected, boost::ref(connection_impl)));

        if (RV_OK != fut.get())
        {
            return RTSP_CLIENT_STATUS_NETWORK_PROBLEM;
        }

        add_connect_request(uri,connection_impl);

        *pconnection = connection_impl;

        return RTSP_CLIENT_STATUS_OK;
    }

    void rtsp_global_mgr::destroy_connection(rtsp_connection_handle_t connection)
    {
        rtsp_connection_info_t *connection_impl = (rtsp_connection_info_t *)connection;
        if (NULL != connection_impl)
        {
            rtsp_client_info_t *client_impl = connection_impl->get_client();
            if (NULL != client_impl)
            {
                client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::connection_end, this, connection_impl));
                destroy_connection_info(connection_impl);
                delete_connect_request(connection_impl);
            }
        }
    }

    xt_rtsp_client_status_t rtsp_global_mgr::create_session(rtsp_connection_handle_t connection, const rtsp_client_session_config_t *config, rtsp_session_handle_t *psession)
    {
        rtsp_connection_info_t *connection_impl = (rtsp_connection_info_t *)connection;
        if (NULL == connection_impl)
        {
            return RTSP_CLIENT_STATUS_NULLPTR;
        }

        rtsp_client_info_t *client_impl = connection_impl->get_client();
        if (NULL == client_impl)
        {
            return RTSP_CLIENT_STATUS_NULLPTR;
        }

        RvRtspSessionConfiguration rv_config;
        rv_config.responseTimeOutResolution = config->response_timeout;
        rv_config.pingTransmissionTimeOutResolution = config->ping_transmission_timeout;

        rtsp_session_info_t *session_impl = NULL;
        boost::unique_future<RvStatus> fut = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::session_init, this, connection_impl, &rv_config, boost::ref(session_impl)));

        if (RV_OK != fut.get())
        {
            return RTSP_CLIENT_STATUS_NETWORK_PROBLEM;
        }

        *psession = session_impl;

        return RTSP_CLIENT_STATUS_OK;
    }

    void rtsp_global_mgr::destroy_session(rtsp_session_handle_t session)
    {
        rtsp_session_info_t *session_impl = (rtsp_session_info_t *)session;
        if (NULL != session_impl)
        {
            rtsp_connection_info_t *connection_impl = session_impl->get_connection();

            if (NULL != connection_impl)
            {
                rtsp_client_info_t *client_impl = connection_impl->get_client();

                if (NULL != client_impl)
                {
                    client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::session_end, this, session_impl));
                }
            }
        }
    }


    bool rtsp_global_mgr::async_connect(rtsp_connection_handle_t connection, const rtsp_client_connect_request_t *request, rtsp_client_connect_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        rtsp_connection_info_t *connection_impl = (rtsp_connection_info_t *)connection;
        if (NULL == connection_impl)
        {
            return false;
        }

        rtsp_client_info_t *client_impl = connection_impl->get_client();
        if (NULL == client_impl)
        {
            return false;
        }

        if (!connect_requests_mgr_.request_task(connection_impl, rtsp_task_factory::create_async_task<rtsp_connect_task_t>(client_impl->native_handle(), request, response, done, ctx, timeout)))
        {
            return false;
        }
        boost::unique_future<RvStatus> fut = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::connect, this, connection_impl));
        if (RV_OK != fut.get())
        {
            connect_requests_mgr_.cancel_task(connection_impl);
            return false;
        }

        return true;
    }

    bool rtsp_global_mgr::async_describe_request(rtsp_connection_handle_t connection, const rtsp_client_describe_request_t *request, rtsp_client_describe_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        rtsp_connection_info_t *connection_impl = (rtsp_connection_info_t *)connection;
        if (NULL == connection_impl)
        {
            return false;
        }

        rtsp_client_info_t *client_impl = connection_impl->get_client();
        if (NULL == client_impl)
        {
            return false;
        }

        RvUint16 next_seq = 0;
        boost::unique_future<RvStatus> fut = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::get_next_cseq, this, connection_impl, &next_seq));
        if (RV_OK != fut.get())
        {
            return false;
        }

        if (!add_seq_request(connection_impl, next_seq, rtsp_task_factory::create_async_task<rtsp_describe_task_t>(client_impl->native_handle(), request, response, done, ctx, timeout)))
        {
            return false;
        }

        boost::unique_future<RvStatus> fut2 = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::describe, this, connection_impl, next_seq, request->uri));

        if (RV_OK != fut2.get())
        {
            cancel_seq_request(connection_impl, next_seq);
            return false;
        }

        return true;
    }

    bool rtsp_global_mgr::async_setup_request(rtsp_session_handle_t session, const rtsp_client_setup_request_t *request, rtsp_client_setup_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        rtsp_session_info_t *session_impl = (rtsp_session_info_t *)session;
        if (NULL == session_impl)
        {
            return false;
        }

        rtsp_connection_info_t *connection_impl = session_impl->get_connection();
        if (NULL == connection_impl)
        {
            return false;
        }

        rtsp_client_info_t *client_impl = connection_impl->get_client();
        if (NULL == client_impl)
        {
            return false;
        }

        RvUint16 next_seq = 0;
        boost::unique_future<RvStatus> fut = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::get_next_cseq, this, connection_impl, &next_seq));
        if (RV_OK != fut.get())
        {
            return false;
        }

        if (!add_seq_request(connection_impl, next_seq, rtsp_task_factory::create_async_task<rtsp_setup_task_t>(client_impl->native_handle(), request, response, done, ctx, timeout)))
        {
            return false;
        }

        boost::unique_future<RvStatus> fut2 = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::set_uri, this, session_impl, request->uri));
        if (RV_OK != fut2.get())
        {
            cancel_seq_request(connection_impl, next_seq);
            return false;
        }

        RvRtspTransportHeader rtsp_transport_header;
        rtsp_transport_header.clientPortA = request->client_rtp_port;
        rtsp_transport_header.clientPortB = request->client_rtcp_port;
        rtsp_transport_header.serverPortA = request->server_rtp_port;
        rtsp_transport_header.serverPortB = request->server_rtcp_port;
        rtsp_transport_header.isUnicast = request->is_unicast ? RV_TRUE : RV_FALSE;
#ifdef _WIN32
        strncpy_s(rtsp_transport_header.destination, request->destination, sizeof(rtsp_transport_header.destination));
#else
        strncpy(rtsp_transport_header.destination, request->destination, sizeof(rtsp_transport_header.destination));
#endif

        rtsp_transport_header.additionalFields = NULL;
        if (request->client_demux)
        {
            std::vector<std::string> fields;
            char id[64] = "";
            sprintf(id, "%d", request->client_demuxid);
            std::string sid = "demuxid=";
            sid += id;
            fields.push_back(sid);

            RvRtspHandle handle_ = client_impl->native_handle();
            rv_msg_header_add_fields(handle_, NULL, rtsp_transport_header.additionalFields, fields, RV_TRUE, ';');
        }

        boost::unique_future<RvStatus> fut3 = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::setup, this, session_impl, next_seq, &rtsp_transport_header));
        if (RV_OK != fut3.get())
        {
            cancel_seq_request(connection_impl, next_seq);
            return false;
        }

        return true;
    }

    bool rtsp_global_mgr::async_play_request(rtsp_session_handle_t session, const rtsp_client_play_request_t *request, rtsp_client_play_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        rtsp_session_info_t *session_impl = (rtsp_session_info_t *)session;
        if (NULL == session_impl)
        {
            return false;
        }

        rtsp_connection_info_t *connection_impl = session_impl->get_connection();
        if (NULL == connection_impl)
        {
            return false;
        }

        rtsp_client_info_t *client_impl = connection_impl->get_client();
        if (NULL == client_impl)
        {
            return false;
        }

        RvUint16 next_seq = 0;
        boost::unique_future<RvStatus> fut = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::get_next_cseq, this, connection_impl, &next_seq));
        if (RV_OK != fut.get())
        {
            return false;
        }

        if (!add_seq_request(connection_impl, next_seq, rtsp_task_factory::create_async_task<rtsp_play_task_t>(client_impl->native_handle(), request, response, done, ctx, timeout)))
        {
            return false;
        }


        boost::unique_future<RvStatus> fut2 = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::set_uri, this, session_impl, request->uri));
        if (RV_OK != fut2.get())
        {
            cancel_seq_request(connection_impl, next_seq);
            return false;
        }

        RvRtspNptTime range;
        range.format = (RvRtspNptFormat)request->range.format;
        range.hours = request->range.hours;
        range.minutes = request->range.mintues;
        range.seconds = request->range.seconds;

        boost::unique_future<RvStatus> fut3 = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::play, this, session_impl, next_seq, &range, request->scale));
        if (RV_OK != fut3.get())
        {
            cancel_seq_request(connection_impl, next_seq);
            return false;
        }

        return true;
    }

    bool rtsp_global_mgr::async_pause_request(rtsp_session_handle_t session, const rtsp_client_pause_request_t *request, rtsp_client_pause_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        rtsp_session_info_t *session_impl = (rtsp_session_info_t *)session;
        if (NULL == session_impl)
        {
            return false;
        }

        rtsp_connection_info_t *connection_impl = session_impl->get_connection();
        if (NULL == connection_impl)
        {
            return false;
        }

        rtsp_client_info_t *client_impl = connection_impl->get_client();
        if (NULL == client_impl)
        {
            return false;
        }

        RvUint16 next_seq = 0;
        boost::unique_future<RvStatus> fut = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::get_next_cseq, this, connection_impl, &next_seq));
        if (RV_OK != fut.get())
        {
            return false;
        }

        if (!add_seq_request(connection_impl, next_seq, rtsp_task_factory::create_async_task<rtsp_pause_task_t>(client_impl->native_handle(), request, response, done, ctx, timeout)))
        {
            return false;
        }


        boost::unique_future<RvStatus> fut2 = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::set_uri, this, session_impl, request->uri));
        if (RV_OK != fut2.get())
        {
            cancel_seq_request(connection_impl, next_seq);
            return false;
        }

        boost::unique_future<RvStatus> fut3 = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::pause, this, session_impl, next_seq));
        if (RV_OK != fut3.get())
        {
            cancel_seq_request(connection_impl, next_seq);
            return false;
        }

        return true;
    }

    bool rtsp_global_mgr::async_teardown_request(rtsp_session_handle_t session, const rtsp_client_teardown_request_t *request, rtsp_client_teardown_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        rtsp_session_info_t *session_impl = (rtsp_session_info_t *)session;
        if (NULL == session_impl)
        {
            return false;
        }

        rtsp_connection_info_t *connection_impl = session_impl->get_connection();
        if (NULL == connection_impl)
        {
            return false;
        }

        rtsp_client_info_t *client_impl = connection_impl->get_client();
        if (NULL == client_impl)
        {
            return false;
        }

        RvUint16 next_seq = 0;
        boost::unique_future<RvStatus> fut = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::get_next_cseq, this, connection_impl, &next_seq));
        if (RV_OK != fut.get())
        {
            return false;
        }

        if (!add_seq_request(connection_impl, next_seq, rtsp_task_factory::create_async_task<rtsp_teardown_task_t>(client_impl->native_handle(), request, response, done, ctx, timeout)))
        {
            return false;
        }

        boost::unique_future<RvStatus> fut2 = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::set_uri, this, session_impl, request->uri));
        if (RV_OK != fut2.get())
        {
            cancel_seq_request(connection_impl, next_seq);
            return false;
        }

        boost::unique_future<RvStatus> fut3 = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::teardown, this, session_impl, next_seq));
        if (RV_OK != fut3.get())
        {
            cancel_seq_request(connection_impl, next_seq);
            return false;
        }

        return true;
    }

    bool rtsp_global_mgr::get_addr(rtsp_connection_handle_t connection, char ip[RTSP_CLIENT_IP_LEN], uint16_t *port)
    {
        rtsp_connection_info_t *connection_impl = (rtsp_connection_info_t *)connection;
        if (NULL == connection_impl)
        {
            return false;
        }

        rtsp_client_info_t *client_impl = connection_impl->get_client();
        if (NULL == client_impl)
        {
            return false;
        }

        boost::unique_future<RvStatus> fut = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::get_addr, this, connection_impl, ip, port));
        return RV_OK == fut.get();
    }

    void rtsp_global_mgr::disconnect(rtsp_connection_handle_t connection)
    {
        rtsp_connection_info_t *connection_impl = (rtsp_connection_info_t *)connection;
        if (NULL != connection_impl)
        {
            rtsp_client_info_t *client_impl = connection_impl->get_client();
            if (NULL != client_impl)
            {
                boost::unique_future<RvStatus> fut = client_impl->add_task(boost::bind(&rv_rtsp_client_adapter::disconnect, this, connection_impl));
                fut.get();
            }
        }
    }

    void rtsp_global_mgr::notify_connect(rtsp_connection_info_t *connection, void *response)
    {
        connect_requests_mgr_.response_task(connection, response);
    }

    void rtsp_global_mgr::notify_rtsp_request(rtsp_connection_info_t *connection, RvUint16 cseq, void *response)
    {
        if (NULL != connection)
        {
            spinlock_t::scoped_lock _lock(seq_requests_mgr_mutex_);
            seq_requests_mgr_[connection].response_task(cseq, response);
        }
    }

    void rtsp_global_mgr::notify_rtsp_request(rtsp_session_info_t *session, RvUint16 cseq, void *response)
    {
        if (NULL != session)
        {
            notify_rtsp_request(session->get_connection(), cseq, response);
        }
    }

    bool rtsp_global_mgr::add_seq_request(rtsp_connection_info_t *connection, RvUint16 seq, xt_rtsp_client::async_task_t *task)
    {
        spinlock_t::scoped_lock _lock(seq_requests_mgr_mutex_);
        return seq_requests_mgr_[connection].request_task(seq, task);
    }

    bool  rtsp_global_mgr::add_connect_request(const std::string uri,rtsp_connection_info_t * connection)
    {
        spinlock_t::scoped_lock _lock(seq_requests_mgr_mutex_);
        if (connect_request_mgr.empty())
        {
            connect_request_mgr[uri] = connection;
            return true;
        }
        std::map<std::string,rtsp_connection_info_t* >::iterator it = connect_request_mgr.begin();
        for (;it!=connect_request_mgr.end();++it)
        {
            std::string str_uri = it->first;
            if (str_uri == uri)
            {
                return false;
            }
            else
            {
                connect_request_mgr[uri] = connection;
                return true;
            }
        }
        return false;
    }

    bool rtsp_global_mgr::cancel_seq_request(rtsp_connection_info_t *connection, RvUint16 seq)
    {
        spinlock_t::scoped_lock _lock(seq_requests_mgr_mutex_);
        return seq_requests_mgr_[connection].cancel_task(seq);
    }

    bool  rtsp_global_mgr::delete_connect_request(rtsp_connection_info_t *connection)
    {
        spinlock_t::scoped_lock _lock(seq_requests_mgr_mutex_);
        std::map<std::string,rtsp_connection_info_t* >::iterator it = connect_request_mgr.begin();
        for (;it!=connect_request_mgr.end();++it)
        {
            if (it->second == connection)
            {
                connect_request_mgr.erase(it);
                return true;
            }
        }
        return false;
    }

    bool rtsp_global_mgr::destroy_connection_info(rtsp_connection_info_t *connection)
    {
        spinlock_t::scoped_lock _lock(seq_requests_mgr_mutex_);
        std::map<rtsp_connection_info_t *, async_task_mgr_t<RvUint16> >::iterator it = seq_requests_mgr_.begin();
        for (;it!=seq_requests_mgr_.end();++it)
        {
            if (it->first == connection)
            {
                seq_requests_mgr_.erase(it);
                return true;
            }
        }
        return false;
    }

    rtsp_connection_info_t*  rtsp_global_mgr::get_connection_by_uri(const std::string uri)
    {
        spinlock_t::scoped_lock _lock(seq_requests_mgr_mutex_);
        std::map<std::string,rtsp_connection_info_t* >::iterator it = connect_request_mgr.begin();
        for (;it!=connect_request_mgr.end();++it)
        {
            if (it->first == uri)
            {
                return it->second;
            }
        }
        return NULL;
    }
    void rtsp_global_mgr::check_overtime_request()
    {
        connect_requests_mgr_.check_overtime_tasks();

        spinlock_t::scoped_lock _lock(seq_requests_mgr_mutex_);
        for (BOOST_AUTO(it, seq_requests_mgr_.begin()); seq_requests_mgr_.end() != it; ++it)
        {
            it->second.check_overtime_tasks();
        }
    }

    void rtsp_global_mgr::on_timer()
    {
        check_overtime_request();
    }
}
