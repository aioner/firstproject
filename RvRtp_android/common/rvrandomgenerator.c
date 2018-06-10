/***********************************************************************
Filename   : rvrandomgenerator.c
Description: Random number generator
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
#include "rvrandomgenerator.h"

/********************************************************************************************
 * RvRandomGeneratorConstruct
 * construct a random generator object.
 * INPUT   : r - a random generator object.
 *           seed - random generator seed.
 * OUTPUT  : None
 * RETURN  : RV_OK.
 */
RVCOREAPI RvStatus RVCALLCONV
RvRandomGeneratorConstruct(
    IN RvRandomGenerator* r,
    IN RvRandom seed)
{
    r->state = seed;
    return RV_OK;
}


/********************************************************************************************
 * RvRandomGeneratorGetValue
 * returns a random value.
 * INPUT   : r - a random generator object.
 * OUTPUT  : value - the random value.
 * RETURN  : RV_OK.
 */
RVCOREAPI RvStatus RVCALLCONV
RvRandomGeneratorGetValue(IN RvRandomGenerator* r, OUT RvRandom* value)
{
    /* Thread Checker: don't lock r->state, it is random after all :) */
    r->state = (r->state * 1103515245 + 12345) & RV_RAND_MAX;
    *value = r->state ;

    return RV_OK;
}


/********************************************************************************************
 * RvRandomGeneratorGetInRange
 * returns a random value.
 * INPUT   : r - a random generator object.
 * INPUT   : n - maximal random value.
 * OUTPUT  : value - the random value in range [0;n-1].
 * RETURN  : RV_OK/RV_ERROR_BADPARAM - if value==NULL
 */
RVCOREAPI RvStatus RVCALLCONV
RvRandomGeneratorGetInRange(RvRandomGenerator* r, RvRandom n, OUT RvRandom* value)
{
	RvRandom TempValue;

	if (NULL==value)
		return RV_ERROR_BADPARAM;	
	RvRandomGeneratorGetValue(r, &TempValue);

	*value = (TempValue == RV_RAND_MAX) ? n-1 :
	   (RvRandom)RvUint64ToRvUint32(
		   RvUint64Div(RvUint64Mul(RvUint64FromRvUint32(TempValue), 
								   RvUint64FromRvUint32(n)), 
		               RvUint64FromRvUint32(RV_RAND_MAX)));

	return RV_OK;
}
