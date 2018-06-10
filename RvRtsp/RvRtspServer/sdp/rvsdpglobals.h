/******************************************************************************
Filename    :rvsdpglobals.h
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

#ifndef _rv_sdp_globals_h_
#define _rv_sdp_globals_h_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "rvsdpglobalsinternal.h"
#include "rvccoreglobals.h"

/* initializes global data structure */
void* RvSdpInitializeGlobals(
        int index, 
        void* usrData);

/* destroys global data structure */
void RvSdpDestroyGlobals(
        int index, 
        void* glDataPtr);

#define RV_SDP_USE_GLOBALS  RvSdpGlobals *__globals__ = (RvSdpGlobals *)RvGetGlobalDataPtr(RV_SDP_GLOBALS_INDEX)

#define RvSdpGlob(name) (__globals__->_##name)

/*
#define gSdpDefaultAlloc        RvSdpGlob(gSdpDefaultAlloc)
*/

#define g_sdpGlobal             RvSdpGlob(g_sdpGlobal)
#define pSdpLogSource           RvSdpGlob(pSdpLogSource)

#ifdef __cplusplus
}
#endif
    
#endif

