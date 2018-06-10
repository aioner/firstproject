/***********************************************************************
Filename   : rvconfig.h
Description: config files which incorporates other config files
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
#ifndef RV_SYSCONFIG_H
#define RV_SYSCONFIG_H

#define RV_RELEASE

#if defined(LINUX)
#define RV_OS_TYPE RV_OS_TYPE_LINUX
#define RV_OS_VERSION RV_OS_LINUX_GENERIC_LINUX  
#define RV_ARCH_ENDIAN RV_ARCH_LITTLE_ENDIAN
#endif

/* Pull in definitions required for configuration */
#include "rvarchdefs.h"
#include "rvosdefs.h"
#include "rvtooldefs.h"
#include "rvinterfacesdefs.h" /* Core interfaces */




/* Generic macros: default values */
#include "rvccoreconfig.h"

/* Configure system based on above definitions */
#include "rvarchconfig.h"
#include "rvosconfig.h"
#include "rvtoolconfig.h"

#if !defined(RV_NOUSRCONFIG)
/* Pull in user (override) definitions */
#include "rvusrconfig.h"
#endif

/* Pull in CFLAGS definitions */
#include "rvcflags.h"

/* Calculated dependencies */
#include "rvdependencies.h"

#endif /* RV_SYSCONFIG_H */
