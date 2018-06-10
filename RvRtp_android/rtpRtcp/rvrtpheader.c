/***********************************************************************
        Copyright (c) 2005 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..

RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/

/****************************************************************************

  rvrtpheader.c  --  RTP header initialisation functions

****************************************************************************/
#include "rvstdio.h"
#include "rvrtpheader.h"


#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
**  == RvRtpParamConstruct() ==                                            **
**                                                                         **
**  initializes a RvRtpParam structure                                     **
**                                                                         **
**  PARAMETERS:                                                            **
**      param      pointer to RvRtpParam structure.                        **
**                                                                         **
**  RETURN  : RV_OK on success, other on failure                           **
**                                                                         **
**=========================================================================*/

RVAPI
RvStatus RVCALLCONV RvRtpParamConstruct(INOUT RvRtpParam* param)
{
    memset(param, 0, sizeof(RvRtpParam));
    return RV_OK;
}

/*===========================================================================
**  == RvRtpParamGetExtensionSize() ==                                     **
**                                                                         **
**  returns RTP header extension size in Bytes                             **
**                                                                         **
**  PARAMETERS:                                                            **
**      INPUT param      pointer to RvRtpParam structure, that filled with **
**                       extension fields.                                 **
**                                                                         **
**  RETURN  : RV_OK on success, other on failure                           **
**                                                                         **
**=========================================================================*/
RVAPI
RvUint32 RVCALLCONV RvRtpParamGetExtensionSize(IN RvRtpParam* param)
{
    if (NULL == param)
    {
        return 0; /* error NULL parameter */
    }
    if (param->extensionBit)
        return (param->extensionLength+1)*sizeof(RvUint32);
    else
        return 0;
}

#ifdef __cplusplus
}
#endif



