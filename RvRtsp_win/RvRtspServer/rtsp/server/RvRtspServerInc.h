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
 *                              <RvRtspServerInc.h>
 *
 * The application include file. The application should include this file to 
 * get access to to all the APIs.
 *
 *********************************************************************************/

#ifndef _RV_RTSP_SERVER_INC_H
#define _RV_RTSP_SERVER_INC_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
// #if (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT)
#include "RvRtspServer.h"
#include "RvRtspServerConnection.h"
#include "RvRtspServerSession.h"
#include "RvRtspServerTypes.h"
#include "RvRtspFirstLineTypes.h"
#include "RvRtspHeaderTypes.h"
#include "RvRtspMessageTypes.h"
#include "RvRtspUtils.h"
#include "RvRtspMsg.h"

#include "rvsdp.h"
#ifdef USE_RTP
#include "rtp.h"
#include "rtcp.h"
#include "payload.h"
#endif  /* ifdef USE_RTP */

// #endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT) */
    
#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_SERVER_INC_H*/


