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
 *                              <RvRtspUtils.c>
 *
 *  Utility functions for RTSP types access, converstion and manipulation.
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							8/1/04
 *
 *********************************************************************************/


#if defined(__cplusplus)
extern "C" {
#endif


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/

#include "rvtypes.h"
#include "rvstdio.h"
#include "rvlog.h"
#include "rpool.h"
#include "ra.h"



#include "RvRtspCommonTypes.h"
#include "RtspUtilsInternal.h"
#include "RvRtspUtils.h"
#include "RtspObject.h"


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



/**************************************************************************
 * RvRtspStrlen
 * ------------------------------------------------------------------------
 * General: 
 * This function returns the length of an RvRtspString.
 *
 * Arguments:
 * Input:  hRtsp     - The Rtsp module the string belongs to.
 *         hStr      - The RvRtspString.
 * Output: pStrSize  - The length of the string.
 * 
 * Return Value: RV_OK - If the method is successfully completed,
 *				 Negative values otherwise.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspStrlen(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspStringHandle	hStr,
					OUT	RvSize_t			*pStrSize)
{
	HRPOOL		hRPool;
	HRPOOLELEM	hElement;

	if ((hRtsp == NULL) || (hStr == NULL) || (pStrSize == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	hRPool	= ((RvRtsp*)hRtsp)->hRPool;
	hElement= (HRPOOLELEM)hStr;

	/* returning the string length			*/
	*pStrSize = RtspUtilsRPOOLELEMStrLen(hRPool, hElement, 0);
	return RV_OK;
}



/**************************************************************************
 * RvRtspStrcpy
 * ------------------------------------------------------------------------
 * General: This function copies the RvRtspString into the specified buffer.
 *
 * Arguments:
 * Input:   hRtsp      - The Rtsp module the string belongs to.
 *          hStr       - The RvRtspString.
 *          bufferSize - The length of the destination buffer.
 * Output:  pBuffer    - The destination buffer.
 * 
 * Return Value:  RV_OK - If the method is successfully completed,
 *                Negative values otherwise.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspStrcpy(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspStringHandle	hStr,
					IN	RvSize_t			bufferSize,
					OUT	RvChar				*pBuffer)
{
	HRPOOL		hRPool = NULL;
	HRPOOLELEM	hElement = NULL;
    RvBool      bResult = RV_FALSE;
    RvStatus    status = RV_OK;

	if ((hRtsp == NULL) || (hStr == NULL) || (pBuffer == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	hRPool	= ((RvRtsp*)hRtsp)->hRPool;
	hElement= (HRPOOLELEM)hStr;

	/* copying the string to the buffer		*/
	bResult = RtspUtilsHPOOLELEMStrCpy(hRPool, hElement, 0, pBuffer, bufferSize);

    if (bResult != RV_TRUE)
        status = RvRtspErrorCode(RV_ERROR_UNKNOWN);

    return status;
    
}

/**************************************************************************
 * RvRtspBloblen
 * ------------------------------------------------------------------------
 * General: 
 * This function returns RvRtspBlob object's length.
 *
 * Arguments:
 * Input:   hRtsp     - The Rtsp module the blob belongs to.
 *          hBlob     - The RvRtspBlob.
 * Output:  pBlobSize - The length of the blob.
 *
 * Return Value:  RV_OK - If the method is successfully completed,
 *                Negative values otherwise.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspBloblen(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspBlobHandle	hBlob,
					OUT	RvSize_t			*pBlobSize)
{
	HRPOOL		hRPool;
	int			result;

	if ((hRtsp == NULL) || (hBlob == NULL) || (pBlobSize == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	hRPool	= ((RvRtsp*)hRtsp)->hRPool;

	/* getting the blob size				*/
	result = rpoolChunkSize(hRPool, (HRPOOLELEM)hBlob);

	if (result < 0)
		return RvRtspErrorCode(RV_ERROR_BADPARAM);

	/* returning the blob size				*/
	*pBlobSize = (RvSize_t)result;
	return RV_OK;
}



/**************************************************************************
 * RvRtspBlobcpy
 * ------------------------------------------------------------------------
 * General: 
 * This function copies the RvRtspBlob into the specified buffer.
 *
 * Arguments:
 * Input:	hRtsp			- The Rtsp module the blob belongs to.
 *			hBlob			- The blob.
 *			bufferSize		- The size of the destination buffer.
 * Output:	pBuffer			- The destination buffer.
 * 
 * Return Value:  RV_OK - If the method is successfully completed,
 *                Negative values otherwise.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspBlobcpy(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspBlobHandle	hBlob,
					IN	int					bufferSize,
					OUT	RvChar				*pBuffer)
{
	HRPOOL		hRPool;
	int			length;

	if ((hRtsp == NULL) || (hBlob == NULL) || (pBuffer == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	hRPool	= ((RvRtsp*)hRtsp)->hRPool;

	/* getting the blob size				*/
	length = rpoolChunkSize(hRPool, (HRPOOLELEM)hBlob);

	if ((length < 0) || (length > bufferSize))
		return RvRtspErrorCode(RV_ERROR_BADPARAM);

	/* copying the blob to the buffer		*/
	rpoolCopyToExternal(hRPool, pBuffer, (HRPOOLELEM)hBlob, 0, length);

	return RV_OK;
}

/*************************************************************************
 * RvRtspBlobHandleSetBlob
 * ------------------------------------------------------------------------
 * General: 
 * This API is used to set a blob into an rpool element (pointed by 
 * stringhandle).
 *
 * Arguments:
 * Input:   hRtsp    - The handle to the RvRtsp structure.
 *          str      - The string to be changed to a String Handle.
 * Output:  phStr    - The blobHandle to be set 
 *
 * Return Value:  RV_OK - If the method is successfully completed,
 *                Negative values otherwise.
 *************************************************************************/
RVAPI  
RvStatus RVCALLCONV RvRtspBlobHandleSetBlob(
                 IN RvRtspHandle           hRtsp,
                 IN const RvChar           *str,                         
                 OUT RvRtspBlobHandle    *phStr)
{

    RvRtsp *pRtsp = (RvRtsp *)hRtsp;

    if ((pRtsp == NULL) || (str == NULL) )
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    *phStr      =  (RvRtspBlobHandle)rpoolAllocCopyExternal(pRtsp->hRPool,
                                                      0,
                                                      str,
                                                      (RvInt32)strlen(str));

    return RV_OK;
}

/**************************************************************************
 * RvRtspArraySize
 * ------------------------------------------------------------------------
 * General: This function returns the size of the array.
 *
 * Arguments:
 * Input:   hRtsp       - The Rtsp module the array belongs to.
 *	        hArray      - The array.
 * Output:  pArraySize  - The size of the array.
 *
 * Return Value:  RV_OK - If the method is successfully completed,
 *	              Negative values otherwise.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspArraySize(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspArrayHandle	hArray,
					OUT	RvSize_t			*pArraySize)
{
	HRA			hRa;
	int			size;

	if ((hRtsp == NULL) || (hArray == NULL) || (pArraySize == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);


	hRa = (HRA)hArray;

	/* getting the RA size					*/
	size = raCurSize(hRa);

	if (size < 0)
		return RvRtspErrorCode(RV_ERROR_BADPARAM);

	*pArraySize = (RvSize_t)size;
	return RV_OK;
}



/**************************************************************************
 * RvRtspArrayGetFirst
 * ------------------------------------------------------------------------
 * General: This function returns the first element of the array.
 *
 * Arguments:
 * Input:   hRtsp          - The Rtsp module the array belongs to.
 *	        hArray         - The array.
 * Output:  pFirstElement  - The first element in the array.
 *
 * Return Value:  RV_OK - If the method is successfully completed,
 *                Negative values otherwise.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspArrayGetFirst(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspArrayHandle	hArray,
					OUT	void				**pFirstElement)
{
	HRA		hRa;
	int		location;

	if ((hRtsp == NULL) || (hArray == NULL) || (pFirstElement == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	hRa = (HRA)hArray;


	/* getting the first element location	*/
	location = raGetNext(hRa,-1);

	if (location < 0)
		return RvRtspErrorCode(RV_ERROR_OUTOFRANGE);

	/* returning the element				*/
	*pFirstElement = (void*)raGet(hRa, location);

	return RV_OK;
}



/**************************************************************************
 * RvRtspArrayGetNext
 * ------------------------------------------------------------------------
 * General: 
 * This function returns the next element of the array.
 *
 * Arguments:
 * Input:   hRtsp        - The Rtsp module the array belongs to.
 *	        hArray       - The array.
 *	        element      - The current array element.
 * Output:  spNextElement	- The next element in the array.
 *
 * Return Value:  RV_OK - If the method is successfully completed.
 *                Negative values otherwise.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspArrayGetNext(
					IN 	RvRtspHandle		hRtsp,
					IN	RvRtspArrayHandle	hArray,
					IN	void				*element,
					OUT	void				**pNextElement)
{
	int		location;

	if ((hRtsp == NULL) || (hArray == NULL) || (element == NULL) || (pNextElement == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	/* getting the current location			*/
	location = raGetByPtr((HRA)hArray, element);

	if (location < 0)
		return RvRtspErrorCode(RV_ERROR_BADPARAM);

	/* getting the next location			*/
	location = raGetNext((HRA)hArray,location);

	if (location < 0)
		return RvRtspErrorCode(RV_ERROR_OUTOFRANGE);

	/* returning the element				*/
	*pNextElement = (void*)raGet((HRA)hArray, location);

	return RV_OK;

}

/**************************************************************************
 * RvRtspArrayHandleInitArray
 * ------------------------------------------------------------------------
 * General:
 * This function allocates memory for an array and returns an array
 * handle.  
 * 
 * Arguments:
 * Input:   hRtsp     - The Rtsp module the array belongs to.
 *          size      - The size of the array.
 * Output:  pHArray   - The handle to the array.
 *
 * Return Value:  RV_OK - If the method is successfully completed.
 *                Negative values otherwise.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspArrayHandleInitArray(
                 IN RvRtspHandle           hRtsp,
                 IN RvSize_t               elemSize,
                 IN RvUint                 numElem,
                 IN RvChar                 *elemInfo,
                 OUT RvRtspArrayHandle     *phArray)
{

    RvRtsp *pRtsp = (RvRtsp *)hRtsp;

    *phArray = (RvRtspArrayHandle)raConstruct((RvInt)elemSize, (RvInt)numElem, RV_TRUE, elemInfo, pRtsp->pLogMgr);

    return RV_OK;
}

/**************************************************************************
 * RvRtspArrayHandleAddElement
 * ------------------------------------------------------------------------
 * General: 
 * This function allocates an RA element for use by the for the 
 * arrayHandle.
 * 
 * Arguments:
 * Input:   hArray        - The array handle.
 * Output:  elem          - Pointer to the element added.
 * 
 * Return Value:  RV_OK - If the method is successfully completed,
 *                Negative values otherwise.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspArrayHandleAddElement(
                 
                 IN RvRtspArrayHandle     hArray,
                 OUT void                 *elem)
{
     raAdd((HRA)hArray, (RAElement*)elem);
     return RV_OK;
}

/*************************************************************************
 * RvRtspStringHandleSetString
 * ----------------------------------------------------------------------------
 * General: 
 *  Sets the given string into an Rpool element (RvRtspStringHandle).
 * 
 * Arguments:
 * Input:   hRtsp               - The rtsp stack handle.
 *          str                 - The string to set in the Rpool.
 *          strLen              - The length of the given string.
 * Output:  hStr                - The handle of the Rpool element.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI  
RvStatus RVCALLCONV RvRtspStringHandleSetString(
                    IN      RvRtspHandle        hRtsp,
                    IN      const RvChar        *str, 
                    IN      RvUint32            strLen,
                    INOUT   RvRtspStringHandle  *hStr)
{
    RvRtsp *pThis = (RvRtsp *)hRtsp;
    

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    *hStr = (RvRtspStringHandle)rpoolAllocCopyExternal(pThis->hRPool, 
                                                       0, 
                                                       (void *)str, 
                                                       (RvInt32)strLen);
    return RV_OK;
}

/******************************************************************************
 * RvRtspStringHandleGetString
 * ----------------------------------------------------------------------------
 * General: 
 *  Gets a string from an Rpool element (RvRtspStringHandle).
 * 
 * Arguments:
 * Input:   hRtsp               - The rtsp stack handle.
 *          hStr                - The Rpool element that holds the string.
 *          strLen              - The allocated size of the given string buffer.
 * Output:  str                 - The string retrieved from the Rpool element.
 *          strLen              - The actual size of the retrieved string.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI  
RvStatus RVCALLCONV RvRtspStringHandleGetString(
                    IN      RvRtspHandle        hRtsp,
                    IN      RvRtspStringHandle  hStr,
                    OUT     RvChar              *str,
                    INOUT   RvUint32            *strLen)
{
    RvRtsp      *pThis = (RvRtsp *)hRtsp;
    RvUint32    stringLen, tmpStrLen;
    RvStatus    status = RV_OK;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    stringLen = (RvUint32)rpoolChunkSize(pThis->hRPool, (HRPOOLELEM)hStr);
    tmpStrLen = stringLen;
    if (stringLen >= *strLen)
    {
        status = RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
        stringLen = (*strLen) - 1;
    }

    memset(str, 0, *strLen);

    rpoolCopyToExternal(pThis->hRPool, (void *)str, (HRPOOLELEM)hStr, 0, (RvInt32)stringLen);

    *strLen = tmpStrLen;

    return status;

}

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
/******************************************************************************
 * RvRtspUtilsSetParameterTokens
 * ----------------------------------------------------------------------------
 * General: 
 *  This utility API is called when the application wants to build a field
 *  value that contains a ',' delimiter.
 * 
 * Arguments:
 * Input:   tokenBufferSize    - The number of tokens in the field value.
 *          tokenBuffer        - The buffer containing the tokens.
 *          paramFieldsLen     - The size of the string allocated by the application to
 *                                hold the field value.
 * Output:  paramFieldsLen     - The actual len of the field value parameter.
 *          paramFields        - The string containing the requested field value.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI 
RvStatus RVCALLCONV RvRtspUtilsSetParameterTokens(
		            IN	    RvUint16	    tokenBufferSize,
		            IN	    RvChar	        **tokenBuffer,
		            INOUT	RvUint16	    *paramFieldsLen,
		            OUT		RvChar	        *paramFields)
{
    RvUint32    i;
    RvUint16    len = *paramFieldsLen;
    RvChar      *pBuf = paramFields;

    memset (paramFields, 0 , len);

    for (i = 0; i < tokenBufferSize; ++i)
    {
        if ((pBuf + strlen(tokenBuffer[i]) - paramFields) > len)
            return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);

        /* copy the current token */
        memcpy(pBuf, tokenBuffer[i], strlen(tokenBuffer[i]));
        pBuf += strlen(tokenBuffer[i]);

        /* Set the delimiter */
        if (i < (RvUint32)tokenBufferSize - 1)
        {
            if ((pBuf + 2 - paramFields) > len)
                return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
            memcpy(pBuf, ", ", 2);
            pBuf += 2;
        }
    }

    *paramFieldsLen = (RvUint16)(pBuf - paramFields);

    return RV_OK;
}

/******************************************************************************
 * RvRtspUtilsGetParameterTokens
 * ----------------------------------------------------------------------------
 * General: 
 *  This utility API is called when the application wants to retrieve parameters
 *  that are separated by ',' from a field value.
 *
 * Arguments:
 * Input:   paramFields         - The string containing the field value.
 *          tokenStrLen         - The size allocated by the application for each token.
 *          tokensBufferSize    - The size of the array of strings allocated by the 
 *                                application to hold the field value tokens. 
 * Output:  tokenBufferSize     - The actual number of tokens in the tokensBuffer
 *                                array.
 *          tokenBuffer         - The array containing the tokens of the field value.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI 
RvStatus RVCALLCONV RvRtspUtilsGetParameterTokens(
		            IN		RvChar	        *paramFields,
		            IN		RvUint16	    tokenStrLen,
		            INOUT	RvUint16	    *tokenBufferSize,
		            OUT		RvChar	        **tokenBuffer)
{
    RvUint32    i = 0;
    RvUint16    size = *tokenBufferSize;
    RvChar      *delim = ", \0";
    RvChar      *token = strtok(paramFields, delim);

    while (token != NULL)
    {
        if (i >= size || strlen(token) >= tokenStrLen)
            return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
        memset(tokenBuffer[i], 0, tokenStrLen);
        memcpy(tokenBuffer[i], token, strlen(token));
        ++i;
        token = strtok(NULL, delim);
    }
    *tokenBufferSize = (RvUint16)i;

    return RV_OK;
}

#endif /* RV_RTSP_USE_RTSP_MSG == RV_TRUE */

/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/

#if defined(__cplusplus)
}
#endif
