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
 *                              <RtspUtilsInternal.c>
 *
 *  Internal RTSP module utils file, provides RPool memory utilities (for queues
 *  and strings).
 *
 *    Author                         Date
 *    ------                        ------
 *      Tom                         8/1/04
 *
 *********************************************************************************/


#if defined(__cplusplus)
extern "C" {
#endif


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/

#include "rvtypes.h"
#include "rvlog.h"
#include "rvstdio.h"

#include "RtspUtilsInternal.h"

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



/* Memory Queue Functions */

/**************************************************************************
 * RtspUtilsClearMemoryQueue
 * ------------------------------------------------------------------------
 * General: This function clears a memory queue, releasing it's elements
 *          from the RPool memory.
 *
 * Return Value:    RV_OK - If the method is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool      - the memory pool containing the queue elements.
 *              pQueue      - the Queue to clear.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
void RtspUtilsClearMemoryQueue(
        IN  HRPOOL  hRPool,
        IN  RvQueue *pQueue)
{
    RvStatus    result;
    HRPOOLELEM  queueElement;
	RvBool		cont = RV_TRUE;

    while (cont)
    {
        result= RvQueueReceive( pQueue,
                                RV_QUEUE_NOWAIT,
                                sizeof(HRPOOLELEM),
                                (void*)(&queueElement));

        if (RvErrorGetCode(result) == RV_QUEUE_ERROR_EMPTY)
            break;

        rpoolFree(hRPool, queueElement);
    }

    RvQueueClear(pQueue);
}


/* String Parsing Functions */

/**************************************************************************
 * RtspUtilsGetIpAddressFromUri
 * ------------------------------------------------------------------------
 * General: This function gets an IP address or domain name from a URI.
 *
 * Return Value:    RV_OK - If the method is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   strUri      - buffer containing the URI string.
 *              bufferSize  - size of the URI buffer.
 * OUTPUT   :   buffer      - will be set to the host address.
 *              pPort       - will be set to the IP port.
 * INOUT    :   None.
 *************************************************************************/
 RVINTAPI RvStatus RtspUtilsGetIpAddressFromUriEx(IN  char const* url, 
		             OUT RvChar          *address,
		            OUT RvUint16        *portNum,
				 RvChar const ** urlSuffix,
			   OUT RvChar              *username,
                        OUT RvChar              *password,
                        OUT RvBool               *IsUser) 
{
	//char  *username;
	//char  *password;
	char const* prefix = "rtsp://";
	unsigned const prefixLength = 7;
	unsigned int parseBufferSize = 100;
	char parseBuffer[100];
	char * from = &url[7];
	char * colonPasswordStart = NULL;
	char * p;
	char * usernameStart ;
	unsigned usernameLen ;
	char * passwordStart;
	unsigned passwordLen;
	char* to;
	unsigned i,j;
	char nextChar;
	int portNumInt;
	do 
	{
		// Parse the URL as "rtsp://[<username>[:<password>]@]<server-address-or-name>[:<port>][/<stream-name>]"

		if (strncmp(url, prefix, prefixLength) != 0) 
		{
			printf("URL is not of the form %s \n", prefix);
			break;
		}



		// Check whether "<username>[:<password>]@" occurs next.
		// We do this by checking whether '@' appears before the end of the URL, or before the first '/'.
		//*username = *password = NULL; // default return values

		for (p = from; *p != '\0' && *p != '/'; ++p) 
		{
			if (*p == ':' && colonPasswordStart == NULL)
			{
				colonPasswordStart = p;
			}
			else if (*p == '@') 
			{
				// We found <username> (and perhaps <password>).  Copy them into newly-allocated result strings:
				if (colonPasswordStart == NULL) colonPasswordStart = p;

				usernameStart = from;
				usernameLen = colonPasswordStart - usernameStart;
				//*username = malloc(usernameLen + 1) ; // allow for the trailing '\0'
				if(username !=NULL)
				{
					for ( i = 0; i < usernameLen; ++i) username[i] = usernameStart[i];
					username[usernameLen] = '\0';
					if(IsUser !=NULL)
						*IsUser  = RV_TRUE;
				}

				passwordStart = colonPasswordStart;
				if (passwordStart < p) ++passwordStart; // skip over the ':'
				passwordLen = p - passwordStart;
				//password = malloc(passwordLen + 1); // allow for the trailing '\0'
				if(username !=NULL)
				{
					for ( j = 0; j < passwordLen; ++j) password[j] = passwordStart[j];
					password[passwordLen] = '\0';
				}

				from = p + 1; // skip over the '@'
				break;
			}
		}

		// Next, parse <server-address-or-name>
		to = &parseBuffer[0];
		
		for (i = 0; i < parseBufferSize; ++i) 
		{
			if (*from == '\0' || *from == ':' || *from == '/')
			{
				// We've completed parsing the address
				*to = '\0';
				break;
			}
			*to++ = *from++;
		}
		if (i == parseBufferSize) 
		{
			printf("URL is too long");
			break;
		}
		strcpy(address,parseBuffer);


		*portNum = 554; // default value
		nextChar = *from;
		if (nextChar == ':') 
		{
			
			if (sscanf(++from, "%d", &portNumInt) != 1) 
			{
				printf("No port number follows ':'");
				break;
			}
			if (portNumInt < 1 || portNumInt > 65535) 
			{
				printf("Bad port number");
				break;
			}
			*portNum = portNumInt;
			while (*from >= '0' && *from <= '9') ++from; // skip over port number
		}

		// The remainder of the URL is the suffix:
		if (urlSuffix != NULL) *urlSuffix = from;

		return RV_OK;
	} while (0);

  return RvRtspErrorCode(RV_ERROR_BADPARAM);
}
RVINTAPI RvStatus RVCALLCONV RtspUtilsGetIpAddressFromUri(
            IN  const RvChar    *strUri,
            IN  RvSize_t        bufferSize,
            OUT RvChar          *buffer,
            OUT RvUint16        *pPort)
{
    const RvChar    *ipStrStart;
    RvSize_t        ipStrLength;
    RvInt           rVal;
    if (strncmp(strUri,"rtsp://",strlen("rtsp://")) != 0)
        return RvRtspErrorCode(RV_ERROR_BADPARAM);

    ipStrStart = strUri;
    ipStrStart += strlen("rtsp://");

    /* the end of the host IP is marked by either ":" (before the port), "/"    */
    /* (before the URI absolute path), or '\0' (marking the end of the string)  */
    ipStrLength = strcspn(ipStrStart, ":/");

    if (bufferSize < ipStrLength+1)
        return RvRtspErrorCode(RV_ERROR_BADPARAM);

    strncpy(buffer,ipStrStart,ipStrLength);
    buffer[ipStrLength] = 0;

    /* if there was a ":" at the end of the IP, the following number is the port*/
    *pPort = 554;
    if (sscanf(ipStrStart+ipStrLength,":%d", &rVal) == 1)
        *pPort = (RvUint16)rVal;

    return RV_OK;
}


/* String Manipulation Functions */




/**************************************************************************
 * RtspUtilsRPOOLELEMGetCell
 * ------------------------------------------------------------------------
 * General: This function returns the character at the specified offset of
 *          elem.
 *
 * Return Value:    Returns RV_TRUE if the character exists, RV_FALSE
 *                  if not (if the length of the element is smaller than
 *                  the specified offset).
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool          - the hRPool the element is in.
 *              elem            - the RPool element to get the char from.
 *              offset          - offset from start of element.
 * OUTPUT   :   pCh             - will be set to the specified character.
 * INOUT    :   None.
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMGetCell(
            IN  HRPOOL      hRPool,
            IN  HRPOOLELEM  elem,
            IN  RvSize_t    offset,
            OUT RvChar      *pCh)
{
    RvInt32     chunkLength;
    RvChar      *pChunk;

    chunkLength = rpoolGetPtr(hRPool, elem, offset, (void **)&pChunk);

    if (chunkLength <= 0 )
        return RV_FALSE;

    *pCh = pChunk[0];
    return RV_TRUE;
}


/**************************************************************************
 * RtspUtilsRPOOLELEMSetCell
 * ------------------------------------------------------------------------
 * General: This function sets the value of the specified character of the
 *          element
 *
 * Return Value:    Returns RV_TRUE if the character exists, RV_FALSE
 *                  if not (if the length of the element is smaller than
 *                  the specified offset).
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool          - the hRPool the element is in.
 *              elem            - the RPool element to get the char from.
 *              offset          - offset from start of element.
 *              ch              - value for the specified character.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMSetCell(
            IN  HRPOOL      hRPool,
            IN  HRPOOLELEM  elem,
            IN  RvSize_t    offset,
            IN  RvChar      ch)
{
    RvInt32     chunkLength;
    RvChar      *pChunk;

    chunkLength = rpoolGetPtr(hRPool, elem, offset, (void **)&pChunk);

    if (chunkLength <= 0 )
        return RV_FALSE;

    pChunk[0] = ch;
    return RV_TRUE;
}


/**************************************************************************
 * RtspUtilsRPOOLELEMCopyInternal
 * ------------------------------------------------------------------------
 * General: This function copies data from one RPool element to another
 *
 * Return Value:    Returns RV_TRUE if successfull, RV_FALSE if not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool              - the hRPool the element is in.
 *              hDestination        - the destination element.
 *              destinationOffset   - the offset in destination to copy to.
 *              hSource             - the source element.
 *              sourceOffset        - the offset in source to copy from.
 *              length              - the length to copy.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMCopyInternal(
            IN  HRPOOL      hRPool,
            IN  HRPOOLELEM  hDestination,
            IN  RvSize_t    destinationOffset,
            IN  HRPOOLELEM  hSource,
            IN  RvSize_t    sourceOffset,
            IN  RvSize_t    length)
{
    RvInt32     chunkLength;
    RvInt32     lengthToCopy;
    RvChar      *pChunk;
    HRPOOLELEM  hResult;
    RvInt32 destinationLength;

    if ( (hRPool == NULL) || (hDestination == NULL) || (hSource == NULL) )
        return RV_FALSE;

    destinationLength = rpoolChunkSize(hRPool, hDestination);
    if (destinationLength < 0 )
        return RV_FALSE;

    while (length > 0)
    {
        chunkLength = rpoolGetPtr(hRPool, hSource, sourceOffset,(void**)&pChunk);
        if (chunkLength <= 0)
            return RV_FALSE;

        lengthToCopy = RvMin((RvInt32)length, chunkLength);
        hResult = rpoolCopyFromExternal(hRPool, hDestination, pChunk, destinationOffset, lengthToCopy);
        if (hResult == NULL)
            return RV_FALSE;
        sourceOffset += lengthToCopy;
        destinationOffset += lengthToCopy;
        length -= lengthToCopy;
        /* if there was an error in copying, or we overflowed beyond destination bounderies */
        if (destinationOffset > (RvSize_t)destinationLength)
            return RV_FALSE;

    }
    return RV_TRUE;
}

/**************************************************************************
 * RtspUtilsStrChr
 * ------------------------------------------------------------------------
 * General: This function is modeled after the string.h function strchr,
 *          it finds the first occurrence of ch in pBuffer (starting from
 *          startOffset).
 *          Note: If ch is not found, RV_FALSE is returned and pOffset is
 *          set to the end-of-string '0' character or to bufferLength if
 *          no end-of-string was found.
 *
 * Return Value:    Returns RV_TRUE if ch was found in pChunk, RV_FALSE if
 *                  not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pBuffer         - the buffer in which to look for ch.
 *              startOffset     - the offset from start of the buffer to
 *                              start looking from.
 *              bufferLength    - length of the buffer.
 *              ch              - the character to look for.
 * OUTPUT   :   pOffset         - will be set to the offset where ch is.
 * INOUT    :   None.
 *************************************************************************/
RvBool RtspUtilsStrChr(
            IN  RvChar      *pBuffer,
            IN  RvSize_t    startOffset,
            IN  RvSize_t    bufferLength,
            IN  RvChar      ch,
            OUT RvInt32     *pOffset)
{
    RvSize_t    currOffset = startOffset;

    while (currOffset < bufferLength)
    {
        if (pBuffer[currOffset] == ch)
        {
            /* bingo! character found */
            *pOffset = currOffset;
            return RV_TRUE;
        }

        /* if we found null character we'll return false */
        if (pBuffer[currOffset] == 0)
        {
            /* we mark the null character location so the caller could know that    */
            /* we returned RV_FALSE because we found a null character               */
            *pOffset = currOffset;
            return RV_FALSE;
        }

        currOffset++;
    }

    /* we set the pOffset param to buffer length so the caller could know that we */
    /* we returned RV_FALSE because we did not find any occurance of ch parameter*/
    *pOffset = bufferLength;
    return RV_FALSE;
}



/**************************************************************************
 * RtspUtilsStrDelimiters
 * ------------------------------------------------------------------------
 * General: This function finds the first delimiter in pBufferChunk
 *          (starting from startOffset).
 *          If a delimiter is not found in the string, RV_FALSE is returned
 *          and pOffset is set to the end of string '0' character or to
 *          bufferLength if no end-of-string found.
 *          The function checks up to bufferLength characters, or until an
 *          end of string ('0') is encountered.
 *          If negativeDelimeters is set, the delimiter is all the
 *          characters not in the delimiter list.
 *          Note: NULL termination is considered as a delimiter as well,
 *          meaning, if found we return RV_TRUE;
 *
 * Return Value:    Returns RV_TRUE if ch was found in pChunk, RV_FALSE if
 *                  not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pBuffer             - the searched buffer.
 *              startOffset         - the offset from start of the buffer
 *                                  to start looking from.
 *              bufferLength        - length of the buffer.
 *              strDelimiters       - the delimiter characters to look for.
 *              negativeDelimiters  - if set, look for characters NOT in
 *                                  the delimiter string.
 * OUTPUT   :   pOffset             - will be set to the offset where a
 *                                  delimiter was found.
 * INOUT    :   None.
 *************************************************************************/
RvBool RtspUtilsStrDelimiters(
            IN  RvChar      *pBuffer,
            IN  RvSize_t    startOffset,
            IN  RvSize_t    bufferLength,
            IN  RvChar      *strDelimiters,
            IN  RvBool      negativeDelimiters,
            OUT RvSize_t    *pOffset)
{
    RvSize_t    currOffset = startOffset;
    RvChar      *result;

    while (currOffset < bufferLength)
    {
        result = strchr(strDelimiters,pBuffer[currOffset]);

        /* if the elem cell is null.. the string terminated... and we'll get out with RV_TRUE */
        if ((pBuffer[currOffset] == 0) ||
            ((result != NULL) && (negativeDelimiters == RV_FALSE)) ||
            ((result == NULL) && (negativeDelimiters == RV_TRUE)) )
        {
            /* a delimiter was found */
            *pOffset = currOffset;
            return RV_TRUE;
        }

        currOffset++;
    }

    /* we set the pOffset param to bufferLength so the caller could know that we */
    /* we returned RV_FALSE because we did not find any occurance of ch parameter*/
    *pOffset = bufferLength;
    return RV_FALSE;
}




/**************************************************************************
 * RtspUtilsRPOOLELEMStrLen
 * ------------------------------------------------------------------------
 * General: This function is modeled after the string.h function strlen,
 *          it finds the length of the string in hElement (starting from
 *          startOffset). If '\0' is not found, the length of the remainder
 *          of the element is returned.
 *
 * Return Value:    Returns the length of the string in the element.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool              - the hRPool the element is in.
 *              hElement            - the RPool element.
 *              startOffset         - the offset to start from.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RvSize_t RtspUtilsRPOOLELEMStrLen(
            IN  HRPOOL      hRPool,
            IN  HRPOOLELEM  hElement,
            IN  RvSize_t    startOffset)
{
    RvSize_t terminationOffset;

    if (hElement == NULL)
        return 0;

    RtspUtilsRPOOLELEMStrChr(hRPool,hElement,startOffset, '\0', &terminationOffset);
    return terminationOffset - startOffset;
}



/**************************************************************************
 * RtspUtilsRPOOLELEMStrChr
 * ------------------------------------------------------------------------
 * General: This function is modeled after the string.h function strchr,
 *          it finds the first occurrence of ch in elem (starting from
 *          startOffset). If ch is not found, RV_FALSE is returned.
 *          Note: if RV_FALSE is returned, pOffset value is undefined.
 *
 * Return Value:    Returns RV_TRUE if ch was found in elem, RV_FALSE if not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool              - the hRPool the searched element is in.
 *              elem                - the RPool element to search in.
 *              startOffset         - the offset to start looking from.
 *              ch                  - the character to look for.
 * OUTPUT   :   pOffset             - will be set to the offset where ch is.
 * INOUT    :   None.
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMStrChr(
            IN  HRPOOL      hRPool,
            IN  HRPOOLELEM  elem,
            IN  RvSize_t    startOffset,
            IN  RvChar      ch,
            OUT RvSize_t    *pOffset)
{
    RvSize_t    currOffset = startOffset;
    RvInt32     chunkLength;    /* length of the checked chunk      */
    RvChar      *pChunk;        /* checked chunk buffer             */
	RvBool		cont = RV_TRUE;


    while (cont)
    {
        chunkLength = rpoolGetPtr(  hRPool,
                                    elem,
                                    currOffset,
                                    (void **)&pChunk);

        /* check if we reached end of element */
        if (chunkLength <= 0 )
            return RV_FALSE;

        if (RtspUtilsStrChr(pChunk, 0, chunkLength, ch, (RvInt32*)pOffset) == RV_TRUE)
        {
            *pOffset += currOffset;
            return RV_TRUE;
        }

        /* will mean that we found a null character in the chunk    */
        if (*pOffset != (RvSize_t)chunkLength)
            return RV_FALSE;

        /* if we didn't find anything - advance to next chunk */
        currOffset += chunkLength;
    }

	return RV_TRUE;
}




/**************************************************************************
 * RtspUtilsRPOOLELEMStrStr
 * ------------------------------------------------------------------------
 * General: This function is modeled after the string.h function strstr,
 *          it finds the first occurrence of str in elem, looking at the
 *          specified string in the element (characters until the '\0'
 *          character, starting from offset). If str is found, the function
 *          sets pOffset to the offset of the first occurrence of str in
 *          elem.
 *
 * Return Value:    Returns RV_TRUE if found, otherwise RV_FALSE
 *                  If str is NULL or empty, RV_FALSE is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool          - the hRPool the searched element is in.
 *              elem            - the RPool element to search in.
 *              elementOffset   - offset to start looking from.
 *              str             - the string to find in the element.
 * OUTPUT   :   pOffset         - will be set to the offset where the found
 *                              string starts.
 * INOUT    :
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMStrStr(
            IN  HRPOOL      hRPool,
            IN  HRPOOLELEM  elem,
            IN  RvSize_t    elementOffset,
            IN  RvChar      *str,
            OUT RvSize_t    *pOffset)
{
    RvSize_t    elementLength;
    RvSize_t    searchIndex;
    RvSize_t    searchLength;
    RvSize_t    searchOffset;
    RvChar      ch;
	RvBool		cont = RV_TRUE;

    if ( (hRPool == NULL) || (elem == NULL) || (str == NULL) || (pOffset == NULL) )
        return RV_FALSE;

    searchLength = strlen(str);

    if (searchLength == 0)
        return RV_FALSE;

    elementLength = rpoolChunkSize(hRPool, elem);

    while (cont)
    {
        /* search for the first occurance of the first str character inside elem */
        if (RtspUtilsRPOOLELEMStrChr(hRPool, elem, elementOffset, str[0], &searchOffset) == RV_FALSE)
            return RV_FALSE;

        for (searchIndex = 1; searchIndex < searchLength; searchIndex++)
        {
            /* check if we reached the end of the element */
            if (searchOffset + searchIndex >= elementLength)
                return RV_FALSE;

            if (RtspUtilsRPOOLELEMGetCell(hRPool, elem, searchOffset + searchIndex, &ch) == RV_FALSE)
                return RV_FALSE;

            if (ch == 0)
                return RV_FALSE;

            if (ch != str[searchIndex])
            {
                elementOffset = searchOffset+1;
                break;
            }
        } /* for loop */

        /* toshay horay */
        if (searchIndex == searchLength)
        {
            *pOffset = searchOffset;
            return RV_TRUE;
        }

    } /* while */

	return RV_TRUE;
}


/**************************************************************************
 * RtspUtilsRPOOLELEMStrDelimiters
 * ------------------------------------------------------------------------
 * General: This function is built similarly to RtspUtilsRPOOLELEMStrChr,
 *          it finds the first occurrence of a character from the delimiters
 *          list in elem (starting from startOffset) and returns it's offset.
 *          If ch is not found, RV_FALSE is returned.
 *          Note: if negativeDelimiters is set, the delimiter characters are
 *          all characters not in the delimiter string.
 *          Note: end-of-string is also treated as a delimiter, i.e. if
 *          it is encountered the function returns RV_TRUE.
 *          Note: if RV_FALSE is returned, pOffset value is undefined.
 *
 * Return Value:    Returns RV_TRUE if a delimiter was found in the element,
 *                  RV_FALSE if not.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool              - the hRPool the searched element is in.
 *              elem                - the RPool element to search in.
 *              offset              - the offset to start looking from.
 *              strDelimiters       - the character to look for.
 *              negativeDelimiters  - if set, look for characters NOT in
 *                                  the delimiter string.
 * OUTPUT   :   pOffset             - will be set to the delimiter offset.
 * INOUT    :   None.
 *************************************************************************/
RvBool RtspUtilsRPOOLELEMStrDelimiters(
            IN  HRPOOL          hRPool,
            IN  HRPOOLELEM      elem,
            IN  RvSize_t        offset,
            IN  RvChar          *strDelimiters,
            IN  RvBool          negativeDelimiters,
            OUT RvSize_t        *pOffset)
{
    RvSize_t    currOffset      = offset;
    RvChar      ch;
    RvChar      *result;
	RvBool		cont = RV_TRUE;


    /* search elem for the delimiters                               */
    while (cont)
    {
        cont = RtspUtilsRPOOLELEMGetCell(hRPool, elem, currOffset, &ch);

        if (cont == RV_FALSE)
        {
            /* no more elements - means delimiters were not found   */
            return RV_FALSE;
        }

        /* ch holds the checked character */
        result = strchr(strDelimiters,ch);

        /* if the elem cell is null.. the string terminated... and we'll get out with RV_TRUE */
        if ((ch == 0) ||
            ((result != NULL) && (negativeDelimiters == RV_FALSE)) ||
            ((result == NULL) && (negativeDelimiters == RV_TRUE)) )
        {
            *pOffset = currOffset;
            return RV_TRUE;
        }

        currOffset++;

    } /* while loop */

    return RV_FALSE;
}




/**************************************************************************
 * RtspUtilsRPOOLELEMGetToken
 * ------------------------------------------------------------------------
 * General: This function finds a token in the first string of the element
 *          (all characters until a delimiter is reached) and copies it to
 *          strBuffer.
 *          The function checks up to bufferLength characters.
 *          Note: If negativeDelimeters is set, the token is all the
 *          characters until a character not in the delimiter list is found.
 *          Note: the end of string is also treated as a delimiter, i.e.
 *          if encountered the function returns RV_TRUE.
 *          Note: if a token was not found, value in pTokenLength is
 *          not changed.
 *
 * Return Value:    RV_TRUE if a delimiter was found, RV_FALSE otherwise.
 *                  The function
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool              - the hRPool the element is in.
 *              elem                - the RPool element to get the token from.
 *              elementOffset       - offset to start looking from.
 *              strDelimiters       - string containing the delimiters.
 *              negativeDelimiters  - if set, look for characters not in
 *                                  strDelimiters as string delimiters.
 *              strBuffer           - the buffer to put the token in.
 *              bufferLength        - length of the buffer to put the token in.
 * OUTPUT   :   pTokenLength        - will be set to the length of the token.
 * INOUT    :   None.
 *************************************************************************/
RVINTAPI RvBool RVCALLCONV RtspUtilsRPOOLELEMGetToken(
            IN  HRPOOL          hRPool,
            IN  HRPOOLELEM      elem,
            IN  RvSize_t        elementOffset,
            IN  RvChar          *strDelimiters,
            IN  RvBool          negativeDelimiters,
            IN  RvChar          *strBuffer,
            IN  RvSize_t        bufferLength,
            OUT RvSize_t        *pTokenLength)
{
    RvSize_t    currOffset      = 0;
    RvSize_t    elementLength   = rpoolChunkSize(hRPool, elem);
    RvChar      ch;
    RvChar      *result;
	RvBool		cont = RV_TRUE;

    if (bufferLength == 0)
        return RV_FALSE;

    /* search elem for the delimiters                               */
    while (cont)
    {
        if (currOffset + elementOffset >= elementLength)
        {
            *pTokenLength   = currOffset;
            return RV_FALSE;
        }

        if (currOffset >= bufferLength)
            return RV_FALSE;
        cont = RtspUtilsRPOOLELEMGetCell(hRPool, elem, elementOffset + currOffset, &ch);

        if (cont == RV_FALSE)
        {
            /* no more elements - means delimiters were not found   */
            return RV_FALSE;
        }

        /* ch holds the checked character */
        result = strchr(strDelimiters,ch);

        /* if the elem cell is null.. the string terminated...      */
        /* and we'll get out with RV_TRUE                           */
        if ((ch == 0) ||
            ((result != NULL) && (negativeDelimiters == RV_FALSE)) ||
            ((result == NULL) && (negativeDelimiters == RV_TRUE)) )
        {
            strBuffer[currOffset]   = 0;
            *pTokenLength           = currOffset;
            return RV_TRUE;
        }

        else
        {
            if (currOffset == bufferLength-1)
                return RV_FALSE;

            strBuffer[currOffset] = ch;
        }

        currOffset++;

    } /* while loop */

    return RV_FALSE;
}

/**************************************************************************
 * RtspUtilsHPOOLELEMStrCpy
 * ------------------------------------------------------------------------
 * General: Copies from an RPool element into a string buffer.
 *
 * Return Value:    RV_TRUE if the string was successfully copied, RV_FALSE
 *                  otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool              - the hRPool the element is in.
 *              hElement            - the RPool element to get the string from.
 *              elementOffset       - offset to start copying from.
 *              strBuffer           - the buffer to put the string in.
 *              bufferLength        - length of the buffer to put the string in.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RVINTAPI RvBool RVCALLCONV RtspUtilsHPOOLELEMStrCpy(
            IN  HRPOOL      hRPool,
            IN  HRPOOLELEM  hElement,
            IN  RvSize_t    elementOffset,
            IN  RvChar      *strBuffer,
            IN  RvSize_t    bufferLength)
{
    RvBool      success;
    RvSize_t    eosOffset;
    RvSize_t    stringLength;

    success = RtspUtilsRPOOLELEMStrChr(hRPool, hElement, elementOffset,'\0',&eosOffset);
    stringLength = eosOffset - elementOffset;
    /* if NULL is not found in the bufferLength -1 characters (the maximum  */
    /* size of string which fits in buffer) we return RV_FALSE              */
    if ( (success == RV_FALSE) || (stringLength >= bufferLength ) )
        return RV_FALSE;

    /* we copy the string to the buffer - including the NULL terminating character  */
    strBuffer = rpoolCopyToExternal(hRPool, strBuffer, hElement, elementOffset, stringLength+1);
    if (strBuffer == NULL)
        return RV_FALSE;

    return RV_TRUE;
}




/**************************************************************************
 * RtspUtilsHPOOLELEMAppend
 * ------------------------------------------------------------------------
 * General: Appends the content of the source element into the destination
 *          element.
 *
 * Return Value:    RV_TRUE if the element was successfully appended,
 *                  RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRPool          - the RPool to which the elements belong.
 *              hDestination    - the element to append into.
 *              hSource         - the element to append from.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RvBool RtspUtilsHPOOLELEMAppend(
            IN  HRPOOL      hRPool,
            IN  HRPOOLELEM  hDestination,
            IN  HRPOOLELEM  hSource)
{
    RvSize_t    sourceLength;
    RvInt32     lastDestinationSize;
    RvChar      *pAppendedData;

    sourceLength = rpoolChunkSize(hRPool,hSource);

    /*  allocate the space              */
    if ( rpoolAppend(hRPool, hDestination, sourceLength, &lastDestinationSize, (RvUint8**)&pAppendedData) <= 0 )
        return RV_FALSE;

    /* copy the body into the element   */
    return RtspUtilsRPOOLELEMCopyInternal(  hRPool,
                                    hDestination,
                                    lastDestinationSize,
                                    hSource,
                                    0,
                                    sourceLength);

}

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
printCharBuffer(RvChar *buf, RvInt32 len, RvLogSource * msa)
{
    if (RvLogIsSelected(msa, RV_LOGLEVEL_DEBUG))
    {
        char tempBuff[1024];
        char *pBuf = tempBuff;
        RvInt32 i, j = 0;
        
        memset ((void *)tempBuff, 0 , sizeof(tempBuff));
        sprintf(tempBuff, "%4.4d ", j);
        pBuf += 5;
        if (len > (RvInt32)(sizeof(tempBuff) - 1))
            len = sizeof(tempBuff) - 1;
        for (i = 0; i < len; i++)
        {
            if ((buf[i] == '\n') || (i == len - 1))
            {
                if ((i == len - 1) && (buf[i] != '\n' && buf[i] != '\r'))
                    sprintf(pBuf, "%c", buf[i]);
                if (strlen(tempBuff) > 5)
                    RvLogTextDebug(msa, "%s", tempBuff);
                memset((void *)tempBuff, 0 , sizeof(tempBuff));
                pBuf = tempBuff;
                j++;
                sprintf(tempBuff, "%4.4d ", j);
                pBuf += 5;
            }
            else
            {
                if (buf[i] != '\r')
                {
                    sprintf(pBuf, "%c", buf[i]);
                    pBuf++;
                }
            }
        }
    }
}
#endif



/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/




#if defined(__cplusplus)
}
#endif
