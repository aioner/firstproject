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
 *                              <RtspClientConnectionInternal.h>
 *
 * Internal header - used inside the RvRtsp library.
 *
 *	This file contains definitions relevant to the RTSP server connections.
 *	An RTSP Connection instance is a thread safe representation of a connection
 *	to an RTSP server, handling all RTSP communication to and from the server.
 *
 *    Author                         Date
 *    ------                        ------
 *		Shaft						8/1/04
 *
 *********************************************************************************/

#ifndef _RV_RTSP_MSG_H
#define _RV_RTSP_MSG_H

#ifdef __cplusplus
extern "C" {
#endif


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/


/*@****************************************************************************
 * Module: RtspMsg (root)
 * ----------------------------------------------------------------------------
 * Title: 10.RTSP Server Messages
 *
 * RTSP Server connection messages are definitions relevant to the RTSP Server
 * connections. An RTSP Connection instance is a threadsafe representation
 * of a connection to an RTSP Server, handling all RTSP communication
 * to and from the Server.
 *
 * You can find the RTSP Server connection messages in the
 * RtspClientConnectionInternal.h file.
 ***************************************************************************@*/


/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

/*@****************************************************************************
 * Type: RvRtspMsgAppHeader (RtspMsg)
 * ----------------------------------------------------------------------------
 * Type definition of the RtspMsg header to be used by the application to set
 * RtspMsg headers, or by the Stack to handle an RtspMsg header for the 
 * application.
 ***************************************************************************@*/
typedef struct RvRtspMsgAppHeader
{
    RvRtspMsgMessageHandle  hRtspMsgMessage;        /* The handle for the 
                                                       RtspMsgRequest or 
                                                       RtspMsgResponse object. */
    RvBool                  bIsRequest;             /* RV_TRUE if the header 
                                                       belongs to a Request 
                                                       message. */
    RvUint32                headerNameLen;          /* The length of the
                                                       headerName string. */
    RvChar                  *headerName;            /* The string allocated by
                                                       the application for the 
                                                       header name. */  
    RvChar                  delimiter;              /* The delimiter to set 
                                                       between the header 
                                                       fields. Examples of 
                                                       delimiters could be ";" 
                                                       and ","". */	
    RvUint32                headerFieldStrLen;      /* The length of the string
                                                       allocated for the header 
                                                       field values. */
    RvBool                  headerFieldStrLenError; /* Set to RV_TRUE if a 
                                                       header field value 
                                                       exceeds 
                                                       headerFieldStrLen. */
    RvUint32                headerFieldsSize;       /* The size of the 
                                                       headerFields buffer. */
    RvBool                  headerFieldsSizeError;  /* Set to RV_TRUE if other
                                                       header fields exist
                                                       besides headerFieldsSize. */
    RvChar                  **headerFields;         /* The headerFields Array 
                                                       allocated by the 
                                                       application for adding 
                                                       or retrieving header 
                                                       fields. */
}RvRtspMsgAppHeader;

/*@****************************************************************************
 * Type: RvRtspHeaderName (RtspMsg)
 * ----------------------------------------------------------------------------
 * Type definition of the RtspMsg header name object. This object is used for 
 * retrieving headers from a message according to their names.
 ***************************************************************************@*/
typedef struct RvRtspHeaderName
{
    RvUint8                 headerNameLen;          /* The length of the header 
                                                       name string. */
    RvChar                  *headerName;            /* The header name string. */
    RvRtspMsgHeaderHandle   hHeader;
} RvRtspHeaderName;


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/

/*@**************************************************************************
 * RvRtspMsgAddHeaderFields (RtspMsg)
 * ------------------------------------------------------------------------
 * General:
 *  This API should be called in OnSendEv callbacks in the Client,
 *  or before a message is sent (SendResponse or SendRequest APIs) 
 *  in both the Client and the Server, to add header fields to a supported 
 *  valid header.
 *
 * Arguments:
 * Input:   hRtsp            - The RtspClient Stack handle.
 *          msgHeader        - The relevant information for the additional 
 *                             fields provided by the application.
 * Output: hRtspMsgHeader   - A handle to the RtspMsgHeader object that holds
 *                            the additional fields in the Stack.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspMsgAddHeaderFields(
        IN      RvRtspHandle	        hRtsp,
        IN		RvRtspMsgAppHeader		*msgHeader,
        INOUT	RvRtspMsgHeaderHandle	*hRtspMsgHeader);

/*@**************************************************************************
 * RvRtspMsgAddHeader (RtspMsg)
 * ------------------------------------------------------------------------
 * General:
 *  This API should be called in OnSendEv callbacks in the Client,
 *  or before a message is sent (SendResponse or SendRequest APIs) in both the 
 *  Client and the Server, to add or replace a header with its contents in a 
 *  message.
 *  If the added header also exists in the supported headers, the supported
 *  header in the RvRtspRequest/RvRtspResponse object will no longer be valid.
 *  However, headers with the same name can be added more than once using this 
 *  API.
 *
 * Arguments:
 * Input:   hRtsp            - The RtspClient Stack handle.
 *          msgHeader        - The relevant information for the additional 
 *                             fields provided by the application.
 * Output:  None.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspMsgAddHeader (
        IN  RvRtspHandle	            hRtsp,
        IN	RvRtspMsgAppHeader		    *msgHeader);

/*@**************************************************************************
 * RvRtspMsgAddGenericHeaderFields (RtspMsg)
 * ------------------------------------------------------------------------
 * General:
 *  This API should be called by the application to add a field to a generic
 *  header that was previously added using RvRtspMsgAddHeader() or
 *  RvRtspMsgAddHeaderFields().
 *
 * Arguments:
 * Input:   hRtsp            - The RTSP Stack handle.
 *          hRtspMsgHeader   - A handle to the RtspMsgHeader object to which 
 *                             the additional fields should be added.
 *          msgHeader        - The relevant information for the additional 
 *                             fields provided by the application.
 * Output:  None.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspMsgAddGenericHeaderFields(
        IN  RvRtspHandle	        	hRtsp,
        IN	RvRtspMsgHeaderHandle		hRtspMsgHeader,
        IN	RvRtspMsgAppHeader		    *msgHeader);

/*@**************************************************************************
 * RvRtspMsgGetMessageHeaderNames (RtspMsg)
 * ------------------------------------------------------------------------
 * General:
 *  This API should be called by the application to retrieve the names and
 *  handles of all of the headers in a message. Once the application has all 
 *  of the header names, it can decide which headers to retrieve using
 *  RvRtspMsgGetHeaderFieldValues.
 *  This function can be used both for Supported and Unsupported headers.
 *
 * Arguments:
 * Input:   hRtsp               - The RTSP Stack handle.
 *          hRtspMsgMessage     - A handle to the RtspMsgMessage object in the 
 *                                Stack.
 *          bIsRequest          - RV_TRUE if the message is a request message.
 *          headerNameArraySize - The size of headerNameArray allocated by the
 *                                application.
 *          headerNameArray     - The length of the headerName (headerNameLen) 
 *                                should be set by the application.
 * Output:  headerNameArraySize - The actual number of headers retrieved from 
 *                                the message.
 *          headerNameArray     - The array of RvRtspHeaderName objects 
 *                                allocated by the application to retrieve the 
 *                                names of all of the headers in the message.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 *               RV_ERROR_OUTOFRESOURCES is returned if other headers
 *               besides headerNameArraySize exist in the message.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspMsgGetMessageHeaderNames(
        IN      RvRtspHandle	        	hRtsp,
	    IN		RvRtspMsgMessageHandle		hRtspMsgMessage,
        IN      RvBool                      bIsRequest,
	    INOUT	RvUint32				    *headerNameArraySize,
	    OUT		RvRtspHeaderName			*headerNameArray);

/*@**************************************************************************
 * RvRtspMsgGetHeaderByName (RtspMsg)
 * ------------------------------------------------------------------------
 * General:
 *  This API should be called by the application to retrieve a handle for a
 *  specific header in a message.
 *  Please note that if the message contains several headers with the same 
 *  name, only the first one will be retrieved.
 *
 * Arguments:
 * Input:   hRtsp               - The RTSP Stack handle.
 *          hRtspMsgMessage     - A handle to the RtspMsgMessage object in the 
 *                                Stack.
 *          bIsRequest          - RV_TRUE if the message is a Request message.
 *          headerName          - The name of the required header is set in the
 *                                headerName field.
 * Output:  headerName          - The object containing the handle for the 
 *                                header in the Stack. If the specific header 
 *                                is not found, the handle to the header object 
 *                                in headerName will be NULL.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspMsgGetHeaderByName(
        IN      RvRtspHandle	        	hRtsp,
	    IN		RvRtspMsgMessageHandle		hRtspMsgMessage,
        IN      RvBool                      bIsRequest,
        INOUT	RvRtspHeaderName			*headerName);

/*@**************************************************************************
 * RvRtspMsgGetHeaderFieldValues (RtspMsg)
 * ------------------------------------------------------------------------
 * General:
 *  This API should be called by the application to retrieve the header field
 *  values from the given header. This function can be used both for Supported 
 *  and Unsupported headers.
 *
 * Arguments:
 * Input:   hRtsp           - The RTSP Stack handle.
 *          hRtspMsgMessage - A handle to the RtspMsgMessage object in the 
 *                            Stack.
 * Output:  msgHeader       - The object containing the values of the header 
 *                            fields.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspMsgGetHeaderFieldValues(
        IN      RvRtspHandle	        	hRtsp,
    	IN		RvRtspMsgHeaderHandle		hRtspMsgHeader,
	    INOUT	RvRtspMsgAppHeader		    *msgHeader);

/*@**************************************************************************
 * RvRtspMsgRemoveAllHeaders (RtspMsg)
 * ------------------------------------------------------------------------
 * General:
 *  This API removes all headers from the msgHeaders list.
 * 
 * Arguments:
 * Input:   hRtspMsgMessage - A handle to the message object in the Stack.
 *          bIsRequest      - RV_TRUE if the given message is a request.
 * Output:  None.
 *
 * Return Value: None.
 ***************************************************************************@*/
RVAPI void RVCALLCONV RvRtspMsgRemoveAllHeaders(
        IN      RvRtspMsgMessageHandle		hRtspMsgMessage,
        IN      RvBool                      bIsRequest);


#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_MSG_H*/


