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
 *                              <RtspObject.h>
 *
 * Interface functions for the entire RTSP library. The RvRtsp module is responsible
 * for initializing and de-initializing the library, and to holding the RTSP sessions
 * and connections.
 *
 *    Author                         Date
 *    ------                        ------
 *		Shaft						8/1/04
 *
 *********************************************************************************/

#ifndef _RV_RTSP_OBJECT_H
#define _RV_RTSP_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/

#include "rvtypes.h"
#include "rvccore.h"
#include "rvmutex.h"
#include "rvlog.h"
#include "rvloglistener.h"
#include "rpool.h"
#include "rvselect.h"
#include "ra.h"
#include "ares.h"
#include "rvares.h"

#include "RvRtspCommonTypes.h"

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

/* RvRtsp
 * ----------
 * The RTSP module structure, contains a list of the stack's connections,
 * access to the sessions is through the connections.
 */
typedef struct RvRtsp
{
	RvLogMgr			        *pLogMgr; 		/* Log manager for the entire RTSP module	*/
    RvBool                      bLogCreated;
    RvBool                      bCreatedLogMgr;
	RvLogListener		        logListener;    /* Log listener for the entire RTSP module	*/
	RvLogSource			        logSrc; 		/* Log source for the entire RTSP module	*/

	RvRtspConfigurationHandle	configuration; 	/* The configuration for the module			*/

	HRPOOL				        hRPool;			/* The module's memory pool					*/
	RvSelectEngine* 	        pSelectEngine;	/* Points to the module's select engine		*/
	RvDnsEngine     	        dnsEngine;		/* The module's dns engine					*/

	RvMutex				        mutex;			/* Rtsp module guard mutex					*/
	RvMutex				        gMutex;			/* global Rtsp stack mutex, used to 		*/
										        /* synchronize between destruction of		*/
										        /* objects and select/dns callbacks.		*/
	RvBool				        doTerminate;	/* Flag to signal termination to the main	*/
										        /* loop										*/
	RvBool				        terminated;		/* Flag to signal main loop terminated		*/

	HRA					        hRaConnections;	/* Connections array						*/
    HRA                         hRaRequests;    /* RA of RtspMsgRequests to be used when using RtspMsg module */
    HRA                         hRaResponses;   /* RA of RtspMsgResponses to be used when using RtspMsg module */
    HRA                         hRaHeaders;     /* RA of headers to be used when using RtspMsg module */ 
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT)
    HRA                          hRaSessions;    /* Sessions array                                     */
    HRA                          hRaListeningPorts; /* Listening Ports array                           */
#endif /* RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT */
} RvRtsp;



/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/



#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_OBJECT_H*/
