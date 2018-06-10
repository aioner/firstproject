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
 *                               <TRTSP_testapp.h>
 *
 * This file include functions that initiate the stack 
 *
 *******************************************************************************/
#ifndef _TRTSP_TESTAPP_H
#define _TRTSP_TESTAPP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "RvRtspClientTypes.h"

/*-----------------------------------------------------------------------*/
/*                              GLOBALS                                  */
/*-----------------------------------------------------------------------*/

RvBool        gbStackInited;
RvBool        gbStopStack;

/*-----------------------------------------------------------------------*/
/*                           FUNCTION HEADERS                            */
/*-----------------------------------------------------------------------*/

/******************************************************************************
 * TestSetStackVariables
 * ----------------------------------------------------------------------------
 * General: 
 *  Set the values of the stack variables fromthe GUI input.
 *
 * Arguments:
 * Input:  varName   - name of the variable.
 *         value     - value of the variable.
 * Output: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestSetStackVariables(char * varName, char * value);

/******************************************************************************
 * TestStackInitAndStart
 * ----------------------------------------------------------------------------
 * General:
 *  Function to initialize the Stack.
 *
 * Arguments:
 * Input:  argv         - Arguments vector received from the GUI.
 * Output: None
 *
 * Return Value: None
 *****************************************************************************/
void TestStackInitAndStart (char *argv[]);

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
void doneTest(void);

/******************************************************************************
 * TestClientSendResponse
 * ----------------------------------------------------------------------------
 * General: 
 *  sends the response witht the input values provided.
 *
 * Arguments:
 * Input:  argv     - holds the input strings from the GUI.
 * Output: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientSendResponse(char *argv[]);

/******************************************************************************
 * TestClientSendRequest
 * ----------------------------------------------------------------------------
 * General: 
 *  sends the response witht the input values provided.
 *
 * Arguments:
 * Input:  argv     - holds the input strings from the GUI.
 * Output: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientSendRequest(char *argv[]);

/******************************************************************************
 * TestClientUpdateSessionList
 * ----------------------------------------------------------------------------
 * General: 
 *  Provides the information on the sessions attached to the connection 
 *  to the GUI.
 *
 * Arguments:
 * Input:  hApp  = application context of the connection.
 * Output: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientUpdateSessionList(RvUint hApp);

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
void TestClientSendDescRequest(char *argv[]);

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
RvRtspConnectionAppHandle TestClientConstructConn(RvChar *strURI);

/******************************************************************************
 * TestClientSendSetup
 * ----------------------------------------------------------------------------
 * General: 
 *  sends the setup request ont he session
 * 
 * Arguments:
 * Input:  argv     - holds the input strings from the GUI.
 * Output: None
 *
 * Return Value: None.
 *****************************************************************************/
void TestClientSendSetup (char *argv[]);

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
void TestClientSessionPlay(RvUint hApp, RvChar *reqAddl);

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
void TestClientSessionPause(RvUint hApp, RvChar *reqAddl);

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
void TestClientSessionTeardown(RvUint hApp, RvChar *reqAddl);

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
    IN RvUint    index);

/********************************************************************************************
 * TestStackInit
 * purpose : Initialize the CBase and logs
 *
 * input   : none
 * output  : none
 * return  : none
 ********************************************************************************************/
void TestStackInit(void);

/********************************************************************************************
 * TestStackStart
 * purpose : Initialize the stack and starts listening
 *  
 * input   : none
 * output  : none
 * return  : none
 ********************************************************************************************/
void TestStackStart(void);

/********************************************************************************************
 * TestStackLoop
 * purpose : Stack select loop
 *  
 * input   : none
 * output  : none
 * return  : none
 ********************************************************************************************/
RvStatus TestStackLoop(void);

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
RvStatus LogFileReset();

#ifdef __cplusplus
}
#endif

#endif

