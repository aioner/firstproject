/***********************************************************************
Filename   : rvtime.c
Description: basic time structure definition and manipulation
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
#include "rvstdio.h"
#include "rvtime.h"

/* Maximum and minimum values for nsec field depending on sign of sec field. */
#define RV_TIME_NSEC_GETINDEX(_sec) ((_sec) > RvInt32Const(0) ? 2 : ((_sec) == RvInt32Const(0) ? 1 : 0))
#define RV_TIME_MAXNSEC (RV_TIME_NSECPERSEC - RvInt32Const(1))
#define RV_TIME_MINNSEC (RV_TIME_MAXNSEC * RvInt32Const(-1))
/*                               sec is               <0,            == 0,              >0   */
static const RvInt32 RvTimeNsecMax[3] = {              0, RV_TIME_MAXNSEC, RV_TIME_MAXNSEC};
static const RvInt32 RvTimeNsecMin[3] = {RV_TIME_MINNSEC, RV_TIME_MINNSEC,               0};

static void RvTimeAdjustNsec(RvTime *t);
static RvBool RvTimeCheckNsec(RvInt32 sec, RvInt32 nsec);

/********************************************************************************************
 * RvTimeConstruct
 * Constructs a time.
 * INPUT   : secs     - Initial value for seconds.
 *           nanosecs - Initial value for nanoseconds. Valid range for this field depends
 *                      on the sign of 'secs':
 *                      secs > 0 : 0..999999999 
 *                      secs == 0: -999999999..999999999
 *                      secs < 0 : -999999999..0
 * OUTPUT  : t - Pointer to time structure to construct.
 * RETURN  : A pointer to the time structure or, if there is an error, NULL.
 */
RVCOREAPI
RvTime * RVCALLCONV RvTimeConstruct(
    IN RvInt32 secs,
    IN RvInt32 nanosecs,
    OUT RvTime *t)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(t == NULL) return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
	if(RvTimeCheckNsec(secs, nanosecs) == RV_FALSE) return NULL;
#endif

    t->sec = secs;
    t->nsec = nanosecs;
    return t;
}

/********************************************************************************************
 * RvTimeConstructFrom64
 * Constructs a time from a 64 bit number of nanoseconds.
 * INPUT   : nanosecs - Initial time value in nanoseconds.
 * OUTPUT  : t - Pointer to time structure to construct.
 * RETURN  : A pointer to the time structure or, if there is an error, NULL.
 */
RVCOREAPI
RvTime * RVCALLCONV RvTimeConstructFrom64(
    IN RvInt64 nanosecs,
    OUT RvTime *t)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(t == NULL) return NULL;
#endif

	t->sec = RvInt64ToRvInt32(RvInt64Div(nanosecs, RV_TIME64_NSECPERSEC));
	if(RvInt64IsLessThan(nanosecs, RvInt64Const(0,0,0))) {
		nanosecs = RvUint64Mul(nanosecs, RvInt64Const(-1,0,1));
		t->nsec = RvInt64ToRvInt32(RvInt64Mod(nanosecs, RV_TIME64_NSECPERSEC)) * RvInt32Const(-1);
	} else {
		t->nsec = RvInt64ToRvInt32(RvInt64Mod(nanosecs, RV_TIME64_NSECPERSEC));
	}
    return t;
}

/********************************************************************************************
 * RvTimeSubtract
 * Subracts two time values (result = newtime - oldtime).
 * INPUT   : newtime - Pointer to time structure to subract from.
 *           oldtime - Pointer to time structure with value to subract.
 * OUTPUT  : result - Pointer to time structure where result will be stored.
 * RETURN  : A pointer to the time structure or, if there is an error, NULL.
 */
RVCOREAPI
RvTime * RVCALLCONV RvTimeSubtract(
    IN const RvTime *newtime,
    IN const RvTime *oldtime,
    OUT RvTime *result)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((result == NULL) || (newtime == NULL) || (oldtime == NULL)) return NULL;
#endif

    result->sec = newtime->sec - oldtime->sec;
    result->nsec = newtime->nsec - oldtime->nsec;
	RvTimeAdjustNsec(result);
    return result;
}

/********************************************************************************************
 * RvTimeAdd
 * Adds two time values (result = time1 + time2).
 * INPUT   : time1 - Pointer to first time.
 *           time2 - Pointer to second time structure.
 * OUTPUT  : result - Pointer to time structure where result will be stored.
 * RETURN  : A pointer to the time structure or, if there is an error, NULL.
 * NOTE    : Absolute time values wrap in the year 2038.
 */
RVCOREAPI
RvTime * RVCALLCONV RvTimeAdd(
    IN const RvTime *time1,
    IN const RvTime *time2,
    OUT RvTime *result)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((result == NULL) || (time1 == NULL) || (time2 == NULL)) return NULL;
#endif

    result->sec = time1->sec + time2->sec;
	result->nsec = time1->nsec + time2->nsec;
	RvTimeAdjustNsec(result);
    return result;
}

/********************************************************************************************
 * RvTimeConvertTo64
 * Converts a time value to a 64 bit value in nanoseconds.
 * INPUT   : t - Pointer to time structure to be converted.
 * OUTPUT  : None.
 * RETURN  : A 64 bit nanoseconds value.
 */
RVCOREAPI
RvInt64 RVCALLCONV RvTimeConvertTo64(
    IN const RvTime *t)
{
    RvInt64 result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(t == NULL) return RvInt64Const(0,0,0);
#endif

    result = RvInt64Add( RvInt64Mul(RvInt64FromRvInt32(t->sec), RV_TIME64_NSECPERSEC),
                         RvInt64FromRvInt32(t->nsec));
    return result;
}

/* After a mathematical operation, nsec can be out of its valid range */
/* so adjust it to be valid. */
static void RvTimeAdjustNsec(RvTime *t)
{
	RvInt i;

	/* Valid range depends on sign of sec */
	i = RV_TIME_NSEC_GETINDEX(t->sec);

	/* adjust to appropriate max and min */
	if(t->nsec > RvTimeNsecMax[i]) {
		t->nsec -= RV_TIME_NSECPERSEC;
		t->sec += RvInt32Const(1);
	} else {
		if(t->nsec < RvTimeNsecMin[i]) {
			t->nsec += RV_TIME_NSECPERSEC;
			t->sec -= RvInt32Const(1);
		}
	}
}

/* Check to see if nsec is in a valud range */
static RvBool RvTimeCheckNsec(RvInt32 sec, RvInt32 nsec)
{
	RvInt i;

	/* Valid range depends on sign of sec */
	i = RV_TIME_NSEC_GETINDEX(sec);

	if((nsec > RvTimeNsecMax[i]) || (nsec < RvTimeNsecMin[i]))
		return RV_FALSE;
	return RV_TRUE;
}
