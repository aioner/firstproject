/*****************************************************************************
Filename    : rvntptime.c
Description : NTP time structure definition and manipulation
************************************************************************
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
#include "rvntptime.h"

#if (RV_NTPTIME_TYPE == RV_YES)

/* difference between NTP epoc (Jan 1, 1900) and time epoch (Jan 1 1970) in seconds */
#define NTPDELTA RvUint32Const(2208988800)



/********************************************************************************************
 * RvNtpTimeConstruct
 * Constructs an NTP time.
 * INPUT   : secs     - Initial value for seconds.
 *           fraction - Initial fraction value.
 * OUTPUT  : ntime    - Pointer to NTP time structure to construct.
 * RETURN  : A pointer to the NTP time structure or, if there is an error, NULL.
 */
RVCOREAPI
RvNtpTime * RVCALLCONV RvNtpTimeConstruct(
    IN RvUint32 secs,
    IN RvUint32 fraction,
    OUT RvNtpTime *ntime)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(ntime == NULL) return NULL;
#endif

    ntime->secs = secs;
    ntime->fraction = fraction;
    return ntime;
}


/********************************************************************************************
 * RvNtpTimeConstructFromTime
 * Constructs an NTP time from a standard time value. It handles both
 * relative time and absolute (actual clock) times. If timetype is set
 * to RV_NTPTIME_RELATIVE then the time will simply be converted from
 * seconds and nanoseconds to seconds and fraction of a second. If the
 * timetype is set to RV_NTPTIME_ABSOLUTE, then the time will be
 * considered to be seconds and nanoseconds since January 1, 1970 and
 * will be convert to an NTP time of seconds and fraction of a second
 * since January 1, 1900.
 * INPUT   : t        - Time to convert from.
 *           timetype - Type of time to convert: RV_NTPTIME_RELATIVE or RV_NTPTIME_ABSOLUTE.
 * OUTPUT  : ntime    - Pointer to NTP time structure to construct.
 * RETURN  : A pointer to the NTP time structure or, if there is an error, NULL.
 * NOTE    : For ABSOLUTE times, not all valid values of time can be represented
 *           in the the NTP time format. Dates in 2036 and later can not be
 *           converted.
 *           The conversion between nanoseconds and fraction of a second may
 *           incur a small amount of round off error.
 *           Construct an NTP time from a RvTime time. The two formats do not
 *           have the exact same resolution so there can be some round-off
 *           error. The timetype parameter indicates wether absolute or relative
 *           time is stored in time. Absolute times are calander times that need
 *           to have the difference in epochs dealt with. Note that time passed
 *           in can be negative since NTP time epoch is earlier.
 */
RVCOREAPI
RvNtpTime * RVCALLCONV RvNtpTimeConstructFromTime(
    IN const RvTime *t,
    IN RvBool timetype,
    OUT RvNtpTime *ntime)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((ntime == NULL) || (t == NULL)) return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((timetype != RV_NTPTIME_ABSOLUTE) && (timetype != RV_NTPTIME_RELATIVE))
        return NULL;
#endif

    ntime->secs = (RvUint32)RvTimeGetSecs(t);
    if(timetype == RV_NTPTIME_ABSOLUTE)
        ntime->secs += NTPDELTA;
    ntime->fraction = RvNtpTimeNsecsToFraction(RvTimeGetNsecs(t));
    return ntime;
}


/********************************************************************************************
 * RvNtpTimeConstructFrom64
 * Constructs an NTP time from a 64 bit NTP time representation.
 * Obviously the caller needs to indicate which bit number the decimal point is to
 * the left of (thus 0 = no decimal, 64 = no whole number portion).
 * INPUT   : ntime   - 64 bit representation of an NTP time.
 *           decimal - Number of bits in ntime which is the fraction part of the number (0 = no fraction, 64 = no seconds).
 * OUTPUT  : result  - Pointer to NTP time structure to construct.
 * RETURN  : A pointer to the NTP time structure or, if there is an error, NULL.
 */
RVCOREAPI
RvNtpTime * RVCALLCONV RvNtpTimeConstructFrom64(
    IN RvUint64 ntime,
    IN RvUint8 decimal,
    OUT RvNtpTime *result)
{
    RvUint64 temp;
    RvInt shiftnum;


#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(result == NULL) return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if (decimal > RvUint8Const(64)) return NULL;
#endif

    /* Shift decimal place to get a 32.32 number */
    temp = ntime;
    if(decimal != RvUint8Const(32)) {
        if(decimal < RvUint8Const(32)) {
            temp = RvUint64ShiftLeft(temp, RvUint8Const(32) - decimal);
        } else temp = RvUint64ShiftRight(temp, decimal - RvUint8Const(32));
    }

    /* Bottom 32 bits are the fraction */
    result->fraction = RvUint64ToRvUint32(temp);

    /* Get the top 32 bits for the seconds */
    shiftnum = 32; /* compiler workaround */
    result->secs = RvUint64ToRvUint32(RvUint64ShiftRight(temp, shiftnum));

    return result;
}


/********************************************************************************************
 * RvNtpTimeChop
 * Chops bits off of NTP times. Some application require smaller NTP numbers
 * that 32 bits of seconds and a 32 bit fraction. This fucntion will chop
 * off unneeded bits by zeroing them out.
 * INPUT   : ntime    - Pointer to NTP time structure to chop.
 *           secbits  - Number of bits of the seconds value to keep (0 - 32).
 *           fracbits - Number of bits of the fraction to keep (0 - 32).
 * OUTPUT  : result   - Pointer to NTP time structure to construct.
 * RETURN  : A pointer to the NTP time structure or, if there is an error, NULL.
 */
RVCOREAPI
RvNtpTime * RVCALLCONV RvNtpTimeChop(
    IN const RvNtpTime *ntime,
    IN RvUint8 secbits,
    IN RvUint8 fracbits,
    OUT RvNtpTime *result)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((result == NULL) || (ntime == NULL)) return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if ((secbits + fracbits) > RvUint8Const(64)) return NULL;
#endif

    if(secbits < RvUint8Const(32)) {
        result->secs = ntime->secs & (~(RV_UINT32_MAX << secbits));
    } else result->secs = ntime->secs;
    if(fracbits < RvUint8Const(32)) {
        result->fraction = ntime->fraction & (~(RV_UINT32_MAX >> fracbits));
    } else result->fraction = ntime->fraction;
    return result;
}


/********************************************************************************************
 * RvNtpTimeSubtract
 * Subtracts two NTP time values (result = newtime - oldtime).
 * NTP time starts at 1900 and can not go lower than that. Note that newtime MUST
 * be larger than oldtime since NTP time values are unsigned.
 * INPUT   : newtime - Pointer to NTP time structure to subract from.
 *           oldtime - Pointer to NTP time structure with value to subract.
 * OUTPUT  : result  - Pointer to NTP time structure to construct.
 * RETURN  : A pointer to the NTP time structure or, if there is an error, NULL.
 * NOTE    : Since NTP time values are unsigned, newtime must be larger than oldtime and
 *           the earliest real time that can be represented is January 1, 1900.
 */
RVCOREAPI
RvNtpTime * RVCALLCONV RvNtpTimeSubtract(
    IN const RvNtpTime *newtime,
    IN const RvNtpTime *oldtime,
    OUT RvNtpTime *result)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((result == NULL) || (newtime == NULL) || (oldtime == NULL)) return NULL;
#endif

    result->secs = newtime->secs - oldtime->secs;
    result->fraction = newtime->fraction - oldtime->fraction;
    if(result->fraction > newtime->fraction)
        result->secs -= RvUint32Const(1);
    return result;
}


/********************************************************************************************
 * RvNtpTimeAdd
 * Adds two NTP time values (result = time1 + time2).
 * INPUT   : newtime - Pointer to NTP time structure to subract from.
 *           oldtime - Pointer to NTP time structure with value to subract.
 * OUTPUT  : result  - Pointer to NTP time structure to construct.
 * RETURN  : A pointer to the NTP time structure or, if there is an error, NULL.
 * NOTE    : Absolute NTP time values wrap in the year 2036.
 */
RVCOREAPI
RvNtpTime * RVCALLCONV RvNtpTimeAdd(
    IN const RvNtpTime *newtime,
    IN const RvNtpTime *oldtime,
    OUT RvNtpTime *result)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((result == NULL) || (newtime == NULL) || (oldtime == NULL)) return NULL;
#endif

    result->secs = newtime->secs + oldtime->secs;
    result->fraction = newtime->fraction + oldtime->fraction;
    if(result->fraction < newtime->fraction)
        result->secs += RvUint32Const(1);
    return result;
}


/********************************************************************************************
 * RvNtpTimeToTime
 * Converts NTP time to a standard time value.
 * The two formats do not have the exact same resolution so there can be some
 * round-off error. The timetype parameter indicates wether absolute or relative time
 * is stored in the NTP time. Absolute times are calander times that need to have the
 * difference in epochs dealt with. The resulting time can be negative if
 * the NTP time is earlier than the standard time epoch.
 * It handles both relative time and absolute (actual clock) times. If timetype is set
 * to RV_NTPTIME_RELATIVE then the NTP time will simply be converted
 * from seconds and fraction of a second to seconds and nanoseconds. If
 * the timetype is set to RV_NTPTIME_ABSOLUTE, then the NTP time will be
 * considered to be seconds fraction of a second since January 1, 1900
 * and will be coverted to seconds and nanoseconds since January 1, 1970.
 * OUTPUT  : t - Pointer to time to store the result.
 *           timetype - Type of time to convert: RV_NTPTIME_RELATIVE or RV_NTPTIME_ABSOLUTE.
 * INPUT   :  ntime - Pointer to NTP time structure to convert from.
 * RETURN  : A pointer to the time structure (t) or, if there is an error, NULL.
 * NOTE    : For ABSOLUTE times, not all valid values of NTP can be represented
 *           in the standard time format. Dates in 1902 and earlier can not be
 *           converted.
 *           The conversion between fraction of a seconds and nanoseconds may
 *           incur a small amount of round off error.
 */
RVCOREAPI
RvTime * RVCALLCONV RvNtpTimeToTime(
    OUT RvTime *t,
    IN RvBool timetype,
    IN const RvNtpTime *ntime)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((ntime == NULL) || (t == NULL)) return NULL;
#endif

    if(timetype == RV_NTPTIME_ABSOLUTE) {
        RvTimeSetSecs(t, (RvInt32)(ntime->secs - NTPDELTA));
    } else RvTimeSetSecs(t, (RvInt32)(ntime->secs));
    RvTimeSetNsecs(t, RvNtpTimeFractionToNsecs(ntime->fraction));
    return t;
}


/********************************************************************************************
 * RvNtpTimeTo64
 * Converts an NTP time to a 64 bit representation. The number of bits to use for
 * seconds and the fraction may be set. For example, setting secbits to 32 and
 * fracbits to 32 will return a full 64 bit representation of the NTP time. Or,
 * setting secbits to 4 and fracbits to 12 will return a 64 bit number where
 * the low order 12 bits are the fraction, followed by 4 bits of the seconds,
 * and the high order 32 bits simply set to 0.
 * INPUT   : ntime - Pointer to NTP time structure to convert.
 *           secbits - Number of bits of the seconds value to include (0 to 64).
 *           fracbits - Number of bits of the fraction to include (0 to 64).
 * OUTPUT  : None.
 * RETURN  : The 64 bit represention of the NTP value.
 * NOTE    : To allow for easy conversion to various bit sizes, the value may be chopped (see
 *           RvNtpChop). All resulting bits are placed into the low order bits of the 64 bit
 *           result (so the high order bits can be thrown out). The decimal place of the
 *           resulting number will be to the left of the number of fraction bits requested.
 *           Setting secbits and fracbits to 32 will not chop the number and put the decimal
 *           place to the left of the 32nd bit.
 */
RVCOREAPI
RvUint64 RVCALLCONV RvNtpTimeTo64(
    IN const RvNtpTime *ntime,
    IN RvUint8 secbits,
    IN RvUint8 fracbits)
{
    RvUint64 result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(ntime == NULL) return RvUint64Const(0, 0);
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if ((secbits + fracbits) > RvUint8Const(64)) return RvUint64Const(0, 0);
#endif

    /* Get basic 32.32 result first */
    result = RvUint64Or(RvUint64FromRvUint32(ntime->fraction), RvUint64ShiftLeft(RvUint64FromRvUint32(ntime->secs), 32));

    /* Chop any excess seconds off */
    if(secbits < RvUint8Const(32))
        result = RvUint64And(result, RvUint64ShiftRight(RV_UINT64_MAX, (RvUint8Const(32) - secbits)));

    /* Shift everything to where the decimal should go */
    if(fracbits != RvUint8Const(32)) {
        if(fracbits < RvUint8Const(32)) {
            result = RvUint64ShiftRight(result, RvUint8Const(32) - fracbits);
        } else result = RvUint64ShiftLeft(result, fracbits - RvUint8Const(32));
    }

    return result;
}


/********************************************************************************************
 * RvNtpTimeNsecsToFraction
 * Converts a value in nanoseconds to an NTP fraction. The nanoseconds value
 * must be less than one second.
 * INPUT   : nsecs - Nanoseconds (0 to 999999999).
 * OUTPUT  : None.
 * RETURN  : 32 bit NTP fraction.
 * NOTE    : The conversion between nanoseconds and fraction of a second may
 *           incur a small amount of round off error.
 */
RVCOREAPI
RvUint32 RVCALLCONV RvNtpTimeNsecsToFraction(
    IN RvInt32 nsecs)
{
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((nsecs < RvInt32Const(0)) || (nsecs >= RV_TIME_NSECPERSEC))
       return RvUint32Const(0);
#endif

    /* Uses 64 bit math to maintain resolution */
    return RvUint64ToRvUint32(RvUint64Div(RvUint64Mul(RvUint64FromRvInt32(nsecs), 
        RvUint64Const(1, 0)), RV_TIME64_NSECPERSEC));
}


/********************************************************************************************
 * RvNtpTimeFractionToNsecs
 * Converts an NTP fraction to nanoseconds.
 * INPUT   : fraction - NTP fraction.
 * OUTPUT  : None.
 * RETURN  : Nanoseconds (0 to 999999999).
 * NOTE    : The conversion between the NTP fraction of a second and
 *           nanoseconds may incur a small amount of round off error.
 */
RVCOREAPI
RvInt32 RVCALLCONV RvNtpTimeFractionToNsecs(
    IN RvUint32 fraction)
{
    /* Uses 64 bit math to maintain resolution */
    return RvUint64ToRvInt32(RvUint64Div(RvUint64Mul(RvUint64FromRvUint32(fraction), RV_TIME64_NSECPERSEC), RvUint64Const(1, 0)));
}

#endif /* (RV_NTPTIME_TYPE == RV_YES) */
