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
 *                              <RtspUtilsInternal.h>
 *
 *  Internal RTSP module utils file, provides RPool memory utilities (for queues
 *	and strings).
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							8/1/04
 *
 *********************************************************************************/

#ifndef _RTSP_UTILS_INTERNAL_H
#define _RTSP_UTILS_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "rvtypes.h"
#include "rpool.h"
#include "rvqueue.h"


/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

/* used for message parsing */
#define CR_CHARACTER	'\r'	/* carriage return character	*/
#define LF_CHARACTER	'\n'	/* line feed character			*/

#define RV_ERROR_LIBCODE_RTSP	13	/* should be moved to rverror.h 	*/
#define RV_CCORE_MODULE_RTSP	30	/* should be moved to rvccoredefs.h */

#define RvRtspErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_RTSP, RV_CCORE_MODULE_RTSP, (_e))


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/


/* Memory Queue Functions */

/**************************************************************************
 * RtspUtilsClearMemoryQueue
 * ------------------------------------------------------------------------
 * General: This function clears a memory queue, releasing it's elements
 *			from the RPool memory.
 *
 * Return Value:	RV_OK - If the method is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- the memory pool containing the queue elements.
 *				pQueue		- the Queue to clear.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
void RtspUtilsClearMemoryQueue(
		IN	HRPOOL	hRPool,
		IN	RvQueue *pQueue);

/* String Parsing Functions */

/**************************************************************************
 * RtspUtilsGetIpAddressFromUri
 * ------------------------------------------------------------------------
 * General: This function gets an IP address or domain name from a URI.
 *
 * Return Value:	RV_OK - If the method is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	strUri		- buffer containing the URI string.
 *				bufferSize	- size of the URI buffer.
 * OUTPUT	:	buffer		- will be set to the host address.
 *				pPort		- will be set to the IP port.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspUtilsGetIpAddressFromUri(
            IN  const RvChar    *strUri,
            IN  RvSize_t        bufferSize,
            OUT RvChar          *buffer,
            OUT RvUint16        *pPort);



/* String Manipulation Functions */

/**************************************************************************
 * RtspUtilsRPOOLELEMGetCell
 * ------------------------------------------------------------------------
 * General: This function returns the character at the specified offset of
 *			elem.
 *
 * Return Value:	Returns RV_TRUE if the character exists, RV_FALSE
 *					if not (if the length of the element is smaller than
 *					the specified offset).
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool			- the hRPool the element is in.
 *				elem			- the RPool element to get the char from.
 *				offset			- offset from start of element.
 * OUTPUT	:	pCh				- will be set to the specified character.
 * INOUT	:	None.
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMGetCell(
			IN	HRPOOL		hRPool,
			IN	HRPOOLELEM	elem,
			IN	RvSize_t	offset,
			OUT	RvChar		*pCh);


/**************************************************************************
 * RtspUtilsRPOOLELEMSetCell
 * ------------------------------------------------------------------------
 * General: This function sets the value of the specified character of the
 *			element
 *
 * Return Value:	Returns RV_TRUE if the character exists, RV_FALSE
 *					if not (if the length of the element is smaller than
 *					the specified offset).
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool			- the hRPool the element is in.
 *				elem			- the RPool element to get the char from.
 *				offset			- offset from start of element.
 *				ch				- value for the specified character.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMSetCell(
			IN	HRPOOL		hRPool,
			IN	HRPOOLELEM	elem,
			IN	RvSize_t	offset,
			IN	RvChar		ch);



/**************************************************************************
 * RtspUtilsRPOOLELEMCopyInternal
 * ------------------------------------------------------------------------
 * General: This function copies data from one RPool element to another
 *
 * Return Value:	Returns RV_TRUE if successfull, RV_FALSE if not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool				- the hRPool the element is in.
 *				hDestination		- the destination element.
 *				destinationOffset	- the offset in destination to copy to.
 *				hSource				- the source element.
 *				sourceOffset		- the offset in source to copy from.
 *				length				- the length to copy.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMCopyInternal(
			IN	HRPOOL		hRPool,
			IN	HRPOOLELEM	hDestination,
			IN	RvSize_t	destinationOffset,
			IN	HRPOOLELEM	hSource,
			IN	RvSize_t	sourceOffset,
			IN	RvSize_t	length);



/**************************************************************************
 * RtspUtilsStrChr
 * ------------------------------------------------------------------------
 * General: This function is modeled after the string.h function strchr,
 *			it finds the first occurrence of ch in pBuffer (starting from
 *			startOffset).
 *			Note: If ch is not found, RV_FALSE is returned and pOffset is
 *			set to the end-of-string '0' character or to bufferLength if
 *			no end-of-string was found.
 *
 * Return Value:	Returns RV_TRUE if ch was found in pChunk, RV_FALSE if
 *					not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pBuffer			- the buffer in which to look for ch.
 *				startOffset		- the offset from start of the buffer to
 *								start looking from.
 *				bufferLength	- length of the buffer.
 *				ch				- the character to look for.
 * OUTPUT	:	pOffset			- will be set to the offset where ch is.
 * INOUT	:	None.
 *************************************************************************/
RvBool RtspUtilsStrChr(
			IN	RvChar		*pBuffer,
			IN	RvSize_t	startOffset,
			IN	RvSize_t	bufferLength,
			IN	RvChar		ch,
			OUT	RvInt32 	*pOffset);


/**************************************************************************
 * RtspUtilsStrDelimiters
 * ------------------------------------------------------------------------
 * General: This function finds the first delimiter in pBufferChunk
 *			(starting from startOffset).
 *			If a delimiter is not found in the string, RV_FALSE is returned
 *			and pOffset is set to the end of string '0' character or to
 *			bufferLength if no end-of-string found.
 *			The function checks up to bufferLength characters, or until an
 *			end of string ('0') is encountered.
 *			If negativeDelimeters is set, the delimiter is all the
 *			characters not in the delimiter list.
 *			Note: NULL termination is considered as a delimiter as well,
 *			meaning, if found we return RV_TRUE;
 *
 * Return Value:	Returns RV_TRUE if ch was found in pChunk, RV_FALSE if
 *					not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pBuffer				- the searched buffer.
 *				startOffset			- the offset from start of the buffer
 *									to start looking from.
 *				bufferLength		- length of the buffer.
 *				strDelimiters		- the delimiter characters to look for.
 *				negativeDelimiters	- if set, look for characters NOT in
 *									the delimiter string.
 * OUTPUT	:	pOffset				- will be set to the offset where a
 *									delimiter was found.
 * INOUT	:	None.
 *************************************************************************/
RvBool RtspUtilsStrDelimiters(
			IN	RvChar		*pBuffer,
			IN	RvSize_t	startOffset,
			IN	RvSize_t	bufferLength,
			IN	RvChar		*strDelimiters,
			IN	RvBool		negativeDelimiters,
			OUT	RvSize_t	*pOffset);


/**************************************************************************
 * RtspUtilsRPOOLELEMStrLen
 * ------------------------------------------------------------------------
 * General: This function is modeled after the string.h function strlen,
 *			it finds the length of the string in hElement (starting from
 *			startOffset). If '\0' is not found, the length of the remainder
 *			of the element is returned.
 *
 * Return Value:	Returns the length of the string in the element.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool				- the hRPool the element is in.
 *				hElement			- the RPool element.
 *				startOffset			- the offset to start from.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvSize_t RtspUtilsRPOOLELEMStrLen(
			IN	HRPOOL		hRPool,
			IN	HRPOOLELEM	hElement,
			IN	RvSize_t	startOffset);


/**************************************************************************
 * RtspUtilsRPOOLELEMStrChr
 * ------------------------------------------------------------------------
 * General: This function is modeled after the string.h function strchr,
 *			it finds the first occurrence of ch in elem (starting from
 *			startOffset). If ch is not found, RV_FALSE is returned.
 *			Note: if RV_FALSE is returned, pOffset value is undefined.
 *
 * Return Value:	Returns RV_TRUE if ch was found in elem, RV_FALSE if not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool				- the hRPool the searched element is in.
 *				elem				- the RPool element to search in.
 *				startOffset			- the offset to start looking from.
 *				ch					- the character to look for.
 * OUTPUT	:	pOffset				- will be set to the offset where ch is.
 * INOUT	:	None.
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMStrChr(
			IN	HRPOOL		hRPool,
			IN	HRPOOLELEM	elem,
			IN	RvSize_t	startOffset,
			IN	RvChar		ch,
			OUT	RvSize_t	*pOffset);




/**************************************************************************
 * RtspUtilsRPOOLELEMStrStr
 * ------------------------------------------------------------------------
 * General: This function is modeled after the string.h function strstr,
 *			it finds the first occurrence of str in elem, looking at the
 *			specified string in the element (characters until the '\0'
 *			character, starting from offset). If str is found, the function
 *			sets pOffset to the offset of the first occurrence of str in
 *			elem.
 *
 * Return Value:	Returns RV_TRUE if found, otherwise RV_FALSE
 *					If str is NULL or empty, RV_FALSE is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool			- the hRPool the searched element is in.
 *				elem			- the RPool element to search in.
 *				elementOffset	- offset to start looking from.
 *				str				- the string to find in the element.
 * OUTPUT	:	pOffset			- will be set to the offset where the found
 *								string starts.
 * INOUT	:
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMStrStr(
			IN	HRPOOL		hRPool,
			IN	HRPOOLELEM	elem,
			IN	RvSize_t	elementOffset,
			IN	RvChar		*str,
			OUT	RvSize_t	*pOffset);



/**************************************************************************
 * RtspUtilsRPOOLELEMStrDelimiters
 * ------------------------------------------------------------------------
 * General: This function is built similarly to RtspUtilsRPOOLELEMStrChr,
 *			it finds the first occurrence of a character from the delimiters
 *			list in elem (starting from	startOffset) and returns it's offset.
 *			If ch is not found, RV_FALSE is returned.
 *			Note: if negativeDelimiters is set, the delimiter characters are
 *			all characters not in the delimiter string.
 *			Note: end-of-string is also treated as a delimiter, i.e. if
 *			it is encountered the function returns RV_TRUE.
 *			Note: if RV_FALSE is returned, pOffset value is undefined.
 *
 * Return Value:	Returns RV_TRUE if a delimiter was found in the element,
 *					RV_FALSE if not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool				- the hRPool the searched element is in.
 *				elem				- the RPool element to search in.
 *				offset				- the offset to start looking from.
 *				strDelimiters		- the character to look for.
 *				negativeDelimiters	- if set, look for characters NOT in
 *									the delimiter string.
 * OUTPUT	:	pOffset				- will be set to the delimiter offset.
 * INOUT	:	None.
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMStrDelimiters(
			IN	HRPOOL			hRPool,
			IN	HRPOOLELEM		elem,
			IN	RvSize_t		offset,
			IN	RvChar			*strDelimiters,
			IN	RvBool			negativeDelimiters,
			OUT	RvSize_t		*pOffset);



/**************************************************************************
 * RtspUtilsRPOOLELEMGetToken
 * ------------------------------------------------------------------------
 * General: This function finds a token in the first string of the element
 *			(all characters until a delimiter is reached) and copies it to
 *			strBuffer.
 *			The function checks up to bufferLength characters.
 *			Note: If negativeDelimeters is set, the token is all the
 *			characters until a character not in the delimiter list is found.
 *			Note: the end of string is also treated as a delimiter, i.e.
 *			if encountered the function returns RV_TRUE.
 *			Note: if a token was not found, value in pTokenLength is
 *			not changed.
 *
 * Return Value:	RV_TRUE if a delimiter was found, RV_FALSE otherwise.
 *					The function
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool				- the hRPool the element is in.
 *				elem				- the RPool element to get the token from.
 *				elementOffset		- offset to start looking from.
 *				strDelimiters		- string containing the delimiters.
 *				negativeDelimiters	- if set, look for characters not in
 *									strDelimiters as string delimiters.
 *				strBuffer			- the buffer to put the token in.
 *				bufferLength		- length of the buffer to put the token in.
 * OUTPUT	:	pTokenLength		- will be set to the length of the token.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvBool RVCALLCONV RtspUtilsRPOOLELEMGetToken(
            IN  HRPOOL          hRPool,
            IN  HRPOOLELEM      elem,
            IN  RvSize_t        elementOffset,
            IN  RvChar          *strDelimiters,
            IN  RvBool          negativeDelimiters,
            IN  RvChar          *strBuffer,
            IN  RvSize_t        bufferLength,
            OUT RvSize_t        *pTokenLength);

/**************************************************************************
 * RtspUtilsHPOOLELEMStrCpy
 * ------------------------------------------------------------------------
 * General: Copies from an RPool element into a string buffer.
 *
 * Return Value:	RV_TRUE if the string was successfully copied, RV_FALSE
 *					otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool				- the hRPool the element is in.
 *				hElement			- the RPool element to get the string from.
 *				elementOffset		- offset to start copying from.
 *				strBuffer			- the buffer to put the string in.
 *				bufferLength		- length of the buffer to put the string in.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvBool RVCALLCONV RtspUtilsHPOOLELEMStrCpy(
            IN  HRPOOL      hRPool,
            IN  HRPOOLELEM  hElement,
            IN  RvSize_t    elementOffset,
            IN  RvChar      *strBuffer,
            IN  RvSize_t    bufferLength);

/**************************************************************************
 * RtspUtilsHPOOLELEMAppend
 * ------------------------------------------------------------------------
 * General: Appends the content of the source element into the destination
 *			element.
 *
 * Return Value:	RV_TRUE if the element was successfully appended,
 *					RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool			- the RPool to which the elements belong.
 *				hDestination	- the element to append into.
 *				hSource			- the element to append from.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvBool RtspUtilsHPOOLELEMAppend(
			IN	HRPOOL		hRPool,
			IN	HRPOOLELEM	hDestination,
			IN	HRPOOLELEM	hSource);

/* printing functions */

#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
/**************************************************************************
 * printCharBuffer
 * ------------------------------------------------------------------------
 * General: Prints character by character a given buffer.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	buf			- The character buffer to print.
 *				hStr	    - The buffer's length.
 *              msa         - The log source.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI void RVCALLCONV
printCharBuffer(RvChar *buf, RvInt32 len, RvLogSource * msa);
#endif

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RTSP_UTILS_INTERNAL_H */



