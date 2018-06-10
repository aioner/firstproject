/******************************************************************************
Filename    :rvsdpcodec_mp4a_latm.c
Description : handling mp4a_latm codec type.

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

#ifdef RV_SDP_CODEC_MP4A_LATM

/*legal parameters names for this codec*/
typedef enum {
    RV_SDP_CODEC_MP4A_LATM_PTIME,
    RV_SDP_CODEC_MP4A_LATM_PROFILE_LEVEL_ID,
    RV_SDP_CODEC_MP4A_LATM_OBJECT,
    RV_SDP_CODEC_MP4A_LATM_BITRATE,
    RV_SDP_CODEC_MP4A_LATM_CPRESENT,
    RV_SDP_CODEC_MP4A_LATM_CONFIG,
} RvSdpMP4ALATMFmtpParamsEnum;

/*This array defines the grammar (syntax) of the codec parameters value  
  taken from the FMTP attribute*/
static const RvSdpParamStaticParserData gs_MP4ALATM_FmtpParserData[] = {
    {RV_SDP_CODEC_MP4A_LATM_PROFILE_LEVEL_ID,   "profile-level-id = d 1-10000"},
    {RV_SDP_CODEC_MP4A_LATM_OBJECT,             "object = d 1-10000"},
    {RV_SDP_CODEC_MP4A_LATM_BITRATE,            "bitrate = d 1-100000000"},
    {RV_SDP_CODEC_MP4A_LATM_CPRESENT,           "cpresent = d 0-1"},
    {RV_SDP_CODEC_MP4A_LATM_CONFIG,             "config = t ANY"},      
    {-1,                                        NULL},
};

/*This array defines the grammar (syntax) of the AMR codec parameters value
 that are taken from other attributes attributs*/
static const RvSdpParamStaticParserData gs_MP4ALATM_AttrsParserData[] = {
    {RV_SDP_CODEC_MP4A_LATM_PTIME,              "ptime = d 1-5000"},
    {-1,                                        NULL},
};

RvSdpStatus RvSdpCodec_MP4A_LATM_Init(RvAlloc* a)
{
    RvSdpCodecParserInternal *cp;

    /*Constructs the codec params parser*/
    cp = RvSdpCodecParserInternalConstruct(gs_MP4ALATM_FmtpParserData,gs_MP4ALATM_AttrsParserData,a);
    if (!cp)
        return RV_SDPSTATUS_ALLOCFAIL;

    /*set the legal names of this codec and legal separator*/
    cp->iCodecNames[0] = "MP4A-LATM";
    cp->iSeparator = ';';

    if (RvSdpCodecRegister(cp) != RV_SDPSTATUS_OK)
    {
        RvSdpCodecParserInternalDestruct(cp);
        return RV_SDPSTATUS_ALLOCFAIL;
    }

    return RV_SDPSTATUS_OK;
}


#endif /* #ifdef RV_SDP_CODEC_MP4A_LATM    */
#endif /* #ifdef RV_SDP_CODECS_SUPPORTED   */
