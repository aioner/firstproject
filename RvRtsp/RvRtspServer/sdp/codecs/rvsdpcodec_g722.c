/******************************************************************************
Filename    :rvsdpcodec_g722.c
Description : handling g722 codec type.

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

#ifdef RV_SDP_CODEC_G722

/*legal parameters names for this codec*/
typedef enum {
    RV_SDP_CODEC_G722_PTIME,
} RvSdpG722ParamsEnum;

static const RvSdpParamStaticParserData gs_G722_FmtpParserData[] = {
    {-1,                                        NULL},
};

static const RvSdpParamStaticParserData gs_G722_AttrsParserData[] = {
    {RV_SDP_CODEC_G722_PTIME,              "ptime = d 1-5000"},
    {-1,                                        NULL},
};

RvStatus RvSdpCodec_G722_Init(RvAlloc* a)
{
    RvSdpCodecParserInternal *cp;

    /*Constructs the codec params parser*/
    cp = RvSdpCodecParserInternalConstruct(gs_G722_FmtpParserData,gs_G722_AttrsParserData,a);
    if (!cp)
        return RV_SDPSTATUS_ALLOCFAIL;

    /*set the legal names of this codec*/
    cp->iCodecNames[0] = "G722";
    cp->iCodecNames[1] = "G722-64";
    cp->iCodecNames[2] = "G722-56";
    cp->iCodecNames[3] = "G722-48";
    cp->iCodecNames[4] = "G7221";
    cp->iSeparator = ';';

    if (RvSdpCodecRegister(cp) != RV_OK)
    {
        RvSdpCodecParserInternalDestruct(cp);
        return RV_SDPSTATUS_ALLOCFAIL;
    }

    return RV_SDPSTATUS_OK;
}


#endif /* #ifdef RV_SDP_CODEC_G722*/
#endif /* #ifdef RV_SDP_CODECS_SUPPORTED */

