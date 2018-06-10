/*rvcbase.h - cbase main header file */
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
#ifndef RV_CBASE_H
#define RV_CBASE_H

#include "rvccore.h"
#include "rvcbaseconfig.h"
#include "rvcommoncorepublic.h"

/* Module codes (1..1023). One for each module in cbase. See rverror.h for details */
#define RV_CBASE_MODULE_CBASE 1
#define RV_CBASE_MODULE_QUEUE 2
#define RV_CBASE_MODULE_TIMER 3
#define RV_CBASE_MODULE_TIMERENGINE 4
#define RV_CBASE_MODULE_NETHOST 5
#define RV_CBASE_MODULE_IOLAYER 6
#define RV_CBASE_MODULE_DEFAULTNETDRV 7 /* default network driver */
#define RV_CBASE_MODULE_NETBUF 8
#define RV_CBASE_MODULE_SMQ    9


#endif /* RV_CBASE_H */
