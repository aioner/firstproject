/******************************************************************************
Filename    :rvsdpglobals.c
Description :globals data treatment.

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
    ******************************************************************************
Author:Rafi Kiel
******************************************************************************/
#include "rvsdpglobalsinternal.h"
#include "rvmemory.h"
#include <stdlib.h>

void* RvSdpInitializeGlobals(int index, void* usrData)
{
    RvSdpGlobals *sdpGl;        
    RV_UNUSED_ARG(index);
    RV_UNUSED_ARG(usrData);
    
    {
#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)
        sdpGl = (RvSdpGlobals*) malloc(sizeof(RvSdpGlobals));
#else
        static RvSdpGlobals MySdpGlobalData;
        sdpGl = &MySdpGlobalData;
#endif
    }

    sdpGl->_g_sdpGlobal = NULL;
    sdpGl->_pSdpLogSource = NULL;
        
/*
    sdpGl->_gSdpDefaultAlloc = rvAllocGetDefaultAllocator();    
*/
    return sdpGl;
}


void RvSdpDestroyGlobals(int index, void* glDataPtr)
{
    RV_UNUSED_ARG(index);
    RV_UNUSED_ARG(glDataPtr);

#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)    
    free(glDataPtr);
#endif
}

