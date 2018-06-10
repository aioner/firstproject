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
 *                              <RvRtspMessageTypes.h>
 *
 *	This header file contains definitions of all the RTSP message types.
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							12/1/04
 *
 *********************************************************************************/

#ifndef _RV_RTSP_MESSAGE_TYPES_H
#define _RV_RTSP_MESSAGE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILED                           */
/*-----------------------------------------------------------------------*/
#include "RvRtspCommonTypes.h"
#include "RvRtspFirstLineTypes.h"
#include "RvRtspHeaderTypes.h"


/*@****************************************************************************
 * Module: RtspMessageTypes (root)
 * ----------------------------------------------------------------------------
 * Title:  7.RTSP Message Type Definitions
 *
 * The RTSP message types are definitions of all of the RTSP message types.
 * You can find the RTSP message types in the RvRtspMessageTypes.h file.
 ***************************************************************************@*/

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

/*@****************************************************************************
 * Type: RvRtspRequest (RtspMessageTypes)
 * ----------
 * This structure holds an RTSP request message.
 ***************************************************************************@*/
typedef struct
{
    RvRtspRequestLine       requestLine;        /* The method contained in the 
                                                   first line. */
    RvRtspCSeqHeader        cSeq;               /* CSeq header data. */
    RvBool                  cSeqValid;          /* CSeq header validity. */
    RvRtspAcceptHeader      accept;             /* Accept header data. */
    RvBool                  acceptValid;        /* Accept header validity. */
    RvRtspSessionHeader     session;            /* Session header data. */
    RvBool                  sessionValid;       /* Session header validity. */
    RvRtspRangeHeader       range;              /* Range header data. */
    RvBool                  rangeValid;         /* Range header validity. */
    RvRtspTransportHeader   transport;          /* Transport header data. */
    RvBool                  transportValid;     /* Transport header validity. */
    RvRtspLocationHeader    location;           /* Location header data. */
    RvBool                  locationValid;      /* Location header validity. */
    RvBool                  requestBelongs2App; /* RV_TRUE if the request was 
                                                   allocated by the 
                                                   application. */
    RvRtspRequireHeader     require;            /* Require header data. */
    RvBool                  requireValid;       /* Require header validity. */
    RvRtspConnectionHeader  connection;         /* Connection header data. */
    RvBool                  connectionValid;    /* Connection header validity. */
    RvRtspMsgMessageHandle  hRtspMsgMessage;    /* A handle to the 
                                                   RtspMsgRequest object 
                                                   containing the 
                                                   RvRtspRequest. */
} RvRtspRequest;

/*@****************************************************************************
 * Type: RvRtspResponse (RtspMessageTypes)
 * ----------
 * This structure holds an RTSP response message.
 * The RADVISION RTSP Toolkit can parse and understand the following headers, 
 * as specified in the RTSP RFC 2326 Appendix D.1 (Minimal RTSP 
 * implementation): CSeq, Connection, Session, Transport, Content-Language, 
 * Content-Encoding, Content-Length, Content-Type, and RTP-Info.
 ***************************************************************************@*/
typedef struct
{
    RvRtspStatusLine            statusLine;             /* The response status. */
    RvRtspCSeqHeader            cSeq;                   /* Header data. */
    RvBool                      cSeqValid;              /* Header validity. */
    RvRtspConnectionHeader      connection;             /* Header data. */
    RvBool                      connectionValid;        /* Header validity. */
    RvRtspSessionHeader         session;                /* Header data. */
    RvBool                      sessionValid;           /* Header validity. */
    RvRtspTransportHeader       transport;              /* Header data. */
    RvBool                      transportValid;         /* Header validity. */
    RvRtspContentLanguageHeader contentLanguage;        /* Header data. */
    RvBool                      contentLanguageValid;   /* Header validity. */
    RvRtspContentEncodingHeader contentEncoding;        /* Header data. */
    RvBool                      contentEncodingValid;   /* Header validity. */
    RvRtspContentBaseHeader     contentBase;            /* Header data. */
    RvBool                      contentBaseValid;       /* Header validity. */
    RvRtspContentTypeHeader     contentType;            /* Header data. */
    RvBool                      contentTypeValid;       /* Header validity. */
    RvRtspContentLengthHeader   contentLength;          /* Header data. */
    RvBool                      contentLengthValid;     /* Header validity. */
    RvRtspRtpInfoHeader         rtpInfo;                /* Header data. */
    RvBool                      rtpInfoValid;           /* Header validity. */
    RvRtspBlobHandle            hBody;                  /* mem block containing
                                                           the body. */
    RvRtspPublicHeader          publicHdr;              /* Public header data. */
    RvBool                      publicHdrValid;         /* Header validity, */
    RvRtspMsgMessageHandle      hRtspMsgMessage;        /* A handle to the 
                                                           RtspMsgResponse  
                                                           object containing 
                                                           the RvRtspResponse. */
} RvRtspResponse;

/*@****************************************************************************
 * Type: RvRtspMsgType (RtspMessageTypes)
 * ----------
 * Enumeration of message type: Request or reply.
 ***************************************************************************@*/
typedef enum
{
    RV_RTSP_MSG_TYPE_UNKNOWN = -1,
    RV_RTSP_MSG_TYPE_REQUEST,
    RV_RTSP_MSG_TYPE_RESPONSE
} RvRtspMsgType;


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/

/*@**************************************************************************
 * RvRtspMessageConstructRequest (RtspMessageTypes)
 * ------------------------------------------------------------------------
 * General:
 *  Constructs a request message object. memset MUST NOT be called on the
 *  pRequest object after construction; it is called during construction.
 *
 * Arguments:
 * Input:   hRtsp               - The RTSP Stack handle.
 * Output:  pRequest            - The request object.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 *****************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspMessageConstructRequest(
                    IN   RvRtspHandle    hRtsp,
                    OUT  RvRtspRequest   **pRequest);

/*@**************************************************************************
 * RvRtspMessageDestructRequest (RtspMessageTypes)
 * ----------------------------------------------------------------------------
 * General: 
 *  Destructs a request message object.
 *  This API should be called for request messages that were created by
 *  the application using RvRtspMessageConstructRequest after the message was 
 *  sent.
 *
 * Arguments:
 * Input:   hRtsp               - The RTSP Stack handle.
 * Output:  pRequest            - The handle to the request object.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspMessageDestructRequest(
                    IN   RvRtspHandle    hRtsp,
                    IN   RvRtspRequest   *pRequest);

/*@**************************************************************************
 * RvRtspMessageConstructResponse (RtspMessageTypes)
 * ------------------------------------------------------------------------
 * General: 
 *  Constructs a response message object.
 *
 * Arguments:
 * Input:   hRtsp               - The RTSP Stack handle.
 * Output:  pResponse           - The response object.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspMessageConstructResponse(
                    IN   RvRtspHandle    hRtsp,
                    OUT  RvRtspResponse  **pResponse);

/*@**************************************************************************
 * RvRtspMessageDestructResponse (RtspMessageTypes)
 * ------------------------------------------------------------------------
 * General:
 *  Destructs a response message object.
 *  This API should be called for a response message that was created by
 *  the application using the RvRtspMessageConstructResponse after the message   
 *  was sent.
 *
 * Arguments:
 * Input:   hRtsp               - The RTSP Stack handle.
 * Output:  pResponse           - The handle to the response object.
 *
 * Return Value: RV_OK if successful.
 *               Other on failure.
 ***************************************************************************@*/
RVAPI
RvStatus RVCALLCONV RvRtspMessageDestructResponse(
                    IN   RvRtspHandle    hRtsp,
                    IN   RvRtspResponse  *pResponse);



#ifdef __cplusplus
}
#endif

#endif	/* Include guard */


