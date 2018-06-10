/*
 ********************************************************************************
 *                                                                              *
 * NOTICE:                                                                      *
 * This document contains information that is confidential and proprietary to   *
 * RADVision LTD.. No part of this publication may be reproduced in any form    *
 * whatsoever without written prior approval by RADVision LTD..                 *
 *                                                                              *
 * RADVision LTD. reserves the right to revise this publication and make changes*
 * without obligation to notify any person of such revisions or changes.        *
 ********************************************************************************
*/

/********************************************************************************
 *                               <TRTSP_testapp.c>
 *
 * This file include functions that initiate the stack and the tcl.
 *
 *******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "TRTSP_init.h"
#include "TRTSP_general.h"
#include "TRTSP_testapp.h"
#include "TRTSP_hash.h"

#include "RvRtspClientInc.h"
#include "rvloglistener.h"
#include "rvcbase.h"

#include<stdio.h>

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#include <process.h>
#else
#include<pthread.h>
#include<unistd.h>
#endif


/*-----------------------------------------------------------------------*/
/*                          MACRO DEFINITIONS                            */
/*-----------------------------------------------------------------------*/

#define TEST_MAX_CONNECTIONS_ALLOWED        10
#define TEST_MAX_SESSIONS_PER_CONN          20
#define TEST_MAX_TRACKS_PER_SESSION         2

#define TEST_MAX_TRACK_SIZE                 256
#define TEST_MAX_HEADERS_ALLOWED            20
#define TEST_HEADER_NAME_SIZE               20
#define TEST_MAX_HEADER_FIELDS_ALLOWED      10
#define TEST_MAX_HEADER_FIELD_VALUE_SIZE    200
#define FIELDVAL_LEN                        30
#define PRINT_BUFFER_LENGTH                 6000
#define LOG_MESSAGE_BUFFER_LENGTH           500

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#define STRCASECMP(X,Y)  stricmp(X,Y)
#else
#define STRCASECMP(X,Y)  strcasecmp(X,Y)
#endif

/*-----------------------------------------------------------------------*/
/*                              GLOBALS                                  */
/*-----------------------------------------------------------------------*/
RvChar *method[20] = {
        "DESCRIBE",
        "ANNOUNCE",
        "GET_PARAMETER",
        "OPTIONS",
        "PAUSE",
        "PLAY",
        "RECORD",
        "REDIRECT",
        "SETUP",
        "SET_PARAMETER",
        "TEARDOWN",
        "MAX",
        "UNKNOWN"
}; /* Array of method names */

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)

static RvChar * levels[] = { "exp", "err", "wrn", "inf", "dbg", "ent", "lve", "syn", "" };
static RvChar * sources[] = { "ALLOC", "APP", "ARES", "CLOCK", "EHD", "EMA", "EPP",
    "HOST", "LDAP", "LOCK", "RTSP", "MEMORY", "MUTEX", "PORT", "QUEUE", "RA",
    "RCACHE", "RTSP", "SDP", "SCTP", "SELECT", "SEMA4", "SDP", "SNMP", "SOCKET",
    "THREAD", "TIMER", "TIMERMGR", "TIMESTAMP", "TLS", "TM", "" };

#endif

/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/


/* GenericMsgRec
 * ----------
 * This structure holds the details of the parsed additional headers
 * Input: from the GUI .
 */
typedef struct
{
    char method[10];
        /* Method name */
    char header[20];
        /* Header to add to */
    char fieldvalue1[FIELDVAL_LEN];
        /* Field value 1*/
    char fieldvalue2[FIELDVAL_LEN];
        /* Field name2 */
    char fieldvalue3[FIELDVAL_LEN];
        /* Field name3 */
    char delim;
        /* Delimiter */
    int numHeadFields;
}GenericMsgRec;



/* TestRtspStackStruct
 * ----------
 * This structure holds the data relevant for an RTSP stack.
 */
struct TestRtspStackStruct_t
{
    RvRtspHandle                        hRtsp;
        /* the RTSP stack               */
    RvRtspConfiguration                 rtspConfiguration;
        /* the stack's configuration    */
    RvRtspConnectionConfiguration       connectionConfiguration;
        /* the connections configuration*/
    RvRtspConnectionCallbackFunctions   connectionCallbacks;
        /* the connections callbacks    */
    RvRtspSessionConfiguration          sessionConfiguration;
        /* the sessions configuration   */
    RvRtspSessionCallbackFunctions      sessionCallbackFunctions;
        /* the sessions callbacks       */
    RvBool                              isAutoAnswer;
        /* Is auto Answer enable */
    GenericMsgRec                       genericMsgRec[TEST_MAX_HEADERS_ALLOWED];
        /* Generic messages */
    RvUint                              numRecords;
        /* Number of Generic Message Records */
    RvUint                              rtspMethod;

    AppHashObj *                        connectionHash;
    AppHashObj *                        sessionHash;

    RvLogMgr                            logMgr;
    RvLogListener                       logFile;
    RvBool                              bCatchStackLog;

};

typedef struct TestRtspSessionStruct_t TestRtspSessionStruct;

/* TestRtspConnectionStruct
 * --------------
 * This structure holds the details of the connection
 */
typedef struct TestRtspConnectionStruct_t
{
    TestRtspStackStruct *               pRtspApp;
        /* the RTSP application object  */
    RvUint32                            index;
        /* Resource/Hash index */
    RvRtspConnectionHandle              hConnection;
        /* RTSP stack's connection handle */
    TestRtspSessionStruct *             sessions[TEST_MAX_SESSIONS_PER_CONN];
       /* sessions related to the connection */
    RvUint                              sessCount;
       /* number of sessions in the connection */
} TestRtspConnectionStruct;


/* TestRtspSessionStruct
 * ------------
 * This structure holds the details of the session
 */
struct TestRtspSessionStruct_t
{
    TestRtspStackStruct *               pRtspApp;
        /* the RTSP application object */
    RvUint32                            index;
        /* Resource/Hash index */
    RvRtspSessionHandle                 hSession;
        /* RTSP stack's sessions handle */
    TestRtspConnectionStruct *          pConnection;
        /* the connecton this session is using */
    RvChar strURI[100];
        /* URI of the session */
    RvChar trackArray[TEST_MAX_TRACKS_PER_SESSION][TEST_MAX_TRACK_SIZE];
        /* Tracks of the session */
    RvChar state[10];
        /* state of the session */
};

/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/

TestRtspStackStruct   *pTest          = NULL;
TestRtspStackStruct   Test;
RvBool                loop            = RV_TRUE;

/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * TestConnectionOnConnectEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when the connection is established
 * (or connection failed).
 *
 * Arguments:
 * Input::   hConnection    - the connection.
 *          hApp        - application context.
 *          success     - has the connection been established.
 * Output::  None.
 *
 * Return Value:  RV_OK if successfull, negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnConnectEv(
    IN RvRtspConnectionHandle       hConnection,
    IN RvRtspConnectionAppHandle    hApp,
    IN RvBool                       success);


/**************************************************************************
 * TestConnectionOnDisconnectEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when the connection is
 * disconnected.
 *
 * Arguments:
 * Input::   hConnection    - the connection.
 *          hApp        - application context.
 * Output::  None.
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnDisconnectEv(
    IN RvRtspConnectionHandle       hConnection,
    IN RvRtspConnectionAppHandle    hApp);


/**************************************************************************
 * TestConnectionOnErrorEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when an error message is
 * received on the connection.
 *
 * Arguments:
 * Input::   hConnection     - the connection.
 *          hApp            - application context.
 *          hURI            - the URI on which the request was made.
 *          requestMethod   - the request method.
 *          status          - the response status.
 *          hPhrase         - the response phrase.
 * Output::  None.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnErrorEv(
    IN    RvRtspConnectionHandle        hConnection,
    IN    RvRtspConnectionAppHandle     hApp,
    IN    RvRtspStringHandle            hURI,
    IN    RvRtspMethod                  requestMethod,
    IN    RvRtspStatus                  status,
    IN    RvRtspStringHandle            hPhrase);

/**************************************************************************
 * TestConnectionOnErrorExtEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when an error message is
 *            received on the connection.
 *          The same as TestConnectionOnErrorEv except that raise the describe
 *          application handle and the received response to the application.
 *
 * Arguments:
 * Input::    hConnection   - The Connection on which the error occurred.
 *          hApp          - application context.
 *          hDescribe     - The application handle for the failed describe request
 *                          or NULL if the error is not on describe request.
 *          hURI          - The URI requested in the message that
 *                          caused the error.
 *          requestMethod - The requested method.
 *          status        - The response status.
 *          phrase        - The response phrase.
 *          pResponse     - the received response.
 * Output::    None.
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnErrorExtEv(
    IN    RvRtspConnectionHandle                hConnection,
    IN    RvRtspConnectionAppHandle             hApp,
    IN    RvRtspConnectionDescribeAppHandle     hDescribe,
    IN    RvRtspStringHandle                    hURI,
    IN    RvRtspMethod                          requestMethod,
    IN    RvRtspStatus                          status,
    IN    RvRtspStringHandle                    hPhrase,
    IN    const RvRtspResponse                * pResponse);

/**************************************************************************
 * TestConnectionOnDescribeResponseEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when a Describe response is
 * received on the connection.
 *
 * Arguments:
 * Input::   hConnection        - the connection.
 *          hApp               - application context.
 *          pDescribeResponse  - the Describe response.
 *          hURI               - the URI on which the request was made.
 * Output::  None.
 *
 * Return Value:  RV_OK if successfull, negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnDescribeResponseEv(
    IN    RvRtspConnectionHandle               hConnection,
    IN    RvRtspConnectionAppHandle            hApp,
    IN    RvRtspConnectionDescribeAppHandle    hDescribe,
    IN    const RvRtspResponse                *pDescribeResponse,
    IN    RvRtspStringHandle                   hURI);


/**************************************************************************
 * TestConnectionOnRedirectRequestEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when a Redirect request is
 *            received on the connection.
 *
 *
 * ------------------------------------------------------------------------
 * Arguments:
 * Input::  hConnection  - the connection.
 *          hApp         - application context.
 *          pRequest     - the Redirect request.
 * Output::  None.
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnRedirectRequestEv(
    IN    RvRtspConnectionHandle        hConnection,
    IN    RvRtspConnectionAppHandle     hApp,
    IN    const RvRtspRequest          *pRequest);



/**************************************************************************
 * TestSessionOnStateChangeEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when the session's state is
 * changed (due to a response message received for the session).
 *
 * Input::   hSession    - the session.
 *          hApp        - application context.
 *          currState   - the current session state.
 *          newState    - the new session state.
 *          pResponse   - the received response.
 * Output::  None.
 *
 * Return Value: RV_OK if successfull, negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestSessionOnStateChangeEv(
                        IN    RvRtspSessionHandle        hSession,
                        IN    RvRtspSessionAppHandle    hApp,
                        IN    RvRtspSessionState        currState,
                        IN    RvRtspSessionState        newState,
                        IN    const RvRtspResponse    *pResponse);



/**************************************************************************
 * TestSessionOnErrorEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when an error response is
 * received for the session.
 *
 * Input::   hSession       - the session.
 *          hApp           - application context.
 *          requestMethod  - the request for which the error occurred.
 *          status         - the returned status.
 *          hPhrase        - the status phrase.
 * Output::  None
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestSessionOnErrorEv(
                        IN    RvRtspSessionHandle        hSession,
                        IN    RvRtspSessionAppHandle    hApp,
                        IN    RvRtspMethod            requestMethod,
                        IN    RvRtspStatus            status,
                        IN    RvRtspStringHandle        hPhrase);


/**************************************************************************
 * TestRtspConnectionsDisconnect
 * ------------------------------------------------------------------------
 * General:
 * Internal function, used to disconnect the connection objects from
 * the server.
 *
 * Arguments:
 * Input::   pTest    - structure holding the stack related variables.
 * Output::  None.
 *
 * Return Value: None
 *************************************************************************/
void    TestRtspConnectionsDisconnect(
                    IN TestRtspStackStruct    *pTest);


/**************************************************************************
 * TestRtspSessionConstruct
 * ------------------------------------------------------------------------
 * General:
 * Constructs the stack's sessions and sets the URIs according to
 * the streamURI array.
 *
 * Input::   pTest    - structure holding the stack related variables.
 * Output::  None.
 *
 * Return Value:  session application handle.
 **************************************************************************/
RvRtspSessionAppHandle TestRtspSessionConstruct(
    IN int iConnection);


/**************************************************************************
 * TestRtspSessionDestruct
 * ------------------------------------------------------------------------
 * General:
 * Tears down and destructs all the stack's sessions.
 *
 * Input::   pSession    - structure holding the session related variables.
 * Output::  None
 *
 * Return Value:  None
 *************************************************************************/
void TestRtspSessionDestruct(
    IN TestRtspSessionStruct * pSession);

/**************************************************************************
 * TestHandleReceivedMessagee
 * ------------------------------------------------------------------------
 * General:
 * This function updates the GUI variables and prints the received messages
 * including the additional headers with the message on the GUI message
 * window.
 * Arguments:
 * Input:  hMessage   - Message type.
 *         bIsRequest - determines whether message received is request or
 *                      response.
 * Inout:  pStr       - string to be displayed on the message window.
 *
 * Return Value: None
 *************************************************************************/
void TestHandleReceivedMessage(
                            IN     RvRtspMsgMessageHandle  hMessage,
                            IN     RvBool                  bIsRequest,
                            INOUT  RvChar                  *pStr);

/**************************************************************************
 * TestPrintSentMessage
 * ------------------------------------------------------------------------
 * General:
 * This function and prints the sent message on the GUI message window.
 *
 * Arguments:
 * Input:  eMsgType   - Message type.
 *         pRequest   - request received.
 *         pResponse  - response received.
 * Output: pStr       - string to be displayed onthe message window.
 *************************************************************************/
void TestPrintSentMessage(
                          IN RvBool         bIsRequest,
                          IN RvRtspRequest  *pRequest,
                          IN RvRtspResponse *pResponse,
                          OUT RvChar        *pStr );

/******************************************************************************
 * TestParseAndStoreAdditionalHeader
 * ----------------------------------------------------------------------------
 * General:
 *  Parses and adds the additional header record from the GUI into the message
 *
 * Arguments:
 * Input:  strHeader  - records of additional headers from the GUI.
 *
 * Output: None
 *
 * Return Value: RV_OK if successful.
 *               Negative value otherwise.
 ******************************************************************************/
RvStatus TestParseAndStoreAdditionalHeader(
                              IN    RvChar           *strHeader);

/******************************************************************************
 * TestAddAdditionalHeader
 * ----------------------------------------------------------------------------
 * General:
 *  Adds the Generic messages to the outgoing message after they are
 *  populated in the structure.
 *
 * Arguments:
 * Input:  bIsRequest - Boolean indicating whether message is a Request or a
 *                      Response.
 *         pResponse  - response to be updated with the Generic Messages.
 *         pRequest   - request to be updated with the Generic Messages.
 * Output: None
 *
 * Return Value: RV_OK if successful.
 *               Negative value otherwise.
 ******************************************************************************/
RvStatus TestAddAdditionalHeader(
                         IN      RvBool         bIsRequest,
                         IN      RvRtspResponse *pResponse,
                         IN      RvRtspRequest  *pRequest);

/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/


/******************************************************************************
 * AppLogListenerCB
 * ----------------------------------------------------------------------------
 * General: Called to print the log.
 *
 * Return Value: none.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  logRecord        - Log record to print.
 *         userData         - Context - Diameter application object.
 * Output: stateParameters  - List of termination properties.
 *****************************************************************************/
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
static void RVCALLCONV AppLogListenerCB(
    IN RvLogRecord *    logRecord,
    IN void *           userData)
{
    TestRtspStackStruct * pTest = (TestRtspStackStruct *)userData;

    /* if this is an error, print to the application */
    if ((RvLogRecordGetMessageType(logRecord) == RV_LOGID_EXCEP) ||
        (RvLogRecordGetMessageType(logRecord) == RV_LOGID_ERROR) ||
        (RvLogRecordGetMessageType(logRecord) == RV_LOGID_WARNING))
    {
        TclExecute("test:Log {>> %s}", RvLogRecordGetText(logRecord));
    }
    /* check if we want to catch all logs. */
    else if (pTest->bCatchStackLog)
    {
        char logline[256];
        const char * text = RvLogRecordGetText(logRecord);
        int i, index=0;

        for (i=0; index<256; i++)
        {
            if (text[i] == '\t')
            {
                logline[index++] = ' ';
                logline[index++] = ' ';
                continue;
            }
            if ((text[i] == '\n') || (text[i] == '\0') ||
                (index == 255))
            {
                logline[index] = '\0';
                TclExecute("test:Log {%s}", logline);
                if (text[i] == '\0')
                    break;
                if (index == 255)
                {
                    index = 0;
                    logline[index++] = '\t';
                }
                else
                {
                    index = 0;
                }
                continue;
            }
            logline[index++] = text[i];
        }
    }
}
#endif /* #if (RV_LOGMASK != RV_LOGLEVEL_NONE) */


/**************************************************************************
 * TestConnectionOnConnectEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when the connection is
 * established (or connection failed).
 *
 * Input::   hConnection   - the connection.
 *          hApp          - application context.
 *          success       - has the connection been established.
 * Output::  None
 *
 * Return Value:  RV_OK if successful
 *
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnConnectEv(
    IN RvRtspConnectionHandle        hConnection,
    IN RvRtspConnectionAppHandle     hApp,
    IN RvBool                        success)
{
    TestRtspConnectionStruct * pConnection = (TestRtspConnectionStruct *)hApp;

    if (success == RV_TRUE)
    {
       TclExecute("test:Log {Connected to Server};update");
       TclExecute("test:ConnsListAdd {%d} {%p} {%s}", pConnection->index, pConnection, "CONNECTED");
    }
    else
    {
        TclExecute("test:Log {Failed to Connect to Server};update");
        RvRtspConnectionDestruct(pTest->hRtsp, hConnection, RV_FALSE);
    }

    RV_UNUSED_ARG(hApp);
    return RV_OK;
}



/**************************************************************************
 * TestConnectionOnDisconnectEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when the connection is
 *            disconnected.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * Input:    :    hConnection    - the connection.
 *                hApp        - application context.
 * Output:    :    None.
 * INOUT    :    None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnDisconnectEv(
    IN RvRtspConnectionHandle        hConnection,
    IN RvRtspConnectionAppHandle    hApp)
{
    TestRtspConnectionStruct * pConnection = (TestRtspConnectionStruct *)hApp;
    RvUint16 i;

    RvRtspConnectionDestruct(pTest->hRtsp, pConnection->hConnection, RV_FALSE);

    for( i = 0; i < TEST_MAX_SESSIONS_PER_CONN; i++)
    {
        if (pConnection->sessions[i] != NULL)
        {
            AppHashRemove(pConnection->pRtspApp->sessionHash, pConnection->sessions[i]->index);
            pConnection->sessions[i] = NULL;
        }
    }

    TclExecute("test:ConnsListDel {%d}", pConnection->index);
    TclExecute("test:Log {Disconnected from Server};update");

    AppHashRemove(pConnection->pRtspApp->connectionHash, pConnection->index);

    RV_UNUSED_ARG (hConnection);
    return RV_OK;
}



/**************************************************************************
 * TestConnectionOnErrorEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when an error message is
 * received on the connection.
 *
 * Input::   hConnection        - the connection.
 *          hApp            - application context.
 *          hURI            - the URI on which the request was made.
 *          requestMethod    - the request method.
 *          status            - the response status.
 *          hPhrase            - the response phrase.
 * Output::  None
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnErrorEv(
    IN    RvRtspConnectionHandle        hConnection,
    IN    RvRtspConnectionAppHandle     hApp,
    IN    RvRtspStringHandle            hURI,
    IN    RvRtspMethod                  requestMethod,
    IN    RvRtspStatus                  status,
    IN    RvRtspStringHandle            hPhrase)
{

    RvChar      errorStr[100];

    sprintf(errorStr,"test:Log {Error on Connection:STATUS = %d};update", status);
    TclExecute(errorStr);

    RV_UNUSED_ARG(hConnection);
    RV_UNUSED_ARG(requestMethod);
    RV_UNUSED_ARG(hPhrase);
    RV_UNUSED_ARG(hURI);
    RV_UNUSED_ARG(hApp);

    return RV_OK;
}


/**************************************************************************
 * TestConnectionOnErrorExtEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when an error message is
 *            received on the connection.
 *          The same as TestConnectionOnErrorEv except that raise the describe
 *          application handle and the received response to the application.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * Input:    hConnection     - The Connection on which the error occurred.
 *           hApp            - application context.
 *           hDescribe       - The application handle for the failed describe request
 *                             or NULL if the error is not on describe request.
 *           hURI            - The URI requested in the message that
 *                             caused the error.
 *           requestMethod    - The requested method.
 *           status            - The response status.
 *           phrase            - The response phrase.
 *           pResponse        - the received response.
 * Output::  None
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnErrorExtEv(
    IN    RvRtspConnectionHandle              hConnection,
    IN    RvRtspConnectionAppHandle           hApp,
    IN    RvRtspConnectionDescribeAppHandle   hDescribe,
    IN    RvRtspStringHandle                  hURI,
    IN    RvRtspMethod                        requestMethod,
    IN    RvRtspStatus                        status,
    IN    RvRtspStringHandle                  hPhrase,
    IN    const RvRtspResponse               *pResponse)
{
    RV_UNUSED_ARG(hDescribe);
    RV_UNUSED_ARG(pResponse);
    return TestConnectionOnErrorEv(hConnection, hApp, hURI, requestMethod, status, hPhrase);
}

/**************************************************************************
 * TestConnectionOnDescribeResponseEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when a Describe response is
 * received on the connection.
 *
 * Input:   hConnection         - the connection.
 *          hApp                - application context.
 *          pDescribeResponse   - the Describe response.
 *          hURI                - the URI on which the request was made.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/

RvStatus RVCALLCONV TestConnectionOnDescribeResponseEv(
    IN    RvRtspConnectionHandle            hConnection,
    IN    RvRtspConnectionAppHandle         hApp,
    IN    RvRtspConnectionDescribeAppHandle hDescribe,
    IN    const RvRtspResponse *            pDescribeResponse,
    IN    RvRtspStringHandle                hURI)
{
    RvSize_t        blobSize = 0;
    RvChar          uri[512];
    RvChar          body[PRINT_BUFFER_LENGTH];
    TestRtspConnectionStruct * pConnection = (TestRtspConnectionStruct *)hApp;

    RV_UNUSED_ARG(hConnection);
    RV_UNUSED_ARG(hDescribe);

    RvRtspStrcpy(pTest->hRtsp, hURI, sizeof(uri), uri);
    TclExecute("set app(config,reqUri) {%s}", uri);
    RvRtspBloblen(pTest->hRtsp, pDescribeResponse->hBody,&blobSize);
    RvRtspBlobcpy(pTest->hRtsp, pDescribeResponse->hBody, PRINT_BUFFER_LENGTH, body);

    body[blobSize] = 0;

    blobSize = strlen(body);

    TclExecute("set app(config,reqMethod) {%s}", "setup");
    TclExecute("set app(config,reqTrackId) {%d}", 0);
    TclExecute("set app(config,reqSessionId) {}");


    if (pTest->isAutoAnswer == RV_TRUE)
    {
        TclExecute("test:ConnsListSetSelect {%d}", pConnection->index);
        TclExecute("sendRequestVariables");
    }


    RV_UNUSED_ARG(hApp);
    return RV_OK;
}



/**************************************************************************
 * TestConnectionOnRedirectRequestEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when a Redirect request is
 * received on the connection.
 *
 * Input:   hConnection     - the connection.
 *          hApp            - application context.
 *          pRequest        - the Redirect request.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/

RvStatus RVCALLCONV TestConnectionOnRedirectRequestEv(
    IN    RvRtspConnectionHandle        hConnection,
    IN    RvRtspConnectionAppHandle     hApp,
    IN    const RvRtspRequest            *pRequest)
{
    RV_UNUSED_ARG(hConnection);
    RV_UNUSED_ARG(hApp);
    RV_UNUSED_ARG(pRequest);
    return RV_OK;
}

/******************************************************************************
 * TestConnectionOnReceiveEv
 * ----------------------------------------------------------------------------
 * General:
 * This callback is called when an RTSP message that is not
 * related to a session is received on a client's connection.
 * This callback can be used to retrieve additional headers or header fields
 * that are not specifically supported by the client.
 *
 * Input::  hConnection      - The connection handle in the client stack.
 *          hApp             - The handle of the application connection object .
 *          pRequest         - The received message if it is a request,
 *                             NULL if it is a response.
 *          pResponse        - The received message if it is a response,
 *                             NULL if it is a request.
 * Output:  None
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *****************************************************************************/
RvStatus RVCALLCONV TestConnectionOnReceiveEv(
    IN    RvRtspConnectionHandle        hConnection,
    IN    RvRtspConnectionAppHandle     hApp,
    IN    RvRtspRequest                 *pRequest,
    IN    RvRtspResponse                *pResponse)
{
    RvStatus               status = RV_OK;
    RvChar                 strToSend[1000];
    RvChar                 temp[30];
    RvChar                 pPrintStr[100];
    RvUint  len    = 30;
    RvChar  *pStr  = strToSend;
    TestRtspConnectionStruct * pConnection = (TestRtspConnectionStruct *)hApp;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
     RvRtspMsgMessageHandle     hMessage;
     RvBool bIsRequest;
#endif

    RV_UNUSED_ARG(hConnection);

    pTest->rtspMethod = 0;

    memset((void *)strToSend,'\0',1000);

    TclExecute("test:ConnsListSetSelect {%d}", pConnection->index);

    if (pRequest != NULL)
    {

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        /* A request message received */
        hMessage   = pRequest->hRtspMsgMessage;
        bIsRequest = RV_TRUE;
#endif
        pTest->rtspMethod = pRequest->requestLine.method;

        TclExecute("set app(config,wreqMethod) {%d}",pRequest->requestLine.method);
        TclExecute("set app(config,resCseq) {%d}",pRequest->cSeq);
    }
    else
    {
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        /* A response message received */
        hMessage   = pResponse->hRtspMsgMessage;
        bIsRequest = RV_FALSE;
#endif

        sprintf(pPrintStr,"test:Log {<--- Received Message on Connection:%d};update", pConnection->index);
        TclExecute(pPrintStr);

        sprintf(pStr,"test:Log {RTSP/1.0");
        while(*pStr != '\0')
              pStr++;
        RvRtspStringHandleGetString(pTest->hRtsp,pResponse->statusLine.hPhrase , temp, &len);
        sprintf(pStr," %d %s};",pResponse->statusLine.status,temp);
        while(*pStr != '\0')
              pStr++;
        TestHandleReceivedMessage(hMessage, bIsRequest, pStr);
        while(*pStr!= '\0')
              pStr++;
        sprintf(pStr,"update");
        TclExecute(strToSend);
        TclExecute("test:Log {----- End of Received Message -----};update");
        TclExecute("test:Log {};update");
    }
    return status;

}

/**************************************************************************
 * TestConnectionOnRawBufferReceiveEv
 * ------------------------------------------------------------------------
 * General:
 * This is the definition for the callback function to be called
 * when a chunk of data is received on the connection.
 * The application can use the callback to extract interleaved
 * (encapsulated) data from the buffer.
 *
 * Input:    hConnection            - The connection handle in the client stack.
 *           hApp                - The handle of the application connection object.
 *           pBuff                - The received buffer.
 *           buffSize            - The received buffer size.
 * Output:   None
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnRawBufferReceiveEv(
    IN    RvRtspConnectionHandle      hConnection,
    IN    RvRtspConnectionAppHandle    hApp,
    IN  RvUint8                     *pBuff,
    IN  RvUint32                    buffSize)
{
    RV_UNUSED_ARG(hConnection);
    RV_UNUSED_ARG(hApp);
    RV_UNUSED_ARG(pBuff);
    RV_UNUSED_ARG(buffSize);

    return RV_OK;

    /* Call the rawReceive API and return an error code
    RvRtspConnectionReceiveRawBuffer(hConnection, pBuff, buffSize);
    return RV_ERROR_UNKNOWN;*/
}

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)

/******************************************************************************
 * TestConnectionOnSendEv
 * ----------------------------------------------------------------------------
 * General: This callback is called when an RTSP message that is not
 *          related to a session is sent on a client's connection.
 *          This callback can be used to edit the outgoing message and add
 *          additional headers or header fields that are not specifically
 *          supported by the client.This callback is also used to print the
 *          outgoing message on the GUI message window.
 *
 * Arguments:
 * Input:  hConnection      - The connection handle in the client stack.
 *         hApp             - The handle of the application connection object .
 *         pRequest         - The received message if it is a request,
 *                            NULL if it is a response.
 *         pResponse        - The received message if it is a response,
 *                            NULL if it is a request.
 * Output:  None.
 *
 * Return   Value: RV_OK  - if successful.
 *                 Other on failure
 *****************************************************************************/
RvStatus RVCALLCONV TestConnectionOnSendEv(
    IN      RvRtspConnectionHandle              hConnection,
    IN      RvRtspConnectionAppHandle           hApp,
    IN      RvRtspRequest                       *pRequest,
    IN      RvRtspResponse                      *pResponse)
{

    RvBool   bIsRequest;
    RvChar   strToSend[1000];
    RvChar   reqStrURI[100];
    RvChar   strURI[100];
    RvChar   pPrintStr[100];
    RvUint32 strLen;
    TestRtspConnectionStruct * pConnection = (TestRtspConnectionStruct *)hApp;

    RvChar   *pStr   = strToSend;

    RV_UNUSED_ARG(hConnection);

    if(pRequest == NULL)
        return RV_ERROR_BADPARAM;
    else
    {
        bIsRequest = RV_TRUE;
        TestAddAdditionalHeader(bIsRequest, NULL,pRequest);
    }

    memset((void *)strToSend,'\0',sizeof(strToSend));
    memset((void *)reqStrURI,'\0',sizeof(reqStrURI));

    if(pRequest != NULL)
    {
        bIsRequest = RV_TRUE;

        RvRtspStrcpy(pTest->hRtsp, pRequest->requestLine.hURI, 100, reqStrURI);
        strLen = strlen(reqStrURI)-strlen("/track1");

        strncpy(strURI , reqStrURI, strLen);
        sprintf(pPrintStr,"test:Log {---> Sending Message on Connection:%d};update", pConnection->index);
        TclExecute(pPrintStr);
        sprintf(pStr,"test:Log {");

        while(*pStr != '\0')
              pStr++;
        switch(pRequest->requestLine.method)
        {
        case RV_RTSP_METHOD_OPTIONS:
             sprintf(pStr,"OPTIONS %s RTSP/1.0}; ",reqStrURI);
             while(*pStr != '\0')
                   pStr++;
             TestPrintSentMessage(bIsRequest,pRequest,pResponse,pStr);
             break;
        case RV_RTSP_METHOD_DESCRIBE:
             sprintf(pStr,"DESCRIBE %s RTSP/1.0};",reqStrURI);
             while(*pStr != '\0')
                   pStr++;
             TestPrintSentMessage(bIsRequest,pRequest,pResponse,pStr);
             break;
        case RV_RTSP_METHOD_SETUP:
             sprintf(pStr,"SETUP %s RTSP/1.0};",reqStrURI);
             while(*pStr != '\0')
                   pStr++;
             TestPrintSentMessage(bIsRequest,pRequest,pResponse,pStr);
             break;
        case RV_RTSP_METHOD_GET_PARAMETER:
             sprintf(pStr,"GET PARAMETER %s RTSP/1.0};",reqStrURI);
             while(*pStr != '\0')
                   pStr++;
             TestPrintSentMessage(bIsRequest,pRequest,pResponse,pStr);
             break;
        case RV_RTSP_METHOD_PLAY:
        case RV_RTSP_METHOD_PAUSE:
        case RV_RTSP_METHOD_TEARDOWN:
             sprintf(pStr,"Command on Invalid Session};");
             break;
        default:
             sprintf(pStr,"Command not Supported};");
             break;
        }
        while(*pStr!= '\0')
           pStr++;

        sprintf(pStr,"update");

        TclExecute(strToSend);
        TclExecute("test:Log {----- End of Sent Message -----};update");
        TclExecute("test:Log {};update");

     }
     else
     {
         TclExecute("test:Log {TestServerConnectionOnReceiveEv: Unknown Message type};"
                    "update");
         return RV_ERROR_BADPARAM;
     }
    return RV_OK;
}

/******************************************************************************
 * TestSessionOnSendEv
 * ----------------------------------------------------------------------------
 * General: This callback is called when an RTSP messageis sent on a client's
 *           session. This callback can be used to edit the outgoing message and add
 *          additional headers or header fields that are not specifically
 *          supported by the client. This callback is also used to print the
 *          outgoing message on the GUI message window.
 *
 * Arguments:
 * Input:  hSession         - The session handle in the client stack.
 *         hApp             - The handle of the application session object .
 *         pRequest         - The received message if it is a request,
 *                            NULL if it is a response.
 *         pResponse        - The received message if it is a response,
 *                            NULL if it is a request.
 * Output: None
 *
 * Return   Value: RV_OK  - if successful.
 *                 Other on failure
 *****************************************************************************/
RvStatus RVCALLCONV TestSessionOnSendEv(
                                IN      RvRtspSessionHandle         hSession,
                                IN      RvRtspSessionAppHandle      hApp,
                                IN      RvRtspRequest               *pRequest,
                                IN      RvRtspResponse              *pResponse)
{

    RvChar   strURI[100];
    RvChar   strSessId[100];
    RvChar   strToSend[1000];
    RvChar   reqStrURI[100];
    RvChar   pPrintStr[100];
    RvBool   bIsRequest;

    RvUint   len     = 100;
    RvChar   *pStr   = &strToSend[0];

    RV_UNUSED_ARG(hSession);
    RV_UNUSED_ARG(hApp);
    RV_UNUSED_ARG(pRequest);
    RV_UNUSED_ARG(pResponse);

    if(pRequest == NULL)
        return RV_ERROR_BADPARAM;
    else
    {
        bIsRequest = RV_TRUE;
        TestAddAdditionalHeader(bIsRequest, NULL,pRequest);
    }

    memset((void *)strToSend,'\0',sizeof(strToSend));
    memset((void *)reqStrURI,'\0',sizeof(reqStrURI));
    memset((void *)pPrintStr,'\0',sizeof(pPrintStr));
    memset((void *)strSessId,'\0',sizeof(strSessId));

    RvRtspStrcpy(pTest->hRtsp, pRequest->session.hSessionId, 100, strSessId);
    if(pRequest->session.hSessionId != NULL)
    {
        sprintf(pPrintStr,"test:Log {---> Sending Message on Session:%s};update",strSessId);
        TclExecute(pPrintStr);
    }
    else
    {
        TclExecute("test:Log {---> Sending Message on Session };update");
    }

    RvRtspStringHandleGetString(pTest->hRtsp, pRequest->requestLine.hURI, strURI, &len);

    sprintf(pStr,"test:Log {");
    while(*pStr != '\0')
          pStr++;
    bIsRequest = RV_TRUE;

    switch(pRequest->requestLine.method)
    {
        case RV_RTSP_METHOD_SETUP:

             sprintf(pStr,"SETUP %s RTSP/1.0};",strURI);
             while(*pStr != '\0')
                   pStr++;
             TestPrintSentMessage(bIsRequest,pRequest,pResponse,pStr);
             break;
        case RV_RTSP_METHOD_PLAY:

             sprintf(pStr,"PLAY %s RTSP/1.0};",strURI);
             while(*pStr != '\0')
                   pStr++;
             TestPrintSentMessage(bIsRequest,pRequest,pResponse,pStr);
             break;
        case RV_RTSP_METHOD_PAUSE:
             sprintf(pStr,"PAUSE %s RTSP/1.0};",strURI);
             while(*pStr != '\0')
                   pStr++;
             TestPrintSentMessage(bIsRequest,pRequest,pResponse,pStr);
             break;
        case RV_RTSP_METHOD_GET_PARAMETER:

             sprintf(pStr,"GET_PARAMETER  %s RTSP/1.0};",strURI);
             while(*pStr != '\0')
                   pStr++;
             TestPrintSentMessage(bIsRequest,pRequest,pResponse,pStr);
             break;
        case RV_RTSP_METHOD_TEARDOWN:

             sprintf(pStr,"TEARDOWN %s RTSP/1.0};",strURI);
             while(*pStr != '\0')
                   pStr++;
             TestPrintSentMessage(bIsRequest,pRequest,pResponse,pStr);
             break;
        default:
             sprintf(pStr,"Command not Supported};");
             break;
         }

        while(*pStr!= '\0')
              pStr++;

        sprintf(pStr,"update");
        while(*pStr!= '\0')
              pStr++;

        TclExecute(strToSend);
        TclExecute("test:Log {----- End of Sent Message -----};update");


    return RV_OK;
}

#endif

/**************************************************************************
 * TestSessionOnStateChangeEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when the session's state is
 * changed (due to a response message received for the session).
 *
 * Input:    hSession    - the session.
 *           hApp        - application context.
 *           currState    - the current session state.
 *           newState    - the new session state.
 *           pResponse    - the received response.
 * Output:   None.
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RVCALLCONV TestSessionOnStateChangeEv(
    IN    RvRtspSessionHandle       hSession,
    IN    RvRtspSessionAppHandle    hApp,
    IN    RvRtspSessionState        currState,
    IN    RvRtspSessionState        newState,
    IN    const RvRtspResponse      *pResponse)
{
    RvChar                 str[PRINT_BUFFER_LENGTH];
    RvRtspStringHandle     hStr;
    RvChar                 strToSend[1000];
    RvChar                 temp[30];
    RvChar                 pPrintStr[100];

    RvUint  len        = 30;
    RvUint  strLen     = 50;
    RvChar  *pStr      = strToSend;
    TestRtspSessionStruct * pSession = (TestRtspSessionStruct *)hApp;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgMessageHandle     hMessage;
    RvBool  bIsRequest = RV_FALSE;
#endif


    memset((void *)strToSend,'\0',1000);

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    /* A response message received */
    hMessage   = pResponse->hRtspMsgMessage;
#endif

    RvRtspSessionGetId(hSession, &hStr);
    RvRtspStringHandleGetString(pTest->hRtsp, hStr, str, &strLen);
    TclExecute("set app(config,reqSessionId) {%s}",str);

    sprintf(pPrintStr,"test:Log {<--- Received Message on Session:%s};update",str);
    TclExecute(pPrintStr);

    sprintf(pStr,"test:Log {RTSP/1.0");
    while(*pStr != '\0')
          pStr++;
    RvRtspStringHandleGetString(pTest->hRtsp,pResponse->statusLine.hPhrase , temp, &len);
    sprintf(pStr," %d %s};",pResponse->statusLine.status,temp);
    while(*pStr != '\0')
          pStr++;
    TestHandleReceivedMessage(hMessage, bIsRequest, pStr);
    while(*pStr!= '\0')
          pStr++;
    sprintf(pStr,"update");
    TclExecute(strToSend);
    TclExecute("test:Log {----- End of Recevied Message -----};update");
    TclExecute("test:Log {};update");

    switch (newState)
    {
        case RV_RTSP_SESSION_STATE_INIT:
        {
            strcpy(pSession->state, "INIT");
            TestRtspSessionDestruct(pSession);
            break;
        }

        case RV_RTSP_SESSION_STATE_READY:
        {
            strcpy(pSession->state, "READY");
            switch (currState)
            {
                case RV_RTSP_SESSION_STATE_INIT:
                {
                    TclExecute("set app(config,reqUri) {%s}", pSession->trackArray[1]);
                    TclExecute("set app(config,reqTrackId) {%d}", 1);
                    TclExecute("set app(config,reqMethod) {%s}","setup");

                    if (pTest->isAutoAnswer == RV_TRUE)
                    {
                        TclExecute("test:ConnsListSetSelect {%d}", pSession->pConnection->index);
                        TclExecute("test:SessListSetSelect {%d}", pSession->index);
                        TclExecute("sendRequestVariables");
                    }
                    break;
                }

                case RV_RTSP_SESSION_STATE_READY:
                {
                    TclExecute("set app(config,reqMethod) {%s}","play");
                    TclExecute("set app(config,reqUri) {%s}", pSession->strURI);
                    TclExecute("set app(config,reqTrackId) {%d}", 0);
                    if (pTest->isAutoAnswer == RV_TRUE)
                    {
                        TclExecute("test:ConnsListSetSelect {%d}", pSession->pConnection->index);
                        TclExecute("test:SessListSetSelect {%d}", pSession->index);
                        TclExecute("sendRequestVariables");
                    }
                    break;
                }

                case RV_RTSP_SESSION_STATE_PLAYING:
                {
                    TclExecute("test:ConnsListSetSelect {%d}", pSession->pConnection->index);
                    TclExecute("test:SessListSetSelect {%d}", pSession->index);
                    break;
                }

                default:
                {
                    break;
                }
            }   /* switch currState */
            break;
        }

        case RV_RTSP_SESSION_STATE_PLAYING:
        {
            strcpy(pSession->state, "PLAYING");
            TclExecute("test:ConnsListSetSelect {%d}", pSession->pConnection->index);
            TclExecute("test:SessListSetSelect {%d}", pSession->index);
            break;
        }

        default:
        {
            break;
        }
    } /* switch newState */


    return RV_OK;
}


/**************************************************************************
 * TestSessionOnErrorEv
 * ------------------------------------------------------------------------
 * General:
 * Event callback function, called when an error response is
 * received for the session.
 *
 * Arguments:
 * Input:    hSession        - the session.
 *           hApp            - application context.
 *           requestMethod    - the request for which the error occurred.
 *           status            - the returned status.
 *           hPhrase            - the status phrase.
 * Output:   None.
 *
 * Return Value:  RV_OK if successful
 *                Negative values otherwise.
 *************************************************************************/

RvStatus RVCALLCONV TestSessionOnErrorEv(
    IN    RvRtspSessionHandle        hSession,
    IN    RvRtspSessionAppHandle    hApp,
    IN    RvRtspMethod              requestMethod,
    IN    RvRtspStatus              status,
    IN    RvRtspStringHandle        hPhrase)
{
    RvChar phrase[500];
    TestRtspSessionStruct * pSession = (TestRtspSessionStruct *)hApp;
    RvUint32 len = 500;

    RV_UNUSED_ARG(requestMethod);
    RV_UNUSED_ARG(status);

    memset(phrase, 0, 500);

    if (hPhrase != NULL)
        RvRtspStringHandleGetString(Test.hRtsp, hPhrase, phrase, &len);

    TestRtspSessionDestruct(pSession);

    RV_UNUSED_ARG(hSession);

    return RV_OK;
}


/**************************************************************************
 * TestRtspConnectionsDisconnect
 * ------------------------------------------------------------------------
 * General:
 * Internal function, used to disconnect the connection objects from
 * the server.
 *
 * Input:    pTest    - structure holding the stack related variables.
 * Output:   None.
 *
 * Return Value: None
 *************************************************************************/
void TestRtspConnectionsDisconnect(
    IN TestRtspStackStruct    *pTest)
{
    void * key = NULL;
    TestRtspConnectionStruct * pConnection;
    TestRtspConnectionStruct * pNextConnection;

    pConnection = (TestRtspConnectionStruct *)AppHashGetAny(pTest->connectionHash, &key);
    while(pConnection != NULL)
    {
        pNextConnection = (TestRtspConnectionStruct *)AppHashGetAny(pTest->connectionHash, &key);
        RvRtspConnectionDisconnect(pConnection->hConnection);
        pConnection = pNextConnection;
    }
}

/**************************************************************************
 * TestClientConnectionDestruct
 * ------------------------------------------------------------------------
 * General:
 *  The function is used to disconnect the connection object fromthe server.
 *
 * Input:    index    - application context.
 * Output:   None.
 *
 * Return Value: None
 *************************************************************************/
void TestClientConnectionDestruct(
    IN RvUint    index)
{
    TestRtspConnectionStruct * pConnection =
        (TestRtspConnectionStruct *)AppHashGet(pTest->connectionHash, index);
    RvUint16 i;

    RvRtspConnectionDestruct(pTest->hRtsp, pConnection->hConnection, RV_TRUE);

    for( i = 0; i < TEST_MAX_SESSIONS_PER_CONN; i++)
    {
        if (pConnection->sessions[i] != NULL)
        {
            AppHashRemove(pConnection->pRtspApp->sessionHash, pConnection->sessions[i]->index);
            pConnection->sessions[i] = NULL;
        }
    }

    TclExecute("test:ConnsListDel {%d}", index);
    TclExecute("test:Log {Disconnected from Server};update");

    AppHashRemove(pConnection->pRtspApp->connectionHash, pConnection->index);
}

/**************************************************************************
 * TestRtspSessionConstruct
 * ------------------------------------------------------------------------
 * General:
 * Constructs the stack's sessions and sets the URIs according to
 * the streamURI array.
 *
 * Input:    :    pRtsp    - structure holding the stack related variables.
 * Output:    :    None.
 *
 * Return Value: session application handle
 *************************************************************************/
RvRtspSessionAppHandle TestRtspSessionConstruct(
    IN int iConnection)
{
    TestRtspConnectionStruct * pConnection =
        (TestRtspConnectionStruct *)AppHashGet(pTest->connectionHash, iConnection);
    RvStatus result;
    TestRtspSessionStruct * pSession;
    RvInt32 iSession = AppAllocateResourceId(pTest, AppResourceSession);

    AppHashAdd(pTest->sessionHash, iSession, (void **)&pSession);
    pSession->index = iSession;
    pSession->pConnection = pConnection;
    pSession->pRtspApp = pTest;
    pSession->state[0] = '\0';
    pSession->strURI[0] = '\0';

    pTest->sessionCallbackFunctions.pfnOnStateChangeEv              = TestSessionOnStateChangeEv;
    pTest->sessionCallbackFunctions.pfnOnErrorEv                    = TestSessionOnErrorEv;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pTest->sessionCallbackFunctions.pfnOnSendEv                     = TestSessionOnSendEv;
#endif
    pTest->sessionCallbackFunctions.pfnOnDestructEv                 = NULL;
    pTest->sessionConfiguration.pingTransmissionTimeOutResolution    = pTest->sessionConfiguration.pingTransmissionTimeOutResolution;
    pTest->sessionConfiguration.responseTimeOutResolution            = pTest->sessionConfiguration.responseTimeOutResolution;

    result = RvRtspSessionConstruct(
            pConnection->hConnection,
            &pTest->sessionConfiguration,
            sizeof(pTest->sessionConfiguration),
            &pTest->sessionCallbackFunctions,
            sizeof(pTest->sessionCallbackFunctions),
            (RvRtspSessionAppHandle)pSession,
            &pSession->hSession);

    if (result != RV_OK)
    {
       TclExecute("test:Log {Error in session construct}");
       AppHashRemove(pTest->sessionHash, iSession);
       return NULL;
    }

    pConnection->sessions[pConnection->sessCount] = pSession;
    pConnection->sessCount++;

    return (RvRtspSessionAppHandle)pSession;
}



/**************************************************************************
 * TestRtspSessionDestruct
 * ------------------------------------------------------------------------
 * General:
 * Tears down and destructs all the stack's sessions.
 *
 * Arguments:
 * Input::    pSession    - structure holding the session related variables.
 * Output:    None.
 *
 * Return Value: None
 *************************************************************************/
void    TestRtspSessionDestruct(
    IN  TestRtspSessionStruct * pSession)
{
    RvUint i;
    RvBool found = RV_FALSE;

    RvRtspSessionDestruct(pSession->hSession);

    for (i = 0; i < pSession->pConnection->sessCount ; i++)
    {
        if (!found)
        {
            found = (pSession->pConnection->sessions[i] == pSession);
        }
        if (found)
        {
            if (i == (pSession->pConnection->sessCount-1))
            {
                pSession->pConnection->sessions[i] = NULL;
            }
            else
            {
                pSession->pConnection->sessions[i] = pSession->pConnection->sessions[i+1];
            }
        }
    }
    if (found)
    {
        pSession->pConnection->sessCount--;
    }
    TestClientUpdateSessionList(pSession->pConnection->index);

    if (pSession->pConnection->sessCount == 0)
    {
        RvRtspConnectionDisconnect(pSession->pConnection->hConnection);
    }
}

/**************************************************************************
 * doneTest
 * ------------------------------------------------------------------------
 * General: destructs module function
 *
 * Return Value:        void
 * ------------------------------------------------------------------------
 * Arguments:
 * Input:        :       None.
 * Output:       :       None.
 * INOUT        :       None.
 *************************************************************************/
void doneTest(void)
{
    TestRtspConnectionsDisconnect(&Test);
    pTest = NULL;
    RvRtspEnd(Test.hRtsp);
    RvRtspMainLoop(Test.hRtsp, 0);
    TclExecute("test:Log {Rtsp Stack stopped};update");

#if RV_LOGMASK != RV_LOGLEVEL_NONE
    RvLogDestruct(&Test.logMgr);
#endif
    RvCBaseEnd();
}

/******************************************************************************
 * TestStackInitAndStart
 * ----------------------------------------------------------------------------
 * General:
 *  Function to initialize the Stack.
 *
 * Arguments:
 * Input::  argv         - Arguments vector received from the GUI.
 * Output:: None
 *
 * Return Value: None
 *****************************************************************************/
void TestStackInitAndStart(char *argv[])
{
    pTest = &Test;

    /* initialize stack */
    Test.rtspConfiguration.maxConnections             = (RvUint16)atoi(argv[1]);
    Test.rtspConfiguration.memoryElementsNumber       = (RvUint16)atoi(argv[2]);
    Test.rtspConfiguration.memoryElementsSize         = (RvUint16)atoi(argv[3]);
    Test.rtspConfiguration.msgRequestElementsNumber   = (RvUint16)atoi(argv[4]);
    Test.rtspConfiguration.msgResponseElementsNumber  = (RvUint16)atoi(argv[5]);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    Test.rtspConfiguration.maxRtspMsgHeadersInMessage = (RvUint16)atoi(argv[6]);
#endif
    strcpy(Test.rtspConfiguration.strDnsAddress, argv[7]);

    pTest->connectionConfiguration.dnsMaxResults               = (RvUint16)atoi(argv[12]);
    pTest->connectionConfiguration.maxHeadersInMessage         = (RvUint16)atoi(argv[6]);
    pTest->connectionConfiguration.maxSessions                 = (RvUint16)atoi(argv[8]);
    pTest->connectionConfiguration.maxUrlsInMessage            = (RvUint16)atoi(argv[11]);
    pTest->connectionConfiguration.transmitQueueSize           = (RvUint16)atoi(argv[10]);
    pTest->connectionConfiguration.maxWaitingDescribeRequests  = (RvUint16)atoi(argv[9]);
    pTest->connectionConfiguration.describeResponseTimeOut     = (RvUint32)atoi(argv[13]);

    pTest->sessionConfiguration.pingTransmissionTimeOutResolution  = atoi(argv[15]);
    pTest->sessionConfiguration.responseTimeOutResolution          = atoi(argv[14]);

    TestStackInit();
    TestStackStart();
}

/********************************************************************************************
 * TestStackInit
 * purpose : Initialize the CBase and logs
 *
 * input   : none
 * output  : none
 * return  : none
 ********************************************************************************************/
void TestStackInit(void)
{
    pTest = &Test;

    /* Initialize the common module */
    RvCBaseInit();

    pTest->connectionHash = AppHashInit(sizeof(TestRtspConnectionStruct));
    pTest->sessionHash = AppHashInit(sizeof(TestRtspSessionStruct));

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)

    RvLogConstruct(&pTest->logMgr);
    RvLogSetGlobalMask(&pTest->logMgr, RV_LOGLEVEL_EXCEP | RV_LOGLEVEL_ERROR | RV_LOGLEVEL_WARNING );
    RvLogListenerConstructLogfile(&pTest->logFile, &pTest->logMgr, "rtspClientLog.txt", 1, 0, RV_TRUE);
    RvLogRegisterListener(&pTest->logMgr, AppLogListenerCB, (void*)pTest);

#endif
}


/********************************************************************************************
 * TestStackStart
 * purpose : Initialize the stack
 *
 * input   : none
 * output  : none
 * return  : none
 ********************************************************************************************/
void TestStackStart(void)
{
    RvStatus status = RV_OK;

    pTest->connectionCallbacks.pfnOnConnectEv           = TestConnectionOnConnectEv;
    pTest->connectionCallbacks.pfnOnDisconnectEv        = TestConnectionOnDisconnectEv;
    pTest->connectionCallbacks.pfnOnErrorEv             = TestConnectionOnErrorEv;
    pTest->connectionCallbacks.pfnOnDescribeResponseEv  = TestConnectionOnDescribeResponseEv;
    pTest->connectionCallbacks.pfnOnErrorExtEv          = TestConnectionOnErrorExtEv;
    pTest->connectionCallbacks.pfnOnReceiveEv           = TestConnectionOnReceiveEv;
    pTest->connectionCallbacks.pfnOnRawBufferReceiveEv  = TestConnectionOnRawBufferReceiveEv;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pTest->connectionCallbacks.pfnOnSendEv              = TestConnectionOnSendEv;
#endif

    status = RvRtspInit(
        (RvLogHandle)&pTest->logMgr,
        &Test.rtspConfiguration,
        sizeof(Test.rtspConfiguration),
        &Test.hRtsp);

    if (status == RV_OK)
    {
       TclExecute("test:Log {Rtsp Stack Started};update");
       gbStackInited = RV_TRUE;
    }
    else
    {
       TclExecute("test:Log {Rtsp Stack Init Failed};update");
    }

    gbStopStack   = RV_FALSE;
}




/********************************************************************************************
 * TestStackLoop
 * purpose : Stack select loop
 *
 * input   : none
 * output  : none
 * return  : none
 ********************************************************************************************/
RvStatus TestStackLoop(void)
{
    if (RvRtspMainLoop(Test.hRtsp, 0) != RV_OK)
    {
        TclExecute("test:Log {Rtsp Stack stopped};update");
        return -1;
    }
    return RV_OK;
}






/******************************************************************************
 * TestSetStackVariables
 * ----------------------------------------------------------------------------
 * General:
 *  Set the values of the stack variables fromthe GUI Input:.
 *
 * Arguments:
 * Input::  varName   - name of the variable.
 *         value     - value of the variable.
 * Output:: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestSetStackVariables(char * varName, char * value)
{
    if (strncmp(varName, "logFilter,", 10) == 0)
    {
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
        int i, j;

        for (i = 0; sources[i][0] != '\0'; i++)
        {
            if (strncmp(varName+10, sources[i], strlen(sources[i])) == 0)
            {
                for (j = 0; levels[j][0] != '\0'; j++)
                {
                    if (strncmp(varName+10+strlen(sources[i])+1, levels[j], strlen(levels[j])) == 0)
                    {
                        RvLogMessageType curMask, change;
                        RvLogSource logSource;
                        RvStatus status =
                            RvLogGetSourceByName(&pTest->logMgr, sources[i], &logSource);
                        if (status != RV_OK)
                        {
                            /* It doesn't - we create it. This might cause us a "resource leak" of log sources,
                               but we're better off this way than crashing on too many calls to msDelete(). */
                            status = RvLogSourceConstruct(&pTest->logMgr, &logSource, sources[i], "");
                        }

                        curMask = RvLogSourceGetMask(&logSource);
                        if (atoi(value) != 0)
                        {
                            /* log filter was added */
                            change = (RvLogMessageType)(1 << j);
                            curMask |= change;
                        }
                        else
                        {
                            /* log filter was removed */
                            change = (RvLogMessageType)~(1 << j);
                            curMask &= change;
                        }
                        RvLogSourceSetMask(&logSource, curMask);
                    }
                }
            }
        }
#endif
    }
    else if (strcmp(varName, "config,autoAnswer") == 0)
    {
       Test.isAutoAnswer = atoi(value);
    }
    else if (strcmp(varName, "config,maxSession") == 0)
    {
       Test.connectionConfiguration.maxSessions = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,maxDescReq") == 0)
    {
       Test.connectionConfiguration.maxWaitingDescribeRequests = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,transmitQSize") == 0)
    {
       Test.connectionConfiguration.transmitQueueSize = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,maxUri") == 0)
    {
       Test.connectionConfiguration.maxUrlsInMessage = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,dnsMaxResults") == 0)
    {
       Test.connectionConfiguration.dnsMaxResults = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,descTimeout") == 0)
    {
       Test.connectionConfiguration.describeResponseTimeOut = atoi(value);
    }
    else if (strcmp(varName, "config,responseTimeout") == 0)
    {
       Test.sessionConfiguration.responseTimeOutResolution = atoi(value);
    }
    else if (strcmp(varName, "config,pingTimeout") == 0)
    {
       Test.sessionConfiguration.pingTransmissionTimeOutResolution = atoi(value);
    }
    else if (strcmp(varName, "config,maxConnections") == 0)
    {
       Test.rtspConfiguration.maxConnections = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,maxRequests") == 0)
    {
       Test.rtspConfiguration.msgRequestElementsNumber = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,maxResponses") == 0)
    {
       Test.rtspConfiguration.msgResponseElementsNumber = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,memoryElementSize") == 0)
    {
       Test.rtspConfiguration.memoryElementsSize = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,memoryElementNum") == 0)
    {
       Test.rtspConfiguration.memoryElementsNumber = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,maxHeaders") == 0)
    {
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
       Test.rtspConfiguration.maxRtspMsgHeadersInMessage = (RvUint16)atoi(value);
#endif
       Test.connectionConfiguration.maxHeadersInMessage = (RvUint16)atoi(value);
    }
    else if (strcmp(varName, "config,dnsAddr") == 0)
    {
        strcpy(Test.rtspConfiguration.strDnsAddress, value);
    }
    else if (strcmp(varName, "options,catchStackLog")== 0)
    {
       Test.bCatchStackLog = (RvUint16)atoi(value);
    }
}

/******************************************************************************
 * TestClientSendResponse
 * ----------------------------------------------------------------------------
 * General:
 *  sends the response with the input values provided.
 *
 * Arguments:
 * Input::  argv     - holds the Input: strings from the GUI.
 * Output:: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientSendResponse(char *argv[])
{
    RvRtspResponse             *response;
    RvUint32                   strLen;
    RvStatus                   result;
    TestRtspConnectionStruct * pConnection;

    strLen  = 0;

    RvRtspMessageConstructResponse( pTest->hRtsp, &response);

    response->cSeqValid              = RV_TRUE;

    /* Assign the values provided by the GUI to the response */
    response->statusLine.status       = atoi(argv[4]);

    strLen = strlen(argv[5])+1;
    RvRtspStringHandleSetString(pTest->hRtsp, argv[5], strLen, &response->statusLine.hPhrase);

    response->cSeq.value             = (RvUint16)atoi(argv[6]);

    pConnection = (TestRtspConnectionStruct *)AppHashGet(pTest->connectionHash, atoi(argv[2]));
    result = RvRtspConnectionSendResponse(pConnection->hConnection, response);

    if (result == RV_OK)
    {
        TclExecute("test:Log {Response Sent on connection handle %d};update", atoi(argv[2]));
    }
    else
    {
        TclExecute("test:Log {Unable to send response on connection handle%d};update", atoi(argv[2]));
    }

    RvRtspMessageDestructResponse(pTest->hRtsp, response);
}

/******************************************************************************
 * TestClientSendDescRequest
 * ----------------------------------------------------------------------------
 * General:
 *  Function to send describe request.
 *
 * Arguments:
 * Input:  argv         - Arguments vector received from the GUI.
 * Output: None
 *
 * Return Value: None
 *****************************************************************************/
void TestClientSendDescRequest(char *argv[])
{
    RvUint i;
    TestRtspConnectionStruct * pConnection;
    RvRtspConnectionHandle hConnection;

    TestParseAndStoreAdditionalHeader(argv[10]);

    for (i=2; i<=8; i+=2)
    {
        if (atoi(argv[i]) == RV_TRUE)
        {
            RvRtspGetConnectionByURI(pTest->hRtsp, argv[i+1], &hConnection);
            if (hConnection == NULL)
            {
                pConnection = (TestRtspConnectionStruct *) TestClientConstructConn(argv[i+1]);
                if (pConnection != NULL)
                {
                    RvRtspMainLoop(Test.hRtsp, 100);
                    RvRtspConnectionRequestDescribe(pConnection->hConnection, argv[i+1],
                        (RvRtspConnectionDescribeAppHandle)pConnection->index);
                }
            }
            else
            {
                RvRtspConnectionRequestDescribe(hConnection, argv[i+1],
                    (RvRtspConnectionDescribeAppHandle)atoi(argv[1]));
            }
        }
    }
}

/******************************************************************************
 * TestClientConstructConn
 * ----------------------------------------------------------------------------
 * General:
 *  Function to construct a connection.
 *
 * Arguments:
 * Input:  strURI    - Connections URI.
 * Output: None
 *
 * Return Value: hApp of the connection constructed
 *****************************************************************************/
RvRtspConnectionAppHandle TestClientConstructConn(RvChar *strURI)
{
    RvUint     index;
    TestRtspConnectionStruct * pConnection;
    RvStatus   result;

    index = AppAllocateResourceId(pTest, AppResourceConnection);
    AppHashAdd(pTest->connectionHash, index, (void **)&pConnection);
    pConnection->index = index;
    pConnection->pRtspApp = pTest;
    pConnection->sessCount = 0;
    pConnection->sessions[0] = NULL;

    result = RvRtspConnectionConstruct(pTest->hRtsp,
        (RvRtspConnectionAppHandle)pConnection,
        strURI,
        NULL,
        &pTest->connectionConfiguration,
        sizeof(pTest->connectionConfiguration),
        &pConnection->hConnection);
    if (result != RV_OK)
    {
        TclExecute("test:Log {Error in Connection construct}");
        return NULL;
    }

    RvRtspConnectionRegisterCallbacks(pConnection->hConnection,
        &pTest->connectionCallbacks,
        sizeof(pTest->connectionCallbacks));

    result = RvRtspConnectionConnect(pConnection->hConnection);
    if (result != RV_OK)
    {
        TclExecute("test:Log {Unable to connect}");
        RvRtspConnectionDestruct(pTest->hRtsp, pConnection->hConnection, RV_TRUE);
        AppHashRemove(pTest->connectionHash, index);
        return NULL;
    }

    return (RvRtspConnectionAppHandle)pConnection;
}

/******************************************************************************
 * TestClientSendRequest
 * ----------------------------------------------------------------------------
 * General:
 *  sends the response with the input values provided.
 *
 * Arguments:
 * Input::  argv     - holds the Input: strings from the GUI.
 * Output:: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientSendRequest(char *argv[])
{
    RvChar                   uri[512];
    RvRtspRequest            *request  = NULL;
    RvStatus                 status    = RV_OK;
    RvUint                   strLen    = 0;
    RvStatus                 result;
    RvRtspConnectionHandle   hConnection;
    TestRtspConnectionStruct * pConnection;
    TestRtspSessionStruct * pSession;

    /* First construct ther request object */
    status = RvRtspMessageConstructRequest(Test.hRtsp, &request);
    /* memset MUST NOT be called here */

    if (status != RV_OK)
       return;

    strcpy(uri,argv[3]);


    if (STRCASECMP(argv[1],"describe") == 0)
    {
        pConnection = (TestRtspConnectionStruct *)AppHashGet(pTest->connectionHash, atoi(argv[2]));
        RvRtspConnectionRequestDescribe(pConnection->hConnection, uri,
            (RvRtspConnectionDescribeAppHandle)pConnection->index);
    }
    if (STRCASECMP(argv[1],"options") == 0)
    {
        request->requestLine.method = RV_RTSP_METHOD_OPTIONS;

        RvRtspGetConnectionByURI(pTest->hRtsp, uri, &hConnection);
        if (hConnection == NULL)
        {
            pConnection = (TestRtspConnectionStruct *)TestClientConstructConn(uri);
            if (pConnection != NULL)
            {
                RvRtspConnectionSendRequest(pConnection->hConnection, request);
            }
        }
        else
        {
            strLen = strlen(uri);
            RvRtspStringHandleSetString(Test.hRtsp, uri, strLen, &request->requestLine.hURI);

            TestParseAndStoreAdditionalHeader(argv[16]);

            pConnection = (TestRtspConnectionStruct *)AppHashGet(pTest->connectionHash, atoi(argv[2]));
            result = RvRtspConnectionSendRequest(pConnection->hConnection, request);

            if ( result == RV_OK)
            {
                TclExecute("test:Log {Request Sent on connection handle %d};update", pConnection->index);
                TclExecute("test:Log {};update");
            }
            else
            {
                TclExecute("test:Log {Unable to send Request on connection handle %d};update", atoi(argv[2]));
            }
         }

    }
    if (STRCASECMP(argv[1],"setup") == 0)
    {
        if (atoi(argv[8]) == 0)
        {
            pSession = (TestRtspSessionStruct *)TestRtspSessionConstruct(atoi(argv[2]));
            strcpy(pSession->strURI,uri);
            strcpy(pSession->state, "READY");
            TestClientUpdateSessionList(pSession->pConnection->index);

            strcpy(pSession->trackArray[0], uri);
            strcat(pSession->trackArray[0], "/track1");
            strcpy(pSession->trackArray[1], uri);
            strcat(pSession->trackArray[1], "/track2");
        }
        else
        {
            pSession = (TestRtspSessionStruct *)AppHashGet(pTest->sessionHash, atoi(argv[2]));
            if (strstr(uri, pSession->strURI) == NULL)
            {
                TclExecute("test:Log {Unable to send Request on session handle %d};update", atoi(argv[2]));
                TclExecute("test:Log {Mismatch of the URI and session ID}");
                printf("***** Error: Mismatch of the URI and session ID, sessHApp (%d), uri (%s), stored Uri (%s)\n", atoi(argv[2]), uri, pSession->strURI);
                return;
            }
        }

        if (pSession->trackArray[atoi(argv[5])][0] != '\0')
        {
            RvRtspSessionSetUri(pSession->hSession, pSession->trackArray[atoi(argv[5])]);

            request->requestLine.method = RV_RTSP_METHOD_SETUP;
            strLen = strlen(pSession->trackArray[atoi(argv[5])]);
            RvRtspStringHandleSetString(Test.hRtsp, pSession->trackArray[atoi(argv[5])],
                strLen, &request->requestLine.hURI);

            /* Set the transport */
            memset(&request->transport, 0, sizeof(request->transport));
            /* setting transport parameters for Setup request       */
            request->transport.clientPortA = (RvUint16)atoi(argv[10]);
            request->transport.clientPortB = (RvUint16)atoi(argv[11]);
            request->transport.isUnicast   = (RvBool)atoi(argv[14]);
            request->transportValid = RV_TRUE;

            /* Cseq is set automatically by the stack */

            TestParseAndStoreAdditionalHeader(argv[16]);
            /* Send the request */
            result = RvRtspSessionSendRequest(pSession->hSession, request, RV_TRUE);

            if ( result == RV_OK)
            {
                TclExecute("test:Log {Request Sent on session handle %d};update", atoi(argv[2]));
                TclExecute("test:Log {};update");
            }
            else
            {
                TclExecute("test:Log {Unable to send Request on session handle %d};update", atoi(argv[2]));
            }
        }
   }
   if (STRCASECMP(argv[1],"play") == 0)
   {
        request->requestLine.method  = RV_RTSP_METHOD_PLAY;

        pSession = (TestRtspSessionStruct *)AppHashGet(pTest->sessionHash, atoi(argv[2]));
        RvRtspSessionSetUri(pSession->hSession, uri);

        request->range.startTime.hours   = (RvUint8)atoi(argv[17]);
        request->range.startTime.minutes = (RvUint8)atoi(argv[18]);
        request->range.startTime.seconds = atoi(argv[19]);
        request->range.startTime.format  = RV_RTSP_NPT_FORMAT_SEC;

        TestParseAndStoreAdditionalHeader(argv[16]);

        result = RvRtspSessionSendRequest(pSession->hSession, request, RV_TRUE);

        if ( result == RV_OK)
        {
            TclExecute("test:Log {Request Sent on session handle %d};update", atoi(argv[2]));
            TclExecute("test:Log {};update");
        }
        else
        {
            TclExecute("test:Log {Unable to send Request on session handle %d};update", atoi(argv[2]));
        }

    }
    if (STRCASECMP(argv[1],"pause") == 0)
    {
        request->requestLine.method  = RV_RTSP_METHOD_PAUSE;

        TestParseAndStoreAdditionalHeader(argv[16]);

        pSession = (TestRtspSessionStruct *)AppHashGet(pTest->sessionHash, atoi(argv[2]));
        result = RvRtspSessionSendRequest(pSession->hSession, request, RV_TRUE);

        if ( result == RV_OK)
        {
            TclExecute("test:Log {Request Sent on session handle %d};update", atoi(argv[2]));
            TclExecute("test:Log {};update");
        }
        else
        {
            TclExecute("test:Log {Unable to send Request on session handle%d};update", atoi(argv[2]));
        }

   }

   RvRtspMessageDestructRequest(pTest->hRtsp, request);
}

/******************************************************************************
 * TestClientSendSetup
 * ----------------------------------------------------------------------------
 * General:
 *  sends the setup request to the server.
 *
 * Arguments:
 * Input:  argv     - holds the input strings from the GUI.
 * Output: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientSendSetup (char *argv[])
{

    RvStatus                 result;
    RvRtspRequest            *request  = NULL;
    RvUint                   strLen    = 0;
    RvUint                   trackId   = 0;
    TestRtspSessionStruct *  pSession;

    result = RV_OK;

    /* Parse and store the additional header */
    TestParseAndStoreAdditionalHeader(argv[4]);

    /* First construct the request object */
    result = RvRtspMessageConstructRequest(Test.hRtsp, &request);
    /* memset should not be called */

    pSession = (TestRtspSessionStruct *)AppHashGet(pTest->sessionHash, atoi(argv[3]));

    if ((pSession != NULL) &&
        (strncmp(argv[1], pSession->strURI, strlen(pSession->strURI)) == 0))
    {
        trackId = 1;
        TestClientUpdateSessionList(pSession->pConnection->index);
    }
    else
    {
        pSession = (TestRtspSessionStruct *)TestRtspSessionConstruct(atoi(argv[2]));
        strcpy(pSession->strURI, argv[1]);
        strcpy(pSession->state, "READY");
        TestClientUpdateSessionList(pSession->pConnection->index);

        strcpy(pSession->trackArray[0], argv[1]);
        strcat(pSession->trackArray[0], "/track1");
        strcpy(pSession->trackArray[1], argv[1]);
        strcat(pSession->trackArray[1], "/track2");
        trackId = 0;
    }

    RvRtspSessionSetUri(pSession->hSession, pSession->trackArray[trackId]);

    request->requestLine.method = RV_RTSP_METHOD_SETUP;
    strLen = strlen(pSession->trackArray[trackId]);
    RvRtspStringHandleSetString(Test.hRtsp, pSession->trackArray[trackId], strLen,
        &request->requestLine.hURI);

    /* Set the transport */
    memset(&request->transport, 0, sizeof(request->transport));
    /* setting transport parameters for Setup request       */
    request->transport.clientPortA = 5000;
    request->transport.clientPortB = 5001;
    request->transport.isUnicast   = RV_TRUE;
    request->transportValid = RV_TRUE;

    /* Cseq is set automatically by the stack */

    /* Send the request */
    result = RvRtspSessionSendRequest(pSession->hSession, request, RV_TRUE);

    if ( result == RV_OK)
    {
        TclExecute("test:Log {Request Sent on session handle %d};update", pSession->index);
        TclExecute("test:Log {};update");
    }
    else
    {
        TclExecute("test:Log {Unable to send Request on session handle %d};update", pSession->index);
    }
}

/******************************************************************************
 * TestClientSessionPlay
 * ----------------------------------------------------------------------------
 * General:
 *  sends the play request ont he session
 *
 * Arguments:
 * Input:  hApp   - session application context.
 * Output: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientSessionPlay(RvUint hApp, RvChar *reqAddl)
{
    TestRtspSessionStruct *  pSession;
    RvRtspNptTime       NptTime;

    NptTime.hours   = 0;
    NptTime.minutes = 0;
    NptTime.seconds = 0;
    NptTime.format  = RV_RTSP_NPT_FORMAT_SEC;

    TestParseAndStoreAdditionalHeader(reqAddl);
    pSession = (TestRtspSessionStruct *)AppHashGet(pTest->sessionHash, hApp);
    RvRtspSessionPlay(pSession->hSession, &NptTime);
}

/******************************************************************************
 * TestClientSessionPause
 * ----------------------------------------------------------------------------
 * General:
 *  sends the pause request ont he session
 *
 * Arguments:
 * Input:  hApp   - session application context.
 * Output: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientSessionPause(RvUint hApp, RvChar *reqAddl)
{
    TestRtspSessionStruct *  pSession;
    pSession = (TestRtspSessionStruct *)AppHashGet(pTest->sessionHash, hApp);
    TestParseAndStoreAdditionalHeader(reqAddl);
    RvRtspSessionPause(pSession->hSession);
}

/******************************************************************************
 * TestClientSessionTeardown
 * ----------------------------------------------------------------------------
 * General:
 *  sends the teardown request ont he session
 *
 * Arguments:
 * Input:  hApp   - session application context.
 * Output: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientSessionTeardown(RvUint hApp, RvChar *reqAddl)
{
    TestRtspSessionStruct *  pSession;
    pSession = (TestRtspSessionStruct *)AppHashGet(pTest->sessionHash, hApp);
    TestParseAndStoreAdditionalHeader(reqAddl);
    RvRtspSessionTeardown(pSession->hSession);
}

/******************************************************************************
 * TestClientUpdateSessionList
 * ----------------------------------------------------------------------------
 * General:
 *  Provides the information on the sessions attached to the connection
 *  to the GUI.
 *
 * Arguments:
 * Input::  hApp  = application context of the connection.
 * Output:: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientUpdateSessionList(RvUint hApp)
{
    RvUint                 i;
    RvChar                 str[100];
    RvRtspStringHandle     hStr;
    RvUint                 len;
    TestRtspConnectionStruct * pConnection;

    pConnection = (TestRtspConnectionStruct *)AppHashGet(pTest->connectionHash, hApp);

    TclExecute("test:ClearSessionList");

    for (i = 0; i < TEST_MAX_SESSIONS_PER_CONN; i++)
    {
        if (pConnection->sessions[i] != NULL )
        {
            len = 100;
            RvRtspSessionGetId(pConnection->sessions[i]->hSession, &hStr);
            RvRtspStringHandleGetString(pTest->hRtsp, hStr, str, &len);

            TclExecute("test:UpdateSessionList {%d} {%p} {%s} {%s} {%s}",
                pConnection->sessions[i]->index,
                pConnection->sessions[i],
                str,
                pConnection->sessions[i]->strURI,
                pConnection->sessions[i]->state);
        }
    }
}


/******************************************************************************
 * TestParseAndStoreAdditionalHeader
 * ----------------------------------------------------------------------------
 * General:
 *  Parses and the additional header record from the GUI and populates them
 *  into the Generic Header structure.
 *
 * Arguments:
 * Input:  strHeader  - records of additional headers from the GUI.
 *
 * Output: None
 *
 * Return Value: RV_OK if successful.
 *               Negative value otherwise.
 *****************************************************************************/
RvStatus TestParseAndStoreAdditionalHeader(
                              IN    RvChar           *strHeader)
{
    char                   line[20][200];
    char                   *token;
    char                   *token1;
    RvStatus               status;
    char                   argList[10][FIELDVAL_LEN];
    int                    argCount;
    int                    j;

    int numLines = 0, idx=0;
    int numHeadFields = 0;
    status = RV_OK;

    pTest->numRecords = 0;

    memset (line,'\0',sizeof(line));
    memset(pTest->genericMsgRec,0,sizeof(GenericMsgRec)*TEST_MAX_HEADERS_ALLOWED);
    memset(argList, 0, sizeof(argList));

    if (strcmp(strHeader,"")==0)
    {
       return RV_OK;
    }

    token = strtok(strHeader,"}");

    /* retrieve each input record line */
    while (token != NULL)
    {
       strcpy(line[numLines],token);
       numLines++;
       token = strtok(NULL,"}");
    }

    /* Strip the '{' char from each line */
    for (idx=0; idx < numLines; idx++)
    {
        char *str = line[idx];

        if(strstr(line[idx]," {") != NULL)
            str +=2;
        else
            str++;

        strcpy(line[idx],str);
    }

    pTest->numRecords = numLines;

    /* each input record line, break it into method, header, and field/value pairs */
    for(j=0; j < numLines; j++)
    {
       argCount      = 0;
       numHeadFields = 0;

       /* Init */
       memset(pTest->genericMsgRec[j].fieldvalue1, '\0', FIELDVAL_LEN);
       memset(pTest->genericMsgRec[j].fieldvalue2, '\0', FIELDVAL_LEN);
       memset(pTest->genericMsgRec[j].fieldvalue3, '\0', FIELDVAL_LEN);

       token1 = strtok(line[j], " ");

       while (token1 != NULL)
       {
          strcpy(argList[argCount],token1);
          argCount++;
          token1 = strtok(NULL," ");
       }

       /* Example input: */
       strcpy(pTest->genericMsgRec[j].method, argList[0]);
       strcpy(pTest->genericMsgRec[j].header, argList[1]);
       pTest->genericMsgRec[j].delim = argList[argCount-1][0];

       for (idx=2; idx < argCount -1; idx++)
       {
          if (idx == 2)
            strcpy(pTest->genericMsgRec[j].fieldvalue1, argList[idx]);

          if (idx == 3)
            strcpy(pTest->genericMsgRec[j].fieldvalue2, argList[idx]);

          if (idx == 4)
            strcpy(pTest->genericMsgRec[j].fieldvalue3, argList[idx]);

           numHeadFields++;
       }

       pTest->genericMsgRec[j].numHeadFields = numHeadFields;
    }

    return status;
}

/******************************************************************************
 * TestAddAdditionalHeader
 * ----------------------------------------------------------------------------
 * General:
 *  Adds the Generic messages to the outgoing message after they are
 *  populated in the structure.
 *
 * Arguments:
 * Input:  bIsRequest - Boolean indicating whether message is a Request or a
 *                      Response.
 *         pResponse  - response to be updated with the Generic Messages.
 *         pRequest   - request to be updated with the Generic Messages.
 * Output: None
 *
 * Return Value: RV_OK if successful.
 *               Negative value otherwise.
 ******************************************************************************/
RvStatus TestAddAdditionalHeader(
                         IN      RvBool         bIsRequest,
                         IN      RvRtspResponse *pResponse,
                         IN      RvRtspRequest  *pRequest)
{

    RvRtspMsgAppHeader     msgHeader;
    RvRtspHeaderName       headerName;
    RvChar                 *headerFields[3];
    RvStatus               status;
    char                   methodName[50];
    RvUint j;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvBool bIsAdded = RV_FALSE;

    RvBool acceptDone = RV_FALSE;
    RvBool connDone = RV_FALSE;
    RvBool sessionDone = RV_FALSE;
    RvBool transportDone = RV_FALSE;
    RvBool rangeDone = RV_FALSE;
    RvBool locDone = RV_FALSE;
    RvBool requireDone = RV_FALSE;
    RvBool rtpInfoDone = RV_FALSE;
    RvBool publicDone = RV_FALSE;
    RvBool contLenDone = RV_FALSE;
    RvBool contBaseDone = RV_FALSE;
    RvBool contLangDone = RV_FALSE;
    RvBool contTypeDone = RV_FALSE;
    RvBool contEncDone = RV_FALSE;
#endif

    status = RV_OK;
    memset(methodName, 0, sizeof(methodName));

    if(bIsRequest == RV_TRUE)
    {
        if(pRequest != NULL)
        {
            strcpy(methodName,method[pRequest->requestLine.method]);
        }
    }
    else if(bIsRequest == RV_FALSE)
    {
        if(pResponse != NULL)
        {
            strcpy(methodName,method[pTest->rtspMethod]);
        }
    }
    else
    {
        status = RV_ERROR_BADPARAM;
        return status;
    }

for(j=0; j < pTest->numRecords; j++)
    {
       if (STRCASECMP(pTest->genericMsgRec[j].method, methodName) != 0)
           continue;

       headerFields[0] = (RvChar *)malloc(256);
       headerFields[1] = (RvChar *)malloc(256);
       headerFields[2] = (RvChar *)malloc(256);

       memset(headerFields[0], 0, 256);
       memset(headerFields[1], 0, 256);
       memset(headerFields[2], 0, 256);

       msgHeader.headerFields = headerFields;
       msgHeader.bIsRequest = bIsRequest;

       /* Copy only those field/value pairs entered in the GUI */
       if(pTest->genericMsgRec[j].fieldvalue1 != NULL)
       {
             memcpy(headerFields[0], pTest->genericMsgRec[j].fieldvalue1, strlen(pTest->genericMsgRec[j].fieldvalue1));
       }

       if(pTest->genericMsgRec[j].fieldvalue2 != NULL)
       {
             memcpy(headerFields[1], pTest->genericMsgRec[j].fieldvalue2, strlen(pTest->genericMsgRec[j].fieldvalue2));
       }

       if(pTest->genericMsgRec[j].fieldvalue3 != NULL)
       {
             memcpy(headerFields[2], pTest->genericMsgRec[j].fieldvalue3, strlen(pTest->genericMsgRec[j].fieldvalue3));
       }

       msgHeader.delimiter         = pTest->genericMsgRec[j].delim;
       msgHeader.headerFieldStrLen = 255;
       msgHeader.headerFieldsSize  = pTest->genericMsgRec[j].numHeadFields;
       msgHeader.headerName        = pTest->genericMsgRec[j].header;
       msgHeader.headerNameLen     = strlen(pTest->genericMsgRec[j].header);

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
       if (bIsRequest == RV_TRUE)
       {
           msgHeader.hRtspMsgMessage = pRequest->hRtspMsgMessage;
           if (STRCASECMP(msgHeader.headerName,"Accept") == 0)
           {
               pRequest->acceptValid = RV_TRUE;
               if (acceptDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pRequest->accept.additionalFields);
                   acceptDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pRequest->accept.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"Connection") == 0)
           {
               pRequest->connectionValid = RV_TRUE;
               if (connDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pRequest->connection.additionalFields);
                   connDone  = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pRequest->connection.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"Session") == 0)
           {
               pRequest->sessionValid = RV_TRUE;
               if (sessionDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pRequest->session.additionalFields);
                   sessionDone  = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pRequest->session.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"Transport") == 0)
           {
               pRequest->transportValid = RV_TRUE;
               if (transportDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pRequest->transport.additionalFields);
                   transportDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pRequest->transport.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"Range") == 0)
           {
               pRequest->rangeValid = RV_TRUE;
               if (rangeDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pRequest->range.additionalFields);
                   rangeDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pRequest->range.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"Location") == 0)
           {
               pRequest->locationValid = RV_TRUE;
               if (locDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pRequest->location.additionalFields);
                   locDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pRequest->location.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"Require") == 0)
           {
               pRequest->requireValid = RV_TRUE;
               if (requireDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pRequest->require.additionalFields);
                   requireDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pRequest->require.additionalFields, &msgHeader);
           }
           else
           {
               memset(&headerName, 0, sizeof(headerName));
               headerName.headerName = pTest->genericMsgRec[j].header;

               status = RvRtspMsgGetHeaderByName(Test.hRtsp, pRequest->hRtspMsgMessage, RV_TRUE, &headerName);
               if ((bIsAdded == RV_FALSE) || (headerName.hHeader == NULL))
               {
                   status = RvRtspMsgAddHeader(Test.hRtsp, &msgHeader);
                   bIsAdded = RV_TRUE;
               }
               else
               {
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, headerName.hHeader, &msgHeader);
               }
           }
      }
      else
      {
           msgHeader.hRtspMsgMessage = pResponse->hRtspMsgMessage;
           if (STRCASECMP(msgHeader.headerName,"Connection") == 0)
           {
               pResponse->connectionValid = RV_TRUE;
               if (connDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pResponse->connection.additionalFields);
                   connDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pResponse->connection.additionalFields, &msgHeader);

           }
           else if (STRCASECMP(msgHeader.headerName,"Session") == 0)
           {
               pResponse->sessionValid = RV_TRUE;
               if (sessionDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pResponse->session.additionalFields);
                   sessionDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pResponse->session.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"Transport") == 0)
           {
               pResponse->transportValid = RV_TRUE;
               if (transportDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pResponse->transport.additionalFields);
                   transportDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pResponse->transport.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"RtpInfo") == 0)
           {
               pResponse->rtpInfoValid = RV_TRUE;
               if (rtpInfoDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pResponse->rtpInfo.additionalFields);
                   rtpInfoDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pResponse->rtpInfo.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"Public") == 0)
           {
               pResponse->publicHdrValid = RV_TRUE;
               if (publicDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pResponse->publicHdr.additionalFields);
                   publicDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pResponse->publicHdr.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"ContentLength") == 0)
           {
               pResponse->contentLengthValid = RV_TRUE;
               if (contLenDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pResponse->contentLength.additionalFields);
                   contLenDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pResponse->contentLength.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"ContentBase") == 0)
           {
               pResponse->contentBaseValid = RV_TRUE;
               if (contBaseDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pResponse->contentBase.additionalFields);
                   contBaseDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pResponse->contentBase.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"ContentLanguage") == 0)
           {
               pResponse->contentLanguageValid = RV_TRUE;
               if (contLangDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pResponse->contentLanguage.additionalFields);
                   contLangDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pResponse->contentLanguage.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"ContentType") == 0)
           {
               pResponse->contentTypeValid = RV_TRUE;
               if (contTypeDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pResponse->contentType.additionalFields);
                   contTypeDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pResponse->contentType.additionalFields, &msgHeader);
           }
           else if (STRCASECMP(msgHeader.headerName,"ContentEncoding") == 0)
           {
               pResponse->contentEncodingValid = RV_TRUE;
               if (contEncDone == RV_FALSE)
               {
                   status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pResponse->contentEncoding.additionalFields);
                   contEncDone = RV_TRUE;
               }
               else
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pResponse->contentEncoding.additionalFields, &msgHeader);
           }
           else
           {
               memset(&headerName, 0, sizeof(headerName));
               headerName.headerName = pTest->genericMsgRec[j].header;

               status = RvRtspMsgGetHeaderByName(Test.hRtsp, pResponse->hRtspMsgMessage, RV_FALSE, &headerName);
               if ((bIsAdded == RV_FALSE) || (headerName.hHeader == NULL))
               {
                   status = RvRtspMsgAddHeader(Test.hRtsp, &msgHeader);
                   bIsAdded = RV_TRUE;
               }
               else
               {
                   status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, headerName.hHeader, &msgHeader);
               }
           }
      }
#endif /* RV_RTSP_USE_RTSP_MSG */

    free(headerFields[0]);
    free(headerFields[1]);
    free(headerFields[2]);
   }

    return status;
}

/*-----------------------------------------------------------------------*/
/*                      THREAD FUNCTION DEFINITIONS                      */
/*-----------------------------------------------------------------------*/

/**************************************************************************
 * TestHandleReceivedMessagee
 * ------------------------------------------------------------------------
 * General:
 * This function updates the GUI variables and prints the received messages
 * including the additional headers with the message on the GUI message
 * window.
 * Arguments:
 * Input:  hMessage   - Message type.
 *         bIsRequest - determines whether message received is request or
 *                      response.
 * Inout:  pStr       - string to be displayed on the message window.
 *
 * Return Value: None
 *************************************************************************/
void TestHandleReceivedMessage(
    IN     RvRtspMsgMessageHandle  hMessage,
    IN     RvBool                  bIsRequest,
    INOUT  RvChar                  *pStr)
{
    RvUint32            i;
    RvStatus            status;
    RvRtspHeaderName    headerNames[TEST_MAX_HEADERS_ALLOWED];

    RvUint32            numHeaders = TEST_MAX_HEADERS_ALLOWED;

    for (i = 0; i < TEST_MAX_HEADERS_ALLOWED; ++i)
    {
        /* Allocate space for header names and initialize header name objects */
        headerNames[i].headerName = (RvChar *)malloc(TEST_HEADER_NAME_SIZE + 1);
        headerNames[i].headerNameLen = TEST_HEADER_NAME_SIZE;
        memset(headerNames[i].headerName, 0, TEST_HEADER_NAME_SIZE + 1);
    }
    /* Get all the header names in the message */
    status = RvRtspMsgGetMessageHeaderNames(Test.hRtsp, hMessage, bIsRequest, &numHeaders, headerNames);

    if (status == RV_OK)
    {
        RvChar              *fieldsBuffer[TEST_MAX_HEADER_FIELDS_ALLOWED];
        RvRtspMsgAppHeader  appHeader;

        /* Allocate memory for header fields */
        for (i = 0; i < TEST_MAX_HEADER_FIELDS_ALLOWED; ++i)
        {
            fieldsBuffer[i] = (RvChar *)malloc(TEST_MAX_HEADER_FIELD_VALUE_SIZE + 1);
        }
        for (i = 0; i < numHeaders; ++i)
        {
            RvUint32    j;

            /* Set appHeader */
            sprintf(pStr,"test:Log {");
            while(*pStr != '\0')
                  pStr++;
            memset(&appHeader, 0, sizeof (RvRtspMsgAppHeader));
            /* Allocate the memory for the name */
            appHeader.headerName        = (RvChar *)malloc(TEST_HEADER_NAME_SIZE + 1);
            appHeader.headerNameLen     = TEST_HEADER_NAME_SIZE;
            appHeader.bIsRequest        = bIsRequest;
            appHeader.headerFields      = fieldsBuffer;
            appHeader.headerFieldsSize  = TEST_MAX_HEADER_FIELDS_ALLOWED;
            appHeader.headerFieldStrLen = TEST_MAX_HEADER_FIELD_VALUE_SIZE;
            appHeader.hRtspMsgMessage   = hMessage;

            for (j = 0; j < TEST_MAX_HEADER_FIELDS_ALLOWED; ++j)
            {
                memset (fieldsBuffer[j], 0, TEST_MAX_HEADER_FIELD_VALUE_SIZE + 1);
            }

            sprintf(pStr, "%s: ", headerNames[i].headerName);
            while(*pStr != '\0')
                  pStr++;

            /* This is not necessary because we already have the header handle - just for the sake of using an API */
            headerNames[i].hHeader = NULL;
            RvRtspMsgGetHeaderByName(Test.hRtsp, hMessage, bIsRequest, &headerNames[i]);
            /* Get the values of the header fields */
            RvRtspMsgGetHeaderFieldValues(Test.hRtsp, headerNames[i].hHeader, &appHeader);
            for (j = 0; j < appHeader.headerFieldsSize; ++j)
            {
                if(j+1 == appHeader.headerFieldsSize)
                {
                    sprintf(pStr, "%s", appHeader.headerFields[j]); /* Last field of the header */
                    while(*pStr != '\0')
                          pStr++;
                }
                else
                {
                    sprintf(pStr, "%s;", appHeader.headerFields[j]);
                    while(*pStr != '\0')
                          pStr++;
                }
            }
            sprintf(pStr,"};");
            while(*pStr != '\0')
                      pStr++;
            free(appHeader.headerName);
        }

        for (i = 0; i < TEST_MAX_HEADER_FIELDS_ALLOWED; ++i)
        {
            free(fieldsBuffer[i]);
        }
    }
    for (i = 0; i < TEST_MAX_HEADERS_ALLOWED; ++i)
    {
        free(headerNames[i].headerName);
    }
}

/**************************************************************************
 * TestPrintSentMessage
 * ------------------------------------------------------------------------
 * General:
 * This function updates the GUI variables and prints the sent message
 * on the GUI message window
 * Arguments:
 * Input:  eMsgType   - Message type.
 *         pRequest   - request received.
 *         pResponse  - response received.
 * Output: pStr       - string to be displayed onthe message window.
 *************************************************************************/
void TestPrintSentMessage(
    IN RvBool         bIsRequest,
    IN RvRtspRequest  *pRequest,
    IN RvRtspResponse *pResponse,
    OUT RvChar        *pStr )
{
    RvChar     temp[500];
    RvChar     addressType[20];
    RvUint     len = LOG_MESSAGE_BUFFER_LENGTH;
    RvUint     i   = 0;

    memset(temp,'\0',500);

    if (bIsRequest == RV_TRUE)
    {
        if(pRequest->cSeqValid == RV_TRUE)
        {
            sprintf(pStr,"test:Log {CSeq: %d};",pRequest->cSeq.value);
            while(*pStr!= '\0')
                  pStr++;
        }
        if(pRequest->acceptValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pRequest->accept.hStr,temp,&len);
            sprintf(pStr,"test:Log {Accept: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pRequest->sessionValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pRequest->session.hSessionId,temp,&len);
            sprintf(pStr,"test:Log {Session: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pRequest->rangeValid == RV_TRUE)
        {
            sprintf(pStr,"test:Log {Range: npt=%d.%d.%f-%d.%d.%f};",pRequest->range.startTime.hours,pRequest->range.startTime.minutes,pRequest->range.startTime.seconds,pRequest->range.endTime.hours,pRequest->range.endTime.minutes,pRequest->range.endTime.seconds);
            while(*pStr!= '\0')
                  pStr++;
        }
        if(pRequest->locationValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pRequest->location.hStr,temp,&len);
            sprintf(pStr,"test:Log {Location: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pRequest->connectionValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pRequest->connection.hStr,temp,&len);
            sprintf(pStr,"test:Log {Connection: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pRequest->requireValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pRequest->require.hStr,temp,&len);
            sprintf(pStr,"test:Log {Require: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pRequest->transportValid == RV_TRUE)
        {
            if(pRequest->transport.isUnicast == RV_TRUE)
            {
               strcpy(addressType,"unicast");
            }
            else
            {
               strcpy(addressType,"multicast");
            }
            sprintf(pStr,"test:Log {Transport: %s;Client Port:%d-%d",addressType,pRequest->transport.clientPortA,pRequest->transport.clientPortB);
            while(*pStr!= '\0')
                  pStr++;
            if(strlen(pRequest->transport.destination)!= 0)
            {
                sprintf(pStr,";Destination:%s};",pRequest->transport.destination);
                while(*pStr!= '\0')
                      pStr++;
            }
            else
            {
                sprintf(pStr,"};");
                while(*pStr!= '\0')
                      pStr++;
            }
        }
        if(pTest->numRecords != 0)
        {
            for(i=0; i < pTest->numRecords; i++)
            {
                if(STRCASECMP(pTest->genericMsgRec[i].method,method[pRequest->requestLine.method])!= 0)
                {
                    continue;
                }
                else
                {
                    sprintf(pStr,"test:Log {");
                    while(*pStr!= '\0')
                          pStr++;
                    if(strlen(pTest->genericMsgRec[i].header) != 0)
                    {
                        sprintf(pStr,"%s:",pTest->genericMsgRec[i].header);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    if((strlen(pTest->genericMsgRec[i].fieldvalue1) != 0) && (strlen(pTest->genericMsgRec[i].fieldvalue2) || strlen(pTest->genericMsgRec[i].fieldvalue3)) != 0)
                    {
                        sprintf(pStr,"%s%c",pTest->genericMsgRec[i].fieldvalue1,pTest->genericMsgRec[i].delim);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    else
                    {
                        sprintf(pStr,"%s",pTest->genericMsgRec[i].fieldvalue1);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    if((strlen(pTest->genericMsgRec[i].fieldvalue2) != 0) && (strlen(pTest->genericMsgRec[i].fieldvalue3)) != 0)
                    {
                        sprintf(pStr,"%s%c",pTest->genericMsgRec[i].fieldvalue2,pTest->genericMsgRec[i].delim);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    else
                    {
                        sprintf(pStr,"%s",pTest->genericMsgRec[i].fieldvalue2);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    if(strlen(pTest->genericMsgRec[i].fieldvalue3) != 0)
                    {
                        sprintf(pStr,"%s",pTest->genericMsgRec[i].fieldvalue3);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    sprintf(pStr,"};");
                    while(*pStr!='\0')
                          pStr++;
                }
            }
        }

    }
    else if(bIsRequest == RV_FALSE)
    {
        if(pResponse->cSeqValid == RV_TRUE)
        {
            sprintf(pStr,"test:Log {CSeq: %d};",pResponse->cSeq.value);
            while(*pStr!= '\0')
                  pStr++;
        }
        if(pResponse->connectionValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pResponse->connection.hStr,temp,&len);
            sprintf(pStr,"test:Log {Connection: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pResponse->sessionValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pResponse->session.hSessionId,temp,&len);
            sprintf(pStr,"test:Log {Session: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pResponse->transportValid == RV_TRUE)
        {
            if(pResponse->transport.isUnicast == RV_TRUE)
            {
               strcpy(addressType,"unicast");
            }
            else
            {
               strcpy(addressType,"multicast");
            }
            sprintf(pStr,"test:Log {Transport: %s;Client Port:%d-%d",addressType,pResponse->transport.clientPortA,pResponse->transport.clientPortB);
            while(*pStr!= '\0')
                  pStr++;
            if(strlen(pResponse->transport.destination)!= 0)
            {
                sprintf(pStr,";Destination:%s};",pResponse->transport.destination);
                while(*pStr!= '\0')
                      pStr++;
            }
            else
            {
                sprintf(pStr,"};");
                while(*pStr!= '\0')
                      pStr++;
            }
        }
        if(pResponse->contentLanguageValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pResponse->contentLanguage.hStr, temp, &len);
            sprintf(pStr,"test:Log {Content Language: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pResponse->contentEncodingValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pResponse->contentEncoding.hStr , temp, &len);
            sprintf(pStr,"test:Log {Content Encoding: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pResponse->contentBaseValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pResponse->contentBase.hStr , temp, &len);
            sprintf(pStr,"test:Log {Content Base: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pResponse->contentTypeValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pResponse->contentType.hStr , temp, &len);
            sprintf(pStr,"test:Log {Content Type: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pResponse->contentLengthValid == RV_TRUE)
        {
            sprintf(pStr,"test:Log {Content Length: %d};",pResponse->contentLength.value);
            while(*pStr!= '\0')
                  pStr++;
        }
        if(pResponse->rtpInfoValid == RV_TRUE)
        {
            RvSize_t         arraySize;
            RvStatus         result;
            RvRtspRtpInfo    *pArrayElem;

            RvRtspArraySize(pTest->hRtsp, pResponse->rtpInfo.hInfo, &arraySize);
            result = RvRtspArrayGetFirst(pTest->hRtsp,
                                         pResponse->rtpInfo.hInfo,
                                         (void**)&pArrayElem);
            while(result == RV_OK)
            {
                RvRtspStringHandleGetString(pTest->hRtsp, pArrayElem->hURI, temp, &len);

                sprintf(pStr,"test:Log {RTP-Info: url=%s;", temp);
                while(*pStr!= '\0')
                      pStr++;

                if (pArrayElem->seqValid)
                {
                    sprintf(pStr,"seq=%d;", pArrayElem->seq);
                    while(*pStr!= '\0')
                          pStr++;
                }

                if (pArrayElem->rtpTimeValid)
                {
                    sprintf(pStr,"rtptime=%d};", pArrayElem->rtpTime);
                    while(*pStr!= '\0')
                          pStr++;
                }
                else
                {
                    sprintf(pStr,"};");
                    while(*pStr!= '\0')
                          pStr++;
                }

                result = RvRtspArrayGetNext(pTest->hRtsp,
                                            pResponse->rtpInfo.hInfo,
                                           (void*)pArrayElem,
                                           (void**)&pArrayElem);
                len = LOG_MESSAGE_BUFFER_LENGTH;
            }
        }
        if(pResponse->publicHdrValid == RV_TRUE)
        {
            RvRtspStringHandleGetString(pTest->hRtsp,pResponse->session.hSessionId,temp,&len);
            sprintf(pStr,"test:Log {Session: %s};",temp);
            while(*pStr!= '\0')
                  pStr++;
            len = LOG_MESSAGE_BUFFER_LENGTH;
        }
        if(pTest->numRecords != 0)
        {
            for(i=0; i < pTest->numRecords; i++)
            {
                if(STRCASECMP(pTest->genericMsgRec[i].method,method[pTest->rtspMethod])!= 0)
                {
                    continue;
                }
                else
                {
                    sprintf(pStr,"test:Log {");
                    while(*pStr!= '\0')
                          pStr++;
                    if(strlen(pTest->genericMsgRec[i].header) != 0)
                    {
                        sprintf(pStr,"%s:",pTest->genericMsgRec[i].header);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    if((strlen(pTest->genericMsgRec[i].fieldvalue1) != 0) && (strlen(pTest->genericMsgRec[i].fieldvalue2) || strlen(pTest->genericMsgRec[i].fieldvalue3)) != 0)
                    {
                        sprintf(pStr,"%s%c",pTest->genericMsgRec[i].fieldvalue1,pTest->genericMsgRec[i].delim);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    else
                    {
                        sprintf(pStr,"%s",pTest->genericMsgRec[i].fieldvalue1);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    if((strlen(pTest->genericMsgRec[i].fieldvalue2) != 0) && (strlen(pTest->genericMsgRec[i].fieldvalue3)) != 0)
                    {
                        sprintf(pStr,"%s%c",pTest->genericMsgRec[i].fieldvalue2,pTest->genericMsgRec[i].delim);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    else
                    {
                        sprintf(pStr,"%s",pTest->genericMsgRec[i].fieldvalue2);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    if(strlen(pTest->genericMsgRec[i].fieldvalue3) != 0)
                    {
                        sprintf(pStr,"%s",pTest->genericMsgRec[i].fieldvalue3);
                        while(*pStr!='\0')
                              pStr++;
                    }
                    sprintf(pStr,"};");
                    while(*pStr!='\0')
                          pStr++;
                }
            }
        }
    }
}


/******************************************************************************
 * LogFileReset
 * ----------------------------------------------------------------------------
 * General: Reset the log file.
 *
 * Return Value: RV_OK on success, negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  mg               - Gateway object to use.
 * Output: None.
 *****************************************************************************/
RvStatus LogFileReset()
{
    if (pTest == NULL) return RV_OK;

    /* remove file listener */
    RvLogListenerDestructLogfile(&pTest->logFile);

    /* Construct the file listener */
    RvLogListenerConstructLogfile(&pTest->logFile, &pTest->logMgr, "rtspClientLog.txt", 1, 0, RV_FALSE);

    return RV_OK; /* TODO: check function outputs */
}


#ifdef __cplusplus
}
#endif

