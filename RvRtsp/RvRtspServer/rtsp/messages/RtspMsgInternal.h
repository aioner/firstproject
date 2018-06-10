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
 *                              <RtspMsg.h>
 *
 * Internal header - used inside the RvRtsp library.
 *
 *	This file contains definitions relevant to the RTSP generic messaging 
 *  module (RtspMsg).
 *
 *    Author                         Date
 *    ------                        ------
 *		Anat						21/2/07
 *
 *********************************************************************************/

#ifndef _RTSP_MSG_INTERNAL_H
#define _RTSP_MSG_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rvtypes.h"
#include "rverror.h"
#include "RvRtspCommonTypes.h"

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
    

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/


/* RtspMsgHeader
 * ----------------------------------------------------------------------------
 * Type definition of a RtspMsg header. 
 */
typedef struct RtspMsgHeader
{
	RvRtspStringHandle	    headerName;     /* The name of the header */
	RvRtspStringHandle	    headerFields;   /* The header line including all the field, 
                                               field values, delimiters and
                                               TERMINATING_DELIMITERS_STRING */
	struct RtspMsgHeader	*pNext;
	struct RtspMsgHeader	*pPrev;
}RtspMsgHeader;

/* RtspMsgRequest
 * ----------------------------------------------------------------------------
 * Type definition of a RtspMsg request object. 
 */
typedef struct RtspMsgRequest 
{
	RvRtspRequest	    request;        /* Supported headers request object 
                                           (Can be accessed directly by the application) */
	RtspMsgHeader	    *msgHeaders;    /* Linked list of RtspMsg headers */
    RtspMsgHeader       *lastHeader;    /* Points to the last element in the list */
    RvRtspHandle        hRtsp;          /* Handle of rtsp stack */
}RtspMsgRequest;

/* RtspMsgRequest
 * ----------------------------------------------------------------------------
 * Type definition of a RtspMsg request object. 
 */
typedef struct RtspMsgResponse 
{
	RvRtspResponse	    response;       /* Supported headers response object 
                                           (Can be accessed directly by the application) */
	RtspMsgHeader	    *msgHeaders;    /* Linked list of RtspMsg headers */
    RtspMsgHeader       *lastHeader;    /* Points to the last element in the list */
    RvRtspHandle        hRtsp;          /* Handle of rtsp stack */
}RtspMsgResponse;

/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/
/******************************************************************************
 * RtspMsgHeaderInsertdAdditionalFields
 * ----------------------------------------------------------------------------
 * General: 
 *  Sets the given additionalHeaderFields in the given rpool element. 
 * 
 * Arguments:
 * Input:   hRPool           - The Rtsp stack memory pool.
 *          hRtspMsgMessage  - The handle of the RtspMsgRequest/RtspMsgResponse message
 *                             to be sent.
 *          bIsRequest       - RV_TRUE if it is a request message.
 *          additionalFields - The RtspMessageHeader object containing the additional fields.
 * Output:  msgElement       - The message rpool element.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure.
 *****************************************************************************/
RvStatus RtspMsgHeaderInsertdAdditionalFields(
        IN      HRPOOL                  hRPool,
        IN      RvRtspMsgMessageHandle  hRtspMsgMessage,
        IN      RvBool                  bIsRequest,
        IN      RvRtspMsgHeaderHandle   additionalFields, 
        OUT     HRPOOLELEM              msgElement);

/******************************************************************************
 * RtspMsgInsertAdditionalHeaders
 * ----------------------------------------------------------------------------
 * General: 
 *  Adds all headers in the msgHeaders list into the rpool element. 
 *
 * Arguments:
 * Input:   hRPool           - The Rtsp stack memory pool.
 *          hRtspMsgMessage  - The handle of the RtspMsgRequest/RtspMsgResponse 
 *                             message
 *                             to be sent.
 *          bIsRequest       - RV_TRUE if it is a request message.
 * Output:  msgElement       - The message rpool element.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure.
 *****************************************************************************/
RvStatus RtspMsgInsertAdditionalHeaders(
        IN      HRPOOL                  hRPool,
        IN      RvRtspMsgMessageHandle  hRtspMsgMessage,
        IN      RvBool                  bIsRequest,
        OUT     HRPOOLELEM              msgElement);

/******************************************************************************
 * RtspMsgHeaderDesrialize
 * ----------------------------------------------------------------------------
 * General: 
 *  ADesrializes a header from a received message into an RtspMsgHeader object. 
 * 
 * Arguments:
 * Input:   hRtspMsgMessage  - The handle of the RtspMsgRequest/RtspMsgResponse 
 *                             message
 *          bIsRequest       - RV_TRUE if it is a request message.
 *          headerBuffer     - A buffer containing the header line.
 *          headerLen        - The length of the header line.
 * Output:  None.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure.
 *****************************************************************************/
RvStatus RtspMsgHeaderDesrialize(
        IN      RvRtspMsgMessageHandle  hRtspMsgMessage, 
        IN      RvBool                  bIsRequest,
        IN      RvChar*                 headerBuffer);


#ifdef __cplusplus
}
#endif

#endif  /* END OF: define _RTSP_MSG_INTERNAL_H */


