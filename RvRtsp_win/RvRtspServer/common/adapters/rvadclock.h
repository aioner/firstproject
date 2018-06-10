/* rvadclock.h - rvadclock header file */
/************************************************************************
      Copyright (c) 2001,2002 RADVISION Inc. and RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Inc. and RADVISION Ltd.. No part of this document may be
reproduced in any form whatsoever without written prior approval by
RADVISION Inc. or RADVISION Ltd..

RADVISION Inc. and RADVISION Ltd. reserve the right to revise this
publication and make changes without obligation to notify any person of
such revisions or changes.
***********************************************************************/

#ifndef RV_ADCLOCK_H
#define RV_ADCLOCK_H


#include "rvccore.h"
#include "rvtime.h"


#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************************************
 * RvAdTimestampInit
 *
 * Called by RvTimestampInit.
 * Allows the timestamp adapter to perform OS specific module initialization.
 *
 * INPUT   : None.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
void RvAdClockInit(void);


/********************************************************************************************
 * RvAdTimestampGet
 *
 * Called by RvTimestampGet.
 * Returns a timestamp value in nanoseconds.  Values always grow up linearly.
 *
 * INPUT   : None.
 * OUTPUT  : None.
 * RETURN  : Nanosecond timestamp.
 */
void RvAdClockGet(
    OUT RvTime*         t);


/********************************************************************************************
 * RvAdTimestampGet
 *
 * Called by RvTimestampGet.
 * Returns a timestamp value in nanoseconds.  Values always grow up linearly.
 *
 * INPUT   : None.
 * OUTPUT  : None.
 * RETURN  : Nanosecond timestamp.
 */
RvStatus RvAdClockSet(
    IN  const RvTime*   t);


/********************************************************************************************
 * RvAdTimestampResolution
 *
 * Called by RvTimestampResolution.
 * Returns the resolution of the timestamp in nanoseconds.
 *
 * INPUT   : None.
 * OUTPUT  : None.
 * RETURN  : Resolution of the timestamp in nanoseconds.
 */
void RvAdClockResolution(
    OUT RvTime*         t);

#ifdef __cplusplus
}
#endif


#endif
