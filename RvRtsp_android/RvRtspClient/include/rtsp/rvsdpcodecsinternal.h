/******************************************************************************
Filename    :rvsdpcodecsinternal.h
Description : definitions regarding use of codecs internal structs

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

#ifdef RV_SDP_CODECS_SUPPORTED



#ifndef _rvsdpcodecsinternal_h_
#define _rvsdpcodecsinternal_h_ 

#include "rvalloc.h"
#include "rvsdpmedia.h"

#define RV_SDP_ENG_LETTERS          'z' - 'a' + 1
#define RV_SDP_MAX_PARAMS           30
#define RV_SDP_MAX_CODEC_NAMES      10
#define RV_SDP_MAX_CODECS           50

typedef enum 
{
    RvSdpValueDigit,
    RvSdpValueText
} RvSdpFtmpValueSyntaxEnum;

/*
 *	This structure contains information used to define the pattern of parameter value.
 *  That is whether the value is number (with its range) or text (with possible patterns)
 */
typedef struct _RvSdpFtmpValueSyntaxStr
{
    union {
        struct {RvSize_t iMinNumber, iMaxNumber, iAfterDotDigitMax;} iNumber;        
        struct {char *iText; RvSize_t iTxtCount,iTxtLen;} iText;
    } iValue;    
    RvSdpFtmpValueSyntaxEnum iValueSwitch;
    RvBool iIsList;                             /* whether that is a list of values */
    RvSize_t iListMinApp,iListMaxApp;           /* range of elements number in the list */
    char iSepar;                                /* symbol used as list separator */
    RvAlloc*  iAllocator;                   
    struct _RvSdpFtmpValueSyntaxStr* iNextVal;  /* used for the list of patters */
                                                /* (don't mix with the list of same elements */
} RvSdpFtmpValueSyntaxStr;


/*
 *	Data necessary for parsing of one parameter;
 */
typedef struct _RvSdpCodecParamData
{
    RvAlloc*                            iAllocator;
    char*                               iParamName;     /* the parameter's name */  
    RvSize_t                            iParamNameLen;  
    int                                 iParamNum;      /* the numerical representation of */
                                                        /* of the parameter                */
    RvSdpFtmpValueSyntaxStr*            iValueSyntax;   /* the pattern of parameter value */
    struct _RvSdpCodecParamData*        iNextParam;
} RvSdpCodecParamData;

/* will be called when parsing before the work starts */
typedef RvSdpStatus (*RvSdpCodecParseStartCB)(void* usrData);

/* will be called when parsing after the work ends */
typedef RvSdpStatus (*RvSdpCodecParseFinishCB)(void* usrData);

/* will be called when parsing when the unregistered parameter name is encountered */
typedef RvSdpStatus (*RvSdpCodecParseUnknownParamCB)(void* usrData, const char* name, const char* value);

/* will be called when parsing when the parameter value does not match the predefined pattern */
typedef RvSdpStatus (*RvSdpCodecParseErrorCB)(void* usrData, RvSdpCodecParamData *fmtpData, const char* value);

/* will be called when parsing for each successfully parsed parameter */
typedef RvSdpStatus (*RvSdpCodecParseNextParamCB)(void* usrData, RvSdpCodecParamData *fmtpData, const char* value);

/* the internal representation of codec parameters parser */
typedef struct _RvSdpCodecParserInternal
{
    RvAlloc                             *iAllocator;

	/*define 5 optional callbacks that each codec can implement
      RvSdpCodecParseStartCB  - will be called when parsing before the work starts.
      RvSdpCodecParseFinishCB - will be called when parsing after the work ends.
      RvSdpCodecParseErrorCB - will be called when parsing when the parameter 
                               value does not match the predefined pattern. 
      RvSdpCodecParseNextParamCB - will be called when parsing for each 
	                               successfully parsed parameter. 
      RvSdpCodecParseUnknownParamCB - will be called when parsing when the unregistered 
	                                  parameter name is encountered.*/
    RvSdpCodecParseStartCB              iStartFmtpParsingCB;    /* callbacks */
    RvSdpCodecParseFinishCB             iFinishFmtpParsingCB;   /* used during parsing */
    RvSdpCodecParseUnknownParamCB       iFmtpUnknownNameCB;
    RvSdpCodecParseErrorCB              iParsingErrorCB;
    RvSdpCodecParseNextParamCB          iFmtpTreatParamCB;    
    
    /* defines the list of parameters  that can be set in FMTP attribute*/
    /* exanple: a=fmtp:97 profile-level-id=15; object=2; cpresent=0; config=430028100000 */ 
    RvSdpCodecParamData                 *iFmtpParamData[RV_SDP_ENG_LETTERS]; 

    /* symbol used as a separator in fmtp attribute */
    char                                iSeparator;
    
    /* defines the list of parameters  that can be set as separate attributes */
    /* example: a=ptime:20 */
    RvSdpCodecParamData                 *iAttrParamData;

    /* list of codec names */
    char*                               iCodecNames[RV_SDP_MAX_CODEC_NAMES];

} RvSdpCodecParserInternal;


/*
 *	contains data necessary to define parsing data for parser
 */
typedef struct 
{
    int iParamNum;
    const char* iFmtpParamValueSyntax;
} RvSdpParamStaticParserData;

/* name-value construction */
typedef struct _RvSdpNameValue
{
    const char*   iName;
    const char*   iValue;
    RvBool        iValueOK;
    struct _RvSdpNameValue *iNextInList;    
} RvSdpNameValue;

/* list of name-values hashed by first symbol of name*/
typedef struct 
{
    RvSdpNameValue* iNVArray[RV_SDP_MAX_PARAMS];
    RvSdpNameValue* iHeadsVector[RV_SDP_ENG_LETTERS];
    RvSdpNameValue* iTailsVector[RV_SDP_ENG_LETTERS];
    int iNVCount;
} RvSdpNameValuesList;

/* internal representation of parsed codec parameters*/
typedef struct {
    RvAlloc             *iAllocator;
    int                 iCodecParamsParserState;
    RvSdpNameValuesList iCodecNVL;                  /* the parsed name-values */
    RvSdpMediaDescr     *iMediaDescr;               

    char*               iFmtpValue;                 /* the pre-allocated value of fmtp attribute*/
    RvSize_t            iFmtpValueLen;

    RvSize_t            iUnknownParamsCnt;          /* number of unknown parameters */
    RvSize_t            iBadSyntaxParamsCnt;        /* number of params with bad value */

    RvBool              iFmtpBadSyntax;             /* whether the params match the grammar */
} RvSdpCodecParamsInternal;


/*
 *	Constructs the params parser.
 *  This function is called by each supported codec in its initialization function.
 */
RvSdpCodecParserInternal* RvSdpCodecParserInternalConstruct(const RvSdpParamStaticParserData *fpdArr, 
                                                            const RvSdpParamStaticParserData *apdArr, 
                                                            RvAlloc* a);

/*
 *	Destructs the parser;
 */
void RvSdpCodecParserInternalDestruct(RvSdpCodecParserInternal* cp);

/* 
 *  Registers the parser structure in the table of supported codecs parser 
 */
RvSdpStatus RvSdpCodecRegister(RvSdpCodecParserInternal* cp);

/*
 *	Sets the default callbacks for if not set by specific callback
 */
void RvSdpCodecSetCallbacks(RvSdpCodecParserInternal* cp);

/*
 *	Constructs one parameter parser;
 */
RvSdpCodecParamData* RvSdpParamDataConstruct(int fieldNum, const char* txt, RvAlloc *a);

/*
 *	Destructs it;
 */
void RvSdpFmtpParamDataDestruct(RvSdpCodecParamData* fd);


/*
 *	Parses the fmtp attribute when constructing the params structure;
 */
RvSdpStatus RvSdpParseFmtp(RvSdpCodecParamsInternal* prms, RvSdpCodecParserInternal *cp);

/*
 *	Parses the attributes when constructing the params structure;
 */
RvSdpStatus RvSdpParseCodecAttrs(RvSdpCodecParamsInternal* prms, RvSdpCodecParserInternal *cp);


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
RvSdpFtmpValueSyntaxStr* RvSdpFtmpValueSyntaxConstruct(const char* txt, RvAlloc* a);

/*
 *	Destructs the value syntax structure;
 */
void RvSdpFtmpValueSyntaxDestruct(RvSdpFtmpValueSyntaxStr* valSyn);

/*
 *	Standard procedure when non-registered parameters is encountered when parsing fmtp attribute
 */
RvSdpStatus RvSdpCodecParsingUnknownParam(RvSdpCodecParamsInternal* prms, const char* name, const char* value);

/*
 *	Standard procedure when parameter's value does not match the predefined pattern
 */
RvSdpStatus RvSdpCodecParamParsingError(RvSdpCodecParamsInternal* prms, 
                                     RvSdpCodecParamData *fmtpData, const char* value);

/*
 *	Standard procedure when parameter is successfully parsed;
 */
RvSdpStatus RvSdpCodecParsingNextParam(RvSdpCodecParamsInternal* prms, 
                                    RvSdpCodecParamData *fmtpData, const char* value);
    


#endif /*RV_SDP_CODECS_SUPPORTED*/
                                
#endif
