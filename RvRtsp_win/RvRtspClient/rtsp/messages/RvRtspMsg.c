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

#include "RtspObject.h"
#include "RvRtspCommonTypes.h"
#include "RvRtspMessageTypes.h"
#include "RtspUtilsInternal.h"
#include "RtspMsgInternal.h"
#include "RvRtspMsg.h"



/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/
#define MAX_HEADER_FIELDS_LINE_LEN 512
#define MAX_HEADER_NAME_LEN 128


/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/
static RvStatus createHeaderFieldsStr(
        IN    RvRtspMsgAppHeader  *msgHeader,
        INOUT RvUint32            *len,
        INOUT RvChar              *str);

static void addHeaderToList(
        IN  RvRtspMsgMessageHandle hRtspMsgMessage, 
        IN  RvBool                 bIsRequest, 
        IN  RtspMsgHeader          *header);

static void removeHeaderFromList(
        IN  RvRtspMsgMessageHandle hRtspMsgMessage, 
        IN  RvBool                 bIsRequest, 
        IN  RtspMsgHeader          *header);

static void resetValidFlag(
        IN  RvRtspMsgAppHeader  *msgHeader);


/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/

/******************************************************************************
 * RvRtspMsgAddHeaderFields
 * ----------------------------------------------------------------------------
 * General: 
 *  This API should be called in OnSendEv callbacks in the client,
 *  or before a message is sent (SendResponse/Request APIs)
 *  in the server, to add header fields to a supported valid header.
 * 
 * Arguments:
 * Input:   hRtsp            - The RtspClient stack handle.
 *          msgHeader        - The relevant information for the additional fields
 *                            provided by the application.
 * Output: hRtspMsgHeader   - A handle to the RtspMsgHeader object that holds 
 *                            the additional fields in the stack.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspMsgAddHeaderFields(
        IN      RvRtspHandle	        hRtsp,
        IN		RvRtspMsgAppHeader		*msgHeader,
        INOUT	RvRtspMsgHeaderHandle	*hRtspMsgHeader)
{
    
    RvRtsp          *pThis  = (RvRtsp *)hRtsp;
    RvChar          headerStr[MAX_HEADER_FIELDS_LINE_LEN];
    RvChar          *pStr   = headerStr;
    RvUint32        len     = MAX_HEADER_FIELDS_LINE_LEN - 1;
    RvStatus        status  = RV_OK;
    RtspMsgHeader   *header = NULL;
    
    
    if (pThis == NULL)
        return RV_ERROR_NULLPTR;


    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgAddHeaderFields\r\n"));

    memset(headerStr, 0, sizeof(headerStr));

    /* First set the delimiter */
    *pStr = msgHeader->delimiter;
    pStr++;

    status = createHeaderFieldsStr(msgHeader, &len, pStr);

    if (status == RV_OK)
    {
        int res;
        /* Get a header object */
        res = raAdd(pThis->hRaHeaders, (RAElement *)&header);
        if (res < 0)
            status = RV_ERROR_OUTOFRESOURCES;
        else
        {
            len += 1; /* The delimiter at the beginning */
            /* Create rpool element from header fields string */
            //begin :modified by lichao, 20140816 增加初始化
            header->headerName = NULL;
            header->pNext = NULL;
            header->pPrev = NULL;
            //end:
            header->headerFields = (RvRtspStringHandle)rpoolAllocCopyExternal(pThis->hRPool,
                                                                              0,
                                                                              headerStr,
                                                                              (RvInt32)len);
            /* Set added header fields in the addedFields handle of the supported header */ 
            *hRtspMsgHeader = (RvRtspMsgHeaderHandle)header;
            /* Insert the header into the list of headers in the message */
            addHeaderToList(msgHeader->hRtspMsgMessage, msgHeader->bIsRequest, header);
        }

    }

    
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgAddHeaderFields status:%d\r\n", status));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return status;

}

/******************************************************************************
 * RvRtspMsgAddHeader
 * ----------------------------------------------------------------------------
 * General: 
 *  This API should be called in the OnSendEv callbacks ,
 *  or before a message is sent (SendResponse/Request APIs), to add 
 *  or replace a header with its contents to a message.
 *  If the added header is also in the supported headers, the supported 
 *  header in the RvRtspRequest/RvRtspResponse object will be no longer valid,
 *  however headers with the same name can be added more then once, 
 *  using this API.
 *
 * Arguments:
 * Input:   hRtsp            - The RtspClient stack handle. 
 *          msgHeader        - The relevant information for the additional fields
 *                             provided by the application.
 * Output:  None.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspMsgAddHeader (
        IN  RvRtspHandle	            hRtsp,
        IN	RvRtspMsgAppHeader		    *msgHeader)
{
    RvRtsp          *pThis  = (RvRtsp *)hRtsp;
    RvChar          headerStr[MAX_HEADER_FIELDS_LINE_LEN];
    RvChar          headerNameStr[MAX_HEADER_NAME_LEN];
    RvChar          *pStr   = headerStr;
    RvUint32        len     = MAX_HEADER_FIELDS_LINE_LEN;
    RvStatus        status  = RV_OK;
    RvInt32         res;
    RtspMsgHeader   *header = NULL;

    if (pThis == NULL)
        return RV_ERROR_NULLPTR;

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgAddHeader\r\n"));

    /* Set the valid flag of supported header (if exist) to RV_FALSE */
    resetValidFlag(msgHeader);

    memset(headerStr, 0, sizeof(headerStr));
    memset(headerNameStr, 0, sizeof(headerNameStr));

    strncpy(headerNameStr, msgHeader->headerName, sizeof(headerNameStr) - 2);
    strcat(headerNameStr, ": ");

    res = raAdd(pThis->hRaHeaders, (RAElement *)&header);

    if (res < 0)
        status = RV_ERROR_OUTOFRESOURCES;
    else
    {

        /* Set header name in the object */
        header->headerName = (RvRtspStringHandle)rpoolAllocCopyExternal(pThis->hRPool,
                                                                        0,
                                                                        headerNameStr,
                                                                        (RvInt32)strlen(headerNameStr));

        /* Set the header fields in the object */
        status = createHeaderFieldsStr(msgHeader, &len, pStr);

        if (status == RV_OK)
        {
            
            /* Create rpool element from header fields string */
            header->headerFields = (RvRtspStringHandle)rpoolAllocCopyExternal(pThis->hRPool,
                0,
                headerStr,
                (RvInt32)len);
            /* Insert the header into the list of headers in the message */
            addHeaderToList(msgHeader->hRtspMsgMessage, msgHeader->bIsRequest, header);
        }
    }
    
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgAddHeader status:%d\r\n", status));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return status;

}

/******************************************************************************
 * RvRtspMsgAddGenericHeaderFields
 * ----------------------------------------------------------------------------
 * General: 
 *  This API should be called by the application to add a field to a generic 
 *  header that was previously added using RvRtspMsgAddHeader() or 
 *  RvRtspMsgAddHeaderFields().
 *
 * Arguments:
 * Input:   hRtsp            - The rtsp stack handle.   
 *          hRtspMsgHeader   - A handle to the RtspMsgHeader object that the additional
 *                             fields should be added to. 
 *          msgHeader        - The relevant information for the additional fields
 *                             provided by the application.
 * Output:  None
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspMsgAddGenericHeaderFields(
        IN  RvRtspHandle	        	hRtsp,
        IN	RvRtspMsgHeaderHandle		hRtspMsgHeader,
        IN	RvRtspMsgAppHeader		    *msgHeader)			  
{
    RvRtsp          *pThis      = (RvRtsp *)hRtsp;
    RvChar          headerStr[MAX_HEADER_FIELDS_LINE_LEN];
    RvChar          *pStr       = headerStr;
    RvUint32        len         = MAX_HEADER_FIELDS_LINE_LEN - 1;
    RvStatus        status      = RV_OK;
    RtspMsgHeader   *header     = (RtspMsgHeader *)hRtspMsgHeader;
    RvUint32        chunkSize;

    if (pThis == NULL)
        return RV_ERROR_NULLPTR;

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgAddGenericHeaderFields\r\n"));

    memset(headerStr, 0, sizeof(headerStr));
    
    chunkSize = (RvUint32)rpoolChunkSize(pThis->hRPool, (HRPOOLELEM)header->headerFields);
    if (chunkSize > MAX_HEADER_FIELDS_LINE_LEN - 1)
       status = RV_ERROR_OUTOFRESOURCES;
    else
    {
        rpoolCopyToExternal(pThis->hRPool, headerStr, (HRPOOLELEM)header->headerFields, 0, (RvInt32)chunkSize);
        pStr += strlen(headerStr);
        pStr -= 2; /* Remove header delimiter */
        /* First set the delimiter */
        *pStr = msgHeader->delimiter;
        pStr++;
        
        status = createHeaderFieldsStr(msgHeader, &len, pStr);
        
        if (status == RV_OK)
        {
            /* Add the delimiter and the original chunkSize to len */
            len += chunkSize - 1; /* chunkSize - 2 + 1 */
            /* Remove the old header fields */
            rpoolFree(pThis->hRPool, (HRPOOLELEM)header->headerFields);

            /* Set the new header fields */
            header->headerFields = (RvRtspStringHandle)rpoolAllocCopyExternal(pThis->hRPool, 0, (void *)headerStr, (RvInt32)len);
        }
    }

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgAddGenericHeaderFields status:%d\r\n", status));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return status;

}

/******************************************************************************
 * RvRtspMsgGetMessageHeaderNames
 * ----------------------------------------------------------------------------
 * General: 
 *  This API should be called by the application to retrieve the names and 
 *  handles of all the headers in a message.When the application has all 
 *  the header names it can decide which headers to retrieve using 
 *  RvRtspMsgGetHeaderFieldValues. 
 *
 * Arguments:
 * Input:   hRtsp               - The rtsp stack handle.
 *          hRtspMsgMessage     - A handle to the RtspMsgMessage object in the stack.
 *          bIsRequest          - RV_TRUE if the message is a request message.
 *          headerNameArraySize - The size of headerNameArray allocated by the 
 *                                application.
 *          headerNameArray     - The length of the headerName (headerNameLen) should 
 *                                be set by the application.
 * Output:  headerNameArraySize - The actual number of headers retrieved from the 
 *                                message.
 *          headerNameArray     - The array of RvRtspHeaderName objects allocated 
 *                                by the application to retrieve the names of 
 *                                all the headers in the message.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *               RV_ERROR_OUTOFRESOURCES will be returned if there are more
 *               headers in the message than headerNameArraySize. 
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspMsgGetMessageHeaderNames(
        IN      RvRtspHandle	        	hRtsp,
	    IN		RvRtspMsgMessageHandle		hRtspMsgMessage,
        IN      RvBool                      bIsRequest,
	    INOUT	RvUint32				    *headerNameArraySize,
	    INOUT   RvRtspHeaderName			*headerNameArray)
{
    RvRtsp              *pThis      = (RvRtsp *)hRtsp;
    RtspMsgHeader       *header     = NULL;
    RvUint32            curIndex    = 0;
    RvStatus            status      = RV_OK;
    RvUint32            maxSize     = *headerNameArraySize;
    RtspMsgRequest      *request;
    RtspMsgResponse     *response;

    if (pThis == NULL)
        return RV_ERROR_NULLPTR;

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgGetMessageHeaderNames\r\n"));

    if (bIsRequest == RV_TRUE)
    {
        request = (RtspMsgRequest *)hRtspMsgMessage;
        header  = request->msgHeaders;
    }
    else
    {
        response = (RtspMsgResponse *)hRtspMsgMessage;
        header   = response->msgHeaders;
    }

    if (header != NULL)
    {
        while (curIndex < maxSize)
        {
            RvUint32 nameLen = 0;
            if (header == NULL)
                break;
            /* Get the size of the header name */
            nameLen = (RvUint32)rpoolChunkSize(pThis->hRPool, (HRPOOLELEM)header->headerName);
            if (nameLen > (RvUint32)headerNameArray[curIndex].headerNameLen)
            {
                status = RV_ERROR_OUTOFRESOURCES;
                nameLen = headerNameArray[curIndex].headerNameLen;
            }
            rpoolCopyToExternal(pThis->hRPool, 
                (void *)(headerNameArray[curIndex].headerName), 
                (HRPOOLELEM)header->headerName,
                0, 
                (RvInt32)nameLen);
            headerNameArray[curIndex].headerNameLen = (RvUint8)nameLen;
            headerNameArray[curIndex].hHeader = (RvRtspMsgHeaderHandle)header;
            curIndex++;
            header = header->pNext;
        }
        if (header != NULL)
        {
            /* There are too many headers, headerNameArray is too small */
            status = RV_ERROR_OUTOFRESOURCES;
        }
        *headerNameArraySize = curIndex;
    }

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgGetMessageHeaderNames status:%d\r\n", status));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return status;
    
}

/******************************************************************************
 * RvRtspMsgGetHeaderByName
 * ----------------------------------------------------------------------------
 * General: 
 *  This API should be called by the application to retrieve a handle of a 
 *  specific header in a message.
 *  Please notice that if there are several headers with the same name in
 *  the message, only the first one will be retrieved.
 *
 * Arguments:
 * Input:   hRtsp               - The rtsp stack handle.   
 *          hRtspMsgMessage     - A handle to the RtspMsgMessage object in the stack.
 *          bIsRequest          - RV_TRUE   if the message is a request message.
 *          headerName          - The name of the required header is set in the 
 *                                headerName field.
 * Output:  headerName          - The object containing the handle for the header
 *                                in the stack. If the specific header not found, 
 *                                the handle to the header object in headerName
 *                                will be NULL.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspMsgGetHeaderByName(
        IN      RvRtspHandle	        	hRtsp,
	    IN		RvRtspMsgMessageHandle		hRtspMsgMessage,
        IN      RvBool                      bIsRequest,
        INOUT	RvRtspHeaderName			*headerName)
{
    RvRtsp              *pThis  = (RvRtsp *)hRtsp;
    RtspMsgHeader       *header = NULL;
    RtspMsgRequest      *request;
    RtspMsgResponse     *response;

    if (pThis == NULL)
        return RV_ERROR_NULLPTR;

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgGetHeaderByName\r\n"));

    if (bIsRequest == RV_TRUE)
    {
        request = (RtspMsgRequest *)hRtspMsgMessage;
        header  = request->msgHeaders;
    }
    else
    {
        response = (RtspMsgResponse *)hRtspMsgMessage;
        header   = response->msgHeaders;
    }
    
    headerName->hHeader = NULL;

    while (header != NULL)
    {
        if (rpoolCompareExternal(pThis->hRPool, 
                            (HRPOOLELEM)header->headerName, 
                            (void *)(headerName->headerName), 
                            (RvInt32)strlen(headerName->headerName)) == 0)
        {
            /* Found */
            headerName->hHeader = (RvRtspMsgHeaderHandle)header;
            break;
        }
        header = header->pNext;
    }

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgGetHeaderByName status:%d\r\n", 0));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return RV_OK;
    
}

/******************************************************************************
 * RvRtspMsgGetHeaderFieldValues
 * ----------------------------------------------------------------------------
 * General: 
 *  This API should be called by the application to retrieve the header fields
 *  values from the given header..
 *
 * Arguments:
 * Input:   hRtsp           - The rtsp stack handle.
 *          hRtspMsgMessage - A handle to the RtspMsgMessage object in the stack.
 * Output:  msgHeader       - The object containing the values of the header fields.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspMsgGetHeaderFieldValues(
        IN      RvRtspHandle	        	hRtsp,
    	IN		RvRtspMsgHeaderHandle		hRtspMsgHeader,
	    INOUT	RvRtspMsgAppHeader		    *msgHeader)
{
    RvRtsp              *pThis      = (RvRtsp *)hRtsp;
    RtspMsgHeader       *header     = (RtspMsgHeader *)hRtspMsgHeader;
    RvStatus            status      = RV_OK;
    RvUint32            chunkLen    = 0;
    RvUint32            maxFields   = msgHeader->headerFieldsSize;
    RvChar              headerStr[MAX_HEADER_FIELDS_LINE_LEN];

    if (pThis == NULL)
        return RV_ERROR_NULLPTR;

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgGetHeaderFieldValues\r\n"));

    msgHeader->headerFieldsSize = 0;
    msgHeader->headerFieldsSizeError = RV_FALSE;
    msgHeader->headerFieldStrLenError = RV_FALSE;

    /* Get header name if the application allocated memory for the name */
    if (header->headerName != NULL)
    {
        chunkLen = (RvUint32)rpoolChunkSize(pThis->hRPool, (HRPOOLELEM)header->headerName);
        if (chunkLen > msgHeader->headerNameLen)
        {
        /* There's no point returning error here - the application probably knows 
            the name of the header */
            chunkLen = msgHeader->headerNameLen;
        }
        rpoolCopyToExternal(pThis->hRPool, (void *)msgHeader->headerName, (HRPOOLELEM)header->headerName, 0, (RvInt32)chunkLen);
        msgHeader->headerNameLen = chunkLen;
        msgHeader->headerName[chunkLen] = 0;

    }
    
    /* Get the header fields str */
    chunkLen = (RvUint32)rpoolChunkSize(pThis->hRPool, (HRPOOLELEM)header->headerFields);
    if (chunkLen > MAX_HEADER_FIELDS_LINE_LEN)
    {
        status = RV_ERROR_OUTOFRESOURCES;
    }
    else
    {
        RvChar      *delim = "\0";
        RvChar      *token = NULL;
        RvUint32    curInd = 0;
        
        memset(headerStr, 0, sizeof(headerStr));
        rpoolCopyToExternal(pThis->hRPool, (void *)headerStr, (HRPOOLELEM)header->headerFields, 0, (RvInt32)chunkLen);
        if ((strchr(headerStr, ';')) != NULL)
        {
            delim = ";\0";
        }
        else if ((strchr(headerStr, ',')) != NULL)
        {
            delim = ",\0";
        }

        token = strtok(headerStr, delim);
        while (token != NULL)
        {
            RvUint32    strLen;
            if (curInd >= maxFields)
            {
                msgHeader->headerFieldsSizeError = RV_TRUE;
                status = RV_ERROR_OUTOFRESOURCES;
                break;
            }
            /* skip spaces at the beginning of the token */
            while ((*token == ' ') || (*token == '\t'))
                token++;
            strLen = strlen(token);
            if (strLen > msgHeader->headerFieldStrLen)
            {
                msgHeader->headerFieldStrLenError = RV_TRUE;
                strLen = msgHeader->headerFieldStrLen - 1;
                status = RV_ERROR_OUTOFRESOURCES;
            }
            memset(msgHeader->headerFields[curInd], 0, msgHeader->headerFieldStrLen);
            memcpy(msgHeader->headerFields[curInd], token, strLen);
            curInd++;
            token = strtok(NULL, delim);
        }
        msgHeader->headerFieldsSize = curInd;
    }
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgGetHeaderFieldValues status:%d\r\n", 0));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    
    return status;
}

/******************************************************************************
 * RvRtspMsgRemoveAllHeaders
 * ----------------------------------------------------------------------------
 * General: 
 *  This API Removes all headers from the msgHeaders list.
 * 
 * Arguments:
 * Input:   hRtspMsgMessage - A handle to the RtspMsgMessage object in the stack.
 *          bIsRequest      - RV_TRUE if the given message is a request.
 * Output:  None.
 *
 * Return Value: None
 *****************************************************************************/
RVAPI void RVCALLCONV RvRtspMsgRemoveAllHeaders(
        IN      RvRtspMsgMessageHandle		hRtspMsgMessage,
        IN      RvBool                      bIsRequest)
{
    RvRtsp          *pThis;
    RtspMsgHeader   *header;

    if (hRtspMsgMessage == NULL)
        return;

    if (bIsRequest == RV_TRUE)
    {
        RtspMsgRequest  *pRequest = (RtspMsgRequest *)hRtspMsgMessage;
        header = pRequest->msgHeaders;
        pThis = (RvRtsp *)pRequest->hRtsp;
    }
    else
    {
        RtspMsgResponse *pResponse = (RtspMsgResponse *)hRtspMsgMessage;
        header = pResponse->msgHeaders;
        pThis = (RvRtsp *)pResponse->hRtsp;
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgRemoveAllHeaders\r\n"));

    while (header != NULL)
    {
        RtspMsgHeader   *current = header;
        header = header->pNext;
        /* Remove the header from the header list */
        removeHeaderFromList(hRtspMsgMessage, bIsRequest, current);
    }

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspMsgRemoveAllHeaders\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
}

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
        OUT     HRPOOLELEM              msgElement)
{
    RtspMsgHeader   *header = (RtspMsgHeader *)additionalFields;
    RvStatus        status = RV_OK;

    if (hRtspMsgMessage == NULL)
        return status;


    /* First set the additional fields in the rpool element */
    if ((RtspUtilsHPOOLELEMAppend(hRPool, msgElement, (HRPOOLELEM)header->headerFields)) != RV_TRUE)
        status = RV_ERROR_UNKNOWN;
    
    /* Remove the header from the header list */
    removeHeaderFromList(hRtspMsgMessage, bIsRequest, header);

    return status;
    
}

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
        OUT     HRPOOLELEM              msgElement)
{
    RtspMsgHeader   *header;
    RvStatus        status = RV_OK;

    if (hRtspMsgMessage == NULL)
        return status;

    if (bIsRequest == RV_TRUE)
    {
        RtspMsgRequest *request = (RtspMsgRequest *)hRtspMsgMessage;
        header = request->msgHeaders;
    }
    else
    {
        RtspMsgResponse *response = (RtspMsgResponse *)hRtspMsgMessage;
        header = response->msgHeaders;
    }

    while (header != NULL)
    {
        RtspMsgHeader *current;
        /* Append the header name to the message */
        if ((RtspUtilsHPOOLELEMAppend(hRPool, msgElement, (HRPOOLELEM)header->headerName)) != RV_TRUE)
            status = RV_ERROR_UNKNOWN;
        /* Append header fields */
        if ((RtspUtilsHPOOLELEMAppend(hRPool, msgElement, (HRPOOLELEM)header->headerFields)) != RV_TRUE)
            status = RV_ERROR_UNKNOWN;
        
        current = header;
        header = header->pNext;
        /* Remove the added header */
        removeHeaderFromList(hRtspMsgMessage, bIsRequest, current);
    }

    return status;
}

/******************************************************************************
 * RtspMsgHeaderDesrialize
 * ----------------------------------------------------------------------------
 * General: 
 *  Desrializes a header from a received message into an RtspMsgHeader object. 
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
        IN      RvChar*                 headerBuffer)
{
 
    RtspMsgHeader   *header;
    RvRtsp          *pThis;
    RvInt32         res;
    RvChar          fieldsStr[1024];

    if (hRtspMsgMessage == NULL)
        return RV_ERROR_NULLPTR;

    if (bIsRequest == RV_TRUE)
    {
        RtspMsgRequest *request = (RtspMsgRequest *)hRtspMsgMessage;
        pThis = (RvRtsp *)request->hRtsp;
    }
    else
    {
        RtspMsgResponse *response = (RtspMsgResponse *)hRtspMsgMessage;
        pThis = (RvRtsp *)response->hRtsp;
    }

    res = raAdd(pThis->hRaHeaders, (RAElement *)&header);
    if (res < 0)
        return RV_ERROR_OUTOFRESOURCES;
    else
    {
        RvChar  *delim = ":";
        RvChar  *token;
        RvChar  *pStr = fieldsStr;

        memset(fieldsStr, 0, sizeof(fieldsStr));
        memcpy(fieldsStr, headerBuffer, strlen(headerBuffer));

        /* Set the header name */
        token = strtok(headerBuffer, delim);
        if (token != NULL)
        {
            header->headerName = (RvRtspStringHandle)rpoolAllocCopyExternal(pThis->hRPool, 
                                                                            0, 
                                                                            (void *)token, 
                                                                            (RvInt32)strlen(token));
           
            pStr += strlen(token) + 1;
            
            /* Skip space at the beginning */
            if (*pStr == ' ')
                pStr++;
            /* Set the header fields */
            header->headerFields = (RvRtspStringHandle)rpoolAllocCopyExternal(pThis->hRPool, 
                0, 
                (void *)pStr, 
                (RvInt32)strlen(pStr));
        }

        /* Add the header to the message */
        addHeaderToList(hRtspMsgMessage, bIsRequest, header);
    }
    return RV_OK;

}


/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/
/******************************************************************************
 * createHeaderFieldsStr
 * ----------------------------------------------------------------------------
 * General: 
 *  Builds the string containing all given header fields and their values.
 * 
 * Arguments:
 * Input:   msgHeader       - The added header fields information.
 *          len             - The length of the given str buffer.
 * Output:  len             - The actual len of the string containing all 
 *                            given header fields.
 *          str             - The string containing all given header fields.
 *
 * Return Value: RV_OK  - if successful.
 *               RV_ERROR_OUTOFRESOURCES on failure
 *****************************************************************************/
static RvStatus createHeaderFieldsStr(
      IN    RvRtspMsgAppHeader  *msgHeader,
      INOUT RvUint32            *len,
      OUT   RvChar              *str)
{
    RvUint32    maxLen = *len;
    RvUint32    i;
    RvChar      *pStr = str;

    *len = 0;

    for (i = 0; i < msgHeader->headerFieldsSize; ++i)
    {
        if (msgHeader->headerFields[i] != NULL)
        {
            RvUint32 strLen = (RvUint32)strlen(msgHeader->headerFields[i]);
            if ((RvUint32)((pStr + strLen) - str) > maxLen)
            {
                memset(str, 0, maxLen);
                *len = 0;
                return RV_ERROR_OUTOFRESOURCES;
            }
            strncpy(pStr, msgHeader->headerFields[i], strLen);
            pStr += strLen;
            (*len) += strLen;
            if ((i + 1) < msgHeader->headerFieldsSize)
            {
                /*This is not the last header field, add a delimiter */
                *pStr = msgHeader->delimiter;
                pStr++;
                (*len)++;
            }
            else
            {
                /*This is the last field - add End Of Line delimiter */
                if ((RvUint32)((pStr + 2) - str) > maxLen)
                {
                    memset(str, 0, maxLen);
                    *len = 0;
                    return RV_ERROR_OUTOFRESOURCES;
                }
                strcpy(pStr, TERMINATING_DELIMITERS_STRING);
                pStr += 2;
                (*len) += 2;
            }
        }
    }

    return RV_OK;
}

/******************************************************************************
 * addHeaderToList
 * ----------------------------------------------------------------------------
 * General: 
 *  Adds an RtspMsgHeader object to the list of headers in the message.
 * 
 * Arguments:
 * Input:   hRtspMsgMessage     - The message handle.
 *          bIsRequest          - RV_TRUE if the message is a request message.
 *          header              - The header object to be added to the list.
 * Output:  None.
 *
 * Return Value: None.
 *****************************************************************************/
static void addHeaderToList(
        IN  RvRtspMsgMessageHandle hRtspMsgMessage, 
        IN  RvBool                 bIsRequest, 
        IN  RtspMsgHeader          *header)
{
    RtspMsgHeader   **lastHeader;
    RtspMsgHeader   **listStart;
    if (bIsRequest == RV_TRUE)
    {
        RtspMsgRequest *request = (RtspMsgRequest *)hRtspMsgMessage;
        if (request == NULL)
            return;
        listStart  = &request->msgHeaders;
        lastHeader = &request->lastHeader;
    }
    else
    {
        RtspMsgResponse *response = (RtspMsgResponse *)hRtspMsgMessage;
        if (response == NULL)
            return;
        listStart  = &response->msgHeaders;
        lastHeader = &response->lastHeader;
    }
    if (*lastHeader == NULL && *listStart == NULL)
    {
        /* The first in the list */
        *lastHeader = *listStart = header;
    }
    else
    {
        (*lastHeader)->pNext = header;
    }
    header->pPrev = *lastHeader;
    header->pNext = NULL;
    *lastHeader = header;
}

/******************************************************************************
 * removeHeaderFromList
 * ----------------------------------------------------------------------------
 * General: removes an RtspMsgHeader object from the list of headers in the message.
 * 
 * Arguments:
 * Input:   hRtspMsgMessage     - The message handle.
 *          bIsRequest          - RV_TRUE if the message is a request message.
 *          header              - The header object to be added to the list.
 * Output:  None.
 *
 * Return Value: None 
 *****************************************************************************/
static void removeHeaderFromList(
        IN  RvRtspMsgMessageHandle hRtspMsgMessage, 
        IN  RvBool                 bIsRequest, 
        IN  RtspMsgHeader          *header)
{
    RvRtsp          *pThis;
    RtspMsgHeader   **lastHeader = NULL;
    RtspMsgHeader   **listStart = NULL;

    if (bIsRequest == RV_TRUE)
    {
        RtspMsgRequest *request = (RtspMsgRequest *)hRtspMsgMessage;
        if (request == NULL)
            return;
        listStart  = &request->msgHeaders;
        lastHeader = &request->lastHeader;
        pThis = (RvRtsp *)request->hRtsp;
    }
    else
    {
        RtspMsgResponse *response = (RtspMsgResponse *)hRtspMsgMessage;
        if (response == NULL)
            return;
        listStart  = &response->msgHeaders;
        lastHeader = &response->lastHeader;
        pThis = (RvRtsp *)response->hRtsp;
    }

    if (pThis == NULL)
        return;

    if (header->pPrev == *listStart)
    {
        /* The first one */
        *listStart = header->pNext;
        if (*listStart != NULL)
            (*listStart)->pPrev = *listStart;
        else
            *lastHeader = NULL;
    }
    else if (*lastHeader == header)
    {
        /* The lst one */
        header->pPrev->pNext = NULL;
        *lastHeader = header->pPrev;
    }
    else
    {
        header->pPrev->pNext = header->pNext;
        header->pNext->pPrev = header->pPrev;
    }

	/**********************add by kongfd********************/
	if(header->headerName !=NULL)
		rpoolFree(pThis->hRPool,header->headerName);
	if(header->headerFields !=NULL)
		rpoolFree(pThis->hRPool,header->headerFields);
   /**********************add end********************/	

    /* Delete the header pbject */
    raDelete(pThis->hRaHeaders, (RAElement)header);
}
/******************************************************************************
 * resetValidFlag
 * ----------------------------------------------------------------------------
 * General: 
 *  Sets the valid flag of a supported header with the given name, to RV_FALSE.
 *
 * Arguments:
 * Input:   headerName      - The header name.
 *          headerNameLen   - The length of the header name string.
 *          bIsRequest      - RV_TRUE if the header to a request message.
 * Output:  None.
 * 
 * Return Value: None
 *****************************************************************************/
static void resetValidFlag(
        IN  RvRtspMsgAppHeader  *msgHeader)
{
    if (msgHeader->bIsRequest == RV_TRUE)
    {
        RtspMsgRequest *requestMsg = (RtspMsgRequest *)msgHeader->hRtspMsgMessage;
        RvRtspRequest  *supportedReq = &(requestMsg->request);

        if (strcmp(msgHeader->headerName, "Accept") == 0)
        {
            supportedReq->acceptValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Session") == 0)
        {
            supportedReq->sessionValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Range") == 0)
        {
            supportedReq->rangeValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Transport") == 0)
        {
            supportedReq->transportValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Location") == 0)
        {
            supportedReq->locationValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Require") == 0)
        {
            supportedReq->requireValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Connection") == 0)
        {
            supportedReq->connectionValid = RV_FALSE;
        }
    }
    else
    {
        RtspMsgResponse *responseMsg = (RtspMsgResponse *)msgHeader->hRtspMsgMessage;
        RvRtspResponse  *supportedResp = &(responseMsg->response);

        if (strcmp(msgHeader->headerName, "Connection") == 0)
        {
            supportedResp->connectionValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Session") == 0)
        {
            supportedResp->sessionValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Transport") == 0)
        {
            supportedResp->transportValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Content-Language") == 0)
        {
            supportedResp->contentLanguageValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Content-Encoding") == 0)
        {
            supportedResp->contentEncodingValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Content-Length") == 0)
        {
            supportedResp->contentLengthValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Content-Type") == 0)
        {
            supportedResp->contentTypeValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "RTP-Info") == 0)
        {
            supportedResp->rtpInfoValid = RV_FALSE;
        }
        else if(strcmp(msgHeader->headerName, "Public") == 0)
        {
            supportedResp->publicHdrValid = RV_FALSE;
        }
    }
}


#if defined(__cplusplus)
}
#endif
