/******************************************************************************
Filename    :rvsdpcodecs.c
Description : definitions regarding use of codecs

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
#include "rvsdpglobals.h"
#include "rvsdpprivate.h"


#ifdef RV_SDP_CODECS_SUPPORTED


#include "rvsdpcodecs.h"
#include "rvsdpcodecsinternal.h"
#include "rvsdpcodecparseutils.h"
#include "rvstrutils.h"



/* static RvSdpCodecParserInternal* gsParsersTable [RV_SDP_MAX_CODECS]; */
/* static RvSize_t                  gsParsersTableLen;                */
//#define MyrvAllocAllocate(_a,_b) rvAllocAllocate(_a,_b);
//#define MyrvAllocDeallocate(_a,_b,_c) rvAllocDeallocate(_a,_b,_c);


/*those Init functions are called in rvSdpCodecInitialize() that initialize the codec 
  module only if the flag of the specific codec is ON*/ 

/* XXX - Add New Codec */
extern RvSdpStatus RvSdpCodec_DTMF_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_G726_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_G7231_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_G729_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_G722_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_PCMA_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_PCMU_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_H263_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_H261_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_MP4A_LATM_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_MP4V_ES_Init(RvAlloc* a);
extern RvSdpStatus RvSdpCodec_AMR_Init(RvAlloc* a);



/*
 *	This function initializes the SDP Codec module;
 *  It needs to be called exactly once before any other function from
 *  the module s called.
 */
RVSDPCOREAPI  RvSdpStatus rvSdpCodecInitialize()
{
    RV_SDP_USE_GLOBALS;

    memset(gsParsersTable,0,sizeof(gsParsersTable));
    gsParsersTableLen = 0;

/* XXX - Add New Codec */

#ifdef RV_SDP_CODEC_H263
    RvSdpCodec_H263_Init(rvSdpGetDefaultAllocator());
#endif
    
#ifdef RV_SDP_CODEC_H261
    RvSdpCodec_H261_Init(rvSdpGetDefaultAllocator());
#endif
    
#ifdef RV_SDP_CODEC_MP4A_LATM
    RvSdpCodec_MP4A_LATM_Init(rvSdpGetDefaultAllocator());
#endif
    
#ifdef RV_SDP_CODEC_MP4V_ES
    RvSdpCodec_MP4V_ES_Init(rvSdpGetDefaultAllocator());
#endif
    
#ifdef RV_SDP_CODEC_AMR
    RvSdpCodec_AMR_Init(rvSdpGetDefaultAllocator());
#endif

#ifdef RV_SDP_CODEC_DTMF
    RvSdpCodec_DTMF_Init(rvSdpGetDefaultAllocator());
#endif
	
#ifdef RV_SDP_CODEC_G726
    RvSdpCodec_G726_Init(rvSdpGetDefaultAllocator());
#endif
	
#ifdef RV_SDP_CODEC_G7231
    RvSdpCodec_G7231_Init(rvSdpGetDefaultAllocator());
#endif
	
#ifdef RV_SDP_CODEC_G729
    RvSdpCodec_G729_Init(rvSdpGetDefaultAllocator());
#endif
	
#ifdef RV_SDP_CODEC_G722
    RvSdpCodec_G722_Init(rvSdpGetDefaultAllocator());
#endif
	
#ifdef RV_SDP_CODEC_PCMU
    RvSdpCodec_PCMU_Init(rvSdpGetDefaultAllocator());
#endif
	
#ifdef RV_SDP_CODEC_PCMA
    RvSdpCodec_PCMA_Init(rvSdpGetDefaultAllocator());
#endif
	
    
    return RV_SDPSTATUS_OK;
}

/*
 *	This function de-initializes the SDP Codec module;
 *  It needs to be called exactly once at the end of module use.
 */
RVSDPCOREAPI  RvSdpStatus rvSdpCodecEnd()
{    
    RvSize_t cnt;
    RV_SDP_USE_GLOBALS;

    for (cnt = 0; cnt < gsParsersTableLen; cnt++)
        RvSdpCodecParserInternalDestruct(gsParsersTable[cnt]);
    return RV_SDPSTATUS_OK;
}

/*
 *	To get the previously registered codec parser structure by its registered name.
 *  The name is case insensitive;
 */
RVSDPCOREAPI  RvSdpCodecParser rvSdpCodecParserGetByName(const char* codecName)
{
    RvSize_t cnt, i;
    RvSdpCodecParserInternal* cp;
    char *p;
    RV_SDP_USE_GLOBALS;

    for (cnt = 0; cnt < gsParsersTableLen; cnt ++)
    {
        cp = gsParsersTable[cnt];
        i = 0;
        while ((p = cp->iCodecNames[i++]))
        {
            if (!strcasecmp(p,codecName))
                return cp;
        }
    }
    return NULL;
}

/* 
 * Gets the name of the codec by its payload number as defined in RFC 3551 (tables 4 & 5)
   for 'well known codecs (payload 0-->95 )
 */
RVSDPCOREAPI  const char* rvSdpCodecNameGetByPayLoad(int payLoad)
{
	/* XXX - Add New Codec -only if its payload <96*/

    static const char sCodecPayload2Name[][10] =
    {
/* 0  */    "PCMU",
/* 1  */    "",
/* 2  */    "",
/* 3  */    "GSM",
/* 4  */    "G723",
/* 5  */    "DVI4",
/* 6  */    "DVI4",
/* 7  */    "LPC",
/* 8  */    "PCMA",
/* 9  */    "G722",
/* 10 */    "L16",
/* 11 */    "L16",
/* 12 */    "QCELP",
/* 13 */    "CN",
/* 14 */    "MPA",
/* 15 */    "G728",
/* 16 */    "DVI4",
/* 17 */    "DVI4",
/* 18 */    "G729",
/* 19 */    "",
/* 20 */    "",
/* 21 */    "",
/* 22 */    "",
/* 23 */    "",
/* 24 */    "",
/* 25 */    "CelB",
/* 26 */    "JPEG",
/* 27 */    "",
/* 28 */    "nv",
/* 29 */    "",
/* 30 */    "",
/* 31 */    "H261",
/* 32 */    "MPV",
/* 33 */    "MP2T",
/* 34 */    "H263"                
};

    if ((unsigned)payLoad >= sizeof(sCodecPayload2Name)/sizeof(sCodecPayload2Name[0]))
        return NULL;

    if (sCodecPayload2Name[payLoad][0] == 0)
        return NULL;

    return sCodecPayload2Name[payLoad];
}

/*
 *	To get the previously registered codec parser structure by 'well known' (less than 96) 
 *  payload number.
 */
RVSDPCOREAPI  RvSdpCodecParser rvSdpCodecParserGetByPayLoad(int payLoad)
{
    const char *payLoadName;
    payLoadName = rvSdpCodecNameGetByPayLoad(payLoad);
    if (payLoadName == NULL)
        return NULL;
    else
        return rvSdpCodecParserGetByName(payLoadName);
}


const char* rvSdpFindFtmpAttr(RvSdpMediaDescr* mediaD,long payLoad)
{
    RvSdpAttribute* attrP;
    long num;
    const char* attrVal = NULL;
    RvSdpListIter iter;
    RvSize_t numOfFmtp;

    numOfFmtp = rvSdpMediaDescrGetNumOfFmtp(mediaD);
    
    /* FMTP can appear only in mediaD level and not in Msg level.
	go over all the FMTP attr and find the one matches the given payload*/
    for ( attrP = rvSdpMediaDescrGetFirstFmtp (mediaD,&iter);attrP; 
          attrP=(RvSdpAttribute*)rvSdpMediaDescrGetNextFmtp(&iter))
    {
        attrVal = attrP->iAttrValue;
        /* the value is : xx name=value ...*/
        /* xx - is the payload number; get it */
        num = strtol(attrVal,(char**)&attrVal,10);
        
        if (num == payLoad)
        {
            /* that is needed payload, attrVal points currently to the 'name=value ...' */
            return attrVal;
        }
    }
    return attrVal;
}



/*
 * Parses the SDP media descr in order to build the specific codec parameters structure.
 * The parameters built for the codec that its payload number is specified as 'payLoad'
 * The allocator 'a' is optional if NULL will be given the default allocator will be 
 * used.
 */
RVSDPCOREAPI  RvSdpCodecParams rvSdpCodecParamsConstruct(RvSdpCodecParser codecP, int payLoad, 
                                                           RvSdpMediaDescr* mediaD, RvAlloc* a)
{
    RvSdpCodecParserInternal* cp = (RvSdpCodecParserInternal*) codecP;
    RvSdpCodecParamsInternal *params;
    const char *attrVal;

    if (a == NULL)
        a = rvSdpGetDefaultAllocator();

    /* find FMTP attributes matching the payLoad. 
       we will search in the attributes of media first and
       in case we can't find we'll search among the attributes 
       of the message;
       start with the mediaDescr attributes 
       go over all attributes looking for fmtp attr */

    attrVal = rvSdpFindFtmpAttr(mediaD,payLoad);
//        if (attrVal == NULL)
//            attrVal = rvSdpFindFtmpAttr(mediaD,RV_FALSE,payLoad);

    /* create the params struct */
    params = rvSdpAllocAllocate(a,sizeof(RvSdpCodecParamsInternal));
    if (params == NULL)
        return NULL;

    memset(params,0,sizeof(RvSdpCodecParamsInternal));
    params->iAllocator = a;
    params->iMediaDescr = mediaD;    

    RvSdpNameValuesListInitialize(&params->iCodecNVL);

    if (cp->iStartFmtpParsingCB && (*cp->iStartFmtpParsingCB)(params) != RV_SDPSTATUS_OK)
    {
        rvSdpCodecParamsDestruct(params);
        return NULL;
    }
    
    /* could find the fmtp */
    if (attrVal)
    {
        params->iFmtpValueLen = strlen(attrVal)+1;
        params->iFmtpValue = rvSdpAllocAllocate(a,params->iFmtpValueLen);
        if (!params->iFmtpValue)
        {
            rvSdpCodecParamsDestruct(params);
            return NULL;
        }
        strcpy(params->iFmtpValue,attrVal);
        if (RvSdpParseFmtp(params,cp) != RV_SDPSTATUS_OK)
        {
            rvSdpCodecParamsDestruct(params);
            return NULL;
        }
    }    

    if (RvSdpParseCodecAttrs(params,cp) != RV_SDPSTATUS_OK)
    {
        rvSdpCodecParamsDestruct(params);
        return NULL;
    }

    if (cp->iFinishFmtpParsingCB && (*cp->iFinishFmtpParsingCB)(params) != RV_SDPSTATUS_OK)
    {
        rvSdpCodecParamsDestruct(params);
        return NULL;
    }
    
    
    return params;
}

/*
 *	Destrucrs the codec parameters structure;
 */
RVSDPCOREAPI  void rvSdpCodecParamsDestruct(RvSdpCodecParams p)
{
    RvSdpCodecParamsInternal* prms = (RvSdpCodecParamsInternal*)p;

    if (!prms)
        return;
    
    RvSdpNameValuesListEnd(prms->iAllocator,&prms->iCodecNVL);        

    if (prms->iFmtpValue)
        rvSdpAllocDeallocate(prms->iAllocator,prms->iFmtpValueLen,prms->iFmtpValue);

    rvSdpAllocDeallocate(prms->iAllocator,sizeof(RvSdpCodecParamsInternal),prms);

    return;
}

/* 
 * Tests whether the codec parameters match the codec's predefined
 * grammar. Note that not all the codecs define grammar.
 */
RVSDPCOREAPI  RvBool rvSdpCodecIsValidSyntax(RvSdpCodecParams p)
{
    RvSdpCodecParamsInternal* prms = (RvSdpCodecParamsInternal*)p;

    if (prms->iFmtpBadSyntax)
        return RV_FALSE;
    else
        return RV_TRUE;
}

/*
 *	Gets the number of all parsed parameters. 
 */
RVSDPCOREAPI  RvSize_t rvSdpCodecGetNumOfParams(RvSdpCodecParams p)
{
    RvSdpCodecParamsInternal* prms = (RvSdpCodecParamsInternal*)p;
    return prms->iCodecNVL.iNVCount;
}

/*
 *	Gets the number of unknown parsed parameters;
 */
RVSDPCOREAPI  RvSize_t rvSdpCodecGetNumOfUnknownParams(RvSdpCodecParams p)
{
    RvSdpCodecParamsInternal* prms = (RvSdpCodecParamsInternal*)p;
    return prms->iUnknownParamsCnt;
}

/*
 *	Gets the number of parameters whose values did not match the predefined pattern;
 */
RVSDPCOREAPI  RvSize_t rvSdpCodecGetNumOfInvalidParams(RvSdpCodecParams p)
{
    RvSdpCodecParamsInternal* prms = (RvSdpCodecParamsInternal*)p;
    return prms->iBadSyntaxParamsCnt;
}


/*
 *	Get the name, value and pattern-matching of the parameter based on an index. The index
 *  is zero based (to get the first parameter use '0' value for 'index')
 *  Any of 'name','value' or 'isOK' may be set to NULL when calling the function.
 */
RVSDPCOREAPI  RvSdpStatus rvSdpGetParamByIndex(RvSdpCodecParams p, int index, const char** name, 
                                              const char** value, RvBool* isOK)
{
    RvSdpCodecParamsInternal* prms = (RvSdpCodecParamsInternal*)p;
    RvSdpNameValue* nv;

    nv = prms->iCodecNVL.iNVArray[index];
    if (!nv)
        return RV_SDPSTATUS_PARSEFAIL;

    if (name)
        *name = nv->iName;
    if (value)
        *value = nv->iValue;
    if (isOK)
        *isOK = nv->iValueOK;
    
    return RV_SDPSTATUS_OK;
}

/*
 *	Get the value and pattern-matching of the parameter based on its name.
 *  Any of 'value' or 'isOK' may be set to NULL when calling the function.
 */
RVSDPCOREAPI  RvSdpStatus rvSdpGetParamByName(RvSdpCodecParams p, const char* name, 
                                             const char** value, RvBool* isOK)
{
    RvSdpCodecParamsInternal* prms = (RvSdpCodecParamsInternal*)p;
    RvSdpNameValue* nv;
    int index;

    if (!isalpha((int)*name))
        return RV_SDPSTATUS_PARSEFAIL;

    index = tolower(*name) - 'a';

    nv = prms->iCodecNVL.iHeadsVector[index];
    
    while (nv)
    {
        if (strcasecmp(nv->iName,name))
        {
            nv = nv->iNextInList;
            continue;
        }

        if (value)
            *value = nv->iValue;
        if (isOK)
            *isOK = nv->iValueOK;        
        return RV_SDPSTATUS_OK;
    }

    return RV_SDPSTATUS_PARSEFAIL;
}



/*
 *	Constructs the params parser.
 *  This function is called by each supported codec in its initialization function.
 */
RvSdpCodecParserInternal*
RvSdpCodecParserInternalConstruct(const RvSdpParamStaticParserData *fpdArr, 
                                  const RvSdpParamStaticParserData *apdArr, RvAlloc* a)
{
    RvSdpCodecParamData *f, *fm;
    int index,cnt;
    const RvSdpParamStaticParserData *fpd;

    RvSdpCodecParserInternal* cp;

    cp = (RvSdpCodecParserInternal*) rvSdpAllocAllocate(a,sizeof(RvSdpCodecParserInternal));

    if (cp == NULL)
        return NULL;
    
    memset(cp,0,sizeof(RvSdpCodecParserInternal)); 
    cp->iAllocator = a;
    

    /* for all fields definitions contained in 'fpdArr' (RvSdpParamStaticParserData*)*/
    for (cnt = 0; fpdArr[cnt].iParamNum >=0 && fpdArr[cnt].iFmtpParamValueSyntax; cnt++)
    {    
        fpd = &(fpdArr[cnt]);
 
		/*constructs one parameter parser data*/
        f = RvSdpParamDataConstruct(fpd->iParamNum,fpd->iFmtpParamValueSyntax,a);
        if (!f)
            goto finish;
    
        if (!isalpha((int)(f->iParamName[0])))
        {
            RvSdpFmtpParamDataDestruct(f);
            goto finish;
        }
    
        /* insert it to the correct position in the array; index is as the first letter */
        index = tolower(f->iParamName[0]) - 'a';
        fm = cp->iFmtpParamData[index];
    
        /* if the index is not free insert the new field as the head of linked list */
        cp->iFmtpParamData[index] = f;
        f->iNextParam = fm;
    }

    /* for all fields definitions contained in 'apdArr' */
    for (cnt = 0; apdArr[cnt].iParamNum >=0 && apdArr[cnt].iFmtpParamValueSyntax; cnt++)
    {    
        fpd = &(apdArr[cnt]);
        f = RvSdpParamDataConstruct(fpd->iParamNum,fpd->iFmtpParamValueSyntax,a);
        if (!f)
            goto finish;
        fm = cp->iAttrParamData;
        cp->iAttrParamData = f;
        f->iNextParam = fm;
    }

    return cp;

finish:
    RvSdpCodecParserInternalDestruct(cp);
    return NULL;
}

/*
 *	Destructs the parser;
 */
void 
RvSdpCodecParserInternalDestruct(RvSdpCodecParserInternal* cp)
{
    RvSize_t cnt, up;
    RvSdpCodecParamData *f, *fn;


    up = sizeof(cp->iFmtpParamData)/sizeof(RvSdpCodecParamData*);
    for (cnt = 0; cnt < up; cnt ++)
    {
        f = cp->iFmtpParamData[cnt];
        while (f)
        {
            fn = f->iNextParam;
            RvSdpFmtpParamDataDestruct(f);
            f = fn;
        }
    }

    f = cp->iAttrParamData;
    while (f)
    {
        fn = f->iNextParam;
        RvSdpFmtpParamDataDestruct(f);
        f = fn;
    }


    rvSdpAllocDeallocate(cp->iAllocator,sizeof(RvSdpCodecParserInternal),cp);
    return;
}

/* 
 *  Registers the parser structure in the table of supported codecs parser 
 */
RvSdpStatus 
RvSdpCodecRegister(RvSdpCodecParserInternal* cp)
{
    RV_SDP_USE_GLOBALS;
    RvSdpCodecSetCallbacks(cp);

    if (gsParsersTableLen == RV_SDP_MAX_CODECS)
        return RV_SDPSTATUS_ALLOCFAIL;
    gsParsersTable[gsParsersTableLen++] = cp;
    return RV_SDPSTATUS_OK;
}

/*
 *	Constructs one parameter parser;
 */
RvSdpCodecParamData* RvSdpParamDataConstruct(int fieldNum, const char* txt, RvAlloc *a)
{
    const char *p;
    int l;
    RvSdpCodecParamData* fd;
    
    fd = (RvSdpCodecParamData*) rvSdpAllocAllocate(a,sizeof(RvSdpCodecParamData));
    
    if (!fd)
        return NULL;    
    memset(fd,0,sizeof(RvSdpCodecParamData));
    fd->iAllocator = a;
    
    SKIP_SPACES(txt);
    
    if (!*txt)
        goto finish;
    
    p = txt;
    while (*p && *p != ' ' && *p != '=')
        p++;
    
	/*find the len of 'name'*/
    l = p-txt;
    if (l == 0)
        goto finish;
    
    fd->iParamName = (char*) rvSdpAllocAllocate(a,l+1);
    
    if (fd->iParamName == NULL)
        goto finish;
    fd->iParamNameLen = l+1;
    
    strncpy(fd->iParamName,txt,l);
    fd->iParamName[l] =  0;
    
    fd->iParamNum = fieldNum;
    
    txt = p;
    
    /* search for '=' symbol */
    while (*txt && *txt != '=')
        txt++;
    
    if (*txt != '=')
        return fd;
    
    txt++;
    
    fd->iValueSyntax = RvSdpFtmpValueSyntaxConstruct(txt,a);
    if (!fd->iValueSyntax)
        goto finish;

    return fd;
    
finish:
    
    RvSdpFmtpParamDataDestruct(fd);
    return NULL;
}

/*
 *	Destructs it;
 */
void RvSdpFmtpParamDataDestruct(RvSdpCodecParamData* fd)
{
    if (fd->iParamName)
        rvSdpAllocDeallocate(fd->iAllocator,fd->iParamNameLen,fd->iParamName);
    
    if (fd->iValueSyntax)
        RvSdpFtmpValueSyntaxDestruct(fd->iValueSyntax);
    rvSdpAllocDeallocate(fd->iAllocator,sizeof(RvSdpCodecParamData),fd);
}


RvSdpFtmpValueSyntaxStr* RvSdpFtmpValueSyntaxConstructReal(const char* txt, RvAlloc* a, RvBool isFirst);
RvSdpFtmpValueSyntaxStr* RvSdpFtmpValueSyntaxConstruct(const char* txt, RvAlloc* a)
{
    return RvSdpFtmpValueSyntaxConstructReal(txt,a,RV_TRUE);
}

/*
 * allowed syntax values:
 *          all =               [list_construct] 1*value
 *          list_construct =    "l" SEPARATOR [ MIN_LIST_ELEMENTS "-" MAX_LIST_ELEMENTS ]
 *          value =             int_value | text_value
 *          int_value =         "d" MIN_NUM "-" MAX_NUM [ "." MAX_DIGITS_AFTER_DOT ]
 *          text_value =        "t" ( "ANY" | 1*QUOTED_LITERAL )
 *  
 *      SEPARATOR - any symbol (except for SPACE) separating between list elements;
 *      MIN_LIST_ELEMENTS, MAX_LIST_ELEMENTS - define min and max number of list elements
 *               if not set the list may contain any number of elements bigger than 1
 *               if MAX_LIST_ELEMENTS is '-1' the list must contain at least MIN_LIST_ELEMENTS
 *               elements;
 *     MIN_NUM,MAX_NUM define the range of the number;
 *     MAX_DIGITS_AFTER_DOT define the max number of digits after the decimal dot;
 *     QUOTED_LITERAL any text enclosed in double quotes (");
 */
RvSdpFtmpValueSyntaxStr* RvSdpFtmpValueSyntaxConstructReal(const char* txt, RvAlloc* a, RvBool isFirst)
{
    RvBool success,thereIsMore = RV_FALSE;
    RvSdpFtmpValueSyntaxStr* valSyn;

    valSyn = (RvSdpFtmpValueSyntaxStr*) rvSdpAllocAllocate(a,sizeof(RvSdpFtmpValueSyntaxStr));
    if (!valSyn)
        return NULL;
    

    success = RV_FALSE;
    
    memset(valSyn,0,sizeof(RvSdpFtmpValueSyntaxStr));
    valSyn->iAllocator = a;

    SKIP_SPACES(txt);
        
    /* define whether that is the list */
    if (tolower(*txt) == 'l')
    {
        int n1, n2;

        if (!isFirst)
            /* list is not allowed here */
            goto finish;

        valSyn->iIsList = RV_TRUE;
        txt ++;
        if (*txt == ' ')
            /* separator can't be space*/
            goto finish;

        valSyn->iSepar = *txt;
        txt++;
        n1 = n2 = 0;
        if (isdigit((int)*txt))
        {
            n1 = strtol(txt,(char**)&txt,10);
            /* here '-' must come */
            if (*txt != '-')
                goto finish;
            txt ++;
            if (!isdigit((int)*txt))
                goto finish;
            n2 = strtol(txt,(char**)&txt,10);
            valSyn->iListMinApp = n1;
            valSyn->iListMaxApp = n2;
        }
        else
            valSyn->iListMinApp = valSyn->iListMaxApp = 0;

        SKIP_SPACES(txt);
        if (*txt == 0)
            goto finish;
    }

    /* define the type of the value (DIGIT or TEXT) */
    if (tolower(*txt) == 'd')
    {
        int n1,n2,n3;
        n1 = n2 = n3 = 0;

        valSyn->iValueSwitch = RvSdpValueDigit;        
        txt ++;
        SKIP_SPACES(txt);
        
        /* enter the min number */
        if (!isdigit((int)*txt))
            goto finish;
        n1 = strtol(txt,(char**)&txt,10);
        /* here '-' must come */
        if (*txt != '-')
            goto finish;
        txt ++;
        /* enter the max number */
        if (!isdigit((int)*txt))
            goto finish;
        n2 = strtol(txt,(char**)&txt,10);
        if (*txt == '.')
        {
            txt ++;
        /* there is a decimal dot */
            if (!isdigit((int)*txt))
                goto finish;
            n3 = strtol(txt,(char**)&txt,10);
        }
        if (*txt)
            /* nothing more is allowed */
            thereIsMore = RV_TRUE;

        valSyn->iValue.iNumber.iMinNumber = n1;
        valSyn->iValue.iNumber.iMaxNumber = n2;
        valSyn->iValue.iNumber.iAfterDotDigitMax = n3;
        success = RV_TRUE;        
    }
    else if (tolower(*txt) == 't')
    {
        const char *p;
        char *pp;
        int cnt;
        RvBool isAny = RV_FALSE;
        txt ++;

        valSyn->iValueSwitch = RvSdpValueText;
             
        
        /* allocate memory for the text*/
        valSyn->iValue.iText.iTxtLen = strlen(txt)+1;
        pp = valSyn->iValue.iText.iText = (char*) rvSdpAllocAllocate(a,valSyn->iValue.iText.iTxtLen);
        if (!pp)
            goto finish;        
        
        cnt = 0;

//        while (RV_TRUE) 
        for (;;)
        {
            SKIP_SPACES(txt);

            if (cnt == 0 && strcasecmp(txt,"ANY") == 0)
            {
                /* if the text is ANY it can only be one option */
                isAny = RV_TRUE;
                *pp = 0;
                txt += 3;
                break;
            }

            if (*txt != '"')
                break;
            txt++;
            p = txt;
            while (*txt && *txt != '"')
                txt++;

            if (!*txt)
                /* could not find closing '"' */
                goto finish;

            strncpy(pp,p,txt-p);
            pp += txt-p;
            *(pp++) = 0;
            cnt ++;
            txt++;
        }            

        if (*txt)
            thereIsMore = RV_TRUE;
        else if (cnt == 0 && !isAny)
            goto finish;

        valSyn->iValue.iText.iTxtCount = cnt;
        success = RV_TRUE;        
    }
    else
        goto finish;


        
finish:
    if (success && thereIsMore)
    {
        valSyn->iNextVal = RvSdpFtmpValueSyntaxConstructReal(txt,a,RV_FALSE);
        if (valSyn->iNextVal == NULL)
            success = RV_FALSE;
    }

    if (!success)
    {
        RvSdpFtmpValueSyntaxDestruct(valSyn);
        valSyn = NULL;
    }

    return valSyn;
}

/*
 *	Destructs the value syntax structure;
 */
void RvSdpFtmpValueSyntaxDestruct(RvSdpFtmpValueSyntaxStr* valSyn)
{

    if (valSyn->iNextVal)
        RvSdpFtmpValueSyntaxDestruct(valSyn->iNextVal);

    if (valSyn->iValueSwitch == RvSdpValueText && valSyn->iValue.iText.iText)
        rvSdpAllocDeallocate(valSyn->iAllocator,valSyn->iValue.iText.iTxtLen,valSyn->iValue.iText.iText);
    rvSdpAllocDeallocate(valSyn->iAllocator,sizeof(RvSdpFtmpValueSyntaxStr),valSyn);
}

/*
 *	Validates the value based on predefined pattern
 */
RvSdpStatus RvSdpFtmpValueValidate(RvSdpFtmpValueSyntaxStr* valSyn, const char* txt)
{
    const char *p;
    char separ;
    RvSize_t elCnt,cnt,n;

    elCnt = 0;

    if (valSyn->iIsList)
        separ = valSyn->iSepar;
    else
        separ = 0;

//    while (RV_TRUE)
    for (;;)
    {           
        if (valSyn->iValueSwitch == RvSdpValueDigit)
        /* number case */
        {
            if (!isdigit((int)*txt))
                /* only digit can be here */
                return RV_SDPSTATUS_PARSEFAIL;

            /* build the number */
            n = strtol(txt,(char**)&txt,10);
            /* test the number is within the range */
            if (n < valSyn->iValue.iNumber.iMinNumber || (valSyn->iValue.iNumber.iMaxNumber && n > valSyn->iValue.iNumber.iMaxNumber))
                return RV_SDPSTATUS_PARSEFAIL;

            /* test that dot is legal */
            if ((*txt == '.' && valSyn->iValue.iNumber.iAfterDotDigitMax == 0) || /* dot appears but is illegal */
                (*txt != '.' && valSyn->iValue.iNumber.iAfterDotDigitMax != 0))   /* dot has to be but is not */
                return RV_SDPSTATUS_PARSEFAIL;

            if (*txt == '.')
            {
                /* count the number of digits after the dot */
                p = ++txt;
                while (isdigit((int)*p)) 
                    p++;
                /* and test if the count is within the range */
                if ((unsigned)(p - txt) > valSyn->iValue.iNumber.iAfterDotDigitMax)
                    return RV_SDPSTATUS_PARSEFAIL;
                txt = p;
            }
        }
        else if (valSyn->iValueSwitch == RvSdpValueText)
        /* text case */
        {
            p = valSyn->iValue.iText.iText;
         
            if (valSyn->iValue.iText.iTxtCount == 0)
            /* the case of any text (no predefined literals) */
            {
                /* find the end of string or list separator */
                while (isalnum((int)*txt) || *txt == '_' || *txt == '-')
                    txt++;
            }
            else 
            { 
                RvSize_t maxN;
                
                maxN = 0;
                /* test the literal appears in the list of predefined literals */
                for (cnt = 0; cnt < valSyn->iValue.iText.iTxtCount; cnt++)
                {                
                    /* for every predefined literal compare it with the txt 
                       and keep the length of the longest match */
                    n = strlen(p);
                    if (n <= maxN)
                        continue;

                    if (strncasecmp(p,txt,n) == 0)
                        /* found currently longest match */
                        maxN = n;
                    p += n+1;
                }

                if (maxN == 0)
                    /* match was not found*/
                    return RV_SDPSTATUS_PARSEFAIL;
                /* move the txt after the match */
                txt += maxN;
            }
        }

        /* increment the number of matched strings */
        elCnt ++;

        if (valSyn->iNextVal)
        {
            if (!*txt)
                return RV_SDPSTATUS_PARSEFAIL;
            valSyn = valSyn->iNextVal;
            continue;
        }

        if (!valSyn->iIsList)
        {
            if (*txt)
                /* no symbol can be here */
                return RV_SDPSTATUS_PARSEFAIL;
            return RV_SDPSTATUS_OK;
        }

        /* list of strings case */

        if (*txt == 0)
        {
            if (elCnt < valSyn->iListMinApp)
                /* number of list elements is less than min required */
                return RV_SDPSTATUS_PARSEFAIL;
            return RV_SDPSTATUS_OK;
        }

        if (valSyn->iListMaxApp && elCnt == valSyn->iListMaxApp && *txt)
            /* number of list elements is bigger than max allowed */
            return RV_SDPSTATUS_PARSEFAIL;

        if (*txt != valSyn->iSepar)
            /* only separator symbol can be here */
            return RV_SDPSTATUS_PARSEFAIL;
        txt++;
    }

    return RV_SDPSTATUS_OK;
}

/*
 *	builds name-value pair based on name and value fetched from in the input buffer
 */
RvSdpStatus RvSdpCodecParseNameValue(RvSdpCodecParamsInternal* prms, RvSdpCodecParserInternal *cp, 
                                  RvSdpCodecParamData *fmtpData, const char* value)
{
    /* perform validation */
    if ((fmtpData->iValueSyntax && value == NULL) ||
        (!fmtpData->iValueSyntax && value))
    {
        /* there is a value but it is not expected OR
           the value is expected but does not present */ 
        return (*cp->iParsingErrorCB)(prms,fmtpData,value);
    }
    else if (fmtpData->iValueSyntax)
    {
        /* the value is expected AND is set */
        /* try to validate it */
        if (RvSdpFtmpValueValidate(fmtpData->iValueSyntax,value) != RV_SDPSTATUS_OK)
            /* the validation failed */
            return (*cp->iParsingErrorCB)(prms,fmtpData,value);
    }
    
    return (*cp->iFmtpTreatParamCB)(prms,fmtpData,value);
}

/*
 *	Parses the fmtp attribute when constructing the params structure;
 */
RvSdpStatus RvSdpParseFmtp(RvSdpCodecParamsInternal* prms, RvSdpCodecParserInternal *cp)
{
    char *name, *value, *txt;
    int nameLen, valueLen;
    RvSdpCodecParamData *fmtpData;
    RvSdpStatus retV;
    
    txt = prms->iFmtpValue;
    
    retV = RV_SDPSTATUS_OK;
    
//    while (RV_TRUE)
    for (;;)
    {
        if (RvSdpCodecGetNameEqualValue(&txt,&name,&nameLen,&value,&valueLen,cp->iSeparator) != RV_SDPSTATUS_OK)
            break;

        name[nameLen] = 0;
        if (value)
            value[valueLen] = 0;
                
        /* find the correspondent RvSdpCodecParamData instance in RvSdpCodecParserInternal */
        fmtpData = NULL;
        if (isalpha((int)*name))
        {
            fmtpData = cp->iFmtpParamData[tolower(*name)-'a'];
            while (fmtpData && strcasecmp(fmtpData->iParamName,name))
                fmtpData = fmtpData->iNextParam;
        }

        if (!fmtpData)
        {
            /* some unregistered field */
            retV = (*cp->iFmtpUnknownNameCB)(prms,name,value);
            if (retV != RV_SDPSTATUS_OK)
                goto finish;
            continue;
        }

        retV = RvSdpCodecParseNameValue(prms,cp,fmtpData,value);
        if (retV != RV_SDPSTATUS_OK)
            goto finish;        
    }
    
finish:    
        
    return retV;
}

const char* 
RvSdpFindAttribute(RvSdpMediaDescr* media, RvBool searchInMedia, RvSdpCodecParamData *attrData)
{
    RvSize_t attrsNum, cnt;
    RvSdpAttribute *attrP;

    if (searchInMedia)
        attrsNum = rvSdpMediaDescrGetNumOfAttr(media);
    else
        attrsNum = rvSdpMsgGetNumOfAttr(media->iSdpMsg);
    
    for (cnt = 0; cnt < attrsNum; cnt++)
    {
        if (searchInMedia)
            attrP = rvSdpMediaDescrGetAttribute(media, cnt);
        else
            attrP = rvSdpMsgGetAttribute(media->iSdpMsg, cnt);
        
        if (strcasecmp(attrP->iAttrName,attrData->iParamName) == 0)
            return attrP->iAttrValue;
    }

    return NULL;    
}

/*
 *	Parses the attributes when constructing the params structure;
 */
RvSdpStatus RvSdpParseCodecAttrs(RvSdpCodecParamsInternal* prms, RvSdpCodecParserInternal *cp)
{
    RvSdpCodecParamData *attrData;   
    const char* attrVal;
    RvSdpStatus retV;

    attrData = cp->iAttrParamData;

    while (attrData)
    {

        /* find this attr data in the list of attributes of the media descriptor 
           and then later in the list of attributes of the whole message */

        attrVal = RvSdpFindAttribute(prms->iMediaDescr,RV_TRUE,attrData);
        if (attrVal == NULL)
            attrVal = RvSdpFindAttribute(prms->iMediaDescr,RV_FALSE,attrData);

        if (attrVal)
        {
            retV = RvSdpCodecParseNameValue(prms,cp,attrData,attrVal);
            if (retV != RV_SDPSTATUS_OK)
                return retV;
        }                        
        attrData = attrData->iNextParam;
    }
    return RV_SDPSTATUS_OK;
}

/*
 *	Standard procedure when non-registered parameters is encountered when 
    parsing FMTP attribute
 */
RvSdpStatus RvSdpCodecParsingUnknownParam(RvSdpCodecParamsInternal* prms, const char* name, const char* value)
{
    RvSdpNameValue *nv;
    
    prms->iUnknownParamsCnt ++;

    nv = RvSdpConstructNameValue(prms->iAllocator,name,value);
    if (!nv)
        return RV_SDPSTATUS_ALLOCFAIL;
    RvSdpAddNameValue(&prms->iCodecNVL,nv);
    return RV_SDPSTATUS_OK;
}

/*
 *	Standard procedure when parameter's value does not match the predefined pattern
 */
RvSdpStatus RvSdpCodecParamParsingError(RvSdpCodecParamsInternal* prms, 
                                     RvSdpCodecParamData *fmtpData, const char* value)
{
    RvSdpNameValue *nv;
    
    prms->iBadSyntaxParamsCnt ++;
    
    nv = RvSdpConstructNameValue(prms->iAllocator,fmtpData->iParamName,value);
    if (!nv)
        return RV_SDPSTATUS_ALLOCFAIL;
    nv->iValueOK = RV_FALSE;
    RvSdpAddNameValue(&prms->iCodecNVL,nv);
    return RV_SDPSTATUS_OK;
}

/*
 *	Standard procedure when parameter is successfully parsed;
 */
RvSdpStatus RvSdpCodecParsingNextParam(RvSdpCodecParamsInternal* prms, 
                                    RvSdpCodecParamData *fmtpData, const char* value)
{
    RvSdpNameValue *nv;
    
    nv = RvSdpConstructNameValue(prms->iAllocator,fmtpData->iParamName,value);
    if (!nv)
        return RV_SDPSTATUS_ALLOCFAIL;
    nv->iValueOK = RV_FALSE;
    RvSdpAddNameValue(&prms->iCodecNVL,nv);
    return RV_SDPSTATUS_OK;
}  
  
/*
 *	Sets the default callbacks for if not set by specific callback
 */
void RvSdpCodecSetCallbacks(RvSdpCodecParserInternal* cp)
{
    if (cp->iParsingErrorCB == NULL)
        cp->iParsingErrorCB = 
                        (RvSdpCodecParseErrorCB)RvSdpCodecParamParsingError;

    if (cp->iFmtpUnknownNameCB == NULL)
        cp->iFmtpUnknownNameCB = 
                        (RvSdpCodecParseUnknownParamCB)RvSdpCodecParsingUnknownParam;

    if (cp->iFmtpTreatParamCB == NULL)
        cp->iFmtpTreatParamCB = 
                        (RvSdpCodecParseNextParamCB)RvSdpCodecParsingNextParam;
}

RvSdpStatus RvSdpCodecModifyFmtpParam(
    RvSdpMediaDescr* mediaD, 
    const RvChar* name, 
    const RvChar* value,
    RvBool isRemove,
    RvBool removeAttrIfLast,
    RvBool addIfNotExist,
    RvBool isGet,
    RvInt* getIndex,
    RvBool isCount,
    RvInt *count,
    RvInt index,
    RvChar* valBuff,
    RvInt valBuffLen,
    RvUint32 payload,
    RvChar separator,
    RvAlloc* a)
{
    RvSdpAttribute* attrP;
    RvUint32 num;
    const char *attrVal = NULL, *ptr;
    RvSdpListIter iter;
    RvSize_t numOfFmtp;
    int allocLen = 0, nLen = 0, vLen = 0, newValLen, newNameLen, attrValLen,cnt;
#define BUFF_SZ 256
    char buff[BUFF_SZ], *b = NULL, *allocBuf = NULL, *bInit = NULL, *n = NULL, *v = NULL;
    RvBool found;
    RvStatus ret;

    if (count)
        *count = 0;

    if (getIndex)
        *getIndex = -1;

    numOfFmtp = rvSdpMediaDescrGetNumOfFmtp(mediaD);
    
    for ( attrP = rvSdpMediaDescrGetFirstFmtp (mediaD,&iter);attrP; 
          attrP=(RvSdpAttribute*)rvSdpMediaDescrGetNextFmtp(&iter))
    {
        attrVal = ptr = rvSdpAttributeGetValue(attrP);
        num = strtol(ptr,(char**)&ptr,10);        
        if (num == payload)
            break;
    }    

    if (attrP == NULL && (isGet || isRemove))
        /* could not find ftmp attribute with matching payload */
        return RV_SDPSTATUS_NULL_PTR;

    if (attrP == NULL && isCount)
        return RV_SDPSTATUS_OK;


    found = RV_FALSE;
    cnt = 0;
    if (attrP)
    {
        for (;;) 
        {
            if (RvSdpCodecGetNameEqualValue((char**)&ptr,&n,&nLen,&v,&vLen,separator) != RV_SDPSTATUS_OK)
                break;
            cnt++;
            if (isCount)
                continue;
            if ((name && strncmp(name,n,nLen) == 0) || (!name && cnt-1 == index))
            {
                found = RV_TRUE;
                if (getIndex)
                    *getIndex = cnt-1;
                break;
            }
        }
    }

    if (isCount)
    {
        *count = cnt;
        return RV_SDPSTATUS_OK;
    }

    if (!found && (isGet || isRemove || !addIfNotExist))
        /* trying to get OR to remove nonexisting 'name' */
        return RV_SDPSTATUS_NOT_EXISTS;



    if (isGet)
    {
        if (valBuff == NULL)
            return RV_SDPSTATUS_OK;
        if (valBuffLen-1 < vLen)
            return RV_SDPSTATUS_SMALL_BUFFER;
        if (vLen)
            memcpy(valBuff,v,vLen);
        valBuff[vLen] = 0;
        return RV_SDPSTATUS_OK;
    }

    attrValLen = (attrVal) ? strlen(attrVal):0;
    
    if (!isRemove)
    {
        newValLen = (value)?strlen(value):0;
        newNameLen = strlen(name);
    /* need to use the some buffer (to build the value) only if the value is added or replaced */
        allocLen = attrValLen;
        if (!found)
        {
            allocLen += 1/*separator*/+newNameLen/*name*/+1/*'='sign*/+newValLen;
            if (!attrP)
                allocLen += 10; /* need a room for payload */
        }
        else
            allocLen += newValLen-vLen + 10;

        if (allocLen > BUFF_SZ)
        /* the size of 'buff' is not enough, need to allocate */
        {
            if (!a)
                a = rvSdpGetDefaultAllocator();
            allocBuf = rvSdpAllocAllocate(a,allocLen);
            if (!allocBuf)
                return RV_SDPSTATUS_ALLOCFAIL;
            b = allocBuf;
        }
        else
            b = buff;

        bInit = b;

        if (!found)
        /* have to add the ';name=value' at the end */
        {
            if (!attrP)
            {
                sprintf(b,"%d ",payload);
                b += strlen(b);
            }
            else
            {
                memcpy(b,attrVal,attrValLen);
                b += attrValLen;
                *b++ = separator;
            }
            memcpy(b,name,newNameLen);
            b += newNameLen;
            if (value)
            {
                *b++ = '=';
                memcpy(b,value,newValLen);
                b += newValLen;
            }
            *b = 0;            
        }
        else
        {
            /* copy all before the value start */
            char *p;
            if (v)
                /* if value was found use it */
                p = v;
            else
                /* if not we take as an achor the byte next after last byte of the name */
                p = n + nLen;
            memcpy(b,attrVal,p-attrVal);
            b += p-attrVal;
            
            /* now copy the new value */
            if (value)
            {
                if (!v)
                    /* the value was not here, thus the equal sign was not here also
                        have to insert it */
                    *b++ = '=';
                memcpy(b,value,newValLen);
                b += newValLen;
            }
            else
            {
                /* there is no new value */

                if (v)
                {
                    /* have to remove the '=' that probably appears here */
                    b--;
                    while (*b == ' ' || *b == '\t')
                        b--;
                    if (*b != '=')
                        b++;
                }


            }

            p += vLen;

            /* test and treat the case : 'name=' there is equal sign but
            there is no value */
            if (!v)
            {
                int n = 0;
                while (*p == ' ' || *p == '\t' || *p == '=')
                {
                    p++;
                    n++;
                }
                if (n && separator == ' ' && *(p-1) == ' ')
                /* keep at least one separator */
                    p--;
            }


            /* copy the rest part of the attrVal */
            memcpy(b,p,attrVal+attrValLen-p);     
            b[attrVal+attrValLen-p] = 0;
        }
    }
    else
    {
        /* this is remove case thus we perform changes on the attrVal buffer
           find how many bytes need to be copied back */
        const char *pp;
        /* find the separator following the value */
        if (v)
            pp = v + vLen;
        else
            pp = n + nLen;
        while (*pp == ' ' || *pp == '\t')
            pp++;
        if (*pp == separator)
            pp++;
        if (*pp)
        {
            memcpy(n,pp,attrVal + attrValLen - pp);
            n[attrVal + attrValLen - pp] = 0;
        }
        else
        { 
            /* we are at the end of attrVal, this is the last name=value */
            if (cnt == 1 && removeAttrIfLast)   
            {
                /* and this is the only one name=value
                   remove this attribute at all */
                rvSdpMediaDescrRemoveCurrentFmtp(&iter);
                return RV_SDPSTATUS_OK;
            }
            /* go back till you meet separator */
            n--;
            while (*n == ' ' || *n == '\t')
                n--;
            if (*n != separator)
                n++;
            *n = 0;
        }
        bInit = (char*)attrVal;
    }


    ret = (rvSdpMediaDescrAddFmtp(mediaD,bInit)) ? RV_SDPSTATUS_OK:RV_SDPSTATUS_ALLOCFAIL;
    if (attrP)
        /* there is no change value for FMTP attributes, thus
           we first remove the current FMTP and adding the new one */
        rvSdpMediaDescrRemoveCurrentFmtp(&iter);

    if (allocBuf)
        rvSdpAllocDeallocate(a,allocLen,allocBuf);

    return ret;
}

RVSDPCOREAPI RvSdpStatus RvSdpCodecFmtpParamRemoveByName(
    RvSdpMediaDescr* descr, 
    const RvChar* name,
    RvBool removeAttrIfEmpty,
    RvUint32 payload,
    RvChar separator)
{
     return  RvSdpCodecModifyFmtpParam(descr,name,NULL,RV_TRUE,
         removeAttrIfEmpty,RV_FALSE,RV_FALSE,NULL,RV_FALSE,NULL,0,NULL,0,payload,separator,NULL);
}


RVSDPCOREAPI RvSdpStatus RvSdpCodecFmtpParamRemoveByIndex(
    RvSdpMediaDescr* descr, 
    RvInt index,
    RvBool removeAttrIfEmpty,
    RvUint32 payload,
    RvChar separator)
{
    return RvSdpCodecModifyFmtpParam(descr,NULL,NULL,RV_TRUE,removeAttrIfEmpty,RV_FALSE,
        RV_FALSE,NULL,RV_FALSE,NULL,index,NULL,0,payload,separator,NULL);
}

RVSDPCOREAPI RvSdpStatus RvSdpCodecFmtpParamGetByName(
    RvSdpMediaDescr* descr, 
    const RvChar* name,
    RvUint32 payload,
    RvChar separator,
    RvChar* value,
    RvInt valLen,
    RvInt* index)
{
     return  RvSdpCodecModifyFmtpParam(descr,name,NULL,RV_FALSE,RV_FALSE,
         RV_FALSE,RV_TRUE,index,RV_FALSE,NULL,0,value,valLen,payload,separator,NULL);
}

RVSDPCOREAPI RvSdpStatus RvSdpCodecFmtpParamGetByIndex(
    RvSdpMediaDescr* descr, 
    RvInt index,
    RvUint32 payload,
    RvChar separator,    
    RvChar* value,
    RvInt valLen)
{
     return  RvSdpCodecModifyFmtpParam(descr,NULL,NULL,RV_FALSE,RV_FALSE,
         RV_FALSE,RV_TRUE,NULL,RV_FALSE,NULL,index,value,valLen,payload,separator,NULL);
}


RVSDPCOREAPI RvSdpStatus RvSdpCodecFmtpParamSet(
    RvSdpMediaDescr* descr, 
    const RvChar* name,
    const  RvChar* value,
    RvBool addFmtpAttr,
    RvUint32 payload,
    RvChar separator,
    RvAlloc* a)
{
    return  RvSdpCodecModifyFmtpParam(descr,name,value,RV_FALSE,RV_FALSE,
        addFmtpAttr,RV_FALSE,NULL,RV_FALSE,NULL,0,NULL,0,payload,separator,a);
}

RVSDPCOREAPI RvInt RvSdpCodecFmtpParamGetNum(
    RvSdpMediaDescr* descr,  
    RvUint32 payload,
    RvChar separator)
{
    RvInt num;
    RvSdpCodecModifyFmtpParam(descr,NULL,NULL,RV_FALSE,RV_FALSE,
        RV_FALSE,RV_FALSE,NULL,RV_TRUE,&num,0,NULL,0,payload,separator,NULL);
    return num;
}


#endif /*RV_SDP_CODECS_SUPPORTED*/

