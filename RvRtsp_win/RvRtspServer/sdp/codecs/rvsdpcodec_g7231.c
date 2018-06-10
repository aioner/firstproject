/******************************************************************************
Filename    :rvsdpcodec_g7231.c
Description : handling g7231 codec type.

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

#ifdef RV_SDP_CODEC_G7231

/*legal parameters names for this codec*/
typedef enum {
	RV_SDP_CODEC_G7231_PTIME,
	RV_SDP_CODEC_G7231_AUDIO_FRAMES,
	RV_SDP_CODEC_G7231_SILENCE_SUPP
} RvSdpG7231ParamsEnum;

/*This array defines the grammar (syntax) of the codec parameters value  
  taken from the FMTP attribute*/
static const RvSdpParamStaticParserData gs_G7231_FmtpParserData[] = {
    {-1,                              NULL},
};

/*This array defines the grammar (syntax) of the codec parameters value
 that are taken from other attributes attributs*/
static const RvSdpParamStaticParserData gs_G7231_AttrsParserData[] = {
    {RV_SDP_CODEC_G7231_PTIME,              "ptime = d 1-5000"},
    {RV_SDP_CODEC_G7231_AUDIO_FRAMES,       "audioFrames = d 1-5000"},
    {RV_SDP_CODEC_G7231_SILENCE_SUPP,       "SilenceSupp"},
    {-1,                                        NULL},
};

RvStatus RvSdpCodec_G7231_Init(RvAlloc* a)
{
    RvSdpCodecParserInternal *cp;
    
    /*Constructs the codec params parser*/
    cp = RvSdpCodecParserInternalConstruct( gs_G7231_FmtpParserData, gs_G7231_AttrsParserData,a);
    if (!cp)
        return RV_SDPSTATUS_ALLOCFAIL;

    /*set the legal names of this codec and the legal separator*/
    cp->iCodecNames[0] = "G7231";
    cp->iSeparator = ';';

    if (RvSdpCodecRegister(cp) != RV_OK)
    {
        RvSdpCodecParserInternalDestruct(cp);
        return RV_SDPSTATUS_ALLOCFAIL;
    }

    return RV_SDPSTATUS_OK;
}


#endif /* #ifdef RV_SDP_CODEC_G7231*/
#endif /* #ifdef RV_SDP_CODECS_SUPPORTED */


