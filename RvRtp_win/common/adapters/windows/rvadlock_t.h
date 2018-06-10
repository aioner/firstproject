/* rvadlock_t.h - rvadlock_t header file */
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

#ifndef RV_ADLOCK_T_H
#define RV_ADLOCK_T_H


#include "rvtypes.h"
#include <windows.h>

#if (RV_LOCK_TYPE == RV_LOCK_WIN32_MUTEX)

typedef HANDLE RvLock;

typedef int RvLockAttr; /* not used */

#elif (RV_LOCK_TYPE == RV_LOCK_WIN32_CRITICAL)

typedef struct
{
#if defined(RV_LOCK_WIN32_DEBUG)
    int                 isLocked;
#endif
    CRITICAL_SECTION    lock;
} RvLock;

typedef DWORD RvLockAttr; /* spin count (use only on Win2000 and newer) */

#endif


#endif
