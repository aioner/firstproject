/************************************************************************************************************************

Notice:
This document contains information that is proprietary to RADVISION LTD..
No part of this publication may be reproduced in any form whatsoever without
written prior approval by RADVISION LTD..

RADVISION LTD. reserves the right to revise this publication and make changes
without obligation to notify any person of such revisions or changes.

*************************************************************************************************************************/

#ifndef _TRTSP_GENERAL_H
#define _TRTSP_GENERAL_H

#include "rverror.h"

#ifndef RV_OS_TYPE
#error
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32) || (RV_OS_TYPE == RV_OS_TYPE_SOLARIS) || \
 ((RV_OS_TYPE == RV_OS_TYPE_LINUX) && ((RV_OS_VERSION & RV_OS_LINUX_MVISTA) == 0)) || \
 (RV_OS_TYPE == RV_OS_TYPE_TRU64) || (RV_OS_TYPE == RV_OS_TYPE_HPUX)
#ifndef APP_NO_TCL
#define USE_TCL
#endif
#endif

#ifndef USE_TCL
#define SPLIT_APP
#endif

#ifdef USE_TCL
#include <tcl.h>
#include <tk.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef struct TestRtspStackStruct_t TestRtspStackStruct;
typedef struct AppHashObj_tag AppHashObj;

typedef enum
{
    AppResourceConnection,       /* Connection object */
    AppResourceSession,          /* Session object */
    AppResourceTransaction       /* Transaction object */
} RtspClientResourceType;



/******************************************************************************
 * AppAllocateResourceId
 * ----------------------------------------------------------------------------
 * General: Request user to allocate a resource identifier for an object.
 *
 * Return Value: Allocated resource on success, negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  dmtrApp               - Gateway object to use.
 *         resourceType     - Type of resource we need id for.
 * Output: None.
 *****************************************************************************/
RvInt32 AppAllocateResourceId(
    IN TestRtspStackStruct *   dmtrApp,
    IN RtspClientResourceType  resourceType);


#ifdef USE_TCL


extern Tcl_Interp *gInterp;

int TclInit(void);

/******************************************************************************************
 * TclEnd
 * purpose : Deinitialize the TCL interface
 * input   : none
 * output  : none
 * return  : TCL_OK - the command was invoked successfully.
 ******************************************************************************************/
int TclEnd(void);
#endif

/****************************************************************************
 * TclExecute
 * -------------------------------------------------------------------------
 * General: 
 *  Execute a command in tcl
 * 
 * Arguments:
 * Input:   cmd    - the command that is going to be executed
 * Output:  None
 * 
 * Return Value:  TCL_OK - the command was invoked successfully.
 ****************************************************************************/
int TclExecute(const char* cmd, ...);

#ifdef USE_TCL
/******************************************************************************************
 * TclGetVariable
 * purpose : get variable from tcl
 * input   : varName - the name of the variable that is going to be imported
 * input   : none
 * output  : none
 * return  : The variable's string value
 ******************************************************************************************/
char* TclGetVariable(const char* varName);
#endif

#ifdef SPLIT_APP
/******************************************************************************
 * rtspSplitAppListen
 * ----------------------------------------------------------------------------
 * General: 
 *  To wait for an incoming connection we have to open a client. yes, I
 *  know it's silly, but that's the way it is. This should be called
 *  from the C side.
 * 
 * Arguments:
 * Input:  None
 * Output: None
 *
 * Return Value: RV_OK on success
 *               Other on failure.
 *****************************************************************************/
RvStatus rtspSplitAppListen(void);


/******************************************************************************
 * rtspSplitAppTclExecute
 * ----------------------------------------------------------------------------
 * General: 
 *  Send a variable update command from the TCL side to the C side.
 *
 * Arguments:
 * Input:  cmd          - The TCL command string.
 * Output: None.
 *
 * Return Value: RV_OK on success, other on failure.
 *****************************************************************************/
RvStatus rtspSplitAppTclExecute(IN  char *cmd);
#endif /* SPLIT_APP */

#ifdef __cplusplus
}
#endif

#endif  /* _TRTSP_GENERAL_H */
