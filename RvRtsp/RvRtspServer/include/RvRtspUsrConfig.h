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
 *                              <RvRtspUsrConfig.h>
 *
 * General user definitions for the Rtsp stack.
 *
 *    Author                         Date
 *    ------                        ------
 *     Anat                         16.05.07
 *
 *********************************************************************************/

#ifndef _RV_RTSP_USR_CONFIG_H
#define _RV_RTSP_USR_CONFIG_H

/*@****************************************************************************
 * Type: RV_RTSP_TYPE_CLIENT (RvRtspUsrConfig)
 * ----------------------------------------------------------------------------
 * A compilation flag to enforce client only compilation
 ***************************************************************************@*/
#define RV_RTSP_TYPE_CLIENT                 0x01

/*@****************************************************************************
 * Type: RV_RTSP_TYPE_SERVER (RvRtspUsrConfig)
 * ----------------------------------------------------------------------------
 * A compilation flag to enforce server only compilation
 ***************************************************************************@*/
#define RV_RTSP_TYPE_SERVER                 0x02

/*@****************************************************************************
 * Type: RV_RTSP_TYPE_CLIENT_AND_SERVER (RvRtspUsrConfig)
 * ----------------------------------------------------------------------------
 * A compilation flag to enforce server and client compilation
 ***************************************************************************@*/
#define RV_RTSP_TYPE_CLIENT_AND_SERVER      0x04

/*@****************************************************************************
 * Type: RV_RTSP_TYPE (RvRtspUsrConfig)
 * ----------------------------------------------------------------------------
 * A compilation flag to reinforce the compilation type:
 *  1. RV_RTSP_TYPE_CLIENT              - Client only compilation
 *  2. RV_RTSP_TYPE_SERVER              - Server only compilation
 *  3. RV_RTSP_TYPE_CLIENT_AND_SERVER   - Client and Server compilation
 ***************************************************************************@*/
#undef RV_RTSP_TYPE
#define RV_RTSP_TYPE  RV_RTSP_TYPE_CLIENT_AND_SERVER   /* RV_RTSP_TYPE_CLIENT */  /* RV_RTSP_TYPE_SERVER */

/*@****************************************************************************
 * Type: RV_RTSP_USE_RTSP_MSG (RvRtspUsrConfig)
 * ----------------------------------------------------------------------------
 * A compilation flag to enable/disable the use of the generic messages in the
 * Rtsp stack.
 ***************************************************************************@*/
#undef RV_RTSP_USE_RTSP_MSG
#define RV_RTSP_USE_RTSP_MSG RV_YES

#endif  /* _RV_RTSP_USR_CONFIG_H */


