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
 *                              <RtspFirstLine.h>
 *
 *  This file manages the serialization and de-serialization of the RTSP message's
 *	first line. It includes the serialization/de-serialization functions for the
 *	request and status lines.
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							16/12/03
 *********************************************************************************/

#ifndef _RTSP_FIRST_LINE_H
#define _RTSP_FIRST_LINE_H

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
#include "RvRtspFirstLineTypes.h"


/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * RtspRequestLineSerialize
 * ------------------------------------------------------------------------
 * General: serializes the given RtspRequestLine structure into the RPool
 *			element.
 *
 * Return Value:	RV_OK if request line is serialized successfully,
 *					negative values if failed
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's RPool memory pool.
 *				pLine		- the request line to serialize.
 * OUTPUT	:	pHLine		- the element to serialize into.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RtspRequestLineSerialize(
						IN	HRPOOL				hRPool,
						IN	RvRtspRequestLine	*pLine,
						OUT	HRPOOLELEM			*pHLine);

/**************************************************************************
 * RtspStatusLineSerialize
 * ------------------------------------------------------------------------
 * General: serializes the given RvRtspStatusLine structure into the RPool
 *			element.
 *
 * Return Value:	RV_OK if request line is serialized successfully,
 *					negative values if failed
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the module's RPool memory pool.
 *				pLine		- the status line to serialize.
 * OUTPUT	:	pHLine		- the element to serialize into.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RtspStatusLineSerialize(
						IN	HRPOOL				hRPool,
						IN	RvRtspStatusLine	*pLine,
						OUT	HRPOOLELEM			*pHLine);

/**************************************************************************
 * RtspFirstLineDeserialize
 * ------------------------------------------------------------------------
 * General:	De-serializes a message's line from the given RPool element
 *			into one of the the given line structures.
 *
 * Return Value:	RV_OK if line is serialized successfully, a negative
 *					value if failed.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool 			- the module's memory pool.
 *				hLine			- the element containing the line to deserialize.
 *				pRequestLine	- the request line to deserialize into, if hLine
 *								is a request line.
 *				pStatusLine		- the status line to deserialize into, if hLine
 *								is a status line.
 * OUTPUT	:	requestLineValid- set to RV_TRUE if a request line is deserialized
 *								set to RV_FALSE otherwise.
 *				statusLineValid	- set to RV_TRUE is status line is serialized
 *								set to RV_FALSE otherwise.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspFirstLineDeserialize(
                        IN  HRPOOL              hRPool,
                        IN  HRPOOLELEM          hLine,
                        OUT RvRtspRequestLine   *pRequestLine,
                        OUT RvRtspStatusLine    *pStatusLine,
                        OUT RvBool              *pRequestLineValid,
                        OUT RvBool              *pStatusLineValid);

#ifdef __cplusplus
}
#endif

#endif	/* Include guard */



