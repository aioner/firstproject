/* rvtimestamp.c - high resolution timestamp functions */
/************************************************************************
        Copyright (c) 2001 RADVISION Inc. and RADVISION Ltd.
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
#include "rvtimestamp.h"
#include "rvstdio.h"


/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvTstampLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->tstampSource, p);}
#else
#define RvTstampLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvTstampLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->tstampSource, p);}
#else
#define RvTstampLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvTstampLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->tstampSource, p);}
#else
#define RvTstampLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif



/********************************************************************************************
 * RvTimestampInit - Initializes the Timestamp module.
 *
 * Must be called once (and only once) before any other functions in the module are called.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvTimestampInit(void)
{
    RvStatus result;

    /* OS specific setup */
    result = RvAdTimestampInit();

    return result;
}


/********************************************************************************************
 * RvTimestampEnd - Shuts down the Timestamp module.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvTimestampEnd(void)
{
    RvAdTimestampEnd();

    return RV_OK;
}


/********************************************************************************************
 * RvTimestampSourceConstruct - Constructs timestamp module log source.
 *
 * Constructs log source to be used by common core when printing log from the
 * timestamp module. This function is applied per instance of log manager.
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvTimestampSourceConstruct(
    IN RvLogMgr *logMgr)
{
    RvStatus result;

    result = RvLogSourceConstruct(logMgr, &logMgr->tstampSource, "TIMESTAMP", "High resolution timestamp");

    return result;
}



/********************************************************************************************
 * RvTimestampGet - Gets a timestamp value in nanoseconds.
 *
 * Values returned by subsequent calls are guaranteed to be linear
 * (will never go backwards) and will never wrap.
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : none
 * RETURN  : Nanosecond timestamp.
 */
RVCOREAPI
RvInt64 RVCALLCONV RvTimestampGet(
    IN  RvLogMgr    *logMgr)
{
    RvInt64 result;

    RvTstampLogEnter((&logMgr->tstampSource, "RvTimestampGet"));

    result = RvAdTimestampGet();

    RvTstampLogLeave((&logMgr->tstampSource, "RvTimestampGet"));

    return result;
}



/********************************************************************************************
 * RvTimestampResolution - Gets the resolution of the timestamp in nanoseconds.
 *
 * INPUT   : none
 * OUTPUT  : logMgr - log manager instance
 * RETURN  : Resolution of the timestamp in nanoseconds.
 */
RVCOREAPI
RvInt64 RVCALLCONV RvTimestampResolution(
    IN  RvLogMgr    *logMgr)
{
    RvInt64 result;

    RvTstampLogEnter((&logMgr->tstampSource, "RvTimestampResolution"));

    result = RvAdTimestampResolution();

    RvTstampLogLeave((&logMgr->tstampSource, "RvTimestampResolution"));

    return result;
}
