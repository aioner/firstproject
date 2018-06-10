/**********************************************************************************
*                                                                                 *
* NOTICE:                                                                         *
* This document contains information that is confidential and proprietary to      *
* RADVision LTD.. No part of this publication may be reproduced in any form       *
* whatsoever without written prior approval by RADVision LTD..                    *
*                                                                                 *
* RADVision LTD. reserves the right to revise this publication and make changes   *
* without obligation to notify any person of such revisions or changes.           *
**********************************************************************************/


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILED                           */
/*-----------------------------------------------------------------------*/
//#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "rvadtstamp.h"
#include "RvRtspServerInc.h"
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/condition.hpp>

#if (RV_OS_TYPE != RV_OS_TYPE_WIN32)
#include <unistd.h>
#endif

#include "XTRtspServer.h"
#include "XTEngine.h"

#include <string>
#include "stdarg.h"
#include <vector>
#include "xt_log_def.h"

using namespace XT_RTSP;

extern xt_print_cb rtsp_svr_print_;
void RTSP_SVR_PRINT(const xt_log_level ll,
                    const char* format,
                    ...)
{
    if (rtsp_svr_print_)
    {
        char context[4096] = {0};

        va_list arg;
        va_start(arg, format);
        vsnprintf(context, sizeof(context)-1, format, arg);
        va_end(arg);

        rtsp_svr_print_("rtsp_svr", ll, context);
    }	
}

class managed_rv_sdp_mgr
{
public:
    managed_rv_sdp_mgr()
        :created_flag_(false)
    {}

    bool create()
    {
        scoped_lock _lock(mtx_);

        if (created_flag_)
        {
            return false;
        }

        ::RvSdpMgrConstruct();
        created_flag_ = true;

        cv_.notify_all();

        return true;
    }

    void destroy()
    {
        scoped_lock _lock(mtx_);
        if (created_flag_)
        {
            ::RvSdpMgrDestruct();
            created_flag_ = false;
        }
    }

    void wait()
    {
        scoped_lock _lock(mtx_);
        while (!created_flag_)
        {
            cv_.wait(_lock);
        }
    }

    bool created() const
    {
        scoped_lock _lock(mtx_);
        return created_flag_;
    }

private:
    bool created_flag_;
    mutable boost::mutex mtx_;
    typedef boost::unique_lock<boost::mutex> scoped_lock;
    boost::condition_variable cv_;
} static g_managed_rv_sdp_mgr;


// 会话起始索引
#define BASIC_INDEX_SESSION 0

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

/*-----------------------------------------------------------------------*/
/*                          MACRO DEFINITIONS                            */
/*-----------------------------------------------------------------------*/

#define SDP_FILE                            "sdp_info.txt"
#define SDP_BUFFER_SIZE                     2048
#define SERVER_OPTIONS                      "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY"

#define MAX_CONNECTIONS_ALLOWED 1024
#define MAX_SESSIONS_ALLOWED 1024 

#define IP_ADDR_LEN                         1024 
#define STATUS_LINE_HPHRASE_OK              "OK"
#define STATUS_LINE_HPHRASE_SESSION_NOT_FOUND "SESSION NOT FOUND"
#define STATUS_LINE_HPHRASE_NOT_IMPLEMENTED "NOT IMPLEMENTED"
#define RTP_PACKET_LEN                      1400
#define RTP_FILE                            "rtp_send_data"
/*-----------------------------------------------------------------------*/
/*                              GLOBALS                                  */
/*-----------------------------------------------------------------------*/
std::map<int, bool> g_mapSessionIds;
boost::shared_mutex gmutex;
int  get_free_sessionid()
{
    int ret = -1;

    boost::unique_lock<boost::shared_mutex> _lock(gmutex);
    for (int i=1;i<MAX_SESSIONS_ALLOWED;++i)
    {
        if (g_mapSessionIds.find(i) == g_mapSessionIds.end())
        {
            g_mapSessionIds[i] = true;
            return i;
        }
        else if (!g_mapSessionIds[i])
        {
            g_mapSessionIds[i] = true;
            return i;
        }
    }

    return ret;	
}
void  free_sessionid(int sid)
{
    boost::unique_lock<boost::shared_mutex> _lock(gmutex);
    if (sid <= MAX_SESSIONS_ALLOWED)
    {
        g_mapSessionIds[sid] = false;
    }
}

/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/

/* TestRtspServerStackStruct
* ----------
* This structure holds the data relevant for an RTSP stack.
*/
struct RtspServerStackStruct_t
{
    RvRtspHandle                             hRtsp;
    /* the RTSP stack */
    RvRtspServerConfiguration                rtspConfiguration;
    /* the stack's configuration */
    RvRtspServerConnectionHandle             hConnections[MAX_CONNECTIONS_ALLOWED];
    /* holds the stack's connections */
    RvRtspServerConnectionConfiguration      connectionServerConfiguration;
    /* the connections configuration */
    RvRtspServerConnectionCallbackFunctions  connectionCallbacks;
    /* the connections callbacks */
    RvRtspServerSessionHandle                hSessions[MAX_SESSIONS_ALLOWED];
    /* holds the stack's sessions */
    RvRtspServerSessionConfiguration         sessionServerConfiguration[MAX_SESSIONS_ALLOWED];
    /* the sessions configuration */
    RvRtspServerSessionCallbackFunctions     sessionServerCallbackFunctions;
    /* the sessions callbacks */
    RvRtspServerListenConfiguration          listenConfiguration;
    /* Configuration of listening ports */
    RvRtspListeningConnectionHandle          hListeningPort;
    /* Handle to the listening ports */

    RvRtspServerConnectionHandle hListeningConnection;
};

typedef struct RtspServerStackStruct_t RtspServerStackStruct;

typedef struct
{
    RvBool inUse;
    /* Boolean to indicate if structure is already in use */
    RvRtspStringHandle sessionId;
    /* The sessionID of the current session */
    RvRtspRtpInfo rtpInfo[2];
    /* The array to populate the array handle for the rtp-info
    header field when two SETUP requests are received (video) */
    RvUint numRtpInfo;
    /* Integer used as an index for the rtp-info header */
}RtspRtspInfo;


/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/
RtspServerStackStruct *pRouterRtspServer = NULL;
RtspServerStackStruct routerRtspServer;


/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/

/**************************************************************************
* TestServerConnectionOnReceive
* ------------------------------------------------------------------------
* General:
* This callback is called when the server receives a message on an
* existing connection. It then handles the message accordingly.
*
* Arguments:
* Input:  hConnection - Connection on which the message was received
*         hApp        - The Application context handle.
*         eMsgType    - Request/Response message type
*         pRequest    - Request Message
*         pResponse   - Response Message
* Output: None
*
* Return Value: RV_OK if successful.
*               Negative values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerConnectionOnReceiveEv(
    IN     RvRtspServerConnectionHandle           hConnection,
    IN     RvRtspServerConnectionAppHandle        hApp,
    IN     RvRtspMsgType                          eMsgType,
    IN     RvRtspRequest                          *pRequest,
    IN     RvRtspResponse                         *pResponse);

/**************************************************************************
* TestServerConnectionOnAcceptEv
* ------------------------------------------------------------------------
* General:
* This callback is called when the server receives a message from a
* client requesting for a connection. This function decides whether to
* accept or reject the connection request.
*
* Arguments:
* Input:  context     - The connection on which the message was
*                       received.
*         socket      - New Socket.
*         strClientIP - Client IP.
* Output: hConnection - handle of the constructed connection.
*         success     - Boolean to indicate acceptance/rejection of
*                       connection.
*
* Return Value:RV_OK - If the connection is accepted.
*              Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerConnectionOnAcceptEv(
    IN RvRtspServerConnectionHandle     hConnection,
    IN RvRtspServerConnectionAppHandle  hApp,
    IN RvSocket                         socket,
    IN RvChar                           *strClientIP,
    IN RvUint16                         portNum,
    INOUT RvBool                        *success);

/**************************************************************************
* TestServerConnectionOnDisconnectEv
* ------------------------------------------------------------------------
* General:
* This callback closes down the RTSP connection.
*
* Arguments:
* Input:  hConnection - Connection on which the message was received
*         hApp        - The Application context handle.
* Output: None.
*
* Return Value: RV_OK if successful.
*               Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerConnectionOnDisconnectEv(
    IN RvRtspServerConnectionHandle      hConnection,
    IN RvRtspServerConnectionAppHandle   hApp);

/**************************************************************************
* TestServerConnectionOnErrorEv
* ------------------------------------------------------------------------
* General:
* This callback is called when an error is encountered on the connection.
*
* Arguments:
* Input:  hConnection - Connection on which the message was received
*         hApp        - The Application context handle.
*         hURI        - The URI requested in the message that caused the error.
*         requestMethod - The requested method.
*         status - The response status.
*         hPhrase - The response phrase.
* Output: None
*
* Return Value: RV_OK if successful,
*               Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerConnectionOnErrorEv(
    IN      RvRtspServerConnectionHandle    hConnection,
    IN      RvRtspServerConnectionAppHandle hApp,
    IN      RvRtspStringHandle              hURI,
    IN      RvRtspMethod                    requestMethod,
    IN      RvRtspStatus                    status,
    IN      RvRtspStringHandle              hPhrase);

/**************************************************************************
* TestServerSessionOnReceiveEv
* ------------------------------------------------------------------------
* General:
* This callback is called when the server receives a message on an
* existing session. It then handles the message accordingly.
*
* Arguments:
* Input:  hSession    - Sessioon on which the message was received
*         hApp        - Application context.
*         pRequest    - Request to be processed
* Output: None
*
* Return Value: RV_OK if successful.
*               Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerSessionOnReceiveEv(
    IN     RvRtspServerSessionHandle             hSession,
    IN     RvRtspServerSessionAppHandle          hApp,
    IN     RvRtspRequest                         *pRequest);


/**************************************************************************
* TestServerSessionOnStateChangeEv
* ------------------------------------------------------------------------
* General:
* This callback is called when a session's state is changed when a new
* request is received on the session.
*
* Arguments:
* Input:  hSession    - Connection on which the message was received
*         hApp        - Application context.
*         currState   - currState of the session
*         newState    - newState of the session
* Output: None.
*
* Return Value: RV_OK if successful.
*               Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerSessionOnStateChangeEv(
    IN        RvRtspServerSessionHandle     hSession,
    IN        RvRtspServerSessionAppHandle  hApp,
    IN        RvRtspServerSessionState      currState,
    IN        RvRtspServerSessionState      newState);


/**************************************************************************
* TestServerSessionOnDestructEv
* ------------------------------------------------------------------------
* General:
* The event is called when a teardown request is received or the
* application wants to destruct the session.
*
* Arguments:
* Input:  hSession    - the session.
*         hApp        - Application context.
* Output: None.
*
* Return Value: None.
*************************************************************************/
RvStatus RVCALLCONV ServerSessionOnDestructEv(
    IN     RvRtspServerSessionHandle        hSession,
    IN     RvRtspServerSessionAppHandle  hApp);



/**************************************************************************
* ServerSessionOnErrorEv
* ------------------------------------------------------------------------
* General:
* Event callback function, called when an error response is received
* for the session.
*
* Arguments:
* Input:  hSession        - the session.
*         hApp            - Application context.
*         requestMethod   - the request for which the error occurred.
*         status          - the returned status.
*         hPhrase         - the status phrase.
* Output: None.
*
* Return Value: RV_OK if successful.
*               Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerSessionOnErrorEv(
    IN      RvRtspServerSessionHandle       hSession,
    IN      RvRtspServerSessionAppHandle    hApp,
    IN      RvRtspMethod                    requestMethod,
    IN      RvRtspStatus                    status,
    IN      RvRtspStringHandle              hPhrase);


//从uri中获取通道号 
static int get_channel_by_uri(const char *uri)
{
    if (NULL == uri)
    {
        return -1;
    }

    int begin = 0;
    int pos = 0;
    int sep = 0;

    while (0 != uri[pos])
    {
        if ('/' == uri[pos])
        {
            sep++;

            if (3 == sep)
            {
                begin = pos + 1;
            }
            else if (4 == sep)
            {
                break;
            }
        }
        pos++;
    }

    if (begin > 0)
    {
        int size =  pos - begin;
        if ((size >= 12) || (size <= 0))
        {
            return -1;
        }

        char num[16] = { 0 };
        (void)strncpy(num, uri + begin, size);
        int channel = atoi(num);

        return (channel < BASIC_INDEX_SESSION) ? -1 : channel - BASIC_INDEX_SESSION;
    }

    return -1;
}

static int get_trackid_by_uri(const std::string& uri)
{
    if (uri.empty())
    {
        return -1;
    }

    std::string::size_type pos = uri.find_last_not_of("0123456789");
    if (std::string::npos == pos)
    {
        return -1;
    }

    std::string trackid = uri.substr(pos + 1);
    return atoi(trackid.c_str());
}

int sdp_erase_control_full_uri(char *sdp, int len)
{
    if ((NULL == sdp) || (0 == len))
    {
        return -1;
    }

    g_managed_rv_sdp_mgr.wait();

    RvSdpParseStatus stat = RV_SDPPARSER_STOP_ZERO;
    RvSdpMsg *msg = ::rvSdpMsgConstructParse(NULL, sdp, &len, &stat);
    if (msg == NULL)
    {
        return -1;
    }

    RvSdpAttribute *attr = NULL;

    unsigned long global_attr_num = rvSdpMsgGetNumOfAttr(msg);

    for (unsigned long ii = 0; ii < global_attr_num; ++ii)
    {
        attr = rvSdpMsgGetAttribute(msg, ii);
        if ((NULL != attr) && (0 == strcmp("control", rvSdpAttributeGetName(attr))))
        {
            ::rvSdpMsgRemoveAttribute(msg, ii);
            break;
        }
    }

    bool replace_found = false;

    unsigned long media_attr_num = (unsigned long)rvSdpMsgGetNumOfMediaDescr(msg);
    for (unsigned long ii = 0; ii < media_attr_num; ++ii)
    {
        RvSdpMediaDescr *descr = rvSdpMsgGetMediaDescr(msg, ii);
        unsigned long descr_attr_num =  (unsigned long)rvSdpMediaDescrGetNumOfAttr2(descr);

        attr = NULL;
        for (unsigned long jj = 0; jj < descr_attr_num; ++jj)
        {
            attr = rvSdpMediaDescrGetAttribute2(descr, jj);
            if ((NULL != attr) && (0 == strcmp("control", rvSdpAttributeGetName(attr))))
            {
                break;
            }
        }

        if (NULL != attr)
        {
            const char *value = rvSdpAttributeGetValue(attr);
            if ((NULL != value) && ('r' == value[0]) && ('t' == value[1]) && ('s' == value[2]) && ('p' == value[3]))
            {
                //找到trackid部分，去掉前面的rtsp前缀
                const char *trackid_partion = strrchr(value, '/');
                if (NULL != trackid_partion)
                {
                    rvSdpAttributeSetValue(attr, trackid_partion + 1);
                    replace_found = true;
                }
            }
        }
    }

    if (!replace_found)
    {
        return -1;
    }

    RvSdpStatus stat2;
    ::rvSdpMsgEncodeToBuf(msg, sdp, len, &stat2);

    return strlen(sdp);
}

/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/


/**************************************************************************
* initServerTest
* ------------------------------------------------------------------------
* General:
* init module function
*
* Arguments:
* Input:   printCB - callback function for printing usage.
* Output:  None.
*
* Return Value:  None.
*************************************************************************/
void initRTSPServer(void)
{
    //added by lichao, 20141110 sdp和rtspserver一起初始化
    g_managed_rv_sdp_mgr.create();

    pRouterRtspServer = &routerRtspServer;
    memset(pRouterRtspServer, 0, sizeof(routerRtspServer));

    unsigned int max_session = XTEngine::instance()->get_max_session();
    if (max_session <= 16)
    {
        routerRtspServer.rtspConfiguration.maxListeningPorts           = 128;
        routerRtspServer.rtspConfiguration.maxConnections              = 128;
        routerRtspServer.rtspConfiguration.maxSessionsPerConnection    = 16;

        routerRtspServer.rtspConfiguration.memoryElementsNumber        = 256;
        routerRtspServer.rtspConfiguration.memoryElementsSize          = 1024;
        routerRtspServer.rtspConfiguration.maxRequests                 = 64;
        routerRtspServer.rtspConfiguration.maxResponses                = 64; 
    }
    else
    {
        routerRtspServer.rtspConfiguration.maxListeningPorts           = 4 * max_session;
        routerRtspServer.rtspConfiguration.maxConnections              = 4 * max_session;
        routerRtspServer.rtspConfiguration.maxSessionsPerConnection    = max_session;

        routerRtspServer.rtspConfiguration.memoryElementsNumber        = 16 * max_session;
        routerRtspServer.rtspConfiguration.memoryElementsSize          = 1024;
        routerRtspServer.rtspConfiguration.maxRequests                 = 16 * max_session;
        routerRtspServer.rtspConfiguration.maxResponses                = 16 * max_session;
    }
    routerRtspServer.rtspConfiguration.messageMask                 = RV_LOGLEVEL_NONE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    routerRtspServer.rtspConfiguration.maxRtspMsgHeadersInMessage  = 128;
#endif

    /* Init the globals */
    RvStatus status = 0;
    RvLogHandle log = NULL;

    /* initialize stack */
    status = RvRtspServerInit(log,
        &routerRtspServer.rtspConfiguration,
        sizeof(routerRtspServer.rtspConfiguration),
        &routerRtspServer.hRtsp);

    if (1)
    {
#ifdef _OS_WINDOWS
        RTSP_SVR_PRINT(level_info, "RtspServer init tid(%d),status(%d)!", GetCurrentThreadId(), status);
#else
        RTSP_SVR_PRINT(level_info, "RtspServer init tid(%d),status(%d)!", getpid(), status);
#endif
    }

    // 指定监听端口
    unsigned short iRtspPort = 1554;
    string ip = "0.0.0.0";
    XTEngine::instance()->get_listen_addr(ip, iRtspPort);

    routerRtspServer.listenConfiguration.listeningPort = iRtspPort;
    routerRtspServer.listenConfiguration.connConfiguration.transmitQueueSize   = 128;
    routerRtspServer.listenConfiguration.connConfiguration.maxHeadersInMessage = 128;

    /* Set the transport callbacks */
    RvUint        lPortIndex      = 0;
    //RvRtspServerConnectionHandle phConnection;
    status = RvRtspServerStartListening(pRouterRtspServer->hRtsp,
        (RvRtspServerConnectionAppHandle)lPortIndex,
        "0.0.0.0",
        &routerRtspServer.listenConfiguration,
        sizeof(routerRtspServer.listenConfiguration),
        &pRouterRtspServer->hListeningConnection);
    lPortIndex++;
    if (status != RV_OK)
    {
        RTSP_SVR_PRINT(level_info, "ServerMainLoop: Failed to listen on ip (%s) and port (%d)!", "0.0.0.0", iRtspPort);
        return;
    }

    /* Register connection callbacks */
    pRouterRtspServer->connectionCallbacks.pfnServerOnReceiveEv      = NULL;
    pRouterRtspServer->connectionCallbacks.pfnServerOnDisconnectEv   = NULL;
    pRouterRtspServer->connectionCallbacks.pfnServerOnErrorEv        = NULL;
    pRouterRtspServer->connectionCallbacks.pfnServerOnAcceptEv       = ServerConnectionOnAcceptEv;

    status = RvRtspServerConnectionRegisterCallbacks(pRouterRtspServer->hListeningConnection,
        &pRouterRtspServer->connectionCallbacks,
        sizeof(pRouterRtspServer->connectionCallbacks));
    if (status != RV_OK)
    {
        RTSP_SVR_PRINT(level_info, "%s!", "ServerMainLoop: Failed to register callbacks");
        return;
    }
}

//added by lichao 20141110 增加rtsp反初始化
void termRTSPServer(void)
{
    RvRtspServerStopListening(pRouterRtspServer->hListeningConnection);
    ::RvRtspServerEnd(routerRtspServer.hRtsp);
    routerRtspServer.hRtsp = NULL;
    g_managed_rv_sdp_mgr.destroy();
}

/**************************************************************************
* testMainLoop
* ------------------------------------------------------------------------
* General: main function, calls the testing functions.
*
* Arguments:
* Input:   timeOut - will wait on CCore select engine events for at least
*                    timeOut milliseconds
* Output:  None
*
* Return Value:  returns zero (0) when this is the last time to call the
*                mainLoop function, otherwise return a non zero value.
*************************************************************************/
int RTSPServerMainLoop(unsigned int timeOut)
{
    RvStatus  result;

    result = RvRtspServerMainLoop(routerRtspServer.hRtsp, timeOut);

    if (result == RV_OK)
    {
    }
    else
    {
        RTSP_SVR_PRINT(level_info, "ServerMainLoop: RvRtspServerMainLoop fail(%d)!", result);
        return 0;
    }

    return 1;

}


/**************************************************************************
* TestServerConnectionOnAcceptEv
* ------------------------------------------------------------------------
* General:
* This callback is called when the server receives a message from a
* client requesting for a connection. This function decides whether to
* accept or reject the connection request.
*
* Arguments:
* Input:   context     - The connection on which the message was
*                        received.
*          socket      - New Socket.
*          strClientIP - Client IP.
* Output:  hConnection - handle of the constructed connection.
*          success     - Boolean to indicate acceptance/rejection of
*                        connection.
*
* Return Value:  RV_OK - If the connection is accepted.
*                Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerConnectionOnAcceptEv(
    IN RvRtspServerConnectionHandle     hConnection,
    IN RvRtspServerConnectionAppHandle  hApp,
    IN RvSocket                         socket,
    IN RvChar                           *strClientIP,
    IN RvUint16                         portNum,
    INOUT RvBool                        *success)
{

    RvStatus                     result;
    RvRtspServerConnectionHandle hNewConnection = NULL;

    *success = RV_FALSE;
    result   = RV_OK;

    RV_UNUSED_ARG(hConnection);
    RV_UNUSED_ARG(hApp);

    pRouterRtspServer->connectionServerConfiguration.transmitQueueSize   = 128;
    pRouterRtspServer->connectionServerConfiguration.maxHeadersInMessage = 128;

    static unsigned sAppHandle = 1;
    RTSP_SVR_PRINT(level_info, "ServerConnectionOnAcceptEv ClientIP (%s), ClientPort (%d),socket(%d),connectionIndex(%d)!", strClientIP, portNum, socket, sAppHandle);

    /* Construct a connection                                                 */
    result = RvRtspServerConnectionConstruct(pRouterRtspServer->hRtsp,
        (RvRtspServerConnectionAppHandle)(sAppHandle++),
        socket,
        strClientIP,
        portNum,
        &pRouterRtspServer->connectionServerConfiguration,
        sizeof(pRouterRtspServer->connectionServerConfiguration),
        &hNewConnection);

    if( result != RV_OK)
    {
        RTSP_SVR_PRINT(level_error, "ServerTransportOnAcceptEv: Connection Construct failed(%d)!", result);
        return result;
    }

    /* Register connection callbacks */
    pRouterRtspServer->connectionCallbacks.pfnServerOnReceiveEv      = ServerConnectionOnReceiveEv;
    pRouterRtspServer->connectionCallbacks.pfnServerOnDisconnectEv   = ServerConnectionOnDisconnectEv;
    pRouterRtspServer->connectionCallbacks.pfnServerOnErrorEv        = ServerConnectionOnErrorEv;
    pRouterRtspServer->connectionCallbacks.pfnServerOnAcceptEv       = ServerConnectionOnAcceptEv;

    result = RvRtspServerConnectionRegisterCallbacks(hNewConnection,
        &pRouterRtspServer->connectionCallbacks,
        sizeof(pRouterRtspServer->connectionCallbacks));

    if( result != RV_OK)
    {
        RTSP_SVR_PRINT(level_error, "ServerTransportOnAcceptEv: Connection callbacks registration failed(%d)!", result);
        return result;
    }

    *success = RV_TRUE;

    string client_ip = strClientIP;
    rtsp_session::inst()->add_conn((void*)hNewConnection);
    rtsp_session::inst()->set_conn_addr((void*)hNewConnection, client_ip);

    return RV_OK;
}

/**************************************************************************
* TestServerConnectionOnReceive
* ------------------------------------------------------------------------
* General:
* This callback is called when the server receives a message on an
* existing connection. It then handles the message accordingly.
*
* Arguments:
* Input:   hConnection - Connection on which the message was received
*          hApp        - The Application context handle.
*          eMsgType    - Request/Response message type
*          pRequest    - Request Message
*          pResponse   - Response Message
* Output:  None
*
* Return Value:  RV_OK if successful.
*                Negative values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerConnectionOnReceiveEv(
    IN     RvRtspServerConnectionHandle           hConnection,
    IN     RvRtspServerConnectionAppHandle        hApp,
    IN     RvRtspMsgType                          eMsgType,
    IN     RvRtspRequest                          *pRequest,
    IN     RvRtspResponse                         *pResponse)
{

    RvRtspResponse             *response;
    RvRtspServerSessionHandle  hSession;
    //RvRtspStringHandle         phStr;
    RvChar                     strURI[100];
    RvChar                     strSessionId[32];
    RvRtspStringHandle         hSessionId;
    RvUint32                   strLen;

    /* Required for creating sdp using SDP stack */
    strLen  = 0;

    RV_UNUSED_ARG(hApp);
    RV_UNUSED_ARG(pResponse);

    if (eMsgType == RV_RTSP_MSG_TYPE_REQUEST)
    {
        RvRtspMessageConstructResponse(pRouterRtspServer->hRtsp, &response);

        response->statusLine.hPhrase     = NULL;
        response->cSeq.value             = pRequest->cSeq.value;
        response->cSeqValid              = RV_TRUE;

        /* reply with an 200 OK for SETUP or OPTIONS or SET_PARAMETER   */
        /*  or DESCRIBE and with NOT_IMPLEMENTED for others.        */
        if (pRequest->requestLine.method == RV_RTSP_METHOD_OPTIONS)
        {
            strLen = strlen(STATUS_LINE_HPHRASE_OK)+1;
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, STATUS_LINE_HPHRASE_OK, strLen, &response->statusLine.hPhrase);

            response->statusLine.status      = RV_RTSP_STATUS_OK;
            response->publicHdrValid            = RV_TRUE;

            strLen = strlen(SERVER_OPTIONS)+1;
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, SERVER_OPTIONS, strLen,
                &response->publicHdr.hStr);

            RvRtspServerConnectionSendResponse(hConnection, response);
            RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
        }
        else if (pRequest->requestLine.method == RV_RTSP_METHOD_DESCRIBE)
        {
            response->statusLine.status      = RV_RTSP_STATUS_OK;

            strLen = strlen(STATUS_LINE_HPHRASE_OK)+1;
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp,STATUS_LINE_HPHRASE_OK, strLen, &response->statusLine.hPhrase);

            response->contentLengthValid     = RV_TRUE;
            response->contentBaseValid       = RV_FALSE;	// LA 如果是可选，改成false好了
            response->contentTypeValid       = RV_TRUE;

            strLen = strlen("application/sdp")+1;
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp,"application/sdp", strLen, &response->contentType.hStr);

            // 截断字符串，获取会话号
            RvRtspStrcpy(pRouterRtspServer->hRtsp, pRequest->requestLine.hURI, 100, strURI);
            string strSession = strURI;

            // 先用形如rtsp://172.16.9.211/0 
            // 获得字符串最后一位的通道号
            long iSessionNo = get_channel_by_uri(strSession.c_str());
            if ((iSessionNo < 0) || (iSessionNo >= MAX_SESSIONS_ALLOWED))
            {
                RTSP_SVR_PRINT(level_error, "ServerConnectionOnReceiveEv - channel in uri error.channel(%d)!", iSessionNo);

                RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
                return -1;
            }

            char cSdp[4096];
            int iLen = 4096;

            // 获得SDP
            int rGetSdp = XTEngine::instance()->get_sdp(iSessionNo, cSdp, iLen);
            if ((rGetSdp != 0) || (iLen <= 0) || (iLen >= sizeof(cSdp)))
            {
                RTSP_SVR_PRINT(level_error, "ServerConnectionOnReceiveEv - get sdp fail.channel(%d)!", iSessionNo);

                RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
                return -1;
            }

            cSdp[iLen] = 0;

            RvRtspBlobHandleSetBlob(pRouterRtspServer->hRtsp, (const RvChar *)cSdp, &response->hBody);
            response->contentLength.value  = (RvUint16)strlen((const char *)cSdp);

            RvRtspServerConnectionSendResponse(hConnection, response);
            RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
        }
        else if (pRequest->requestLine.method == RV_RTSP_METHOD_SETUP)
        {
            //modified by lichao 20140624 需要对报文内容 进行 验证，
            //且目前支持udp传输协议，radivision底层clientPortA为0即非udp或非法协议
            if ((RV_FALSE == pRequest->transportValid) || (0 == pRequest->transport.clientPortA))
            {
                response->statusLine.status      = RV_RTSP_STATUS_NOT_IMPLEMENTED;
                strLen = strlen(STATUS_LINE_HPHRASE_NOT_IMPLEMENTED)+1;
                RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, STATUS_LINE_HPHRASE_NOT_IMPLEMENTED, strLen, &response->statusLine.hPhrase);

                RvRtspServerConnectionSendResponse(hConnection, response);
                RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
            }
            else
            {
                response->statusLine.status      = RV_RTSP_STATUS_OK;
                response->statusLine.hPhrase     = NULL;

                strLen = strlen(STATUS_LINE_HPHRASE_OK)+1;
                RvRtspStringHandleSetString(pRouterRtspServer->hRtsp,STATUS_LINE_HPHRASE_OK, strLen, &response->statusLine.hPhrase);

                /* Set the session callbacks                                      */
                pRouterRtspServer->sessionServerCallbackFunctions.pfnServerOnStateChangeEv  = ServerSessionOnStateChangeEv;
                pRouterRtspServer->sessionServerCallbackFunctions.pfnServerOnErrorEv        = ServerSessionOnErrorEv;
                pRouterRtspServer->sessionServerCallbackFunctions.pfnServerOnDestructEv     = ServerSessionOnDestructEv;
                pRouterRtspServer->sessionServerCallbackFunctions.pfnServerOnReceiveEv      = ServerSessionOnReceiveEv;

                /* Populate the sessionId */
                RvInt session_id = get_free_sessionid();
                if (session_id < 0)
                {
                    RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
                    return -1;
                }

                sprintf(strSessionId, "%d", session_id);
                strLen = strlen(strSessionId)+1;
                RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, strSessionId, strLen, &hSessionId);

                pRouterRtspServer->sessionServerConfiguration[session_id].strURI = pRequest->requestLine.hURI;

                /* Construct a session */
                 XTEngine::instance()->get_heartbit_time(pRouterRtspServer->sessionServerConfiguration[session_id].checkTimerInterval,
                 pRouterRtspServer->sessionServerConfiguration[session_id].timeOutInterval);
                RvRtspServerSessionConstruct(hConnection,
                    (RvRtspServerSessionAppHandle)session_id,
                    &pRouterRtspServer->sessionServerConfiguration[session_id],
                    sizeof(RvRtspServerSessionConfiguration),
                    &pRouterRtspServer->sessionServerCallbackFunctions,
                    sizeof(pRouterRtspServer->sessionServerCallbackFunctions),
                    &hSession,
                    &hSessionId); /* Send the sessionId to stack */

                /* If the hSessionId points to a NULL value, then
                the stack will generate a sessionId and return it's handle */


                RvRtspStrcpy(pRouterRtspServer->hRtsp, pRequest->requestLine.hURI, 100, strURI);

                RTSP_SVR_PRINT(level_info, "ServerConnectionOnReceiveEv: URI(%s) sid(%d)!", strURI, session_id);

                strLen = strlen(strURI)+1;
                std::string sURI = strURI;
                int iTrackNo = get_trackid_by_uri(sURI);
                if (iTrackNo < 0)
                {
                    RTSP_SVR_PRINT(level_error, "ServerConnectionOnReceiveEv - trackid error(%d)!", iTrackNo);

                    RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
                    return -1;
                }

                int iSessionNo = get_channel_by_uri(sURI.c_str());
                if ((iSessionNo < 0) || (iSessionNo >= MAX_SESSIONS_ALLOWED))
                {
                    RTSP_SVR_PRINT(level_error, "ServerConnectionOnReceiveEv - channel error(%d)!", iSessionNo);

                    RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
                    return -1;
                }

                string client_ip;
                if(pRequest->transportValid && strlen(pRequest->transport.destination)>0)
                {
                    client_ip.assign(pRequest->transport.destination);
                }
                else
                {
                    rtsp_session::inst()->get_conn_addr((void*)hConnection, client_ip);
                }

                // 复用
                //////////////////////////////////////////////////////////////////////////
                bool demux = false;
                unsigned int demuxid = 0;
                if (pRequest->transport.additionalFields)
                {
                    RvChar *app_header_fields[32];
                    for (int jj = 0; jj < 32; ++jj)
                    {
                        app_header_fields[jj] = (RvChar *)malloc(512);
                    }

                    RvRtspMsgAppHeader app_header;

                    (void)memset(&app_header, 0, sizeof(RvRtspMsgAppHeader));
                    app_header.headerName = (RvChar *)malloc(32);
                    app_header.headerNameLen = 32;
                    app_header.headerFieldsSize = 32;
                    app_header.headerFieldStrLen = 512;
                    app_header.headerFields = app_header_fields;

                    ::RvRtspMsgGetHeaderFieldValues(pRouterRtspServer->hRtsp, pRequest->transport.additionalFields, &app_header);

                    for (RvUint32 jj = 0; jj<32&&jj<app_header.headerFieldsSize; ++jj)
                    {
                        RvChar *field = app_header.headerFields[jj];//demuxid=xxx

                        string sdemuxid = field;
                        std::string sub = "demuxid=";
                        int offset = sdemuxid.find(sub);						
                        if (offset >= 0)
                        {
                            sdemuxid = sdemuxid.substr(offset+sub.length());
                            demux = true;
                            demuxid = atoi(sdemuxid.c_str());
                            break;
                        }
                    }

                    for (RvUint32 jj = 0; jj < 32; ++jj)
                    {
                        free(app_header_fields[jj]);
                    }

                    free(app_header.headerName);
                }
                //////////////////////////////////////////////////////////////////////////

                // 创建转发
                rtsp_session::inst()->add_session((void*)hConnection, (void*)hSession, session_id, iSessionNo);

                XTEngine::instance()->add_send((void*)hSession, iSessionNo, iTrackNo, client_ip.c_str(), pRequest->transport.clientPortA, demux, demuxid);

                //////////////////////////////////////////////////////////////////////////
                response->transportValid      = RV_TRUE;
                response->transport.isUnicast = pRequest->transport.isUnicast;
                response->sessionValid        = RV_TRUE;
                response->session.hSessionId  = hSessionId;

                strcpy( response->transport.destination, client_ip.c_str());

                response->transport.clientPortA = pRequest->transport.clientPortA;
                response->transport.clientPortB = pRequest->transport.clientPortB;

                // 服务端口
                unsigned short send_port = 0;
                demux = false;
                demuxid = 0;
                XTEngine::instance()->get_snd_port(iSessionNo, iTrackNo, send_port, demux, demuxid);
                response->transport.serverPortA = send_port;
                response->transport.serverPortB = send_port + 1;

                response->transport.additionalFields = NULL;
                if (demux)
                {
                    std::vector<std::string> fields;
                    char id[64] = "";
                    sprintf(id, "%d", demuxid);
                    std::string sid = "demuxid=";
                    sid += id;
                    fields.push_back(sid);

                    rv_msg_header_add_fields(routerRtspServer.hRtsp, NULL, response->transport.additionalFields, fields, RV_TRUE, ';');
                }

                RvRtspServerSessionSendResponse(hSession, response);
                RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
                //////////////////////////////////////////////////////////////////////////
            }
        }
        else if (pRequest->requestLine.method == RV_RTSP_METHOD_GET_PARAMETER)
        {
            response->statusLine.status      = RV_RTSP_STATUS_OK;

            strLen = strlen(STATUS_LINE_HPHRASE_OK)+1;
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, STATUS_LINE_HPHRASE_OK, strLen, &response->statusLine.hPhrase);

            RvRtspServerConnectionSendResponse(hConnection, response);
            RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
        }
        else if ((pRequest->requestLine.method == RV_RTSP_METHOD_PLAY) ||
            (pRequest->requestLine.method == RV_RTSP_METHOD_PAUSE) ||
            (pRequest->requestLine.method == RV_RTSP_METHOD_TEARDOWN))
        {
            //edit by zhouzx 2015/07/14
            if (pRequest->requestLine.method == RV_RTSP_METHOD_TEARDOWN)
            {
                // 删除转发
                XTEngine::instance()->del_send_connection((void*)hConnection);

                rtsp_session::inst()->del_conn((void*)hConnection);
            }

            response->statusLine.status      = RV_RTSP_STATUS_SESSION_NOT_FOUND;
            strLen = strlen(STATUS_LINE_HPHRASE_SESSION_NOT_FOUND)+1;
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, STATUS_LINE_HPHRASE_SESSION_NOT_FOUND, strLen, &response->statusLine.hPhrase);

            RvRtspServerConnectionSendResponse(hConnection, response);
            RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
        }
        else
        {
            response->statusLine.status      = RV_RTSP_STATUS_NOT_IMPLEMENTED;
            strLen = strlen(STATUS_LINE_HPHRASE_NOT_IMPLEMENTED)+1;
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, STATUS_LINE_HPHRASE_NOT_IMPLEMENTED, strLen, &response->statusLine.hPhrase);

            RvRtspServerConnectionSendResponse(hConnection, response);
            RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
        }

    }
    else if (eMsgType == RV_RTSP_MSG_TYPE_RESPONSE)
    {
        RTSP_SVR_PRINT(level_info, "%s", "ServerConnectionOnReceiveEv: Response Messages not handled!");
        return RV_ERROR_NOTSUPPORTED;
    }
    else
    {
        RTSP_SVR_PRINT(level_info, "%s", "ServerConnectionOnReceiveEv: Unknown Message type!");
        return RV_ERROR_BADPARAM;
    }
    return RV_OK;
}


/**************************************************************************
* ServerConnectionOnErrorEv
* ------------------------------------------------------------------------
* General:
* This callback is called when an error is encountered on the connection.
*
* Arguments:
* Input:   hConnection - Connection on which the message was received
*          hApp        - The Application context handle.
*          hURI        - The URI requested in the message that caused the error.
*          requestMethod - The requested method.
*          status - The response status.
*          hPhrase - The response phrase.
* Output:  None
*
* Return Value:  RV_OK if successful,
*                Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerConnectionOnErrorEv(
    IN      RvRtspServerConnectionHandle    hConnection,
    IN      RvRtspServerConnectionAppHandle hApp,
    IN      RvRtspStringHandle              hURI,
    IN      RvRtspMethod                    requestMethod,
    IN      RvRtspStatus                    status,
    IN      RvRtspStringHandle              hPhrase)
{

    RV_UNUSED_ARG(hURI);
    RV_UNUSED_ARG(requestMethod);
    RV_UNUSED_ARG(status);
    RV_UNUSED_ARG(hPhrase);
    RV_UNUSED_ARG(hApp);

    RTSP_SVR_PRINT(level_info, "ServerConnectionOnErrorEv requestMethod (%d), status (%d)!", requestMethod, status);

    // 删除转发
    XTEngine::instance()->del_send_connection((void*)hConnection);

    rtsp_session::inst()->del_conn((void*)hConnection);

    RvRtspServerConnectionDestruct(hConnection, RV_FALSE);

    return RV_OK;
}


/**************************************************************************
* TestServerConnectionOnDisconnectEv
* ------------------------------------------------------------------------
* General:
* This callback closes down the RTSP connection.
*
* Arguments:
* Input:   hConnection - Connection on which the message was received
*          hApp        - The Application context handle.
* Output:  None.
*
* Return Value:  RV_OK if successful.
*                Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerConnectionOnDisconnectEv(
    IN RvRtspServerConnectionHandle       hConnection,
    IN RvRtspServerConnectionAppHandle    hApp)
{
    RTSP_SVR_PRINT(level_info, "ServerConnectionOnDisconnectEv (%d)!", hApp);

    // 删除转发
    XTEngine::instance()->del_send_connection((void*)hConnection);

    rtsp_session::inst()->del_conn((void*)hConnection);

    RvRtspServerConnectionDestruct(hConnection, RV_FALSE);

    return RV_OK;
}


/**************************************************************************
* ServerSessionOnReceiveEv
* ------------------------------------------------------------------------
* General:
* This callback is called when the server receives a message on an
* existing session. It then handles the message accordingly.
*
* Arguments:
* Input:   hSession    - Session on which the message was received
*          hApp        - Application context.
*          pRequest    - Request to be processed
* Output:  None
*
* Return Value:  RV_OK if successful.
*                Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerSessionOnReceiveEv(
    IN     RvRtspServerSessionHandle             hSession,
    IN     RvRtspServerSessionAppHandle          hApp,
    IN     RvRtspRequest                         *pRequest)
{

    RvChar               strURI[100];
    RvRtspResponse       *response;
    RvInt strLen;

    RV_UNUSED_ARG(hApp);

    strLen = 0;

    RvRtspMessageConstructResponse( pRouterRtspServer->hRtsp, &response);

    response->cSeq.value          = pRequest->cSeq.value;
    response->cSeqValid           = RV_TRUE;
    response->statusLine.hPhrase  = NULL;
    response->sessionValid        = RV_TRUE;
    response->session.hSessionId  = pRequest->session.hSessionId;

    if (pRequest->requestLine.method == RV_RTSP_METHOD_SETUP)
    {
        response->statusLine.status       = RV_RTSP_STATUS_OK;

        strLen = strlen(STATUS_LINE_HPHRASE_OK)+1;
        RvRtspStringHandleSetString(pRouterRtspServer->hRtsp,STATUS_LINE_HPHRASE_OK, strLen, &response->statusLine.hPhrase);

        string client_ip;
        if(pRequest->transportValid && strlen(pRequest->transport.destination)>0)
        {
            client_ip.assign(pRequest->transport.destination);
        }
        else
        {
            rtsp_session::inst()->get_session_addr((void*)hSession, client_ip);
        }

        RvRtspStrcpy(pRouterRtspServer->hRtsp, pRequest->requestLine.hURI, 100, strURI);

        string sURI = strURI;
        int iTrackNo = get_trackid_by_uri(sURI);
        if (iTrackNo < 0)
        {
            RTSP_SVR_PRINT(level_error, "ServerSessionOnReceiveEv - channel iTrackNo(%d)!", iTrackNo);
            RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
            return -1;
        }

        // Session号为uri中的通道号
        int iSessionNo = get_channel_by_uri(sURI.c_str());
        if ((iSessionNo < 0) || (iSessionNo >= MAX_SESSIONS_ALLOWED))
        {
            RTSP_SVR_PRINT(level_error, "ServerSessionOnReceiveEv - channel error(%d)!", iSessionNo);

            RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
            return -1;
        }

        RTSP_SVR_PRINT(level_info, "ServerSessionOnReceiveEv: URI (%s)!", strURI);		

        strLen = strlen(strURI)+1;

        RvChar strSessionId[32];
        RvUint32 strLenOfSessionId = 32;
        RvRtspStringHandleGetString(pRouterRtspServer->hRtsp, pRequest->session.hSessionId, strSessionId, &strLenOfSessionId);

        // 复用
        //////////////////////////////////////////////////////////////////////////
        bool demux = false;
        unsigned int demuxid = 0;
        if (pRequest->transport.additionalFields)
        {
            RvChar *app_header_fields[32];
            for (int jj = 0; jj < 32; ++jj)
            {
                app_header_fields[jj] = (RvChar *)malloc(512);
            }

            RvRtspMsgAppHeader app_header;

            (void)memset(&app_header, 0, sizeof(RvRtspMsgAppHeader));
            app_header.headerName = (RvChar *)malloc(32);
            app_header.headerNameLen = 32;
            app_header.headerFieldsSize = 32;
            app_header.headerFieldStrLen = 512;
            app_header.headerFields = app_header_fields;

            ::RvRtspMsgGetHeaderFieldValues(pRouterRtspServer->hRtsp, pRequest->transport.additionalFields, &app_header);

            for (RvUint32 jj = 0; jj<32 && jj<app_header.headerFieldsSize; ++jj)
            {
                RvChar *field = app_header.headerFields[jj];//demuxid=xxx

                string sdemuxid = field;
                std::string sub = "demuxid=";
                int offset = sdemuxid.find(sub);						
                if (offset >= 0)
                {
                    sdemuxid = sdemuxid.substr(offset+sub.length());
                    demux = true;
                    demuxid = atoi(sdemuxid.c_str());
                    break;
                }
            }

            for (RvUint32 jj = 0; jj < 32; ++jj)
            {
                free(app_header_fields[jj]);
            }

            free(app_header.headerName);
        }
        //////////////////////////////////////////////////////////////////////////

        // 创建转发
        XTEngine::instance()->add_send((void*)hSession, iSessionNo, iTrackNo, client_ip.c_str(), pRequest->transport.clientPortA, demux, demuxid);

        //////////////////////////////////////////////////////////////////////////
        strcpy( response->transport.destination, client_ip.c_str());

        response->transportValid        = RV_TRUE;
        response->transport.isUnicast   = pRequest->transport.isUnicast;
        response->transport.clientPortA = pRequest->transport.clientPortA;
        response->transport.clientPortB = pRequest->transport.clientPortB;

        // 服务端口
        unsigned short send_port = 0;
        demux = false;
        demuxid = 0;
        XTEngine::instance()->get_snd_port(iSessionNo, iTrackNo, send_port, demux, demuxid);
        response->transport.serverPortA = send_port;
        response->transport.serverPortB = send_port + 1;

        response->transport.additionalFields = NULL;
        if (demux)
        {
            std::vector<std::string> fields;
            char id[64] = "";
            sprintf(id, "%d", demuxid);
            std::string sid = "demuxid=";
            sid += id;
            fields.push_back(sid);

            rv_msg_header_add_fields(routerRtspServer.hRtsp, NULL, response->transport.additionalFields, fields, RV_TRUE, ';');
        }
        //////////////////////////////////////////////////////////////////////////
    }
    else if (pRequest->requestLine.method == RV_RTSP_METHOD_PLAY)
    {
        double npt = .0;
        float scale = 0;
        if ((RV_TRUE == pRequest->rangeValid) && (RV_RTSP_NPT_FORMAT_SEC == pRequest->range.startTime.format))
        {
            npt = pRequest->range.startTime.seconds;
        }
        RvRtspStrcpy(pRouterRtspServer->hRtsp, pRequest->requestLine.hURI, 100, strURI);
        string sURI = strURI;
        int iSessionNo = get_channel_by_uri(sURI.c_str());
        if ((iSessionNo < 0) || (iSessionNo >= MAX_SESSIONS_ALLOWED))
        {
            RTSP_SVR_PRINT(level_error, "ServerSessionOnReceiveEv - channel error(%d)!", iSessionNo);

            RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
            return -1;
        }
        unsigned int rtp_pkt_seq = 0;
        unsigned int rtp_pkt_timestamp = 0;
        int ret = XTEngine::instance()->do_play((void *)hSession, iSessionNo, -1, npt, scale, &rtp_pkt_seq, &rtp_pkt_timestamp);
        if (0 == ret)
        {
            RvRtspRtpInfo       *pArrayElem;

            memset(&response->rtpInfo,0,sizeof(response->rtpInfo));

            response->statusLine.status      = RV_RTSP_STATUS_OK;
            response->statusLine.hPhrase = NULL;
            response->rtpInfoValid       = RV_TRUE;

            RvRtspArrayHandleInitArray(pRouterRtspServer->hRtsp, sizeof(RvRtspRtpInfo),1,"RvRtspRtpInfo", &response->rtpInfo.hInfo);
            RvRtspArrayHandleAddElement(response->rtpInfo.hInfo, (void **)&pArrayElem);

            pArrayElem->hURI         =  pRequest->requestLine.hURI;

            if (0 == rtp_pkt_seq)
            {
                pArrayElem->seqValid = RV_FALSE;
            }
            else
            {
                pArrayElem->seqValid     =  RV_TRUE;
                pArrayElem->seq = rtp_pkt_seq;
            }

            if (0 == rtp_pkt_timestamp)
            {
                pArrayElem->rtpTimeValid =  RV_FALSE;
            }
            else
            {
                pArrayElem->rtpTimeValid =  RV_TRUE;
                pArrayElem->rtpTime = rtp_pkt_timestamp;
            }

            response->statusLine.status = RV_RTSP_STATUS_OK;
            strLen = strlen(STATUS_LINE_HPHRASE_OK)+1;
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, STATUS_LINE_HPHRASE_OK, strLen, &response->statusLine.hPhrase);
        }
        else
        {
            response->statusLine.status  = RV_RTSP_STATUS_NOT_IMPLEMENTED;
            strLen = sizeof(STATUS_LINE_HPHRASE_NOT_IMPLEMENTED);
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, STATUS_LINE_HPHRASE_NOT_IMPLEMENTED, strLen, &response->statusLine.hPhrase);
        }
    }
    else if (pRequest->requestLine.method == RV_RTSP_METHOD_PAUSE)
    {
        RvRtspStrcpy(pRouterRtspServer->hRtsp, pRequest->requestLine.hURI, 100, strURI);
        string sURI = strURI;

        // Session号为uri中的通道号
        int iSessionNo = get_channel_by_uri(sURI.c_str());
        if ((iSessionNo < 0) || (iSessionNo >= MAX_SESSIONS_ALLOWED))
        {
            RTSP_SVR_PRINT(level_error, "ServerSessionOnReceiveEv - channel error(%d)!", iSessionNo);

            RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
            return -1;
        }

        int ret = XTEngine::instance()->do_pause((void *)hSession, iSessionNo, -1);

        if (0 == ret)
        {
            response->statusLine.status      = RV_RTSP_STATUS_OK;
            strLen = strlen(STATUS_LINE_HPHRASE_OK)+1;
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp,STATUS_LINE_HPHRASE_OK, strLen, &response->statusLine.hPhrase);
        }
        else
        {
            response->statusLine.status  = RV_RTSP_STATUS_NOT_IMPLEMENTED;
            strLen = sizeof(STATUS_LINE_HPHRASE_NOT_IMPLEMENTED);
            RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, STATUS_LINE_HPHRASE_NOT_IMPLEMENTED, strLen, &response->statusLine.hPhrase);
        }
    }
    else if (pRequest->requestLine.method == RV_RTSP_METHOD_TEARDOWN)
    {
        response->statusLine.status      = RV_RTSP_STATUS_OK;

        strLen = strlen(STATUS_LINE_HPHRASE_OK)+1;
        RvRtspStringHandleSetString(pRouterRtspServer->hRtsp,STATUS_LINE_HPHRASE_OK, strLen, &response->statusLine.hPhrase);

        RvChar strSessionId[32];
        RvUint32 strLenOfSessionId = 32;
        RvRtspStringHandleGetString(pRouterRtspServer->hRtsp, pRequest->session.hSessionId, strSessionId, &strLenOfSessionId);

        string sessionid = strSessionId;

        // TEARDOWN时执行DelSend
        RvRtspStrcpy(pRouterRtspServer->hRtsp, pRequest->requestLine.hURI, 100, strURI);

        string strSession = strURI;
        int iSessionNo = get_channel_by_uri(strSession.c_str());
        if ((iSessionNo < 0) || (iSessionNo >= MAX_SESSIONS_ALLOWED))
        {
            RTSP_SVR_PRINT(level_error, "ServerSessionOnReceiveEv - channel error(%d)!", iSessionNo);

            RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);
            return -1;
        }

        RTSP_SVR_PRINT(level_info, "ServerSessionOnReceiveEv:teardown uri(%s),channel(%d),sid(%s)!", strURI, iSessionNo, strSessionId);

        // 删除转发
        XTEngine::instance()->del_send_session((void*)hSession);
    }
    else if (RV_RTSP_METHOD_OPTIONS == pRequest->requestLine.method)
    {
        strLen = strlen(STATUS_LINE_HPHRASE_OK)+1;
        RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, STATUS_LINE_HPHRASE_OK, strLen, &response->statusLine.hPhrase);

        response->statusLine.status = RV_RTSP_STATUS_OK;
        response->publicHdrValid = RV_TRUE;

        strLen = strlen(SERVER_OPTIONS)+1;
        RvRtspStringHandleSetString(pRouterRtspServer->hRtsp, SERVER_OPTIONS, strLen, &response->publicHdr.hStr);
    }
    else
    {
        response->statusLine.status      = RV_RTSP_STATUS_NOT_IMPLEMENTED;
    }

    RvRtspServerSessionSendResponse(hSession, response);
    RvRtspMessageDestructResponse(pRouterRtspServer->hRtsp, response);

    return RV_OK;
}




/**************************************************************************
* ServerSessionOnStateChangeEv
* ------------------------------------------------------------------------
* General:
* This callback is called when a session's state is changed when a new
* request is received on the session.
*
* Arguments:
* Input:   hSession    - Connection on which the message was received
*          hApp        - Application context.
*          currState   - currState of the session
*          newState    - newState of the session
* Output:  None.
*
* Return Value:  RV_OK if successful.
*                Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerSessionOnStateChangeEv(
    IN        RvRtspServerSessionHandle           hSession,
    IN        RvRtspServerSessionAppHandle        hApp,
    IN        RvRtspServerSessionState            currState,
    IN        RvRtspServerSessionState            newState)
{
    RV_UNUSED_ARG(hApp);

    switch(newState)
    {
    case RV_RTSP_SESSION_STATE_INIT:
        RvRtspServerSessionDestruct(hSession);
        break;
    case RV_RTSP_SESSION_STATE_READY:
        {
            switch(currState)
            {
            case RV_RTSP_SESSION_STATE_PLAYING:
                break;
            case RV_RTSP_SESSION_STATE_READY:
                break;
            case RV_RTSP_SESSION_STATE_INIT:
                break;
            default:
                break;
            }
            break;
        }
    case RV_RTSP_SESSION_STATE_PLAYING:
        if(currState == RV_RTSP_SESSION_STATE_READY)
        {
        }
        break;
    default:
        break;
    }
    return RV_OK;
}


/**************************************************************************
* ServerSessionOnErrorEv
* ------------------------------------------------------------------------
* General:
* Event callback function, called when an error response is received
* for the session.
*
* Arguments:
* Input:   hSession        - the session.
*          hApp            - Application context.
*          requestMethod   - the request for which the error occurred.
*          status          - the returned status.
*          hPhrase         - the status phrase.
* Output:  None.
*
* Return Value:  RV_OK if successful.
*                Negative Values otherwise.
*************************************************************************/
RvStatus RVCALLCONV ServerSessionOnErrorEv(
    IN      RvRtspServerSessionHandle       hSession,
    IN      RvRtspServerSessionAppHandle    hApp,
    IN      RvRtspMethod                    requestMethod,
    IN      RvRtspStatus                    status,
    IN      RvRtspStringHandle              hPhrase)
{

    RV_UNUSED_ARG(requestMethod);
    RV_UNUSED_ARG(status);
    RV_UNUSED_ARG(hPhrase);
    RV_UNUSED_ARG(hApp);

    RTSP_SVR_PRINT(level_info, "ServerSessionOnErrorEv: requestMethod (%d), status (%d)!", requestMethod, status);


    RvRtspServerSessionDestruct(hSession);

    return RV_OK;
}


/**************************************************************************
* ServerSessionOnDestructEv
* ------------------------------------------------------------------------
* General:
* The event is called when a teardown request is received or the
* application wants to destruct the session.
*
* Arguments:
* Input:   hSession    - the session.
*          hApp        - Application context.
* Output:  None.
*
* Return Value:  None.
*************************************************************************/
RvStatus RVCALLCONV ServerSessionOnDestructEv(
    IN     RvRtspServerSessionHandle        hSession,
    IN     RvRtspServerSessionAppHandle     hApp)
{
    RV_UNUSED_ARG(hApp);

    RvRtspServerSessionDestruct(hSession);

    return RV_OK;
}
