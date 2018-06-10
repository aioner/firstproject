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
#ifndef RV_COMMONCOREPUBLIC_H
#define RV_COMMONCOREPUBLIC_H

#include "rverror.h"

/*@****************************************************************************
* Module: RvCommonCorePublic (root)
* ----------------------------------------------------------------------------
* Title:  RADVISION Public API
*
* This section describes the RADVISION toolkits public API
***************************************************************************@*/


#if defined(__cplusplus)
extern "C" {
#endif

/* Prototypes: See documentation blocks below for details. */



/*@****************************************************************************
 * RvCBaseInit (RvCommonCorePublic)
 * ----------------------------------------------------------------------------
 * General:
 * RADVISION CommonCore initialization function. Must be called prior to any other
 * RADVISION API call. 
 * The function RvCBaseEnd must be called as the last RADVISION API function when no
 * RADVISION Toolkits are used any longer.
 * This function is not re-entrant. Simultaneous calls to this function must not be
 * made.
 * 
 * Arguments:
 * Input:           None.
 * Output:          None.
 *
 * Return Value:    RV_OK, or the relevant error code.
 ***************************************************************************@*/
RVCOREAPI RvStatus RVCALLCONV RvCBaseInit(void);

/*@****************************************************************************
* RvCBaseEnd (RvCommonCorePublic)
* ----------------------------------------------------------------------------
* General:
* RADVISION CommonCore shutdown function. Must be called as the last
* RADVISION API call. 
* This function is not re-entrant. Simultaneous calls to this function must not be
* made.
* 
* Arguments:
* Input:           None.
* Output:          None.
*
* Return Value:    RV_OK, or the relevant error code.
***************************************************************************@*/
RVCOREAPI RvStatus RVCALLCONV RvCBaseEnd(void);

#if defined(__cplusplus)
}
#endif

#endif /* RV_CBASE_H */
