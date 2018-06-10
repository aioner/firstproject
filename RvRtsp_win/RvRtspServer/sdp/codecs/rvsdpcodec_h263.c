/******************************************************************************
Filename    :rvsdpcodec_h263.c
Description : handling h263 codec type.

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

#ifdef RV_SDP_CODEC_H263

/*legal parameters names for this codec*/
typedef enum {
    RV_SDP_CODEC_H263_SQCIF_V,
    RV_SDP_CODEC_H263_QCIF_V,
    RV_SDP_CODEC_H263_CIF_V,
    RV_SDP_CODEC_H263_CIF4_V,
    RV_SDP_CODEC_H263_CIF16_V,
    RV_SDP_CODEC_H263_XMAX_V,
    RV_SDP_CODEC_H263_YMAX_V,
    RV_SDP_CODEC_H263_MPI_V,
    RV_SDP_CODEC_H263_D_V,
    RV_SDP_CODEC_H263_E_V,
    RV_SDP_CODEC_H263_F_V,
    RV_SDP_CODEC_H263_G_V,
    RV_SDP_CODEC_H263_I_V,
    RV_SDP_CODEC_H263_J_V,
    RV_SDP_CODEC_H263_K_V,
    RV_SDP_CODEC_H263_L_V,
    RV_SDP_CODEC_H263_M_V,
    RV_SDP_CODEC_H263_N_V,
    RV_SDP_CODEC_H263_O_V,
    RV_SDP_CODEC_H263_P_V,
    RV_SDP_CODEC_H263_Q_V,
    RV_SDP_CODEC_H263_R_V,
    RV_SDP_CODEC_H263_S_V,
    RV_SDP_CODEC_H263_T_V,
    RV_SDP_CODEC_H263_PAR_V,
    RV_SDP_CODEC_H263_CPCF_V,
    RV_SDP_CODEC_H263_MAXBR_V,
    RV_SDP_CODEC_H263_BPP_V,
    RV_SDP_CODEC_H263_HRD_V,
    RV_SDP_CODEC_H263_INTERLACED_V
} RvSdpH263FmtpParamsEnum;

/*This array defines the grammar (syntax) of the codec parameters value  
  taken from the FMTP attribute*/
static const RvSdpParamStaticParserData gs_H263_FmtpParsingData[] = {
    {RV_SDP_CODEC_H263_SQCIF_V,         "SQCIF = d 1-99"}, 
    {RV_SDP_CODEC_H263_QCIF_V,          "QCIF = d 1-99"},
    {RV_SDP_CODEC_H263_CIF_V,           "CIF = d 1-99"},
    {RV_SDP_CODEC_H263_CIF4_V,          "CIF4 = d 1-99"},
    {RV_SDP_CODEC_H263_CIF16_V,         "CIF16 = d 1-99"},
    {RV_SDP_CODEC_H263_XMAX_V,          "XMAX = d 1-9999"},
    {RV_SDP_CODEC_H263_YMAX_V,          "YMAX = d 1-9999"},
    {RV_SDP_CODEC_H263_MPI_V,           "MPI = d 1-99"},
    {RV_SDP_CODEC_H263_D_V,             "D = l, d 1-2"},
    {RV_SDP_CODEC_H263_E_V,             "E"},
    {RV_SDP_CODEC_H263_F_V,             "F"},
    {RV_SDP_CODEC_H263_G_V,             "G"},
    {RV_SDP_CODEC_H263_I_V,             "I"},
    {RV_SDP_CODEC_H263_J_V,             "J"},
    {RV_SDP_CODEC_H263_K_V,             "K = l, d 1-4"},
    {RV_SDP_CODEC_H263_L_V,             "L = l, d 1-7"},
    {RV_SDP_CODEC_H263_M_V,             "M"},
    {RV_SDP_CODEC_H263_N_V,             "N = l, d 1-4"},
    {RV_SDP_CODEC_H263_O_V,             "O = l, d 1-3"},
    {RV_SDP_CODEC_H263_P_V,             "P"},
    {RV_SDP_CODEC_H263_Q_V,             "Q"},
    {RV_SDP_CODEC_H263_R_V,             "R"},
    {RV_SDP_CODEC_H263_S_V,             "S"},
    {RV_SDP_CODEC_H263_T_V,             "T"},
    {RV_SDP_CODEC_H263_PAR_V,           "PAR = d 1-999 t \":\" d 1-0"}, 
    {RV_SDP_CODEC_H263_CPCF_V,          "CPCF = d 1-99.3"},
    {RV_SDP_CODEC_H263_MAXBR_V,         "MAXBR = d 1-99999"},
    {RV_SDP_CODEC_H263_BPP_V,           "BPP = d 1-99999"},
    {RV_SDP_CODEC_H263_HRD_V,           "HRD"},
    {RV_SDP_CODEC_H263_INTERLACED_V,    "INTERLACED"},
    {-1,                                NULL},
};

/*no attr parsing data*/
static const RvSdpParamStaticParserData gs_H263_AttrsParsingData[] = {
    {-1,                                NULL},
};


typedef enum 
{
    RvSdpH263FmtpParserAtStart,
    RvSdpH263FmtpParserInSize,
    RvSdpH263FmtpParserAfterXMax,
    RvSdpH263FmtpParserAfterYMax,
    RvSdpH263FmtpParserInAnnex,
    RvSdpH263FmtpParserInParams
} RvSdpH263FmtpParserState;

/*RvSdpCodecParseFinishCB-will be called when parsing after the work ends */
RvSdpStatus RvSdpH263FinishParsing(RvSdpCodecParamsInternal* prms)
{        
    if (prms->iCodecParamsParserState == RvSdpH263FmtpParserAfterXMax ||
        prms->iCodecParamsParserState == RvSdpH263FmtpParserAfterYMax)
    {
        prms->iFmtpBadSyntax = RV_TRUE;
    }
    
    return RV_SDPSTATUS_OK;
}


void RvSdpH263TestFmtpSyntax(RvSdpCodecParamsInternal* prms, RvSdpCodecParamData *fmtpData)
{
    int fNum;

    if (prms->iFmtpBadSyntax)
        return;

    fNum = fmtpData->iParamNum;
    switch (prms->iCodecParamsParserState)
    {  
    case RvSdpH263FmtpParserAtStart:
        {
        
            if (fNum >= RV_SDP_CODEC_H263_SQCIF_V && fNum <= RV_SDP_CODEC_H263_CIF16_V)
                prms->iCodecParamsParserState = RvSdpH263FmtpParserInSize;
            else if (fNum == RV_SDP_CODEC_H263_XMAX_V)
                prms->iCodecParamsParserState = RvSdpH263FmtpParserAfterXMax;
            else if (fNum >= RV_SDP_CODEC_H263_D_V && fNum <= RV_SDP_CODEC_H263_T_V)
                prms->iCodecParamsParserState = RvSdpH263FmtpParserInAnnex;
            else if (fNum >= RV_SDP_CODEC_H263_PAR_V && fNum <= RV_SDP_CODEC_H263_INTERLACED_V)
                prms->iCodecParamsParserState = RvSdpH263FmtpParserInParams;
            else 
                prms->iFmtpBadSyntax = RV_TRUE;
        }
        break;
    case RvSdpH263FmtpParserInSize:
        if (fNum == RV_SDP_CODEC_H263_XMAX_V)
            prms->iCodecParamsParserState = RvSdpH263FmtpParserAfterXMax;
        else if (fNum < RV_SDP_CODEC_H263_SQCIF_V || fNum > RV_SDP_CODEC_H263_CIF16_V)
            prms->iFmtpBadSyntax = RV_TRUE;
        break;
    case RvSdpH263FmtpParserAfterXMax:
        if (fNum == RV_SDP_CODEC_H263_YMAX_V)
            prms->iCodecParamsParserState = RvSdpH263FmtpParserAfterYMax;
        else 
            prms->iFmtpBadSyntax = RV_TRUE;
        break;
    case RvSdpH263FmtpParserAfterYMax:
        if (fNum == RV_SDP_CODEC_H263_MPI_V)
            prms->iCodecParamsParserState = RvSdpH263FmtpParserInSize;
        else 
            prms->iFmtpBadSyntax = RV_TRUE;
        break;
    case RvSdpH263FmtpParserInAnnex:
        if (fNum < RV_SDP_CODEC_H263_D_V || fNum > RV_SDP_CODEC_H263_T_V)
            prms->iFmtpBadSyntax = RV_TRUE;
        break;
    case RvSdpH263FmtpParserInParams:
        if (fNum < RV_SDP_CODEC_H263_PAR_V || fNum > RV_SDP_CODEC_H263_INTERLACED_V)
            prms->iFmtpBadSyntax = RV_TRUE;
        break;        
    };
}

/*RvSdpCodecParseErrorCB - will be called when parsing when the parameter 
  value does not match the predefined pattern*/
RvSdpStatus RvSdpH263ParsingError(RvSdpCodecParamsInternal* prms, RvSdpCodecParamData *fmtpData, const char* value)
{
    RvSdpH263TestFmtpSyntax(prms,fmtpData);
    RvSdpCodecParamParsingError(prms,fmtpData,value);
    return RV_SDPSTATUS_OK;
}
 
/*RvSdpCodecParseNextParamCB - will be called when parsing for each successfully parsed parameter*/
RvSdpStatus RvSdpH263TreateFmtpParam(RvSdpCodecParamsInternal* prms, RvSdpCodecParamData *fmtpData, const char* value)
{
    RvSdpH263TestFmtpSyntax(prms,fmtpData);
    RvSdpCodecParsingNextParam(prms,fmtpData,value);
    return RV_SDPSTATUS_OK;
}

RvSdpStatus RvSdpCodec_H263_Init(RvAlloc* a)
{
    RvSdpCodecParserInternal *cp;
    
    /*Constructs the codec params parser*/
    cp = RvSdpCodecParserInternalConstruct(gs_H263_FmtpParsingData,gs_H263_AttrsParsingData,a);
    if (!cp)
        return RV_SDPSTATUS_ALLOCFAIL;

    /*set the legal names of this codec*/
    cp->iCodecNames[0] = "H263";
    cp->iCodecNames[1] = "H.263";
    
    /*setting the callback functions*/
    cp->iFinishFmtpParsingCB = (RvSdpCodecParseFinishCB) RvSdpH263FinishParsing;
    cp->iParsingErrorCB = (RvSdpCodecParseErrorCB) RvSdpH263ParsingError;
    cp->iFmtpTreatParamCB = (RvSdpCodecParseNextParamCB) RvSdpH263TreateFmtpParam;

    if (RvSdpCodecRegister(cp) != RV_SDPSTATUS_OK)
    {
        RvSdpCodecParserInternalDestruct(cp);
        return RV_SDPSTATUS_ALLOCFAIL;
    }

    return RV_SDPSTATUS_OK;
}


#endif /* #ifdef RV_SDP_CODEC_H263 */
#endif /* #ifdef RV_SDP_CODECS_SUPPORTED*/
 
