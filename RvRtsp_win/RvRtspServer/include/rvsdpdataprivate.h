/******************************************************************************
Filename    :rvsdpdataprivate.h
Description : definition of data structures not used directly by library user

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

#ifndef _rvsdpdataprivate_h_
#define _rvsdpdataprivate_h_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "rvsdplist.h"

/* To keep typed time data */
typedef struct 
{
    RvListNode              iNextTypedTime;     /* the instances of this struct are kept in the LList
                                                 * thus next is required */
    RvUint32                iTimeValue;         
    RvSdpTimeUnit           iTimeType;
    RvBool                  iPrivateAllocation; /* whether was allocated (RV_TRUE) or taken from pool */
    void*                   iObj;               /* points whether to RvSdpMsg or to rvAllocator*/
} RvSdpTypedTime;


/* each SDP line is represented by some sdp data type (RvSdpOrigin, RvSdpConnection etc) 
   thus each such type contains data member of RvSdpLineObject in order to keep all 
   SDP line objects as one linked list */
typedef struct _RvSdpLineObject
{
    struct _RvSdpLineObject*    iNext;     
    struct _RvSdpLineObject*    iPrev;
    RvUint16                    iOffset;        /* the offset of the RvSdpLineObject field from
                                                   the beginning of the SDP structure */
    RvSdpFieldTypes             iFieldType;     /* the value is from RvSdpFieldTypes */
    RvUint8                     iUsed;          /* if this specific SDP object is a part
                                                   of SDP message */
} RvSdpLineObject;

/* used when iterating on line objects (RvSdpLineObject) mainly for bad syntax objects */
typedef struct 
{
	RvSdpLineObject			*iCurrentLO;    /* the currently pointed  line object */
	struct _RvSdpMediaDescr *iMediaDescr;   /* if iterating on bad syntax objects held
                                               in media descriptor */
	struct _RvSdpMsg		*iMsg;          /* if iterating on bad syntax objects held
                                               in message */
} RvSdpLineObjIter;

/* possible errors when parsing integer number */
typedef enum
{
    RvSdpIntNonWarning,
    RvSdpWarnNonZeroDigitExpected,
    RvSdpWarnDigitExpected,
    RvSdpWarnNumberIsNotInRange,
    RvSdpWarnNumberOverflow
} RvSdpIntegerParseWarning;

/* status of single line parsing 
   this status defines the next parser action through the 
   gsParseActions array where the RvSdpLineParseStatus values are  used
   as an indices */
typedef enum 
{
    RvSdpParseEmptyLine = 0,        /* empty line */
    RvSdpParseEmptyLineAtStart =1,  /* empty line at the beginning of the buffer */
    RvSdpParseDot =2,               /* line containing '.' */
    RvSdpParseCloseBrace =3,        /* line containing '}' */
    RvSdpParseEndOfBuffer =4,       /* end of input buffer reached */
    RvSdpLineSyntaxOK =5,           /* correct SDP line: tag=value */
    RvSdpLineTagOrderingOK =6,      /* the tag ordering was fine */
    RvSdpLineTagMultipleOK =7,      /* the tag multiple appearance test was fine */
    RvSdpParseNoTag =8,             /* line that with the first symbol which is not letter*/
    RvSdpParseNoEqualSign =9,       /* there is no '=' sign after the tag letter */
    RvSdpParseNoValue =10,           /* there is no value after the '=' sign */
    RvSdpParseMultipleNotAllowed =11,/* the tag multiple appearance test failed */ 
    RvSdpParseMissingTag =12,        /* some mandatory tag (that had to be before the 
                                     * current tag) does not appear in the input */
    RvSdpParseTagOutOfOrder =13,     /* the current tag had to be somewhere before */
    RvSdpParseUnknownTagInMsg =14,   /* this tag is unknown in message context */
    RvSdpParseUnknownTagInMedia =15, /* this tag is unknown in media context */
    RvSdpValueParseFailed =16,       /* the tag's value does not comply with the standard*/
    RvSdpStatusIgnore =17,           /* this SDP line has to be ignored though it is 
                                     * correct. for example  there is 'r=' line but there
                                     * is no 't=' line before */
    RvSdpParseUnknownStatus =18      /* special usage */
} RvSdpLineParseStatus;

/* possible next steps after single line parsing */
typedef enum
{
    RvSdpActTestMultiple = 0,    /* test if tag's multiple appearance is allowed */
    RvSdpActTestTagOrdering = 1, /* test that tag appears in the standard desaignated
                                    place and that all mandatopry tags are present */
    RvSdpActParseTagValue = 2,   /* test that tag's value is standatd compliant */
    RvSdpActCreateAsOther = 3,   /* the tag is unknown in its context, thus create
                                    RvSdpOther object */
    RvSdpActMissingTag = 4,      /* some mandatory tag missing, create warning and 
                                    continue the test for missing tags */
    RvSdpActStopParsing = 5,     /* parsing has to be stopped, due to the end of 
                                    input buffer or there is some critical error */
    RvSdpActIgnoreLine = 6,      /* this line should be ignored */
    RvSdpActReplaceExisting = 7, /* there is multiple appearance of the tag 
                                    allowing at most one appearance, destroy the
                                    previous object and create it again */
    RvSdpActCreateBadSyntax =8   /* the value of the tag is not standard compliant,
                                    create the object as bad syntax */
} RvSdpParseNextStep;

/* contains data defining behavior of parser depending on line parse results;
 * there is array 'gsParseActions' of type 'RvSdpParseActions' indexed by 
 * values of enum RvSdpLineParseStatus .
 * Line parsing ends up with some value from RvSdpLineParseStatus
 * and this value defines the next parser actions */
typedef struct _RvSdpParseActions
{
    RvSdpParseNextStep      iNextStep;          /* defines the next parser action */
    RvBool                  iCreateWarning;     /* whether the warning has to be created
                                                   as the result of current line parsing*/
    RvSdpParseStatus        iStopParseStatus;   /* whether parser should stop */
    RvSdpParseWarning       iWarningNum;        /* the warning type 
                                                   (used if iCreateWarning if true) */
} RvSdpParseActions; 

struct _RvSdpParserData;
struct _RvSdpParseActions;

/* Parser warning data */
typedef struct 
{
    struct _RvSdpParserData* iParserData;
    RvListNode              iNextWarning;  /* instances of RvSdpParserWarningData are kept in LList
                                            * thus 'next' is needed */
    RvUint16                iLineOfEvent;  /* the line number in the input where the warning was generated */
    RvChar                  iMissedTag;    /* if not zero tells tag which is missing in the iput */
    RvChar                  iCurrentTag;   /* tells the current tag */
    RvSdpParseWarning       iWarningNum;   /* the number of the warning */
} RvSdpParserWarningData;


/* creates pasing warning based on the data in 'act' */
RvBool rvSdpCreateWarning(
            struct _RvSdpParserData* pD, 
            const struct _RvSdpParseActions* act);

#define RV_SDP_WARNING_SET_AND_CREATE_WARNING(_c1,_c2)  \
{                                                       \
    (_c2)->iWarning.iWarningNum = (_c1);                \
    rvSdpCreateWarning((_c2),NULL);                     \
}


extern const unsigned int gsSymbolMsks[];

/* set of macros used to replace some characters in a string by zeros while
   preserving the possibility to restore the orginal string state */

#define REPLACING_START     \
    char _chars[30];        \
    int _charsCnt = 0;      \
	char* _pos[30];

#define REPLACING_ARGS  _chars, &_charsCnt, _pos

#define REPLACING_ARGS2 _chars, _charsCnt, _pos

#define REPLACING_DECL  char* _chars, int* _charsCnt, char** _pos

#define UNUSE_REPLACING_DECL  RV_UNUSED_ARG(_chars); RV_UNUSED_ARG(_charsCnt); RV_UNUSED_ARG(_pos);


#define _REPLACING(_p,_cnt)         \
{                               \
    _chars[_cnt] =      *(_p);  \
    *(_p) = 0;                  \
    _pos[_cnt++] = (_p);        \
}

#define REPLACING(_p)  _REPLACING(_p,_charsCnt)
#define REPLACING2(_p) _REPLACING(_p,(*_charsCnt))

#define REPLACING_UNDO                      \
{                                           \
    char **_Pos = _pos, *_Chars = _chars;   \
    while (_charsCnt--)                     \
	**(_Pos++) = *(_Chars++);           \
}

struct _RvSdpAttribute;
struct _RvSdpMsg;
struct _RvSdpParserData;
struct _RvSdpCommonFields;
struct _RvSdpMediaDescr;
struct _RvSdpSessionTime;
struct _RvSdpRtpMap;
struct _RvSdpInformation;

/* possible results of special attributes parsing */
typedef enum {
	rvSdpSpAttrPrsAllocFail,
	rvSdpSpAttrPrsFail,
	rvSdpSpAttrPrsCrtRegular,
	rvSdpSpAttrPrsOK	
} RvSdpSpecAttrParseSts;

/* function responsible for parsing of specific 'tag' value */
typedef RvBool (*rvSdpParseFieldValueFunc)(struct _RvSdpParserData* pd, RvBool crtBadSyntax, RvBool replaceExisting);
/* this  function tests whether the specific tag line already presents in a parsed SDP
   input. This is needed for tags not allowing multiple appearnces (like 'v', 'i' etc */
typedef RvBool (*rvSdpTestMultipleTagFunc)(struct _RvSdpParserData* pd);

/* contains data needed to parse the specific SDP tag */
typedef struct 
{
    RvInt16                     iIndex;
    RvChar                      iTag;                   /* the tag letter */
    RvBool                      iMandatory;             /* whether the tag is mandatory
                                                           or optional */
    rvSdpTestMultipleTagFunc    iTestMultipleTag;       /* function testing whether this 
                                                           line tag already presents 
                                                           in a SDP input */
    rvSdpParseFieldValueFunc    iValueParseFunc;        /* tag value parser */ 
    RvBool                      iAllowSpaceValues;      /* does this tag allows values consisting 
                                                           of spaces only */
} RvSdpTagDef;


/* this struct contains the data used during parsing 
    one instance of this struct is used during parsing */
typedef struct _RvSdpParserData
{
    const RvSdpTagDef       *iPrevTagDef;  /* the previous tag data, used to test 
                                              the correct order of SDP lines */
    RvChar                  *iCurrPtr;     /* the current pointer in the input */
    RvChar                  *iEndOfData;   /* points to the last byte in parsed input*/
    RvUint16                iLinesCnt;     /* lines counter */
    RvChar                  iCurrTag;      /* the current tag letter */
    const RvSdpTagDef       *iCurrTagDef;  /* current tag data */
    RvChar                  *iCurrValue;   /* the current line value (after '=' char) */
    RvUint16                iCurrValueLen; /* the length of the current value */
    RvSdpLineParseStatus    iLineStatus;   /* the status of current line parsing
                                              will define the parser next action through
                                              gsParseActions array */             
    RvSdpParseStatus        iStopParseSts;  /* if the  parser was stopped, this reason
                                               will be set here */
    const RvSdpTagDef       *iExpectedTag;  /* the next expected tag */
    struct _RvSdpMsg        *iMsg;          /* the constructed message */
    struct _RvSdpMediaDescr *iMediaDescr;   /* the constructed media descriptor */
    struct _RvSdpSessionTime  *iSessionTime;/* the constructed session time, we need this 
                                               pointer for easier appending 'r' lines */
    RvBool                  iInMediaDescr;  /* if set to RV_TRUE parser is in the context
                                               of media descriptor otherwise we are in
                                               message  context */
    RvBool                  iStopping;      /* parser is about to finish, thus
                                               we need to perform some final tests which 
                                               are impossible till there is some unparsed
                                               input */
    RvSdpLineParseStatus    iSavedLineStatus; 
              /* this field is used when the when the last line of SDP input is parsed
               * and the parsing process can be finished but we still need to test
               * that all mandatory tags present. Thus we save in this field the actual
               * status of the last lien parsing replace this  status by OK*/

    RvBool                  iCriticalError; /* if set to RV_TRUE the parser will stop */                                             
     
#ifdef RV_SDP_PARSE_WARNINGS
    RvSdpList               iWarningsList;/* the linked list of collected parse warnings*/
    RvSdpObjPool            iWarningsPool;/* the pool of warings */
#endif /*RV_SDP_PARSE_WARNINGS*/
    RvSdpParserWarningData  iWarning;  /* the currently used warning, used for logging */
    RvAlloc*                iAllocator; /* the allocator */
} RvSdpParserData;


struct _RvSdpSpecAttributeData;

/* set of callbacks for special attributes treatment */

/* callback to destroy the special attribute instance */
typedef void (*rvAttrDestructCB)(
            struct _RvSdpAttribute *attr);

/* callback to copy the special attribute instance from src to dst,
   msg points to SDP message where  dst attribute will be part of */
typedef struct _RvSdpAttribute* (*rvAttrCopyCB)(
            struct _RvSdpAttribute *dst, 
            const struct _RvSdpAttribute *src, 
            struct _RvSdpMsg *msg);

/* sets the 'txt' to the value of attribute (after ':' character) */
typedef char* (*rvAttrGetValueCB)(
            struct _RvSdpAttribute *dst, 
            char *txt);

/* callback for special attribute  reshuffling */
typedef void  (*rvAttrReshuffleCB)(
            struct _RvSdpAttribute *dst, 
            char **ptr);   /* here special attribute  will copy its string fields
                              while keeping the pointers to the copied strings locations*/

/* to parse special attribute value */
typedef RvSdpSpecAttrParseSts (*rvAttrParsingCB)(
            const struct _RvSdpSpecAttributeData *sad,
            RvBool createAsBadSyntax, 
            struct _RvSdpParserData* pD,
            struct _RvSdpCommonFields *cm, 
            char* n, 
            char* v, 
            REPLACING_DECL);

/* this structs holds data needed for special attribute treatment */
typedef struct _RvSdpSpecAttributeData {
    const char* iName;                  /* the name of attribute (rtpmap, key-mgmt etc)*/
    RvSdpFieldTypes iFieldType;         /* the enum of the field type */
    rvAttrDestructCB iDestructFunc;     /*  callbacks */
    rvAttrCopyCB iCopyFunc;
    rvAttrGetValueCB iGetValueFunc;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)	
    RvInt iBadValueOffset;              /* the offset of iBadSyntaxField from the 
                                           beginning of the special attribute struct */
#endif
    rvAttrReshuffleCB iReshuffleFunc;
    rvAttrParsingCB iParsingFunc;
	RvBool iIsMultiple;                 /* whether allows multiple appearance */
} RvSdpSpecAttributeData;

/* this two functions help in getting spec attr data from 
   the global array gcSpecAttributesData */
const RvSdpSpecAttributeData* rvSdpFindSpecAttrDataByFieldType(
            RvSdpFieldTypes specAttrType);
const RvSdpSpecAttributeData* rvSdpFindSpecAttrDataByName(
            const char* name);

#ifdef __cplusplus
}
#endif
#endif

