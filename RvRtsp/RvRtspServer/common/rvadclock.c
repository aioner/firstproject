/* rvadclock.c - Windows adapter clock functions */
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
#include "rvadclock.h"
#include <time.h>
#include <sys/time.h>


/* Windows epoc is Jan 1, 1601, not Jan 1 1970 (100ns increments)*/
#define EPOCHDELTA RvUint64Const(0x19db1de,0xd53e8000)


void RvAdClockInit(void)
{
}


void RvAdClockGet(
    OUT RvTime*         t)
{
#if (RV_CLOCK_TYPE == RV_CLOCK_LINUX)
    struct timeval tv;
#endif

 #if (RV_CLOCK_TYPE == RV_CLOCK_WIN32)
    SYSTEMTIME systime;
    FILETIME filetime;
    ULARGE_INTEGER *uint_time;
#endif

#if (RV_CLOCK_TYPE == RV_CLOCK_WIN32)    
    GetSystemTime(&systime);
    SystemTimeToFileTime(&systime, &filetime);
    uint_time = (ULARGE_INTEGER *)&filetime;
    uint_time->QuadPart = uint_time->QuadPart - EPOCHDELTA;
    RvTimeSetSecs(t, (RvInt32)(uint_time->QuadPart / 10000000Ui64));
    RvTimeSetNsecs(t, (RvInt32)((uint_time->QuadPart % 10000000Ui64) * 100Ui64));
#endif    

#if (RV_CLOCK_TYPE == RV_CLOCK_LINUX)
    gettimeofday(&tv, NULL);
    TIMEVAL_TO_TIMESPEC(&tv, (struct timespec *)t);
#endif

}


RvStatus RvAdClockSet(
    IN  const RvTime*   t)
{
#if (RV_CLOCK_TYPE == RV_CLOCK_LINUX) || (RV_CLOCK_TYPE == RV_CLOCK_UNIXWARE)
    struct timeval tv;
#endif

#if (RV_CLOCK_TYPE == RV_CLOCK_WIN32)
    SYSTEMTIME systime;
    FILETIME filetime;
    ULARGE_INTEGER *uint_time;
#endif    
#if (RV_CLOCK_TYPE == RV_CLOCK_WIN32)
    uint_time = (ULARGE_INTEGER *)&filetime;
    uint_time->QuadPart = (ULONGLONG)RvTimeGetSecs(t) * 10000000Ui64 +
        (ULONGLONG)RvTimeGetNsecs(t) / 100Ui64 + EPOCHDELTA;
    if((FileTimeToSystemTime(&filetime, &systime) == 0) || (SetSystemTime(&systime) == 0))
        return (RV_ERROR_UNKNOWN);
#endif

#if (RV_CLOCK_TYPE == RV_CLOCK_LINUX)
    TIMESPEC_TO_TIMEVAL(&tv, (struct timespec *)t);
    if(settimeofday(&tv, NULL)!= 0)
        return (RV_ERROR_UNKNOWN);
#endif

    return RV_OK;
}


void RvAdClockResolution(
    OUT RvTime*         t)
{
    RvTimeSetSecs(t, RvInt32Const(0));
    RvTimeSetNsecs(t, (RvInt32)(RV_TIME_NSECPERSEC / CLOCKS_PER_SEC));
}
