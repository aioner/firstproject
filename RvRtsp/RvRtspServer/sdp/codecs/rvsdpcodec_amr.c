/******************************************************************************
Filename    :rvsdpcodec_amr.c
Description : handling AMR codec type.

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

/*legal parameters names for this codec*/
typedef enum {
    RV_SDP_CODEC_AMR_MAXPTIME,
    RV_SDP_CODEC_AMR_PTIME,
    RV_SDP_CODEC_AMR_OCTET_ALIGN,
    RV_SDP_CODEC_AMR_MODE_SET,
    RV_SDP_CODEC_AMR_MODE_CHANGE_PERIOD,
    RV_SDP_CODEC_AMR_MODE_CHANGE_NEGHBOR,
    RV_SDP_CODEC_AMR_CRC,
    RV_SDP_CODEC_AMR_INTERLEAVING,
    RV_SDP_CODEC_AMR_ROBUST_SORTING,
} RvSdpAMRFmtpParamsEnum;


#ifdef RV_SDP_CODEC_AMR

/*This array defines the grammar (syntax) of the codec parameters value  
  taken from the FMTP attribute*/
static const RvSdpParamStaticParserData gs_AMR_FmtpParserData[] = {
    {RV_SDP_CODEC_AMR_OCTET_ALIGN,              "octet-align = d 0-1"},
    {RV_SDP_CODEC_AMR_MODE_SET,                 "mode-set = l, d 0-7"},
    {RV_SDP_CODEC_AMR_MODE_CHANGE_PERIOD,       "mode-change-period = d 1-999999"},
    {RV_SDP_CODEC_AMR_MODE_CHANGE_NEGHBOR,      "mode-change-neighbor = d 0-1"},
    {RV_SDP_CODEC_AMR_CRC,                      "crc = d 0-1"},
    {RV_SDP_CODEC_AMR_ROBUST_SORTING,           "robust-sorting = d 0-1"},
    {RV_SDP_CODEC_AMR_INTERLEAVING,             "interleaving = d 1-999999"},
    {-1,                                        NULL},
};

/*This array defines the grammar (syntax) of the AMR codec parameters value
 that are taken from other attributes attributs*/
static const RvSdpParamStaticParserData gs_AMR_AttrsParserData[] = {
    {RV_SDP_CODEC_AMR_PTIME,                    "ptime = d 1-999999"},
    {RV_SDP_CODEC_AMR_MAXPTIME,                 "maxptime = d 1-999999"},
    {-1,                                        NULL},
};

RvSdpStatus RvSdpCodec_AMR_Init(RvAlloc* a)
{
    RvSdpCodecParserInternal *cp;
    
	/*Constructs the codec params parser*/
    cp = RvSdpCodecParserInternalConstruct(gs_AMR_FmtpParserData,gs_AMR_AttrsParserData,a);
    if (!cp)
        return RV_SDPSTATUS_ALLOCFAIL;

	/*set the legal names of this codec*/
    cp->iCodecNames[0] = "AMR";
    cp->iCodecNames[1] = "AMR-WB";
    
	/*set the legal separator between codec's parameters*/
    cp->iSeparator = ';';

    if (RvSdpCodecRegister(cp) != RV_SDPSTATUS_OK)
    {
        RvSdpCodecParserInternalDestruct(cp);
        return RV_SDPSTATUS_ALLOCFAIL;
    }

    return RV_SDPSTATUS_OK;
}


#endif /* #ifdef RV_SDP_CODEC_AMR */

#endif /*RV_SDP_CODECS_SUPPORTED*/

 
