/************************************************************************
 File Name	   : rvrtplogger.h
 Description   : Functions declarations related to logger printouts
*************************************************************************
************************************************************************
        Copyright (c) 2005 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..

RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************
$Revision: $
$Date:   03/29/2005 $
$Author: Michael Zak $
************************************************************************/

#ifndef __RV_RTP_LOGGER_H
#define __RV_RTP_LOGGER_H

#include "rtp.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum _RvRtpModules
{
   RVRTP_GLOBAL_MASK = -1,
   RVRTP_RTP_MODULE  = 0,
   RVRTP_RTCP_MODULE,
   RVRTP_PAYLOAD_MODULE,
   RVRTP_SECURITY_MODULE,
   RVRTP_SRTP_MODULE,
   RVRTP_RTCP_XR_MODULE,
   RVRTP_FEC_MODULE,
   RVRTP_RTCP_FB_MODULE,
   RVRTP_MODULES_NUMBER                 /*  defines number of modules for logger */

} RvRtpModule;

typedef enum _RvRtpLoggerFilters
{
	RVRTP_LOG_DEBUG_FILTER    = 0x01,
	RVRTP_LOG_INFO_FILTER     = 0x02,
	RVRTP_LOG_WARN_FILTER     = 0x04,
	RVRTP_LOG_ERROR_FILTER    = 0x08,
	RVRTP_LOG_EXCEP_FILTER    = 0x10,
	RVRTP_LOG_LOCKDBG_FILTER  = 0x20,
	RVRTP_LOG_ENTER_FILTER    = 0x40,
	RVRTP_LOG_LEAVE_FILTER    = 0x80
		
} RvRtpLoggerFilterBit;

/* Mask type of Filter bits */
typedef RvUint8 RvRtpLoggerFilter; /* Filter Mask */

/***************************************************************************
* RvRtpPrintLogEntry_CB
* ------------------------------------------------------------------------
* General: Notifies the application each time a line should be printed to
*          the log. The application can decide whether to print the line
*          to the screen, file or other output device. You set this
*          callback by RvRtpSetPrintEntryCallback. 
*          If you do not implement this function a default logging
*          will be used and the line will be written to the rtplog.txt file.
* Return Value: (-)
* ------------------------------------------------------------------------
* Arguments:
* Input:     context - The context that was given in the callback registration
*                    process.
*             filter -    The filter that this message is using (info, error..)
*            formattedText - The text to be printed to the log. The text
*                          is formatted as follows:
*                          <filer> - <module> - <message>
*                          for example:
*                          "INFO  - STACK - Stack was constructed successfully"
***************************************************************************/
typedef void
(RVCALLCONV * RvRtpPrintLogEntry_CB)(
     IN void*           context,
     IN RvRtpLoggerFilter filter,
     IN const RvChar   *formattedText);
    
#if(RV_LOGMASK != RV_LOGLEVEL_NONE)
/************************************************************************************
 * RvRtpGetLogManager
 * description:  Gets pointer to log-manager used in RTP stack
 * input:  none
 * output: logMgrPtr.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * NOTE: must be used after calling of RvRtpCreateLogManager or RvRtpSetExternalLogManager
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV RvRtpGetLogManager(
	OUT RvRtpLogger* logMgrPtr);
/************************************************************************************
 * RvRtpCreateLogManager
 * description:  creates and executes internal log - manager
 *               and after it sets for all modules logger masks to default masks
 * output: logMgrPtr.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * NOTE: 1) RV_LOGLISTENER_TYPE must be difined in common core
 *       2) RV_RTP_LOGFILE_NAME must be definef in rvrtpconfig.h
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV  RvRtpCreateLogManager(
	OUT RvRtpLogger* logMgrPtr);
/************************************************************************************
 * RvRtpSetExternalLogManager
 * description:  sets external log - manager for RTP logging
 *               and after it sets for all modules logger masks to default masks
 * input: loggerPtr - pointer to external log manager.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV RvRtpSetExternalLogManager(
	IN RvRtpLogger* loggerPtr);
/************************************************************************************
 * RvRtpDestructLogManager
 * description:  destructs log- manager
 * input: logMgrPtr.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * NOTE: 1) must be created by RvRtpCreateLogManager or used external
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV  RvRtpDestructLogManager(
	IN RvRtpLogger* loggerPtr);
/************************************************************************************
 * RvRtpSetLogModuleFilter
 * description:  sets logger mask for appropriate module
 * input: module - module enumerator.
 *        filter - mask-filter
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 ***********************************************************************************/ 

RVAPI
RvStatus RVCALLCONV RvRtpSetLogModuleFilter(
	IN RvRtpModule module,
	IN RvRtpLoggerFilter filter);

/************************************************************************************
 * RvRtpSetPrintEntryCallback
 * description:  sets user defined callback function for entry printing
 * input: printEntryCB - user defined callback function.
 *        context - user-context
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV RvRtpSetPrintEntryCallback( 
	   IN RvRtpPrintLogEntry_CB printEntryCB,
	   IN void* context);

#endif /*(RV_LOGMASK != RV_LOGLEVEL_NONE)*/

#ifdef __cplusplus
}
#endif


#endif /*__RV_RTP_LOGGER_H*/

