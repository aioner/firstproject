/*
*********************************************************************************
*                                                                               *
* NOTICE:                                                                       *
* This document contains information that is confidential and proprietary to    *
* RADVision LTD.. No part of this publication may be reproduced in any form     *
* whatsoever without written prior approval by RADVision LTD..                  *
*                                                                               *
* RADVision LTD. reserves the right to revise this publication and make changes *
* without obligation to notify any person of such revisions or changes.         *
*********************************************************************************
*/


/*********************************************************************************
 *                              <RtspFirstLine.c>
 *
 *  This file manages the serialization and de-serialization of the RTSP message's
 *  first line. It includes the serialization/de-serialization functions for the
 *  request and status lines.
 *
 *    Author                         Date
 *    ------                        ------
 *      Tom                         16/12/03
 *********************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILED                           */
/*-----------------------------------------------------------------------*/
#include "rvtypes.h"
#include "rvstdio.h"
#include "rvansi.h"


#include "RvRtspCommonTypes.h"
#include "RtspFirstLine.h"
#include "RtspUtilsInternal.h"


/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/
#define RTSP_VERSION_1_1        "RTSP/1.1"
#define RTSP_VERSION_1_0        "RTSP/1.0"
#define RTSP_VERSION            RTSP_VERSION_1_0

#define STATUS_CODE_LENGTH  3

/* RvRtspMethodStr
 * ----------
 * a static array, containing the method strings, used for serializing/
 * de-serializing methods.
 */
static RvChar   *RvRtspMethodStr[RV_RTSP_METHOD_MAX] =
{
    "DESCRIBE",
    "ANNOUNCE",
    "GET_PARAMETER",
    "OPTIONS",
    "PAUSE",
    "PLAY",
    "RECORD",
    "REDIRECT",
    "SETUP",
    "SET_PARAMETER",
    "TEARDOWN"
};

/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/



/**************************************************************************
 * RtspGetMethod
 * ------------------------------------------------------------------------
 * General: de-serializes and returns the given method.
 *
 * Return Value:    The string method as RvRtspMethod.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   strMethod   - the method in string form.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvRtspMethod RtspGetMethod(
                IN  RvChar  *strMethod);




/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/



/**************************************************************************
 * RtspRequestLineSerialize
 * ------------------------------------------------------------------------
 * General: serializes the given RtspRequestLine structure into the RPool
 *          element.
 *
 * Return Value:    RV_OK if request line is serialized successfully,
 *                  negative values if failed
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's RPool memory pool.
 *              pLine       - the request line to serialize.
 * OUTPUT   :   pHLine      - the element to serialize into.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspRequestLineSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvRtspRequestLine   *pLine,
                        OUT HRPOOLELEM          *pHLine)
{
    RvSize_t    uriLength;
    RvSize_t    elementLength;
    RvSize_t    elementOffset;
    RvSize_t    methodStrLen = 0;
    RvChar      methodStr[50];

    /* checking parameters */
    if ( (hRPool == NULL) || (pLine == NULL) || (pHLine == NULL) )
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    if ( (pLine->method > RV_RTSP_METHOD_UNKNOWN) || (pLine->hURI == NULL) )
        return RvRtspErrorCode(RV_ERROR_BADPARAM);

    /* allocating a suitable chunk */
    uriLength = RtspUtilsRPOOLELEMStrLen(hRPool,(HRPOOLELEM)pLine->hURI,0);

    /* returns the length of the URI string */
    if (pLine->method < RV_RTSP_METHOD_MAX)
        methodStrLen = strlen(RvRtspMethodStr[pLine->method]);
    else if (pLine->methodStr != NULL && pLine->method == RV_RTSP_METHOD_UNKNOWN)
        methodStrLen = (RvSize_t)rpoolChunkSize(hRPool, (HRPOOLELEM)pLine->methodStr);
    elementLength = methodStrLen +                                   /* the method name length */
                    1 +                                             /* " "              */
                    uriLength +                                     /* the URI          */
                    1 +                                             /* " "              */
                    strlen(RTSP_VERSION) +                          /* the RTSP version */
                    2;                                              /* "\r\n"           */

    *pHLine = rpoolAlloc(hRPool, (RvInt32)elementLength);
    if (*pHLine == NULL)
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);

    elementOffset = 0;

    memset(methodStr, 0, sizeof(methodStr));

    if (pLine->method < RV_RTSP_METHOD_MAX)
        memcpy(methodStr, RvRtspMethodStr[pLine->method], methodStrLen);
    else if (pLine->methodStr != NULL && pLine->method == RV_RTSP_METHOD_UNKNOWN)
        rpoolCopyToExternal(hRPool, (void *)methodStr, (HRPOOLELEM)pLine->methodStr, 0, (RvInt32)methodStrLen);

    /* serializing rtsp method */
    rpoolCopyFromExternal(  hRPool,
                            *pHLine,
                            methodStr,
                            (RvInt32)elementOffset,
                            (RvInt32)methodStrLen);
    elementOffset += methodStrLen;


    /* serializing ' ' */
    rpoolCopyFromExternal(  hRPool,
                            *pHLine,
                            " ",
                            (RvInt32)elementOffset,
                            1);
    elementOffset++;


    /* searialzing the URI */
    RtspUtilsRPOOLELEMCopyInternal( hRPool,
                                    *pHLine,
                                    elementOffset,
                                    (HRPOOLELEM)pLine->hURI,
                                    0,
                                    uriLength);
    elementOffset += uriLength;

    /* serializing ' ' */
    rpoolCopyFromExternal(  hRPool,
                            *pHLine,
                            " ",
                            (RvInt32)elementOffset,
                            1);
    elementOffset++;

    /* serializing the RTSP version */
    rpoolCopyFromExternal(  hRPool,
                            *pHLine,
                            RTSP_VERSION,
                            (RvInt32)elementOffset,
                            (RvInt32)strlen(RTSP_VERSION));
    elementOffset += strlen(RTSP_VERSION);


    /* serializing '\r\n' */
    rpoolCopyFromExternal(  hRPool,
                            *pHLine,
                            "\r\n",
                            (RvInt32)elementOffset,
                            2);
    elementOffset += 2;

    return RV_OK;
}




/**************************************************************************
 * RtspStatusLineSerialize
 * ------------------------------------------------------------------------
 * General: serializes the given RvRtspStatusLine structure into the RPool
 *          element.
 *
 * Return Value:    RV_OK if request line is serialized successfully,
 *                  negative values if failed
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's RPool memory pool.
 *              pLine       - the status line to serialize.
 * OUTPUT   :   pHLine      - the element to serialize into.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspStatusLineSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvRtspStatusLine    *pLine,
                        OUT HRPOOLELEM          *pHLine)
{
    RvSize_t    phraseLength;
    RvSize_t    elementLength;
    RvSize_t    elementOffset = 0;
    RvChar      strStatusCode[STATUS_CODE_LENGTH+1];

    /* checking parameters */
    if ( (hRPool == NULL) || (pLine == NULL) || (pHLine == NULL) )
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    /* allocating the space for the status line */
    phraseLength = RtspUtilsRPOOLELEMStrLen(hRPool,(HRPOOLELEM)pLine->hPhrase,0);
                                                /* returns the length of phrase string */

    elementLength = strlen(RTSP_VERSION) +                          /* RTSP version     */
                    1 +                                             /* " "              */
                    STATUS_CODE_LENGTH +                            /* status code      */
                    phraseLength +                                  /* status phrase    */
                    2;                                              /* "\r\n"           */
    if (phraseLength)
        elementLength += 1;                                         /* " " before the phrase */

    *pHLine = rpoolAlloc(hRPool, (RvInt32)elementLength);
    if (*pHLine == NULL)
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);


    /* serializing the rtsp version */
    rpoolCopyFromExternal(  hRPool,
                            *pHLine,
                            RTSP_VERSION,
                            (RvInt32)elementOffset,
                            (RvInt32)strlen(RTSP_VERSION));
    elementOffset += strlen(RTSP_VERSION);

    /* serializing " " */
    rpoolCopyFromExternal(  hRPool,
                            *pHLine,
                            " ",
                            (RvInt32)elementOffset,
                            1);
    elementOffset++;

    /* serializing status code */
    sprintf(strStatusCode, "%3d", pLine->status);
    rpoolCopyFromExternal(  hRPool,
                            *pHLine,
                            strStatusCode,
                            (RvInt32)elementOffset,
                            STATUS_CODE_LENGTH);
    elementOffset += STATUS_CODE_LENGTH;

    /* serializing the phrase */
    if (phraseLength > 0)
    {
        /* serializing " " */
        rpoolCopyFromExternal(  hRPool,
                                *pHLine,
                                " ",
                                (RvInt32)elementOffset,
                                1);
        elementOffset++;

        RtspUtilsRPOOLELEMCopyInternal( hRPool,
                                        *pHLine,
                                        elementOffset,
                                        (HRPOOLELEM)pLine->hPhrase,
                                        0,
                                        phraseLength);

        elementOffset += phraseLength;
    }

    /* serializing the "\r\n" */
    rpoolCopyFromExternal(  hRPool,
                            *pHLine,
                            "\r\n",
                            (RvInt32)elementOffset,
                            2);
    elementOffset += 2;

    return RV_OK;
}




/**************************************************************************
 * RtspFirstLineDeserialize
 * ------------------------------------------------------------------------
 * General: De-serializes a message's line from the given RPool element
 *          into one of the the given line structures.
 *
 * Return Value:    RV_OK if line is serialized successfully, a negative
 *                  value if failed.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool          - the module's memory pool.
 *              hLine           - the element containing the line to deserialize.
 *              pRequestLine    - the request line to deserialize into, if hLine
 *                              is a request line.
 *              pStatusLine     - the status line to deserialize into, if hLine
 *                              is a status line.
 * OUTPUT   :   requestLineValid- set to RV_TRUE if a request line is deserialized
 *                              set to RV_FALSE otherwise.
 *              statusLineValid - set to RV_TRUE is status line is serialized
 *                              set to RV_FALSE otherwise.
 * INOUT    :   None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspFirstLineDeserialize(
                        IN  HRPOOL              hRPool,
                        IN  HRPOOLELEM          hLine,
                        OUT RvRtspRequestLine   *pRequestLine,
                        OUT RvRtspStatusLine    *pStatusLine,
                        OUT RvBool              *pRequestLineValid,
                        OUT RvBool              *pStatusLineValid)
{
    RvBool      success;
    RvChar      token[50];
    RvSize_t    tokenLength;
    RvSize_t    lineOffset;
    RvSize_t    nextOffset;
    RvSize_t    phraseLength;

    *pRequestLineValid  = RV_FALSE;
    *pStatusLineValid   = RV_FALSE;

    success = RtspUtilsRPOOLELEMGetToken(hRPool,
                                        hLine,
                                        0,
                                        " ",
                                        RV_FALSE,
                                        token,
                                        40,
                                        &tokenLength);
    if (success == RV_FALSE)
        return RvRtspErrorCode(RV_ERROR_BADPARAM);

    token[tokenLength] = '\0';
    lineOffset = tokenLength + 1;

    if (strstr(token, RTSP_VERSION_1_1) || strstr(token, RTSP_VERSION_1_0))
    {
        /* this is a status line */
        RvInt rVal;

        /* get the code */
        RtspUtilsRPOOLELEMGetToken(hRPool, hLine, lineOffset, " ", RV_FALSE, token, 40, &tokenLength);
        token[tokenLength] = '\0';
        lineOffset += tokenLength + 1;
        if (sscanf(token,"%d", &rVal) == 1)
            pStatusLine->status = (RvRtspStatus)rVal;

        /* allocate and get the phrase */
        /* the phrase length is the remaining length of the message minus the "\r\n"  + null terminating*/
        phraseLength = (RvSize_t)rpoolChunkSize(hRPool,hLine) - lineOffset;
        pStatusLine->hPhrase = (RvRtspStringHandle)rpoolAlloc(hRPool, (RvInt32)phraseLength);
        RtspUtilsRPOOLELEMCopyInternal(hRPool, (HRPOOLELEM)pStatusLine->hPhrase, 0, hLine, lineOffset, phraseLength);

        /* adding null terminating character */
        RtspUtilsRPOOLELEMSetCell(hRPool, (HRPOOLELEM)pStatusLine->hPhrase, phraseLength, 0);

        *pStatusLineValid = RV_TRUE;
        return RV_OK;
    } /* status line */

    else
    {
        /* this is a request line */
        pRequestLine->method = RtspGetMethod(token);

        if (pRequestLine->method == RV_RTSP_METHOD_UNKNOWN)
            return RvRtspErrorCode(RV_ERROR_UNKNOWN);


        /* allocate and get the URI */
        /* the URI length is the remaining length of the message minus the  */
        /* RTSP version and the "\r\n"                                      */
        success = RtspUtilsRPOOLELEMStrChr(hRPool, hLine, lineOffset, ' ', &nextOffset);
        if (success == RV_FALSE)
            return RvRtspErrorCode(RV_ERROR_UNKNOWN);

        pRequestLine->hURI = (RvRtspStringHandle)rpoolAlloc(hRPool, (RvInt32)(nextOffset-lineOffset) + 1);
        RtspUtilsRPOOLELEMCopyInternal( hRPool,
                                        (HRPOOLELEM)pRequestLine->hURI,
                                        0,
                                        hLine,
                                        lineOffset,
                                        nextOffset-lineOffset);

        /* adding null terminating character */
        RtspUtilsRPOOLELEMSetCell(  hRPool,
                                    (HRPOOLELEM)pRequestLine->hURI,
                                    lineOffset + nextOffset,
                                    0);


        *pRequestLineValid = RV_TRUE;
        return RV_OK;
    } /* request line */
}



/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/



/**************************************************************************
 * RtspGetMethod
 * ------------------------------------------------------------------------
 * General: de-serializes and returns the given method.
 *
 * Return Value:    The string method as RvRtspMethod.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   strMethod   - the method in string form.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvRtspMethod RtspGetMethod(
                IN  RvChar  *strMethod)
{
    RvRtspMethod method;

    for (method = RV_RTSP_METHOD_DESCRIBE; method < RV_RTSP_METHOD_MAX; method++)
    {
        /* try to find the method in each of the array cells    */
        if (strstr(strMethod, RvRtspMethodStr[method]))
            return method;
    }

    return RV_RTSP_METHOD_UNKNOWN;
}



#if defined(__cplusplus)
}
#endif
