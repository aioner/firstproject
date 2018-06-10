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
 *                              <RvRtspFirstLineTypes.h>
 *
 *	This header file contains definitions of the RTSP message's first line.
 *	It includes the definitions for the request and status lines.
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							16/12/03
 *********************************************************************************/

#ifndef _RV_RTSP_FIRST_LINE_TYPES_H
#define _RV_RTSP_FIRST_LINE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILED                           */
/*-----------------------------------------------------------------------*/


/*@****************************************************************************
 * Module: RtspFirstLineTypes (root)
 * ----------------------------------------------------------------------------
 * Title:  9.RTSP First Line Type Definitions
 *
 * The RTSP first line type definitions are definitions of the first line
 * of the RTSP message.	It includes the definitions for the request and status
 * lines.
 *
 * You can find the RTSP first line type definitions in the
 * RvRtspFirstLineTypes.h file.
 ***************************************************************************@*/

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

/*@****************************************************************************
 * Type: RvRtspMethod (RtspFirstLineTypes)
 * ----------------------------------------------------------------------------
 * The RTSP methods, passed by the request messages.
 ***************************************************************************@*/
typedef enum
{
    RV_RTSP_METHOD_DESCRIBE = 0,
    RV_RTSP_METHOD_ANNOUNCE,            /* Not implemented. */
    RV_RTSP_METHOD_GET_PARAMETER,
    RV_RTSP_METHOD_OPTIONS,             /* Not implemented. */
    RV_RTSP_METHOD_PAUSE,
    RV_RTSP_METHOD_PLAY,
    RV_RTSP_METHOD_RECORD,              /* Not implemented. */
    RV_RTSP_METHOD_REDIRECT,
    RV_RTSP_METHOD_SETUP,
    RV_RTSP_METHOD_SET_PARAMETER,       /* Not implemented. */
    RV_RTSP_METHOD_TEARDOWN,
    RV_RTSP_METHOD_MAX,
    RV_RTSP_METHOD_UNKNOWN
} RvRtspMethod;

/*@****************************************************************************
 * Type: RvRtspStatus (RtspFirstLineTypes)
 * ----------------------------------------------------------------------------
 * The RTSP status (present on response messages) can be one of the statuses
 * listed below. It can be extended to include other methods. The statuses are
 * grouped according to their first digit, as follows:
 * 1xx: Informational - Request received, process is continued.
 * 2xx: Success - The action was received successfully, understood,
 *      and accepted.
 * 3xx: Redirection - Further action must be taken to
 *      complete the request.
 * 4xx: Client Error - The request contains bad syntax or cannot be
 *      fulfilled.
 * 5xx: Server Error - The Server failed to fulfill an apparently
 *      valid request.
 *
 * RvRtspStatus includes special enumerated values for working with the groups.
 * These appear in the following format: RV_RTSP_STATUS_GROUP_<group name>.
 ***************************************************************************@*/
typedef enum RvRtspStatus_
{
    RV_RTSP_STATUS_GROUP_INFORMATIONAL_BEGIN            = 100,
    RV_RTSP_STATUS_CONTINUE                             = 100,
    RV_RTSP_STATUS_GROUP_INFORMATIONAL_END              = 199,

    RV_RTSP_STATUS_GROUP_SUCCESS_BEGIN                  = 200,
    RV_RTSP_STATUS_OK                                   = 200,
    RV_RTSP_STATUS_CREATED                              = 201,
    RV_RTSP_STATUS_LOW_ON_STORAGE_SPACE                 = 250,
    RV_RTSP_STATUS_GROUP_SUCCESS_END                    = 299,

    RV_RTSP_STATUS_GROUP_REDIRECTION_BEGIN              = 300,
    RV_RTSP_STATUS_MULTIPLE_CHOICES                     = 300,
    RV_RTSP_STATUS_MOVED_PERMANENTLY                    = 301,
    RV_RTSP_STATUS_MOVED_TEMPORARILY                    = 302,
    RV_RTSP_STATUS_SEE_OTHER                            = 303,
    RV_RTSP_STATUS_NOT_MODIFIED                         = 304,
    RV_RTSP_STATUS_USE_PROXY                            = 305,
    RV_RTSP_STATUS_GROUP_REDIRECTION_END                = 399,

    RV_RTSP_STATUS_GROUP_CLIENT_ERROR_BEGIN             = 400,
    RV_RTSP_STATUS_BAD_REQUEST                          = 400,
    RV_RTSP_STATUS_UNAUTHORIZED                         = 401,
    RV_RTSP_STATUS_PAYMENT_REQUIRED                     = 402,
    RV_RTSP_STATUS_FORBIDDEN                            = 403,
    RV_RTSP_STATUS_NOT_FOUND                            = 404,
    RV_RTSP_STATUS_METHOD_NOT_ALLOWED                   = 405,
    RV_RTSP_STATUS_NOT_ACCEPTABLE                       = 406,
    RV_RTSP_STATUS_PROXY_AUTHENTICATION_REQUIRED        = 407,
    RV_RTSP_STATUS_REQUEST_TIME_OUT                     = 408,
    RV_RTSP_STATUS_GONE                                 = 410,
    RV_RTSP_STATUS_LENGTH_EQUIRED                       = 411,
    RV_RTSP_STATUS_PRECONDITION_FAILED                  = 412,
    RV_RTSP_STATUS_REQUEST_ENTITY_TOO_LARGE             = 413,
    RV_RTSP_STATUS_REQUEST_URI_TOO_LARGE                = 414,
    RV_RTSP_STATUS_UNSUPPORTED_MEDIA_TYPE               = 415,
    RV_RTSP_STATUS_PARAMETER_NOT_UNDERSTOOD             = 451,
    RV_RTSP_STATUS_CONFERENCE_NOT_FOUND                 = 452,
    RV_RTSP_STATUS_NOT_ENOUGH_BANDWIDTH                 = 453,
    RV_RTSP_STATUS_SESSION_NOT_FOUND                    = 454,
    RV_RTSP_STATUS_METHOD_NOT_VALID_IN_THIS_STATE       = 455,
    RV_RTSP_STATUS_HEADER_FIELD_NOT_VALID_FOR_RESOURCE  = 456,
    RV_RTSP_STATUS_INVALID_RANGE                        = 457,
    RV_RTSP_STATUS_PARAMETER_IS_READ_ONLY               = 458,
    RV_RTSP_STATUS_AGGREGATE_OPERATION_NOT_ALLOWED      = 459,
    RV_RTSP_STATUS_ONLY_AGGREGATE_OPERATION_ALLOWED     = 460,
    RV_RTSP_STATUS_UNSUPPORTED_TRANSPORT                = 461,
    RV_RTSP_STATUS_DESTINATION_UNREACHABLE              = 462,
    RV_RTSP_STATUS_GROUP_CLIENT_ERROR_END               = 499,

    RV_RTSP_STATUS_GROUP_SERVER_ERROR_BEGIN             = 500,
    RV_RTSP_STATUS_INTERNAL_SERVER_ERROR                = 500,
    RV_RTSP_STATUS_NOT_IMPLEMENTED                      = 501,
    RV_RTSP_STATUS_BAD_GATEWAY                          = 502,
    RV_RTSP_STATUS_SERVICE_UNAVAILABLE                  = 503,
    RV_RTSP_STATUS_GATEWAY_TIME_OUT                     = 504,
    RV_RTSP_STATUS_RTSP_VERSION_NOT_SUPPORTED           = 505,
    RV_RTSP_STATUS_OPTION_NOT_SUPPORTED                 = 551,
    RV_RTSP_STATUS_GROUP_SERVER_ERROR_END               = 599,


    RV_RTSP_STATUS_GROUP_INTERNAL_STACK_BEGIN           = 5000,
    RV_RTSP_STATUS_RESPONSE_TIME_OUT                    = 5000,
    RV_RTSP_STATUS_GROUP_INTERNAL_STACK_END             = 5099
} RvRtspStatus;

/*@****************************************************************************
 * Type: RvRtspRequestLine (RtspFirstLineTypes)
 * ----------------------------------------------------------------------------
 * The request line contains the request code and the requested URI string.
 ***************************************************************************@*/
typedef struct
{
    RvRtspMethod        method;     /* The requested method. */
    RvRtspStringHandle  methodStr;  /* The method string can be used for 
                                       propriety methods. The Stack will use it
                                       if the method field is set to 
                                       RV_RTSP_METHOD_UNKNOWN. */  
    RvRtspStringHandle  hURI;       /* The message URI. */
} RvRtspRequestLine;


/* Type: RvRtspStatusLine (RtspFirstLineTypes)
 * ----------
 * The status line contains the status code and the status phrase.
 ***************************************************************************@*/
typedef struct
{
	RvRtspStatus		status; /* The code for the Server's response to the 
								   request. */
	RvRtspStringHandle	hPhrase;/* The human-readable phrase for the response 
	                               code. */
} RvRtspStatusLine;


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/



#ifdef __cplusplus
}
#endif

#endif	/* Include guard */



