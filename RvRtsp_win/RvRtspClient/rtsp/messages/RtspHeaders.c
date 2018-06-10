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
 *                              <RtspHeaders.c>
 *
 *  This code handles the message headers serialization and de-serialization.
 *
 *
 *    Author                         Date
 *    ------                        ------
 *      Tom                         12/1/04
 *
 *********************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILED                           */
/*-----------------------------------------------------------------------*/
#include "rvtypes.h"
#include "rvstdio.h"


#include "RvRtspCommonTypes.h"
#include "RtspHeaders.h"
#include "RtspUtilsInternal.h"

//added by lichao, 20140814
#include "RvRtspMsg.h"

/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/




/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/



/**************************************************************************
 * RtspHeadersStrSerialize
 * ------------------------------------------------------------------------
 * General: serializes a header containing a string into an rpool element
 *          containing the header name and value.
 *
 * Return Value: RV_TRUE if serialization is successfull, negative values if not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool          - The module's memory pool.
 *              strPrefix       - The prefix to write at the start of the element
 *              hStr            - The RtspString to write after the prefix.
 *              addDelimiter    - RV_TRUE if the "\r\n" delimiter should be added.
 * OUTPUT   :   phElement       - an RPool element with strPrefix followed by
 *                                hStr.
 * INOUT    :   None.
 *************************************************************************/
static RvBool RtspHeadersStrSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvChar              *strPrefix,
                        IN  RvRtspStringHandle  hStr,
                        IN  RvBool              addDelimiter,
                        OUT HRPOOLELEM          *phElement);



/**************************************************************************
 * RtspHeadersStrDeserialize
 * ------------------------------------------------------------------------
 * General: deserializes a header containing a string into an Rtsp string.
 *
 * Return Value:    RV_TRUE if serialization is successfull, negative values
 *                  if not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool          - The module's memory pool.
 *              hLine           - The line to deserialize from.
 *              lineOffset      - The offset from which to start taking the
 *                              header content.
 *              strDelimiters   - The delimiters for the end of the header
 *                              content.
 * OUTPUT   :   pHStr           - an Rtsp string with the header content.
 * INOUT    :   None.
 *************************************************************************/
static RvBool RtspHeadersStrDeserialize(
                        IN  HRPOOL                  hRPool,
                        IN  HRPOOLELEM              hLine,
                        IN  RvSize_t                lineOffset,
                        IN  RvChar                  *strDelimiters,
                        OUT RvRtspStringHandle      *pHStr);



/**************************************************************************
 * RtspHeadersNptTimeSerialize
 * ------------------------------------------------------------------------
 * General: serializes the given RvRtspNptTime structure into the given
 *          buffer.
 *
 * Return Value: RV_TRUE    - Time range serialized
 *               RV_FALSE   - Serialization unsuccessful.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   time        - The time to serialize.
 * OUTPUT   :   buffer      - The buffer to write into.
 * INOUT    :   None.
 *************************************************************************/
static RvBool RtspHeadersNptTimeSerialize(
            IN  RvRtspNptTime   *time,
            OUT RvChar          *buffer);


/**************************************************************************
 * RtspHeadersRtpInfoLineDeserialize
 * ------------------------------------------------------------------------
 * General: De-serializes an RTP-Info header line from the given element
 *          into the given RvRtspRtpInfoHeader structure .
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pRtpInfo    - the header structure to deserialize into.
 * INOUT    :   None.
 *******************************************************************/
static RvBool RtspHeadersRtpInfoLineDeserialize(
                        IN  HRPOOL          hRPool,
                        IN  HRPOOLELEM      hLine,
                        IN  RvSize_t        lineOffset,
                        OUT RvRtspRtpInfo   *pRtpInfo);



/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/



/* serialization */



/**************************************************************************
 * RtspHeadersConnectionSerialize 
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Connection 
 *          header into it.
 * Return Value: RV_TRUE - Serialization successfull, negative values on 
 *               failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Connection header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 **************************************************************************/
RvBool RtspHeadersConnectionSerialize(
                        IN  HRPOOL                   hRPool,
                        IN  RvRtspConnectionHeader   *pHeader,
                        OUT HRPOOLELEM               *phElement)
{
    RvBool addDelimiter = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    if (pHeader->additionalFields != NULL)
        addDelimiter = RV_FALSE;
#endif
    return RtspHeadersStrSerialize(hRPool, "Connection: ", pHeader->hStr, addDelimiter, phElement);
}



/**************************************************************************
 * RtspHeadersCSeqSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the CSeq header into it.
 *
 * Return Value:    RV_TRUE - Serialization successfull, negative values on
 *                  failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the CSeq header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 *******************************************************************/
RvBool RtspHeadersCSeqSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvRtspCSeqHeader    *pHeader,
                        OUT HRPOOLELEM          *phElement)
{
    RvChar      strCSeq[50];
    RvSize_t    headerLength;

    /* checking parameters */
    if ( (hRPool == NULL) || (pHeader == NULL) )
        return RV_FALSE;

    sprintf(strCSeq, "CSeq: %d\r\n", pHeader->value);

    headerLength = strlen(strCSeq);

    /* allocate a suitable space in hElement */
    *phElement = rpoolAlloc(hRPool, (RvInt32)headerLength);

    /* copy the header name */
    rpoolCopyFromExternal(  hRPool,
                            *phElement,
                            strCSeq,
                            0,
                            (RvInt32)headerLength);
    return  RV_TRUE;
}

/*add by kongfd*/
/**************************************************************************
 * RtspHeadersAuthorizationSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Authorization header
 *          into it.
 *
 * Return Value: RV_TRUE - Serialization successfull, negative values on failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Accept header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 *******************************************************************/
RvBool RtspHeadersAuthSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvRtspAuthHeader  *pHeader,
                        OUT HRPOOLELEM          *phElement)
{
    RvBool addDelimiter = RV_TRUE;

    return RtspHeadersStrSerialize(hRPool, "Authorization: ", pHeader->hStr, addDelimiter, phElement);
}
/*add end */
/**************************************************************************
 * RtspHeadersAcceptSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Accept header
 *          into it.
 *
 * Return Value: RV_TRUE - Serialization successfull, negative values on failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Accept header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 *******************************************************************/
RvBool RtspHeadersAcceptSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvRtspAcceptHeader  *pHeader,
                        OUT HRPOOLELEM          *phElement)
{
    RvBool addDelimiter = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    if (pHeader->additionalFields != NULL)
        addDelimiter = RV_FALSE;
#endif
    return RtspHeadersStrSerialize(hRPool, "Accept: ", pHeader->hStr, addDelimiter, phElement);

}

/**************************************************************************
 * RtspHeadersSessionSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Session header into it.
 *
 * Return Value: RV_TRUE - Serialization successfull, negative values on failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Session header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 *******************************************************************/
RvBool RtspHeadersSessionSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvRtspSessionHeader *pHeader,
                        OUT HRPOOLELEM          *phElement)
{
    RvBool addDelimiter = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    if (pHeader->additionalFields != NULL)
        addDelimiter = RV_FALSE;
#endif
    return RtspHeadersStrSerialize(hRPool, "Session: ", pHeader->hSessionId, addDelimiter, phElement);
}


/**************************************************************************
 * RtspHeadersRequireSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Require header
 *          into it.
 * Return Value: RV_TRUE - Serialization successful, negative values on 
 *               failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Require header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 *************************************************************************/
RvBool RtspHeadersRequireSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvRtspRequireHeader *pHeader,
                        OUT HRPOOLELEM          *phElement)
{
    RvBool addDelimiter = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    if (pHeader->additionalFields != NULL)
        addDelimiter = RV_FALSE;
#endif
    return RtspHeadersStrSerialize(hRPool, "Require: ", pHeader->hStr, addDelimiter, phElement);

}


/**************************************************************************
 * RtspHeadersLocationSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Location header
 *          into it.
 * Return Value: RV_TRUE - Serialization successful, negative values on 
 *               failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Location header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 *************************************************************************/
RvBool RtspHeadersLocationSerialize(
                        IN  HRPOOL                hRPool,
                        IN  RvRtspLocationHeader  *pHeader,
                        OUT HRPOOLELEM            *phElement)
{
    RvBool addDelimiter = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    if (pHeader->additionalFields != NULL)
        addDelimiter = RV_FALSE;
#endif
    return RtspHeadersStrSerialize(hRPool, "Location: ", pHeader->hStr, addDelimiter, phElement);

}


/**************************************************************************
 * RtspPublicSerialize
 * ------------------------------------------------------------------------
 * General: Serializes the given RvRtspPublicHeader structure into the RPool
 *          element.
 *
 * Return Value:    RV_OK if request line is serialized successfully,
 *                  negative values if failed
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's RPool memory pool.
 *              pPublic     - the public header to serialize.
 * OUTPUT   :   pHPublic    - the element to serialize into.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspPublicSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvRtspPublicHeader  *pPublic,
                        OUT HRPOOLELEM          *pHPublic)
{
    RvSize_t    headerLength;
    RvSize_t    elementLength;
    RvSize_t    elementOffset = 0;
    RvChar      *strInfo = "Public:";
    
    /* checking parameters */
    if ( (hRPool == NULL) || (pPublic== NULL) || (pHPublic== NULL) )
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    /* allocating the space for the status line */
    headerLength = RtspUtilsRPOOLELEMStrLen(hRPool,(HRPOOLELEM)pPublic->hStr,0);
                                                /* returns the length of phrase string */

    elementLength = strlen(strInfo) +           /* "PUBLIC:"        */
                    headerLength +              /* public header    */
                    2;                          /* "\r\n"           */
  
    if (headerLength)
    {
        elementLength += 1;                     /* " " before the phrase */
    }
    
    *pHPublic= rpoolAlloc(hRPool, (RvInt32)elementLength);

    if (*pHPublic == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
    }
    
    /* serializing the header information */
    rpoolCopyFromExternal(  hRPool,
                            *pHPublic,
                            strInfo,
                            (RvInt32)elementOffset,
                            (RvInt32)strlen(strInfo));
    elementOffset += strlen(strInfo);

    
    /* serializing the phrase */
    if (headerLength > 0)
    {
        /* serializing " " */
        rpoolCopyFromExternal(  hRPool,
                                *pHPublic,
                                " ",
                                (RvInt32)elementOffset,
                                1);
        elementOffset++;

        RtspUtilsRPOOLELEMCopyInternal( hRPool,
                                        *pHPublic,
                                        elementOffset,
                                        (HRPOOLELEM)pPublic->hStr,
                                        0,
                                        headerLength);

        elementOffset += headerLength;
    }

    /* serializing the "\r\n" */
    rpoolCopyFromExternal(  hRPool,
                            *pHPublic,
                            "\r\n",
                            (RvInt32)elementOffset,
                            2);
    elementOffset += 2;

    return RV_OK;
}



/**************************************************************************
 * RtspHeadersRangeSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Range header into it.
 *
 * Return Value: RV_TRUE - Serialization successfull, negative values on failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Range header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 *******************************************************************/
RvBool RtspHeadersRangeSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvRtspRangeHeader   *pHeader,
                        OUT HRPOOLELEM          *phElement)
{

    RvChar      strStartTimeBuffer[20];
    RvChar      strEndTimeBuffer[20];
    RvChar      strTimeBuffer[100];

    /* checking parameters */
    if ( (hRPool == NULL) || (pHeader == NULL) )
        return RV_FALSE;

    /* header format: "Range: startTime-endTime\r\n" */

    if  ( (pHeader->startTime.format == RV_RTSP_NPT_FORMAT_NOT_EXISTS) &&
          (pHeader->endTime.format == RV_RTSP_NPT_FORMAT_NOT_EXISTS) )
    {
        /* if none of the range times exist, the header is empty, and will not be printed. */
        *phElement = NULL;
        return RV_TRUE;
    }

    RtspHeadersNptTimeSerialize(&pHeader->startTime, strStartTimeBuffer);
    RtspHeadersNptTimeSerialize(&pHeader->endTime, strEndTimeBuffer);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    if (pHeader->additionalFields != NULL)
        sprintf(strTimeBuffer, "Range: npt=%s-%s", strStartTimeBuffer, strEndTimeBuffer);
    else
#endif
        sprintf(strTimeBuffer, "Range: npt=%s-%s\r\n", strStartTimeBuffer, strEndTimeBuffer);

    /* allocate a suitable space in hElement */
    *phElement = rpoolAlloc(hRPool, (RvInt32)strlen(strTimeBuffer));

    /* copy the header name */
    rpoolCopyFromExternal(  hRPool,
                            *phElement,
                            strTimeBuffer,
                            0,
                            (RvInt32)strlen(strTimeBuffer));

    return  RV_TRUE;
}



/**************************************************************************
 * RtspHeadersTransportSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Transport header
 *          into it.
 *
 * Return Value:    RV_TRUE - Serialization successfull, negative values on
 *                  failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Transport header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 *******************************************************************/
RvBool RtspHeadersTransportSerialize(
                        IN  HRPOOL                  hRPool,
                        IN  RvRtspTransportHeader   *pHeader,
                        OUT HRPOOLELEM              *phElement)
{
    RvChar      strTransport[300];
    RvChar      *pStr = &strTransport[0];

	/* checking parameters */
	if ( (hRPool == NULL) || (pHeader == NULL) )
		return RV_FALSE;

    memset((void *)strTransport, 0, sizeof(strTransport));

	/* header format: "Transport: RTP/AVP/UDP;<unicast/multicast>;[destination=<address>];client_port=<clientPortA>-<clientPortB>\r\n" */
    sprintf(pStr, "Transport: RTP/AVP/UDP");
    while (*pStr != 0)
        pStr++;
	if (pHeader->isUnicast == RV_TRUE)
	{
        sprintf(pStr, ";unicast");
        while (*pStr != 0)
            pStr++;
    }
    else
    {
        sprintf(pStr, ";multicast");
        while (*pStr != 0)
            pStr++;
    }
    if (pHeader->destination[0] != 0)
    {
        sprintf(pStr, ";destination=%s", pHeader->destination);
        while (*pStr != 0)
            pStr++;
    }
    if (pHeader->clientPortA != 0)
    {
        sprintf(pStr, ";client_port=%d-%d", pHeader->clientPortA, pHeader->clientPortB);
        while (*pStr != 0)
            pStr++;
    }
    if (pHeader->serverPortA != 0)
    {
        sprintf(pStr, ";server_port=%d-%d",pHeader->serverPortA,pHeader->serverPortB);
        while (*pStr !=0)
            pStr++;
    }

#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    if (pHeader->additionalFields == NULL)
#endif
        sprintf(pStr, "\r\n");
    /* allocate a suitable space in hElement */
    *phElement = rpoolAlloc(hRPool, (RvInt32)strlen(strTransport));

    /* copy the header name */
    rpoolCopyFromExternal(  hRPool,
                            *phElement,
                            strTransport,
                            0,
                            (RvInt32)strlen(strTransport));
    return  RV_TRUE;
}

/**************************************************************************
 * RtspHeadersRtpInfoSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the RTP-Info header
 *          into it.
 *
 * Return Value: RV_TRUE if Serialization successful, negative values on
 *               failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Rtp Info header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 **************************************************************************/
RvBool RtspHeadersRtpInfoSerialize(
                        IN  HRPOOL                 hRPool,
                        IN  RvRtspRtpInfoHeader    *pHeader,
                        OUT HRPOOLELEM             *phElement)
{
    RvChar         strRtpInfo[300];
    RvChar         *pStr = &strRtpInfo[0];
    RvInt32        location;
    RvRtspRtpInfo  *pRtpInfo;
    //RvSize_t       headerLength;
    RvChar         URIBuff[100];

    /* TODO - check why this line is needed */
    //headerLength = RtspUtilsRPOOLELEMStrLen(hRPool,(HRPOOLELEM)pHeader->hInfo,0);
    /* checking parameters */
    if ( (hRPool == NULL) || (pHeader == NULL) )
        return RV_FALSE;

    memset((void *)strRtpInfo, 0, sizeof(strRtpInfo));
    
    sprintf(pStr, "RTP-Info: ");
    while (*pStr != 0)
        pStr++;
    
    
    location = raGetNext((HRA)pHeader->hInfo,-1);
    while(location>=0)    
    {
        pRtpInfo = (RvRtspRtpInfo *)raGet((HRA)pHeader->hInfo, location);
 
        sprintf(pStr,"url=");
        while (*pStr != 0)
            pStr++;
 
        RtspUtilsHPOOLELEMStrCpy(hRPool, (HRPOOLELEM)pRtpInfo->hURI, 0, URIBuff, 100); 
 
        sprintf(pStr,"%s",URIBuff);
        while (*pStr != 0)
            pStr++;
        
        if(pRtpInfo->seqValid == RV_TRUE)
        {
            sprintf(pStr,";seq=%d",pRtpInfo->seq);
            while (*pStr != 0)
                pStr++;
        }
        
        if(pRtpInfo->rtpTimeValid == RV_TRUE)
        {
            sprintf(pStr,";rtptime=%d",pRtpInfo->rtpTime);
            while (*pStr != 0)
                pStr++;
        }

        location = raGetNext( (HRA)pHeader->hInfo, location);
        if(location>=0)
        {
            sprintf(pStr,",");
            pStr++;
        }
    }

    sprintf(pStr, "\r\n");

    /* allocate a suitable space in hElement */
    *phElement = rpoolAlloc(hRPool, (RvInt32)strlen(strRtpInfo));

    /* copy the header name */
    rpoolCopyFromExternal(  hRPool,
                            *phElement,
                            strRtpInfo,
                            0,
                            (RvInt32)strlen(strRtpInfo));
    return  RV_TRUE;
}


/**************************************************************************
 * RtspContentBaseSerialize
 * ------------------------------------------------------------------------
 * General: Serializes the given RvRtspContentBaseHeader structure into the
 *          RPool element.
 *
 * Return Value:    RV_OK if request line is serialized successfully,
 *                  negative values if failed
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool        - the module's RPool memory pool.
 *              pContentBase  - the content base header to serialize.
 * OUTPUT   :   pHContentBase - the element to serialize into.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspContentBaseSerialize(
                        IN  HRPOOL                   hRPool,
                        IN  RvRtspContentBaseHeader  *pContentBase,
                        OUT HRPOOLELEM               *pHContentBase)
{
    RvSize_t    baseLength;
    RvSize_t    elementLength;
    RvSize_t    elementOffset = 0;
   
    RvChar      *strInfo = "Content-Base:";

    /* checking parameters */
    if ( (hRPool == NULL) || (pContentBase== NULL) || (pHContentBase== NULL) )
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    /* allocating the space for the content base */
    baseLength = RtspUtilsRPOOLELEMStrLen(hRPool,(HRPOOLELEM)pContentBase->hStr,0);
                                                /* returns the length of phrase string */

    elementLength = strlen(strInfo) +           /* "Content-Base: "    */
                    baseLength +                /* Content-Base header */
                    2;                          /* "\r\n"              */
  
    if (baseLength)
    {
        elementLength += 1;                     /* " " before the phrase */
    }
    
    *pHContentBase= rpoolAlloc(hRPool, (RvInt32)elementLength);

    if (*pHContentBase == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
    }
    
    /* serializing the header information */
    rpoolCopyFromExternal(  hRPool,
                            *pHContentBase,
                            strInfo,
                            (RvInt32)elementOffset,
                            (RvInt32)strlen(strInfo));
    elementOffset += strlen(strInfo);

    
    /* serializing the phrase */
    if (baseLength > 0)
    {
        /* serializing " " */
        rpoolCopyFromExternal(  hRPool,
                                *pHContentBase,
                                " ",
                                (RvInt32)elementOffset,
                                1);
        elementOffset++;

        RtspUtilsRPOOLELEMCopyInternal( hRPool,
                                        *pHContentBase,
                                        elementOffset,
                                        (HRPOOLELEM)pContentBase->hStr,
                                        0,
                                        baseLength);

        elementOffset += baseLength;
    }

    /* serializing the "\r\n" */
    rpoolCopyFromExternal(  hRPool,
                            *pHContentBase,
                            "\r\n",
                            (RvInt32)elementOffset,
                            2);
    elementOffset += 2;

    return RV_OK;
}

/**************************************************************************
 * RtspContentTypeSerialize 
 * ------------------------------------------------------------------------
 * General: Serializes the given RvRtspContentTypeHeader structure into the
 *          RPool element.
 *
 * Return Value:    RV_OK if request line is serialized successfully,
 *                  negative values if failed
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool        - the module's RPool memory pool.
 *              pContentType  - the content type header to serialize.
 * OUTPUT   :   pHContentType - the element to serialize into.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspContentTypeSerialize(
                        IN  HRPOOL                   hRPool,
                        IN  RvRtspContentTypeHeader  *pContentType,
                        OUT HRPOOLELEM               *pHContentType)
{
    RvSize_t    typeLength;
    RvSize_t    elementLength;
    RvSize_t    elementOffset = 0;
   
    RvChar *strInfo = "Content-Type:";

    /* checking parameters */
    if ( (hRPool == NULL) || (pContentType== NULL) || (pHContentType== NULL) )
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    /* allocating the space for the content type */
    typeLength = RtspUtilsRPOOLELEMStrLen(hRPool,(HRPOOLELEM)pContentType->hStr,0);
                                                /* returns the length of phrase string */

    elementLength = strlen(strInfo) +           /* "Content-Type: "    */
                    typeLength +                /* Content-Type header */
                    2;                          /* "\r\n"              */
  
    if (typeLength)
    {
          elementLength += 1;                     /* " " before the phrase */
    }
    
    *pHContentType= rpoolAlloc(hRPool, (RvInt32)elementLength);

    if (*pHContentType == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
    }
    
    /* serializing the header information */
    rpoolCopyFromExternal(  hRPool,
                            *pHContentType,
                            strInfo,
                            (RvInt32)elementOffset,
                            (RvInt32)strlen(strInfo));
    elementOffset += strlen(strInfo);

    
    /* serializing the phrase */
    if (typeLength > 0)
    {
        /* serializing " " */
        rpoolCopyFromExternal(  hRPool,
                                *pHContentType,
                                " ",
                                (RvInt32)elementOffset,
                                1);
        elementOffset++;

        RtspUtilsRPOOLELEMCopyInternal( hRPool,
                                        *pHContentType,
                                        elementOffset,
                                        (HRPOOLELEM)pContentType->hStr,
                                        0,
                                        typeLength);

        elementOffset += typeLength;
    }

    /* serializing the "\r\n" */
    rpoolCopyFromExternal(  hRPool,
                            *pHContentType,
                            "\r\n",
                            (RvInt32)elementOffset,
                            2);
    elementOffset += 2;

    return RV_OK;
}

/**************************************************************************
 * RtspContentLengthSerialize 
 * ------------------------------------------------------------------------
 * General: Serializes the given RvRtspContentLengthHeader structure into the
 *          RPool element.
 *
 * Return Value:    RV_OK if request line is serialized successfully,
 *                  negative values if failed
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool           - the module's RPool memory pool.
 *              pContentLength   - the Content-Length header to serialize.
 * OUTPUT   :   pHContentLength  - the element to serialize into.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspContentLengthSerialize(
                        IN  HRPOOL                     hRPool,
                        IN  RvRtspContentLengthHeader  *pContentLength,
                        OUT HRPOOLELEM                 *pHContentLength)
{

    RvSize_t    elementLength;
    RvSize_t    elementOffset = 0;

    RvChar      strContentLength[sizeof(RvUint16)+1];
    RvChar      *strInfo = "Content-Length:";
    
    /* checking parameters */
    if ( (hRPool == NULL) || (pContentLength== NULL) || (pHContentLength== NULL) )
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    sprintf(strContentLength, "%d", pContentLength->value);

    elementLength = strlen(strInfo) +             /* "Content-Length: "    */
                    1 +                           /* ""                    */
                    strlen(strContentLength) +    /* content-length header */                 
                    2+2;                          /* "\r\n"                */
  

    *pHContentLength= rpoolAlloc(hRPool, (RvInt32)elementLength);

    if (*pHContentLength == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
    }
    
    /* serializing the header information */
    rpoolCopyFromExternal(  hRPool,
                            *pHContentLength,
                            strInfo,
                            (RvInt32)elementOffset,
                            (RvInt32)strlen(strInfo));
    elementOffset += strlen(strInfo);

    /* "" */
    rpoolCopyFromExternal(  hRPool,
                                *pHContentLength,
                                " ",
                                (RvInt32)elementOffset,
                                1);
    elementOffset++;
    
    rpoolCopyFromExternal(  hRPool,
                            *pHContentLength,
                            strContentLength,
                            (RvInt32)elementOffset,
                            (RvInt32)strlen(strContentLength));
    elementOffset += strlen(strContentLength);

    /* serializing the "\r\n" */
    rpoolCopyFromExternal(  hRPool,
                            *pHContentLength,
                            "\r\n",
                            (RvInt32)elementOffset,
                            2);
    elementOffset += 2;

    /* serializing the "\r\n" */
    rpoolCopyFromExternal(  hRPool,
                            *pHContentLength,
                            "\r\n",
                            (RvInt32)elementOffset,
                            2);
    elementOffset += 2;

    return RV_OK;
}


/**************************************************************************
 * RtspHeadersContentLanguageSerialize 
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Content-Language
 *          header into it.
 * Return Value: RV_TRUE - Serialization successfull, negative values on 
 *               failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Content-Language header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 **************************************************************************/
RvBool RtspHeadersContentLanguageSerialize(
                        IN  HRPOOL                      hRPool,
                        IN  RvRtspContentLanguageHeader *pHeader,
                        OUT HRPOOLELEM                  *phElement)
{
    RvBool addDelimiter = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    if (pHeader->additionalFields != NULL)
        addDelimiter = RV_FALSE;
#endif
    return RtspHeadersStrSerialize(hRPool, "Content-Language: ", pHeader->hStr, addDelimiter, phElement);
}



/**************************************************************************
 * RtspHeadersContentEncodingSerialize 
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Content-Encoding
 *          header into it.
 * Return Value: RV_TRUE - Serialization successfull, negative values on 
 *               failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Content-Encoding header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 **************************************************************************/
RvBool RtspHeadersContentEncodingSerialize(
                        IN  HRPOOL                       hRPool,
                        IN  RvRtspContentEncodingHeader  *pHeader,
                        OUT HRPOOLELEM                   *phElement)
{
    RvBool addDelimiter = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    if (pHeader->additionalFields != NULL)
        addDelimiter = RV_FALSE;
#endif
    return RtspHeadersStrSerialize(hRPool, "Content-Encoding: ", pHeader->hStr, addDelimiter, phElement);
}



/* de-serialization */


/******************************************************************************
 * RtspHeadersCSeqDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Cseq header line into the given RvRtspCSeqHeader
 *          structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersCSeqDeserialize(
                        IN  HRPOOL              hRPool,
                        IN  HRPOOLELEM          hLine,
                        IN  RvSize_t            lineOffset,
                        OUT RvRtspCSeqHeader    *pHeader)
{
    RvChar      token[50];
    RvSize_t    tokenLength;
    RvInt       rVal;


    if ( (hRPool == NULL) || (hLine == NULL) || (pHeader == NULL) )
        return RV_FALSE;

    RtspUtilsRPOOLELEMGetToken( hRPool,
                                hLine,
                                lineOffset,
                                " \r\n",
                                RV_FALSE,
                                token,
                                40,
                                &tokenLength);
    token[tokenLength] = '\0';
    if (sscanf(token,"%d", &rVal) == 1)
        pHeader->value = (RvUint16)rVal;
    return RV_TRUE;
}


/******************************************************************************
 * RtspHeadersAcceptDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Accept header line into the given
 *          RvRtspConnectionHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                            header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersAcceptDeserialize(
                        IN  HRPOOL                  hRPool,
                        IN  HRPOOLELEM              hLine,
                        IN  RvSize_t                lineOffset,
                        OUT RvRtspAcceptHeader  *pHeader)
{
    if (pHeader == NULL)
        return RV_FALSE;

    return RtspHeadersStrDeserialize(hRPool, hLine, lineOffset, "\r\n", &pHeader->hStr);
}

/*add by kongfd*/
/******************************************************************************
 * RtspHeadersAuthorizationDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Authorization header line into the given
 *          RvRtspConnectionHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                            header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersAuthorizationDeserialize(
                        IN  HRPOOL                  hRPool,
                        IN  HRPOOLELEM              hLine,
                        IN  RvSize_t                lineOffset,
                        OUT RvRtspAuthHeader  *pHeader)
{
    if (pHeader == NULL)
        return RV_FALSE;

    return RtspHeadersStrDeserialize(hRPool, hLine, lineOffset, "\r\n", &pHeader->hStr);
}

/*add end*/
/******************************************************************************
 * RtspHeadersConnectionDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Connection header line into the given
 *          RvRtspConnectionHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersConnectionDeserialize(
                        IN  HRPOOL                  hRPool,
                        IN  HRPOOLELEM              hLine,
                        IN  RvSize_t                lineOffset,
                        OUT RvRtspConnectionHeader  *pHeader)
{
    if (pHeader == NULL)
        return RV_FALSE;

    return RtspHeadersStrDeserialize(hRPool, hLine, lineOffset, "\r\n", &pHeader->hStr);
}



/******************************************************************************
 * RtspHeadersSessionDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Session header line into the given
 *          RvRtspSessionHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersSessionDeserialize(
                        IN  HRPOOL              hRPool,
                        IN  HRPOOLELEM          hLine,
                        IN  RvSize_t            lineOffset,
                        OUT RvRtspSessionHeader *pHeader)
{
    if (pHeader == NULL)
        return RV_FALSE;

    return RtspHeadersStrDeserialize(hRPool, hLine, lineOffset, " ;\r\n", &pHeader->hSessionId);
}



/******************************************************************************
 * RtspHeadersRangeDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Session header line into the given
 *          RvRtspRangeHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersRangeDeserialize(
                        IN  HRPOOL                  hRPool,
                        IN  HRPOOLELEM              hLine,
                        IN  RvSize_t                lineOffset,
                        OUT RvRtspRangeHeader       *pHeader)
{
    RvChar      token[100];
    RvSize_t    tokenLength;
    RvInt   stTimehrs;
    RvInt   stTimemnt;
    RvInt   stTimesec;
    RvInt   edTimehrs;
    RvInt   edTimemnt;
    RvInt   edTimesec;


    if ( (hRPool == NULL) || (hLine == NULL) || (pHeader == NULL) )
        return RV_FALSE;  

    RtspUtilsRPOOLELEMGetToken(hRPool,hLine,lineOffset," \r\n",RV_FALSE,token,40,&tokenLength);
    token[tokenLength] = '\0';

    if(strstr(token,"npt")) 
    {
       if(sscanf(token,"npt=%d.%d%d-%d.%d%d",&stTimehrs,&stTimemnt,&stTimesec,&edTimehrs,&edTimemnt,&edTimesec)==6)
       {
            pHeader->startTime.hours   = (RvUint8)stTimehrs;
            pHeader->startTime.minutes = (RvUint8)stTimemnt;
            pHeader->startTime.seconds = (double) stTimesec;
            pHeader->endTime.hours     = (RvUint8)edTimehrs;
            pHeader->endTime.minutes   = (RvUint8)edTimemnt;
            pHeader->endTime.seconds   = (double) stTimesec;
       }
    }
    return RV_TRUE;
    
}
/******************************************************************************
 * RtspHeadersTransportDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Session header line into the given
 *          RvRtspTransportHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersTransportDeserialize(
                        IN  HRPOOL                  hRPool,
                        IN  HRPOOLELEM              hLine,
                        IN  RvSize_t                lineOffset,
                        OUT RvRtspTransportHeader   *pHeader)
{
    RvChar      token[100];
    RvSize_t    tokenLength;
    RvSize_t    offset;
    RvBool      success = RV_TRUE;

    if ( (hRPool == NULL) || (hLine == NULL) || (pHeader == NULL) )
        return RV_FALSE;

    /* default values */
    pHeader->isUnicast      = RV_FALSE;
    pHeader->serverPortA    = 0;
    pHeader->serverPortB    = 0;
    pHeader->clientPortA    = 0;
    pHeader->clientPortB    = 0;
    memset((void *)(pHeader->destination), 0, sizeof(pHeader->destination));

    /* releveant header field: unicast/multicast, client_port */
    success = RtspUtilsRPOOLELEMStrChr(hRPool, hLine, lineOffset, ';', &offset);
    lineOffset = offset+1;

    while (success)
    {
        RvInt rValA, rValB;
        /* get the parameters*/
        success = RtspUtilsRPOOLELEMGetToken(hRPool, hLine, lineOffset, ";\r\n ", RV_FALSE,
            token, 90, &tokenLength);
        token[tokenLength] = '\0';

        if (strstr(token,"unicast"))
        {
            pHeader->isUnicast = RV_TRUE;
        }
        else if (strstr(token, "destination"))
        {
            RvChar destination[50];
            memset(destination, 0, sizeof(destination));
            sscanf(token, "destination=%s", destination);
            memcpy(pHeader->destination, destination, sizeof(pHeader->destination) - 1);
        }
        else if (strstr(token,"client_port"))
        {
            if (sscanf(token,"client_port=%d-%d",&rValA, &rValB) == 2)
            {
                pHeader->clientPortA = (RvUint16)rValA;
                pHeader->clientPortB = (RvUint16)rValB;
            }
        }
        else if (strstr(token,"server_port"))
        {
            if (sscanf(token,"server_port=%d-%d",&rValA, &rValB) == 2)
            {
                pHeader->serverPortA = (RvUint16)rValA;
                pHeader->serverPortB = (RvUint16)rValB;
            }
        }
        success = RtspUtilsRPOOLELEMStrChr(hRPool, hLine, lineOffset, ';', &offset);
        lineOffset = offset+1;
    } /* while(success) */

    return RV_TRUE;
}

//begin : added by lichao, 20140814 transport字段增加额外fields的解析
/******************************************************************************
 * RtspHeadersTransportWithFieldsDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Session header line into the given
 *			RvRtspTransportHeader structure with additionalFields.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	
 *              hRtsp       - the handle of Rtsp
 *              hRtspMsgMessage -the handle of response message
 *              hRPool		- the module's memory pool.
 *				hLine		- the header line element.
 *				lineOffset	- the offset into the line from which the
 *							header content starts.
 * OUTPUT	:	pHeader		- the header structure to deserialize into.
 * INOUT	:	None.
 *****************************************************************************/
RvBool RtspHeadersTransportWithFieldsDeserialize(
                                       IN  RvRtspHandle            hRtsp,
                                       IN  RvRtspMsgMessageHandle  hRtspMsgMessage,
                                       IN  HRPOOL                  hRPool,
                                       IN  HRPOOLELEM              hLine,
                                       IN  RvSize_t                lineOffset,
                                       OUT RvRtspTransportHeader   *pHeader)
{
    RvChar      token[100];
    RvSize_t    tokenLength;
    RvSize_t    offset;
    RvBool      success = RV_TRUE;
    RvRtspMsgAppHeader msgHeader;
    RvChar *field = token;

    if ( (hRPool == NULL) || (hLine == NULL) || (pHeader == NULL) )
        return RV_FALSE;

    /* default values */
    pHeader->isUnicast      = RV_FALSE;
    pHeader->serverPortA    = 0;
    pHeader->serverPortB    = 0;
    pHeader->clientPortA    = 0;
    pHeader->clientPortB    = 0;
    memset((void *)(pHeader->destination), 0, sizeof(pHeader->destination));

    /* releveant header field: unicast/multicast, client_port */
    success = RtspUtilsRPOOLELEMStrChr(hRPool, hLine, lineOffset, ';', &offset);
    lineOffset = offset+1;

    while (success)
    {
        RvInt rValA, rValB;
        /* get the parameters*/
        success = RtspUtilsRPOOLELEMGetToken(hRPool, hLine, lineOffset, ";\r\n ", RV_FALSE,
            token, 90, &tokenLength);
        token[tokenLength] = '\0';

        if (strstr(token,"unicast"))
        {
            pHeader->isUnicast = RV_TRUE;
        }
        else if (strstr(token, "destination"))
        {
            RvChar destination[50];
            memset(destination, 0, sizeof(destination));
            sscanf(token, "destination=%s", destination);
            memcpy(pHeader->destination, destination, sizeof(pHeader->destination) - 1);
        }
        else if (strstr(token,"client_port"))
        {
            if (sscanf(token,"client_port=%d-%d",&rValA, &rValB) == 2)
            {
                pHeader->clientPortA = (RvUint16)rValA;
                pHeader->clientPortB = (RvUint16)rValB;
            }
        }
        else if (strstr(token,"server_port"))
        {
            if (sscanf(token,"server_port=%d-%d",&rValA, &rValB) == 2)
            {
                pHeader->serverPortA = (RvUint16)rValA;
                pHeader->serverPortB = (RvUint16)rValB;
            }
        }
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        else
        {
            memset(&msgHeader, 0, sizeof(RvRtspMsgAppHeader));
            msgHeader.bIsRequest = RV_FALSE;
            msgHeader.hRtspMsgMessage = hRtspMsgMessage;
            msgHeader.delimiter = ';';

            msgHeader.headerFields = &field;
            msgHeader.headerFieldsSize = 1;
            msgHeader.headerFieldStrLen = tokenLength;

            if (NULL == pHeader->additionalFields)
            {
                RvRtspMsgAddHeaderFields(hRtsp, &msgHeader, &pHeader->additionalFields);
            }
            else
            {
                RvRtspMsgAddGenericHeaderFields(hRtsp, pHeader->additionalFields, &msgHeader);
            }
        }
#endif
        success = RtspUtilsRPOOLELEMStrChr(hRPool, hLine, lineOffset, ';', &offset);
        lineOffset = offset+1;
    } /* while(success) */

    return RV_TRUE;
}
//end : added by lichao, 20140814

/******************************************************************************
 * RtspHeadersContentLanguageDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Content-Language header line into the given
 *          RvRtspContentLanguageHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersContentLanguageDeserialize(
                        IN  HRPOOL                      hRPool,
                        IN  HRPOOLELEM                  hLine,
                        IN  RvSize_t                    lineOffset,
                        OUT RvRtspContentLanguageHeader *pHeader)
{
    if (pHeader == NULL)
        return RV_FALSE;

    return RtspHeadersStrDeserialize(hRPool, hLine, lineOffset, "\r\n", &pHeader->hStr);
}



/******************************************************************************
 * RtspHeadersContentEncodingDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Content-Encoding header line into the given
 *          RvRtspContentEncodingHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersContentEncodingDeserialize(
                        IN  HRPOOL                      hRPool,
                        IN  HRPOOLELEM                  hLine,
                        IN  RvSize_t                    lineOffset,
                        OUT RvRtspContentEncodingHeader *pHeader)
{
    if (pHeader == NULL)
        return RV_FALSE;

    return RtspHeadersStrDeserialize(hRPool, hLine, lineOffset, "\r\n", &pHeader->hStr);
}



/******************************************************************************
 * RtspHeadersContentLengthDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Content-Length header line into the given
 *          RvRtspContentLengthHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to de-serialize into.
 * INOUT    :   None.
 *****************************************************************************/
RVINTAPI RvBool RVCALLCONV RtspHeadersContentLengthDeserialize(
                        IN  HRPOOL                      hRPool,
                        IN  HRPOOLELEM                  hLine,
                        IN  RvSize_t                    lineOffset,
                        OUT RvRtspContentLengthHeader   *pHeader)
{
    RvChar      token[50];
    RvSize_t    tokenLength;
    RvInt       rVal;

    if ( (hRPool == NULL) || (hLine == NULL) || (pHeader == NULL) )
        return RV_FALSE;

    RtspUtilsRPOOLELEMGetToken(hRPool, hLine, lineOffset, "\r", RV_FALSE, token, 40, &tokenLength);
    token[tokenLength] = '\0';
    if (sscanf(token,"%d", &rVal) == 1)
        pHeader->value = (RvUint16)rVal;
    return RV_TRUE;
}

/******************************************************************************
 * RtspHeadersContentTypeDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Content-Type header line into the given
 *          RvRtspContentTypeHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersContentTypeDeserialize(
                        IN  HRPOOL                  hRPool,
                        IN  HRPOOLELEM              hLine,
                        IN  RvSize_t                lineOffset,
                        OUT RvRtspContentTypeHeader *pHeader)
{
    if (pHeader == NULL)
        return RV_FALSE;

    return RtspHeadersStrDeserialize(hRPool, hLine, lineOffset, "\r\n", &pHeader->hStr);
}

/******************************************************************************
 * RtspHeadersLocationDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Location header line into the given
 *          RvRtspContentTypeHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeadersLocationDeserialize(
                        IN  HRPOOL                  hRPool,
                        IN  HRPOOLELEM              hLine,
                        IN  RvSize_t                lineOffset,
                        OUT RvRtspLocationHeader    *pHeader)
{
    if (pHeader == NULL)
        return RV_FALSE;

    return RtspHeadersStrDeserialize(hRPool, hLine, lineOffset, "\r\n", &pHeader->hStr);
}

/**************************************************************************
 * RtspHeadersRtpInfoDeserialize
 * ------------------------------------------------------------------------
 * General: De-serializes an RTP-Info header line from the given element
 *          into the given RvRtspRtpInfoHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool              - The module's memory pool.
 *              hLine               - The header line element.
 *              lineOffset          - The offset into the line from which
 *                                  the header content starts.
 *              maxRtpInfoStreams   - The maximum number of streams to
 *                                  get from an RTP-Info header.
 * OUTPUT   :   pHeader             - the header structure to deserialize
 *                                  into.
 * INOUT    :   None.
 *******************************************************************/
RvBool RtspHeadersRtpInfoDeserialize(
                        IN  HRPOOL              hRPool,
                        IN  HRPOOLELEM          hLine,
                        IN  RvSize_t            lineOffset,
                        IN  RvUint16            maxRtpInfoStreams,
                        OUT RvRtspRtpInfoHeader *pHeader)
{
    RAElement       hRAElement;
    RvRtspRtpInfo   *pRtpInfo;
    RvInt32         result;
    RvBool          cont = RV_TRUE;

    if ( (hRPool == NULL) || (hLine == NULL) || (pHeader == NULL) )
        return RV_FALSE;

    pHeader->hInfo = (RvRtspArrayHandle)raConstruct(
            sizeof(RvRtspRtpInfo),  /* Element size           */
            maxRtpInfoStreams,      /* Max number of elements */
            RV_TRUE,                /* ThreadSafety           */
            "RTP-Info",             /* RA name                */
            NULL);                  /* Log manager            */

    while (cont)
    {
        /* allocate and initialize the RtpInfo object */
        result = raAdd((HRA)pHeader->hInfo, &hRAElement);

        if (result < 0)
            return RV_FALSE;

        pRtpInfo = (RvRtspRtpInfo*) hRAElement;
        RtspHeadersRtpInfoLineDeserialize(hRPool, hLine, lineOffset, pRtpInfo);

        cont = RtspUtilsRPOOLELEMStrChr(hRPool,
                                        hLine,
                                        lineOffset,
                                        ',',
                                        &lineOffset); 
        if (cont == RV_FALSE)
            return RV_TRUE;

        lineOffset++;
    }

    return RV_TRUE;
}


/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/



/**************************************************************************
 * RtspHeadersStrSerialize
 * ------------------------------------------------------------------------
 * General: serializes a header containing a string into an rpool element
 *          containing the header name and value.
 *
 * Return Value: RV_TRUE if serialization is successfull, negative values if not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool          - The module's memory pool.
 *              strPrefix       - The prefix to write at the start of the element
 *              hStr            - The RtspString to write after the prefix.
 *              addDelimiter    - RV_TRUE if the "\r\n" delimiter should be added.
 * OUTPUT   :   phElement       - an RPool element with strPrefix followed by
 *                                hStr.
 * INOUT    :   None.
 *************************************************************************/
static RvBool RtspHeadersStrSerialize(
                        IN  HRPOOL              hRPool,
                        IN  RvChar              *strPrefix,
                        IN  RvRtspStringHandle  hStr,
                        IN  RvBool              addDelimiter,
                        OUT HRPOOLELEM          *phElement)
{
    RvSize_t    headerLength;
    RvSize_t    headerStrLength;
    RvSize_t    elementOffset = 0;


    /* checking parameters */
    if ((hRPool == NULL) || (strPrefix == NULL))
        return RV_FALSE;

    headerStrLength = RtspUtilsRPOOLELEMStrLen(hRPool, (HRPOOLELEM)hStr, 0);
    headerLength = strlen(strPrefix) + headerStrLength + ((RvUint32)(addDelimiter)*2); /* space for the header name,*/
                                                                /* header value and "\r\n"  */

    /* allocate a suitable space in hElement */
    *phElement = rpoolAlloc(hRPool, (RvInt32)headerLength);

    /* copy the header name */
    rpoolCopyFromExternal(  hRPool,
                            *phElement,
                            strPrefix,
                            (RvInt32)elementOffset,
                            (RvInt32)strlen(strPrefix));
    elementOffset += strlen(strPrefix);

    /* copy the header value */
    RtspUtilsRPOOLELEMCopyInternal( hRPool,
                                    *phElement,
                                    elementOffset,
                                    (HRPOOLELEM)hStr,
                                    0,
                                    headerStrLength);
    elementOffset += headerStrLength;


    if (addDelimiter == RV_TRUE)
    {
        /* copy the "\r\n" */
        rpoolCopyFromExternal(hRPool,
                              *phElement,
                              "\r\n",
                              (RvInt32)elementOffset,
                              2);
    }

    return  RV_TRUE;
}



/**************************************************************************
 * RtspHeadersStrDeserialize
 * ------------------------------------------------------------------------
 * General: deserialize a header containing a string into an Rtsp string.
 *
 * Return Value:    RV_TRUE if serialization is successfull, negative values
 *                  if not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool          - The module's memory pool.
 *              hLine           - The line to deserialize from.
 *              lineOffset      - The offset from which to start taking the
 *                              header content.
 *              strDelimiters   - The delimiters for the end of the header
 *                              content.
 * OUTPUT   :   pHStr           - an Rtsp string with the header content.
 * INOUT    :   None.
 *************************************************************************/
static RvBool RtspHeadersStrDeserialize(
                        IN  HRPOOL                  hRPool,
                        IN  HRPOOLELEM              hLine,
                        IN  RvSize_t                lineOffset,
                        IN  RvChar                  *strDelimiters,
                        OUT RvRtspStringHandle      *pHStr)
{
    RvSize_t    strEndOffset;

    if ( (hRPool == NULL) || (hLine == NULL) || (pHStr == NULL))
        return RV_FALSE;

    RtspUtilsRPOOLELEMStrDelimiters(hRPool, hLine, lineOffset, strDelimiters, RV_FALSE, &strEndOffset);

    /* allocate and copy the header value */
    *pHStr = (RvRtspStringHandle)rpoolAlloc(hRPool, (RvInt32)(strEndOffset-lineOffset) + 1);
    RtspUtilsRPOOLELEMCopyInternal( hRPool,
                                    (HRPOOLELEM)*pHStr,
                                    0,
                                    hLine,
                                    lineOffset,
                                    strEndOffset-lineOffset);

    /* setting the null terminating character */
    RtspUtilsRPOOLELEMSetCell(hRPool, (HRPOOLELEM)*pHStr, lineOffset+strEndOffset, 0);
    return RV_TRUE;
}



/**************************************************************************
 * RtspHeadersNptTimeSerialize
 * ------------------------------------------------------------------------
 * General: serializes the given RvRtspNptTime structure into the given
 *          buffer.
 *
 * Return Value: RV_TRUE    - Time range serialized
 *               RV_FALSE   - Serialization unsuccessful.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   time        - The time to serialize.
 * OUTPUT   :   buffer      - The buffer to write into.
 * INOUT    :   None.
 *************************************************************************/
static RvBool RtspHeadersNptTimeSerialize(
            IN  RvRtspNptTime   *time,
            OUT RvChar          *buffer)
{
    switch (time->format)
    {
        case RV_RTSP_NPT_FORMAT_NOT_EXISTS:
        {
            buffer[0] = 0;
            break;
        }

        case RV_RTSP_NPT_FORMAT_NOW:
        {
            sprintf(buffer, "now");
            break;
        }

        case RV_RTSP_NPT_FORMAT_SEC:
        {
            sprintf(buffer, "%9.7f", time->seconds);
            break;
        }

        case RV_RTSP_NPT_FORMAT_HHMMSS:
        {
            sprintf(buffer, "%01d:%02d:%9.7f", time->hours, time->minutes, time->seconds);
            break;
        }

        default:
        {
            return RV_FALSE;
        }
    }

    return RV_TRUE;
}



/**************************************************************************
 * RtspHeadersRtpInfoLineDeserialize
 * ------------------------------------------------------------------------
 * General: De-serializes an RTP-Info header line from the given element
 *          into the given RvRtspRtpInfoHeader structure .
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pRtpInfo    - the header structure to deserialize into.
 * INOUT    :   None.
 *******************************************************************/
static RvBool RtspHeadersRtpInfoLineDeserialize(
                    IN  HRPOOL          hRPool,
                    IN  HRPOOLELEM      hLine,
                    IN  RvSize_t        lineOffset,
                    OUT RvRtspRtpInfo   *pRtpInfo)
{
    RvChar      token[100];
    RvSize_t    tokenLength;
    RvSize_t    tokenStartOffset;
    RvChar      *str;


    if ( (hRPool == NULL) || (hLine == NULL) || (pRtpInfo == NULL) )
        return RV_FALSE;

    /* retrieving the URI */
    RtspUtilsRPOOLELEMStrChr(hRPool, hLine, lineOffset, '=', &tokenStartOffset);
    tokenStartOffset++;
    RtspHeadersStrDeserialize(hRPool, hLine, tokenStartOffset, ";", (RvRtspStringHandle*)&pRtpInfo->hURI);

    /* retrieving parameter */
    RtspUtilsRPOOLELEMStrChr(hRPool, hLine, tokenStartOffset, ';', &tokenStartOffset);
    tokenStartOffset++;
    RtspUtilsRPOOLELEMGetToken(hRPool, hLine, tokenStartOffset, ", \r\n", RV_FALSE, token, 90, &tokenLength);
    token[tokenLength] = '\0';

    pRtpInfo->seqValid      = RV_FALSE;
    pRtpInfo->rtpTimeValid  = RV_FALSE;

    str = strstr(token,"seq=");
    if (str)
    {
        /* set the field value */
        sscanf(str,"seq=%d", &pRtpInfo->seq);
        pRtpInfo->seqValid = RV_TRUE;
    }

    str = strstr(token,"rtptime=");
    if (str)
    {
        /* set the field value */
        sscanf(str,"rtptime=%d", &pRtpInfo->rtpTime);
        pRtpInfo->rtpTimeValid = RV_TRUE;
    }

    return RV_TRUE;
}

/******************************************************************************
 * RtspHeaderswwwauthenticateDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Session header line into the given
 *          RvRtsp3WAuthtHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              hLine       - the header line element.
 *              lineOffset  - the offset into the line from which the
 *                          header content starts.
 * OUTPUT   :   pHeader     - the header structure to deserialize into.
 * INOUT    :   None.
 *****************************************************************************/
RvBool RtspHeaderswwwauthenticateDeserialize(
                        IN  HRPOOL                  hRPool,
                        IN  HRPOOLELEM              hLine,
                        IN  RvSize_t                lineOffset,
                        OUT RvRtsp3WAuthtHeader   *pHeader)
{
    RvChar      token[100];
    RvSize_t    tokenLength;
    RvSize_t    offset;
    RvBool      success = RV_TRUE;

    if ( (hRPool == NULL) || (hLine == NULL) || (pHeader == NULL) )
        return RV_FALSE;

    /* default values */
    pHeader->stale= RV_FALSE;

    memset((void *)(pHeader->realm), 0, sizeof(pHeader->realm));
    memset((void *)(pHeader->nonce), 0, sizeof(pHeader->nonce));

    
    //success = RtspUtilsRPOOLELEMStrStr(hRPool, hLine, lineOffset, "Digest", &offset);
    success = RtspUtilsRPOOLELEMStrChr(hRPool, hLine, lineOffset, ' ', &offset);
    lineOffset = offset+1;

    while (success)
    {
        RvInt rValA, rValB;
        /* get the parameters*/
        success = RtspUtilsRPOOLELEMGetToken(hRPool, hLine, lineOffset, ",", RV_FALSE,
            token, 90, &tokenLength);
        token[tokenLength] = '\0';

        if (strstr(token,"stale"))
        {
			RvChar stale[50];
            memset(stale, 0, sizeof(stale));
            //scanf(token, " stale=\"%s", stale);  
			sscanf(token, "stale=%*\"%[0-9a-zA-Z]%*\"", stale);
            if(strcmp(stale,"FALSE")==0)
	            pHeader->stale = RV_FALSE;
	     else
	            pHeader->stale = RV_TRUE;
        }
        else if (strstr(token, "realm"))
        {
            RvChar realm[50];
            memset(realm, 0, sizeof(realm));
            sscanf(token, "realm=%*\"%[0-9a-zA-Z]%*\"", realm);
			//sscanf(token, "%*[^\"]/%[^\"]", realm);
            memcpy(pHeader->realm, realm, sizeof(pHeader->realm) - 1);
        }
         else if (strstr(token, "nonce"))
        {
            RvChar nonce[50];
            memset(nonce, 0, sizeof(nonce));
			sscanf(token, "nonce=%*\"%[0-9a-zA-Z]%*\"", nonce);
            memcpy(pHeader->nonce, nonce, sizeof(pHeader->nonce) - 1);
        }

        success = RtspUtilsRPOOLELEMStrChr(hRPool, hLine, lineOffset, '", ', &offset);
        lineOffset = offset+1;
    } /* while(success) */

    return RV_TRUE;
}

#if defined(__cplusplus)
}
#endif
