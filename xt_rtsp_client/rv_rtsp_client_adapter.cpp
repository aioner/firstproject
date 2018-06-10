#include "rv_rtsp_client_adapter.h"
#include "rtsp_global_mgr.h"

extern void RTSP_C_LOG(const rtsp_client_level_t log_level,const char* fmt,...);
#define XT_RTSP_CLIENT_ASSERT(x)

namespace 
{
    RvStatus RVCALLCONV RvRtspConnectionOnConnectCB(
        IN RvRtspConnectionHandle       hConnection,
        IN RvRtspConnectionAppHandle    hApp,
        IN RvBool                       success)
    {
        xt_rtsp_client::rtsp_connection_info_t *connection = (xt_rtsp_client::rtsp_connection_info_t *)hApp;
        if ((NULL != connection) && (RV_TRUE == success))
        {
            connection->set_connected();
        }

        xt_rtsp_client::rtsp_global_mgr::instance()->notify_connect(connection, &success);

        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspConnectionOnDisconnectCB(
        IN RvRtspConnectionHandle       hConnection,
        IN RvRtspConnectionAppHandle    hApp)
    {
        RTSP_C_LOG(rtsp_c_log_info, "RvRtspConnectionOnDisconnectCB. connection(%p)", hConnection);
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspConnectionOnErrorCB(
        IN  RvRtspConnectionHandle      hConnection,
        IN  RvRtspConnectionAppHandle   hApp,
        IN  RvRtspStringHandle          hURI,
        IN  RvRtspMethod                requestMethod,
        IN  RvRtspStatus                status,
        IN  RvRtspStringHandle          hPhrase)
    {
        RTSP_C_LOG(rtsp_c_log_info, "RvRtspConnectionOnErrorCB. connection(%p)", hConnection);
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspConnectionOnDescribeResponseCB(
        IN  RvRtspConnectionHandle              hConnection,
        IN  RvRtspConnectionAppHandle           hApp,
        IN  RvRtspConnectionDescribeAppHandle   hDescribe,
        IN  const RvRtspResponse                *pDescribeResponse,
        IN  RvRtspStringHandle                  hURI)
    {
        if ((NULL != pDescribeResponse) && (RV_TRUE == pDescribeResponse->cSeqValid))
        {
            xt_rtsp_client::rtsp_global_mgr::instance()->notify_rtsp_request((xt_rtsp_client::rtsp_connection_info_t *)hDescribe, pDescribeResponse->cSeq.value, (void *)pDescribeResponse);
        }
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspConnectionOnRedirectRequestCB(
        IN	RvRtspConnectionHandle      hConnection,
        IN  RvRtspConnectionAppHandle   hApp,
        IN	const RvRtspRequest         *pRequest)
    {
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspConnectionOnErrorExtCB(
        IN  RvRtspConnectionHandle              hConnection,
        IN  RvRtspConnectionAppHandle           hApp,
        IN  RvRtspConnectionDescribeAppHandle   hDescribe,
        IN  RvRtspStringHandle                  hURI,
        IN  RvRtspMethod                        requestMethod,
        IN  RvRtspStatus                        status,
        IN  RvRtspStringHandle                  hPhrase,
        IN  const RvRtspResponse                *pResponse)
    {
        RTSP_C_LOG(rtsp_c_log_info, "RvRtspConnectionOnErrorExtCB. connection(%p)", hApp);
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspConnectionOnReceiveCB(
        IN  RvRtspConnectionHandle      hConnection,
        IN  RvRtspConnectionAppHandle   hApp,
        IN  RvRtspRequest               *pRequest,
        IN  RvRtspResponse              *pResponse)
    {
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspConnectionOnSendCB(
        IN  RvRtspConnectionHandle      hConnection,
        IN  RvRtspConnectionAppHandle   hApp, 
        IN  RvRtspRequest               *pRequest,
        IN  RvRtspResponse              *pResponse)
    {
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspConnectionOnRawBufferReceiveCB(
        IN  RvRtspConnectionHandle      hConnection,
        IN  RvRtspConnectionAppHandle   hApp, 
        IN  RvUint8                     *pBuff,
        IN  RvUint32                    buffSize)
    {
        return RvRtspConnectionReceiveRawBuffer(hConnection, pBuff, buffSize);
    }

    RvStatus RVCALLCONV RvRtspSessionOnStateChangeCB(
        IN  RvRtspSessionHandle     hSession,
        IN  RvRtspSessionAppHandle  hApp,
        IN  RvRtspSessionState      currState,
        IN  RvRtspSessionState      newState,
        IN  const RvRtspResponse    *pResponse)
    {
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspSessionOnErrorCB(
        IN  RvRtspSessionHandle     hSession,
        IN  RvRtspSessionAppHandle  hApp,
        IN  RvRtspMethod            requestMethod,
        IN  RvRtspStatus            status,
        IN  RvRtspStringHandle      hPhrase)
    {
        RTSP_C_LOG(rtsp_c_log_info, "RvRtspSessionOnErrorCB. connection(%p)", hApp);
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspSessionOnDestructCB(
        IN  RvRtspSessionHandle     hSession,
        IN  RvRtspSessionAppHandle  hApp)
    {
        xt_rtsp_client::rtsp_session_info_t *session_impl = (xt_rtsp_client::rtsp_session_info_t *)hApp;
        if (NULL != session_impl)
        {
            session_impl->init(NULL, NULL);
        }

        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspSessionOnErrorExtCB(
        IN  RvRtspSessionHandle     hSession,
        IN  RvRtspSessionAppHandle  hApp,
        IN  RvRtspMethod            requestMethod,
        IN  RvRtspStatus            status,
        IN  RvRtspStringHandle      hPhrase,
        IN  const RvRtspResponse    *pResponse)
    {
        RTSP_C_LOG(rtsp_c_log_info, "RvRtspSessionOnErrorExtCB. connection(%p)", hApp);
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspSessionOnReceiveCB( 
        IN  RvRtspSessionHandle     hSession,
        IN  RvRtspSessionAppHandle  hApp, 
        IN  RvRtspRequest           *pRequest,
        IN  RvRtspResponse          *pResponse)
    {
        if ((NULL != pResponse) && (RV_TRUE == pResponse->cSeqValid))
        {
            xt_rtsp_client::rtsp_global_mgr::instance()->notify_rtsp_request((xt_rtsp_client::rtsp_session_info_t *)hApp, pResponse->cSeq.value, (void *)pResponse);
        }
        return RV_OK;
    }

    RvStatus RVCALLCONV RvRtspSessionOnSendCB( 
        IN  RvRtspSessionHandle     hSession,
        IN  RvRtspSessionAppHandle  hApp, 
        IN  RvRtspRequest           *pRequest,
        IN  RvRtspResponse          *pResponse)
    {
        return RV_OK;
    }
}

namespace xt_rtsp_client
{
    rtsp_client_info_t::rtsp_client_info_t()
        :closed_flag_t(),
        thread_(),
        task_queue_(),
        handle_(NULL)
    {}

    RvStatus rtsp_client_info_t::client_init(const RvRtspConfiguration *config)
    {
        boost::promise<RvStatus> promise;
        boost::unique_future<RvStatus> fut = promise.get_future();

        thread_.reset(new (std::nothrow) boost::thread(boost::bind(&rtsp_client_info_t::thread_worker, this, boost::ref(promise), config)));

        RvStatus stat = fut.get();
        if (RV_OK != stat)
        {
            client_end();
        }

        return stat;
    }

    void rtsp_client_info_t::client_end()
    {
        close();
        thread_->join();
    }

    bool rtsp_client_info_t::schdule_one_task()
    {
        return task_queue_.try_excuting_one();
    }

    void rtsp_client_info_t::thread_worker(boost::promise<RvStatus> &promise, const RvRtspConfiguration *config)
    {
        RvStatus stat = RvRtspInit(NULL, config, sizeof(RvRtspConfiguration), &handle_);
        promise.set_value(stat);

        if (RV_OK != stat)
        {
            return ;
        }

        while (!closed())
        {
            RvRtspMainLoop(handle_, RV_RTSP_MAINLOOP_TIMEOUT);
            schdule_one_task();
        }

        while (schdule_one_task());

        RvRtspEnd(handle_);
    }

    RvStatus rv_rtsp_client_adapter::client_init(const RvRtspConfiguration *config, rtsp_client_info_t *&client)
    {
        std::auto_ptr<rtsp_client_info_t> sp(new (std::nothrow) rtsp_client_info_t);
        RvStatus stat = sp->client_init(config);
        if (RV_OK == stat)
        {
            client = sp.release();
        }

        return stat;
    }

    RvStatus rv_rtsp_client_adapter::client_end(rtsp_client_info_t *client)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != client);

        client->client_end();
        delete client;

        return true;
    }

    RvStatus rv_rtsp_client_adapter::connection_init(rtsp_client_info_t *client, const char *uri, const char *local_ip, uint16_t local_port, const RvRtspConnectionConfiguration *pConfiguration, int8_t *connected, rtsp_connection_info_t *&connection)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != client);

        if (NULL == connected)
        {
            return RV_ERROR_NULLPTR;
        }

        *connected = 0;
        RvRtspConnectionHandle rv_connection_handle;
        RvStatus stat = RvRtspGetConnectionByURI(client->native_handle(), uri, &rv_connection_handle);
        if (RV_OK != stat)
        {
            return stat;
        }

        if (NULL != rv_connection_handle)
        {
            *connected = 1;
        }

        std::auto_ptr<rtsp_connection_info_t> sp(new (std::nothrow) rtsp_connection_info_t);
        stat = RvRtspConnectionConstruct(client->native_handle(), (RvRtspConnectionAppHandle)sp.get(), uri, local_ip, pConfiguration, sizeof(RvRtspConnectionConfiguration), &rv_connection_handle);
        if (RV_OK != stat)
        {
            return stat;
        }

        if (0 == *connected)
        {
            RvRtspConnectionCallbackFunctions callbacks = { 0 };

            callbacks.pfnOnConnectEv = &RvRtspConnectionOnConnectCB;
            callbacks.pfnOnDisconnectEv = &RvRtspConnectionOnDisconnectCB;
            callbacks.pfnOnErrorEv = &RvRtspConnectionOnErrorCB;
            callbacks.pfnOnDescribeResponseEv = &RvRtspConnectionOnDescribeResponseCB;
            callbacks.pfnOnRedirectRequestEv = &RvRtspConnectionOnRedirectRequestCB;
            callbacks.pfnOnErrorExtEv = &RvRtspConnectionOnErrorExtCB;
            callbacks.pfnOnReceiveEv = &RvRtspConnectionOnReceiveCB;
            callbacks.pfnOnSendEv = &RvRtspConnectionOnSendCB;
            callbacks.pfnOnRawBufferReceiveEv = &RvRtspConnectionOnRawBufferReceiveCB;

            stat = RvRtspConnectionRegisterCallbacks(rv_connection_handle, &callbacks, sizeof(RvRtspConnectionCallbackFunctions));
        }

        if (RV_OK == stat)
        {
            sp->init(rv_connection_handle, client);
            connection = sp.release();
        }

        return stat;
    }

    RvStatus rv_rtsp_client_adapter::connection_end(rtsp_connection_info_t *connection)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != connection);

        RvRtspHandle handle_ = connection->get_client()->native_handle();
        RvStatus stat = RvRtspConnectionDestruct(handle_, connection->native_handle(), RV_FALSE);
        delete connection;
        return stat;
    }

    RvStatus rv_rtsp_client_adapter::get_next_cseq(rtsp_connection_info_t *connection, RvUint16 *seq)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != connection);
        return RvRtspConnectionGetNextCSeq(connection->native_handle(), seq);
    }

    RvStatus rv_rtsp_client_adapter::connect(rtsp_connection_info_t *connection)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != connection);
        return RvRtspConnectionConnect(connection->native_handle());
    }

    RvStatus rv_rtsp_client_adapter::disconnect(rtsp_connection_info_t *connection)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != connection);
        return RvRtspConnectionDisconnect(connection->native_handle());
    }

    RvStatus rv_rtsp_client_adapter::describe(rtsp_connection_info_t *connection, RvUint16 seq, const char *uri)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != connection);
        return RvRtspConnectionRequestDescribeEx(connection->native_handle(), uri, seq, (RvRtspConnectionDescribeAppHandle)connection);
    }

    RvStatus rv_rtsp_client_adapter::get_addr(rtsp_connection_info_t *connection, char ip[RTSP_CLIENT_IP_LEN], uint16_t *port)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != session);

        RvStatus stat = RvRtspConnectionGetIPAddress(connection->native_handle(), RTSP_CLIENT_IP_LEN, ip);
        if (RV_OK != stat)
        {
            return stat;
        }

        RvUint16 rv_port = 0;
        stat = RvRtspConnectionGetPort(connection->native_handle(), &rv_port);
        if (RV_OK != stat)
        {
            return stat;
        }

        *port = rv_port;

        return RV_OK;
    }

    RvStatus rv_rtsp_client_adapter::session_init(rtsp_connection_info_t *connection, RvRtspSessionConfiguration *pConfiguration, rtsp_session_info_t *&session)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != connection);

        RvRtspSessionCallbackFunctions callbacks = { 0 };

        callbacks.pfnOnStateChangeEv = &RvRtspSessionOnStateChangeCB;
        callbacks.pfnOnErrorEv = &RvRtspSessionOnErrorCB;
        callbacks.pfnOnDestructEv = &RvRtspSessionOnDestructCB;
        callbacks.pfnOnErrorExtEv = &RvRtspSessionOnErrorExtCB;
        callbacks.pfnOnReceiveEv = &RvRtspSessionOnReceiveCB;
        callbacks.pfnOnSendEv = &RvRtspSessionOnSendCB;

        std::auto_ptr<rtsp_session_info_t> sp(new (std::nothrow) rtsp_session_info_t);
        RvRtspSessionHandle rv_session_handle;
        RvStatus stat = RvRtspSessionConstruct(connection->native_handle(), pConfiguration, sizeof(RvRtspSessionConfiguration), &callbacks, sizeof(RvRtspSessionCallbackFunctions), (RvRtspSessionAppHandle)sp.get(), &rv_session_handle);
        if (RV_OK == stat)
        {
            sp->init(rv_session_handle, connection);
            session = sp.release();
        }
        return stat;
    }

    RvStatus rv_rtsp_client_adapter::session_end(rtsp_session_info_t *session)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != session);
        if (NULL == session->native_handle())
        {
            return RV_ERROR_NULLPTR;
        }
        RvStatus stat =  RvRtspSessionDestruct(session->native_handle());
        delete session;
        return stat;
    }

    RvStatus rv_rtsp_client_adapter::setup(rtsp_session_info_t *session, RvUint16 seq, RvRtspTransportHeader *pTransportHeader)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != session);
        if (NULL == session->native_handle())
        {
            return RV_ERROR_NULLPTR;
        }
        return RvRtspSessionSetupEx(session->native_handle(), seq, pTransportHeader);
    }

    RvStatus rv_rtsp_client_adapter::set_uri(rtsp_session_info_t *session, const char *uri)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != session);
        if (NULL == session->native_handle())
        {
            return RV_ERROR_NULLPTR;
        }
        return RvRtspSessionSetUri(session->native_handle(), uri);
    }

    RvStatus rv_rtsp_client_adapter::play(rtsp_session_info_t *session, RvUint16 seq, RvRtspNptTime *pNptTime, float scale)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != session);
        if (NULL == session->native_handle())
        {
            return RV_ERROR_NULLPTR;
        }
        return RvRtspSessionPlayEx(session->native_handle(), seq, pNptTime, scale);
    }

    RvStatus rv_rtsp_client_adapter::pause(rtsp_session_info_t *session, RvUint16 seq)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != session);
        if (NULL == session->native_handle())
        {
            return RV_ERROR_NULLPTR;
        }
        return RvRtspSessionPauseEx(session->native_handle(), seq);
    }

    //modified by lichao 20151203 ÔöÇ¿½¡×³ÐÔ
    RvStatus rv_rtsp_client_adapter::teardown(rtsp_session_info_t *session, RvUint16 seq)
    {
        XT_RTSP_CLIENT_ASSERT(NULL != session);
        if (NULL == session->native_handle())
        {
            return RV_ERROR_NULLPTR;
        }
        return RvRtspSessionTeardownEx(session->native_handle(), seq);
    }
}
