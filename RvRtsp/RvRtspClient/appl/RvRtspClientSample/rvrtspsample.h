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
 *                              <rvrtspsample.h>
 *
 *  Sample program, showing usage of the RTSP stack, the program initializes
 *	the module, opens connections and communicates with the server.
 *
 *    Author                         Date
 *    ------                        ------
 *		Shaft						2/1/04
 *********************************************************************************/
#ifndef _RV_RTSP_SAMPLE_H
#define _RV_RTSP_SAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include <RvRtspClientInc.h>

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

/**************************************************************************
 * printCB
 * ------------------------------------------------------------------------
 * General: This callback is called by the sample to invoke sample ptint out
 *			calls.
 *
 * Return Value:	None
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	str	- print out of a message
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef void (RVCALLCONV *printCB)(IN char* str);

/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
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
void initTest(IN printCB printCBParam);

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
int testMainLoop(IN RvUint32 timeOut);


#ifdef __cplusplus
}
#endif

#endif  




