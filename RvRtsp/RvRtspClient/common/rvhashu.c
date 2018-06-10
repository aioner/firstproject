#if (0)
******************************************************************************
Filename:
Description:
******************************************************************************
                Copyright (c) 2000 RADVision Inc.
************************************************************************
NOTICE:
This document contains information that is proprietary to RADVision LTD.
No part of this publication may be reproduced in any form whatsoever 
without written prior approval by RADVision LTD.

RADVision LTD. reserves the right to revise this publication and make 
changes without obligation to notify any person of such revisions or 
changes.
******************************************************************************
$Revision:$
$Date:$
$Author: S. Cipolli$
******************************************************************************
#endif

#include "rvhashu.h"

RvSize_t RvHashNextPrime(size_t n) {
	static const RvSize_t primes[] = {
	  17,			53,         97,         193,       389,	
	  769,			1543,       3079,       6151,      12289,
	  24593,		49157,      98317,      196613,    393241,
	  786433,		1572869,    3145739,    6291469,   12582917,
	  25165843,		50331653,   100663319,  201326611, 402653189,
	  805306457,	1610612741, 3221225473U,	4294967291U
	};
	static const size_t numPrimes = sizeof(primes) / sizeof(size_t);
	RvSize_t i;
	
	for (i = 0; i < numPrimes; ++i)
		if (n < primes[i])
			return primes[i];
	return primes[numPrimes - 1];
}
