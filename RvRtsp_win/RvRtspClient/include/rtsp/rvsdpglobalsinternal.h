/******************************************************************************
Filename    :rvsdpglobalsinternal.h
Description : definitions regarding use of global variables

  ******************************************************************************
  Copyright (c) 2005 RADVision Inc.
  ************************************************************************
  NOTICE:
  This document contains information that is proprietary to RADVision LTD.
  No part of this publication may be reproduced in any form whatsoever
  without written prior approval by RADVision LTD..
  
    RADVision LTD. reserves the right to revise this publication and make
    changes without obligation to notify any person of such revisions or
    changes.
 Author:Rafi Kiel
******************************************************************************/
#ifndef _rv_sdpglobalsinternal_h_
#define _rv_sdpglobalsinternal_h_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "rvalloc.h"
#include "rvlog.h"
#include "rvloglistener.h"
#include "rvsdp.h"

typedef struct
{
    RvLogMgr           logMgr;
    RvLogMgr*          pLogMgr;
    RvBool             bLogMgrSuppliedByApp;
    RvLogSource        logSource;
    RvLogListener      defaultLogFileListener;
    RvBool             bFileListenerInitialized;
    RvBool             bAppListenerInitialized;
    RvSdpStackCfg      stackCfg;    
}sdpGlobl;


/* the global data structure */
typedef struct 
{
    sdpGlobl*	    _g_sdpGlobal;
    RvLogSource*    _pSdpLogSource;    
/*
    RvAlloc         *_gSdpDefaultAlloc;
*/
} RvSdpGlobals;


#ifdef __cplusplus
}
#endif
#endif

