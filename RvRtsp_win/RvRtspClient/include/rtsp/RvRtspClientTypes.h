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
 *		Shaft						8/1/04
 *		Shaft						14/05/2006 -	added definitions for new ccore
 *													1.1.12 - mostly DNS interface has changed
 *
 *********************************************************************************/

#ifndef _RV_RTSP_CLIENT_TYPES_H
#define _RV_RTSP_CLIENT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER)
#include "rvtypes.h"
#include "rverror.h"


/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/


/* Handle to the RvRtspConnection object */
RV_DECLARE_HANDLE(RvRtspConnectionHandle);
/* Handle to the RvRtspConnectionDescribeAppHandle object */
RV_DECLARE_HANDLE(RvRtspConnectionDescribeAppHandle);

/* Handle to the RvRtspConnectionApp object */
RV_DECLARE_HANDLE(RvRtspConnectionAppHandle);

/* Handle to the RvRtspSession object */
RV_DECLARE_HANDLE(RvRtspSessionHandle);

/* Handle to the RvRtspSessionApp object */
RV_DECLARE_HANDLE(RvRtspSessionAppHandle);

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER) */

#ifdef __cplusplus
}
#endif

#endif  /* _RV_RTSP_CLIENT_TYPES_H */



