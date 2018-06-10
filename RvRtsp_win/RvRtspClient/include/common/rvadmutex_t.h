/* rvadmutex_t.h - rvadmutex_t header file */
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

#ifndef RV_ADMUTEX_T_H
#define RV_ADMUTEX_T_H


#include "rvtypes.h"
#include <windows.h>

#if (RV_MUTEX_TYPE == RV_MUTEX_WIN32_MUTEX)

typedef HANDLE RvAdMutex;

/************************************************************************
 * RvMutexAttr:
 *	OS specific attributes and options used for mutexes. 
 *	See definitions in rvmutex.h
 *  along with the default values in rvccoreconfig.h for more information.
 ************************************************************************/
typedef int RvMutexAttr; /* not used */

#elif (RV_MUTEX_TYPE == RV_MUTEX_WIN32_CRITICAL)

typedef CRITICAL_SECTION RvAdMutex;

/************************************************************************
 * RvMutexAttr:
 *	OS specific attributes and options used for mutexes. 
 *	See definitions in rvmutex.h
 *  along with the default values in rvccoreconfig.h for more information.
 ************************************************************************/
typedef DWORD RvMutexAttr; /* spin count (use only on Win2000 and newer) */

#endif


#endif
