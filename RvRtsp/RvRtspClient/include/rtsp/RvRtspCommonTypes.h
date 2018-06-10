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
 *                              <rvRtspClientTypes.h>
 *
 * This header contains the definitions of data types and constants relevant to
 * the entire module.
 *
 *    Author                         Date
 *    ------                        ------
 *      Shaft                       8/1/04
 *      Shaft                       14/05/2006 -    added definitions for new ccore
 *                                                  1.1.12 - mostly DNS interface has changed
 *
 *********************************************************************************/

#ifndef _RV_RTSP_COMMON_TYPES_H
#define _RV_RTSP_COMMON_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "rvtypes.h"
#include "rverror.h"
#include "RvRtspUsrConfig.h"

/*@****************************************************************************
 * Module: RtspCommonTypes (root)
 * ----------------------------------------------------------------------------
 * Title:  2.RTSP Common Type Definitions
 *
 * The RTSP common type definitions are definitions of data types and constants
 * relevant to the entire module.
 *
 * You can find the RTSP common type definitions in the
 * rvRtspCommonTypes.h file.
 ***************************************************************************@*/

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/
/*@****************************************************************************
 * Page: RTSPClientTypeDefinitions (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Title: RTSP Client Type Definitions
 *
 * The RTSP Client type definitions are definitions of data types and constants
 * relevant to the entire module.
 ***************************************************************************@*/

/*@****************************************************************************
 * Type: RV_RTSP_SERVER_DEFAULT_PORT (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Default port for RTSP communication.
 ***************************************************************************@*/
#define RV_RTSP_SERVER_DEFAULT_PORT         554


/*@****************************************************************************
 * Type: RV_RTSP_SERVER_NAME_MAX_LENGTH (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Maximum length for the Server domain name.
 ***************************************************************************@*/
#define RV_RTSP_SERVER_NAME_MAX_LENGTH      512


/*@****************************************************************************
 * Type: RV_RTSP_IP_ADDRESS_MAX_LENGTH (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Maximum length of an IP address.
 ***************************************************************************@*/
#define RV_RTSP_IP_ADDRESS_MAX_LENGTH       50


/*@****************************************************************************
 * Type: TERMINATING_DELIMITERS_STRING (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Used for message parsing.
 ***************************************************************************@*/
#define TERMINATING_DELIMITERS_STRING       "\r\n"


/*@****************************************************************************
 * Type: DNS_TCP_RECEIVE_BUFFER_LENGTH (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Length of the TCP buffer used to receive DNS replies.
 ***************************************************************************@*/
#define DNS_TCP_RECEIVE_BUFFER_LENGTH       2048


/*@****************************************************************************
 * Type: RvRtspHandle (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Handle to the RvRtsp object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspHandle);

/*@****************************************************************************
 * Type: RvRtspConfigurationHandle (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Handle to the configuration object of the Stack.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspConfigurationHandle);

/*@****************************************************************************
 * Type: RvLogHandle (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Handle to the logMgr object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvLogHandle);

/*@****************************************************************************
 * Type: RvRtspStringHandle (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Handle to a string object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspStringHandle);

/*@****************************************************************************
 * Type: RvRtspBlobHandle (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Handle to a memory block object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspBlobHandle);

/*@****************************************************************************
 * Type: RvRtspArrayHandle (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Handle to an array object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspArrayHandle);

/*@****************************************************************************
 * Type: RvRtspMsgMessageHandle (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Handle to an RtspMsgRequest or RtspMsgResponse object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspMsgMessageHandle);

/*@****************************************************************************
 * Type: RvRtspMsgHeaderHandle (RtspCommonTypes)
 * ----------------------------------------------------------------------------
 * Handle to an RtspMsgHeader object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspMsgHeaderHandle);

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_COMMON_TYPES_H*/



