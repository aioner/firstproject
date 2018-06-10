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
 *                              <RvRtspServerTypes.h>
 *
 * This header contains the definitions of data types and constants relevant to
 * the entire module.
 *
 *********************************************************************************/

#ifndef _RV_RTSP_SERVER_TYPES_H
#define _RV_RTSP_SERVER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT)
#include "rvtypes.h"
#include "rverror.h"

/*@****************************************************************************
 * Module: RtspServerTypes (root)
 * ----------------------------------------------------------------------------
 * Title:  4.RTSP Server Type Definitions
 *
 * The RTSP Server types are definitions of data types and constants relevant
 * to the entire module.
 *
 * You can find the RTSP Server types in the RvRtspServerTypes.h file.
 ***************************************************************************@*/

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

/*@****************************************************************************
 * Type: RvRtspListeningConnectionHandle (RtspServerTypes)
 * ----------------------------------------------------------------------------
 * Handle to the RvRtspListeningConnection object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspListeningConnectionHandle);

/*@****************************************************************************
 * Type: RvRtspServerConnectionHandle (RtspServerTypes)
 * ----------------------------------------------------------------------------
 * Handle to the RvRtspServerConnection object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspServerConnectionHandle);

/*@****************************************************************************
 * Type: RvRtspServerConnectionAppHandle (RtspServerTypes)
 * ----------------------------------------------------------------------------
 * Handle to the RvRtspServerConnectionApp object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspServerConnectionAppHandle);

/*@****************************************************************************
 * Type: RvRtspServerSessionHandle (RtspServerTypes)
 * ----------------------------------------------------------------------------
 * Handle to the RvRtspServerSession object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspServerSessionHandle);

/*@****************************************************************************
 * Type: RvRtspServerSessionAppHandle (RtspServerTypes)
 * ----------------------------------------------------------------------------
 * Handle to the RvRtspServerSessionApp object.
 ***************************************************************************@*/
RV_DECLARE_HANDLE(RvRtspServerSessionAppHandle);

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT) */

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_SERVER_TYPES_H*/



