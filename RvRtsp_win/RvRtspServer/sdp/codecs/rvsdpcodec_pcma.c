/******************************************************************************
Filename    :rvsdpcodec_pcma.c
Description : handling PCMA codec type.

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
#include "rvsdp.h"
#include "rvsdpcodecsconfig.h"

#ifdef RV_SDP_CODECS_SUPPORTED

#include "rvsdpcodecsinternal.h"

#include "rvsdpobjs.h"

#ifdef RV_SDP_CODEC_PCMA

/*legal parameters names for this codec*/
typedef enum {
    RV_SDP_CODEC_PCMA_PTIME,
} RvSdpPCMAParamsEnum;

/*no special syntax for FMTP parameters*/
static const RvSdpParamStaticParserData gs_PCMA_FmtpParserData[] = {
    {-1,                                        NULL},
};

/*This array defines the grammar (syntax) of the AMR codec parameters value
 that are taken from other attributes attributs*/
static const RvSdpParamStaticParserData gs_PCMA_AttrsParserData[] = {
    {RV_SDP_CODEC_PCMA_PTIME,              "ptime = d 1-5000"},
    {-1,                                        NULL},
};

RvStatus RvSdpCodec_PCMA_Init(RvAlloc* a)
{
    RvSdpCodecParserInternal *cp;

    /*Constructs the codec params parser*/
    cp = RvSdpCodecParserInternalConstruct(gs_PCMA_FmtpParserData,gs_PCMA_AttrsParserData,a);
    if (!cp)
        return RV_SDPSTATUS_ALLOCFAIL;
    
    /*set the legal names of this codec & legal separator*/
    cp->iCodecNames[0] = "PCMA";
    cp->iSeparator = ';';

    if (RvSdpCodecRegister(cp) != RV_OK)
    {
        RvSdpCodecParserInternalDestruct(cp);
        return RV_SDPSTATUS_ALLOCFAIL;
    }

    return RV_SDPSTATUS_OK;
}


#endif /* #ifdef RV_SDP_CODEC_PCMA*/
#endif /* #ifdef RV_SDP_CODECS_SUPPORTED */

 
