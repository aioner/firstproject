/******************************************************************************
Filename    :rvsdpcodec_mp4v_es.c
Description : handling mp4v_es codec type.

  ************************************************************************
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

#ifdef RV_SDP_CODEC_MP4V_ES

/*legal parameters names for this codec*/
typedef enum {
    RV_SDP_CODEC_MP4V_ES_PROFILE_LEVEL_ID,
    RV_SDP_CODEC_MP4V_ES_CONFIG,
} RvSdpMP4VESFmtpParamsEnum;

/*This array defines the grammar (syntax) of the codec parameters value  
  taken from the FMTP attribute*/
static const RvSdpParamStaticParserData gs_MP4VES_FmtpParserData[] = {
    {RV_SDP_CODEC_MP4V_ES_PROFILE_LEVEL_ID,   "profile-level-id = d 1-10000"},
    {RV_SDP_CODEC_MP4V_ES_CONFIG,             "config = t ANY"},      
    {-1,                                        NULL},
};

/*no parameters from other attributes*/
static const RvSdpParamStaticParserData gs_MP4VES_AttrsParserData[] = {
    {-1,                                        NULL},
};

RvSdpStatus RvSdpCodec_MP4V_ES_Init(RvAlloc* a)
{
    RvSdpCodecParserInternal *cp;
    
    /*Constructs the codec params parser*/
    cp = RvSdpCodecParserInternalConstruct(gs_MP4VES_FmtpParserData,gs_MP4VES_AttrsParserData,a);
    if (!cp)
        return RV_SDPSTATUS_ALLOCFAIL;

    /*set the legal names of this codec and legal separator*/
    cp->iCodecNames[0] = "MP4V-ES";
    cp->iSeparator = ';';

    if (RvSdpCodecRegister(cp) != RV_SDPSTATUS_OK)
    {
        RvSdpCodecParserInternalDestruct(cp);
        return RV_SDPSTATUS_ALLOCFAIL;
    }

    return RV_SDPSTATUS_OK;
}



#endif /* #ifdef RV_SDP_CODEC_MP4V_ES */

#endif /*RV_SDP_CODECS_SUPPORTED*/
