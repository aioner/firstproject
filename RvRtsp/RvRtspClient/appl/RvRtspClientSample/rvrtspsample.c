/*
*********************************************************************************
*                                                                               *
* NOTICE:                                                                       *
* This document contains information that is confidential and proprietary to    *
* RADVision LTD.. No part of this publication may be reproduced in any form     *
* whatsoever without written prior approval by RADVision LTD..                  *
*                                                                               *
* RADVision LTD. reserves the right to revise this publication and make changes *
* without obligation to notify any person of such revisions or changes.         *
*********************************************************************************
*/


/*********************************************************************************
 *                              <rvrtspsample.c>
 *
 *  Sample program, showing usage of the RTSP stack, the program initializes
 *	the module, opens connections and communicates with the server.
 *
 *    Author                         Date
 *    ------                        ------
 *		Shaft						2/1/04
 *		Shaft						15/05/2006 - changed to fit windows CE sample test.
 *********************************************************************************/


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILED                           */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rvtypes.h"
#include "rvrtspsample.h"
#include "RvRtspClientInc.h"
#include "rvadtstamp.h"
#include "rvloglistener.h"
#include "rvsdp.h"
#if (RV_OS_TYPE != RV_OS_TYPE_WIN32)
#include <unistd.h>/* Linux/Unix */
#endif


/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/

#define DNS_SERVER_ADDRESS "172.16.9.1"

#define TEST_MAX_DNS_RESULTS_ALLOWED		5
#define TEST_MAX_CONNECTIONS_ALLOWED        2
#define TEST_MAX_CONNECTION_RECONNECT       4
#define TEST_MAX_TRACKS_PER_CONNECTION      2
#define TEST_MAX_TRACK_SIZE                 256
#define TEST_MAX_SESSIONS_ALLOWED           20
#define TEST_MAX_HEADERS_ALLOWED			20
#define TEST_MAX_URL_IN_MSG_ALLOWED			4
#define TEST_MAX_DESCRIBE_REQUESTS_ALLOWED	10
#define TEST_MAX_TX_QUEUE_SIZE_ALLOWED		50
#define TEST_HEADER_NAME_SIZE               20
#define TEST_MAX_HEADER_FIELDS_ALLOWED      10
#define TEST_MAX_HEADER_FIELD_VALUE_SIZE    50

#define PRINT_BUFFER_LENGTH					6000
#define CLIENT_IP_ADDRESS                   "172.16.9.233"
#define IP_ADDR_LEN                         50
#define RTP_PACKET_LEN                      1554
#define RTP_FILE                            "rtp_receive_data"
char*   uriArray[TEST_MAX_CONNECTIONS_ALLOWED] =
{
    "rtsp://172.16.9.233:1554/0"

    //"rtsp://172.16.6.221/2.mpg"
	//"rtsp://172.16.2.110:8557/PSIA/Streaming/channels/2?videoCodecType=H.264"
 /*   "rtsp://172.16.70.21/balloon1.mpg"
	"rtsp://real1.itc.u-tokyo.ac.jp/Lecture/Okabe/qe1-1.rm",
    "rtsp://real.cctv.com.cn/dspp/dannishaonian08.rm" */
};

char trackArray[TEST_MAX_CONNECTIONS_ALLOWED][TEST_MAX_TRACKS_PER_CONNECTION][TEST_MAX_TRACK_SIZE];

#ifdef USE_RTP
RvRtpSession  ghRTP;         /* RTP session handle */
#endif
RvBool        gReceiveRTP;    /* Whether RTP packets should be streamed */
RvInt16       gServerPortA;
RvInt16       gServerPortB;
FILE          *fp;
/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/


/* TestRtspStackStruct
 * ----------
 * This structure holds the data relevant for an RTSP stack.
 */
typedef struct
{
	RvRtspHandle		hRtsp;				/* the RTSP stack				*/
	RvRtspConfiguration	rtspConfiguration;	/* the stack's configuration	*/

	RvRtspConnectionHandle				hConnections[TEST_MAX_CONNECTIONS_ALLOWED];
											/* holds the stack's connections*/
    RvUint32                            numReconnects[TEST_MAX_CONNECTIONS_ALLOWED];
                                            /* Holds the number of reconnect failures */
	RvRtspConnectionConfiguration		connectionConfiguration;
											/* the connections configuration*/
	RvRtspConnectionCallbackFunctions	connectionCallbacks;
											/* the connections callbacks 	*/

	RvRtspSessionHandle					hSessions[TEST_MAX_CONNECTIONS_ALLOWED];
											/* holds the stack's sessions	*/
	RvRtspSessionConfiguration			sessionConfiguration;
											/* the sessions configuration	*/
	RvRtspSessionCallbackFunctions		sessionCallbackFunctions;
											/* the sessions callbacks		*/
#if RV_LOGMASK != RV_LOGLEVEL_NONE
    RvLogMgr                            logMgr;
    RvLogListener                       logFile;
#endif
} TestRtspStackStruct;

/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/


int					stop		= 0;    /* stop the stack       */
TestRtspStackStruct *pTest		= NULL;
TestRtspStackStruct	Test;
RvBool				firstTime	= RV_TRUE;
int					sleepCycles	= 50;
printCB				pfnPrintCB	= NULL;

char				printOut[2500];

/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/



/**************************************************************************
 * TestConnectionOnConnectEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when the connection is
 *			established (or connection failed).
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection	- the connection.
 *				hApp		- application context.
 *				success		- has the connection been established.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnConnectEv(
								IN RvRtspConnectionHandle		hConnection,
								IN RvRtspConnectionAppHandle	hApp,
								IN RvBool						success);


/**************************************************************************
 * TestConnectionOnDisconnectEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when the connection is
 *			disconnected.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection	- the connection.
 *				hApp		- application context.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnDisconnectEv(
								IN RvRtspConnectionHandle		hConnection,
								IN RvRtspConnectionAppHandle	hApp);


/**************************************************************************
 * TestConnectionOnErrorEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when an error message is
 *			received on the connection.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- the connection.
 *				hApp			- application context.
 *				hURI			- the URI on which the request was made.
 *				requestMethod	- the request method.
 *				status			- the response status.
 *				hPhrase			- the response phrase.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnErrorEv(
								IN	RvRtspConnectionHandle		hConnection,
								IN	RvRtspConnectionAppHandle	hApp,
								IN	RvRtspStringHandle			hURI,
								IN	RvRtspMethod				requestMethod,
								IN	RvRtspStatus				status,
								IN	RvRtspStringHandle			hPhrase);

/**************************************************************************
 * TestConnectionOnErrorExtEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when an error message is
 *			received on the connection.
 *          The same as TestConnectionOnErrorEv except that raise the describe
 *          application handle and the received response to the application.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- The Connection on which the error occurred.
 *				hApp			- application context.
 *              hDescribe       - The application handle for the failed describe request
 *                                or NULL if the error is not on describe request.
 *				hURI			- The URI requested in the message that
 *								  caused the error.
 *				requestMethod	- The requested method.
 *				status			- The response status.
 *				phrase			- The response phrase.
 *				pResponse	    - the received response.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnErrorExtEv(
								IN	RvRtspConnectionHandle		        hConnection,
								IN	RvRtspConnectionAppHandle	        hApp,
                                IN  RvRtspConnectionDescribeAppHandle   hDescribe,
								IN	RvRtspStringHandle			        hURI,
								IN	RvRtspMethod				        requestMethod,
								IN	RvRtspStatus				        status,
								IN	RvRtspStringHandle			        hPhrase,
								IN	const RvRtspResponse		        *pResponse);

/**************************************************************************
 * TestConnectionOnDescribeResponseEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when a Describe response is
 *			received on the connection.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection			- the connection.
 *				hApp				- application context.
 *				pDescribeResponse	- the Describe response.
 *				hURI				- the URI on which the request was made.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnDescribeResponseEv(
								IN	RvRtspConnectionHandle				hConnection,
								IN	RvRtspConnectionAppHandle			hApp,
								IN	RvRtspConnectionDescribeAppHandle	hDescribe,
								IN	const RvRtspResponse				*pDescribeResponse,
								IN	RvRtspStringHandle					hURI);


/**************************************************************************
 * TestConnectionOnRedirectRequestEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when a Redirect request is
 *			received on the connection.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- the connection.
 *				hApp			- application context.
 *				pRequest		- the Redirect request.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnRedirectRequestEv(
								IN	RvRtspConnectionHandle		hConnection,
								IN  RvRtspConnectionAppHandle	hApp,
								IN	const RvRtspRequest			*pRequest);



/**************************************************************************
 * TestSessionOnStateChangeEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when the session's state is
 *			changed (due to a response message received for the session).
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession	- the session.
 *				hApp		- application context.
 *				currState	- the current session state.
 *				newState	- the new session state.
 *				pResponse	- the received response.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestSessionOnStateChangeEv(
						IN	RvRtspSessionHandle		hSession,
						IN	RvRtspSessionAppHandle	hApp,
						IN	RvRtspSessionState		currState,
						IN	RvRtspSessionState		newState,
						IN	const RvRtspResponse	*pResponse);



/**************************************************************************
 * TestSessionOnErrorEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when an error response is
 *			received for the session.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession		- the session.
 *				hApp			- application context.
 *				requestMethod	- the request for which the error occurred.
 *				status			- the returned status.
 *				hPhrase			- the status phrase.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestSessionOnErrorEv(
						IN	RvRtspSessionHandle		hSession,
						IN	RvRtspSessionAppHandle	hApp,
						IN	RvRtspMethod			requestMethod,
						IN	RvRtspStatus			status,
						IN	RvRtspStringHandle		hPhrase);


#ifdef USE_RTP
/**************************************************************************
 * TestStartRtp
 * ------------------------------------------------------------------------
 * General:
 *  This function initializes the RTP/RTCP stack, and sets the IP/Port
 *  parameters to enable sending and receiving RTP/RTCP packets
 *
 * Arguments:
 * Input :  None.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative Values otherwise.
 *************************************************************************/
RvStatus TestStartRtp (void);

/**************************************************************************
 * TestStopRtp
 * ------------------------------------------------------------------------
 * General:
 *  This function stops the streaming of RTP/RTCP packets from the server
 *
 * Arguments:
 * Input :  None.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative Values otherwise.
 *************************************************************************/
RvStatus TestStopRtp (void);

/**************************************************************************
 * TestReceiveRtp
 * ------------------------------------------------------------------------
 * General:
 *  This function sends a RTP/RTCP packet from the server to client
 *
 * Arguments:
 * Input:   None.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative Values otherwise.
 *************************************************************************/
RvStatus TestReceiveRtp (void);
#endif /* USE_RTP */


/**************************************************************************
 * TestRtspConnectionsConstruct
 * ------------------------------------------------------------------------
 * General: internal function, used to construct connection over an RTSP stack
 *			connections.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pTest	- structure holding the stack related variables.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void	TestRtspConnectionsConstruct(
					IN TestRtspStackStruct	*pTest);


/**************************************************************************
 * TestRtspConnectionsConnect
 * ------------------------------------------------------------------------
 * General: internal function, used to connect the connection objects to
 *          the server.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pTest	- structure holding the stack related variables.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void	TestRtspConnectionsConnect(
                    IN TestRtspStackStruct	*pTest);


/**************************************************************************
 * TestRtspConnectionsDisconnect
 * ------------------------------------------------------------------------
 * General: internal function, used to disconnect the connection objects from
 *          the server.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pTest	- structure holding the stack related variables.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void	TestRtspConnectionsDisconnect(
                    IN TestRtspStackStruct	*pTest);


/**************************************************************************
 * TestRtspSessionConstruct
 * ------------------------------------------------------------------------
 * General: constructs the stack's sessions and sets the URIs according to
 *			the streamURI array.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pTest	- structure holding the stack related variables.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void	TestRtspSessionConstruct(
					IN int iConnection);


/**************************************************************************
 * TestRtspSessionDestruct
 * ------------------------------------------------------------------------
 * General: tears down and destructs all the stack's sessions.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pTest	- structure holding the stack related variables.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void TestRtspSessionDestruct(
					IN int index);

static RvStatus TestRtspSessionSendRequestSetup(IN	RvRtspSessionHandle	hSession,
                                                IN  const RvChar        *strURI);


/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * initTest
 * ------------------------------------------------------------------------
 * General: init module function
 *
 * Return Value:	void
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	printCB - callback function for printing usage.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void initTest(IN printCB printCBParam)
{

    pTest = &Test;
	pfnPrintCB = printCBParam;

    memset(trackArray, 0, sizeof(trackArray));

	/* initialize stack */
    Test.rtspConfiguration.maxConnections		= TEST_MAX_CONNECTIONS_ALLOWED;
    Test.rtspConfiguration.memoryElementsNumber	= 300;
    Test.rtspConfiguration.memoryElementsSize	= 300;
    strcpy(Test.rtspConfiguration.strDnsAddress, DNS_SERVER_ADDRESS);
    Test.rtspConfiguration.msgRequestElementsNumber = 10;
    Test.rtspConfiguration.msgResponseElementsNumber = 10;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    Test.rtspConfiguration.maxRtspMsgHeadersInMessage = TEST_MAX_HEADERS_ALLOWED;
#endif
    /* Initialize the globals */
    gServerPortA = 0;
    gServerPortB = 0;
#ifdef USE_RTP
    ghRTP        = NULL;
#endif
    gReceiveRTP   = RV_FALSE;

#if RV_LOGMASK != RV_LOGLEVEL_NONE
    RvRtspInit(NULL,
               &Test.rtspConfiguration,
               sizeof(Test.rtspConfiguration),
               &Test.hRtsp);
#else
    RvRtspInit(NULL,
               &Test.rtspConfiguration,
               sizeof(Test.rtspConfiguration),
               &Test.hRtsp);
#endif
}

/**************************************************************************
 * doneTest
 * ------------------------------------------------------------------------
 * General: destructs module function
 *
 * Return Value:	void
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	None.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void doneTest(void)
{
    TestRtspConnectionsDisconnect(&Test);
    pTest = NULL;
    RvRtspEnd(Test.hRtsp);
}

/**************************************************************************
 * testMainLoop
 * ------------------------------------------------------------------------
 * General: main function, calls the testing functions.
 *
 * Return Value: returns zero (0) when this is the last time to call the
 *				 mainLoop function, otherwise return a non zero value.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	timeOut - will wait on CCore select engine events for at least
 *						  timeOut milliseconds
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
int testMainLoop(IN RvUint32 timeOut)
{
    if (sleepCycles != 0)
    {
        if (RvRtspMainLoop(Test.hRtsp, timeOut) != RV_OK)
		{
			doneTest();
			return 0;
		}

        if (firstTime)
        {
            firstTime = RV_FALSE;
            TestRtspConnectionsConstruct(&Test);
            TestRtspConnectionsConnect(&Test);
        }

        if (stop == TEST_MAX_CONNECTIONS_ALLOWED)
        {
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
              Sleep(1000);
#else
              sleep(1);
#endif
            sleepCycles--;
        }

		return 1;
    }

	else
	{
		doneTest();
		return 0;
	}
}


/**************************************************************************
 * TestConnectionOnConnectEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when the connection is
 *			established (or connection failed).
 *
 * Return Value:	RV_OK if successful, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection	- the connection.
 *				hApp		- application context.
 *				success		- has the connection been established.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnConnectEv(
								IN RvRtspConnectionHandle		hConnection,
								IN RvRtspConnectionAppHandle	hApp,
								IN RvBool						success)
{
    int index = (int)hApp;

    if (hConnection != pTest->hConnections[index])
        return RV_OK;

    sprintf(printOut,"TestConnectionOnConnectEv %d %d\r\n", index, success);
    if (pfnPrintCB != NULL)
        pfnPrintCB(printOut);

    if (success == RV_TRUE)
    {
            pTest->numReconnects[index] = 0;
            RvRtspConnectionRequestDescribe(pTest->hConnections[index], uriArray[index],(RvRtspConnectionDescribeAppHandle)index);

    }
    else
    {
        RvRtspConnectionDestruct(pTest->hRtsp, pTest->hConnections[index], RV_TRUE);
        pTest->hConnections[index] = NULL;

        if (pTest->numReconnects[index] <= TEST_MAX_CONNECTION_RECONNECT)
        {

            RvRtspConnectionConstruct(	pTest->hRtsp,
                hApp,
                uriArray[index],
                NULL,
                &pTest->connectionConfiguration,
                sizeof(pTest->connectionConfiguration),
                &pTest->hConnections[index]);

            RvRtspConnectionRegisterCallbacks(  pTest->hConnections[index],
                &pTest->connectionCallbacks,
                sizeof(pTest->connectionCallbacks));

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
              Sleep(5000);
#else
              sleep(5);
#endif
            pTest->numReconnects[index]++;

            RvRtspConnectionConnect(pTest->hConnections[index]);
        }
    }

    if (pTest->hConnections[index] == NULL)
    {
        stop++;
    }
    return RV_OK;
}



/**************************************************************************
 * TestConnectionOnDisconnectEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when the connection is
 *			disconnected.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection	- the connection.
 *				hApp		- application context.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnDisconnectEv(
								IN RvRtspConnectionHandle		hConnection,
								IN RvRtspConnectionAppHandle	hApp)
{
    int index = (int) hApp;

    sprintf(printOut, "TestConnectionOnDisconnectEv %d\r\n", index);
	if (pfnPrintCB != NULL)
		pfnPrintCB(printOut);
    if (hConnection == pTest->hConnections[index])
    {
       RvRtspConnectionDestruct(pTest->hRtsp, pTest->hConnections[index], RV_TRUE);

        sprintf(printOut, "TestConnectionOnDisconnectEv %d\r\n", index);
	    if (pfnPrintCB != NULL)
		    pfnPrintCB(printOut);

        RvRtspConnectionConstruct(	pTest->hRtsp,
            hApp,
            uriArray[index],
            NULL,
            &pTest->connectionConfiguration,
            sizeof(pTest->connectionConfiguration),
            &pTest->hConnections[index]);

        RvRtspConnectionRegisterCallbacks(  pTest->hConnections[index],
            &pTest->connectionCallbacks,
            sizeof(pTest->connectionCallbacks));
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
              Sleep(5000);
#else
              sleep(5);
#endif

        pTest->numReconnects[index]++;

        RvRtspConnectionConnect(pTest->hConnections[index]);
    }

	RvRtspConnectionDestruct(pTest->hRtsp, pTest->hConnections[index], RV_FALSE);

#ifdef USE_RTP
    TestStopRtp();
#endif
    return RV_OK;
}



/**************************************************************************
 * TestConnectionOnErrorEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when an error message is
 *			received on the connection.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- the connection.
 *				hApp			- application context.
 *				hURI			- the URI on which the request was made.
 *				requestMethod	- the request method.
 *				status			- the response status.
 *				hPhrase			- the response phrase.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnErrorEv(
								IN	RvRtspConnectionHandle		hConnection,
								IN	RvRtspConnectionAppHandle	hApp,
								IN	RvRtspStringHandle			hURI,
								IN	RvRtspMethod				requestMethod,
								IN	RvRtspStatus				status,
								IN	RvRtspStringHandle			hPhrase)
{
	RvChar				uri[PRINT_BUFFER_LENGTH];
	RvChar				phrase[PRINT_BUFFER_LENGTH];
    int index = (int)hApp;

    if (hConnection == pTest->hConnections[index])
    {
        RvRtspStrcpy(pTest->hRtsp, hURI, PRINT_BUFFER_LENGTH, uri);
        RvRtspStrcpy(pTest->hRtsp, hPhrase, PRINT_BUFFER_LENGTH, phrase);

        sprintf(printOut, "TestConnectionOnErrorEv Index = %d Status = %d \r\n", index, status);
        if (pfnPrintCB != NULL)
            pfnPrintCB(printOut);
	if(status == RV_RTSP_STATUS_UNAUTHORIZED && requestMethod ==RV_RTSP_METHOD_DESCRIBE)
	{
		//RvRtspConnectionRequestDescribeEx(pTest->hConnections[index], uriArray[index], (RvRtspConnectionDescribeAppHandle)index,pTest->username[index],pTest->password[index]);
		RvRtspConnectionRequestDescribe(pTest->hConnections[index], uriArray[index], (RvRtspConnectionDescribeAppHandle)index);
		return RV_OK;
	}

        RvRtspConnectionDestruct(pTest->hRtsp, pTest->hConnections[index], RV_FALSE);
        pTest->hConnections[index] = NULL;

        stop++;
    }
	RV_UNUSED_ARG(requestMethod);
	return RV_OK;
}


/**************************************************************************
 * TestConnectionOnErrorExtEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when an error message is
 *			received on the connection.
 *          The same as TestConnectionOnErrorEv except that raise the describe
 *          application handle and the received response to the application.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- The Connection on which the error occurred.
 *				hApp			- application context.
 *              hDescribe       - The application handle for the failed describe request
 *                                or NULL if the error is not on describe request.
 *				hURI			- The URI requested in the message that
 *								  caused the error.
 *				requestMethod	- The requested method.
 *				status			- The response status.
 *				phrase			- The response phrase.
 *				pResponse	    - the received response.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnErrorExtEv(
								IN	RvRtspConnectionHandle		        hConnection,
								IN	RvRtspConnectionAppHandle	        hApp,
                                IN  RvRtspConnectionDescribeAppHandle   hDescribe,
								IN	RvRtspStringHandle			        hURI,
								IN	RvRtspMethod				        requestMethod,
								IN	RvRtspStatus				        status,
								IN	RvRtspStringHandle			        hPhrase,
								IN	const RvRtspResponse		        *pResponse)
{
    RV_UNUSED_ARG(hDescribe);
    RV_UNUSED_ARG(pResponse);
    return TestConnectionOnErrorEv(hConnection, hApp, hURI, requestMethod, status, hPhrase);
}

/**************************************************************************
 * TestConnectionOnDescribeResponseEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when a Describe response is
 *			received on the connection.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection			- the connection.
 *				hApp				- application context.
 *				pDescribeResponse	- the Describe response.
 *				hURI				- the URI on which the request was made.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnDescribeResponseEv(
								IN	RvRtspConnectionHandle		hConnection,
								IN	RvRtspConnectionAppHandle	hApp,
								IN	RvRtspConnectionDescribeAppHandle	hDescribe,
								IN	const RvRtspResponse		*pDescribeResponse,
								IN	RvRtspStringHandle			hURI)
{
	RvSize_t			blobSize = 0;
	RvChar				uri[512];
	RvChar				body[PRINT_BUFFER_LENGTH];
    RvRtspTransportHeader	transportHeader;
    int index = (int)hDescribe;
    int connIndex = (int)hApp;
    char track[128];


    char szReply[1024] = {0};
    int nResult = -1;

	RvSdpMsg * pSdpMsg = NULL;
	RvSdpParseStatus rvSdpPraseStatus = RV_SDPPARSER_STOP_ZERO;
	RvSdpMediaDescr *pMediaDesrc = NULL;
	RvSdpRtpMap *pRtpMap = NULL;

	int nSize = 0;
	int nMediaCount=0, nMediaID=0;
	int nPayloadType=0;
	RvSdpAttribute * att;
	int num,i;

    if (connIndex != index)
        return RV_ERROR_UNKNOWN;
	RvRtspBloblen(pTest->hRtsp, pDescribeResponse->hBody,&blobSize);
	RvRtspStrcpy(pTest->hRtsp, hURI, sizeof(uri), uri);
	RvRtspBlobcpy(pTest->hRtsp, pDescribeResponse->hBody, PRINT_BUFFER_LENGTH, body);
	body[blobSize] = 0;

	blobSize = strlen(body);

    sprintf(printOut, "TestConnectionOnDescribeResponseEv Index = %d\r\n", index);
	if (pfnPrintCB != NULL)
		pfnPrintCB(printOut);

		// 创建SDP解析单元
	nSize = strlen(body)+1;
	RvSdpMgrConstruct();
	pSdpMsg = rvSdpMsgConstructParse(NULL, body, &nSize, &rvSdpPraseStatus);
	//printf("SDP---%s \n",body);
	if(pSdpMsg == NULL)
		return -1;

	nMediaCount = rvSdpMsgGetNumOfMediaDescr(pSdpMsg);
	for(nMediaID=0; nMediaID<nMediaCount; nMediaID++)
    {
		//获取绘画描述
		pMediaDesrc = rvSdpMsgGetMediaDescr(pSdpMsg, nMediaID);
		if(pMediaDesrc == NULL)
			continue;

        num =rvSdpMediaDescrGetNumOfAttr2(pMediaDesrc);

		for(i=0;i<num;i++)
		{

			att=rvSdpMediaDescrGetAttribute2(pMediaDesrc,i);

			strcpy(track,att->iAttrName);
			//_strlwr_s(track,sizeof(track));
			if(strcmp(track,"control")==0)
			{
				printf("rvSdpMediaDescrGetAttribute %s=%s\n",att->iAttrName,att->iAttrValue);
				//strcpy(trackArray[index][nMediaID],att->iAttrValue);
				sprintf(trackArray[index][nMediaID],"%s/%s",uriArray[index],att->iAttrValue);
				break;
			}
		}

	}
	rvSdpMsgDestruct(pSdpMsg);
	RvSdpMgrDestruct();



    if (hConnection == pTest->hConnections[connIndex])
    {
        //int i, j=0;

        ///* get the tracks */
        //for(i=0; (body[i] != '\0'); i++)
        //{
        //    if ((body[i] == 'a') && ((blobSize-i) > 10))
        //    {
        //        if (sscanf(&body[i], "a=control:%s\n", track) == 1)
        //        {
        //            if (track[0] != '*')
        //            {
        //                sprintf(trackArray[index][j], "%s/%s", uriArray[index], track);
        //                j++;
        //                if (j==2)
        //                    break;
        //            }
        //        }
        //    }
        //}

        if (trackArray[index][0] != NULL)
        {

            TestRtspSessionConstruct(index);
            RvRtspSessionSetUri(pTest->hSessions[index], trackArray[index][0]);

            /* Send the setup request using SessionSendRequest API instead of dedicated API*/
			printf("TestRtspSessionSendRequestSetup=%s \n",trackArray[index][0]);
            TestRtspSessionSendRequestSetup(pTest->hSessions[index], trackArray[index][0]);

            memset(&transportHeader, 0, sizeof(transportHeader));
            /* setting transport parameters for Setup request	*/
            //transportHeader.clientPortA = 5000;
            //transportHeader.clientPortB = 5001;
            //transportHeader.isUnicast   = RV_TRUE;
            //transportHeader.serverPortA	= 4000;
            //transportHeader.serverPortB	= 3000;
            //strcpy(transportHeader.destination, "172.16.70.21");
            /* Dedicated API */
            //RvRtspSessionSetup(pTest->hSessions[index], &transportHeader);
        }
    }

	RV_UNUSED_ARG(hApp);
    return RV_OK;
}



/**************************************************************************
 * TestConnectionOnRedirectRequestEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when a Redirect request is
 *			received on the connection.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- the connection.
 *				hApp			- application context.
 *				pRequest		- the Redirect request.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnRedirectRequestEv(
								IN	RvRtspConnectionHandle		hConnection,
								IN  RvRtspConnectionAppHandle	hApp,
								IN	const RvRtspRequest			*pRequest)
{
    int index = (int)hApp;
    sprintf(printOut, "TestConnectionOnRedirectRequestEv Index = %d \r\n", index);
	if (pfnPrintCB != NULL)
		pfnPrintCB(printOut);

	RV_UNUSED_ARG(hConnection);
	RV_UNUSED_ARG(pRequest);
    return RV_OK;
}

/******************************************************************************
 * TestConnectionOnReceiveEv
 * ----------------------------------------------------------------------------
 * General: This callback is called when an RTSP message that is not
 *          related to a session is received on a client's connection.
 *          This callback can be used to retrieve additional headers or header fields
 *          that are not specifically supported by the client.
 * Return   Value: RV_OK  - if successful.
 *               Other on failure
 * See Also:    RvRtspConnectionOnSendEv
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  hConnection      - The connection handle in the client stack.
 *         hApp             - The handle of the application connection object .
 *         pRequest         - The received message if it is a request,
 *                            NULL if it is a response.
 *         pResponse        - The received message if it is a response,
 *                            NULL if it is a request.
 *****************************************************************************/
RvStatus RVCALLCONV TestConnectionOnReceiveEv(
                                IN	RvRtspConnectionHandle		hConnection,
                                IN	RvRtspConnectionAppHandle	hApp,
                                IN	RvRtspRequest			    *pRequest,
                                IN	RvRtspResponse			    *pResponse)
{
    RvRtspMsgMessageHandle hMessage;
    RvBool                 bIsRequest;
    RvStatus               status = RV_OK;

    RV_UNUSED_ARG(hConnection);
    RV_UNUSED_ARG(hApp);

    sprintf(printOut, "TestConnectionOnReceiveEv \r\n");
    if (pfnPrintCB != NULL)
        pfnPrintCB(printOut);

    if (pRequest != NULL)
    {
        /* A request message received */
        hMessage = pRequest->hRtspMsgMessage;
        bIsRequest = RV_TRUE;
    }
    else
    {
        /* A response message received */
        hMessage = pResponse->hRtspMsgMessage;
        bIsRequest = RV_FALSE;
    }

    if (hMessage != NULL)
    {
        RvUint32            i;
        RvRtspHeaderName    haederNames[TEST_MAX_HEADERS_ALLOWED];
        RvUint32            numHeaders = TEST_MAX_HEADERS_ALLOWED;

        for (i = 0; i < TEST_MAX_HEADERS_ALLOWED; ++i)
        {
            /* Allocate space for header names and initialize header name objects */
            haederNames[i].headerName = (RvChar *)malloc(TEST_HEADER_NAME_SIZE + 1);
            haederNames[i].headerNameLen = TEST_HEADER_NAME_SIZE;
            memset(haederNames[i].headerName, 0, TEST_HEADER_NAME_SIZE + 1);
        }
        /* Get all the header names in the message */
        status = RvRtspMsgGetMessageHeaderNames(Test.hRtsp, hMessage, bIsRequest, &numHeaders, haederNames);

        if (status == RV_OK)
        {
            RvChar              *fieldsBuffer[TEST_MAX_HEADER_FIELDS_ALLOWED];
            RvRtspMsgAppHeader appHeader;

            /* Allocate memory for header fields */
            for (i = 0; i < TEST_MAX_HEADER_FIELDS_ALLOWED; ++i)
            {
                fieldsBuffer[i] = (RvChar *)malloc(TEST_MAX_HEADER_FIELD_VALUE_SIZE + 1);
            }
            for (i = 0; i < numHeaders; ++i)
            {
                RvUint32    j;

                /* Set appHeader */

                memset(&appHeader, 0, sizeof (RvRtspMsgAppHeader));
                /* Allocate the memory for the name */
                appHeader.headerName = (RvChar *)malloc(TEST_HEADER_NAME_SIZE + 1);
                appHeader.headerNameLen = TEST_HEADER_NAME_SIZE;
                appHeader.bIsRequest = bIsRequest;
                appHeader.headerFields = fieldsBuffer;
                appHeader.headerFieldsSize = TEST_MAX_HEADER_FIELDS_ALLOWED;
                appHeader.headerFieldStrLen = TEST_MAX_HEADER_FIELD_VALUE_SIZE;
                appHeader.hRtspMsgMessage = hMessage;
                for (j = 0; j < TEST_MAX_HEADER_FIELDS_ALLOWED; ++j)
                {
                    memset (fieldsBuffer[j], 0, TEST_MAX_HEADER_FIELD_VALUE_SIZE + 1);
                }
                sprintf(printOut, "<-- Received header - %s: ", haederNames[i].headerName);
                if (pfnPrintCB != NULL)
                    pfnPrintCB(printOut);
                /* This is not necessary because we already have the header handle - just for the sake of using an API */
                haederNames[i].hHeader = NULL;
                RvRtspMsgGetHeaderByName(Test.hRtsp, hMessage, bIsRequest, &haederNames[i]);
                /* Get the values of the header fields */
                RvRtspMsgGetHeaderFieldValues(Test.hRtsp, haederNames[i].hHeader, &appHeader);
                sprintf(printOut, "Header values: ");
                if (pfnPrintCB != NULL)
                    pfnPrintCB(printOut);
                for (j = 0; j < appHeader.headerFieldsSize; ++j)
                {
                    sprintf(printOut, "%s ", appHeader.headerFields[j]);
                    if (pfnPrintCB != NULL)
                        pfnPrintCB(printOut);
                }
                sprintf(printOut, "\r\n");
                if (pfnPrintCB != NULL)
                    pfnPrintCB(printOut);
            }
            for (i = 0; i < TEST_MAX_HEADER_FIELDS_ALLOWED; ++i)
            {
                free(fieldsBuffer[i]);
            }
        }
        for (i = 0; i < TEST_MAX_HEADERS_ALLOWED; ++i)
        {
            free(haederNames[i].headerName);
        }

    }

    return status;

}

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
/******************************************************************************
 * RvRtspSessionOnSendEv
 * ----------------------------------------------------------------------------
 * General: This callback is called when an RTSP messageis sent on a client's
 *           session. This callback can be used to edit the outgoing message and add
 *          additional headers or header fields that are not specifically
 *          supported by the client.
 * Return   Value: RV_OK  - if successful.
 *               Other on failure
 * See Also:    RvRtspSessionOnReceiveEv
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  hSession         - The session handle in the client stack.
 *         hApp             - The handle of the application session object .
 *         pRequest         - The received message if it is a request,
 *                            NULL if it is a response.
 *         pResponse        - The received message if it is a response,
 *                            NULL if it is a request.
 *****************************************************************************/
RvStatus RVCALLCONV TestSessionOnSendEv(
                                IN	RvRtspSessionHandle		    hSession,
                                IN 	RvRtspSessionAppHandle	    hApp,
                                IN	RvRtspRequest			    *pRequest,
                                IN	RvRtspResponse			    *pResponse)
{
    RvStatus               status;
    RvRtspMsgAppHeader     msgHeader;
    RvChar                 *headerFields[2];

    RV_UNUSED_ARG(hSession);
    RV_UNUSED_ARG(hApp);
    RV_UNUSED_ARG(pResponse);

    if (pRequest == NULL)
        return RV_OK;

    if (pRequest->requestLine.method != RV_RTSP_METHOD_SETUP || pRequest->transportValid != RV_TRUE)
        return RV_OK;

    sprintf(printOut,"TestSessionOnSendEv \r\n");
    if (pfnPrintCB != NULL)
        pfnPrintCB(printOut);


    headerFields[0] = (RvChar *)malloc(256);
    memset(headerFields[0], 0, 256);
    msgHeader.headerFields = headerFields;
    msgHeader.bIsRequest = RV_TRUE;
    memcpy(headerFields[0], "destination=172.16.70.21", 24);
    msgHeader.hRtspMsgMessage = pRequest->hRtspMsgMessage;
    msgHeader.delimiter = ';';
    msgHeader.headerFieldStrLen = 255;
    msgHeader.headerFieldsSize = 1;

    /* Lets add the destination to the setup request message from here */
    status = RvRtspMsgAddHeaderFields(Test.hRtsp, &msgHeader, &pRequest->transport.additionalFields);

     memset(headerFields[0], 0, 256);
     memcpy(headerFields[0], "ttl=127", 7);
    /* let's add ttl using the AddGenericHeader */
    RvRtspMsgAddGenericHeaderFields(Test.hRtsp, pRequest->transport.additionalFields, &msgHeader);


    free(headerFields[0]);

    return status;

}

/******************************************************************************
 * RvRtspConnectionOnSendEv
 * ----------------------------------------------------------------------------
 * General: This callback is called when an RTSP message that is not
 *          related to a session is sent on a client's connection.
 *          This callback can be used to edit the outgoing message and add
 *          additional headers or header fields that are not specifically
 *          supported by the client.
 * Return   Value: RV_OK  - if successful.
 *               Other on failure
 * See Also:    RvRtspConnectionOnReceiveEv
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  hConnection      - The connection handle in the client stack.
 *         hApp             - The handle of the application connection object .
 *         pRequest         - The received message if it is a request,
 *                            NULL if it is a response.
 *         pResponse        - The received message if it is a response,
 *                            NULL if it is a request.
 *****************************************************************************/
RvStatus RVCALLCONV TestConnectionOnSendEv(
                                IN	RvRtspConnectionHandle		    hConnection,
                                IN 	RvRtspConnectionAppHandle	    hApp,
                                IN	RvRtspRequest			        *pRequest,
                                IN	RvRtspResponse			        *pResponse)
{
    RvStatus               status;
    RvRtspMsgAppHeader     msgHeader;
    RvChar                 *headerFields[2];
    RvRtspHeaderName       headerName;
    RvChar                 *tokenBuffer[3];
    RvChar                 paramFieldStr[256];
    RvUint16               paramFieldLen = 256;

    RV_UNUSED_ARG(hConnection);
    RV_UNUSED_ARG(hApp);
    RV_UNUSED_ARG(pResponse);

    if (pRequest == NULL)
        return RV_OK;

    if (pRequest->requestLine.method != RV_RTSP_METHOD_DESCRIBE)
        return RV_OK;
    sprintf(printOut,"TestConnectionOnSendEv \r\n");
    if (pfnPrintCB != NULL)
        pfnPrintCB(printOut);

    memset(&msgHeader, 0, sizeof(msgHeader));
    /* Lets add the Accept header to the setup nessage */
    headerFields[0] = (RvChar *)malloc(256);
    headerFields[1] = (RvChar *)malloc(256);
    memset(headerFields[0], 0, 256);
    memset(headerFields[1], 0, 256);
    msgHeader.headerFields = headerFields;
    msgHeader.bIsRequest = RV_TRUE;
    /* Lets build the new field value using the RvRtspUtilsSetParameterTokens API */
    tokenBuffer[0] = (RvChar *)malloc(50);
    memset(tokenBuffer[0], 0, 50);
    tokenBuffer[1] = (RvChar *)malloc(50);
    memset(tokenBuffer[1], 0, 50);
    tokenBuffer[2] = (RvChar *)malloc(50);
    memset(tokenBuffer[2], 0, 50);
    memset(paramFieldStr, 0, sizeof(paramFieldStr));
    memcpy(tokenBuffer[0], "application/sdp", sizeof("application/sdp"));
    memcpy(tokenBuffer[1], "application/rtsl", sizeof("application/rtsl"));
    memcpy(tokenBuffer[2], "application/mheg", sizeof("application/mheg"));

    RvRtspUtilsSetParameterTokens(3, tokenBuffer, &paramFieldLen, paramFieldStr);

    memcpy(headerFields[0], paramFieldStr, strlen(paramFieldStr));
    msgHeader.headerName = "Accept";
    msgHeader.headerNameLen = strlen("Accept");
    msgHeader.hRtspMsgMessage = pRequest->hRtspMsgMessage;
    msgHeader.delimiter = ';';
    msgHeader.headerFieldStrLen = 255;
    msgHeader.headerFieldsSize = 1;

    status = RvRtspMsgAddHeader(Test.hRtsp, &msgHeader);

    /* Again no clever reason to do it this way - this is just for the sake of a sample code*/
    /* Add a field to the Accept message */
    memset(&headerName, 0, sizeof(headerName));
    headerName.headerName = "Accept";

    status = RvRtspMsgGetHeaderByName(Test.hRtsp, pRequest->hRtspMsgMessage, RV_TRUE, &headerName);
    /* Set the additional field */
    memset(headerFields[0], 0, 256);
    memset(headerFields[1], 0, 256);
    memcpy(headerFields[0], "level=2", 7);
    msgHeader.delimiter = ';';
    msgHeader.headerFieldsSize = 1;

    status = RvRtspMsgAddGenericHeaderFields(Test.hRtsp, headerName.hHeader, &msgHeader);

    free(headerFields[0]);
    free(headerFields[1]);
    free(tokenBuffer[0]);
    free(tokenBuffer[1]);
    free(tokenBuffer[2]);
    return status;
}
#endif
/**************************************************************************
 * TestConnectionOnRawBufferReceiveEv
 * ------------------------------------------------------------------------
 * General: This is the definition for the callback function to be called
 *			when a chunk of data is received on the connection.
 *          The application can use the callback to extract interleaved
 *          (encapsulated) data from the buffer.
 *
 * Return Value:	RV_OK if the received buffer should be handled in the
 *                  usual manner or negative values if buffer should be ignored
 *                  by the transport layer.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection			- The connection handle in the client stack.
 *              hApp                - The handle of the application connection object.
 *				pBuff		        - The received buffer.
 *              buffSize            - The received buffer size.
 * OUTPUT	:	None
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestConnectionOnRawBufferReceiveEv(
                                IN	RvRtspConnectionHandle      hConnection,
                                IN	RvRtspConnectionAppHandle	hApp,
                                IN  RvUint8                     *pBuff,
                                IN  RvUint32                    buffSize)
{
    RV_UNUSED_ARG(hApp);

    /* Call the rawReceive API and return an error code so that the buffer
       will not be processed again by the transport*/
    RvRtspConnectionReceiveRawBuffer(hConnection, pBuff, buffSize);
    return RV_ERROR_UNKNOWN;
    //return RV_OK;

}

/**************************************************************************
 * TestSessionOnStateChangeEv
 * ------------------------------------------------------------------------
 * General: Event callback function, called when the session's state is
 *			changed (due to a response message received for the session).
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession	- the session.
 *				hApp		- application context.
 *				currState	- the current session state.
 *				newState	- the new session state.
 *				pResponse	- the received response.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestSessionOnStateChangeEv(
						IN	RvRtspSessionHandle		hSession,
						IN	RvRtspSessionAppHandle	hApp,
						IN	RvRtspSessionState		currState,
						IN	RvRtspSessionState		newState,
						IN	const RvRtspResponse	*pResponse)
{
    RvRtspNptTime		NptTime;
    RvChar				str[PRINT_BUFFER_LENGTH];
	RvRtspStringHandle	hStr;
    RvSize_t			arraySize;
    RvRtspRtpInfo       *pArrayElem;
    RvStatus            result;
    RvRtspTransportHeader	transportHeader;
    int                 index = (int)hApp;

    sprintf(printOut, "TestSessionOnStateChangeEv Index = %d old = %d new = %d\r\n", index, currState, newState);
	if (pfnPrintCB != NULL)
		pfnPrintCB(printOut);

    if (pResponse->cSeqValid == RV_TRUE)
    {
        sprintf(printOut, "TestSessionOnStateChangeEv CSeq = %d\r\n", pResponse->cSeq.value);
		if (pfnPrintCB != NULL)
			pfnPrintCB(printOut);
    }

    RvRtspSessionGetId(hSession, &hStr);
    RvRtspStrcpy(pTest->hRtsp, hStr, PRINT_BUFFER_LENGTH, str);
    sprintf(printOut, "TestSessionOnStateChangeEv ID = %s\r\n", str);
	if (pfnPrintCB != NULL)
		pfnPrintCB(printOut);

    if (pResponse->transportValid == RV_TRUE)
    {
        sprintf(printOut, "TestSessionOnStateChangeEv Transport Destination = %s, ClientA = %d, ClientB = %d, ServerA = %d, ServerB = %d\r\n",
            pResponse->transport.destination,
			pResponse->transport.clientPortA, pResponse->transport.clientPortB,
			pResponse->transport.serverPortA, pResponse->transport.serverPortB);
		if (pfnPrintCB != NULL)
			pfnPrintCB(printOut);
            gServerPortA = pResponse->transport.serverPortA;
            gServerPortB = pResponse->transport.serverPortB;
    }

    if (pResponse->rtpInfoValid== RV_TRUE)
    {
        RvRtspArraySize(pTest->hRtsp, pResponse->rtpInfo.hInfo, &arraySize);
        sprintf(printOut, "TestSessionOnStateChangeEv Rtp Info - Array Size = %d\r\n", arraySize);
		if (pfnPrintCB != NULL)
			pfnPrintCB(printOut);

        result = RvRtspArrayGetFirst(   pTest->hRtsp,
                                        pResponse->rtpInfo.hInfo,
                                        (void**)&pArrayElem);

        while(result == RV_OK)
        {
            RvRtspStrcpy(pTest->hRtsp, pArrayElem->hURI, PRINT_BUFFER_LENGTH, str);
            sprintf(printOut, "TestSessionOnStateChangeEv Rtp Info - URI = %s\r\n", str);
			if (pfnPrintCB != NULL)
				pfnPrintCB(printOut);

            if (pArrayElem->rtpTimeValid)
			{
                sprintf(printOut, "TestSessionOnStateChangeEv Rtp Info - rtptime = %d\r\n", pArrayElem->rtpTime);
				if (pfnPrintCB != NULL)
					pfnPrintCB(printOut);
			}

            if (pArrayElem->seqValid)
			{
                sprintf(printOut, "TestSessionOnStateChangeEv Rtp Info - seq = %d\r\n", pArrayElem->seq);
				if (pfnPrintCB != NULL)
					pfnPrintCB(printOut);
			}

            result = RvRtspArrayGetNext(pTest->hRtsp,
                                        pResponse->rtpInfo.hInfo,
                                        (void*)pArrayElem,
                                        (void**)&pArrayElem);
        }
    }

    switch (newState)
    {
        case RV_RTSP_SESSION_STATE_INIT:
        {
            TestRtspSessionDestruct(index);
#ifdef USE_RTP
            TestStopRtp();
#endif
            break;
        }

        case RV_RTSP_SESSION_STATE_READY:
        {
            switch (currState)
            {
                case RV_RTSP_SESSION_STATE_INIT:
                {
                    if (trackArray[index][1] != NULL)
                    {
                        RvRtspSessionSetUri(
                            pTest->hSessions[index],
                            trackArray[index][1]);

                        memset(&transportHeader, 0, sizeof(transportHeader));
                        /* setting transport parameters for Setup request	*/
                        transportHeader.clientPortA = 5002;
                        transportHeader.clientPortB = 5003;
                        transportHeader.isUnicast   = RV_TRUE;
//                        transportHeader.serverPortA	= 4000;
//                        transportHeader.serverPortB	= 3000;
                        //strcpy(transportHeader.destination, "172.16.70.21");
                        /* Dedicated API */
                        RvRtspSessionSetup(pTest->hSessions[index], &transportHeader);
                    }
                    break;
                }

                case RV_RTSP_SESSION_STATE_READY:
                {
                    RvRtspSessionSetUri(
                        pTest->hSessions[index], uriArray[index]);

                    NptTime.hours   = 0;
                    NptTime.minutes = 0;
                    NptTime.seconds = 0;
                    NptTime.format  = RV_RTSP_NPT_FORMAT_SEC;

                    RvRtspSessionPlay(pTest->hSessions[index], &NptTime);
                    break;
                }

                case RV_RTSP_SESSION_STATE_PLAYING:
                {
                    stop++;
#ifdef USE_RTP
                    TestStopRtp(); /* Received a PAUSE */
#endif

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
#ifdef USE_RTP
            if (TestStartRtp() == RV_OK)
            {
                gReceiveRTP = RV_TRUE;
                sprintf(printOut, "TestSessionOnStateChangeEv: Receiving of RTP Packets will start \n");
                if (pfnPrintCB != NULL)
                    pfnPrintCB(printOut);
            }
#endif
       //     RvRtspSessionPause(pTest->hSessions[index]);
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
 * General: Event callback function, called when an error response is
 *			received for the session.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession		- the session.
 *				hApp			- application context.
 *				requestMethod	- the request for which the error occurred.
 *				status			- the returned status.
 *				hPhrase			- the status phrase.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RVCALLCONV TestSessionOnErrorEv(
						IN	RvRtspSessionHandle		hSession,
						IN	RvRtspSessionAppHandle	hApp,
						IN	RvRtspMethod			requestMethod,
						IN	RvRtspStatus			status,
						IN	RvRtspStringHandle		hPhrase)
{
	RvChar		 phrase[500];
    int          index          = (int)hApp;
    RvUint32     len            = 500;

    memset(phrase, 0, 500);

    if (hPhrase != NULL)
        RvRtspStringHandleGetString(Test.hRtsp, hPhrase, phrase, &len);

	//RvRtspStrcpy(pTest->hRtsp, hPhrase, 500, phrase);

    sprintf(printOut, "RvRtspSessionOnErrorEv Index = %d Method = %d Status = %d Phrase = %s\r\n",
    	index, requestMethod, status, phrase);
	if (pfnPrintCB != NULL)
		pfnPrintCB(printOut);

    TestRtspSessionDestruct((int)hApp);
    stop++;

	RV_UNUSED_ARG(hSession);

    return RV_OK;
}


/**************************************************************************
 * TestRtspConnectionsConnect
 * ------------------------------------------------------------------------
 * General: internal function, used to connect the connection objects to
 *          the server.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pTest	- structure holding the stack related variables.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void TestRtspConnectionsConnect(
                             IN TestRtspStackStruct	*pTest)
{
    RvUint16	index = 0;

    for (index=0; index<TEST_MAX_CONNECTIONS_ALLOWED; index++)
    {
        RvRtspConnectionConnect(pTest->hConnections[index]);
    }
}


/**************************************************************************
 * TestRtspConnectionsDisconnect
 * ------------------------------------------------------------------------
 * General: internal function, used to disconnect the connection objects from
 *          the server.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pTest	- structure holding the stack related variables.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void	TestRtspConnectionsDisconnect(
                             IN TestRtspStackStruct	*pTest)
{
    RvUint16	index = 0;

    for (index=0; index<TEST_MAX_CONNECTIONS_ALLOWED; index++)
    {
        RvRtspConnectionDisconnect(pTest->hConnections[index]);
    }
}

/**************************************************************************
 * TestRtspConnectionsConstruct
 * ------------------------------------------------------------------------
 * General: internal function, used to construct an RTSP stack and
 *			connections.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pRtsp	- structure holding the stack related variables.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void	TestRtspConnectionsConstruct(
					IN TestRtspStackStruct	*pTest)
{
	RvUint32	index = 0;

	for (index=0; index<TEST_MAX_CONNECTIONS_ALLOWED; index++)
		pTest->hConnections[index] = NULL;

	index = 0;

	pTest->connectionConfiguration.dnsMaxResults				= TEST_MAX_DNS_RESULTS_ALLOWED;
	pTest->connectionConfiguration.maxHeadersInMessage			= TEST_MAX_HEADERS_ALLOWED;
	pTest->connectionConfiguration.maxSessions					= TEST_MAX_SESSIONS_ALLOWED;
	pTest->connectionConfiguration.maxUrlsInMessage				= TEST_MAX_URL_IN_MSG_ALLOWED;
	pTest->connectionConfiguration.transmitQueueSize			= TEST_MAX_TX_QUEUE_SIZE_ALLOWED;
	pTest->connectionConfiguration.maxWaitingDescribeRequests   = TEST_MAX_DESCRIBE_REQUESTS_ALLOWED;
	pTest->connectionConfiguration.describeResponseTimeOut      = 10000;

	pTest->connectionCallbacks.pfnOnConnectEv			= TestConnectionOnConnectEv;
	pTest->connectionCallbacks.pfnOnDisconnectEv		= TestConnectionOnDisconnectEv;
	pTest->connectionCallbacks.pfnOnErrorEv				= TestConnectionOnErrorEv;
	pTest->connectionCallbacks.pfnOnDescribeResponseEv	= TestConnectionOnDescribeResponseEv;
	pTest->connectionCallbacks.pfnOnErrorExtEv          = TestConnectionOnErrorExtEv;
	pTest->connectionCallbacks.pfnOnReceiveEv           = TestConnectionOnReceiveEv;
	pTest->connectionCallbacks.pfnOnRawBufferReceiveEv  = TestConnectionOnRawBufferReceiveEv;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pTest->connectionCallbacks.pfnOnSendEv              = TestConnectionOnSendEv;
#endif

	for (index=0; index<TEST_MAX_CONNECTIONS_ALLOWED; index++)
	{
		RvRtspConnectionConstruct(	pTest->hRtsp,
									(RvRtspConnectionAppHandle)index,
									uriArray[index],
									NULL,
									&pTest->connectionConfiguration,
                                    sizeof(pTest->connectionConfiguration),
									&pTest->hConnections[index]);

		RvRtspConnectionRegisterCallbacks(  pTest->hConnections[index],
                                            &pTest->connectionCallbacks,
                                            sizeof(pTest->connectionCallbacks));
	}
}


/**************************************************************************
 * TestRtspSessionConstruct
 * ------------------------------------------------------------------------
 * General: constructs the stack's sessions and sets the URIs according to
 *			the streamURI array.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pRtsp	- structure holding the stack related variables.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void	TestRtspSessionConstruct(
					IN int iConnection)
{
	pTest->sessionCallbackFunctions.pfnOnStateChangeEv              = TestSessionOnStateChangeEv;
	pTest->sessionCallbackFunctions.pfnOnErrorEv		            = TestSessionOnErrorEv;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pTest->sessionCallbackFunctions.pfnOnSendEv                     = TestSessionOnSendEv;
#endif
	pTest->sessionCallbackFunctions.pfnOnDestructEv		            = NULL;
	pTest->sessionConfiguration.pingTransmissionTimeOutResolution	= 30000;
	pTest->sessionConfiguration.responseTimeOutResolution			= 10000;

    RvRtspSessionConstruct(
			pTest->hConnections[iConnection],
			&pTest->sessionConfiguration,
            sizeof(pTest->sessionConfiguration),
			&pTest->sessionCallbackFunctions,
            sizeof(pTest->sessionCallbackFunctions),
			(RvRtspSessionAppHandle)iConnection,
			&pTest->hSessions[iConnection]);
}



/**************************************************************************
 * TestRtspSessionDestruct
 * ------------------------------------------------------------------------
 * General: tears down and destructs all the stack's sessions.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pRtsp	- structure holding the stack related variables.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void	TestRtspSessionDestruct(
					int	iConnection)
{
    int i, count=0;

    RvRtspSessionDestruct(pTest->hSessions[iConnection]);
    for (i=0; i<TEST_MAX_CONNECTIONS_ALLOWED; i++)
    {
        if (pTest->hConnections[iConnection] == pTest->hConnections[i])
            count++;
    }
    if (count == 1)
    {
        RvRtspConnectionDisconnect(pTest->hConnections[iConnection]);
    }
    else
    {
        pTest->hConnections[iConnection] = NULL;
    }
}

/**************************************************************************
 * TestRtspSessionSendRequestSetup
 * ------------------------------------------------------------------------
 * General:
 *  Used to send a SETUP request on a session, instead of using the dedicated API.
 *
 * Arguments:
 * Input	:	pRtsp	- structure holding the stack related variables.
 * Output	:	None.
 *
 *  Return Value:	RV_OK on success.
 *                  Other values on error.
 *************************************************************************/
static RvStatus TestRtspSessionSendRequestSetup(IN	RvRtspSessionHandle	hSession,
                                                IN  const RvChar        *strURI)
{
    RvRtspRequest *request  = NULL;

    RvStatus      status    = RV_OK;
    RvUint32      strLen    = strlen(strURI);

    sprintf(printOut, "TestRtspSessionSendRequestSetup strURI = %s\r\n", strURI);
	if (pfnPrintCB != NULL)
		pfnPrintCB(printOut);
    /* First construct the request object */
    status = RvRtspMessageConstructRequest(Test.hRtsp, &request);
    /* memset MUST NOT be called here */

    if (status == RV_OK)
    {
        /* Set the first line */
        request->requestLine.method = RV_RTSP_METHOD_SETUP;
        RvRtspStringHandleSetString(Test.hRtsp, (RvChar *)strURI, strLen, &request->requestLine.hURI);

        /* Set the transport */
        memset(&request->transport, 0, sizeof(request->transport));
        /* setting transport parameters for Setup request	*/
        request->transport.clientPortA = 5000;
        request->transport.clientPortB = 5001;
        request->transport.isUnicast   = RV_TRUE;
        request->transportValid = RV_TRUE;

        /* Cseq is set automatically by the stack */

        /* Send the request */
        RvRtspSessionSendRequest(hSession, request, RV_TRUE);

        /* destruct the request */
        RvRtspMessageDestructRequest(Test.hRtsp, request);
    }
    return status;
}

#ifdef USE_RTP
/**************************************************************************
 * TestStartRtp
 * ------------------------------------------------------------------------
 * General:
 *  This function initializes the RTP/RTCP stack, and sets the IP/Port
 *  parameters to enable sending and receiving RTP/RTCP packets
 *
 * Arguments:
 * Input:   None.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative Values otherwise.
 *************************************************************************/
RvStatus TestStartRtp (void)
{
    RvStatus      status;
    RvAddress     *tmpAddr;
    RvNetAddress  srcRtpAddr;
    RvNetAddress  dstRtpAddr; /* IP address/port from which RTP packets are received */
    RvUint32      ssrcPattern;
    RvUint32      ssrcMask;
    RvChar        *cname = "RvRtspClient";
    RvChar        serverIP[IP_ADDR_LEN];

    status      = RV_OK;
    ssrcPattern = 0x0F0F;
    ssrcMask    = 0xFFFF;
    memset(serverIP,'\0',IP_ADDR_LEN);
    sprintf(printOut, "TestStartRtp: Enter \n");
    if (pfnPrintCB != NULL)
        pfnPrintCB(printOut);

    /* Copy the client IP address */
    tmpAddr = (RvAddress*)srcRtpAddr.address;
    tmpAddr->addrtype = RV_ADDRESS_TYPE_IPV4;

    /* copy the IP address in Network Format */
    RvAddressSetString(CLIENT_IP_ADDRESS, tmpAddr);

    /* Copy the port */
    tmpAddr->data.ipv4.port = 6050;

    /* Copy the Server IP */
    strcpy(serverIP,uriArray[0]);
    if ((status = RvRtpInitEx(&srcRtpAddr)) != RV_OK)
    {
        sprintf(printOut, "TestStartRtp: RvRtpInitEx failed for srcRtpAddr (%s).\n", srcRtpAddr.address);
        if (pfnPrintCB != NULL)
            pfnPrintCB(printOut);

        return status;
    }

    ghRTP = RvRtpOpenEx(&srcRtpAddr, ssrcPattern, ssrcMask, cname);
    if (ghRTP == NULL)
    {
        sprintf(printOut, "TestStartRtp: Could not open Rtp: port (%d)\n",
                           tmpAddr->data.ipv4.port);
        if (pfnPrintCB != NULL)
            pfnPrintCB(printOut);

        return status;
    }
    RvRtpSetEventHandler(ghRTP,(RvRtpEventHandler_CB)TestReceiveRtp,NULL);
    /* Copy the server address/port */
    tmpAddr = (RvAddress*)dstRtpAddr.address;
    tmpAddr->addrtype = RV_ADDRESS_TYPE_IPV4;
    tmpAddr->data.ipv4.port =  8000;

    /* copy the IP address in Network Format */
    RvAddressSetString(serverIP, tmpAddr);

    /* Specify the remote IP address/port */
    RvRtpSetRemoteAddress(ghRTP, &dstRtpAddr);

    sprintf(printOut, "TestStartRtp: Leave \n");
    if (pfnPrintCB != NULL)
        pfnPrintCB(printOut);

    fp = fopen(RTP_FILE, "w");

    return status;

}

/**************************************************************************
 * TestStopRtp
 * ------------------------------------------------------------------------
 * General:
 *  This function stops the receiving of the RTP/RTCP packets from the
 *  server.
 *
 * Arguments:
 * Input:   None.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative Values otherwise.
 *************************************************************************/
RvStatus TestStopRtp (void)
{
    RvStatus      status;

    status      = RV_OK;

    sprintf(printOut, "TestStopRtp: Streaming of RTP Packets will stop\n");
    if (pfnPrintCB != NULL)
        pfnPrintCB(printOut);

    if (ghRTP != NULL)
    {
        RvRtpClose(ghRTP);
    }

    RvRtpEnd();
    RvRtcpEnd();

    if(fp != NULL)
    fclose(fp);

    ghRTP      = NULL;
    gReceiveRTP = RV_FALSE;

    return status;
}

/**************************************************************************
 * TestReceiveRtp
 * ------------------------------------------------------------------------
 * General:
 *  This function receives an RTP/RTCP packet sent from the server to
 *  the client.
 *
 * Arguments:
 * Input:   None.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative Values otherwise.
 *************************************************************************/
RvStatus TestReceiveRtp (void)
{
    RvRtpParam    rtpParam;
    RvUint8       rtpPacket[RTP_PACKET_LEN];
    RvUint8       *dataStartPtr; /* pointer to actual payload start */
    RvStatus      status;

    RvInt32        i;
    RvInt32       rtpByte;
    RvChar        rtpChar;

    status       = RV_OK;
    dataStartPtr = NULL;

    /* Required for writing rtp data to file */

    rtpByte = 0;
    memset((void *)rtpPacket,0,RTP_PACKET_LEN);
    /* Initialize the RTP header structure */
    RvRtpParamConstruct(&rtpParam);

    memset(rtpPacket, 0, RTP_PACKET_LEN);

    /* Receive a RTP packet */
    if (RvRtpRead(ghRTP, rtpPacket, RTP_PACKET_LEN, &rtpParam)<0)
    {
        sprintf(printOut, "TestReceiveRtp: RvRtpRead failed.\n");
        if (pfnPrintCB != NULL)
            pfnPrintCB(printOut);
        TestStopRtp();
        return RV_ERROR_UNKNOWN;
    }

    /* Write the RTP data from the buffer to a file */

    if (fp == NULL)
    {
        sprintf(printOut, "Could not open file (%s) to write data to be streamed.\n", RTP_FILE);
        if (pfnPrintCB != NULL)
            pfnPrintCB(printOut);
        TestStopRtp();
        return RV_ERROR_UNKNOWN;
    }
    else
    {
        for(i=12; i<rtpParam.len; i++)
        {
            rtpChar = (RvChar)rtpPacket[i];
            fputc(rtpChar,fp);
        }
    }

    return status;
}
#endif /* USE_RTP */

