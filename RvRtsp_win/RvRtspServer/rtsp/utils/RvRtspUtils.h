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
 *                              <RvRtspUtils.h>
 *
 *  Utility functions for RTSP types access, conversion and manipulation.
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							8/1/04
 *
 *********************************************************************************/

#ifndef _RV_RTSP_TYPES_UTILS_H
#define _RV_RTSP_TYPES_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/


#include "RvRtspCommonTypes.h"

/*@****************************************************************************
 * Module: RtspUtils (root)
 * ----------------------------------------------------------------------------
 * Title: 11.RTSP Utility API Functions
 *
 * The RTSP Utility API functions are for access, conversion, and manipulation
 * of RTSP types.
 *
 * You can find the RTSP Utility API functions in the RvRtspUtils.h file.
 ***************************************************************************@*/


/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/

/*@**************************************************************************
 * RvRtspStrlen (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 * This function returns the length of an RvRtspString.
 *
 * Arguments:
 * Input:   hRtsp     - The RTSP module to which the string belongs.
 *          hStr      - The RvRtspString.
 * Output:  pStrSize  - The length of the string.
 *
 * Return Value:  RV_OK if the method is completed successfully.
 *				  Negative values otherwise.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspStrlen(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspStringHandle	hStr,
					OUT	RvSize_t			*pStrSize);

/*@**************************************************************************
 * RvRtspStrcpy (RtspUtils)
 * ------------------------------------------------------------------------
 * General: This function copies the RvRtspString into the specified buffer.
 *
 * Arguments:
 * Input:   hRtsp      - The RTSP module to which the string belongs.
 *          hStr       - The RvRtspString.
 *          bufferSize - The length of the destination buffer.
 * Output:  pBuffer    - The destination buffer.
 *
 * Return Value:  RV_OK if the method is completed successfully.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspStrcpy(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspStringHandle	hStr,
					IN	RvSize_t			bufferSize,
					OUT	RvChar				*pBuffer);


/*@**************************************************************************
 * RvRtspBloblen (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 * This function returns the length of the RvRtspBlob object.
 *
 * Arguments:
 * Input:   hRtsp     - The RTSP module to which the blob belongs.
 *          hBlob     - The RvRtspBlob.
 * Output:  pBlobSize - The length of the blob.
 *
 * Return Value:  RV_OK if the method is completed successfully. 
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspBloblen(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspBlobHandle	hBlob,
					OUT	RvSize_t			*pBlobSize);



/*@**************************************************************************
 * RvRtspBlobcpy (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 * This function copies the RvRtspBlob into the specified buffer.
 *
 * Arguments:
 * Input:	hRtsp			- The RTSP module to which the blob belongs.
 *			hBlob			- The blob.
 *			bufferSize		- The size of the destination buffer.
 * Output:	pBuffer			- The destination buffer.
 *
 * Return Value:  RV_OK if the method is completed successfully.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspBlobcpy(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspBlobHandle	hBlob,
					IN	int					bufferSize,
					OUT	RvChar				*pBuffer);


/*@**************************************************************************
 * RvRtspBlobHandleSetBlob (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 * This API is used to set a blob into an rpool element (pointed to by a
 * string handle).
 *
 * Arguments:
 * Input:   hRtsp    - The handle to the RvRtsp structure.
 *          str      - The string to be changed to a string handle.
 * Output:  phStr    - The blobHandle to be set.
 *
 * Return Value:  RV_OK if the method is completed successfully.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspBlobHandleSetBlob(
                 IN RvRtspHandle           hRtsp,
                 IN const RvChar           *str,                         
                 OUT RvRtspBlobHandle    *phStr);


/*@**************************************************************************
 * RvRtspArraySize (RtspUtils)
 * ------------------------------------------------------------------------
 * General: This function returns the size of the array.
 *
 * Arguments:
 * Input:   hRtsp       - The RTSP module to which the array belongs. 
 *	        hArray      - The array.
 * Output:  pArraySize  - The size of the array.
 *
 * Return Value:  RV_OK if the method is completed successfully.
 *	              Negative values otherwise.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspArraySize(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspArrayHandle	hArray,
					OUT	RvSize_t			*pArraySize);



/*@**************************************************************************
 * RvRtspArrayGetFirst (RtspUtils)
 * ------------------------------------------------------------------------
 * General: This function returns the first element of the array.
 *
 * Arguments:
 * Input:   hRtsp          - The RTSP module to which the array belongs.
 *	        hArray         - The array.
 * Output:  pFirstElement  - The first element in the array.
 *
 * Return Value:  RV_OK if the method is completed successfully.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspArrayGetFirst(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspArrayHandle	hArray,
					OUT	void				**pFirstElement);



/*@**************************************************************************
 * RvRtspArrayGetNext (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 * This function returns the next element of the array.
 *
 * Arguments:
 * Input:   hRtsp        - The RTSP module to which the array belongs.
 *	        hArray       - The array.
 *	        element      - The current element in the array.
 * Output:  spNextElement	- The next element in the array.
 *
 * Return Value:  RV_OK if the method is completed successfully.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspArrayGetNext(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspArrayHandle	hArray,
					IN	void				*element,
					OUT	void				**pNextElement);

/*@**************************************************************************
 * RvRtspArrayHandleInitArray (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 * This function allocates memory for an array and returns an array
 * handle.
 *
 * Arguments:
 * Input:   hRtsp     - The RTSP module to which the array belongs.
 *          size      - The size of the array.
 * Output:  pHArray   - The handle to the array.
 *
 * Return Value:  RV_OK if the method is completed successfully.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspArrayHandleInitArray(
                 IN RvRtspHandle           hRtsp,
                 IN RvSize_t               elemSize,
                 IN RvUint                 numElem,
                 IN RvChar                 *elemInfo,
                 OUT RvRtspArrayHandle     *phArray);


/*@**************************************************************************
 * RvRtspArrayHandleAddElement (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 * This function allocates an RA element for use by the array handle.
 *
 * Arguments:
 * Input:   hArray        - The array handle.
 * Output:  elem          - Pointer to the added element.
 *
 * Return Value:  RV_OK if the method is completed successfully.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspArrayHandleAddElement(
                 IN RvRtspArrayHandle     hArray,
                 OUT void                 *elem);


/*@**************************************************************************
 * RvRtspStringHandleSetString (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 *  Sets the given string in an Rpool element (RvRtspStringHandle).
 *
 * Arguments:
 * Input:   hRtsp               - The RTSP Stack handle.
 *          str                 - The string to set in the Rpool.
 *          strLen              - The length of the given string.
 * Output:  hStr                - The handle of the Rpool element.
 *
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspStringHandleSetString(
                    IN RvRtspHandle             hRtsp,
                    IN const RvChar             *str,
                    IN RvUint32                 strLen,
                    INOUT RvRtspStringHandle    *hStr);

/*@**************************************************************************
 * RvRtspStringHandleGetString (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 *  Gets a string from an Rpool element (RvRtspStringHandle).
 *
 * Arguments:
 * Input:   hRtsp               - The RTSP Stack handle.
 *          hStr                - The Rpool element that holds the string.
 *          strLen              - The allocated size of the given string buffer.
 * Output:  str                 - The string retrieved from the Rpool element.
 *          strLen              - The actual size of the retrieved string.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspStringHandleGetString(
                    IN RvRtspHandle             		   hRtsp,
                    IN RvRtspStringHandle                   hStr,
                    OUT RvChar                              *str,
                    INOUT RvUint32                          *strLen);

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
/*@**************************************************************************
 * RvRtspUtilsSetParameterTokens (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 *  This utility API is called when the application wants to build a field
 *  value that contains a ',' delimiter.
 *
 * Arguments:
 * Input:   tokenBufferSize    - The number of tokens in the field value.
 *          tokenBuffer        - The buffer containing the tokens.
 *          paramFieldsLen     - The size of the string allocated by the 
 *                               application to hold the field value.
 * Output:  paramFieldsLen     - The actual len of the field value parameter.
 *          paramFields        - The string containing the requested field 
 *                               value.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspUtilsSetParameterTokens(
		            IN	    RvUint16	    tokenBufferSize,
		            IN	    RvChar	        **tokenBuffer,
		            INOUT	RvUint16	    *paramFieldsLen,
		            OUT		RvChar	        *paramFields);

/*@**************************************************************************
 * RvRtspUtilsGetParameterTokens (RtspUtils)
 * ------------------------------------------------------------------------
 * General:
 *  This utility API is called when the application wants to retrieve parameters
 *  that are separated by ',' from a field value.
 *
 * Arguments:
 * Input:   paramFields         - The string containing the field value.
 *          tokenStrLen         - The size allocated by the application for 
 *                                each token.
 *          tokensBufferSize    - The size of the array of strings allocated  
 *                                by the application to hold the field value
 *                                tokens.
 * Output:  tokenBufferSize     - The actual number of tokens in the 
 *                                tokensBuffer array.
 *          tokenBuffer         - The array containing the tokens of the field 
 *                                value.
 *
 * Return Value: RV_OK  if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspUtilsGetParameterTokens(
		            IN		RvChar	        *paramFields,
		            IN		RvUint16	    tokenStrLen,
		            INOUT	RvUint16	    *tokenBufferSize,
		            OUT		RvChar	        **tokenBuffer);

#endif /* RV_RTSP_USE_RTSP_MSG == RV_TRUE */


#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_TYPES_UTILS_H */



