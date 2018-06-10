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
 *                              <RtspHeaders.h>
 *
 *	This code handles the message headers serialization and de-serialization.
 *
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							12/1/04
 *
 *********************************************************************************/

#ifndef _RTSP_HEADERS_H
#define _RTSP_HEADERS_H

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


#include "RvRtspCommonTypes.h"
#include "RvRtspHeaderTypes.h"


/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/


/* serialization */


/**************************************************************************
 * RtspContentLanguageSerialize
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
                        OUT HRPOOLELEM               *phElement);


/**************************************************************************
 * RtspHeadersCSeqSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the CSeq header into it.
 *
 * Return Value:	RV_TRUE - Serialization successfull, negative values on
 *					failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's memory pool.
 *				pHeader		- the CSeq header to serialize.
 * OUTPUT	:	phElement 	- the serialized element.
 * INOUT	:	None.
 *******************************************************************/
RvBool RtspHeadersCSeqSerialize(
						IN	HRPOOL				hRPool,
						IN	RvRtspCSeqHeader	*pHeader,
						OUT	HRPOOLELEM			*phElement);



/**************************************************************************
 * RtspHeadersAcceptSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Accept header
 *			into it.
 *
 * Return Value: RV_TRUE - Serialization successfull, negative values on failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT	:  	hRPool		- the module's memory pool.
 *				pHeader		- the Accept header to serialize.
 * OUTPUT	:	phElement 	- the serialized element.
 * INOUT	:	None.
 *******************************************************************/
RvBool RtspHeadersAcceptSerialize(
						IN	HRPOOL				hRPool,
						IN	RvRtspAcceptHeader	*pHeader,
						OUT	HRPOOLELEM			*phElement);
/*add by kongfd*/
/**************************************************************************
 * RtspHeadersAcceptSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Accept header
 *			into it.
 *
 * Return Value: RV_TRUE - Serialization successfull, negative values on failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT	:  	hRPool		- the module's memory pool.
 *				pHeader		- the Accept header to serialize.
 * OUTPUT	:	phElement 	- the serialized element.
 * INOUT	:	None.
 *******************************************************************/
RvBool RtspHeadersAuthSerialize(
						IN	HRPOOL				hRPool,
						IN	RvRtspAuthHeader	*pHeader,
						OUT	HRPOOLELEM			*phElement);
/*add end*/



/**************************************************************************
 * RtspHeadersSessionSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Session header into it.
 *
 * Return Value: RV_TRUE - Serialization successfull, negative values on failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT	:  	hRPool		- the module's memory pool.
 *				pHeader		- the Session header to serialize.
 * OUTPUT	:	phElement	- the serialized element.
 * INOUT	:	None.
 *******************************************************************/
RvBool RtspHeadersSessionSerialize(
						IN	HRPOOL				hRPool,
						IN	RvRtspSessionHeader	*pHeader,
						OUT	HRPOOLELEM			*phElement);
						
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
                        OUT HRPOOLELEM          *phElement);

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
                        OUT HRPOOLELEM            *phElement);


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
                        OUT HRPOOLELEM          *pHPublic);



/**************************************************************************
 * RtspHeadersRangeSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Range header into it.
 *
 * Return Value: RV_TRUE - Serialization successfull, negative values on failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT	:  	hRPool		- the module's memory pool.
 *				pHeader		- the Range header to serialize.
 * OUTPUT	:	phElement	- the serialized element.
 * INOUT	:	None.
 *******************************************************************/
RvBool RtspHeadersRangeSerialize(
						IN	HRPOOL				hRPool,
						IN	RvRtspRangeHeader	*pHeader,
						OUT	HRPOOLELEM			*phElement);




/**************************************************************************
 * RtspHeadersTransportSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the Transport header
 *			into it.
 *
 * Return Value:	RV_TRUE - Serialization successfull, negative values on
 *					failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT	:  	hRPool		- the module's memory pool.
 *				pHeader		- the Transport header to serialize.
 * OUTPUT	:	phElement	- the serialized element.
 * INOUT	:	None.
 *******************************************************************/
RvBool RtspHeadersTransportSerialize(
						IN	HRPOOL					hRPool,
						IN	RvRtspTransportHeader	*pHeader,
						OUT	HRPOOLELEM				*phElement);


/**************************************************************************
 * RtspHeadersRtpInfoSerialize
 * ------------------------------------------------------------------------
 * General: Allocates an RPool element and Serializes the RTP-Info header
 *          into it.
 *
 * Return Value:    RV_TRUE - Serialization successfull, negative values on
 *                  failure
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the module's memory pool.
 *              pHeader     - the Rtp Info header to serialize.
 * OUTPUT   :   phElement   - the serialized element.
 * INOUT    :   None.
 *******************************************************************/
RvBool RtspHeadersRtpInfoSerialize(
                        IN  HRPOOL                 hRPool,
                        IN  RvRtspRtpInfoHeader    *pHeader,
                        OUT HRPOOLELEM             *phElement);


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
 * INPUT    :   hRPool      - the module's RPool memory pool.
 *              pContentBase  - the content base header to serialize.
 * OUTPUT   :   pHContentBase - the element to serialize into.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspContentBaseSerialize(
                        IN  HRPOOL                   hRPool,
                        IN  RvRtspContentBaseHeader  *pContentBase,
                        OUT HRPOOLELEM               *pHContentBase);

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
 * INPUT    :   hRPool      - the module's RPool memory pool.
 *              pContentType  - the content type header to serialize.
 * OUTPUT   :   pHContentType - the element to serialize into.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspContentTypeSerialize(
                        IN  HRPOOL                   hRPool,
                        IN  RvRtspContentTypeHeader  *pContentType,
                        OUT HRPOOLELEM               *pHContentType);

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
                        OUT HRPOOLELEM                 *pHContentLength);


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
                        OUT HRPOOLELEM                  *phElement);


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
                        OUT HRPOOLELEM                   *phElement);


/* de-serialization */


/******************************************************************************
 * RtspHeadersCSeqDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Cseq header line into the given	RvRtspCSeqHeader
 *			structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's memory pool.
 *				hLine		- the header line element.
 *				lineOffset	- the offset into the line from which the
 *							header content starts.
 * OUTPUT	:	pHeader		- the header structure to deserialize into.
 * INOUT	:	None.
 *****************************************************************************/
RvBool RtspHeadersCSeqDeserialize(
						IN	HRPOOL				hRPool,
						IN	HRPOOLELEM			hLine,
						IN	RvSize_t			lineOffset,
						OUT RvRtspCSeqHeader	*pHeader);

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
                        OUT RvRtspAcceptHeader  *pHeader);

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
                        OUT RvRtspAuthHeader  *pHeader);
/*add end*/

/******************************************************************************
 * RtspHeadersConnectionDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Connection header line into the given
 *			RvRtspConnectionHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's memory pool.
 *				hLine		- the header line element.
 *				lineOffset	- the offset into the line from which the
 *							header content starts.
 * OUTPUT	:	pHeader		- the header structure to deserialize into.
 * INOUT	:	None.
 *****************************************************************************/
RvBool RtspHeadersConnectionDeserialize(
						IN	HRPOOL					hRPool,
						IN	HRPOOLELEM				hLine,
						IN	RvSize_t				lineOffset,
						OUT RvRtspConnectionHeader	*pHeader);



/******************************************************************************
 * RtspHeadersSessionDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Session header line into the given
 *			RvRtspSessionHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's memory pool.
 *				hLine		- the header line element.
 *				lineOffset	- the offset into the line from which the
 *							header content starts.
 * OUTPUT	:	pHeader		- the header structure to deserialize into.
 * INOUT	:	None.
 *****************************************************************************/
RvBool RtspHeadersSessionDeserialize(
						IN	HRPOOL				hRPool,
						IN	HRPOOLELEM			hLine,
						IN	RvSize_t			lineOffset,
						OUT RvRtspSessionHeader	*pHeader);


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
                        OUT RvRtspRangeHeader       *pHeader);


/******************************************************************************
 * RtspHeadersTransportDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Session header line into the given
 *			RvRtspTransportHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's memory pool.
 *				hLine		- the header line element.
 *				lineOffset	- the offset into the line from which the
 *							header content starts.
 * OUTPUT	:	pHeader		- the header structure to deserialize into.
 * INOUT	:	None.
 *****************************************************************************/
RvBool RtspHeadersTransportDeserialize(
						IN	HRPOOL					hRPool,
						IN	HRPOOLELEM				hLine,
						IN	RvSize_t				lineOffset,
						OUT RvRtspTransportHeader	*pHeader);



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
                                       OUT RvRtspTransportHeader   *pHeader);

/******************************************************************************
 * RtspHeadersContentLanguageDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Content-Language header line into the given
 *			RvRtspContentLanguageHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's memory pool.
 *				hLine		- the header line element.
 *				lineOffset	- the offset into the line from which the
 *							header content starts.
 * OUTPUT	:	pHeader		- the header structure to deserialize into.
 * INOUT	:	None.
 *****************************************************************************/
RvBool RtspHeadersContentLanguageDeserialize(
						IN	HRPOOL						hRPool,
						IN	HRPOOLELEM					hLine,
						IN	RvSize_t					lineOffset,
						OUT RvRtspContentLanguageHeader	*pHeader);



/******************************************************************************
 * RtspHeadersContentEncodingDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Content-Encoding header line into the given
 *			RvRtspContentEncodingHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's memory pool.
 *				hLine		- the header line element.
 *				lineOffset	- the offset into the line from which the
 *							header content starts.
 * OUTPUT	:	pHeader		- the header structure to deserialize into.
 * INOUT	:	None.
 *****************************************************************************/
RvBool RtspHeadersContentEncodingDeserialize(
						IN	HRPOOL						hRPool,
						IN	HRPOOLELEM					hLine,
						IN	RvSize_t					lineOffset,
						OUT RvRtspContentEncodingHeader	*pHeader);


/******************************************************************************
 * RtspHeadersContentLengthDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Content-Length header line into the given
 *			RvRtspContentLengthHeader structure .
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's memory pool.
 *				hLine		- the header line element.
 *				lineOffset	- the offset into the line from which the
 *							header content starts.
 * OUTPUT	:	pHeader		- the header structure to de-serialize into.
 * INOUT	:	None.
 *****************************************************************************/
RVINTAPI RvBool RVCALLCONV RtspHeadersContentLengthDeserialize(
                        IN  HRPOOL                      hRPool,
                        IN  HRPOOLELEM                  hLine,
                        IN  RvSize_t                    lineOffset,
                        OUT RvRtspContentLengthHeader   *pHeader);

/******************************************************************************
 * RtspHeadersContentTypeDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Content-Type header line into the given
 *			RvRtspContentTypeHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's memory pool.
 *				hLine		- the header line element.
 *				lineOffset	- the offset into the line from which the
 *							header content starts.
 * OUTPUT	:	pHeader		- the header structure to deserialize into.
 * INOUT	:	None.
 *****************************************************************************/
RvBool RtspHeadersContentTypeDeserialize(
						IN	HRPOOL					hRPool,
						IN	HRPOOLELEM				hLine,
						IN	RvSize_t				lineOffset,
						OUT RvRtspContentTypeHeader	*pHeader);


/******************************************************************************
 * RtspHeadersLocationDeserialize
 * ----------------------------------------------------------------------------
 * General: De-serializes a Location header line into the given
 *			RvRtspContentTypeHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ----------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's memory pool.
 *				hLine		- the header line element.
 *				lineOffset	- the offset into the line from which the
 *							header content starts.
 * OUTPUT	:	pHeader		- the header structure to deserialize into.
 * INOUT	:	None.
 *****************************************************************************/
RvBool RtspHeadersLocationDeserialize(
						IN	HRPOOL					hRPool,
						IN	HRPOOLELEM				hLine,
						IN	RvSize_t				lineOffset,
						OUT RvRtspLocationHeader	*pHeader);


/**************************************************************************
 * RtspHeadersRtpInfoDeserialize
 * ------------------------------------------------------------------------
 * General: De-serializes an RTP-Info header line from the given element
 *			into the given RvRtspRtpInfoHeader structure.
 *
 * Return Value: RV_TRUE if deserialized successfully, RV_FALSE otherwise.
 * ------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool				- The module's memory pool.
 *				hLine				- The header line element.
 *				lineOffset			- The offset into the line from which
 *									the header content starts.
 *				maxRtpInfoStreams	- The maximum number of streams to
 *									get from an RTP-Info header.
 * OUTPUT	:	pHeader				- the header structure to deserialize
 *									into.
 * INOUT	:	None.
 *******************************************************************/
RvBool RtspHeadersRtpInfoDeserialize(
						IN	HRPOOL				hRPool,
						IN	HRPOOLELEM			hLine,
						IN	RvSize_t			lineOffset,
						IN	RvUint16			maxRtpInfoStreams,
						OUT RvRtspRtpInfoHeader	*pHeader);

/*add by kongfd*/
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
                        OUT RvRtsp3WAuthtHeader   *pHeader);
/*add end*/
#ifdef __cplusplus
}
#endif

#endif	/* Include guard */

