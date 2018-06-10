/* rvtm.c - tm functions - Calendar time */
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
#include "rvtm.h"
#include "rvansi.h"

#if (RV_OS_TYPE == RV_OS_TYPE_HPUX) && (RV_OS_VERSION == RV_OS_HPUX_11)
#include <time.h>
#endif

/* Required OS Specific headers files */
#if (RV_TM_TYPE == RV_TM_VXWORKS)
#include <vxWorks.h>
#include <sysLib.h>
#include <tickLib.h>
#include <limits.h>
#include <wdLib.h>

#elif (RV_TM_TYPE == RV_TM_PSOS)
#include <psos.h>

#elif (RV_TM_TYPE == RV_TM_OSE)
#include <ose.h>
#include <limits.h>

#elif (RV_TM_TYPE == RV_TM_NUCLEUS)
#include <nucleus.h>
#include <target.h>
#include <limits.h>
static char *RvAsctime_r(const struct tm *timeptr, char *result);
static struct tm *RvGmtime_r(time_t t, struct tm *tm);
static struct tm *RvLocaltime_r(time_t t, struct tm *tm);

#elif (RV_TM_TYPE == RV_TM_WINCE)

struct tm * __cdecl localtime(const time_t *);

static char *RvAsctime_r(const struct tm *timeptr, char *result);
static struct tm *RvGmtime_r(time_t t, struct tm *tm);
static struct tm *RvLocaltime_r(time_t t, struct tm *tm);

size_t strftime(char *s, size_t maxs, const char *f, const struct tm *t);
static void strfmt(char *str, const char *fmt, ...);

#elif ((RV_TM_TYPE == RV_TM_MOPI) && !defined(_WIN32)) || \
      (RV_TM_TYPE == RV_TM_OSA) 

static char *RvAsctime_r(const struct tm *timeptr, char *result);
static struct tm *RvGmtime_r(time_t t, struct tm *tm);
static struct tm *RvLocaltime_r(time_t t, struct tm *tm);
#if (RV_TM_TYPE != RV_TM_OSA)
size_t strftime(char *s, size_t maxs, const char *f, const struct tm *t);
#endif
static void strfmt(char *str, const char *fmt, ...);

#endif


/* Lets make error codes a little easier to type */
#define RvTmErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_TM, (_e))

/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvTmLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->tmSource, p);}
#else
#define RvTmLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvTmLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->tmSource, p);}
#else
#define RvTmLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvTmLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->tmSource, p);}
#else
#define RvTmLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif


/********************************************************************************************
 * RvTmInit - Initializes the Tm module.
 *
 * Must be called once (and only once) before any other functions in the module are called.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : Always RV_OK
 */
RvStatus RvTmInit(void)
{
    return RV_OK;
}


/********************************************************************************************
 * RvTmEnd - Shuts down the Tm module.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : Always RV_OK
 */
RvStatus RvTmEnd(void)
{
    return RV_OK;
}


/********************************************************************************************
 * RvTmSourceConstruct - Constructs tm module log source.
 *
 * Constructs log source to be used by common core when printing log from the
 * tm module. This function is applied per instance of log manager.
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvTmSourceConstruct(
    IN RvLogMgr *logMgr)
{
    RvStatus result = RV_OK;

    result = RvLogSourceConstruct(logMgr, &logMgr->tmSource, "TM", "Calendar time");

    return result;
}


/********************************************************************************************
 * RvTmConvertToTime - Converts a calendar time into a standard time (seconds and nanoseconds since 1970.
 *
 * This function will also normalize the values of the calendar object (which might even
 * be the sole purpose for calling it). For example, if the value of seconds is set to 65,
 * it will be normalized to a value of 5 and minutes will be incremented along with any
 * other values that might be affected by it.
 * note:    The date range of calendar times is larger than the range that can be
 *          represented by time. If the calendar time fall outside of this range
 *          then the time result will not be correct (but the calenter time will
 *          still be normalized properly).
 *
 * INPUT   : tm     - pointer to calendar time that is to be converted.
 *           logMgr - log manager instance.
 * OUTPUT  : t      - pointer to time structure where the result will be stored.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTmConvertToTime(
    IN  RvTm        *tm,
    IN  RvLogMgr*   logMgr,
    OUT RvTime      *t)
{
    time_t secs;
    RvStatus result;
#if (RV_TM_TYPE == RV_TM_OSE)
    int tmpdst;
#endif

    RvTmLogEnter((&logMgr->tmSource, "RvTmConvertToTime"));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((t == NULL) || (tm == NULL)) {
        RvTmLogError((&logMgr->tmSource, "RvTmConvertToTime: NULL param(s)"));
        return RvTmErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    result = RV_OK;

    /* We have to adjust nanoseconds ourselves. */
    while(tm->nsec < 0) {
        tm->tm.tm_sec--;
        tm->nsec += RV_TIME_NSECPERSEC;
    }
    while(tm->nsec >= RV_TIME_NSECPERSEC) {
        tm->tm.tm_sec++;
        tm->nsec -= RV_TIME_NSECPERSEC;
    }

#if (RV_TM_TYPE == RV_TM_OSE)
    /* OSE's version of mktime errors if isdst != 0 so */
    /* We'll make it happy. */
    tmpdst = tm->tm.tm_isdst;
    tm->tm.tm_isdst = 0;
#endif

#if (RV_TM_TYPE == RV_TM_NUCLEUS) || (RV_TM_TYPE == RV_TM_WINCE) || \
    (RV_TM_TYPE == RV_TM_MOPI) || (RV_TM_TYPE == RV_TM_OSA)
    /* mktime not supported under WinCE, Nucleus, MOPI & OSA */
    RvTmLogError((&logMgr->tmSource, "RvTmConvertToTime: Not supported"));
    return RvTmErrorCode(RV_ERROR_NOTSUPPORTED);
#else
    /* Now send tm structure to mktime. */
    secs = mktime(&tm->tm);
#endif
    if(secs != (time_t)(-1)) {
        RvTimeSetSecs(t, (RvInt32)secs);
        RvTimeSetNsecs(t, tm->nsec);
    } else {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConvertToTime: mktime"));
    }

#if (RV_TM_TYPE == RV_TM_OSE)
    tm->tm.tm_isdst = tmpdst; /* Undo the OSE hack. */
#endif

    RvTmLogLeave((&logMgr->tmSource, "RvTmConvertToTime"));

    return result;
}


/********************************************************************************************
 * RvTmConstructUtc - Creates a calendar time object representing UTC time based on a standard time.
 *
 * note:    Some systems do not support time zones, thus UTC time may actually
 *          be local time, depending on how the clock was set.
 *
 * INPUT   : t      - pointer to time structure containing the time.
 *           logMgr - log manager instance.
 * OUTPUT  : tm     - pointer to a calendar time object to be constructed.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTmConstructUtc(
    IN  RvTime*     t,
    IN  RvLogMgr*   logMgr,
    OUT RvTm*       tm)
{
    RvStatus result;
    time_t secs;
#if (RV_TM_TYPE == RV_TM_WIN32) || ((RV_OS_TYPE == RV_OS_TYPE_HPUX) && (RV_OS_VERSION == RV_OS_HPUX_11)) || \
	((RV_TM_TYPE == RV_TM_MOPI) && defined(_WIN32))
    struct tm *tmresult;
#endif

    RvTmLogEnter((&logMgr->tmSource, "RvTmConstructUtc"));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((t == NULL) || (tm == NULL)) {
        RvTmLogError((&logMgr->tmSource, "RvTmConstructUtc: NULL param(s)"));
        return RvTmErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    result = RV_OK;
    secs = (time_t)RvTimeGetSecs(t);

#if (RV_TM_TYPE == RV_TM_POSIX) ||(RV_TM_TYPE == RV_TM_OSE)
#if (RV_OS_TYPE == RV_OS_TYPE_HPUX) && (RV_OS_VERSION == RV_OS_HPUX_11)
    tmresult = gmtime(&secs);
    if(tmresult != NULL) {
        memcpy(&tm->tm, tmresult, sizeof(tm->tm));
    } else {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConstructUtc: gmtime_r"));
    }
#else
    if(gmtime_r(&secs, &tm->tm) == NULL) {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConstructUtc: gmtime_r"));
    }
#endif	
#endif
#if (RV_TM_TYPE == RV_TM_WIN32) || \
	((RV_TM_TYPE == RV_TM_MOPI) && defined(_WIN32))
    tmresult = gmtime(&secs);
    if(tmresult != NULL) {
        memcpy(&tm->tm, tmresult, sizeof(tm->tm));
    } else {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConstructUtc: gmtime"));
    }
#endif
#if (RV_TM_TYPE == RV_TM_VXWORKS)
    if(gmtime_r(&secs, &tm->tm) != OK) {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConstructUtc: gmtime_r"));
    }
#endif
#if (RV_TM_TYPE == RV_TM_PSOS)
    /* In pSOS, gmtime is declared right but always returns NULL. */
    gmtime_r(&secs, &tm->tm);
#endif
#if (RV_TM_TYPE == RV_TM_NUCLEUS) || (RV_TM_TYPE == RV_TM_WINCE) || \
	((RV_TM_TYPE == RV_TM_MOPI) && !defined(_WIN32)) || (RV_TM_TYPE == RV_TM_OSA)
    /* Use our own gmtime since there's no standard one for Nucleus */
    if(RvGmtime_r(secs, &tm->tm) == NULL) {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConstructUtc: RvGmtime_r"));
    }
#endif

    if(result == RV_OK) {
        tm->tm.tm_isdst = 0; /* Some OS's forget to clear this */
        tm->nsec = RvTimeGetNsecs(t);
    }

    RvTmLogLeave((&logMgr->tmSource, "RvTmConstructUtc"));

    return result;
}


/********************************************************************************************
 * RvTmConstructLocal
 *
 * Creates a calendar time object representing local time based on a standard time.
 * note:    Some systems do not support time zones and local time will report the
 *          same time as UTC time.
 * note:    Some systems do not support daylight savings time properly and will not
 *          automatically adjust for it.
 *
 * INPUT   : t      - pointer to time structure containing the time.
 *           logMgr - log manager instance.
 * OUTPUT  : tm     - pointer to a calendar time object to be constructed.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTmConstructLocal(
    IN  RvTime*     t,
    IN  RvLogMgr*   logMgr,
    OUT RvTm*       tm)
{
    RvStatus result;
    time_t secs;
#if (RV_TM_TYPE == RV_TM_WIN32) || ((RV_OS_TYPE == RV_OS_TYPE_HPUX) && (RV_OS_VERSION == RV_OS_HPUX_11)) || \
	((RV_TM_TYPE == RV_TM_MOPI) && defined(_WIN32))
    struct tm *tmresult;
#endif

    RvTmLogEnter((&logMgr->tmSource, "RvTmConstructLocal"));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((t == NULL) || (tm == NULL)) {
        RvTmLogError((&logMgr->tmSource, "RvTmConstructLocal: NULL param(s)"));
        return RvTmErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    result = RV_OK;
    secs = (time_t)RvTimeGetSecs(t);

#if (RV_TM_TYPE == RV_TM_POSIX) || (RV_TM_TYPE == RV_TM_OSE) || \
    (RV_TM_TYPE == RV_TM_PSOS)
#if (RV_OS_TYPE == RV_OS_TYPE_HPUX) && (RV_OS_VERSION == RV_OS_HPUX_11)
    tmresult = localtime(&secs);
    if(tmresult != NULL) {
        memcpy(&tm->tm, tmresult, sizeof(tm->tm));
    } else {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConstructUtc: gmtime_r"));
    }
#else
    if(localtime_r(&secs, &tm->tm) == NULL) {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConstructLocal: localtime_r"));
    }
#endif
#endif
#if (RV_TM_TYPE == RV_TM_WIN32) || \
	((RV_TM_TYPE == RV_TM_MOPI) && defined(_WIN32))
    tmresult = localtime(&secs);
    if(tmresult != NULL) {
        memcpy(&tm->tm, tmresult, sizeof(tm->tm));
    } else {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConstructLocal: localtime"));
    }
#endif
#if (RV_TM_TYPE == RV_TM_VXWORKS)
    if(localtime_r(&secs, &tm->tm) != OK) {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConstructLocal: localtime_r"));
    }
#endif
#if (RV_TM_TYPE == RV_TM_NUCLEUS) || (RV_TM_TYPE == RV_TM_WINCE) || \
	((RV_TM_TYPE == RV_TM_MOPI) && !defined(_WIN32)) || (RV_TM_TYPE == RV_TM_OSA)
    /* Use our own localtime since there's no standard one for Nucleus */
    if(RvLocaltime_r(secs, &tm->tm) == NULL) {
        result = RvTmErrorCode(RV_ERROR_UNKNOWN);
        RvTmLogError((&logMgr->tmSource, "RvTmConstructLocal: RvLocaltime_r"));
    }
#endif

    if(result == RV_OK)
        tm->nsec = RvTimeGetNsecs(t);

    RvTmLogLeave((&logMgr->tmSource, "RvTmConstructLocal"));

    return result;
}


/********************************************************************************************
 * RvTmAsctime
 *
 * Creates a character string representation of calendar time. Equivalent to
 * ANSI asctime function.
 *
 * INPUT   : tm     - pointer to a calendar time object to create the string from.
 *           bufsize- size of buffer (must be at least size RV_TM_ASCTIME_BUFSIZE).
 *           logMgr - log manager instance.
 * OUTPUT  : buf    - pointer to buffer where string should be copied.
 * RETURN  : A pointer to the string buffer.
 */
RVCOREAPI
RvChar * RVCALLCONV RvTmAsctime(
    IN  RvTm*       tm,
    IN  RvSize_t    bufsize,
    IN  RvLogMgr*   logMgr,
    OUT RvChar*     buf)
{
    RvChar *result;

    RvTmLogEnter((&logMgr->tmSource, "RvTmAsctime"));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((tm == NULL) || (buf == NULL)) {
        RvTmLogError((&logMgr->tmSource, "RvTmAsctime: NULL param(s)"));
        return NULL;
    }
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(bufsize < RV_TM_ASCTIME_BUFSIZE) {
        RvTmLogError((&logMgr->tmSource, "RvTmAsctime: Buffer size too small"));
        return NULL;
    }
#endif

#if (RV_TM_TYPE == RV_TM_OSE)
    result = (RvChar *)asctime_r(&tm->tm, buf, bufsize);

#elif (RV_TM_TYPE == RV_TM_POSIX)

#if (RV_OS_TYPE == RV_OS_TYPE_HPUX) && (RV_OS_VERSION == RV_OS_HPUX_11)
    result = asctime(&tm->tm);
    if(result != NULL)
        result = strncpy(buf, result, bufsize);	
#else   
    RV_UNUSED_ARG(bufsize);
    result = (RvChar *)asctime_r(&tm->tm, buf);
#endif
#elif (RV_TM_TYPE == RV_TM_PSOS)
#if (RV_OS_VERSION == RV_OS_PSOS_2_0)
    result = (RvChar *)asctime_r(&tm->tm, buf, (int)bufsize);
#else
    result = (RvChar *)asctime_r(&tm->tm, buf);
#endif  /* RV_OS_VERSION == RV_OS_PSOS_2_0 */

#elif (RV_TM_TYPE == RV_TM_WIN32) || \
	  ((RV_TM_TYPE == RV_TM_MOPI) && defined(_WIN32))
    result = asctime(&tm->tm);
    if(result != NULL)
        result = strncpy(buf, result, bufsize);

#elif (RV_TM_TYPE == RV_TM_VXWORKS)
    if(asctime_r(&tm->tm, buf, &bufsize) > 0) {
        result = buf;
    } else result = NULL;

#elif (RV_TM_TYPE == RV_TM_NUCLEUS) || (RV_TM_TYPE == RV_TM_WINCE) || \
	  ((RV_TM_TYPE == RV_TM_MOPI) && !defined(_WIN32)) || (RV_TM_TYPE == RV_TM_OSA)
    result = RvAsctime_r(&tm->tm, buf); /* use our own since we can't count on libs */
#endif

    if (result == NULL)
    {
        RvTmLogError((&logMgr->tmSource, "RvTmAsctime: asctime"));
    }

    RvTmLogLeave((&logMgr->tmSource, "RvTmAsctime"));

    return result;
}



/********************************************************************************************
 * RvTmStrftime
 *
 * Creates a custom character string representation of calendar time. Equivalent to
 * ANSI strftime function.
 * note:    Use only ANSI standard formats in the format string to insure compatability
 *          across different systems.
 *
 * INPUT   : tm     - pointer to a calendar time object to create the string from.
 *           format - standard ANSI strftime formatting string.
 *           bufsize- size of result buffer
 *           logMgr - log manager instance.
 * OUTPUT  : result - pointer to buffer where the resulting string should be copied.
 * RETURN  : The size of the string that was generated (0 if there was a problem).
 */
RVCOREAPI RvSize_t RVCALLCONV RvTmStrftime(
    IN  RvTm*       tm,
    IN  RvChar*     format,
    IN  RvSize_t    maxsize,
    IN  RvLogMgr*   logMgr,
    OUT RvChar*     result)
{
    RvSize_t size;

    RvTmLogEnter((&logMgr->tmSource, "RvTmStrftime"));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((tm == NULL) || (format == NULL) || (result == NULL)) {
        RvTmLogError((&logMgr->tmSource, "RvTmStrftime: NULL param(s)"));
        return RvInt32Const(0);
    }
#endif

    size = strftime(result, maxsize, format, &tm->tm);

    RvTmLogLeave((&logMgr->tmSource, "RvTmStrftime"));

    return size;
}


/* Only use for Nucleus, WinCE & MOPI. other OS's have their own libs */
#if (RV_TM_TYPE == RV_TM_NUCLEUS) || (RV_OS_TYPE == RV_OS_TYPE_WINCE) || \
	((RV_TM_TYPE == RV_TM_MOPI) && !defined(_WIN32)) || (RV_TM_TYPE == RV_TM_OSA)

/**************************************************************************************/
/* This following part of this file contains primitive versions of gmtime_r */
/* and localtime_r (asctime_r is generic). These are for embedded OS's */
/* (ie Nucleus) that don't have these standard reentrant functions. Things */
/* like leap-seconds are not accounted for and no timezone work is done */
/* (localtime = gmtime). Eventually a full set of time functions should be */
/* included to handle it all properly for each OS. */

/* Parts based on the following public domain software: */
/* $FreeBSD: src/lib/libc/stdtime/asctime.c,v 1.8 2001/01/24 13:01:02 deischen Exp $ */
/* $FreeBSD: src/lib/libc/stdtime/localtime.c,v 1.27 2001/02/15 22:17:04 tegge Exp $ */
/* June 5, 1996 by Arthur David Olson (arthur_david_olson@nih.gov). */

#define SECSPERMIN  60
#define MINSPERHOUR 60
#define HOURSPERDAY 24
#define DAYSPERWEEK 7
#define DAYSPERNYEAR    365
#define DAYSPERLYEAR    366
#define SECSPERHOUR (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY  ((long) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR 12

#define TM_SUNDAY   0
#define TM_MONDAY   1
#define TM_TUESDAY  2
#define TM_WEDNESDAY    3
#define TM_THURSDAY 4
#define TM_FRIDAY   5
#define TM_SATURDAY 6

#define TM_JANUARY  0
#define TM_FEBRUARY 1
#define TM_MARCH    2
#define TM_APRIL    3
#define TM_MAY      4
#define TM_JUNE     5
#define TM_JULY     6
#define TM_AUGUST   7
#define TM_SEPTEMBER    8
#define TM_OCTOBER  9
#define TM_NOVEMBER 10
#define TM_DECEMBER 11

#define TM_YEAR_BASE    1900

#define EPOCH_YEAR  1970
#define EPOCH_WDAY  TM_THURSDAY

/*
** Accurate only for the past couple of centuries;
** that will probably do.
*/

#define isleap(_y) (((_y) % 4) == 0 && (((_y) % 100) != 0 || ((_y) % 400) == 0))

static const int mon_lengths[2][MONSPERYEAR] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const int year_lengths[2] = {
    DAYSPERNYEAR, DAYSPERLYEAR
};

/*
** A la X3J11, with core dump avoidance.
*/

/* result must be at least 26 characters long */
static char *RvAsctime_r(const struct tm *timeptr, char *result)
{
    static const char wday_name[][4] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const char mon_name[][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    const char *wn;
    const char *mn;
    int mday, hour, min, sec, year;

    if((timeptr->tm_wday < 0) || (timeptr->tm_wday >= DAYSPERWEEK)) {
        wn = "???";
    } else wn = wday_name[timeptr->tm_wday];
    if((timeptr->tm_mon < 0) || (timeptr->tm_mon >= MONSPERYEAR)) {
        mn = "???";
    } else mn = mon_name[timeptr->tm_mon];

    year = TM_YEAR_BASE + timeptr->tm_year;
    mday = timeptr->tm_mday;
    hour = timeptr->tm_hour;
    min = timeptr->tm_min;
    sec = timeptr->tm_sec;

    /* Add checks to prevent overruns */
    if((timeptr->tm_mday < 0) || (timeptr->tm_mday > 99))
        mday = 0;
    if((timeptr->tm_hour < 0) || (timeptr->tm_hour > 99))
        hour = 0;
    if((timeptr->tm_min < 0) || (timeptr->tm_min > 99))
        min = 0;
    if((timeptr->tm_sec < 0) || (timeptr->tm_sec > 99))
        sec = 0;
    if((year < 0) || (year > 9999))
        year = 0;

    /*
    ** The X3J11-suggested format is
    **  "%.3s %.3s%3d %02.2d:%02.2d:%02.2d %d\n"
    ** Since the .2 in 02.2d is ignored, we drop it.
    */
    RvSprintf(result, "%.3s %.3s%3d %02d:%02d:%02d %d\n", wn, mn,
            mday, hour, min, sec, year);
    return result;
}

/* Does not account for leap seconds or anything fancy. IEEE spec */
/* even forgot about leap seconds. So we do things the simple way */
/* for now */
static struct tm *RvGmtime_r(time_t t, struct tm *tm)
{
    RvUint year, month, leapyear;
	RvInt days;
    time_t temp;

    temp = t;
    tm->tm_sec = temp % SECSPERMIN;
    temp = temp/SECSPERMIN;
    tm->tm_min = temp % MINSPERHOUR;
    temp = temp/MINSPERHOUR;
    tm->tm_hour = temp % HOURSPERDAY;
    temp = temp / HOURSPERDAY;
    tm->tm_wday = (temp + EPOCH_WDAY) % DAYSPERWEEK;

    year = EPOCH_YEAR;
    for(;;) {
        leapyear = isleap(year);
        days = year_lengths[leapyear];
        if((RvInt)temp < days) break;
        temp -= days;
        year++;
    }
    tm->tm_year = year - TM_YEAR_BASE;
    tm->tm_yday = temp;

    month = TM_JANUARY;
    for(;;) {
        days = mon_lengths[leapyear][month];
        if((RvInt)temp < days) break;
        temp -= days;
        month++;
    }
    tm->tm_mon = month;
    tm->tm_mday = temp + 1;
    tm->tm_isdst = 0;
    return tm;
}

static struct tm *RvLocaltime_r(time_t t, struct tm *tm)
{
    /* To do local time, we should find out time zone offset */
    /* and subtract it from the gmtime result along with */
    /* any offset for daylight savings time if it is in effect */
    return RvGmtime_r(t, tm);
}

/*************************************************************************/
#endif /* Nucleus */ /* WinCE */ /* MOPI */



#if (RV_OS_TYPE == RV_OS_TYPE_WINCE && !defined(HAS_WCECOMPAT))

struct tm * __cdecl localtime(const time_t * ptime)
{
  SYSTEMTIME stime;
  static struct tm t;

    if (ptime);

    GetSystemTime(&stime);

    t.tm_year = stime.wYear;
    t.tm_mon = stime.wMonth;
    t.tm_wday = stime.wDayOfWeek;
    t.tm_mday = stime.wDay;
    t.tm_hour = stime.wHour;
    t.tm_min = stime.wMinute;
    t.tm_sec = stime.wSecond;
    t.tm_yday = 0;
    t.tm_isdst = 0;

    return &t;
}
#endif


#if (RV_OS_TYPE == RV_OS_TYPE_WINCE) || ((RV_OS_TYPE == RV_OS_TYPE_MOPI) && !defined(_WIN32)) || (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)

/* =========================================== */
/* =========================================== */
/* Effi */
/* Implementation of strftime(). */
/* found on the internet, needs checking */

static char *aday[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static char *day[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

static char *amonth[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static char *month[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static char *tzname_[2] = {"CST", "CDT"};        /* Add your own defaults here */

static char buf[26];

static void strfmt(char *str, const char *fmt, ...);

/**
 *
 * size_t strftime(char *str,
 *                  size_t maxs,
 *                  const char *fmt,
 *                  const struct tm *t)
 *
 *      this functions acts much like a sprintf for time/date output.
 *      given a pointer to an output buffer, a format string and a
 *      time, it copies the time to the output buffer formatted in
 *      accordance with the format string.  the parameters are used
 *      as follows:
 *
 *          str is a pointer to the output buffer, there should
 *          be at least maxs characters available at the address
 *          pointed to by str.
 *
 *          maxs is the maximum number of characters to be copied
 *          into the output buffer, included the '\0' terminator
 *
 *          fmt is the format string.  a percent sign (%) is used
 *          to indicate that the following character is a special
 *          format character.  the following are valid format
 *          characters:
 *
 *              %A      full weekday name (Monday)
 *              %a      abbreviated weekday name (Mon)
 *              %B      full month name (January)
 *              %b      abbreviated month name (Jan)
 *              %c      standard date and time representation
 *              %d      day-of-month (01-31)
 *              %H      hour (24 hour clock) (00-23)
 *              %I      hour (12 hour clock) (01-12)
 *              %j      day-of-year (001-366)
 *              %M      minute (00-59)
 *              %m      month (01-12)
 *              %p      local equivalent of AM or PM
 *              %S      second (00-59)
 *              %U      week-of-year, first day sunday (00-53)
 *              %W      week-of-year, first day monday (00-53)
 *              %w      weekday (0-6, sunday is 0)
 *              %X      standard time representation
 *              %x      standard date representation
 *              %Y      year with century
 *              %y      year without century (00-99)
 *              %Z      timezone name
 *              %%      percent sign
 *
 *      the standard date string is equivalent to:
 *
 *          %a %b %d %Y
 *
 *      the standard time string is equivalent to:
 *
 *          %H:%M:%S
 *
 *      the standard date and time string is equivalent to:
 *
 *          %a %b %d %H:%M:%S %Y
 *
 *      strftime() returns the number of characters placed in the
 *      buffer, not including the terminating \0, or zero if more
 *      than maxs characters were produced.
 *
**/

size_t strftime(char *s, size_t maxs, const char *f, const struct tm *t)
{
      int w;
      char *p, *q, *r;

      p = s;
      q = s + maxs - 1;
      while ((*f != '\0'))
      {
            if (*f++ == '%')
            {
                  r = buf;
                  switch (*f++)
                  {
                  case '%' :
                        r = "%";
                        break;

                  case 'a' :
                        r = aday[t->tm_wday];
                        break;

                  case 'A' :
                        r = day[t->tm_wday];
                        break;

                  case 'b' :
                        r = amonth[t->tm_mon];
                        break;

                  case 'B' :
                        r = month[t->tm_mon];
                        break;

                  case 'c' :
                        strfmt(r, "%0 %0 %2 %2:%2:%2 %4",
                              aday[t->tm_wday], amonth[t->tm_mon],
                              t->tm_mday,t->tm_hour, t->tm_min,
                              t->tm_sec, t->tm_year+1900);
                        break;

                  case 'd' :
                        strfmt(r,"%2",t->tm_mday);
                        break;

                  case 'H' :
                        strfmt(r,"%2",t->tm_hour);
                        break;

                  case 'I' :
                        strfmt(r,"%2",(t->tm_hour%12)?t->tm_hour%12:12);
                        break;

                  case 'j' :
                        strfmt(r,"%3",t->tm_yday+1);
                        break;

                  case 'm' :
                        strfmt(r,"%2",t->tm_mon+1);
                        break;

                  case 'M' :
                        strfmt(r,"%2",t->tm_min);
                        break;

                  case 'p' :
                        r = (t->tm_hour>11)?"PM":"AM";
                        break;

                  case 'S' :
                        strfmt(r,"%2",t->tm_sec);
                        break;

                  case 'U' :
                        w = t->tm_yday/7;
                        if (t->tm_yday%7 > t->tm_wday)
                              w++;
                        strfmt(r, "%2", w);
                        break;

                  case 'W' :
                        w = t->tm_yday/7;
                        if (t->tm_yday%7 > (t->tm_wday+6)%7)
                              w++;
                        strfmt(r, "%2", w);
                        break;

                  case 'w' :
                        strfmt(r,"%1",t->tm_wday);
                        break;

                  case 'x' :
                        strfmt(r, "%3s %3s %2 %4", aday[t->tm_wday],
                              amonth[t->tm_mon], t->tm_mday, t->tm_year+1900);
                        break;

                  case 'X' :
                        strfmt(r, "%2:%2:%2", t->tm_hour,
                              t->tm_min, t->tm_sec);
                        break;

                  case 'y' :
                        strfmt(r,"%2",t->tm_year%100);
                        break;

                  case 'Y' :
                        strfmt(r,"%4",t->tm_year+1900);
                        break;

                  case 'Z' :
                        r = (t->tm_isdst && tzname_[1][0]) ?
                              tzname_[1] : tzname_[0];
                        break;

                  default:
                        buf[0] = '%';     /* reconstruct the format */
                        buf[1] = f[-1];
                        buf[2] = '\0';
                        if (buf[1] == 0)
                              f--;        /* back up if at end of string */
                  }
                  while (*r)
                  {
                        if (p == q)
                        {
                              *q = '\0';
                              return 0;
                        }
                        *p++ = *r++;
                  }
            }
            else
            {
                  if (p == q)
                  {
                        *q = '\0';
                        return 0;
                  }
                  *p++ = f[-1];
            }
      }
      *p = '\0';
      return p - s;
}


static int power[5] = { 1, 10, 100, 1000, 10000 };

/**
 * static void strfmt(char *str, char *fmt);
 *
 * simple sprintf for strftime
 *
 * each format descriptor is of the form %n
 * where n goes from zero to four
 *
 * 0    -- string %s
 * 1..4 -- int %?.?d
 *
**/

static void strfmt(char *str, const char *fmt, ...)
{
      int ival, ilen;
      char *sval;
      va_list vp;

      va_start(vp, fmt);
      while (*fmt)
      {
            if (*fmt++ == '%')
            {
                  ilen = *fmt++ - '0';
                  if (ilen == 0)                /* zero means string arg */
                  {
                        sval = va_arg(vp, char*);
                        while (*sval)
                              *str++ = *sval++;
                  }
                  else                          /* always leading zeros */
                  {
                        ival = va_arg(vp, int);
                        while (ilen)
                        {
                              ival %= power[ilen--];
                              *str++ = (char)('0' + ival / power[ilen]);
                        }
                  }
            }
            else  *str++ = fmt[-1];
      }
      *str = '\0';
      va_end(vp);
}


#endif /* WINCE */ /* MOPI */
