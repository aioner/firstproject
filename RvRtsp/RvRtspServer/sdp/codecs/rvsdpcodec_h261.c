/******************************************************************************
Filename    :rvsdpcodec_h261.c
Description : handling h261 codec type.

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

#ifdef RV_SDP_CODEC_H261

/*legal parameters names for this codec*/
typedef enum {
    RV_SDP_CODEC_H261_QCIF_V,
    RV_SDP_CODEC_H261_CIF_V,
    RV_SDP_CODEC_H261_D_V,
} RvSdpH261FmtpParamsEnum;

/*This array defines the grammar (syntax) of the codec parameters value  
  taken from the FMTP attribute*/
static const RvSdpParamStaticParserData gs_H261_FmtpParsingData[] = {
    {RV_SDP_CODEC_H261_QCIF_V,          "QCIF = d 1-99"},
    {RV_SDP_CODEC_H261_CIF_V,           "CIF = d 1-99"},
    {RV_SDP_CODEC_H261_D_V,             "D"},
    {-1,                                NULL},
};

/*This array defines the grammar (syntax) of the codec parameters value
 that are taken from other attributes attributs*/
static const RvSdpParamStaticParserData gs_H261_AttrsParsingData[] = {
    {-1,                                NULL},
};


typedef enum 
{
    RvSdpH261FmtpParserAtStart,
    RvSdpH261FmtpParserInSize,
    RvSdpH261FmtpParserInAnnex,
} RvSdpH261FmtpParserState;


/*called in RvSdpH261ParsingError()*/
void RvSdpH261TestFmtpSyntax(RvSdpCodecParamsInternal* prms, RvSdpCodecParamData *fmtpData)
{
    int fNum;

    if (prms->iFmtpBadSyntax)
        return;

    fNum = fmtpData->iParamNum;
    switch (prms->iCodecParamsParserState)
    {  
    case RvSdpH261FmtpParserAtStart:
        {
        
            if (fNum == RV_SDP_CODEC_H261_QCIF_V || fNum == RV_SDP_CODEC_H261_CIF_V)
                prms->iCodecParamsParserState = RvSdpH261FmtpParserInSize;
            else if (fNum == RV_SDP_CODEC_H261_D_V)
                prms->iCodecParamsParserState = RvSdpH261FmtpParserInAnnex;
            else 
                prms->iFmtpBadSyntax = RV_TRUE;
        }
        break;
    case RvSdpH261FmtpParserInSize:
        if (fNum != RV_SDP_CODEC_H261_QCIF_V && fNum != RV_SDP_CODEC_H261_CIF_V)
            prms->iFmtpBadSyntax = RV_TRUE;
        break;
    case RvSdpH261FmtpParserInAnnex:
        prms->iFmtpBadSyntax = RV_TRUE;
        break;
    };
}

/*RvSdpCodecParseErrorCB*/
RvSdpStatus RvSdpH261ParsingError(RvSdpCodecParamsInternal* prms, RvSdpCodecParamData *fmtpData, const char* value)
{
    RvSdpH261TestFmtpSyntax(prms,fmtpData);
    RvSdpCodecParamParsingError(prms,fmtpData,value);
    return RV_SDPSTATUS_OK;
}
 
/*RvSdpCodecParseNextParamCB*/
RvSdpStatus RvSdpH261TreateFmtpParam(RvSdpCodecParamsInternal* prms, RvSdpCodecParamData *fmtpData, const char* value)
{
    RvSdpH261TestFmtpSyntax(prms,fmtpData);
    RvSdpCodecParsingNextParam(prms,fmtpData,value);
    return RV_SDPSTATUS_OK;
}

RvSdpStatus RvSdpCodec_H261_Init(RvAlloc* a)
{
    RvSdpCodecParserInternal *cp;

    /*Constructs the codec params parser*/
    cp = RvSdpCodecParserInternalConstruct(gs_H261_FmtpParsingData,gs_H261_AttrsParsingData,a);
    if (!cp)
        return RV_SDPSTATUS_ALLOCFAIL;

    /*set the legal names of this codec and the legal separator*/
    cp->iCodecNames[0] = "H261";
    cp->iCodecNames[1] = "H.261";

    /*setting the callback functions*/
    cp->iParsingErrorCB = (RvSdpCodecParseErrorCB)RvSdpH261ParsingError;
    cp->iFmtpTreatParamCB = (RvSdpCodecParseNextParamCB)RvSdpH261TreateFmtpParam;

    if (RvSdpCodecRegister(cp) != RV_SDPSTATUS_OK)
    {
        RvSdpCodecParserInternalDestruct(cp);
        return RV_SDPSTATUS_ALLOCFAIL;
    }

    return RV_SDPSTATUS_OK;
}

#endif /*RV_SDP_CODEC_H261*/

#endif /*RV_SDP_CODECS_SUPPORTED */

