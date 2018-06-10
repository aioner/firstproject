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
 *                              <RtspMessages.h>
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

#ifndef _RTSP_MESSAGES_H
#define _RTSP_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILED                           */
/*-----------------------------------------------------------------------*/
#include "rvtypes.h"
#include "rvlog.h"
#include "ra.h"
#include "rpool.h"
#include "rvqueue.h"


#include "RvRtspCommonTypes.h"
#include "RvRtspMessageTypes.h"


/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/



/**************************************************************************
 * RtspMessagesRequestConstruct
 * ------------------------------------------------------------------------
 * General: Constructs the message, initializing it's fields.
 *
 * Return Value: RV_OK if message is constructed successfully, negative
 *				 values if unsuccessfull.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pRequest		- the message to be constructed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspMessagesRequestConstruct(
			IN	RvRtspRequest		*pRequest);

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
			IN	RvRtspResponse		*pResponse);

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
					IN	RvRtspRequest		*pRequest);

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
					IN	RvRtspResponse		*pResponse);

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
					OUT HRPOOLELEM			*phMessageElement);

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
			OUT HRPOOLELEM			*phMessageElement);

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
						OUT	RvRtspRequest		*pRequest);

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
						OUT	RvRtspResponse		*pResponse);


#ifdef __cplusplus
}
#endif

#endif	/* Include guard */


