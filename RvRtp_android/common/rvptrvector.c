#if (0)
******************************************************************************
Filename    :
Description :
******************************************************************************
                Copyright (c) 1999 RADVision Inc.
************************************************************************
NOTICE:
This document contains information that is proprietary to RADVision LTD.
No part of this publication may be reproduced in any form whatsoever 
without written prior approval by RADVision LTD..

RADVision LTD. reserves the right to revise this publication and make 
changes without obligation to notify any person of such revisions or 
changes.
******************************************************************************
$Revision:$
$Date:$
$Author: S. Cipolli$
******************************************************************************
#endif

#include "rvccore.h"
#include "rvptrvector.h"

rvDefineVector(RvVoidPtr)

RvVoidPtr rvPtrVectorFront(RvPtrVector* l) {
	return *rvVectorFront(l);
}
RvVoidPtr rvPtrVectorBack(RvPtrVector* l) {
	return *rvVectorBack(l);
}
RvPtrVectorIter rvPtrVectorPushBack(RvPtrVector* l, RvVoidPtr x)	{	
	RvVoidPtr* rx = &x;
	return rvVectorPushBack(RvVoidPtr)(l, rx);
}
RvPtrVectorIter rvPtrVectorInsert(RvPtrVector* l, RvPtrVectorIter i, RvVoidPtr x) {	
	RvVoidPtr* rx = &x;
	return rvVectorInsert(RvVoidPtr)(l, i, rx);
}
