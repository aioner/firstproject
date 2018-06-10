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
 *                              <RvRtspServerSample.h>
 *
 * Sample program, showing usage of the RTSP server stack, the program initializes
 *	the module, accepts connections and communicates with the client
 *
 *********************************************************************************/

#ifndef _XT_RTSP_SERVER_H
#define _XT_RTSP_SERVER_H

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/

#include <string>

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/

/**************************************************************************
 * initServerTest
 * ------------------------------------------------------------------------
 * General: init module function
 *
 * Arguments:
 * Input:  printCB - callback function for printing usage.
 * Output: None.
 *
 * Return Value:	void
 *************************************************************************/
void initRTSPServer(void);

/**************************************************************************
 * ServerMainLoop
 * ------------------------------------------------------------------------
 * General: 
 * Main function, calls the testing functions.
 *
 * Arguments:
 * Input:  timeOut - will wait on CCore select engine events for at least
 *                   timeOut milliseconds
 * Output: None.
 *
 * Return Value: returns zero (0) when this is the last time to call the 
 *				 mainLoop function, otherwise return a non zero value.
 *************************************************************************/
int RTSPServerMainLoop(unsigned int timeOut);

//rtsp server unit
void termRTSPServer(void);

#endif  

