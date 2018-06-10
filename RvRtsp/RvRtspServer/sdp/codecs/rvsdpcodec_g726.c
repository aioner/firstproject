/******************************************************************************
Filename    :rvsdpcodec_g726.c
Description : handling g726 codec type.

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

#ifdef RV_SDP_CODEC_G726

/*ypedef enum {
} RvSdpG726FmtpParamsEnum;*/

/*no special syntax*/
static const RvSdpParamStaticParserData gs_G726_FmtpParserData[] = {
    {-1,                                        NULL},
};

/*no special syntax*/
static const RvSdpParamStaticParserData gs_G726_AttrsParserData[] = {
    {-1,                                        NULL},
};

RvStatus RvSdpCodec_G726_Init(RvAlloc* a)
{
    RvSdpCodecParserInternal *cp;

    /*Constructs the codec params parser*/
    cp = RvSdpCodecParserInternalConstruct(gs_G726_FmtpParserData,gs_G726_AttrsParserData,a);
    if (!cp)
        return RV_SDPSTATUS_ALLOCFAIL;

    /*set the legal names of this codec and the legal separator*/
    cp->iCodecNames[0] = "G726-24";
    cp->iCodecNames[1] = "G726-32";
    cp->iCodecNames[2] = "G726-48";
    cp->iSeparator = ';';

    if (RvSdpCodecRegister(cp) != RV_OK)
    {
        RvSdpCodecParserInternalDestruct(cp);
        return RV_SDPSTATUS_ALLOCFAIL;
    }

    return RV_SDPSTATUS_OK;
}


#endif /* #ifdef RV_SDP_CODEC_G726*/
#endif /* #ifdef RV_SDP_CODECS_SUPPORTED */

 
