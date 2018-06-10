/******************************************************************************
Filename    :rvsdpcodec_g729.c
Description : handling g729 codec type.

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

#ifdef RV_SDP_CODEC_G729

/*legal parameters names for this codec*/
typedef enum {
    RV_SDP_CODEC_G729_ANNEXA,
	RV_SDP_CODEC_G729_ANNEXB,
	RV_SDP_CODEC_G729_PTIME
} RvSdpG729ParamsEnum;

/*This array defines the grammar (syntax) of the codec parameters value  
  taken from the FMTP attribute*/
static const RvSdpParamStaticParserData gs_G729_FmtpParserData[] = {
    {RV_SDP_CODEC_G729_ANNEXA,    "annexa"},
    {RV_SDP_CODEC_G729_ANNEXB,    "annexb"},
    {-1,                              NULL},
};

/*This array defines the grammar (syntax) of the codec parameters value
 that are taken from other attributes attributs*/
static const RvSdpParamStaticParserData gs_G729_AttrsParserData[] = {
    {RV_SDP_CODEC_G729_PTIME,              "ptime = d 1-5000"},
    {-1,                                        NULL},
};

RvStatus RvSdpCodec_G729_Init(RvAlloc* a)
{
    RvSdpCodecParserInternal *cp;
    
    /*Constructs the codec params parser*/
    cp = RvSdpCodecParserInternalConstruct( gs_G729_FmtpParserData, gs_G729_AttrsParserData,a);
    if (!cp)
        return RV_SDPSTATUS_ALLOCFAIL;

    /*set the legal names of this codec*/
    cp->iCodecNames[0] = "G729";
    cp->iCodecNames[1] = "G729a";
    cp->iCodecNames[2] = "G729b";
    cp->iCodecNames[3] = "G729ab";
    /*set the legal separator between codec's parameters*/
    cp->iSeparator = ';';

    if (RvSdpCodecRegister(cp) != RV_OK)
    {
        RvSdpCodecParserInternalDestruct(cp);
        return RV_SDPSTATUS_ALLOCFAIL;
    }

    return RV_SDPSTATUS_OK;
}


#endif /* #ifdef RV_SDP_CODEC_G729*/
#endif /* #ifdef RV_SDP_CODECS_SUPPORTED */

 
