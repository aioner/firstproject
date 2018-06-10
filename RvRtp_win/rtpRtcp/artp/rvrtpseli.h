/***********************************************************************
Copyright (c) 2004 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..
RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/

/**************************************************************************
 *
 * Select Interface.
 *
 * Provides absrtact selection of file descriptors upon read, write and
 *  timeouts per task.
 *
 * Supporting calls from different tasks (using thread local storage).
 *
 **************************************************************************/

#ifndef _RV_RTP_SELI_H
#define _RV_RTP_SELI_H

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/

#include "rvtypes.h"
#include "rverror.h"


#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/
/* SELI events */
typedef enum
{
    RvRtpSeliEvRead = 0x01, /* READ event */
    RvRtpSeliEvWrite = 0x02 /* WRITE event */
} RvRtpSeliEvents;


/************************************************************************
 * RvRtpSeliCallback
 * purpose: Callback function returned on events
 * input  : fd      - Handle/file descriptor this event occured on
 *          sEvent  - Event that occured
 *          error   - RV_TRUE if there was an error in the event
 * output : none
 * return : none
 ************************************************************************/
typedef void (RVCALLCONV *RvRtpSeliCallback) (
    IN int               fd,
    IN RvRtpSeliEvents   sEvent,
    IN RvBool            error);


/*-----------------------------------------------------------------------*/
/*                           FUNCTIONS HEADERS                           */
/*-----------------------------------------------------------------------*/

/************************************************************************
 * RvRtpSeliInit
 * purpose: Initialize a SELI interface.
 * input  : none
 * output : none
 * return : RV_OK on success, negative value on failure
 ************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtpSeliInit(void);

/************************************************************************
 * RvRtpSeliEnd
 * purpose: End a SELI interface.
 * input  : none
 * output : none
 * return : RV_OK on success, negative value on failure
 ************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtpSeliEnd(void);

/******************************************************************************
 * RvRtpSeliSelectUntil
 * ----------------------------------------------------------------------------
 * General: Block on the select() interface on some operating systems.
 *          Use parallel interfaces on other operating systems.
 *          This function also gives the application the ability to give a
 *          maximum delay to block for.
 *
 * Return Value: RV_OK on success, negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  delay    - Maximum time to block on milliseconds.
 * Output: None.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtpSeliSelectUntil(IN RvUint32 delay);

/************************************************************************
 * RvRtpSeliCallOn
 * purpose: Ask the SELI interface for events on a given handle/file
 *          descriptor.
 * input  : fd              - Handle/File descriptor to wait on for events
 *          sEvents         - Events to wait for
 *          callbackFunc    - Function to call when the event occurs
 * output : none
 * return : RV_OK on success, negative value on failure
 ************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtpSeliCallOn(
    IN int                   fd,
    IN RvRtpSeliEvents       sEvents,
    IN RvRtpSeliCallback     callbackFunc);

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTP_SELI_H */
