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
 *                              <RvRtspClientInc.h>
 *
 * Interface functions for the entire RTSP library of the client stack. 
 * The RvRtsp Client module is responsible
 * for initializing and de-initializing the library, and to holding the RTSP sessions
 * and connections.
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							8/1/04
 *
 *********************************************************************************/

#ifndef _RV_RTSP_CLIENT_INC_H
#define _RV_RTSP_CLIENT_INC_H

#ifdef __cplusplus
extern "C" {
#endif


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER)
#include "RvRtspClient.h"
#include "RvRtspClientConnection.h"
#include "RvRtspClientSession.h"
#include "RvRtspClientTypes.h"
#include "RvRtspFirstLineTypes.h"
#include "RvRtspHeaderTypes.h"
#include "RvRtspMessageTypes.h"
#include "RvRtspUtils.h"
#include "RvRtspMsg.h"

#ifdef   USE_RTP
#include "rtp.h"
#include "rtcp.h"
#include "payload.h"
#endif

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER) */

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_CLIENT_INC_H*/


