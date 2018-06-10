/* rvadtstamp.c - Windows adapter timestamp functions */
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
#include "rvadtstamp.h"
#include "rvtime.h"


RvInt64 RvWinTimestampGet(void);
RvAdTimestampGet_f RvAdTimestampGet = RvWinTimestampGet;

static RvInt64 RvTickHz; /* Clock frequency (cycles/second) - set by Init */


RvStatus RvAdTimestampInit(void)
{
    LARGE_INTEGER Frequency;

    if (QueryPerformanceFrequency(&Frequency) == 0)
    {
        RvTickHz = RvInt64Const(1,0,100000000); /* just to prevent accidental divide by zero */
        return RV_ERROR_UNKNOWN;
    }

    RvTickHz = (RvInt64)Frequency.QuadPart;

    return RV_OK;
}


void RvAdTimestampEnd(void)
{
}


RvInt64 RvWinTimestampGet(void)
{
    RvInt64 result;
    LARGE_INTEGER PerformanceCount;

    QueryPerformanceCounter(&PerformanceCount);
    
    /* convert to nanoseconds and maintain resolution */
    result = (PerformanceCount.QuadPart / RvTickHz) * RV_TIME64_NSECPERSEC;
    result += (((PerformanceCount.QuadPart % RvTickHz) * RV_TIME64_NSECPERSEC) / RvTickHz);

    return result;
}


RvInt64 RvAdTimestampResolution(void)
{
    RvInt64 result;

    if(RvInt64IsGreaterThan(RvTickHz, RvInt64Const(1,0,1)))
        result = RvInt64Div(RV_TIME64_NSECPERSEC, RvTickHz);
    else
        result = RV_TIME64_NSECPERSEC;
    
    return result;
}
