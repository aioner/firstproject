/*
 *******************************************************************************
 *                                                                             *
 * NOTICE:                                                                     *
 * This document contains information that is confidential and proprietary to  *
 * RADVision LTD.. No part of this publication may be reproduced in any form   *
 * whatsoever without written prior approval by RADVision LTD..                *
 *                                                                             *
 * RADVision LTD. reserves the right to revise this publication and make       *
 * changes without obligation to notify any person of such revisions           *
 * or changes.                                                                 *
 *******************************************************************************
 */


/*******************************************************************************
 *                         <RtspServerSession.h>
 *
 * This file contains definitions relevant to the RTSP server session
 * initializations called from the server.
 *
 ******************************************************************************/


#ifndef _RTSP_SERVER_SESSION_H
#define _RTSP_SERVER_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                             FUNCTION HEADERS                               */
/*----------------------------------------------------------------------------*/

/**************************************************************************
 * RtspServerSessionInit
 * ------------------------------------------------------------------------
 * General: 
 *  This method constructs an RA object for the session.
 *
 * Arguments:
 * Input:   maxSessions - maximum number of sessions.
 *          logMgr      - log manager used.
 * Output:  None.
 *
 * Return Value:  HRA - constructed RA element.
 *************************************************************************/
HRA RtspServerSessionInit(
         IN RvUint           maxSessions,
         IN RvLogMgr*        logMgr);

#ifdef __cplusplus
}
#endif

#endif  /* end of define _RTSP_SERVER_SESSION_H */

