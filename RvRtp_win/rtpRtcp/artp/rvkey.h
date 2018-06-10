/************************************************************************
 File Name     : rvkey.h
 Description   : This file contains the RvKey class. It is used to 
                 represent an encryption key.
*************************************************************************
 Copyright (c)  2002 , RADVision, Inc. All rights reserved.
*************************************************************************
 NOTICE:
 This document contains information that is proprietary to RADVISION Inc. 
 No part of this publication may be reproduced in any form whatsoever
 without written prior approval by RADVISION Inc. 
 
 RADVISION Inc. reserves the right to revise this publication and make
 changes without obligation to notify any person of such revisions or
 changes.
*************************************************************************
 $Revision: $
 $Date:   07/02/2002 $
 $Author: Scott K. Eaton $
************************************************************************/

#if !defined(RVKEY_H)
#define RVKEY_H

#include "rvtypes.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RV_KEY_MAXIMUMKEYBITLENGTH     256

typedef struct RvKey_
{
    size_t  bitLength;                                    /* Key Bitlength */
    RvUint8 material[(RV_KEY_MAXIMUMKEYBITLENGTH+7)/8];   /* key value */
} RvKey;
/********************************************************************************
 * RvKeyConstruct  RvKey constructor. 
 * INPUT: buf - buffer to serialize (unused)
 *        len - length of the buf   (unused)
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to g722 (unused)
 * result : return RV_OK if there are no errors
 ********************************************************************************/
#define RvKeyConstruct(_t)       (((_t)->bitLength = 0)/*,(_t)*/)

#define RvKeyDestruct(_t)        RV_UNUSED_ARG(_t)

#define RvKeyCopy(_t,_s)         ((_t)->bitLength = (_s)->bitLength, memcpy((void*)(_t)->material,(const void*)(_s)->material, (((_t)->bitLength)+7)/8))
/********************************************************************************
 * rvKeyEqual  checks if keys are equal. 
 * INPUT: thisPtr - first key to be checked
 *        bPtr    - second key to be checked     
 * result : RV_TRUE, if keys are equal, RV_FALSE otherwise
 ********************************************************************************/
RVAPI
RvBool RVCALLCONV RvKeyEqual(const RvKey* thisPtr, const RvKey* bPtr);

#define RvKeyGetLengthInBits(_t) ((_t)->bitLength)
#define RvKeyGetMaterial(_t)     ((const RvUint8*)(_t)->material)
#define RvKeySetValue(_t,_m,_l)  ((_t)->bitLength = (_l), memcpy((void*)(_t)->material,(const void*)(_m), ((_l)+7)/8))
/*
RvBool  rvKeySetValueFromPassPhrase(RvKey* thisPtr, const char* passPhrase, size_t bitLength);
*/

#ifdef __cplusplus
}
#endif

#endif  /* Include guard */

