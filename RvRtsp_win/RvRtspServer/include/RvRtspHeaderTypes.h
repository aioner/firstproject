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
 *                              <RvRtspHeaderTypes.h>
 *
 *	This header file contains definitions of all headers types that are used by
 *	RTSP messages.
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							12/1/04
 *
 *********************************************************************************/

#ifndef _RV_RTSP_HEADERS_TYPES_H
#define _RV_RTSP_HEADERS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILED                           */
/*-----------------------------------------------------------------------*/


/*@****************************************************************************
 * Module: RtspHeaderTypes (root)
 * ----------------------------------------------------------------------------
 * Title:  8.RTSP Header Type Definitions
 *
 * The RTSP header types are definitions of all header types used by
 * RTSP messages.
 *
 * You can find the RTSP header types in the RvRtspHeaderTypes.h file.
 ***************************************************************************@*/

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

/*@****************************************************************************
 * Type: RvRtspNptFormat (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Specifies the type of NPT time format that the structure represents.
 ***************************************************************************@*/
typedef enum
{
    RV_RTSP_NPT_FORMAT_NOT_EXISTS,      /* The structure is null.           */
    RV_RTSP_NPT_FORMAT_NOW,             /* NPT is set to the "now" constant. */
    RV_RTSP_NPT_FORMAT_SEC,             /* NPT is in seconds.               */
    RV_RTSP_NPT_FORMAT_HHMMSS           /* NPT is in the hh:mm:ss format.   */
} RvRtspNptFormat;

/*@****************************************************************************
 * Type: RvRtspNptTime (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * An NPT (Normal Play Time) structure. Represents the time since the beginning
 * of the stream.
 ***************************************************************************@*/
typedef struct
{
    RvRtspNptFormat format;             /* The format of the NPT.       */

    RvUint8         hours;              /* Hours since the beginning of the 
                                           stream. */
    RvUint8         minutes;            /* Minutes since the beginning of the 
                                           stream. */
    double          seconds;            /* Seconds since the beginning of the 
                                           stream. */
} RvRtspNptTime;

/*@****************************************************************************
 * Type: RvRtspCSeqHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Data structure for the CSeq mandatory header.
 ***************************************************************************@*/
typedef struct
{
    RvUint16    value;              /* The sequential number of the message. */
} RvRtspCSeqHeader;

/*@****************************************************************************
 * Type: RvRtspSessionHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Data structure for the Session header, containing the session ID and
 * the Server timeout (time in seconds between requests). If this timeout
 * is reached, the Server closes the session.
 * Session-Id is a string of unspecified length. Usually, 4 or 8 characters are
 * used. The RADVISION RTSP Toolkit limits this to four characters.
 ***************************************************************************@*/
typedef struct
{
    RvRtspStringHandle      hSessionId;     /* Server-determined string.        */
    RvUint16                timeout;        /* Server timeout, in seconds.      */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle   additionalFields;
#endif
} RvRtspSessionHeader;

/*@****************************************************************************
 * Type: RvRtspTransportHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Data structure for the Transport header, containing the RTP and RTCP
 * Client ports and Server ports.
 ***************************************************************************@*/
typedef struct
{
    RvUint16                clientPortA;            /* RTP Client port.        */
    RvUint16                clientPortB;            /* RTCP Client port.       */
    RvUint16                serverPortA;            /* RTP Server port.        */
    RvUint16                serverPortB;            /* RTCP Server port.       */
    RvBool                  isUnicast;              /* The transport unicast.  */
    RvChar                  destination[40];        /* The destination to which 
                                                       the Server should send. */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle   additionalFields;
#endif
} RvRtspTransportHeader;

/*@****************************************************************************
 * Type: RvRtspAcceptHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Data structure for the Accept header.
 ***************************************************************************@*/
typedef struct
{
    RvRtspStringHandle      hStr;           /* Acceptable description formats.  */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle   additionalFields;
#endif
} RvRtspAcceptHeader;

/*@****************************************************************************
 * Type: RvRtspRangeHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Data structure for the Range header, containing a start time and an end
 * time. A null start time means from the beginning. A null end time means 
 * until the end. If both are null, the header is empty.
 ***************************************************************************@*/
typedef struct
{
    RvRtspNptTime           startTime;          /* Time range start.                */
    RvRtspNptTime           endTime;            /* Time range end.                  */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle   additionalFields;
#endif
} RvRtspRangeHeader;

/*@****************************************************************************
 * Type: RvRtspConnectionHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * General header structure. Data structure for the Connection header.
 * Specifies options that should not be communicated by proxies.
 ***************************************************************************@*/
typedef struct
{
    RvRtspStringHandle      hStr;           /* Proxy directives.                */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle   additionalFields;
#endif
} RvRtspConnectionHeader;

/*@****************************************************************************
 * Type: RvRtspContentLanguageHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Entity (message body) header structure. Data structure for the
 * Content-Language header. Specifies the language of the entity data.
 ***************************************************************************@*/
typedef struct
{
    RvRtspStringHandle      hStr;           /* Language of the message body. */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle   additionalFields;
#endif
} RvRtspContentLanguageHeader;

/*@****************************************************************************
 * Type: RvRtspContentEncodingHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Entity (message body) header structure. Data structure for the
 * Content-Encoding header. Specifies the encoding in which the entity is
 * encoded, in addition to its content type.
***************************************************************************@*/
typedef struct
{
    RvRtspStringHandle      hStr;           /* Encoding of the message body. */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle   additionalFields;
#endif
} RvRtspContentEncodingHeader;

/*@****************************************************************************
 * Type: RvRtspContentBaseHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Entity (message body) header. Data structure for the Content-Base
 * header, containing the base URI for resolving relative URLs within the
 * entity.
 ***************************************************************************@*/
typedef struct
{
    RvRtspStringHandle     hStr;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle  additionalFields;
#endif
} RvRtspContentBaseHeader;

/*@****************************************************************************
 * Type: RvRtspContentLengthHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Entity (message body) header. Data structure for the Content-Length
 * header, containing the length of the message body.
 ***************************************************************************@*/
typedef struct
{
    RvUint16               value;              /* Length of the message body. */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle  additionalFields;
#endif
} RvRtspContentLengthHeader;

/*@****************************************************************************
 * Type: RvRtspContentTypeHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Entity (message body) header. Data structure for the Content-Type
 * header, specifies the content type of the message body.
 ***************************************************************************@*/
typedef struct
{
    RvRtspStringHandle     hStr;           /* Type of the message body. */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle  additionalFields;
#endif
} RvRtspContentTypeHeader;

/*@****************************************************************************
 * Type: RvRtspLocationHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Response header. Data structure for the Location header, used to redirect
 * the Client from the Request's request-line URI to the specified location.
 ***************************************************************************@*/
typedef struct
{
    RvRtspStringHandle      hStr;           /* The new location of the URI. */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle   additionalFields;
#endif
} RvRtspLocationHeader;

/*@****************************************************************************
 * Type: RvRtspRtpInfo (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Data structure for RTP-Info. Contains RTP-specific information
 * for RTP synchronization (a list of url, seq, rtptime).
 ***************************************************************************@*/
typedef struct
{
    RvRtspStringHandle      hURI;           /* URI to which is referred. */
    RvUint32                seq;            /* RTP sequence number.      */
    RvBool                  seqValid;       /* Seq specified.            */
    RvUint32                rtpTime;        /* RTP time.                 */
    RvBool                  rtpTimeValid;   /* RPT time specified.       */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle   additionalFields;
#endif
} RvRtspRtpInfo;

/*@****************************************************************************
 * Type: RvRtspRtpInfoHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Data structure for the "RTP-Info" header. Contains RvRptInfo structures.
 * Used for RTP synchronization.
 ***************************************************************************@*/
typedef struct
{
    RvRtspArrayHandle       hInfo;          /* RTP info.                    */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle   additionalFields;
#endif
} RvRtspRtpInfoHeader;

/*@****************************************************************************
 * Type: RvRtspRequireHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Request header. The data structure for the Require header.
 * The Require header is used by Clients to query the Server about
 * options that it may or may not support.
 ***************************************************************************@*/
typedef struct RvRtspRequireHeader
{
    RvRtspStringHandle     hStr;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle  additionalFields;
#endif
}RvRtspRequireHeader;

/*@****************************************************************************
 * Type: RvRtspPublicHeader (RtspHeaderTypes)
 * ----------------------------------------------------------------------------
 * Response header. The data structure for the Public header.
 * The public header is used by Servers to announce their supported options.
 ***************************************************************************@*/
typedef struct RvRtspPublicHeader
{
    RvRtspStringHandle     hStr;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvRtspMsgHeaderHandle  additionalFields;
#endif
}RvRtspPublicHeader;





/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_HEADERS_TYPES_H*/



