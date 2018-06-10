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
 *                              <RtspMessages.c>
 *
 *	This file manges the serialization and de-serialization of the RTSP messages.
 *
 *	Note: the messages are represented using RPool elements, whereas the internal
 *	serialization/de-serialization functions use RvChar buffers, the rtspmessages
 *	is responsible for converting between the two.
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							12/1/04
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

#include "RtspMessages.h"
#include "RtspFirstLine.h"
#include "RtspHeaders.h"
#include "RtspUtilsInternal.h"
#include "RtspMsgInternal.h"
#include "RtspObject.h"
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



/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/

/******************************************************************************
 * RvRtspMessageConstructRequest
 * ----------------------------------------------------------------------------
 * General: 
 *  Constructs a request message object. memset MUST NOT be called on the
 *  pRequest object after construction, it is done during construction.
 *
 * Arguments:
 * Input:   hRtsp               - The rtsp stack handle.
 * Output:  pRequest            - The request object.

 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI  
RvStatus RVCALLCONV RvRtspMessageConstructRequest(
                    IN   RvRtspHandle    hRtsp,	
                    OUT  RvRtspRequest   **pRequest)
{
    RvRtsp          *pThis = (RvRtsp *)hRtsp;
    void            *tmpRequest;
    RvInt32         res;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
#endif

    if (pThis == NULL || pRequest == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    res = raAdd(pThis->hRaRequests, (RAElement *)&tmpRequest);
    if (res < 0)
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequestMsg = (RtspMsgRequest *)tmpRequest;
    memset(RtspMsgRequestMsg, 0, sizeof(RtspMsgRequest));
    RtspMsgRequestMsg->hRtsp = hRtsp;
    RtspMsgRequestMsg->msgHeaders = NULL;
    RtspMsgRequestMsg->lastHeader = NULL;
    *pRequest = &RtspMsgRequestMsg->request;
    (*pRequest)->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgRequestMsg;
#else
    *pRequest = (RvRtspRequest *)tmpRequest;
    memset(*pRequest, 0, sizeof(RvRtspRequest));
#endif
    (*pRequest)->requestBelongs2App = RV_TRUE;
    return RV_OK;
}

/******************************************************************************
 * RvRtspMessageDestructRequest
 * ----------------------------------------------------------------------------
 * General: 
 *  Destructs a request message object.
 *  This API should be called for request messages that were created by 
 *  the application using the RvRtspMessageConstructRequest, from 
 *  RvRtspConnectionReleaseMessageEv or RvRtspSessionReleaseMessageEv. 
 *
 * Arguments:
 * Input:   hRtsp               - The rtsp stack handle.
 * Output:  pRequest            - The handle to the request object.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI  
RvStatus RVCALLCONV RvRtspMessageDestructRequest(
                    IN   RvRtspHandle    hRtsp,	
                    IN   RvRtspRequest   *pRequest)
{
     RvRtsp          *pThis      =  (RvRtsp *)hRtsp;
     void            *tmpRequest =  (void *)pRequest;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg; 
#endif

    if (pThis == NULL || pRequest == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequestMsg = (RtspMsgRequest  *)pRequest->hRtspMsgMessage;
    tmpRequest = (void *)RtspMsgRequestMsg;
#endif

    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
    raDelete(pThis->hRaRequests, (RAElement)tmpRequest);
    
    return RV_OK;       
}

/******************************************************************************
 * RvRtspMessageConstructResponse
 * ----------------------------------------------------------------------------
 * General: 
 *  Constructs a response message object.
 *
 * Arguments:
 * Input:   hRtsp               - The rtsp stack handle.
 * Output:  pResponse           - The response object.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI  
RvStatus RVCALLCONV RvRtspMessageConstructResponse(
                    IN   RvRtspHandle    hRtsp,	
                    OUT  RvRtspResponse  **pResponse)
{
    RvRtsp          *pThis = (RvRtsp *)hRtsp;
    void            *tmpResponse;
    RvInt32         res;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgResponse  *RtspMsgResponseMsg;
#endif

    if (pThis == NULL || pResponse == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    res = raAdd(pThis->hRaResponses, (RAElement *)&tmpResponse);
    if (res < 0)
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgResponseMsg = (RtspMsgResponse *)tmpResponse;
    memset(RtspMsgResponseMsg, 0, sizeof(RtspMsgResponse));
    RtspMsgResponseMsg->hRtsp = hRtsp;
    RtspMsgResponseMsg->msgHeaders = NULL;
    RtspMsgResponseMsg->lastHeader = NULL;
    *pResponse = &RtspMsgResponseMsg->response;
    (*pResponse)->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgResponseMsg;
#else
    *pResponse = (RvRtspResponse *)tmpResponse;
    memset(*pResponse, 0, sizeof(RvRtspResponse));
#endif
    return RV_OK;
}

/******************************************************************************
 * RvRtspMessageDestructResponse
 * ----------------------------------------------------------------------------
 * General: 
 *  Destructs a response message object.
 *  This API should be called for response messages that were created by 
 *  the application using the RvRtspMessageConstructResponse, from 
 *  RvRtspConnectionReleaseMessageEv or RvRtspSessionReleaseMessageEv.  
 *
 * Arguments:
 * Input:   hRtsp               - The rtsp stack handle.
 * Output:  pResponse           - The handle to the response object.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI  
RvStatus RVCALLCONV RvRtspMessageDestructResponse(
                    IN   RvRtspHandle    hRtsp,	
                    IN   RvRtspResponse  *pResponse)
{
     RvRtsp          *pThis      =  (RvRtsp *)hRtsp;
     void            *tmpResponse =  (void *)pResponse;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgResponse  *RtspMsgResponseMsg; 
#endif

    if (pThis == NULL || pResponse == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgResponseMsg = (RtspMsgResponse  *)pResponse->hRtspMsgMessage;
    tmpResponse = (void *)RtspMsgResponseMsg;
#endif

    RtspMessagesResponseDestruct(pThis->hRPool, pResponse);
    raDelete(pThis->hRaResponses, (RAElement)tmpResponse);
    
    return RV_OK;       
}

/**************************************************************************
 * RtspMessagesRequestConstruct
 * ------------------------------------------------------------------------
 * General: Constructs the message, initializing it's fields.
 *
 * Return Value: RV_OK if message is constructed successfully, other
 *				 values if unsuccessfull.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pRequest		- the message to be constructed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspMessagesRequestConstruct(
			IN	RvRtspRequest		*pRequest)
{
    /* DON'T ever call memset in this function !!! */
	/* checking parameters		*/
	if (pRequest == NULL)
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	pRequest->requestLine.hURI	= NULL;
	pRequest->cSeqValid			= RV_FALSE;
	pRequest->acceptValid		= RV_FALSE;
	pRequest->sessionValid		= RV_FALSE;
	pRequest->rangeValid		= RV_FALSE;
	pRequest->transportValid	= RV_FALSE;
	pRequest->locationValid		= RV_FALSE;
    pRequest->connectionValid   = RV_FALSE;
    pRequest->requireValid      = RV_FALSE;

	return RV_OK;
}



/**************************************************************************
 * RtspMessagesResponseConstruct
 * ------------------------------------------------------------------------
 * General: Constructs the message, initializing it's fields.
 *
 * Return Value: RV_OK if message is constructed successfully, negative
 *				 values if unsuccessfull.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pResponse		- the message to be constructed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspMessagesResponseConstruct(
					IN	RvRtspResponse		*pResponse)
{
    /* DON'T ever call memset in this function !!! */
	/* checking parameters		*/
	if (pResponse == NULL)
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	pResponse->statusLine.hPhrase	= NULL;
	pResponse->cSeqValid			= RV_FALSE;
	pResponse->connectionValid		= RV_FALSE;
	pResponse->sessionValid			= RV_FALSE;
	pResponse->transportValid		= RV_FALSE;
	pResponse->contentLanguageValid = RV_FALSE;
	pResponse->contentEncodingValid = RV_FALSE;
	pResponse->contentLengthValid	= RV_FALSE;
	pResponse->contentTypeValid		= RV_FALSE;
	pResponse->rtpInfoValid			= RV_FALSE;
    pResponse->publicHdrValid       = RV_FALSE;
	pResponse->hBody				= NULL;

	return RV_OK;
}


/**************************************************************************
 * RtspMessagesRequestDestruct
 * ------------------------------------------------------------------------
 * General: Destructs the specified message, releasing it's elements back
 *			to the memory pool.
 *
 * Return Value: RV_OK if message is destructed successfully, negative
 *				 values if unsuccessfull.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool			- the RPool to which the elements in the
 *								message belong.
 *				pRequest		- the message to be destructed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspMessagesRequestDestruct(
					IN	HRPOOL				hRPool,
					IN	RvRtspRequest		*pRequest)
{
	/* checking parameters		*/
	if ((hRPool == NULL) || (pRequest == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	if (pRequest->requestLine.hURI != NULL)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pRequest->requestLine.hURI);
		pRequest->requestLine.hURI = NULL;
	}

	pRequest->cSeqValid	= RV_FALSE;

	if (pRequest->acceptValid == RV_TRUE)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pRequest->accept.hStr);
		pRequest->accept.hStr = NULL;
		pRequest->acceptValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
       pRequest->accept.additionalFields = NULL;
#endif
	}

	if (pRequest->sessionValid == RV_TRUE)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pRequest->session.hSessionId);
		pRequest->session.hSessionId = NULL;
		pRequest->sessionValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pRequest->session.additionalFields = NULL;
#endif
	}

	if (pRequest->locationValid == RV_TRUE)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pRequest->location.hStr);
		pRequest->location.hStr = NULL;
        pRequest->locationValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pRequest->location.additionalFields = NULL;
#endif
		
	}
    if (pRequest->requireValid == RV_TRUE) 
    {
        rpoolFree(hRPool, (HRPOOLELEM)pRequest->require.hStr);
        pRequest->require.hStr = NULL;
        pRequest->requireValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pRequest->require.additionalFields = NULL;
#endif

    }

    if (pRequest->connectionValid == RV_TRUE)
    {
        rpoolFree(hRPool, (HRPOOLELEM)pRequest->connection.hStr);
        pRequest->connection.hStr = NULL;
        pRequest->connectionValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pRequest->connection.additionalFields = NULL;
#endif
    }

	pRequest->rangeValid		= RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pRequest->range.additionalFields = NULL;
#endif
	pRequest->transportValid	= RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pRequest->transport.additionalFields = NULL;
    RvRtspMsgRemoveAllHeaders(pRequest->hRtspMsgMessage, RV_TRUE);
#endif

	return RV_OK;
}



/**************************************************************************
 * RtspMessagesResponseDestruct
 * ------------------------------------------------------------------------
 * General: Destructs the specified message, releasing it's elements back
 *			to the memory pool.
 *
 * Return Value: RV_OK if message is destructed successfully, negative
 *				 values if unsuccessfull.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool			- the RPool to which the elements in the
 *								message belong.
 *				pResponse		- the message to be destructed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspMessagesResponseDestruct(
					IN	HRPOOL				hRPool,
					IN	RvRtspResponse		*pResponse)
{
	RvInt32			location;
	RvRtspRtpInfo	*pRtpInfo;

	/* checking parameters		*/
	if ((hRPool == NULL) || (pResponse == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);


	if (pResponse->statusLine.hPhrase != NULL)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pResponse->statusLine.hPhrase);
		pResponse->statusLine.hPhrase = NULL;
	}


	// LA 20140325
	// 孔总解决内存泄露 
	/* add by kongfd*/
	if (pResponse->publicHdr.hStr != NULL)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pResponse->publicHdr.hStr);
		pResponse->publicHdr.hStr = NULL;
	}
	/* add end*/

	pResponse->cSeqValid = RV_FALSE;

	if (pResponse->connectionValid == RV_TRUE)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pResponse->connection.hStr);
		pResponse->connection.hStr = NULL;
		pResponse->connectionValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pResponse->connection.additionalFields = NULL;
#endif

	}

	if (pResponse->sessionValid == RV_TRUE)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pResponse->session.hSessionId);
		pResponse->session.hSessionId = NULL;
		pResponse->sessionValid	= RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pResponse->session.additionalFields = NULL;
#endif
	}

	pResponse->transportValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pResponse->transport.additionalFields = NULL;
#endif

	if (pResponse->contentLanguageValid == RV_TRUE)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pResponse->contentLanguage.hStr);
		pResponse->contentLanguage.hStr = NULL;
		pResponse->contentLanguageValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pResponse->contentLanguage.additionalFields = NULL;
#endif
	}

	if (pResponse->contentEncodingValid == RV_TRUE)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pResponse->contentEncoding.hStr);
		pResponse->contentEncoding.hStr = NULL;
		pResponse->contentEncodingValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pResponse->contentEncoding.additionalFields = NULL;
#endif
	}

	pResponse->contentLengthValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pResponse->contentLength.additionalFields = NULL;
#endif

	if (pResponse->contentTypeValid == RV_TRUE)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pResponse->contentType.hStr);
		pResponse->contentType.hStr = NULL;
		pResponse->contentTypeValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pResponse->contentType.additionalFields = NULL;
#endif        
    }

    if (pResponse->contentBaseValid == RV_TRUE)
    {
        rpoolFree(hRPool, (HRPOOLELEM)pResponse->contentBase.hStr);
        pResponse->contentBase.hStr = NULL;
        pResponse->contentBaseValid = RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pResponse->contentBase.additionalFields = NULL;
#endif
	}

	if (pResponse->rtpInfoValid== RV_TRUE)
	{
		/* empty the RA and free its elements */
		location = raGetNext((HRA)pResponse->rtpInfo.hInfo, -1);
		while (location >= 0)
		{
			pRtpInfo = (RvRtspRtpInfo*) raGet((HRA)pResponse->rtpInfo.hInfo, location);
			rpoolFree(hRPool, (HRPOOLELEM)pRtpInfo->hURI);
			location = raGetNext((HRA)pResponse->rtpInfo.hInfo, location);
		}
		raClear((HRA)pResponse->rtpInfo.hInfo);
		raDestruct((HRA)pResponse->rtpInfo.hInfo);

		pResponse->rtpInfo.hInfo	= NULL;
		pResponse->rtpInfoValid		= RV_FALSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pResponse->rtpInfo.additionalFields = NULL;
#endif
	}

	if (pResponse->hBody != NULL)
	{
		rpoolFree(hRPool, (HRPOOLELEM)pResponse->hBody);
		pResponse->hBody = NULL;
	}

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgRemoveAllHeaders(pResponse->hRtspMsgMessage, RV_FALSE);
#endif

	return RV_OK;
}




/* serializations */


/**************************************************************************
 * RtspMessagesRequestSerialize
 * ------------------------------------------------------------------------
 * General: Serializes the given message into an RPool element.
 *
 * Return Value: RV_OK if message is serialized successfully, negative
 *				 values if unsuccessfull.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool				- the RPool to which the elements belong.
 *				pMessage			- the message to be serialized.
 * OUTPUT	:	phMessageElement	- the serialized element.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspMessagesRequestSerialize(
					IN	HRPOOL				hRPool,
					IN	RvRtspRequest		*pMessage,
					OUT HRPOOLELEM			*phMessageElement)
{
	HRPOOLELEM	hTempElement;
	int			lastSize;
	RvUint8*	appendedDataPtr;

	/* checking parameters				*/
	if ( (hRPool == NULL) || (pMessage == NULL) || (phMessageElement == NULL) )
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	/* serializing the request line		*/
	RtspRequestLineSerialize(hRPool, &pMessage->requestLine, phMessageElement);


	/* appending the headers 			*/
	if (pMessage->cSeqValid == RV_TRUE)
	{
		RtspHeadersCSeqSerialize(hRPool, &pMessage->cSeq, &hTempElement);
		RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
		rpoolFree(hRPool, hTempElement);
	}

	if (pMessage->acceptValid == RV_TRUE)
	{
		RtspHeadersAcceptSerialize(hRPool, &pMessage->accept, &hTempElement);
		RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
		rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->accept.additionalFields != NULL)
        {
            /* Append the additional fields of the accept header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool, 
                                                 pMessage->hRtspMsgMessage, 
                                                 RV_TRUE, 
                                                 pMessage->accept.additionalFields, 
                                                 *phMessageElement);
        }
#endif
        }
    
    if (pMessage->locationValid == RV_TRUE)
    {
        RtspHeadersLocationSerialize(hRPool, &pMessage->location, &hTempElement);
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
        rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->location.additionalFields != NULL)
        {
            /* Append the additional fields of the location header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool, 
                                                 pMessage->hRtspMsgMessage, 
                                                 RV_TRUE, 
                                                 pMessage->location.additionalFields, 
                                                 *phMessageElement);
        }
#endif
    }

    if (pMessage->sessionValid == RV_TRUE)
    {
        RtspHeadersSessionSerialize(hRPool, &pMessage->session, &hTempElement);
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
        rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->session.additionalFields != NULL)
        {
            /* Append the additional fields of the session header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool, 
                                                 pMessage->hRtspMsgMessage, 
                                                 RV_TRUE, 
                                                 pMessage->session.additionalFields, 
                                                 *phMessageElement);
        }
#endif
	}

	if (pMessage->rangeValid == RV_TRUE)
	{
		RtspHeadersRangeSerialize(hRPool, &pMessage->range, &hTempElement);
		RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
		rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->range.additionalFields != NULL)
        {
            /* Append the additional fields of the range header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool, 
                                                 pMessage->hRtspMsgMessage, 
                                                 RV_TRUE, 
                                                 pMessage->range.additionalFields, 
                                                 *phMessageElement);
        }
#endif
	}

	if (pMessage->transportValid == RV_TRUE)
	{
		RtspHeadersTransportSerialize(hRPool, &pMessage->transport, &hTempElement);
		RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
		rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->transport.additionalFields != NULL)
        {
            /* Append the additional fields of the transport header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool, 
                                                 pMessage->hRtspMsgMessage, 
                                                 RV_TRUE, 
                                                 pMessage->transport.additionalFields, 
                                                 *phMessageElement);
        }
#endif
    }
    
    if (pMessage->requireValid == RV_TRUE)
    {
        RtspHeadersRequireSerialize(hRPool, &pMessage->require, &hTempElement);
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
        rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->require.additionalFields != NULL)
        {
            /* Append the additional fields of the require header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool, 
                                                 pMessage->hRtspMsgMessage, 
                                                 RV_TRUE, 
                                                 pMessage->require.additionalFields, 
                                                 *phMessageElement);
        }
#endif
    }
    
    if (pMessage->connectionValid == RV_TRUE)
    {
        RtspHeadersConnectionSerialize(hRPool, &pMessage->connection, &hTempElement);
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
        rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->connection.additionalFields != NULL)
        {
            /* Append the additional fields of the connection header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool, 
                                                 pMessage->hRtspMsgMessage, 
                                                 RV_TRUE, 
                                                 pMessage->connection.additionalFields, 
                                                 *phMessageElement);
        }
#endif
    }    
        
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
    if (pMessage->hRtspMsgMessage != NULL)
    {
        RtspMsgInsertAdditionalHeaders(hRPool,
                                       pMessage->hRtspMsgMessage, 
                                       RV_TRUE, 
                                       *phMessageElement);
    }
#endif

	rpoolAppend(hRPool,
				*phMessageElement,
				(RvInt32)strlen(TERMINATING_DELIMITERS_STRING),
				&lastSize,
				&appendedDataPtr);

	memcpy(appendedDataPtr, TERMINATING_DELIMITERS_STRING, 2);

	return RV_OK;
}

/**************************************************************************
 * RtspMessagesResponseSerialize
 * ------------------------------------------------------------------------
 * General: Serializes the response into the given RPool element, only cSeq
 *			header is serialized.
 *
 * Return Value: RV_OK if message is serialized successfully, negative
 *				 values if unsuccessfull.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool				- the RPool to which the elements belong.
 *				pMessage			- the message to be serialized.
 * OUTPUT	:	phMessageElement	- the serialized element.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspMessagesResponseSerialize(
			IN	HRPOOL				hRPool,
			IN	RvRtspResponse		*pMessage,
			OUT HRPOOLELEM			*phMessageElement)
{
    HRPOOLELEM	hTempElement;
    int			lastSize;
    RvUint8*	appendedDataPtr;

    lastSize = 0;
    appendedDataPtr = NULL;

	/* checking parameters				*/
	if ( (hRPool == NULL) || (pMessage == NULL) || (phMessageElement == NULL) )
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	/* serializing the request line		*/
	RtspStatusLineSerialize(hRPool, &pMessage->statusLine, phMessageElement);

	/* appending the headers 			*/
	RtspHeadersCSeqSerialize(hRPool, &pMessage->cSeq, &hTempElement);
	RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
	rpoolFree(hRPool, hTempElement);
	
    /* Serializing the Connection Header */
    if (pMessage->connectionValid == RV_TRUE)
    {
       RtspHeadersConnectionSerialize (hRPool, &pMessage->connection, &hTempElement);
       RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
       rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->connection.additionalFields != NULL)
        {
            /* Append the additional fields of the connection header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool,
                                                 pMessage->hRtspMsgMessage,
                                                 RV_FALSE,
                                                 pMessage->connection.additionalFields,
                                                 *phMessageElement);
        }
#endif
    }

    /* Serializing the Session Header */
    if (pMessage->sessionValid == RV_TRUE)
    {
       RtspHeadersSessionSerialize (hRPool, &pMessage->session, &hTempElement);
       RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
       rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->session.additionalFields != NULL)
        {
            /* Append the additional fields of the session header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool,
                                                 pMessage->hRtspMsgMessage,
                                                 RV_FALSE,
                                                 pMessage->session.additionalFields,
                                                 *phMessageElement);
        }
#endif
    }
    
    /* Serializing the public header       */
    if (pMessage->publicHdrValid == RV_TRUE)
    {
        RtspPublicSerialize (hRPool, &pMessage->publicHdr, &hTempElement);
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
        rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->publicHdr.additionalFields != NULL)
        {
            /* Append the additional fields of the public header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool,
                                                 pMessage->hRtspMsgMessage,
                                                 RV_FALSE,
                                                 pMessage->publicHdr.additionalFields,
                                                 *phMessageElement);
        }
#endif
    }
    
    /* Serializing the Content-Base header */
    if (pMessage->contentBaseValid == RV_TRUE)
    {
        RtspContentBaseSerialize (hRPool,&pMessage->contentBase,&hTempElement);
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
        rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->contentBase.additionalFields != NULL)
        {
            /* Append the additional fields of the contentBase header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool,
                                                 pMessage->hRtspMsgMessage,
                                                 RV_FALSE,
                                                 pMessage->contentBase.additionalFields,
                                                 *phMessageElement);
        }
#endif
    }
 
    /* Serializing the Content-Encoding header */
    if (pMessage->contentEncodingValid == RV_TRUE)
    {
        RtspHeadersContentEncodingSerialize (hRPool,&pMessage->contentEncoding,&hTempElement);
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
        rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->contentEncoding.additionalFields != NULL)
        {
            /* Append the additional fields of the contentEncoding header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool,
                                                 pMessage->hRtspMsgMessage,
                                                 RV_FALSE,
                                                 pMessage->contentEncoding.additionalFields,
                                                 *phMessageElement);
        }
#endif
    }

    /* Serializing the Content-Language header */
    if (pMessage->contentLanguageValid == RV_TRUE)
    {
        RtspHeadersContentLanguageSerialize (hRPool,&pMessage->contentLanguage,&hTempElement);
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
        rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->contentLanguage.additionalFields != NULL)
        {
            /* Append the additional fields of the contentLanguage header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool,
                                                 pMessage->hRtspMsgMessage,
                                                 RV_FALSE,
                                                 pMessage->contentLanguage.additionalFields,
                                                 *phMessageElement);
        }
#endif
    }

    /* Serializing the Content-Type header */   
    if (pMessage->contentTypeValid == RV_TRUE)
    {
        RtspContentTypeSerialize (hRPool,&pMessage->contentType,&hTempElement);
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
        rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->contentType.additionalFields != NULL)
        {
            /* Append the additional fields of the contentType header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool,
                                                 pMessage->hRtspMsgMessage,
                                                 RV_FALSE,
                                                 pMessage->contentType.additionalFields,
                                                 *phMessageElement);
        }
#endif
    }

    /* Serializing the Content-Length header */
    if (pMessage->contentLengthValid == RV_TRUE)
    {
        RtspContentLengthSerialize (hRPool,&pMessage->contentLength,&hTempElement);
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
        rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->contentLength.additionalFields != NULL)
        {
            /* Append the additional fields of the contentLength header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool,
                                                 pMessage->hRtspMsgMessage,
                                                 RV_FALSE,
                                                 pMessage->contentLength.additionalFields,
                                                 *phMessageElement);
        }
#endif
    }
    
    /* Serializing the Transport Header */
    if (pMessage->transportValid == RV_TRUE)
    {
       RtspHeadersTransportSerialize (hRPool, &pMessage->transport, &hTempElement);
       RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
       rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->transport.additionalFields != NULL)
        {
            /* Append the additional fields of the transport header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool,
                                                 pMessage->hRtspMsgMessage,
                                                 RV_FALSE,
                                                 pMessage->transport.additionalFields,
                                                 *phMessageElement);
        }
#endif
    }
    
    /* Serializing the RTP-Info Header */
    if (pMessage->rtpInfoValid == RV_TRUE)
    {
       RtspHeadersRtpInfoSerialize (hRPool, &pMessage->rtpInfo, &hTempElement);
       RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, hTempElement);
       rpoolFree(hRPool, hTempElement);
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        if (pMessage->rtpInfo.additionalFields != NULL)
        {
            /* Append the additional fields of the rtpInfo header */
            RtspMsgHeaderInsertdAdditionalFields(hRPool,
                                                 pMessage->hRtspMsgMessage,
                                                 RV_FALSE,
                                                 pMessage->rtpInfo.additionalFields,
                                                 *phMessageElement);
        }
#endif
    }

#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
    if (pMessage->hRtspMsgMessage != NULL)
    {
        RtspMsgInsertAdditionalHeaders(hRPool,
                                       pMessage->hRtspMsgMessage,
                                       RV_FALSE,
                                       *phMessageElement);
    }
#endif
 
    /* appending the body                */
    if (pMessage->hBody != NULL)
    {
        RtspUtilsHPOOLELEMAppend(hRPool, *phMessageElement, (HRPOOLELEM)pMessage->hBody);
        
        return RV_OK;
    }
    
	rpoolAppend(hRPool,
				*phMessageElement,
				(RvInt32)strlen(TERMINATING_DELIMITERS_STRING),
				&lastSize,
				&appendedDataPtr);

    if (appendedDataPtr&&lastSize>0)
    {
        memcpy(appendedDataPtr, TERMINATING_DELIMITERS_STRING, 2);
    }

	/* appending the body				*/
	/* no body! */

	return RV_OK;

}




/* de-serializations */


/**************************************************************************
 * RtspMessagesRequestDeserialize
 * ------------------------------------------------------------------------
 * General:	De-serializes the serial headers and body into the given message.
 *			After this function the source Queue and body are empty.
 *
 * Return Value:	RV_OK if line is serialized successfully,
 *					negative values if failed
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool 			- the module's RPool memory pool.
 *				pHeadersQueue	- the queue containing the header elements.
 *				hBody			- the message body to be deserialized.
 * OUTPUT	:	pRequest		- the response structure to deserialize into.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspMessagesRequestDeserialize(
						IN	HRPOOL				hRPool,
						IN	RvQueue				*pHeadersQueue,
						OUT	RvRtspRequest		*pRequest)
{
	RvBool 		success;
	RvChar 		token[50];
	RvSize_t	tokenLength;
	RvSize_t	headerOffset;
	HRPOOLELEM	hHeaderElement;
	RvStatus	result;
	RvBool		cont = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvChar      RtspMsgToken[256];
    RvSize_t    RtspMsgTokenLength;
#endif

	/* checking parameters */
	if ( (hRPool == NULL) || (pHeadersQueue == NULL) || (pRequest == NULL) )
		return RvRtspErrorCode(RV_ERROR_NULLPTR);


	/* initializing the response message.	*/
	pRequest->cSeqValid			= RV_FALSE;
	pRequest->acceptValid		= RV_FALSE;
	pRequest->sessionValid		= RV_FALSE;
	pRequest->rangeValid		= RV_FALSE;
	pRequest->transportValid	= RV_FALSE;
	pRequest->locationValid		= RV_FALSE;

	while (cont)
	{
		result = RvQueueReceive(pHeadersQueue,
								RV_QUEUE_NOWAIT,
								sizeof(HRPOOLELEM),
								(void*)(&hHeaderElement));

		if (RvErrorGetCode(result) == RV_QUEUE_ERROR_EMPTY)
			break;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        /* Get the token until the first delimiter */
        memset(RtspMsgToken, 0, sizeof(RtspMsgToken));
        success = RtspUtilsRPOOLELEMGetToken(hRPool,
											hHeaderElement,
											0,
                                            TERMINATING_DELIMITERS_STRING,
                                            RV_FALSE,
                                            RtspMsgToken,
                                            255,
                                            &RtspMsgTokenLength);

        if (success)
            /* Add the header into the MsgHeaders list as well*/
            RtspMsgHeaderDesrialize(pRequest->hRtspMsgMessage, RV_TRUE, RtspMsgToken);
#endif

		success = RtspUtilsRPOOLELEMGetToken(hRPool,
											hHeaderElement,
											0,
											" ",
											RV_FALSE,
											token,
											40,
											&tokenLength);
		if (success == RV_FALSE)
			return RvRtspErrorCode(RV_ERROR_BADPARAM);

		token[tokenLength] = '\0';
		headerOffset = tokenLength + 1;

        /* get the header, deserialize it and deallocate the element from which it was deserialized */
        if (strstr(token, "CSeq"))
        {
            pRequest->cSeqValid = RtspHeadersCSeqDeserialize(hRPool,hHeaderElement,headerOffset,&pRequest->cSeq);
        }
        
        if (strstr(token,"Accept"))
        {
            pRequest->acceptValid = RtspHeadersAcceptDeserialize(hRPool,hHeaderElement,headerOffset,&pRequest->accept);
        }
 
        if (strstr(token, "Session"))
        {
           pRequest->sessionValid = RtspHeadersSessionDeserialize(hRPool,hHeaderElement,headerOffset, &pRequest->session);
        }

        if (strstr(token, "Location"))
        {
            pRequest->locationValid = RtspHeadersLocationDeserialize(hRPool,hHeaderElement,headerOffset,&pRequest->location);
        }
        
        if (strstr(token,"Range"))
        {
           pRequest->rangeValid = RtspHeadersRangeDeserialize(hRPool,hHeaderElement,headerOffset,&pRequest->range);
        }

        if (strstr(token, "Transport"))
        {
            //modified by lichao, 20140814 增加额外fields反序列化
#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
            pRequest->transportValid = RtspHeadersTransportWithFieldsDeserialize(((RtspMsgRequest *)pRequest->hRtspMsgMessage)->hRtsp, pRequest->hRtspMsgMessage, hRPool,hHeaderElement,headerOffset,&pRequest->transport);
#else
            pRequest->transportValid = RtspHeadersTransportDeserialize(hRPool,hHeaderElement,headerOffset,&pRequest->transport);
#endif
        }

        if (strstr(token,"Connection"))
        {
            pRequest->connectionValid = RtspHeadersConnectionDeserialize(hRPool,hHeaderElement,headerOffset,&pRequest->connection);
        }
        
       

		/* freeing the element from which we deserialized */
		rpoolFree(hRPool,hHeaderElement);

	} /* end while */

	return RV_OK;
}




/**************************************************************************
 * RtspMessagesResponseDeserialize
 * ------------------------------------------------------------------------
 * General:	De-serializes the serial headers and body into the given message.
 *			After this function the source Queue and body are empty.
 *
 * Return Value:	RV_OK if line is serialized successfully,
 *					negative values if failed
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool 			- the module's RPool memory pool.
 *				pHeadersQueue	- the queue containing the header elements.
 *				hBody			- the message body to be deserialized.
 *				maxRtpInfoStreams - maximum number of RTP-Info streams allowed.
 * OUTPUT	:	pResponse		- the response structure to deserialize into.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspMessagesResponseDeserialize(
						IN	HRPOOL				hRPool,
						IN	RvQueue				*pHeadersQueue,
						IN	HRPOOLELEM			hBody,
						IN	RvUint16			maxRtpInfoStreams,
						OUT	RvRtspResponse		*pResponse)
{
	RvBool 		success;
	RvChar 		token[50];
	RvSize_t	tokenLength;
	RvSize_t	headerOffset;
	HRPOOLELEM	hHeaderElement;
	RvStatus	result;
    RvChar      ch;
    RvSize_t    i;
	RvBool		cont = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvChar      RtspMsgToken[256];
    RvSize_t    RtspMsgTokenLength;
#endif

	/* checking parameters */
	if ( (hRPool == NULL) || (pHeadersQueue == NULL) || (pResponse == NULL) )
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	/* initializing the response */
	pResponse->cSeqValid			= RV_FALSE;
	pResponse->connectionValid		= RV_FALSE;
	pResponse->sessionValid			= RV_FALSE;
	pResponse->transportValid		= RV_FALSE;
	pResponse->contentLanguageValid = RV_FALSE;
	pResponse->contentEncodingValid = RV_FALSE;
	pResponse->contentLengthValid	= RV_FALSE;
	pResponse->contentTypeValid		= RV_FALSE;
	pResponse->rtpInfoValid 		= RV_FALSE;
	pResponse->hBody				= NULL;

	while (cont)
	{
		result = RvQueueReceive(pHeadersQueue,
								RV_QUEUE_NOWAIT,
								sizeof(HRPOOLELEM),
								(void*)(&hHeaderElement));

		if (RvErrorGetCode(result) == RV_QUEUE_ERROR_EMPTY)
			break;

#if ((RV_RTSP_USE_RTSP_MSG == RV_YES))
        /* Get the token until the first delimiter */
        memset(RtspMsgToken, 0, sizeof(RtspMsgToken));
        success = RtspUtilsRPOOLELEMGetToken(hRPool,
											hHeaderElement,
											0,
                                            TERMINATING_DELIMITERS_STRING,
                                            RV_FALSE,
                                            RtspMsgToken,
                                            255,
                                            &RtspMsgTokenLength);
        if (success)
            /* Add the header into the MsgHeaders list as well*/
            RtspMsgHeaderDesrialize(pResponse->hRtspMsgMessage, RV_FALSE, RtspMsgToken);
#endif

		success = RtspUtilsRPOOLELEMGetToken(hRPool,
											hHeaderElement,
											0,
											":",
											RV_FALSE,
											token,
											40,
											&tokenLength);
		if (success == RV_FALSE)
			return RvRtspErrorCode(RV_ERROR_BADPARAM);

		token[tokenLength] = '\0';
		headerOffset = tokenLength + 1;

        for(i = 0; i < tokenLength; i++)
            token[i] = (RvChar)tolower(token[i]);

        RtspUtilsRPOOLELEMGetCell(
                            hRPool,
                            hHeaderElement,
                            headerOffset,
                            &ch);
        if (ch == ' ')
            headerOffset += 1;
        

		/* get the header, deserialize it and deallocate the element from which it was deserialized */
		if (strstr(token, "cseq"))
		{
			pResponse->cSeqValid = RtspHeadersCSeqDeserialize(hRPool,hHeaderElement,headerOffset,&pResponse->cSeq);
		}

		else if (strstr(token, "connection"))
		{
			pResponse->connectionValid = RtspHeadersConnectionDeserialize(hRPool,hHeaderElement,headerOffset,&pResponse->connection);
		}

		else if (strstr(token, "session"))
		{
			pResponse->sessionValid = RtspHeadersSessionDeserialize(hRPool,hHeaderElement,headerOffset,&pResponse->session);
		}

		else if (strstr(token, "transport"))
		{
			pResponse->transportValid = RtspHeadersTransportDeserialize(hRPool,hHeaderElement,headerOffset,&pResponse->transport);
		}

		else if (strstr(token, "content-language"))
		{
			pResponse->contentLanguageValid = RtspHeadersContentLanguageDeserialize(hRPool,hHeaderElement,headerOffset,&pResponse->contentLanguage);
		}

		else if (strstr(token, "content-encoding"))
		{
			pResponse->contentEncodingValid = RtspHeadersContentEncodingDeserialize(hRPool,hHeaderElement,headerOffset,&pResponse->contentEncoding);
		}

		else if (strstr(token, "content-length"))
		{
			pResponse->contentLengthValid = RtspHeadersContentLengthDeserialize(hRPool,hHeaderElement,headerOffset,&pResponse->contentLength);
		}

		else if (strstr(token, "content-type"))
		{
			pResponse->contentTypeValid = RtspHeadersContentTypeDeserialize(hRPool,hHeaderElement,headerOffset,&pResponse->contentType);
		}

		else if (strstr(token, "rtp-info"))
		{
			pResponse->rtpInfoValid = RtspHeadersRtpInfoDeserialize(hRPool,hHeaderElement,headerOffset, maxRtpInfoStreams, &pResponse->rtpInfo);
		}

		/* freeing the element from which we deserialized */
		rpoolFree(hRPool,hHeaderElement);

	} /* end while */

	/* move the body to the message */
	pResponse->hBody = (RvRtspBlobHandle)hBody;

	return RV_OK;
}



/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/

#if defined(__cplusplus)
}
#endif

