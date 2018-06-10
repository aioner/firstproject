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
 *                               <TRTSP_init.h>
 *
 * This file is used for initiating the test application.
 * It also used for defining global variables and common functions.
 *
 *******************************************************************************/



#ifndef _TRTSP_INIT_H
#define _TRTSP_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "TRTSP_general.h"

#ifdef USE_TCL
#include <tcl.h>
#include <tk.h>
#endif

/******************************************************************************
 * rtspUpdateVariable
 * ----------------------------------------------------------------------------
 * General:
 *  Update a variable in the Test object.
 *
 * Arguments:
 * Input:  varname          - String name of the variable.
 *         value            - String value of the variable.
 * Output: None.
 *
 * Return Value: none.
 *****************************************************************************/
void rtspUpdateVariable(char * varName, char * value);


/******************************************************************************
 * rtspCallFunction
 * ----------------------------------------------------------------------------
 * General:
 *  Call a function as if called from the TCL.
 *
 * Arguments:
 * Input:  funcName    - String name of the function to call.
 *         argc        - Number of arguments.
 *         argv        - Arguments vector.
 * Output: None
 *
 * Return Value:  None
 *****************************************************************************/
void rtspCallFunction(char * funcName, int argc, char *argv[]);

#ifdef SPLIT_APP
/******************************************************************************
 * InitSplitAppNetwork
 * ----------------------------------------------------------------------------
 * General: 
 *  Initialize the split application network - connect to the TCL side.
 *
 * Arguments:
 * Input:   None
 * Output:  None
 *
 * Return Value:  Non-negative value on success
 *                Negative value on failure
 ******************************************************************************/
int InitSplitAppNetwork(void);
#endif

#ifdef USE_TCL 
Tcl_Interp* InitTcl(const char* executable, char* versionString, char** reason);

/********************************************************************************************
 * InitApp
 * purpose : Initialize the test application
 *           This includes parts as RTP/RTCP support, etc.
 * input   : none
 * output  : none
 * return  : Non-negative value on success
 *           Negative value on failure
 ********************************************************************************************/
int InitApp(void);

/********************************************************************************************
 * InitApp
 * purpose : Initialize the stack
 *
 * input   : none
 * output  : none
 * return  : Non-negative value on success
 *           Negative value on failure
 ********************************************************************************************/
int InitStack(void);

/********************************************************************************************
 * PutError
 * purpose : Notify the user about errors that occured
 * input   : title  - Title of the error
 *           reason - Reason that caused the error
 * output  : none
 * return  : none
 ********************************************************************************************/
void PutError(const char* title, const char* reason);


RvBool IsGuiStopped(void);

#endif

#ifdef __cplusplus
}
#endif

#endif

