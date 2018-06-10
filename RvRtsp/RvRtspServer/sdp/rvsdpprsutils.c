/******************************************************************************
Filename    :rvsdpprsutils.c
Description : SDP parse routines.

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
    ******************************************************************************
Author:Rafi Kiel
******************************************************************************/
#include <string.h>
#include "rvsdpprivate.h"
#include "rvsdpprsutils.h"
#include "rvstrutils.h"

#include <stdio.h>
#include <ctype.h>

/*
 *	This is array of 128 entries. Each entry is either 0 or 'OR'ed combination of 
 *  different masks. Each mask corresponds to some characters set good for some SDP 
 *  field. For example by mask 'M_BWT' marked symbols ('+','-' digits and letters)
 *  that are legal for bandwidth type field ib 'b=' lines of SDP input.
 *  Thus in order to test that some symbol 'c' is valid banddith type we can say:
 *              if (gsSymbolMsks[c] & M_BWT)
 *  instead of 
 *  if (c=='+'||c=='-'||(c>='0'&&c<='9')||(c>='a'&&c<='z')||(c>='A'&&c<='Z'))
 * 
 *  This array is used in rvSdpParseText2 and rvSdpParseText functions and different
 *  macros like : RV_SDP_IS_HEX or RV_SDP_IS_ALPHA
 *
 * Here the list of masks used for different SDP fields:

  M_SF    a-z,A-Z,0-9,'\'','-','.','/',':','?','"','#','$','&','*',';','=','@',
                      '[',']','^','_','`','{','|','}','+','~'
  M_H     a-f,A-F,0-9
  M_A     a-z,A-Z
  M_D     0-9
  M_AD    a-z,A-Z,0-9
  M_PD    1-9
  M_PH    0-9,'-',' ','(',')'
  M_ADH   a-z,A-Z,0-9,'-'
  M_MD    a-z,A-Z,0-9,'_','/'
  M_PR    a-z,A-Z,0-9,'_','/','-'
  M_FMT   a-z,A-Z,0-9,'/','-','.'
  M_BWT   a-z,A-Z,0-9,'-','+'
  M_D_SL  a-z,A-Z,0-9,'/'
  M_ADDR  a-z,A-Z,0-9,'-','_'
  M_B64   a-z,A-Z,0-9,'=','+','/'    
  
 */
const unsigned int gsSymbolMsks[] = 
{
/*  0    */ 0,
/*  1    */ 0,
/*  2    */ 0,
/*  3    */ 0,
/*  4    */ 0,
/*  5    */ 0,
/*  6    */ 0,
/*  7    */ 0,
/*  8    */ 0,
/*  9    */ 0,
/* 10    */ 0,
/* 11    */ 0,
/* 12    */ 0,
/* 13    */ 0,
/* 14    */ 0,
/* 15    */ 0,
/* 16    */ 0,
/* 17    */ 0,
/* 18    */ 0,
/* 19    */ 0,
/* 20    */ 0,
/* 21    */ 0,
/* 22    */ 0,
/* 23    */ 0,
/* 24    */ 0,
/* 25    */ 0,
/* 26    */ 0,
/* 27    */ 0,
/* 28    */ 0,
/* 29    */ 0,
/* 30    */ 0,
/* 31    */ 0,
/* 32    */ 0 | M_PH,
/* 33  ! */ 0,
/* 34  " */ 0 | M_SF,
/* 35  # */ 0 | M_SF,
/* 36  $ */ 0 | M_SF,
/* 37  % */ 0,
/* 38  & */ 0 | M_SF,
/* 39  ' */ 0 | M_SF,
/* 40  ( */ 0 | M_PH,
/* 41  ) */ 0 | M_PH,
/* 42  * */ 0 | M_SF,
/* 43  + */ 0 | M_SF | M_BWT | M_B64,
/* 44  , */ 0,
/* 45  - */ 0 | M_SF | M_PH | M_ADH | M_PR | M_FMT | M_BWT | M_ADDR,
/* 46  . */ 0 | M_SF | M_FMT,
/* 47  / */ 0 | M_SF | M_MD | M_PR | M_FMT | M_D_SL | M_B64,
/* 48  0 */ 0 | M_SF | M_H | M_D | M_AD | M_PH | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_D_SL | M_ADDR | M_B64,
/* 49  1 */ 0 | M_SF | M_H | M_D | M_AD | M_PD | M_PH | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_D_SL | M_ADDR | M_B64,
/* 50  2 */ 0 | M_SF | M_H | M_D | M_AD | M_PD | M_PH | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_D_SL | M_ADDR | M_B64,
/* 51  3 */ 0 | M_SF | M_H | M_D | M_AD | M_PD | M_PH | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_D_SL | M_ADDR | M_B64,
/* 52  4 */ 0 | M_SF | M_H | M_D | M_AD | M_PD | M_PH | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_D_SL | M_ADDR | M_B64,
/* 53  5 */ 0 | M_SF | M_H | M_D | M_AD | M_PD | M_PH | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_D_SL | M_ADDR | M_B64,
/* 54  6 */ 0 | M_SF | M_H | M_D | M_AD | M_PD | M_PH | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_D_SL | M_ADDR | M_B64,
/* 55  7 */ 0 | M_SF | M_H | M_D | M_AD | M_PD | M_PH | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_D_SL | M_ADDR | M_B64,
/* 56  8 */ 0 | M_SF | M_H | M_D | M_AD | M_PD | M_PH | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_D_SL | M_ADDR | M_B64,
/* 57  9 */ 0 | M_SF | M_H | M_D | M_AD | M_PD | M_PH | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_D_SL | M_ADDR | M_B64,
/* 58  : */ 0 | M_SF,
/* 59  ; */ 0 | M_SF,
/* 60  < */ 0,
/* 61  = */ 0 | M_SF | M_B64,
/* 62  > */ 0,
/* 63  ? */ 0 | M_SF,
/* 64  @ */ 0 | M_SF,
/* 65  A */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 66  B */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 67  C */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 68  D */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 69  E */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 70  F */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 71  G */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 72  H */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 73  I */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 74  J */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 75  K */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 76  L */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 77  M */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 78  N */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 79  O */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 80  P */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 81  Q */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 82  R */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 83  S */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 84  T */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 85  U */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 86  V */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 87  W */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 88  X */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 89  Y */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 90  Z */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 91  [ */ 0 | M_SF,
/* 92  \ */ 0,
/* 93  ] */ 0 | M_SF,
/* 94  ^ */ 0 | M_SF,
/* 95  _ */ 0 | M_SF | M_MD | M_PR | M_ADDR,
/* 96  ` */ 0 | M_SF,
/* 97  a */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 98  b */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/* 99  c */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*100  d */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*101  e */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*102  f */ 0 | M_SF | M_H | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*103  g */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*104  h */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*105  i */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*106  j */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*107  k */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*108  l */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*109  m */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*110  n */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*111  o */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*112  p */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*113  q */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*114  r */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*115  s */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*116  t */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*117  u */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*118  v */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*119  w */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*120  x */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*121  y */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*122  z */ 0 | M_SF | M_A | M_AD | M_ADH | M_MD | M_PR | M_FMT | M_BWT | M_ADDR | M_B64,
/*123  { */ 0 | M_SF,
/*124  | */ 0 | M_SF,
/*125  } */ 0 | M_SF,
/*126  ~ */ 0 | M_SF,
/*127    */ 0,
};


RvSdpNetType rvSdpNetTypeTxt2Val(char* p)
{
	if (!p || !*p)
		return RV_SDPNETTYPE_UNKNOWN;
    else if (!strcasecmp(p,"IN"))
        return RV_SDPNETTYPE_IN;
    if (!strcasecmp(p,"ATM"))
        return RV_SDPNETTYPE_ATM;
    if (!strcasecmp(p,"LOCAL"))
        return RV_SDPNETTYPE_LOCAL;
    if (!strcasecmp(p,"Other"))
        return RV_SDPNETTYPE_OTHER;
    if (!strcasecmp(p,"Tn"))
        return RV_SDPNETTYPE_TN;    
    else if (strcasecmp(p,"$"))
        return RV_SDPNETTYPE_ANY;
    else if (strcasecmp(p,"-"))
        return RV_SDPNETTYPE_IGNORE;
    else
        return RV_SDPNETTYPE_UNKNOWN;
}

const char* rvSdpConnModeVal2Txt(RvSdpConnectionMode m)
{
    switch (m)
    {
        case RV_SDPCONNECTMODE_SENDONLY:
            return "sendonly";
        case RV_SDPCONNECTMODE_RECVONLY:
            return "recvonly";
        case RV_SDPCONNECTMODE_SENDRECV:
            return "sendrecv";
        case RV_SDPCONNECTMODE_INACTIVE:
            return "inactive";
        default:
            return "";
    }
}

RvSdpConnectionMode rvSdpConnModeTxt2Val(const char* txt)
{
	if (!txt || !*txt)
		return RV_SDPCONNECTMODE_NOTSET;
    else if (!strncasecmp(txt,"sendonly",8))
        return RV_SDPCONNECTMODE_SENDONLY;
    else if (!strncasecmp(txt,"recvonly",8))
        return RV_SDPCONNECTMODE_RECVONLY;
    else if (!strncasecmp(txt,"sendrecv",8))
        return RV_SDPCONNECTMODE_SENDRECV;
    else if (!strncasecmp(txt,"inactive",8))
        return RV_SDPCONNECTMODE_INACTIVE;
    else
        return RV_SDPCONNECTMODE_NOTSET;
}

const char* rvSdpNetTypeVal2Txt(RvSdpNetType t)
{
    switch (t)
    {
    case RV_SDPNETTYPE_IN:
        return "IN";
    case RV_SDPNETTYPE_ATM:
        return "ATM";
    case RV_SDPNETTYPE_LOCAL:
        return "LOCAL";
    case RV_SDPNETTYPE_OTHER:
        return "Other";
    case RV_SDPNETTYPE_TN:
        return "Tn";
    case RV_SDPNETTYPE_ANY:
        return "$";
    case RV_SDPNETTYPE_IGNORE:
        return "-";
    case RV_SDPNETTYPE_UNKNOWN:
    default:
        return "Unknown";
    }
}

RvSdpAddrType rvSdpAddrTypeTxt2Val(char* p)
{
	if (!p || !*p)
		return RV_SDPADDRTYPE_UNKNOWN;
    else if (!strcasecmp(p,"IP4"))
        return RV_SDPADDRTYPE_IP4;
    else if (!strcasecmp(p,"IP6"))
        return RV_SDPADDRTYPE_IP6;
    else if (!strcasecmp(p,"epn"))
        return RV_SDPADDRTYPE_ENDPOINT;
    else if (!strcasecmp(p,"nsap"))
        return RV_SDPADDRTYPE_NSAP;
    else if (!strcasecmp(p,"e164"))
        return RV_SDPADDRTYPE_E164;
    else if (!strcasecmp(p,"GWID"))
        return RV_SDPADDRTYPE_GWID;
    else if (!strcasecmp(p,"alias"))
        return RV_SDPADDRTYPE_ALIAS;
    else if (!strcasecmp(p,"rfc2543"))
        return RV_SDPADDRTYPE_RFC2543;
    else if (!strcasecmp(p,"$"))
        return RV_SDPADDRTYPE_ANY; 
    else if (!strcasecmp(p,"-"))
        return RV_SDPADDRTYPE_IGNORE;
    else
        return RV_SDPADDRTYPE_UNKNOWN;
}

const char* rvSdpAddrTypeVal2Txt(RvSdpAddrType t)
{
    switch (t)
    {
    case RV_SDPADDRTYPE_IP4:
        return "IP4";
    case RV_SDPADDRTYPE_IP6:
        return "IP6";
    case RV_SDPADDRTYPE_ENDPOINT:
        return "epn";
    case RV_SDPADDRTYPE_NSAP:
        return "nsap";
    case RV_SDPADDRTYPE_E164:
        return "e164";
    case RV_SDPADDRTYPE_GWID:
        return "GWID";
    case RV_SDPADDRTYPE_ALIAS:
        return "alias";
    case RV_SDPADDRTYPE_RFC2543:
        return "rfc2543";
    case RV_SDPADDRTYPE_ANY:
        return "$";
    case RV_SDPADDRTYPE_IGNORE:
        return "-";
    case RV_SDPADDRTYPE_UNKNOWN:
    default:
        return "Unknown";
    }
}


RvSdpEncrMethod rvSdpKeyTypeTxt2Val(const char *txt)
{
	if (!txt || !*txt)
		return RV_SDPENCRMTHD_UNKNOWN;
    else if (!strcasecmp(txt,"clear"))
        return RV_SDPENCRMTHD_CLEAR;
    else if (!strcasecmp(txt,"base64"))
        return RV_SDPENCRMTHD_BASE64;
    else if (!strcasecmp(txt,"uri"))
        return RV_SDPENCRMTHD_URI;
    else if (!strcasecmp(txt,"prompt"))
        return RV_SDPENCRMTHD_PROMPT;
    else if (!strcasecmp(txt,"key"))
        return RV_SDPENCRMTHD_KEY;    
    else 
        return RV_SDPENCRMTHD_UNKNOWN;
}

const char* rvSdpKeyTypeVal2Txt(RvSdpEncrMethod m)
{
    switch (m)
    {
    case RV_SDPENCRMTHD_CLEAR:
        return "clear";
    case RV_SDPENCRMTHD_BASE64:
        return "base64";
    case RV_SDPENCRMTHD_URI:
        return "uri";
    case RV_SDPENCRMTHD_PROMPT:
        return "prompt";
    case RV_SDPENCRMTHD_KEY:
        return "key";        
    case RV_SDPENCRMTHD_UNKNOWN:
    default:
        return "Unknown";
    }
}

RvSdpMediaType rvSdpMediaTypeTxt2Val(const char* mediaTxt)
{
	if (!mediaTxt || !*mediaTxt)
		return RV_SDPMEDIATYPE_UNKNOWN;
    else if (!strcasecmp(mediaTxt,"audio"))
        return RV_SDPMEDIATYPE_AUDIO;
    else if (!strcasecmp(mediaTxt,"nas"))
        return RV_SDPMEDIATYPE_NAS;
    else if (!strcasecmp(mediaTxt,"video"))
        return RV_SDPMEDIATYPE_VIDEO;
    else if (!strcasecmp(mediaTxt,"application"))
        return RV_SDPMEDIATYPE_APP;
    else if (!strcasecmp(mediaTxt,"data"))
        return RV_SDPMEDIATYPE_DATA;
    else if (!strcasecmp(mediaTxt,"image"))
        return RV_SDPMEDIATYPE_IMAGE;
    else if (!strcasecmp(mediaTxt,"control"))
        return RV_SDPMEDIATYPE_CONTROL;
    else 
        return RV_SDPMEDIATYPE_UNKNOWN;
}

const char* rvSdpMediaTypeVal2Txt(RvSdpMediaType m)
{
    switch (m)
    {
    case RV_SDPMEDIATYPE_AUDIO:
        return "audio";
    case RV_SDPMEDIATYPE_NAS:
        return "nas";
    case RV_SDPMEDIATYPE_VIDEO:
        return "video";
    case RV_SDPMEDIATYPE_APP:
        return "application";
    case RV_SDPMEDIATYPE_DATA:
        return "data";
    case RV_SDPMEDIATYPE_IMAGE:
        return "image";
    case RV_SDPMEDIATYPE_CONTROL:
        return "control";
    case RV_SDPMEDIATYPE_UNKNOWN:
    default:
        return "Unknown";
    }
}


RvSdpProtocol rvSdpMediaProtoTxt2Val(const char* protoTxt)
{
	if (!protoTxt || !*protoTxt)
		return RV_SDPPROTOCOL_UNKNOWN;
    else if (!strcasecmp("RTP/AVP",protoTxt))
        return RV_SDPPROTOCOL_RTP;
    else if(!strcasecmp("RTP/SAVP",protoTxt))         
        return RV_SDPPROTOCOL_RTP_SAVP;    
    else if(!strcasecmp("LOCAL",protoTxt))           
        return RV_SDPPROTOCOL_LOCAL;
    else if(!strcasecmp("ATM/AVP",protoTxt))         
        return RV_SDPPROTOCOL_ATM;
    else if(!strcasecmp("udptl",protoTxt))				
        return RV_SDPPROTOCOL_UDP_T38;
    else if(!strcasecmp("udp",protoTxt))             
        return RV_SDPPROTOCOL_UDP;
    else if(!strcasecmp("tcp",protoTxt))             
        return RV_SDPPROTOCOL_TCP;
    else if(!strcasecmp("AAL1/ATMF",protoTxt))       
        return RV_SDPPROTOCOL_AAL1ATMF;
    else if(!strcasecmp("AAL1/ITU",protoTxt))        
        return RV_SDPPROTOCOL_AAL1ITU;
    else if(!strcasecmp("AAL1/custom",protoTxt))     
        return RV_SDPPROTOCOL_AAL1CUSTOM;
    else if(!strcasecmp("AAL2/ATMF",protoTxt))       
        return RV_SDPPROTOCOL_AAL2ATMF;
    else if(!strcasecmp("AAL2/ITU",protoTxt))        
        return RV_SDPPROTOCOL_AAL2ITU;
    else if(!strcasecmp("AAL2/custom",protoTxt))     
        return RV_SDPPROTOCOL_AAL2CUSTOM;
    else if(!strcasecmp("AAL5/ATMF",protoTxt))       
        return RV_SDPPROTOCOL_AAL5ATMF;
    else if(!strcasecmp("AAL5/ITU",protoTxt))        
        return RV_SDPPROTOCOL_AAL5ITU;
    else if(!strcasecmp("AAL5/custom",protoTxt))     
        return RV_SDPPROTOCOL_AAL5CUSTOM;
    else if(!strcasecmp("H323c",protoTxt))           
        return RV_SDPPROTOCOL_H323C;
    else 
        return RV_SDPPROTOCOL_UNKNOWN;
}

const char* rvSdpMediaProtoVal2Txt(RvSdpProtocol p)
{
    switch (p)
    {
    case RV_SDPPROTOCOL_RTP:
        return "RTP/AVP";
    case RV_SDPPROTOCOL_RTP_SAVP:
        return "RTP/SAVP";
    case RV_SDPPROTOCOL_LOCAL:
        return "LOCAL";
    case RV_SDPPROTOCOL_ATM:
        return "ATM/AVP";
    case RV_SDPPROTOCOL_UDP_T38:
        return "udptl";
    case RV_SDPPROTOCOL_UDP:
        return "udp";
    case RV_SDPPROTOCOL_TCP:
        return "tcp";
    case RV_SDPPROTOCOL_AAL1ATMF:
        return "AAL1/ATMF";
    case RV_SDPPROTOCOL_AAL1ITU:
        return "AAL1/ITU";
    case RV_SDPPROTOCOL_AAL1CUSTOM:
        return "AAL1/custom";
    case RV_SDPPROTOCOL_AAL2ATMF:
        return "AAL2/ATMF";
    case RV_SDPPROTOCOL_AAL2ITU:
        return "AAL2/ITU";
    case RV_SDPPROTOCOL_AAL2CUSTOM:
        return "AAL2/custom";
    case RV_SDPPROTOCOL_AAL5ATMF:
        return "AAL5/ATMF";
    case RV_SDPPROTOCOL_AAL5ITU:
        return "AAL5/ITU";
    case RV_SDPPROTOCOL_AAL5CUSTOM:
        return "AAL5/custom";
    case RV_SDPPROTOCOL_H323C:
        return "H323c";
    case RV_SDPPROTOCOL_UNKNOWN:
    default:
        return "Unknown";
    }
}


/*
 *	Parses the integer number. Moves the '*ptr' to a position after the number.
 *  Returns RV_TRUE if successfull, otherwise returns RV_FALSE and the 'w' is set
 *  to correspondent value.
 */

RvBool rvSdpParseInt(
        RvInt minNum,               /* min possible number */ 
        RvUint maxNum,              /* max possible number */
                                    /* if the range is not important set minNum and 
                                       maxNum to zero values */
        RvUint* res,                /* the parsed number value will be set here */ 
        RvBool allowLeadingZero,    /* whether the leading zeros are allowed */
        RvBool isHex,               /* whetherthe number is hexadecimal or decimal */
        RvChar** ptr,               /* the start of the number, the '*ptr' will be moved
                                       to a position after the number end */
        RvSdpIntegerParseWarning* w)    /* the possible failure  reason will be set here */
{
    RvUint num = 0, num1, num2;
    RvChar *ps = *ptr;
    RvBool rangeCheckNeeded =  minNum || maxNum || res;

    while (RV_SDP_IS_DIGIT((int)**ptr) || (isHex && RV_SDP_IS_HEX((int)**ptr)))
    {
        if (**ptr == '0' && ps == *ptr && !allowLeadingZero)
        {
            if (w)
                *w = RvSdpWarnNonZeroDigitExpected;
            return RV_FALSE;
        }

        if (rangeCheckNeeded)
        {
            num1 = num;
            if (isHex)
                num <<= 4;
            else
            {
                num2 = num << 3;
                num *= 10;
            }

            if ((isHex&&num>>4 != num1) || (!isHex&&num/10 != num1))
            {
                if (w)
                    *w = RvSdpWarnNumberOverflow;
                return RV_FALSE;
            }
            

            if (!RV_SDP_IS_DIGIT((int)**ptr))
                num += 10 + tolower(**ptr) - 'a';
            else
                num += (**ptr-'0');
        }

        (*ptr)++;
    }


    if (ps == *ptr)
    {
        if (w)
            *w = RvSdpWarnDigitExpected;
        return RV_FALSE;
    }

    if (num < (RvUint)minNum || (maxNum>0 && num > (RvUint)maxNum))
    {
        if (w)
            *w = RvSdpWarnNumberIsNotInRange;
        return RV_FALSE;
    }

    if (res)
        *res = num;

    return RV_TRUE;

}

/* 
 * Moves '*ptr' to the next space, tab, 'c1' symbol (if 'c1' is not zero) or the end of
 * line.
 * RV_TRUE is returned if '*ptr' could be moved (there were some printable, non-space 
 * characters) or RV_FALSE otherwise. 
 */
RvBool rvSdpParseNonSpace(RvChar** ptr, char c1)
{
    char *ps = *ptr;    

    while (**ptr != ' ' && **ptr != '\t' && 0x21<=**ptr && **ptr<=0x7e && (c1==0 || **ptr != c1))
        (*ptr)++;
    
    if (*ptr == ps)
        return RV_FALSE;    
    
    return RV_TRUE;    
}

/*
 * Moves '*ptr' to the end of buffer.
 * RV_TRUE is returned if '*ptr' could be moved or RV_FALSE otherwise. 
 */
RvBool rvSdpParseAnyText(RvChar** ptr)
{
    char *ps = *ptr;    
    
    while (**ptr)
        (*ptr)++;
    
    if (*ptr == ps)
        return RV_FALSE;    
    
    return RV_TRUE;    
}



/*
 * Moves '*ptr' to next character different from the one defined by 'msk' in 
 * 'gsSymbolMsks' array or to the end of buffer.
 * RV_TRUE is returned if '*ptr' could be moved (there were some symbols marked
 * by 'msk' in 'gsSymbolMsks' array) or RV_FALSE otherwise. 
 */
RvBool rvSdpParseText(RvChar** ptr, RvUint msk)
{
    return rvSdpParseText2(ptr,msk,(char)-1);
}

/*
 * Moves '*ptr' to next character different from the one defined by 'msk' in 
 * 'gsSymbolMsks' array or 'c1' character OR to the end of buffer.
 * RV_TRUE is returned if '*ptr' could be moved (there were some symbols marked
 * by 'msk' in 'gsSymbolMsks' array or 'c1' character) or RV_FALSE otherwise. 
 */
RvBool rvSdpParseText2(RvChar** ptr, RvUint msk, char c1)
{
    
    char *ps = *ptr;
    
    for (;;)
    {
        if ((unsigned)**ptr>127 || !(gsSymbolMsks[(int)**ptr]&msk || **ptr == c1))
            break;
        (*ptr)++;
        
    }
    
    if (*ptr == ps)
        return RV_FALSE;    
    
    return RV_TRUE;
    
}


/*
 * Moves '*ptr' to the next non-space character or to the end of the buffer.
 * RV_TRUE is returned if '*ptr' could be moved (there  were some spaces) or 
 * RV_FALSE otherwise. 
 */
RvBool rvSdpParseSpace(RvChar** ptr)
{
    RvChar *ps;
    
    ps = *ptr;
    
    while (**ptr == ' ' || **ptr == '\t')
        (*ptr)++;
    
    if (*ptr == ps)
        return RV_FALSE;    
    
    return RV_TRUE;
}

#define ENCODE_TAG_VALUE(_t,_v,_p,_pE) \
{\
    if (_v && *_v)\
    {\
        int _l = strlen(_v);\
        char *ptr = *_p;\
        if (_pE - *_p < _l+4)\
            return RV_FALSE;\
        *ptr++ = _t;\
        *ptr++ = '=';\
        memcpy(ptr,_v,_l);\
        ptr += _l;\
        *ptr++ = '\r';\
        *ptr++ = '\n';\
        *p = ptr;\
    }\
}

#define ENCODE_TEXT(_s,_v,_l,_ptr)\
if (_v && *_v)\
{\
    *_ptr++ = _s;\
    memcpy(_ptr,_v,_l);\
    _ptr += _l;\
}

/*
 *	Encodes the SDP version line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeVersion(char **p, char *pEnd, char* v)
{    
    ENCODE_TAG_VALUE('v',v,p,pEnd)
    return RV_TRUE;
}

/*
 *	Encodes the SDP origin line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeOrigin(char **p, char *pEnd, RvSdpOrigin *o)
{    
    int l1,l2,l3,l4,l5,l6;
    char *p4, *p5;
    char *ptr = *p;

#if defined(RV_SDP_CHECK_BAD_SYNTAX)    
    if (o->iBadSyntaxField) 
    {
        ENCODE_TAG_VALUE('o',o->iBadSyntaxField,p,pEnd)
        return RV_TRUE;
    }
#endif

    l1 = (o->iUserName) ? strlen(o->iUserName):0;
    l2 = (o->iSessionId) ? strlen(o->iSessionId):0;
    l3 = (o->iVersion) ? strlen(o->iVersion):0;
    p4 = rvSdpOriginGetNetTypeStr(o);
    l4 = strlen(p4);
    p5 = rvSdpOriginGetAddressTypeStr(o);
    l5 = strlen(p5);
    l6 = (o->iAddress) ? strlen(o->iAddress):0;

    if (pEnd - ptr < 1+1+l1+1+l2+1+l3+1+l4+1+l5+1+l6+1+1)
        return RV_FALSE;

    *ptr++ = 'o';
    ENCODE_TEXT('=',o->iUserName,l1,ptr)
    ENCODE_TEXT(' ',o->iSessionId,l2,ptr)
    ENCODE_TEXT(' ',o->iVersion,l3,ptr)
    ENCODE_TEXT(' ',p4,l4,ptr)
    ENCODE_TEXT(' ',p5,l5,ptr)
    ENCODE_TEXT(' ',o->iAddress,l6,ptr)
    *ptr++ = '\r';        
    *ptr++ = '\n';                
    *p = ptr;
    return RV_TRUE;
}

/*
 *	Encodes the SDP session name line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeSessionName(char **p, char *pEnd, char *n)
{
    ENCODE_TAG_VALUE('s',n,p,pEnd)
    return RV_TRUE;
}

/*
 *	Encodes the SDP session information line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeSessionInfo(char **p, char *pEnd, char *i)
{
    ENCODE_TAG_VALUE('i',i,p,pEnd)
    return RV_TRUE;
}

/*
 *	Encodes the SDP URI line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeURI(char **p, char *pEnd, char *u)
{
    ENCODE_TAG_VALUE('u',u,p,pEnd)
    return RV_TRUE;
}

/*
 *	Encodes the SDP email line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeEmail(char **p, char *pEnd, RvSdpEmail *e)
{
    int l1,l2;
    char *p1, *p2, c1, c2;
    char *ptr = *p;
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)    
    if (e->iBadSyntaxField)
    { 
        ENCODE_TAG_VALUE('e',e->iBadSyntaxField,p,pEnd)
        return RV_TRUE;
    }
#endif      

    p1 = e->iAddress;
    p2 = e->iText;
    c1 = '(';
    c2 = ')';
    if (e->iSeparSymbol == '<')
    {
        p1 = e->iText;
        p2 = e->iAddress;
        c1 = '<'; c2 = '>';
    }
    else if (e->iSeparSymbol != '(')
        p2 = NULL;

    l1 = (p1) ? strlen(p1) : 0;
    l2 = (p2) ? strlen(p2) : 0;
    
    if (pEnd - ptr < 1+1+l1+1+l2+1+1+1)
        return RV_FALSE;
    
    *ptr++ = 'e';
    ENCODE_TEXT('=',p1,l1,ptr)
    if (p2)
    {
        ENCODE_TEXT(c1,p2,l2,ptr)
        *ptr++ = c2;            
    }
    *ptr++ = '\r';        
    *ptr++ = '\n';                
    *p = ptr;
    return RV_TRUE;
}

/*
 *	Encodes the SDP phone line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodePhone(char **p, char *pEnd, RvSdpPhone *ph)
{
    int l1,l2;
    char *p1, *p2, c1, c2;
    char *ptr = *p;
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)    
    if (ph->iBadSyntaxField)
    {
        ENCODE_TAG_VALUE('p',ph->iBadSyntaxField,p,pEnd)
        return RV_TRUE;
    }
#endif      
    p1 = ph->iPhoneNumber;
    p2 = ph->iText;
    c1 = '(';
    c2 = ')';

    if (ph->iSeparSymbol == '<')
    {
        p1 = ph->iText;
        p2 = ph->iPhoneNumber;
        c1 = '<'; c2 = '>';
    }
    else if (ph->iSeparSymbol != '(')
        p2 = NULL;

    l1 = (p1) ? strlen(p1) : 0;
    l2 = (p2) ? strlen(p2) : 0;
    
    if (pEnd - ptr < 1+1+l1+1+l2+1+1+1)
        return RV_FALSE;
    
    *ptr++ = 'p';
    ENCODE_TEXT('=',p1,l1,ptr)
    if (p2)
    {
        ENCODE_TEXT(c1,p2,l2,ptr)
            *ptr++ = c2;            
    }
    *ptr++ = '\r';        
    *ptr++ = '\n';                
    *p = ptr;
    return RV_TRUE;
}


/*
 *	Encodes the SDP connection line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeConnection(char **p, char *pEnd, RvSdpConnection *cn)
{
    int l1,l2,l3,l4,l5,ll;
    char *p1, *p2, *p4, *p5;
    char *ptr = *p;    
    char numTxt1[20], numTxt2[20];
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)    
    if (cn->iBadSyntaxField)
    {
        ENCODE_TAG_VALUE('c',cn->iBadSyntaxField,p,pEnd)
        return RV_TRUE;
    }        
#endif      


    p1 = rvSdpConnectionGetNetTypeStr(cn);
    l1 = strlen(p1);
    p2 = rvSdpConnectionGetAddrTypeStr(cn);
    l2 = strlen(p2);
    l3 = (cn->iAddress)?strlen(cn->iAddress):0;

    ll = 0;
    p4 = p5 = NULL;
    l4 = l5 = 0;
    if (cn->iTtl != RV_SDP_INT_NOT_SET)
    {
        p4 = rvSdpUitoa(numTxt1,cn->iTtl);
        l4 = strlen(p4);
        ll += 1 + l4;
        if (cn->iNumAddr != RV_SDP_INT_NOT_SET)
        {
            p5 = rvSdpUitoa(numTxt2,cn->iNumAddr);
            l5 = strlen(p5);
            ll += 1 + l5;
        }
    }
        
    
    if (pEnd - ptr < 1+1+l1+1+l2+1+l3+ll+1+1)
        return RV_FALSE;
    
    *ptr++ = 'c';
    ENCODE_TEXT('=',p1,l1,ptr)
    ENCODE_TEXT(' ',p2,l2,ptr)
    ENCODE_TEXT(' ',cn->iAddress,l3,ptr)
    if (p4)
        ENCODE_TEXT('/',p4,l4,ptr)
    if (p5)
        ENCODE_TEXT('/',p5,l5,ptr)
    *ptr++ = '\r';        
    *ptr++ = '\n';                
    *p = ptr;
    return RV_TRUE;    
}

/*
 *	Encodes the SDP bandwidth line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeBandwidth(char **p, char *pEnd, RvSdpBandwidth* bw)
{
    int l1,l2;
    char *p2;
    char *ptr = *p;    
    char numTxt[20];
    
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)    
    if (bw->iBadSyntaxField)
    {
        ENCODE_TAG_VALUE('b',bw->iBadSyntaxField,p,pEnd)
        return RV_TRUE;
    }
#endif      

    l1 = (bw->iBWType)?strlen(bw->iBWType):0;
    p2 = rvSdpUitoa(numTxt,bw->iBWValue);
    l2 = strlen(p2);
    
    if (pEnd - ptr < 1+1+l1+1+l2+1+1)
        return RV_FALSE;
    
    *ptr++ = 'b';
    ENCODE_TEXT('=',bw->iBWType,l1,ptr)
    ENCODE_TEXT(':',p2,l2,ptr)
    
    *ptr++ = '\r';        
    *ptr++ = '\n';                
    *p = ptr;
    return RV_TRUE;    
}

/*#define TYPE_TIME_TYPE(_t) ((_t==RV_SDPTIMETYPE_DAY)?"d":((_t==RV_SDPTIMETYPE_HOUR)?"h":((_t==RV_SDPTIMETYPE_MONTH)?"m":"")))*/

static const char gsTypedTimes[] = 
{
    /*RV_SDPTIMETYPE_DAY*/      'd',
    /*RV_SDPTIMETYPE_HOUR*/     'h',
    /*RV_SDPTIMETYPE_MONTH*/    'm',
};

#define TYPE_TIME_TYPE(_t) ((_t<=RV_SDPTIMETYPE_MONTH)?gsTypedTimes[_t]:0)

/*
 *	Encodes the SDP session time line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeSessionTime(char** p, char *pEnd, RvSdpSessionTime *sessT)
{
    int l1,l2;
    char *p1,*p2;
    char *ptr = *p;    
    char numTxt1[20],numTxt2[20];
    
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)    
    if (sessT->iBadSyntaxField)
    {
        ENCODE_TAG_VALUE('t',sessT->iBadSyntaxField,p,pEnd)
        return RV_TRUE;
    }
#endif      
    
    p1 = rvSdpUitoa(numTxt1,sessT->iStart);
    l1 = strlen(p1);
    p2 = rvSdpUitoa(numTxt2,sessT->iEnd);
    l2 = strlen(p2);
    
    if (pEnd - ptr < 1+1+l1+1+l2+1+1)
        return RV_FALSE;
    
    *ptr++ = 't';
    ENCODE_TEXT('=',p1,l1,ptr)
    ENCODE_TEXT(' ',p2,l2,ptr)
        
    *ptr++ = '\r';        
    *ptr++ = '\n';                
    *p = ptr;
    return RV_TRUE;    
}

/*
 *	Encodes the SDP repeat interval line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeRepeatInterval(char** p, char *pEnd, RvSdpRepeatInterval *rinterv)
{
    char numTxt1[20], numTxt2[20], *p1, *p2, c;
    int l1, l2;
    RvSdpListIter i;
    RvUint32 tVal;
    RvSdpTimeUnit tUnit;
    RvBool retV;
    char *ptr = *p;
  

            
#if defined(RV_SDP_CHECK_BAD_SYNTAX)    
    if (rinterv->iBadSyntaxField)
    {
        ENCODE_TAG_VALUE('r',rinterv->iBadSyntaxField,p,pEnd)
        return RV_TRUE;    
    }
#endif  

    p1 = rvSdpUitoa(numTxt1,rvSdpRepeatIntervalGetIntervalTime(rinterv));
    l1 = strlen(p1);
    p2 = rvSdpUitoa(numTxt2,rvSdpRepeatIntervalGetDurationTime(rinterv));
    l2 = strlen(p2);
    
    if (pEnd - ptr < 1+1+l1+1+1+l2+1+1+1)
        return RV_FALSE;
    
    *ptr++ = 'r';
    ENCODE_TEXT('=',p1,l1,ptr)

    c = (char) TYPE_TIME_TYPE(rinterv->iInterval.iTimeType);
    if (c)
        *ptr++ = c;
    
    ENCODE_TEXT(' ',p2,l2,ptr)
    c = (char) TYPE_TIME_TYPE(rinterv->iDuration.iTimeType);
    if (c)
        *ptr++ = c;
    
    for (retV = rvSdpRepeatIntervalGetFirstOffset(rinterv,&i,&tVal,&tUnit); retV;
                                            retV = rvSdpRepeatIntervalGetNextOffset(&i,&tVal,&tUnit))
    {
        p1 = rvSdpUitoa(numTxt1,tVal);
        l1 = strlen(p1);

        if (pEnd - ptr < l1+1+1+1+1)
            return RV_FALSE;
        
        ENCODE_TEXT(' ',p1,l1,ptr)
        c = (char)TYPE_TIME_TYPE(tUnit);
        if (c)
            *ptr++ = c;
    }
    *ptr++ = '\r';        
    *ptr++ = '\n';                
    *p = ptr;
    return RV_TRUE;    
}

/*
 *	Encodes the SDP time zone adjustment line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeTimeZone(char **p, char *pEnd, RvSdpTZA *tza)
{
    RvSdpTimeZoneAdjust* tz;
    RvSdpListIter i;
    char *p1, *p2, c, numTxt1[20], numTxt2[20];
    int cnt,l1,l2,offs;
    RvSdpMsg* msg;
    char *ptr = *p;
    
    offs = RV_OFFSETOF(RvSdpMsg,iTZA);
    msg = (RvSdpMsg*)((char*)tza - offs);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (msg->iTZA.iBadSyntaxField)
    {
        ENCODE_TAG_VALUE('z',msg->iTZA.iBadSyntaxField,p,pEnd)
        return RV_TRUE;    
    }
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

    if (pEnd - ptr < 20)
        return RV_FALSE;

    cnt = 0;
    for (tz = rvSdpMsgGetFirstZoneAdjustment(msg,&i); tz; tz = rvSdpMsgGetNextZoneAdjustment(&i))
    {

        p1 = rvSdpUitoa(numTxt1,rvSdpTimeZoneAdjustGetTime(tz));
        l1 = strlen(p1);
        p2 = rvSdpItoa(numTxt2,rvSdpTimeZoneAdjustGetOffsetTime(tz));
        l2 = strlen(p2);

        if (pEnd - ptr < l1+l2+10)
            return RV_FALSE;

        if (!cnt)
        {
            *ptr++ = 'z';
            c = '=';
        }
        else
            c = ' ';

        ENCODE_TEXT(c,p1,l1,ptr)
        ENCODE_TEXT(' ',p2,l2,ptr)
        c = (char)TYPE_TIME_TYPE(tz->iOffsetUnits);
        if (c)
            *ptr++ = c;
        cnt ++;
    }
    *ptr++ = '\r';
    *ptr++ = '\n';
    *p = ptr;
    return RV_TRUE;
}

/*
 *	Encodes the SDP key line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeKey(char **p, char *pEnd, RvSdpKey *k)
{
    const char *p1;
    int l1, l2;
    char *ptr = *p;
        
#if defined(RV_SDP_CHECK_BAD_SYNTAX)    
    if (k->iBadSyntaxField)
    {
        ENCODE_TAG_VALUE('k',k->iBadSyntaxField,p,pEnd)
        return RV_TRUE;    
    }
#endif  

    p1 = rvSdpKeyGetTypeStr(k);
    l1 = strlen(p1);
    if (k->iData)
        l2 = strlen(k->iData);
    else
        l2 = 0;
    
    if (pEnd - ptr < 1+1+l1+1+l2+1+1)
        return RV_FALSE;
    
    *ptr++ = 'k';
    ENCODE_TEXT('=',p1,l1,ptr)
    if (k->iData)
    {
        ENCODE_TEXT(':',k->iData,l2,ptr)
    }

    *ptr++ = '\r';
    *ptr++ = '\n';
    *p = ptr;
    return RV_TRUE;
}

/*
 *	Encodes the SDP attribute line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeAttr(char **p, char *pEnd, RvSdpAttribute *a)
{
    char *p1, *p2;
    int l1, l2;
    char *ptr = *p;
    

    p1 = (char*)rvSdpAttributeGetName(a);
    l1 = strlen(p1);
    p2 = (char*)rvSdpAttributeGetValue(a);
    if (p2 && *p2)
        l2 = strlen(p2);
    else
        l2 = 0;


    if (pEnd - ptr < 1+1+l1+1+l2+1+1)
        return RV_FALSE;
    
    *ptr++ = 'a';
    p1 = (char*)rvSdpAttributeGetName(a);
    ENCODE_TEXT('=',p1,l1,ptr)
    if (p2)
    {
        ENCODE_TEXT(':',p2,l2,ptr)
    }
    
    *ptr++ = '\r';
    *ptr++ = '\n';
    *p = ptr;
    return RV_TRUE;
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)        
/*
 *	Encodes the SDP proprietary tag line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeOther(char **p, char *pEnd, RvSdpOther *o)
{
    char *p1;
    int l1;
    char *ptr = *p;

    p1 = rvSdpOtherGetValue(o);
    l1 = strlen(p1);
    if (pEnd - ptr < l1 + 10)
        return RV_FALSE;

    if (rvSdpOtherGetTag(o))
    {
        *ptr++ = rvSdpOtherGetTag(o);
        *ptr++ = '=';
    }
    memcpy(ptr,p1,l1);
    ptr += l1;
    *ptr++ = '\r';
    *ptr++ = '\n';
    *p = ptr;
    return RV_TRUE;
}
#endif /*#if defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/*
 *	Encodes the SDP media descriptor line into '*p' and all objects
 *  contained in the media descriptor object.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeMediaDescrFields(char **p, char* pEnd, RvSdpMediaDescr* md)
{
    int l1,l2,l3,l4,cnt;
    char  numTxt1[20],numTxt2[20],*p2,*p3,**pp;
    char *ptr = *p;
    

#if defined(RV_SDP_CHECK_BAD_SYNTAX)       
    if (md->iBadSyntaxField)
    {
        ENCODE_TAG_VALUE('m',md->iBadSyntaxField,p,pEnd)
        return RV_TRUE;
    }
#endif  

    l1 = (md->iMediaTypeStr)?strlen(md->iMediaTypeStr):0;
    p2 = rvSdpUitoa(numTxt1,md->iPort);
    l2 = strlen(p2);
    if (md->iNumOfPorts)
    {
        p3 = rvSdpUitoa(numTxt2,md->iNumOfPorts);
        l3 = strlen(p3);
    }
    else
    {
        p3 = NULL;
        l3 = 0;
    }

    l4 = (md->iProtocolStr)?strlen(md->iProtocolStr):0;
    
    if (pEnd - ptr < l1+l2+l3+l4+10)
        return RV_FALSE;
    
    *ptr++ = 'm';
    ENCODE_TEXT('=',md->iMediaTypeStr,l1,ptr)
    ENCODE_TEXT(' ',p2,l2,ptr)
    ENCODE_TEXT('/',p3,l3,ptr) 
    ENCODE_TEXT(' ',md->iProtocolStr,l4,ptr) 
    

    for (cnt = 0, pp = md->iMediaFormats; cnt < md->iMediaFormatsNum; cnt++, pp ++)
    {
        l1 = strlen(*pp);
        if (pEnd - ptr < l1 + 3)
            return RV_FALSE;
        ENCODE_TEXT(' ',(*pp),l1,ptr) 
    }    
    *ptr++ = '\r';
    *ptr++ = '\n';
    *p = ptr;
    return RV_TRUE;
}


/*
 *	Encodes the SDP media descriptor line into '*p' and all objects
 *  contained in the media descriptor object.
 *  '*p' is moved at the end of encoded portion.
 */
RvBool rvSdpEncodeMediaDescr(char **p, char* pEnd, RvSdpMsg* msg)
{
    RvSdpListIter i, i1;
    RvSdpMediaDescr *md;
    RvSdpConnection *cn;
    RvSdpBandwidth *bw;
    RvSdpAttribute *a;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)        
    RvSdpOther* o;
#endif
        

    for (md = rvSdpMsgGetFirstMediaDescr(msg,&i); md;md = rvSdpMsgGetNextMediaDescr(&i))
    {
        if (!rvSdpEncodeMediaDescrFields(p,pEnd,md))
            return RV_FALSE;

        if (md->iCommonFields.iInfo.iInfoTxt)
            if (!rvSdpEncodeSessionInfo(p,pEnd,md->iCommonFields.iInfo.iInfoTxt))
                return RV_FALSE;

        for (cn = rvSdpMediaDescrGetFirstConnection(md,&i1); cn; cn = rvSdpMediaDescrGetNextConnection(&i1))
            if (!rvSdpEncodeConnection(p,pEnd,cn))
                return RV_FALSE;
        
        for (bw = rvSdpMediaDescrGetFirstBandwidth(md,&i1); bw; bw = rvSdpMediaDescrGetNextBandwidth(&i1))
            if (!rvSdpEncodeBandwidth(p,pEnd,bw))            
                return RV_FALSE;

        if (RV_SDP_KEY_IS_USED(&md->iCommonFields.iKey))
            if (!rvSdpEncodeKey(p,pEnd,&md->iCommonFields.iKey))
                return RV_FALSE;

        for (a = rvSdpMediaDescrGetFirstAttribute2(md,&i1); a; a = rvSdpMediaDescrGetNextAttribute2(&i1))
            if (!rvSdpEncodeAttr(p,pEnd,a))
                return RV_FALSE;
                
#if defined(RV_SDP_CHECK_BAD_SYNTAX)         
        for (o = rvSdpMediaDescrGetFirstOther(md,&i1); o; o = rvSdpMediaDescrGetNextOther(&i1))
            if (!rvSdpEncodeOther(p,pEnd,o))
                return RV_FALSE;
#endif
    }
    return RV_TRUE;
}

/***************************************************************************
 * rvSdpMsgEncodeToBuf
 * ------------------------------------------------------------------------
 * General: 
 *      Takes an RvSdpMsg as input and encodes it as text into a buffer (according
 *      to the SDP syntax).
 *          
 * Return Value: 
 *      Returns a pointer past the end of the encoding (buf+number of encoded bytes)
 *      or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the SDP message to be encoded.
 *      buf - a pointer to a buffer where the message will be encoded.
 *      len - the length of the buffer.
 *      stat - a pointer to a variable where the status of the encoding will be set. 
 *             If encoding was successful, stat should be equal to rvOk in
 *             return. Any other value means that encoding failed.
 ***************************************************************************/
char* rvSdpMsgEncodeToBuf(RvSdpMsg* msg, char* buf, int len, RvSdpStatus* stat)
{
    char *p;
    RvSdpListIter i, i1;
    RvSdpEmail *e;
    RvSdpPhone *ph;
    RvSdpConnection *cn;
    RvSdpBandwidth *bw;
    RvSdpSessionTime *sessT;
    RvSdpRepeatInterval *rinterv;
    RvSdpAttribute *a;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)        
    RvSdpOther* o;
#endif
    char *pEnd = buf+len;
    

    p = buf;

    if (msg->iVersion.iVersionTxt)
        if (!rvSdpEncodeVersion(&p,pEnd,msg->iVersion.iVersionTxt))
            goto encFailed;

    if (RV_SDP_ORIGIN_IS_USED(&msg->iOrigin))
        if (!rvSdpEncodeOrigin(&p,pEnd,&msg->iOrigin))
            goto encFailed;

    if (msg->iSessId.iSessIdTxt)
        if (!rvSdpEncodeSessionName(&p,pEnd,msg->iSessId.iSessIdTxt))
            goto encFailed;            

    if (msg->iCommonFields.iInfo.iInfoTxt)
        if (!rvSdpEncodeSessionInfo(&p,pEnd,msg->iCommonFields.iInfo.iInfoTxt))
            goto encFailed;

    if (msg->iUri.iUriTxt)
        if (!rvSdpEncodeURI(&p,pEnd,msg->iUri.iUriTxt))
            goto encFailed;

    for (e = rvSdpMsgGetFirstEmail(msg,&i); e; e = rvSdpMsgGetNextEmail(&i))
        if (!rvSdpEncodeEmail(&p,pEnd,e))
            goto encFailed;

    for (ph = rvSdpMsgGetFirstPhone(msg,&i); ph; ph = rvSdpMsgGetNextPhone(&i))
        if (!rvSdpEncodePhone(&p,pEnd,ph))
            goto encFailed;
    
    for (cn = rvSdpMsgGetFirstConnection(msg,&i); cn; cn = rvSdpMsgGetNextConnection(&i))
        if (!rvSdpEncodeConnection(&p,pEnd,cn))
            goto encFailed;

    for (bw = rvSdpMsgGetFirstBandwidth(msg,&i); bw; bw = rvSdpMsgGetNextBandwidth(&i))
        if (!rvSdpEncodeBandwidth(&p,pEnd,bw))
            goto encFailed;

    for (sessT = rvSdpMsgGetFirstSessionTime(msg,&i); sessT; sessT = rvSdpMsgGetNextSessionTime(&i))
    {
        if (!rvSdpEncodeSessionTime(&p,pEnd,sessT))
            goto encFailed;
        for (rinterv = rvSdpSessionTimeGetFirstRepeatInterval(sessT,&i1); rinterv;
                                                                    rinterv = rvSdpSessionTimeGetNextRepeatInterval(&i1))
            if (!rvSdpEncodeRepeatInterval(&p,pEnd,rinterv))
                goto encFailed;
    }

    if (rvSdpMsgTZAIsUsed(msg))
        if (!rvSdpEncodeTimeZone(&p,pEnd,&msg->iTZA))
            goto encFailed;

    if (RV_SDP_KEY_IS_USED(&msg->iCommonFields.iKey))
        if (!rvSdpEncodeKey(&p,pEnd,&msg->iCommonFields.iKey))
            goto encFailed;
    
    for (a = rvSdpMsgGetFirstAttribute2(msg,&i); a; a = rvSdpMsgGetNextAttribute2(&i))
        if (!rvSdpEncodeAttr(&p,pEnd,a))
            goto encFailed;
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)        
    for (o = rvSdpMsgGetFirstOther(msg,&i); o; o = rvSdpMsgGetNextOther(&i))
        if (!rvSdpEncodeOther(&p,pEnd,o))
            goto encFailed;
#endif   
    if (!rvSdpEncodeMediaDescr(&p,pEnd,msg))
        goto encFailed;

    *p = 0;


    if (stat)
        *stat = RV_SDPSTATUS_OK;
    return p;
        
encFailed:
    if (stat)
        *stat = RV_SDPSTATUS_ENCODEFAILBUF;    
    return NULL;
}

