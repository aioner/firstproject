/******************************************************************************
Filename    :rvsdpmsgparse.c
Description : parse routines.

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
#include "rvsdp.h"
#include "rvsdpprivate.h"
#include "rvsdpprsutils.h"
#include "rvstrutils.h"

#include <stdio.h>
#include <ctype.h>


/*
 *	This  array defines parsing states machine.
 *  Each parse state results in some value from RvSdpLineParseStatus edum.
 *  And this value is used as index in this array to define the next parser
 *  action.
 */
const RvSdpParseActions gsParseActions[] =
{

    /*  iNextStep                       iStopParseStatus                            *
     *                  iCreateWarning                              iWarningNum     *
     *                                                                              */
    /*RvSdpParseEmptyLine*/
    {RvSdpActStopParsing,   RV_FALSE,   RV_SDPPARSER_STOP_BLANKLINE,RvSdpWarnNoWarning},
    /*RvSdpParseEmptyLineAtStart*/
    {RvSdpActIgnoreLine,    RV_FALSE,   RV_SDPPARSER_STOP_NOT_SET,  RvSdpWarnNoWarning},
    /*RvSdpParseDot*/
    {RvSdpActStopParsing,   RV_FALSE,   RV_SDPPARSER_STOP_DOTLINE,  RvSdpWarnNoWarning},
    /*RvSdpParseCloseBrace*/
    {RvSdpActStopParsing,   RV_FALSE,   RV_SDPPARSER_STOP_CLOSEBRACE,RvSdpWarnNoWarning},
    /*RvSdpParseEndOfBuffer*/
    {RvSdpActStopParsing,   RV_FALSE,   RV_SDPPARSER_STOP_ZERO,      RvSdpWarnNoWarning},
    /*RvSdpLineSyntaxOK*/
    {RvSdpActTestTagOrdering,0,         RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnNoWarning},
    /*RvSdpLineTagOrderingOK*/
    {RvSdpActTestMultiple,  0,          RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnNoWarning},
    /*RvSdpLineTagMultipleOK*/
    {RvSdpActParseTagValue, 0,          RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnNoWarning},
    /*RvSdpParseNoTag*/
    {RvSdpActCreateAsOther, RV_TRUE,    RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnNotSdpLine},
//    {RvSdpActStopParsing,   RV_TRUE,    RV_SDPPARSER_STOP_ERROR,     RvSdpWarnNoTag},
    /*RvSdpParseNoEqualSign*/
    {RvSdpActCreateAsOther, RV_TRUE,    RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnNotSdpLine},
//    {RvSdpActStopParsing,   RV_TRUE,    RV_SDPPARSER_STOP_ERROR,     RvSdpWarnNoEq},
    /*RvSdpParseNoValue*/
    {RvSdpActIgnoreLine,    RV_TRUE,    RV_SDPPARSER_STOP_ERROR,     RvSdpWarnNoVal},
    /*RvSdpParseMultipleNotAllowed*/
    {RvSdpActReplaceExisting,RV_TRUE,   RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnMultNotAllowed},
    /*RvSdpParseMissingTag*/
    {RvSdpActMissingTag,    RV_TRUE,    RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnMissingTag},
    /*RvSdpParseTagOutOfOrder*/
    {RvSdpActTestMultiple,  RV_TRUE,    RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnTagOutOfOrder},
    /*RvSdpParseUnknownTagInMsg*/
    {RvSdpActCreateAsOther, RV_TRUE,    RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnUnknownTagInMsg},
    /*RvSdpParseUnknownTagInMedia*/
    {RvSdpActCreateAsOther, RV_TRUE,    RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnUnknownTagInMedia},
    /*RvSdpValueParseFailed*/
    {RvSdpActCreateBadSyntax, RV_TRUE,  RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnPrivateWarning},
    /*RvSdpStatusIgnore*/
    {RvSdpActIgnoreLine,    RV_TRUE,    RV_SDPPARSER_STOP_NOT_SET,   RvSdpWarnPrivateWarning},
    /*RvSdpParseUnknownStatus*/
    {RvSdpActStopParsing,   RV_TRUE,    RV_SDPPARSER_STOP_ERROR,     RvSdpWarnInternErr},
};

const RvUint16  gsParseActionsSz = sizeof(gsParseActions)/sizeof(RvSdpParseActions);

/*
 *	Tests whether version field is set in a parsed message
 */
RvBool rvSdpMsgVersionIsSet(RvSdpParserData* pd)
{
    return (pd->iMsg->iVersion.iVersionTxt && !pd->iMsg->iVersion.iSetByMsgConstruct) ? RV_TRUE : RV_FALSE;
}

/*
 *	Tests whether origin field is set in a parsed message.
 */
RvBool rvSdpMsgOriginIsSet(RvSdpParserData* pd)
{
    return RV_SDP_ORIGIN_IS_USED(&pd->iMsg->iOrigin);
}

/*
 *	Tests whether session name field is set in a parsed message.
 */
RvBool rvSdpMsgSessionNameIsSet(RvSdpParserData* pd)
{
    return (pd->iMsg->iSessId.iSessIdTxt) ? RV_TRUE : RV_FALSE;
}

/*
 *	Tests whether information field is set in a parsed message.
 */
RvBool rvSdpMsgInformationIsSet(RvSdpParserData* pd)
{
    return (pd->iMsg->iCommonFields.iInfo.iInfoTxt) ? RV_TRUE : RV_FALSE;
}

/*
 *	Tests whether URI field is set in a parsed message.
 */
RvBool rvSdpMsgURIIsSet(RvSdpParserData* pd)
{
    return (pd->iMsg->iUri.iUriTxt) ? RV_TRUE : RV_FALSE;
}

/*
 *	Tests whether connection field is set in a parsed message.
 */
RvBool rvSdpMsgConnectionIsSet(RvSdpParserData* pd)
{
    return (pd->iMsg->iCommonFields.iConnectionList.iListSize) ? RV_TRUE : RV_FALSE;
}

/*
 *	Tests whether time zone adjustment field is set in a parsed message.
 */
RvBool rvSdpMsgTimeZoneAdjIsSet(RvSdpParserData* pd)
{
    return rvSdpMsgTZAIsUsed(pd->iMsg);
}

/*
 *	Tests whether key field is set in a parsed message.
 */
RvBool rvSdpMsgKeyIsSet(RvSdpParserData* pd)
{
    return RV_SDP_KEY_IS_USED(&pd->iMsg->iCommonFields.iKey);
}

/*
 *	Tests whether media information field is set in a parsed media descriptor.
 */
RvBool rvSdpMediaDescrInformationIsSet(RvSdpParserData* pd)
{
    return (pd->iMediaDescr->iCommonFields.iInfo.iInfoTxt) ? RV_TRUE : RV_FALSE;
}

/*
 *	Tests whether media key field is set in a parsed media descriptor.
 */
RvBool rvSdpMediaDescrKeyIsSet(RvSdpParserData* pd)
{
    return RV_SDP_KEY_IS_USED(&pd->iMediaDescr->iCommonFields.iKey);
}

/*
    array with message (but not media) tags data definitions.
    this array also sets the standard required order of tags in
    SDP message.
    Each set contains following:
        -   index within the array;
        -   the tag letter;
        -   whether this tag is mandataory within the message;
        -   'isSet' callback. This callback is defined only for tags not allowing
            multiple appearance. If the callback is defined and returns RV_TRUE
            that means such tag already appears in the input message and the
            correspondent field must re-created.
            If the value is NULL the multiple appearance of this tag is allowed.
        -   tag parse callback. This callback function is responsible for this
            tag format.
*/
const RvSdpTagDef gsSdpMsgTagsDef[] =
{
/* index    tag  mandatory  testMultipleFunc             valueParseFunc             allowSpaceValues*/
    { 0,    'v', RV_TRUE,   rvSdpMsgVersionIsSet,        rvSdpParseVersion,         RV_FALSE},
    { 1,    'o', RV_TRUE,   rvSdpMsgOriginIsSet,         rvSdpParseOrigin,          RV_FALSE},
    { 2,    's', RV_TRUE,   rvSdpMsgSessionNameIsSet,    rvSdpParseSessionName,     RV_TRUE},
    { 3,    'i', RV_FALSE,  rvSdpMsgInformationIsSet,    rvSdpParseInformation,     RV_FALSE},
    { 4,    'u', RV_FALSE,  rvSdpMsgURIIsSet,            rvSdpParseURI,             RV_FALSE},
    { 5,    'e', RV_FALSE,  NULL,                        rvSdpParseEmail,           RV_FALSE},
    { 6,    'p', RV_FALSE,  NULL,                        rvSdpParsePhone,           RV_FALSE},
    { 7,    'c', RV_FALSE,  rvSdpMsgConnectionIsSet,     rvSdpParseConnection,      RV_FALSE},
    { 8,    'b', RV_FALSE,  NULL,                        rvSdpParseBandwidth,       RV_FALSE},
    { 9,    't', RV_TRUE,   NULL,                        rvSdpParseTime,            RV_FALSE},
    {10,    'r', RV_FALSE,  NULL,                        rvSdpParseTimeRepeat,      RV_FALSE},
    {11,    'z', RV_FALSE,  rvSdpMsgTimeZoneAdjIsSet,    rvSdpParseZoneAdjust,      RV_FALSE},
    {12,    'k', RV_FALSE,  rvSdpMsgKeyIsSet,            rvSdpParseKey,             RV_FALSE},
    {13,    'a', RV_FALSE,  NULL,                        rvSdpParseAttribute,       RV_FALSE},
    {14,    'm', RV_FALSE,  NULL,                        rvSdpParseMediaDescr,      RV_FALSE},
    {-1,    0,   0,         NULL,                        NULL,                      RV_FALSE},  /* the last line must be set */
};


/* this array sets the index number within the gsSdpMsgTagsDef for each alphabet letter
   or the value -1 if such letter is not used as SDP tag in the context of SDP message
   (but not media).
   That is the first entry in the array that corresponds to the first letter in the
   alphabet ('a') has value 13. That means the data needed to treat the correspondent
   tag ('a') line is located in gsSdpMsgTagsDef[13].
*/
const RvInt16 gsSdpTagMsgIndexes[] =
{
    /* a */ 13,
    /* b */  8,
    /* c */  7,
    /* d */ -1,
    /* e */  5,
    /* f */ -1,
    /* g */ -1,
    /* h */ -1,
    /* i */  3,
    /* j */ -1,
    /* k */ 12,
    /* l */ -1,
    /* m */ 14,
    /* n */ -1,
    /* o */  1,
    /* p */  6,
    /* q */ -1,
    /* r */ 10,
    /* s */  2,
    /* t */  9,
    /* u */  4,
    /* v */  0,
    /* w */ -1,
    /* x */ -1,
    /* y */ -1,
    /* z */ 11,
};

/*
array with media tags data definitions
this array also sets the standard required order of tags in
SDP media descriptor
*/
const RvSdpTagDef gsSdpMediaTagsDef[] =
{
/* index    tag  mandatory  testMultipleFunc                    valueParseFunc          allowSpaceValues*/
    { 0,    'i', RV_FALSE,  rvSdpMediaDescrInformationIsSet,    rvSdpParseInformation,  RV_FALSE},
    { 1,    'c', RV_TRUE,   NULL,                               rvSdpParseConnection,   RV_FALSE},
    { 2,    'b', RV_FALSE,  NULL,                               rvSdpParseBandwidth,    RV_FALSE},
    { 3,    'k', RV_FALSE,  rvSdpMediaDescrKeyIsSet,            rvSdpParseKey,          RV_FALSE},
    { 4,    'a', RV_FALSE,  NULL,                               rvSdpParseAttribute,    RV_FALSE},
    {-1,    0,   0,         NULL,                               NULL,                   RV_FALSE},  /* the last line must be set */
};

/* this array sets the index number within the gsSdpMediaTagsDef for each alphabet letter
   or the value -1 if such letter is not used as SDP tag in the context of SDP media
   descriptor.
   That is the first entry in the array that corresponds to the first letter in the
   alphabet ('a') has value 4. That means the data needed to treat the correspondent
   tag ('a') line is located in gsSdpMediaTagsDef[4].
*/
const RvInt16 gsSdpTagMediaIndexes[] =
{
    /* a */  4,
    /* b */  2,
    /* c */  1,
    /* d */ -1,
    /* e */ -1,
    /* f */ -1,
    /* g */ -1,
    /* h */ -1,
    /* i */  0,
    /* j */ -1,
    /* k */  3,
    /* l */ -1,
    /* m */ -1,
    /* n */ -1,
    /* o */ -1,
    /* p */ -1,
    /* q */ -1,
    /* r */ -1,
    /* s */ -1,
    /* t */ -1,
    /* u */ -1,
    /* v */ -1,
    /* w */ -1,
    /* x */ -1,
    /* y */ -1,
    /* z */ -1,
};



void rvSdpResetParseData(RvSdpParserData* pD)
{
    pD->iCurrTag = 0;
    pD->iCurrTagDef = NULL;
    pD->iCurrValue = NULL;
    pD->iCurrValueLen = 0;
    memset(&pD->iWarning,0,sizeof(RvSdpParserWarningData));
}

void rvSdpCopyWarning(RvSdpParserWarningData* dst, RvSdpParserWarningData* src)
{
    dst->iCurrentTag = src->iCurrentTag;
    dst->iLineOfEvent = src->iLineOfEvent;
    dst->iMissedTag = src->iMissedTag;
    dst->iWarningNum = src->iWarningNum;
}

/*
 *	message is empty if its line objects list is empty or consists of one element
 *  and that is version set by msg construct and not through the parsing
 */
RvBool rvSdpMsgIsEmpty(RvSdpMsg *m)
{
    if (m->iLOCnt == 0)
        return RV_TRUE;
    if (m->iLOCnt == 1 && m->iHeadLO->iFieldType == SDP_FIELDTYPE_VERSION)
    {
        RvSdpVersion* v =  (RvSdpVersion*)((char*)m->iHeadLO - m->iHeadLO->iOffset);
        if (v->iSetByMsgConstruct)
            return RV_TRUE;
    }
    return RV_FALSE;
}


/*
 *	Parses most common SDP format: tag=value.
 *  This  function finds the tag letter the pointer to 'value' and its
 *  length. Also sets the 'pD->iCurrPtr' to the beginning of the next line.
 */
void rvSdpParseLine(RvSdpParserData* pD)
{
    RvChar *p, *pE, *pS;
    const RvSdpTagDef* tagDef = NULL;
    RvInt  tagInd;

    p = pD->iCurrPtr;
    pE = pD->iEndOfData;
    pS = p;

#ifdef RV_SDP_SPACES_ALLOWED
    {
        /* skip spaces (if alloed) before the tag */
        while (*p == ' ' || *p == '\t')
            p++;
    }
#endif

    if (p >= pE)
    /* we've got out of the buffer*/
    {
        /* the tag was not parsed yet */
        pD->iLineStatus = RvSdpParseEndOfBuffer;
endOfBuffer:
        /* save the appropriate data */
        pD->iWarning.iLineOfEvent = pD->iLinesCnt;
        pD->iCurrPtr = pD->iEndOfData;
        return;
    }

    /* the tag has to be a letter */
    if (!RV_SDP_IS_ALPHA((int)*p))
    {
        /* the current character is not letter */
        if (*p == '\r' || *p == '\n')
        {
            /* we are  at the end of the line */
            if (rvSdpMsgIsEmpty(pD->iMsg))
                pD->iLineStatus = RvSdpParseEmptyLineAtStart;
            else
                pD->iLineStatus = RvSdpParseEmptyLine;
        }
        else if (*p == 0)
            /* we are  at the end of the buffer */
            pD->iLineStatus = RvSdpParseEndOfBuffer;
        else if (*p == '.')
            pD->iLineStatus = RvSdpParseDot;
        else if (*p == '}')
            pD->iLineStatus = RvSdpParseCloseBrace;
        else
        {
            pD->iLineStatus = RvSdpParseNoTag;
            pD->iCurrValue = pS;
            pD->iCurrTag = 0;
        }

        /* anyway need to find the beginning of the next line*/
        goto findNextLine;
    }

    /* found tag; save it*/
    pD->iCurrTag = (RvChar) tolower(*p);
    pD->iWarning.iCurrentTag = pD->iCurrTag;
    if (pD->iInMediaDescr)
        tagInd = gsSdpTagMediaIndexes[pD->iCurrTag-'a'];
    else
        tagInd = gsSdpTagMsgIndexes[pD->iCurrTag-'a'];
    if (tagInd >= 0)
    {
        if (pD->iInMediaDescr)
            tagDef = &gsSdpMediaTagsDef[gsSdpTagMediaIndexes[pD->iCurrTag-'a']];
        else
            tagDef = &gsSdpMsgTagsDef[gsSdpTagMsgIndexes[pD->iCurrTag-'a']];
    }

    p++;

#ifdef RV_SDP_SPACES_ALLOWED
    {
    /* skip spaces between the tag and '=' */
        while (*p == ' ' || *p == '\t')
            p++;
    }
#endif

    if (p >= pE)
    {
        /* got out of the buffer*/
        pD->iLineStatus = RvSdpParseNoEqualSign;
        pD->iCurrValue = pS;
        pD->iCurrTag = 0;
        goto endOfBuffer;
    }

    if (*p != '=')
    {
        /* there is no equal sign */
        pD->iLineStatus = RvSdpParseNoEqualSign;
        pD->iCurrValue = pS;
        pD->iCurrTag = 0;
        goto findNextLine;
    }
    /* found equal sign*/
    p++;

    /* skip spaces after '=' */
    if (!tagDef || !tagDef->iAllowSpaceValues)
    {
        while (*p == ' ' || *p == '\t')
            p++;
    }

    if (p >= pE)
    {
        /* got out of the buffer*/
        pD->iLineStatus = RvSdpParseNoValue;
        goto endOfBuffer;
    }

    if (*p == '\r' || *p == '\n')
        /* end of lien and there is no value */
        pD->iLineStatus = RvSdpParseNoValue;
    else
    {
        /* value was found */
        pD->iLineStatus = RvSdpLineSyntaxOK;
        pD->iCurrValue = p;
    }

findNextLine:

    pD->iWarning.iLineOfEvent = pD->iLinesCnt;

    /* go to the end of line */
    while (*p != '\r' && *p != '\n' && p <= pE)
        p++;
    /* now 'p' points at the end of line  (\n or \r)*/


    if (pD->iCurrValue)
    {
        /* the case when value exists */
        pD->iCurrValueLen = (RvUint16) (p - pD->iCurrValue);
        if (pD->iCurrValueLen == 0)
            /* no, it does not exist */
            pD->iLineStatus = RvSdpParseNoValue;
        else
        {
        /* remove the trailing spaces */
            if (!tagDef || !tagDef->iAllowSpaceValues)
            {
            char* p1;
            p1 = p-1;
            while (*p1 == ' ' || *p1 == '\t')
            {
                p1--;
                pD->iCurrValueLen --;
            }
            if (pD->iCurrValueLen == 0)
                /* there were nothing except for spaces */
                pD->iLineStatus = RvSdpParseNoValue;
            }
        }
    }

    if (p != pE)
    {
        /* increase the lines counter and move the 'p' to
           the beginning of the next line
           remember p points  to \n or \r
        */

        pD->iLinesCnt++;

        /* treat different cases: the normal case (standard required) is
            when the line ends with \n\r
           but we also treat \r\n or \r only or \n only */

        if (*p == '\n' && *(p+1) == '\r')
            p += 2; /* normal */
        else if (*p == '\r' && *(p+1) == '\n')
            p += 2; /* strange */
        else
            p++;    /* \n or \r only */
    }

    /* set the iCurrPtriCurrPtr to point to the start of the next line */
    pD->iCurrPtr = (pD->iEndOfData<p) ? pD->iEndOfData : p;

    return;
}

/* this function tests different aspects of tags ordering;
   the purpose of this function is to set the appropriate
   pD->iLineStatus that will impact the next parser action */
void rvSdpTestTagOrdering(RvSdpParserData* pD)
{
    RvInt16 currTagInd;

    /* set the data that might be needed for the warning creation*/
    pD->iWarning.iLineOfEvent = (RvUint16)(pD->iLinesCnt-1);
    pD->iWarning.iCurrentTag = pD->iCurrTag;
    pD->iWarning.iWarningNum = RvSdpWarnNoWarning;


    if (pD->iPrevTagDef && pD->iPrevTagDef->iTag == pD->iCurrTag)
    /* test the case the current tag is as the previous */
    {
        pD->iCurrTagDef = pD->iPrevTagDef;

        /* that is the same tag */
        if (pD->iPrevTagDef->iTestMultipleTag)
        {
            /* and the multiple appearance is not allowed */
            pD->iLineStatus = RvSdpParseMultipleNotAllowed;
            return;
        }
        else
        {
            /* the multiple appearance of this tag is allowed */
            pD->iLineStatus = RvSdpLineTagOrderingOK;
            return;
        }
    }


    /* test that this tag is known (that is: defined in the correspondent array )*/
    currTagInd = -1;
    if (pD->iCurrTag > 0)
    {
        /* find the tags index in the correspondent array (for media or message)*/
        if (pD->iInMediaDescr)
            currTagInd = gsSdpTagMediaIndexes[pD->iCurrTag - 'a'];
        else
            currTagInd = gsSdpTagMsgIndexes[pD->iCurrTag - 'a'];

        if (currTagInd < 0)  /* the tag is unknown */
        {
            if (pD->iInMediaDescr && pD->iCurrTag == 'm')
            /* we were in the context of media, and that is beginning of
               the next media, another 'm=' line.
               Tne 'm' tag is defined in the message context */
            {
                pD->iCurrTagDef = &gsSdpMsgTagsDef[gsSdpTagMsgIndexes['m'-'a']];

                if (pD->iExpectedTag)
                    /* need to test whether all mandatory tags of media were processed*/
                    goto testExpected;

                /* set the expected tag as  the first tag of media context */
                pD->iExpectedTag = &gsSdpMsgTagsDef[gsSdpTagMsgIndexes['m'-'a']];
                pD->iInMediaDescr = RV_FALSE;
                pD->iPrevTagDef = NULL;
                return;
            }
            if (pD->iInMediaDescr)
                pD->iLineStatus = RvSdpParseUnknownTagInMedia;
            else
                pD->iLineStatus = RvSdpParseUnknownTagInMsg;
            return;
        }
        else
        {
            if (pD->iInMediaDescr)
                pD->iCurrTagDef = &gsSdpMediaTagsDef[currTagInd];
            else
                pD->iCurrTagDef = &gsSdpMsgTagsDef[currTagInd];
        }
    }

    /* now test that the tag is not the one that had to be somewhere before */
    if (pD->iPrevTagDef && currTagInd >= 0 && currTagInd < pD->iPrevTagDef->iIndex)
    {
    /* have to test the case of next 'time field' */
        if (!(!pD->iInMediaDescr && pD->iCurrTag == 't' && pD->iPrevTagDef->iTag == 'r'))
        {
            /* this tag can't be here */
            /* in this case we do not update the value of iPrevTagDef */
            pD->iLineStatus = RvSdpParseTagOutOfOrder;
            return;
        }
    }

testExpected:

    /* test the case when we know which tag has to come now */
    /*
    if (!pD->iExpectedTag)
            printf("Very strange\n");
    */

    for (;;)
    {
        /* test :
         *  reached the end of the table
         *  or the current tag is as expected   */
        if (pD->iExpectedTag->iIndex < 0 || pD->iExpectedTag->iTag == pD->iCurrTag)
        {

            if (pD->iStopping)
            {
                /*
                 *   we are at the end of the parsing; we were here only
                 *   because there was a need to collect warnings about missing
                 *   mandatory tags; now all such warnings were collected and
                 *   we restore 'pD->iLineStatus' to its original value
                 *   probably (RvSdpParseEmptyLine,RvSdpParseDot or something like that)
                 *   and set the pD->iSavedLineStatus to RvSdpParseUnknownStatus what
                 *   will cause 'return RV_FALSE;' in rvSdpTreatLineStatus.
                 */
                pD->iLineStatus = pD->iSavedLineStatus;
                pD->iSavedLineStatus = RvSdpParseUnknownStatus;
                return;
            }

            /*
             *	either the tag current tag is as expected or there are no
             *  more expected tags
             */

            pD->iLineStatus = RvSdpLineTagOrderingOK;

            if (pD->iCurrTag == 'm')
            {
                /* entering the media definition*/
                pD->iInMediaDescr = RV_TRUE;
                pD->iExpectedTag = &gsSdpMediaTagsDef[0];
                pD->iPrevTagDef = NULL;
            }
            else
            {
                pD->iPrevTagDef = pD->iCurrTagDef;
                pD->iExpectedTag = pD->iPrevTagDef+1;
                if (pD->iExpectedTag->iIndex < 0)
                    /* no more expected tags */
                    pD->iExpectedTag = NULL;
            }
            return;
        }

        /* the current tag is not as expected and expected tag is mandatory */
        /* thus need to create the warning about missing tag, the missing tag is
           the one that was expected */
        if (pD->iExpectedTag->iMandatory &&
             /* test also the case of missing connection field in the media descriptor
                when it was set in the whole message context */
            !(pD->iInMediaDescr && pD->iExpectedTag->iTag == 'c' && rvSdpMsgGetConnection(pD->iMsg)))
        {

            pD->iWarning.iMissedTag = pD->iExpectedTag->iTag;
            pD->iLineStatus = RvSdpParseMissingTag;
            /* move the expected tag to the next position*/
            pD->iExpectedTag++;
            return;
        }
        pD->iExpectedTag++;
    }
}

/* creates pasing warning based on the data in 'act' */
RvBool rvSdpCreateWarning(RvSdpParserData* pD, const RvSdpParseActions* act)
{
    char txt[256];
    RvSdpParserWarningData *wd = NULL;
#if RV_LOGMASK != RV_LOGLEVEL_NONE
    RV_SDP_USE_GLOBALS;
#endif

    RV_UNUSED_ARG(wd);

    if (act && act->iWarningNum != RvSdpWarnPrivateWarning)
        pD->iWarning.iWarningNum = act->iWarningNum;
#define SPE "SDP Parse Error: "
#define SPE_LEN 17

    /* prepare message for logging*/
    strcpy(txt,SPE);
    rvSdpGetWarningText(&pD->iWarning,txt+SPE_LEN,sizeof(txt)-SPE_LEN);
    RvLogError(pSdpLogSource, (pSdpLogSource, txt));

#ifdef RV_SDP_PARSE_WARNINGS
    /* get the warning struct from the pool, fill it and append to the list*/
    wd = rvSdpPoolTake(&pD->iWarningsPool);
    if (!wd)
        return RV_FALSE;
    /* fill*/
    rvSdpCopyWarning(wd,&pD->iWarning);
    /* append */
    rvSdpListTailAdd(&pD->iWarningsList,wd);
#endif /*RV_SDP_PARSE_WARNINGS*/


    return RV_TRUE;
}

#ifdef RV_SDP_PARSE_WARNINGS
/***************************************************************************
 * rvSdpParserGetFirstWarning
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first warning object in the warnings list of parser data.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpParserWarningData  object or the NULL pointer if
 *      there are no warnings in the list.
 * ------------------------------------------------------------------------
 * Arguments:
 *      pD - a pointer to the RvSdpParserData object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpParserGetNextWarning calls.
 ***************************************************************************/
RvSdpParserWarningData* rvSdpParserGetFirstWarning(RvSdpParserData* pD, RvSdpListIter* iter)
{
    return rvSdpListGetFirst(&pD->iWarningsList,iter);
}

/***************************************************************************
 * rvSdpParserGetNextWarning
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next warning object in the warnings list of parser data.
 *
 * Return Value:
 *      Pointer to the RvSdpParserWarningData  object or the NULL pointer if
 *      there are no more warnings in the list.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull
 *             rvSdpParserGetFirst/NextWarning call.
 ***************************************************************************/
RvSdpParserWarningData* rvSdpParserGetNextWarning(RvSdpListIter* iter)
{
    return rvSdpListGetNext(iter);
}

#endif /*RV_SDP_PARSE_WARNINGS*/

/***************************************************************************
 * rvSdpGetWarningText
 * ------------------------------------------------------------------------
 * General:
 *      Fetches the text from the parse waring.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      wd - a pointer to the valid RvSdpParserWarningData object.
 *      txt -  buffer to be used for the parse waring text.
 *      len - the  length of 'txt' buffer.
 ***************************************************************************/
void rvSdpGetWarningText(RvSdpParserWarningData* wd, char *txt, int len)
{
    typedef struct
    {
        RvSdpParseWarning iNum;
        const char* iTxt;
    } _rvSdpWarnText;
    static const _rvSdpWarnText sWT[] =
    {
        {RvSdpWarnNoWarning,"No warning"},
        {RvSdpWarnInternErr,"Internal error"},
        {RvSdpWarnNoTag,"There is no tag"},
        {RvSdpWarnNoEq,"There is no equal sign"},
        {RvSdpWarnNoVal,"There is no value, ignoring the line"},
        {RvSdpWarnMultNotAllowed,"Multiple appearance of this tag is not allowed, replacing the previous"},
        {RvSdpWarnMissingTag,"Mandatory tag '%MISSING_TAG' is missing"},
        {RvSdpWarnTagOutOfOrder,"This tag is out of order"},
        {RvSdpWarnNotSdpLine,"Not standard SDP line"},
        {RvSdpWarnUnknownTagInMsg,"The tag is unknown"},
        {RvSdpWarnUnknownTagInMedia,"The tag is unknown in media descriptor context"},
        {RvSdpWarnVersionFailed,"Illegal version value"},
        {RvSdpWarnOriginUserNameFailed,"Illegal origin user name value"},
        {RvSdpWarnOriginSessIdTooLong,"Origin session id value is too long"},
        {RvSdpWarnOriginSessIdFailed,"Illegal origin session id value"},
        {RvSdpWarnOriginSessVersionFailed,"Illegal origin session version value"},
        {RvSdpWarnOriginSessVersionTooLong,"Origin session version value is too long"},
        {RvSdpWarnSessionNameFailed,"Illegal session name value"},
        {RvSdpWarnSessionInfoFailed,"Illegal session information value"},
        {RvSdpWarnConnectionNumOfAddrTooLong,"The value of addresses number is too big"},
        {RvSdpWarnConnectionNumOfAddrIllegal,"Illegal value of addresses number"},
        {RvSdpWarnConnectionTTLTooLong,"The value of TTL of connection address is too big"},
        {RvSdpWarnConnectionTTLIllegal,"Illegal value of TTL of connection address"},
        {RvSdpWarnEmailFailed,"Illegal format of email tag"},
        {RvSdpWarnEmailValueIllegal,"Illegal email address value"},
        {RvSdpWarnPhoneFailed,"Illegal format of phone tag"},
        {RvSdpWarnPhoneValueIllegal,"Illegal phone number value"},
        {RvSdpWarnBandwidthTypeIllegal,"Illegal bandwidth type"},
        {RvSdpWarnBandwidthValueIllegal,"Illegal bandwidth value"},
        {RvSdpWarnBandwidthValueTooLong,"Bandwidth value is too big"},
        {RvSdpWarnSessStartTimeTooLong,"Session start time value is too big"},
        {RvSdpWarnSessStartTimeIllegal,"Illegal value of session start time"},
        {RvSdpWarnSessStopTimeTooLong,"Session stop time value is too big"},
        {RvSdpWarnSessStopTimeIllegal,"Illegal value of session stop time"},
        {RvSdpWarnSessStartTimeTooSmall,"Session start time is too small"},
        {RvSdpWarnSessStopTimeTooSmall,"Session stop time is too small"},
        {RvSdpWarnSessStartLessThanStop,"Session start time is later than stop time"},
        {RvSdpWarnRepeatIntervalValueIllegal,"Illegal value of session repeat time interval"},
        {RvSdpWarnRepeatIntervalValueTooLong,"The value of session repeat time interval is too big"},
        {RvSdpWarnRepeatDurationValueIllegal,"Illegal value of session repeat time duration"},
        {RvSdpWarnRepeatDurationValueTooLong,"The value of session repeat time duration"},
        {RvSdpWarnRepeatOffsetValueIllegal,"Illegal value of session repeat time offset"},
        {RvSdpWarnRepeatOffsetValueTooLong,"The value of session repeat time offset is too big"},
        {RvSdpWarnRepeatNoOffsets,"At least one offset has to be set for the session repeat times"},
        {RvSdpWarnRepeatNoSessTime,"Ignoring repeat times because no valid session time was given"},
        {RvSdpWarnTimeZoneIllegalTime,"Illegal value of time zone adjustment time"},
        {RvSdpWarnTimeZoneTimeTooLong,"The value of time zone adjustment time is too big"},
        {RvSdpWarnTimeZoneIllegalOffset,"Illegal value of time zone adjustment offset"},
        {RvSdpWarnTimeZoneOffsetTooLong,"The value of time zone adjustment offset is too big"},
        {RvSdpWarnTimeZoneNoData,"At least one time zone adjustment has to be set"},
        {RvSdpWarnKeyMethodIllegal,"Illegal value of encryption method"},
        {RvSdpWarnKeyValueNotSet,"Encryption data not set"},
        {RvSdpWarnAttrNameIllegal,"Illegal attribute name"},
        {RvSdpWarnFailedToParseNetType,"Illegal value of network type"},
        {RvSdpWarnFailedToParseAddrType,"Illegal value of address type"},
        {RvSdpWarnFailedToParseAddress,"Illegal address value"},
        {RvSdpWarnIllegalMedia,"Illegal media value"},
        {RvSdpWarnIllegalMediaPort,"Illegal media port number"},
        {RvSdpWarnMediaPortTooLong,"The number of media port is too big"},
        {RvSdpWarnIllegalMediaSubPort,"Illegal number of ports value"},
        {RvSdpWarnMediaSubPortTooLong,"Media ports number is too big"},
        {RvSdpWarnIllegalMediaProtocol,"Illegal media protocol value"},
        {RvSdpWarnNoFormatsForMedia,"At least one format has to be set in the media descriptor"},
        {RvSdpWarnIllegalMediaFormat,"Illegal media format value"},
        {RvSdpWarnRtpMapNoPayload,"Illegal rtp map payload value"},
        {RvSdpWarnRtpMapPayloadTooLong,"Rtp map payload value is too big"},
        {RvSdpWarnRtpMapNoEncName,"Illegal rtp map encryption name value"},
        {RvSdpWarnRtpMapBadEncRate,"Illegal rtp map clock-rate value"},
        {RvSdpWarnRtpMapEncRateTooLong,"Rtp map clock-rate value is too big"},
        {RvSdpWarnConnModeSetAlready,"Connection mode was already set, ignoring the previous setting"},
        {RvSdpWarnDomainAddressTooLong,"Domain name address is too long"},
        {RvSdpWarnIp4AddressTooLong,"IP4 address is too long"},
        {RvSdpWarnIp4AddressIllegal,"IP4 address is invalid"},
        {RvSdpWarnDomainNameIllegal,"Domain address name contains illegal characters"},
        {RvSdpWarningAddressTooLong,"The address length exceeds 256 characters"},
        {RvSdpWarnIllegalURI,"The URI is illegal"},
#ifdef RV_SDP_KEY_MGMT_ATTR
        {RvSdpWarnKeyMgmtIllegalPrtcl,"The key-mgmt protocol ID is illegal"},
        {RvSdpWarnKeyMgmtIllegalData,"The key-mgmt key data contains non base64 symbols"},
#endif /*RV_SDP_KEY_MGMT_ATTR*/
#ifdef RV_SDP_CRYPTO_ATTR
        {RvSdpWarnCryptoIllegalTag,"Illegal value of crypto attribute tag"},
        {RvSdpWarnCryptoIllegalSuite,"Illegal value of crypto attribute suite"},
        {RvSdpWarnCryptoIllegalKeyMethod,"Illegal value of crypto attribute key-method"},
        {RvSdpWarnCryptoIllegalKeyInfo,"Illegal value of crypto attribute key-info"},
        {RvSdpWarnCryptoNoKeyParams,"At least one key parameter has to be set for the crypto argument"},
        {RvSdpWarnCryptoIllegalKeyParamsTooMany,"Too many key parameters for the crypo attribute"},
        {RvSdpWarnCryptoIllegalSessParamsTooMany,"Too many session parameters for the crypo attribute"},
#endif /*RV_SDP_CRYPTO_ATTR*/
        {RvSdpWarnPrivateWarning,NULL},
    };

    char myTxt[256]; /* should be enough */
    char *p, *p1, *p2;
    int up;
    const _rvSdpWarnText *wt;

    if (!txt || len == 0)
        return;
    *txt = 0;

    sprintf(myTxt,"Line %d: Tag %c: ",wd->iLineOfEvent,(wd->iCurrentTag) ? (wd->iCurrentTag): '0');
    p = myTxt + strlen(myTxt);

    /* find the warning txt*/
    wt = sWT;
    while (wt->iTxt && wt->iNum != wd->iWarningNum)
        wt++;

    if (!wt->iTxt)
        return;

    p1 = (char*) wt->iTxt;

    for (;;)
    {
        p2 = strchr(p1,'%');
        if (!p2)
        {
            strcpy(p,p1);
            break;
        }

        strncpy(p,p1,p2-p1);
        p += p2-p1;

        p2 ++;
        if (!strncmp(p2,"MISSING_TAG",strlen("MISSING_TAG")))
        {
            sprintf(p,"%c",wd->iMissedTag);
            p1 = p2 + strlen("MISSING_TAG");
        }
        else
            return;
        p += strlen(p);
    }

    up = strlen(myTxt);
    if (up > len-1)
        up = len-1;

    strncpy(txt,myTxt,up);
    txt[up] = 0;
}

/*
 *	Implements states machine defined in 'gsParseActions'.
 */
RvBool rvSdpTreatLineStatus(RvSdpParserData* pD)
{
    const RvSdpParseActions* act;

    for (;;)
    {
        if (pD->iLineStatus >= gsParseActionsSz)
            act = &gsParseActions[RvSdpParseUnknownStatus];
        else
            act = &gsParseActions[pD->iLineStatus];

        /* if the current parser action requires warning creation - do it */
        if (act->iCreateWarning)
        {
            if (!rvSdpCreateWarning(pD,act))
            {
                pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
                return RV_FALSE;
            }
        }

        /* now perform the next step as required by parser action */
        switch (act->iNextStep)
        {
        case RvSdpActTestMultiple:
            if (pD->iCurrTagDef->iTestMultipleTag && (pD->iCurrTagDef->iTestMultipleTag)(pD) == RV_TRUE)
                pD->iLineStatus = RvSdpParseMultipleNotAllowed;
            else
                pD->iLineStatus = RvSdpLineTagMultipleOK;
            break;
        case RvSdpActMissingTag:
        case RvSdpActTestTagOrdering:
            /* could come here if the line was syntacticallly correct
               that is : there are tag, equal sign and non-empty value
               OR
               some mandatory tag before the current is missing */
            rvSdpTestTagOrdering(pD);
            break;
        case RvSdpActParseTagValue:
        case RvSdpActCreateBadSyntax:
        case RvSdpActReplaceExisting:
            {
                RvBool ret;
                char c;
                c = pD->iCurrValue[pD->iCurrValueLen];
                pD->iCurrValue[pD->iCurrValueLen] = 0;
                ret = (pD->iCurrTagDef->iValueParseFunc)(pD,
                                (act->iNextStep==RvSdpActCreateBadSyntax),
                                (act->iNextStep==RvSdpActReplaceExisting));
                pD->iCurrValue[pD->iCurrValueLen] = c;
                if (ret == RV_TRUE)
                    return RV_TRUE;
                if (pD->iCriticalError)
                    return RV_FALSE;
                break;
            }
        case RvSdpActCreateAsOther:
#ifdef RV_SDP_CHECK_BAD_SYNTAX
            {
                char c;
                RvBool ret;
                c = pD->iCurrValue[pD->iCurrValueLen];
                pD->iCurrValue[pD->iCurrValueLen] = 0;
                ret =  ((pD->iMediaDescr&&rvSdpMediaDescrAddOther(pD->iMediaDescr,pD->iCurrTag,pD->iCurrValue)==NULL) ||
                        (!pD->iMediaDescr&&rvSdpMsgAddOther(pD->iMsg,pD->iCurrTag,pD->iCurrValue)==NULL));
                pD->iCurrValue[pD->iCurrValueLen] = c;
                if (ret)
                {
                    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
                    return RV_FALSE;
                }
            }
            return RV_TRUE;
#else  /*RV_SDP_CHECK_BAD_SYNTAX*/
            pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
            return RV_FALSE;
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/
        case RvSdpActStopParsing:
            pD->iStopParseSts = act->iStopParseStatus;

            if (pD->iStopParseSts != RV_SDPPARSER_STOP_ALLOCFAIL &&
                pD->iStopParseSts != RV_SDPPARSER_STOP_ERROR &&
                pD->iExpectedTag &&
                pD->iSavedLineStatus != RvSdpParseUnknownStatus)
            {
                pD->iStopping = RV_TRUE;
                pD->iSavedLineStatus = pD->iLineStatus;
                pD->iLineStatus = RvSdpLineSyntaxOK;
                break;
            }

            return RV_FALSE;
        case RvSdpActIgnoreLine:
            return RV_TRUE;
        }
    }
}

#ifdef RV_SDP_PARSE_WARNINGS

RvSdpParserWarningData* rvSdpWarningCreateByPool(void *p)
{
    RvSdpParserWarningData* wd;
    RvAlloc *a;

    a = ((RvSdpParserData*)p)->iAllocator;

    wd = rvSdpAllocAllocate(a,sizeof(RvSdpParserWarningData));
    if (!wd)
        return NULL;

    memset(wd,0,sizeof(RvSdpParserWarningData));
    wd->iParserData = (RvSdpParserData*)p;
    return wd;
}

void rvSdpWarningDestroyByPool(RvSdpParserWarningData* wd)
{
    rvSdpAllocDeallocate(wd->iParserData->iAllocator,
        sizeof(RvSdpParserWarningData),wd);
}

#endif /*RV_SDP_PARSE_WARNINGS*/

/***************************************************************************
 * rvSdpMsgConstructParserData
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpParserData object. This parser data is used
 *      in further call to rvSdpMsgConstructParse2.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      pD - a pointer to the RvSdpParserData object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
***************************************************************************/
RvSdpParserData*
rvSdpMsgConstructParserData(RvSdpParserData *pD, RvAlloc *a)
{
    memset(pD,0,sizeof(RvSdpParserData));
    if (!a)
        a = rvSdpGetDefaultAllocator();
    pD->iAllocator = a;
#ifdef RV_SDP_PARSE_WARNINGS
    if (rvSdpPoolInitialize(&pD->iWarningsPool,RV_OFFSETOF(RvSdpParserWarningData,iNextWarning),10,5,
                            (rvCreatePoolObjFunc)rvSdpWarningCreateByPool,(void*)pD,
                            (rvListNodeDestructFunc)rvSdpWarningDestroyByPool) != RV_SDPSTATUS_OK)
        return NULL;
    rvSdpListInitialize(&pD->iWarningsList,RV_OFFSETOF(RvSdpParserWarningData,iNextWarning),
            (rvListNodeDestructFunc)rvSdpWarningDestroyByPool);
#endif /*RV_SDP_PARSE_WARNINGS*/
    return pD;
}

/***************************************************************************
 * rvSdpMsgDestroyParserData
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the RvSdpParserData object and frees up all internal memory.
 *
 * Return Value:
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      pD - pointer to a RvSdpParserData object to be destructed.
 ***************************************************************************/
void
rvSdpMsgDestroyParserData(RvSdpParserData *pD)
{
#ifdef RV_SDP_PARSE_WARNINGS
    rvSdpPoolDestroy(&pD->iWarningsPool);
    rvSdpListClear(&pD->iWarningsList);
#else /*RV_SDP_PARSE_WARNINGS*/
    RV_UNUSED_ARG(pD);
#endif /*RV_SDP_PARSE_WARNINGS*/
}

/***************************************************************************
 * rvSdpMsgConstructParse2
 * ------------------------------------------------------------------------
 * General:
 *      Parses an SDP text message and constructs an RvSdpMsg from the SDP text
 *      message. Collects parse warnings during the parse process.
 *
 * Return Value:
 *      A pointer to the new SDP message, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      pD - pointer to constructed RvSdpParserData object to be used during
 *           the parsing.
 *      msg - a pointer to the RvSdpMsg to be constructed. Must point to valid memory.
 *      txt - contains a pointer to the beginning of the SDP text message.
 *      len - the length of the parsed SDP message.
 *      stat - the status of parsing termination. Will be RV_SDPPARSER_STOP_ERROR
 *             if there was a parsing error, otherwise another status specifying
 *             a legal character that caused the parser to stop. For more
 *             information on RvSdpParseStatus, see the Enumerations section.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpMsg*
rvSdpMsgConstructParse2(RvSdpParserData* pD, RvSdpMsg* msg, char* txt, int* len,
                        RvSdpParseStatus* stat, RvAlloc* a)
{
    int rLen;
#if RV_LOGMASK != RV_LOGLEVEL_NONE
    RV_SDP_USE_GLOBALS;
    RvLogInfo(pSdpLogSource, (pSdpLogSource, "Starting SDP parsing"));
#endif
    /* construct the message */
    msg = rvSdpMsgConstruct2(msg,a);
    if (!msg)
    {
        if (stat)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        RvLogError(pSdpLogSource, (pSdpLogSource, "Failed to construct SDP message"));
        return NULL;
    }

    pD->iCurrPtr = txt;
    rLen = strlen(txt);
    pD->iEndOfData = txt + *len;
    /* treat the case when there is a NULL byte inside the buffer */
    if (rLen < *len)
        pD->iEndOfData = txt + rLen;
    pD->iLinesCnt = 1;

    /* the expected  tag is the first tag in the message (not media descriptor) context*/
    pD->iExpectedTag = &gsSdpMsgTagsDef[0];
    pD->iMsg = msg;

    while (pD->iCurrPtr < pD->iEndOfData)
    {
        rvSdpResetParseData(pD);
        rvSdpParseLine(pD);

        if (!rvSdpTreatLineStatus(pD))
            break;
    }

    *len = pD->iCurrPtr - txt;

    if (stat)
        *stat = pD->iStopParseSts;
    if (pD->iStopParseSts == RV_SDPPARSER_STOP_ALLOCFAIL ||
        pD->iStopParseSts == RV_SDPPARSER_STOP_ERROR)
    {
        RvLogError(pSdpLogSource, (pSdpLogSource, "SDP parsing failed with %d", pD->iStopParseSts));
        if (msg)
            rvSdpMsgDestruct(msg);
        return NULL;
    }

    RvLogInfo(pSdpLogSource, (pSdpLogSource, "SDP parsing completed, stat %d", pD->iStopParseSts));

    return msg;
}

/***************************************************************************
 * rvSdpMsgConstructParseA
 * ------------------------------------------------------------------------
 * General:
 *      Parses an SDP text message and constructs an RvSdpMsg from the SDP text
 *      message.
 *
 * Return Value:
 *      A pointer to the new SDP message, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg to be constructed. Must point to valid memory.
 *      txt - contains a pointer to the beginning of the SDP text message.
 *      len - the length of the parsed SDP message.
 *      stat - the status of parsing termination. Will be RV_SDPPARSER_STOP_ERROR
 *             if there was a parsing error, otherwise another status specifying
 *             a legal character that caused the parser to stop. For more
 *             information on RvSdpParseStatus, see the Enumerations section.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpMsg*
rvSdpMsgConstructParseA(RvSdpMsg* msg, char* txt, int* len,
                        RvSdpParseStatus* stat, RvAlloc* a)
{
    RvSdpParserData pD;

    if (!a)
        a = rvSdpGetDefaultAllocator();

    if (!rvSdpMsgConstructParserData(&pD,a))
        return NULL;
    msg = rvSdpMsgConstructParse2(&pD,msg,txt,len,stat,a);
    rvSdpMsgDestroyParserData(&pD);
    return msg;
}

/***************************************************************************
 * rvSdpMsgConstructParse
 * ------------------------------------------------------------------------
 * General:
 *      Parses an SDP text message and constructs an RvSdpMsg from the SDP text
 *      message.
 *
 * Return Value:
 *      A pointer to the new SDP message, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg to be constructed. Must point to valid memory.
 *      txt - contains a pointer to the beginning of the SDP text message.
 *      len - the length of the parsed SDP message.
 *      stat - the status of parsing termination. Will be RV_SDPPARSER_STOP_ERROR
 *             if there was a parsing error, otherwise another status specifying
 *             a legal character that caused the parser to stop. For more
 *             information on RvSdpParseStatus, see the Enumerations section.
 ***************************************************************************/
RvSdpMsg*
rvSdpMsgConstructParse(RvSdpMsg* msg, char* txt, int* len, RvSdpParseStatus* stat)
{
    return rvSdpMsgConstructParseA(msg,txt,len,stat,NULL);
}


/*
 *	Parses the version line ('v='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseVersion(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting)     /* set to RV_TRUE for the tags which appears
                                       more than once in the input and that
                                       contradicts the standard */
{
    char *p;
    RV_UNUSED_ARG(createAsBadSyntax);

    if (replaceExisting)
        rvSdpMsgDestroyVersion(pD->iMsg);

    p = pD->iCurrValue;

    if (!rvSdpParseInt(0,0,NULL,RV_TRUE,RV_FALSE,&p,NULL) || *p)
    {
        RV_SDP_WARNING_SET_AND_CREATE_WARNING(RvSdpWarnVersionFailed,pD);
    }

    if (rvSdpMsgSetVersion2(pD->iMsg,pD->iCurrValue) != RV_SDPSTATUS_OK)
        goto allocFail;

    return RV_TRUE;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    return RV_FALSE;
}

/*
 *	Tests the correctness of IP V4 or domain name.
 */
RvBool testIP4Addr(RvSdpNetType netType,  RvSdpAddrType addrType, char* addr, RvSdpParseWarning* w)
{
    int cnt,num;
    char *p,*p1,*p2;
    RvBool retV = RV_FALSE;
    RvBool isDomain = RV_TRUE;
    REPLACING_START

    RV_UNUSED_ARG(netType);
    RV_UNUSED_ARG(addrType);

    if ((*addr == '-' || *addr == '$') && *(addr+1) == 0)
        return RV_TRUE;

    retV = RV_FALSE;

    p = strchr(addr,'/');
    if (p)
    {
        REPLACING(p)
        p++;
        while (*p)
        {
            if (!RV_SDP_IS_D_SL(*p))
            {
                *w = RvSdpWarnFailedToParseAddress;
                goto finish;
            }
            p++;
        }
    }

    p = addr;
    cnt = 0;
    for (;;)
    {
        if ((isDomain && cnt == 10) || (!isDomain && cnt == 4))
        {
            if (isDomain)
                *w = RvSdpWarnDomainAddressTooLong;
            else
                *w = RvSdpWarnIp4AddressTooLong;
            goto finish;
        }

        p2 = strchr(p,'.');
        if (p2)
        {
            REPLACING(p2)
        }

        p1 = p;

        if (rvSdpParseText(&p1,M_D))
        {
            p1 = p;
            if (cnt == 0)
                isDomain = RV_FALSE;
            if (!isDomain)
            {
                if (!rvSdpParseInt(0,255,(RvUint*)&num,RV_TRUE,RV_FALSE,&p1,NULL))
                {
                    *w = RvSdpWarnIp4AddressIllegal;
                    goto finish;
                }
            }
        }
        else if (cnt && !isDomain)
        {
            *w = RvSdpWarnIp4AddressIllegal;
            goto finish;
        }
        else if (!rvSdpParseText(&p1,M_ADDR))
        {
            *w = RvSdpWarnDomainNameIllegal;
            goto finish;
        }
        else if (cnt == 0)
            isDomain = RV_TRUE;

        if (!p2)
            break;

        cnt ++;
        p = p2+1;
    }

    retV = RV_TRUE;

finish:
    REPLACING_UNDO;
    return retV;
}

/*
 *	Tests the correctness of IPv6.
 */
RvBool testIP6Addr(RvSdpNetType netType,  RvSdpAddrType addrType, char* addr, RvSdpParseWarning* w)
{
    RV_UNUSED_ARG(netType);
    RV_UNUSED_ARG(addrType);
    RV_UNUSED_ARG(w);
    RV_UNUSED_ARG(addr);

    return RV_TRUE;
}

/*
 *	Parses network_type - address_type - address construction.
 */
RvBool rvSdpParseNetType_AddrType_Addr(char** ptr, RvSdpNetType* netType, char** netTypeTxt,
                                       RvSdpAddrType* addrType, char** addrTypeTxt,
                                       char** addr, RvSdpParseWarning* w, REPLACING_DECL)
{

    char *pp;

    *netTypeTxt = *ptr;

    if (!((**ptr == '-' || **ptr == '$') && *(*ptr+1) == ' '))
    /*support the '-' or '$' as possible values*/
    {
        if (!rvSdpParseText(ptr,M_ADH))
        {
            *w = RvSdpWarnFailedToParseNetType;
            return RV_FALSE;
        }
    }
    pp = *ptr;
    if (!rvSdpParseSpace(ptr))
    {
        *w = RvSdpWarnFailedToParseNetType;
        return RV_FALSE;
    }
    REPLACING2(pp);


    *addrTypeTxt = *ptr;
    if (!((**ptr == '-' || **ptr == '$') && *(*ptr+1) == ' '))
    /*support the '-' or '$' as possible values*/
    {
        if (!rvSdpParseText(ptr,M_ADH))
        {
            *w = RvSdpWarnFailedToParseAddrType;
            return RV_FALSE;
        }
    }
    pp = *ptr;
    if (!rvSdpParseSpace(ptr))
    {
        *w = RvSdpWarnFailedToParseAddrType;
        return RV_FALSE;
    }
    REPLACING2(pp);

    *addr = *ptr;
    if (!rvSdpParseNonSpace(ptr,0))
    {
        *w = RvSdpWarnFailedToParseAddress;
        return RV_FALSE;
    }

    if (**ptr)
        *w = RvSdpWarnFailedToParseAddress;

    *netType = rvSdpNetTypeTxt2Val(*netTypeTxt);
    *addrType = rvSdpAddrTypeTxt2Val(*addrTypeTxt);

    if (strlen(*addr) > 256)
    {
        *w = RvSdpWarningAddressTooLong;
        return RV_FALSE;
    }

    {
        typedef RvBool (*rvSdpParseAddressFunc)(RvSdpNetType netType,  RvSdpAddrType addrType, char* addr, RvSdpParseWarning* w);
        static const rvSdpParseAddressFunc prsAddr[RV_SDPNETTYPE_UNKNOWN+1][RV_SDPADDRTYPE_UNKNOWN+1] =
        {
        /*  NOTSET  IP4     IP6     ENDPOINT    NSAP    E164    GWID    ALIAS   RFC2543     ANY     IGNORE  UNKNOWN*/
/*NotSet*/  {NULL,  NULL,   NULL,   NULL,       NULL,   NULL,   NULL,   NULL,   NULL,       NULL,   NULL,   NULL,},   /*NotSet*/
/*IN*/      {NULL,  testIP4Addr,
                            testIP6Addr,
                                    NULL,       NULL,   NULL,   NULL,   NULL,   NULL,       NULL,   NULL,   NULL,},   /*IN*/
/*ATM*/     {NULL,  NULL,   NULL,   NULL,       NULL,   NULL,   NULL,   NULL,   NULL,       NULL,   NULL,   NULL,},   /*ATM*/
/*local*/   {NULL,  NULL,   NULL,   NULL,       NULL,   NULL,   NULL,   NULL,   NULL,       NULL,   NULL,   NULL,},   /*local*/
/*other*/   {NULL,  NULL,   NULL,   NULL,       NULL,   NULL,   NULL,   NULL,   NULL,       NULL,   NULL,   NULL,},   /*other*/
/*tn*/      {NULL,  NULL,   NULL,   NULL,       NULL,   NULL,   NULL,   NULL,   NULL,       NULL,   NULL,   NULL,},   /*tn*/
/*any*/     {NULL,  NULL,   NULL,   NULL,       NULL,   NULL,   NULL,   NULL,   NULL,       NULL,   NULL,   NULL,},   /*any*/
/*ignore*/  {NULL,  NULL,   NULL,   NULL,       NULL,   NULL,   NULL,   NULL,   NULL,       NULL,   NULL,   NULL,},   /*ignore*/
/*unknown*/ {NULL,  NULL,   NULL,   NULL,       NULL,   NULL,   NULL,   NULL,   NULL,       NULL,   NULL,   NULL,},   /*unknown*/
        };

        if (prsAddr[*netType][*addrType])
        {
            if (!(*prsAddr[*netType][*addrType])(*netType,*addrType,*addr,w))
                return RV_FALSE;
        }
    }

    return RV_TRUE;
}

/*
 *	Parses the origin line ('o='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseOrigin(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting)     /* set to RV_TRUE for the tags which appears
                                       more than once in the input and that
                                       contradicts the standard */
{
    char *p, *pp, *user, *sessId, *sessVer, *netTypeTxt, *addrTypeTxt, *addr;
    RvSdpNetType netType;
    RvSdpAddrType addrType;
    RvBool retV = RV_TRUE;
    RvSdpParseWarning naw;
    RvSdpIntegerParseWarning intW;

    REPLACING_START

    if (replaceExisting)
        rvSdpOriginDestruct(&pD->iMsg->iOrigin);

    if (createAsBadSyntax)
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    {
        if (rvSdpOriginConstruct2(pD->iMsg,&pD->iMsg->iOrigin,NULL,NULL,NULL,
                                        RV_SDPNETTYPE_NOTSET,RV_SDPADDRTYPE_NOTSET,
                                        NULL,pD->iCurrValue,NULL,RV_FALSE) == NULL)
            goto allocFail;
        return RV_TRUE;
    }
#else
    {
        pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
        pD->iCriticalError = RV_TRUE;
        return RV_FALSE;
    }
#endif

    p = pD->iCurrValue;

    // username
    user = p;
    if (!rvSdpParseText(&p,M_SF))
    {
       pD->iWarning.iWarningNum = RvSdpWarnOriginUserNameFailed;
       goto parseFailed;
    }
    pp = p;
    if (!rvSdpParseSpace(&p))
    {
        pD->iWarning.iWarningNum = RvSdpWarnOriginUserNameFailed;
        goto parseFailed;
    }
    REPLACING(pp);

    // sess_id
    sessId = p;
    if (!rvSdpParseInt(0,0,NULL,RV_TRUE,RV_FALSE,&p,&intW))
    {
        if (intW == RvSdpWarnNumberOverflow)
        {
            pD->iWarning.iWarningNum = RvSdpWarnOriginSessIdTooLong;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnOriginSessIdFailed;
        }
        goto parseFailed;
    }
    pp = p;
    if (!rvSdpParseSpace(&p))
    {
        pD->iWarning.iWarningNum = RvSdpWarnOriginSessIdFailed;
        goto parseFailed;
    }
    REPLACING(pp);

    // sess_version
    sessVer = p;
    if (!rvSdpParseInt(0,0,NULL,RV_TRUE,RV_FALSE,&p,&intW))
    {
        if (intW == RvSdpWarnNumberOverflow)
        {
            pD->iWarning.iWarningNum = RvSdpWarnOriginSessVersionTooLong;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnOriginSessVersionFailed;
        }
        goto parseFailed;
    }
    pp = p;
    if (!rvSdpParseSpace(&p))
    {
        pD->iWarning.iWarningNum = RvSdpWarnOriginSessVersionFailed;
        goto parseFailed;
    }
    REPLACING(pp);

    if (!rvSdpParseNetType_AddrType_Addr(&p,&netType,&netTypeTxt,&addrType,&addrTypeTxt,
										 &addr,&naw,REPLACING_ARGS))
    {
        pD->iWarning.iWarningNum = naw;
        goto parseFailed;
    }


    if (rvSdpOriginConstruct2(pD->iMsg,&pD->iMsg->iOrigin,user,sessId,sessVer,netType,addrType,addr,NULL,NULL,RV_FALSE) == NULL)
        goto allocFail;

    if (rvSdpOriginSetNetTypeStr(&pD->iMsg->iOrigin,netTypeTxt) != RV_SDPSTATUS_OK ||
        rvSdpOriginSetAddressTypeStr(&pD->iMsg->iOrigin,addrTypeTxt) != RV_SDPSTATUS_OK)
    {
        rvSdpOriginDestruct(&pD->iMsg->iOrigin);
        goto allocFail;
    }

    goto realReturn;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    retV = RV_FALSE;
    goto realReturn;

parseFailed:
    retV = RV_FALSE;
    pD->iLineStatus = RvSdpValueParseFailed;
    goto realReturn;

realReturn:
    REPLACING_UNDO;
    return retV;
}

/*
 *	Parses the session name line ('s='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseSessionName(RvSdpParserData* pD, RvBool createAsBadSyntax,
                             RvBool replaceExisting)
{
    char *p;
    RV_UNUSED_ARG(createAsBadSyntax);

    if (replaceExisting)
        rvSdpMsgDestroySessionName(pD->iMsg);

    p = pD->iCurrValue;

    if (!rvSdpParseAnyText(&p))
    {
        pD->iWarning.iWarningNum = RvSdpWarnSessionNameFailed;
        pD->iLineStatus = RvSdpValueParseFailed;
        return RV_FALSE;
    }

    if (rvSdpMsgSetSessionName(pD->iMsg,pD->iCurrValue) != RV_SDPSTATUS_OK)
        goto allocFail;

    return RV_TRUE;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    return RV_FALSE;
}



/*
 *	Parses the information line ('i='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseInformation(RvSdpParserData* pD, RvBool createAsBadSyntax,
                             RvBool replaceExisting)
{
    char *p;
    RV_UNUSED_ARG(createAsBadSyntax);

    if (replaceExisting)
    {
        if (pD->iMediaDescr)
            rvSdpMediaDescrDestroyInformation(pD->iMediaDescr);
        else
            rvSdpMsgDestroyInformation(pD->iMsg);
    }

    p = pD->iCurrValue;

    if (!rvSdpParseAnyText(&p))
    {
        pD->iWarning.iWarningNum = RvSdpWarnSessionInfoFailed;
        pD->iLineStatus = RvSdpValueParseFailed;
        return RV_FALSE;
    }

    if (pD->iMediaDescr)
    {
        if (rvSdpMediaDescrSetInformation(pD->iMediaDescr,pD->iCurrValue) != RV_SDPSTATUS_OK)
            goto allocFail;
    }
    else
    {
        if (rvSdpMsgSetSessionInformation(pD->iMsg,pD->iCurrValue) != RV_SDPSTATUS_OK)
            goto allocFail;
    }

    return RV_TRUE;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    return RV_FALSE;
}

/*
 *	Parses the URI line ('u='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseURI(RvSdpParserData* pD, RvBool createAsBadSyntax,
                     RvBool replaceExisting)
{
    char *p;

    if (replaceExisting)
        rvSdpMsgDestroyUri(pD->iMsg);

    p = pD->iCurrValue;

    if (!rvSdpParseText(&p,M_A) || strncmp(p,"://",3) || strchr(p+3,' '))
        goto setBadURI;

    goto setURI;

setBadURI:
    RV_SDP_WARNING_SET_AND_CREATE_WARNING(RvSdpWarnIllegalURI,pD);
    createAsBadSyntax = RV_TRUE;

setURI:
    if (rvSdpMsgSetURI2(pD->iMsg,pD->iCurrValue,createAsBadSyntax) != RV_SDPSTATUS_OK)
        goto allocFail;

    return RV_TRUE;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    return RV_FALSE;
}

/*
 *	Tests the correctness of 'e=' or 'p=' lines
 */
RvBool rvSdpTryMailPhone(char* p, int len, char** p1, char** p2,
                         char c1, char c2, REPLACING_DECL)
{
    char *ps;

    ps = p;
    p += len-1;

    if (*p != c2)
    {
/*
        if (strchr(ps,c1))
            return RV_FALSE;
*/
        *p1 = *p2 = NULL;
        return RV_TRUE;
    }

    REPLACING2(p);

    /* now find the c1 */

    while (*p != c1 && p != ps)
        p--;

    if (p == ps && *p != c1)
        /* could not find */
        return RV_FALSE;
    REPLACING2(p);

    *p2 = p+1;
    *p1 = ps;

    return RV_TRUE;
}

/*
 *	Parses the email line ('e='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseEmail(RvSdpParserData* pD, RvBool createAsBadSyntax,
                       RvBool replaceExisting)
{
    char *p, *pT, *pE, *pEE;
    RvBool retV = RV_TRUE;
    RvChar separSymbol = 0;
    RvSdpEmail *email;
    int n;
    REPLACING_START;

    RV_UNUSED_ARG(replaceExisting);

    if (createAsBadSyntax)
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    {
        if (rvSdpMsgAddBadSyntaxEmail(pD->iMsg,pD->iCurrValue) == NULL)
            goto allocFail;
        return RV_TRUE;
    }
#else
    {
        pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
        pD->iCriticalError = RV_TRUE;
        return RV_FALSE;
    }
#endif

    /* try to parse 'email ( text )' */
    /* test whether the last char is ')' */
    p = pD->iCurrValue + pD->iCurrValueLen - 1;

    if (!rvSdpTryMailPhone(pD->iCurrValue,pD->iCurrValueLen,&pE,&pT,'(',')',REPLACING_ARGS))
    {
        pD->iWarning.iWarningNum = RvSdpWarnEmailFailed;
        goto parseFail;
    }

    if (pE)
    {
        separSymbol = '(';
        goto testEmail;
    }

    if (!rvSdpTryMailPhone(pD->iCurrValue,pD->iCurrValueLen,&pT,&pE,'<','>',REPLACING_ARGS))
    {
        pD->iWarning.iWarningNum = RvSdpWarnEmailFailed;
        goto parseFail;
    }

    if (!pE)
    {
        pT = NULL;
        pE = pD->iCurrValue;
    }
    else
        separSymbol = '<';


testEmail:

    pEE = pE + strlen(pE) - 1;
    n = 0;
    while (*pEE == ' ')
    {
        pEE--;
        n++;
    }

    if (n)
    {
        pEE++;
        REPLACING(pEE);
    }


    if (strchr(pE,'@') == NULL || strchr(pE,' '))
    {
        pD->iWarning.iWarningNum = RvSdpWarnEmailValueIllegal;
        goto parseFail;
    }

    email = rvSdpMsgAddEmail(pD->iMsg,pE,pT);
    if (email == NULL)
        goto allocFail;

    email->iSeparSymbol = separSymbol;

    goto realReturn;

parseFail:
    pD->iLineStatus = RvSdpValueParseFailed;
    retV = RV_FALSE;
    goto realReturn;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    retV = RV_FALSE;
    goto realReturn;

realReturn:
    REPLACING_UNDO;
    return retV;
}

/*
 *	Parses the phone line ('p='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParsePhone(RvSdpParserData* pD, RvBool createAsBadSyntax,
                       RvBool replaceExisting)
{
    char *p, *pT, *pP;
    RvBool retV = RV_TRUE;
    RvChar separSymbol = 0;
    RvSdpPhone *phone;

    REPLACING_START;

    RV_UNUSED_ARG(replaceExisting);

    if (createAsBadSyntax)
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    {
        if (rvSdpMsgAddBadSyntaxPhone(pD->iMsg,pD->iCurrValue) == NULL)
            goto allocFail;
        return RV_TRUE;
    }
#else
    {
        pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
        pD->iCriticalError = RV_TRUE;
        return RV_FALSE;
    }
#endif

    /* try to parse 'email ( text )' */
    /* test whether the last char is ')' */
    p = pD->iCurrValue + pD->iCurrValueLen - 1;

    if (!rvSdpTryMailPhone(pD->iCurrValue,pD->iCurrValueLen,&pP,&pT,'(',')',REPLACING_ARGS))
    {
        pD->iWarning.iWarningNum = RvSdpWarnPhoneFailed;
        goto parseFail;
    }

    if (pP)
    {
        separSymbol = '(';
        goto testPhone;
    }

    if (!rvSdpTryMailPhone(pD->iCurrValue,pD->iCurrValueLen,&pT,&pP,'<','>',REPLACING_ARGS))
    {
        pD->iWarning.iWarningNum = RvSdpWarnPhoneFailed;
        goto parseFail;
    }

    if (!pP)
    {
        pT = NULL;
        pP = pD->iCurrValue;
    }
    else
        separSymbol = '<';


testPhone:
    p = pP;
    /* skip '+' by standard it could be one at most but I've seen
       examples with few '+' characters */
    while (*p == '+')
        p++;

    if (!rvSdpParseText(&p,M_PH) || *p)
    {
        pD->iWarning.iWarningNum = RvSdpWarnPhoneValueIllegal;
        goto parseFail;
    }

/*
    cnt = 0;
    for (;;)
    {
        if (!RV_SDP_IS_PHONE((int)*p))
            break;
        p++;
        cnt ++;
    }

    if (cnt == 0)
    {
        pD->iWarning.iWarningNum = RvSdpWarnPhoneValueIllegal;
        goto parseFail;
    }
*/

    phone = rvSdpMsgAddPhone(pD->iMsg,pP,pT);
    if (phone == NULL)
        goto allocFail;
    phone->iSeparSymbol = separSymbol;

    goto realReturn;

parseFail:
    pD->iLineStatus = RvSdpValueParseFailed;
    retV = RV_FALSE;
    goto realReturn;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    retV = RV_FALSE;
    goto realReturn;

realReturn:
    REPLACING_UNDO;
    return retV;
}

/*
 *	Parses the connection line ('c='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseConnection(RvSdpParserData* pD, RvBool createAsBadSyntax,
                            RvBool replaceExisting)
{
    char *p, *addr, *netTypeTxt, *addrTypeTxt, *p1, *p2;
    RvSdpNetType netType;
    RvSdpAddrType addrType;
    RvSdpConnection *conn = NULL;
    RvSdpParseWarning naw;
    int ttl = RV_SDP_INT_NOT_SET,numOfAddr = RV_SDP_INT_NOT_SET;
    RvUint32 n;
    void *obj;
    RvBool retV = RV_TRUE;
    RvSdpIntegerParseWarning intW;

    REPLACING_START

    obj = (pD->iMediaDescr) ? (void*)pD->iMediaDescr : (void*)pD->iMsg;

    RV_UNUSED_ARG(replaceExisting);

    if (createAsBadSyntax)
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    {
        if (rvSdpAddConnection2(obj,RV_SDPNETTYPE_NOTSET,RV_SDPADDRTYPE_NOTSET,NULL,
                                                pD->iCurrValue) == NULL)
            goto allocFail;
        goto realReturn;
    }
#else
    {
        pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
        pD->iCriticalError = RV_TRUE;
        return RV_FALSE;
    }
#endif

    p = pD->iCurrValue;

    if (!rvSdpParseNetType_AddrType_Addr(&p,&netType,&netTypeTxt,&addrType,&addrTypeTxt,&addr,&naw,REPLACING_ARGS))
    {
        pD->iWarning.iWarningNum = naw;
        goto parseFailed;
    }

    p1 = strchr(addr,'/');
    if (p1)
    {
        REPLACING(p1);
        p1++;
        p2 = strchr(p1,'/');
        if (p2)
        {
            REPLACING(p2);
            p2++;
            if (rvSdpParseInt(0,0,&n,RV_TRUE,RV_FALSE,&p2,&intW))
                numOfAddr = n;
            else
            {
                if (intW == RvSdpWarnNumberOverflow)
                {
                    pD->iWarning.iWarningNum = RvSdpWarnConnectionNumOfAddrTooLong;
                }
                else
                {
                    pD->iWarning.iWarningNum = RvSdpWarnConnectionNumOfAddrIllegal;
                }
                goto parseFailed;
            }
        }
        if (rvSdpParseInt(0,0,&n,RV_TRUE,RV_FALSE,&p1,&intW))
        {
            ttl = n;
            if (ttl > 255)
            {
                RV_SDP_WARNING_SET_AND_CREATE_WARNING(RvSdpWarnConnectionTTLTooLong,pD);
            }
        }
        else
        {
            if (intW == RvSdpWarnNumberOverflow)
            {
                pD->iWarning.iWarningNum = RvSdpWarnConnectionTTLTooLong;
            }
            else
            {
                pD->iWarning.iWarningNum = RvSdpWarnConnectionTTLIllegal;
            }
            goto parseFailed;
        }
    }

    conn = rvSdpAddConnection2(obj,netType,addrType,addr,NULL);
    if (conn == NULL)
        goto allocFail;

    if (ttl != RV_SDP_INT_NOT_SET)
    {
        conn->iTtl = ttl;
        if (numOfAddr != RV_SDP_INT_NOT_SET)
            conn->iNumAddr = numOfAddr;
    }

    rvSdpConnectionSetNetTypeStr(conn,netTypeTxt);
    rvSdpConnectionSetAddrTypeStr(conn,addrTypeTxt);

    goto realReturn;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    retV = RV_FALSE;
    goto realReturn;

parseFailed:
    pD->iLineStatus = RvSdpValueParseFailed;
    retV = RV_FALSE;
    goto realReturn;

realReturn:

    REPLACING_UNDO;
    return retV;
}

/*
 *	Parses the bandwidth line ('b='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseBandwidth(RvSdpParserData* pD, RvBool createAsBadSyntax,
                           RvBool replaceExisting)
{
    char *p;
    RvUint32 bwNum;
    RvBool retV = RV_TRUE;
    RvSdpBandwidth *bw = NULL;
    void* obj = (pD->iMediaDescr) ? (void*)pD->iMediaDescr : (void*)pD->iMsg;
    RvSdpIntegerParseWarning intW;


    REPLACING_START

    RV_UNUSED_ARG(replaceExisting);

    if (createAsBadSyntax)
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    {
        bw = rvSdpAddBandwidth2(obj,NULL,0,pD->iCurrValue);
        if (bw == NULL)
            goto allocFail;
        goto realReturn;
    }
#else
    {
        pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
        pD->iCriticalError = RV_TRUE;
        return RV_FALSE;
    }
#endif

    p = pD->iCurrValue;
    if (!rvSdpParseText(&p,M_BWT))
    {
        pD->iWarning.iWarningNum = RvSdpWarnBandwidthTypeIllegal;
        goto parseFailed;
    }

    if (*p != ':')
    {
        pD->iWarning.iWarningNum = RvSdpWarnBandwidthValueIllegal;
        goto parseFailed;
    }
    REPLACING(p)
    p++;

    if (!rvSdpParseInt(0,0,&bwNum,RV_TRUE,RV_FALSE,&p,&intW))
    {
        if (intW == RvSdpWarnNumberOverflow)
        {
            pD->iWarning.iWarningNum = RvSdpWarnBandwidthValueTooLong;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnBandwidthValueIllegal;
        }
        goto parseFailed;
    }

    if (*p)
    {
        pD->iWarning.iWarningNum = RvSdpWarnBandwidthValueIllegal;
        goto parseFailed;
    }

    bw = rvSdpAddBandwidth2(obj,pD->iCurrValue,bwNum,NULL);
    if (bw == NULL)
        goto allocFail;

    goto realReturn;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    retV = RV_FALSE;
    goto realReturn;

parseFailed:
    pD->iLineStatus = RvSdpValueParseFailed;
    retV = RV_FALSE;
    goto realReturn;

realReturn:
    REPLACING_UNDO;
    return retV;
}

/*
 *	Parses time start - stop construction.
 */
RvBool rvSdpParseStartStop(char**ptr, RvUint32* t, RvSdpIntegerParseWarning* intW)
{
    if (**ptr == '0')
    {
        (*ptr)++;
        *t = 0;
        return RV_TRUE;
    }
    return rvSdpParseInt(0,0xFFFFFFFF,t,RV_FALSE,RV_FALSE,ptr,intW);
}

/*
 *	Parses the session time line ('t='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseTime(RvSdpParserData* pD, RvBool createAsBadSyntax, RvBool replaceExisting)
{
    char *p;
    RvUint32 startT, stopT;
    RvSdpIntegerParseWarning intW;

    RV_UNUSED_ARG(replaceExisting);

    if (createAsBadSyntax)
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    {
        pD->iSessionTime = rvSdpMsgAddBadSyntaxSessionTime(pD->iMsg,pD->iCurrValue);
        if (pD->iSessionTime == NULL)
            goto allocFail;
        return RV_TRUE;
    }
#else
    {
        pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
        pD->iCriticalError = RV_TRUE;
        return RV_FALSE;
    }
#endif

    p = pD->iCurrValue;
    if (!rvSdpParseStartStop(&p,&startT,&intW))
    {
        if (intW == RvSdpWarnNumberOverflow)
        {
            pD->iWarning.iWarningNum = RvSdpWarnSessStartTimeTooLong;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnSessStartTimeIllegal;
        }
        goto parseFailed;
    }
    if (startT && startT < 1000000000)
        RV_SDP_WARNING_SET_AND_CREATE_WARNING(RvSdpWarnSessStartTimeTooSmall,pD);

    if (!rvSdpParseSpace(&p))
    {
        pD->iWarning.iWarningNum = RvSdpWarnSessStopTimeIllegal;
        goto parseFailed;
    }

    if (!rvSdpParseStartStop(&p,&stopT,&intW))
    {
        if (intW == RvSdpWarnNumberOverflow)
        {
            pD->iWarning.iWarningNum = RvSdpWarnSessStopTimeTooLong;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnSessStopTimeIllegal;
        }
        goto parseFailed;
    }
    if (stopT && stopT < 1000000000)
        RV_SDP_WARNING_SET_AND_CREATE_WARNING(RvSdpWarnSessStopTimeTooSmall,pD);

    if (stopT && startT > stopT)
        RV_SDP_WARNING_SET_AND_CREATE_WARNING(RvSdpWarnSessStartLessThanStop,pD);


    if (*p)
    {
        pD->iWarning.iWarningNum = RvSdpWarnSessStopTimeIllegal;
        goto parseFailed;
    }

    pD->iSessionTime = rvSdpMsgAddSessionTime(pD->iMsg,startT,stopT);
    if (pD->iSessionTime == NULL)
        goto allocFail;

    return RV_TRUE;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    return RV_FALSE;

parseFailed:
    pD->iLineStatus = RvSdpValueParseFailed;
    return RV_FALSE;
}

/*
 *	Parses typed time.
 */
RvBool rvSdpParseTypedTime(char** ptr, RvSdpTypedTime* tt, RvSdpIntegerParseWarning* intW)
{
    RvBool dontMove = RV_FALSE;

    *intW = RvSdpIntNonWarning;

    if (!rvSdpParseInt(0,0xFFFFFFFF,&tt->iTimeValue,RV_TRUE,RV_FALSE,ptr,intW))
        return RV_FALSE;
    if (**ptr == 'm')
        tt->iTimeType = RV_SDPTIMETYPE_MONTH;
    else if (**ptr == 'd')
        tt->iTimeType = RV_SDPTIMETYPE_DAY;
    else if (**ptr == 'h')
        tt->iTimeType = RV_SDPTIMETYPE_HOUR;
    else if (**ptr == 's')
        tt->iTimeType = RV_SDPTIMETYPE_SECOND;
    else if (**ptr == ' ' || **ptr == 0)
    {
        tt->iTimeType = RV_SDPTIMETYPE_SECOND;
        dontMove = RV_TRUE;
    }
    else
        return RV_FALSE;

    if (!dontMove)
        (*ptr)++;
    return RV_TRUE;
}

/*
 *	Parses the repeat interval line ('r='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseTimeRepeat(RvSdpParserData* pD, RvBool createAsBadSyntax,
                            RvBool replaceExisting)
{
    char *p;
    int cnt;
    RvSdpTypedTime interv, duration, offs;
    RvSdpRepeatInterval* rinterv = NULL;
    RvSdpIntegerParseWarning intW;

    RV_UNUSED_ARG(replaceExisting);

    if (createAsBadSyntax)
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    {
        rinterv = rvSdpRepeatIntervalConstruct2(pD->iMsg,NULL,
                                0,RV_SDPTIMETYPE_NOT_SET,0,RV_SDPTIMETYPE_NOT_SET,
                                pD->iCurrValue,NULL,RV_FALSE);
        if (rinterv == NULL)
            goto allocFail;
        goto attachToMsg;
    }
    else
#else
    {
        pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
        pD->iCriticalError = RV_TRUE;
        return RV_FALSE;
    }
#endif


    p = pD->iCurrValue;

    if (!rvSdpParseTypedTime(&p,&interv,&intW))
    {
        if (intW == RvSdpWarnNumberOverflow)
        {
            pD->iWarning.iWarningNum = RvSdpWarnRepeatIntervalValueTooLong;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnRepeatIntervalValueIllegal;
        }
        goto parseFailed;
    }

    if (!rvSdpParseSpace(&p))
    {
        pD->iWarning.iWarningNum = RvSdpWarnRepeatDurationValueIllegal;
        goto parseFailed;
    }

    if (!rvSdpParseTypedTime(&p,&duration,&intW))
    {
        if (intW == RvSdpWarnNumberOverflow)
        {
            pD->iWarning.iWarningNum = RvSdpWarnRepeatDurationValueTooLong;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnRepeatDurationValueIllegal;
        }
        goto parseFailed;
    }

    for (cnt=0;;cnt++)
    {
        if (*p == 0)
        {
            if (cnt == 0)
            {
                pD->iWarning.iWarningNum = RvSdpWarnRepeatNoOffsets;
                goto parseFailed;
            }
            break;
        }

        if (!rvSdpParseSpace(&p))
        {
            pD->iWarning.iWarningNum = RvSdpWarnRepeatOffsetValueIllegal;
            goto parseFailed;
        }

        if (!rvSdpParseTypedTime(&p,&offs,&intW))
        {
            if (intW == RvSdpWarnNumberOverflow)
            {
                pD->iWarning.iWarningNum = RvSdpWarnRepeatOffsetValueTooLong;
            }
            else
            {
                pD->iWarning.iWarningNum = RvSdpWarnRepeatOffsetValueIllegal;
            }
            goto parseFailed;
        }

        if (cnt == 0)
        {
            rinterv = rvSdpRepeatIntervalConstruct2(pD->iMsg,NULL,interv.iTimeValue,interv.iTimeType,
                duration.iTimeValue,duration.iTimeType,NULL,NULL,RV_FALSE);
            if (!rinterv)
                goto allocFail;
        }

        if (rinterv && rvSdpRepeatIntervalAddOffset(rinterv,offs.iTimeValue,
                                                    offs.iTimeType) != RV_SDPSTATUS_OK)
            goto allocFail;
    }

    goto attachToMsg;  /*keep compiler happy*/
attachToMsg:
    if (pD->iSessionTime)
        rvSdpSessionTimeAddRepeatInterval2(rinterv,pD->iSessionTime,
                        0,RV_SDPTIMETYPE_NOT_SET,0,RV_SDPTIMETYPE_NOT_SET,NULL);
    else
    {
        pD->iWarning.iWarningNum = RvSdpWarnRepeatNoSessTime;
        if (rinterv)
            rvSdpRepeatIntervalDestruct(rinterv);
        pD->iLineStatus = RvSdpStatusIgnore;
        return RV_FALSE;
    }

    return RV_TRUE;

allocFail:
    if (rinterv)
        rvSdpRepeatIntervalDestruct(rinterv);
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    return RV_FALSE;

parseFailed:
    if (rinterv)
        rvSdpRepeatIntervalDestruct(rinterv);
    pD->iLineStatus = RvSdpValueParseFailed;
    return RV_FALSE;
}

/*
 *	Parses the time zone adjustment line ('z='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseZoneAdjust(RvSdpParserData* pD, RvBool createAsBadSyntax,
                            RvBool replaceExisting)
{
    char *p;
    RvSdpTimeZoneAdjust* tz = NULL;
    RvUint32 t1;
    RvInt32  t2;
    RvSdpTypedTime tt;
    RvBool isNegative;
    RvSdpIntegerParseWarning intW;
    RV_UNUSED_ARG(tz);

    if (replaceExisting)
        rvSdpMsgTZADestroy(pD->iMsg);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (createAsBadSyntax)
    {
        if (rvSdpMsgSetBadSyntaxZoneAdjustment(pD->iMsg,pD->iCurrValue) != RV_SDPSTATUS_OK)
            goto allocFail;
        return RV_TRUE;
    }
#endif
    if (createAsBadSyntax)
    {
        pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
        pD->iCriticalError = RV_TRUE;
        return RV_FALSE;
    }

    p = pD->iCurrValue;

    for (;;)
    {

        if (!*p)
        {
            if (p != pD->iCurrValue)
                break;
            else
            {
                pD->iWarning.iWarningNum = RvSdpWarnTimeZoneNoData;
                goto parseFailed;
            }
        }

        if (p != pD->iCurrValue)
        /* not the first pair*/
        {
            if (!rvSdpParseSpace(&p))
            {
                pD->iWarning.iWarningNum = RvSdpWarnTimeZoneIllegalOffset;
                goto parseFailed;
            }
        }

        if (!rvSdpParseInt(0,0xFFFFFFFF,&t1,RV_TRUE,RV_FALSE,&p,&intW))
        {
            if (intW == RvSdpWarnNumberOverflow)
            {
                pD->iWarning.iWarningNum = RvSdpWarnTimeZoneTimeTooLong;
            }
            else
            {
                pD->iWarning.iWarningNum = RvSdpWarnTimeZoneIllegalTime;
            }
            goto parseFailed;
        }

        if (!rvSdpParseSpace(&p))
        {
            pD->iWarning.iWarningNum = RvSdpWarnTimeZoneIllegalTime;
            goto parseFailed;
        }

        if (*p == '-')
        {
            isNegative = RV_TRUE;
            p++;
        }
        else
            isNegative = RV_FALSE;

        if (!rvSdpParseTypedTime(&p,&tt,&intW))
        {
            if (intW == RvSdpWarnNumberOverflow)
            {
                pD->iWarning.iWarningNum = RvSdpWarnTimeZoneOffsetTooLong;
            }
            else
            {
                pD->iWarning.iWarningNum = RvSdpWarnTimeZoneIllegalOffset;
            }
            goto parseFailed;
        }

        t2 = tt.iTimeValue;
        if (isNegative)
            t2 *= -1;

        if (!rvSdpMsgTimeAddZoneAdjustment(pD->iMsg,t1,t2,tt.iTimeType))
            goto allocFail;
    }
    return RV_TRUE;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    return RV_FALSE;

parseFailed:
    pD->iLineStatus = RvSdpValueParseFailed;
    return RV_FALSE;
}

/*
 *	Parses the key line ('k='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseKey(RvSdpParserData* pD, RvBool createAsBadSyntax, RvBool replaceExisting)
{
    char *p;
    char *kt, *kd;
    RvSdpEncrMethod meth;
    RvBool retV = RV_TRUE;
    RvSdpCommonFields* commF;

    REPLACING_START

    if (pD->iMediaDescr)
        commF = &pD->iMediaDescr->iCommonFields;
    else
        commF = &pD->iMsg->iCommonFields;

    if (replaceExisting)
        rvSdpKeyDestruct(&commF->iKey);

    if (createAsBadSyntax)
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    {

        if (rvSdpKeyConstruct2(pD->iMsg,&commF->iKey,RV_SDPENCRMTHD_NOTSET,
                                            NULL,pD->iCurrValue,NULL,RV_FALSE) == NULL)
            goto allocFail;
        return RV_TRUE;
    }
#else
    {
        pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
        pD->iCriticalError = RV_TRUE;
        return RV_FALSE;
    }
#endif

    p = pD->iCurrValue;

    if (!rvSdpParseText(&p,M_ADH))
    {
        pD->iWarning.iWarningNum = RvSdpWarnKeyMethodIllegal;
        goto parseFailed;
    }

    kt = pD->iCurrValue;
    kd = NULL;

    if (*p != ':' && *p)
    {
        pD->iWarning.iWarningNum = RvSdpWarnKeyMethodIllegal;
        goto parseFailed;
    }

    if (*p == ':' && *(p+1) == 0)
    {
        pD->iWarning.iWarningNum = RvSdpWarnKeyValueNotSet;
        goto parseFailed;
    }

    if (*p != 0)
    {
        REPLACING(p)
        kd = p + 1;
    }

    meth = rvSdpKeyTypeTxt2Val(kt);
    if (rvSdpKeyConstruct2(pD->iMsg,&commF->iKey,meth,kd,NULL,NULL,RV_FALSE) == NULL)
        goto allocFail;


    if (rvSdpKeySetTypeStr(&commF->iKey,kt) != RV_SDPSTATUS_OK)
    {
        rvSdpKeyDestruct(&commF->iKey);
        goto allocFail;
    }

    goto realReturn;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    retV = RV_FALSE;
    goto realReturn;

parseFailed:
    pD->iLineStatus = RvSdpValueParseFailed;
    retV = RV_FALSE;
    goto realReturn;

realReturn:
    REPLACING_UNDO;
    return retV;
}



/*
 *	Parses the attribute line ('a='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseAttribute(RvSdpParserData* pD, RvBool createAsBadSyntax, RvBool replaceExisting)
{
    char *p, *p1, *n, *v = NULL;
    RvBool retV = RV_TRUE;
    RvSdpCommonFields *cm;
	const RvSdpSpecAttributeData* attrData;
	RvSdpSpecAttrParseSts specAttrPrsRes;
    REPLACING_START;

    RV_UNUSED_ARG(replaceExisting);

    cm = (pD->iMediaDescr) ? &pD->iMediaDescr->iCommonFields : &pD->iMsg->iCommonFields;

    n = p = pD->iCurrValue;
    if (!rvSdpParseText(&p,M_SF))
    {
        RV_SDP_WARNING_SET_AND_CREATE_WARNING(RvSdpWarnAttrNameIllegal,pD);
        goto createAttr;
    }

    p1 = strchr(n,':');
    if (p1 && p1 < p)
        p = p1;

    if (*p != ':' && *p)
    {
        RV_SDP_WARNING_SET_AND_CREATE_WARNING(RvSdpWarnAttrNameIllegal,pD);
        goto createAttr;
    }

    if (*p)
    {
        REPLACING(p);
        v = p+1;
    }
    else
        v = NULL;

	attrData = rvSdpFindSpecAttrDataByName(n);

	if (attrData && attrData->iParsingFunc)
	{
		specAttrPrsRes = attrData->iParsingFunc(attrData,createAsBadSyntax,pD,cm,n,v,REPLACING_ARGS);

		if (specAttrPrsRes == rvSdpSpAttrPrsAllocFail)
			goto allocFail;
		else if (specAttrPrsRes == rvSdpSpAttrPrsFail)
			retV = RV_FALSE;
		else if (specAttrPrsRes == rvSdpSpAttrPrsCrtRegular)
			goto createAttr;

		goto realReturn;
	}

createAttr:
    if (rvSdpAddAttr2(pD->iMsg,cm,NULL,n,v)==NULL)
        goto allocFail;

    goto realReturn;

allocFail:
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    retV = RV_FALSE;
    goto realReturn;


realReturn:
    REPLACING_UNDO;
    return retV;
}

/*
 *	Parses the media description line ('m='), constructs correspondent
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise.
 */
RvBool rvSdpParseMediaDescr(RvSdpParserData* pD, RvBool createAsBadSyntax, RvBool replaceExisting)
{
    char *p, *pp, *mediaTxt, *protoTxt, *fmt = NULL;
    RvSdpMediaDescr *md = NULL;
    RvSdpMediaType  medType;
    RvSdpProtocol   proto;
    RvUint32 portNum,subPortNum,cnt;
    RvBool  retV = RV_TRUE;
    RvSdpIntegerParseWarning intW;
    REPLACING_START

    RV_UNUSED_ARG(replaceExisting);

    if (createAsBadSyntax)
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    {
        pD->iMediaDescr = rvSdpMsgAddBadSyntaxMediaDescr(pD->iMsg,pD->iCurrValue);
        if (pD->iMediaDescr == NULL)
            goto allocFail;
        return RV_TRUE;
    }
#else
    {
        pD->iStopParseSts = RV_SDPPARSER_STOP_ERROR;
        pD->iCriticalError = RV_TRUE;
        return RV_FALSE;
    }
#endif

    p = pD->iCurrValue;

    if (!rvSdpParseText(&p,M_MD))
    {
        pD->iWarning.iWarningNum = RvSdpWarnIllegalMedia;
        goto parseFailed;
    }

    mediaTxt = pD->iCurrValue;
    pp = p;

    if (!rvSdpParseSpace(&p))
    {
        pD->iWarning.iWarningNum = RvSdpWarnIllegalMedia;
        goto parseFailed;
    }
    REPLACING(pp)

    if (!rvSdpParseInt(0,0,&portNum,RV_TRUE,RV_FALSE,&p,&intW))
    {
        if (intW == RvSdpWarnNumberOverflow)
        {
            pD->iWarning.iWarningNum = RvSdpWarnMediaPortTooLong;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnIllegalMediaPort;
        }
        goto parseFailed;
    }

    if (*p == '/')
    {
        p++;
        if (!rvSdpParseInt(1,0,&subPortNum,RV_FALSE,RV_FALSE,&p,&intW))
        {
            if (intW == RvSdpWarnNumberOverflow)
            {
                pD->iWarning.iWarningNum = RvSdpWarnMediaSubPortTooLong;
            }
            else
            {
                pD->iWarning.iWarningNum = RvSdpWarnIllegalMediaSubPort;
            }
            goto parseFailed;
        }
    }
    else
        subPortNum = 0;

    if (!rvSdpParseSpace(&p))
    {
        if (subPortNum)
        {
            pD->iWarning.iWarningNum = RvSdpWarnIllegalMediaSubPort;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnIllegalMediaPort;
        }
        goto parseFailed;
    }

    protoTxt = p;
    if (!rvSdpParseText(&p,M_PR))
    {
        pD->iWarning.iWarningNum = RvSdpWarnIllegalMediaProtocol;
        goto parseFailed;
    }
    pp = p;

    for (cnt = 0;;cnt++)
    {
        if (*p == 0)
        {
            if (cnt == 0 || !md)
            {
                pD->iWarning.iWarningNum = RvSdpWarnNoFormatsForMedia;
                goto parseFailed;
            }

            if (fmt && rvSdpMediaDescrAddFormat(md,fmt) != RV_SDPSTATUS_OK)
                goto allocFail;

            rvSdpListTailAdd(&pD->iMsg->iMediaDescriptors,md);
            pD->iMediaDescr = md;
            goto realReturn;
        }

        if (!rvSdpParseSpace(&p))
        {
            if (cnt == 0)
            {
                pD->iWarning.iWarningNum = RvSdpWarnIllegalMediaProtocol;
            }
            else
            {
                pD->iWarning.iWarningNum = RvSdpWarnIllegalMediaFormat;
            }
            goto parseFailed;
        }
        REPLACING(pp)

        if (fmt && md && rvSdpMediaDescrAddFormat(md,fmt) != RV_SDPSTATUS_OK)
            goto allocFail;

        fmt = p;
        if (!rvSdpParseText(&p,M_FMT))
        {
            pD->iWarning.iWarningNum = RvSdpWarnIllegalMediaFormat;
            goto parseFailed;
        }
        pp = p;

        if (cnt == 0)
        /* creating the media descriptor now */
        {
            proto = rvSdpMediaProtoTxt2Val(protoTxt);
            medType = rvSdpMediaTypeTxt2Val(mediaTxt);

            md = rvSdpMediaDescrConstructEx(pD->iMsg,NULL,medType,portNum,proto,
                NULL,NULL,RV_FALSE);
            if (md == NULL)
                goto allocFail;

			if (rvSdpMediaDescrSetMediaTypeStr(md,mediaTxt) != RV_SDPSTATUS_OK)
				goto allocFail;
			if (rvSdpMediaDescrSetProtocolStr(md,protoTxt) != RV_SDPSTATUS_OK)
				goto allocFail;
            rvSdpMediaDescrSetNumOfPorts(md,subPortNum);
        }
    }

allocFail:
    if (md)
        rvSdpMediaDescrDestruct(md);
    pD->iStopParseSts = RV_SDPPARSER_STOP_ALLOCFAIL;
    pD->iCriticalError = RV_TRUE;
    retV = RV_FALSE;
    goto realReturn;

parseFailed:
    if (md)
        rvSdpMediaDescrDestruct(md);

    pD->iLineStatus = RvSdpValueParseFailed;
    retV = RV_FALSE;
    goto realReturn;

realReturn:
    REPLACING_UNDO;
    return retV;
}

