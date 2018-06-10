/*

NOTICE:
This document contains information that is proprietary to RADVISION LTD..
No part of this publication may be reproduced in any form whatsoever without
written prior approval by RADVISION LTD..

RADVISION LTD. reserves the right to revise this publication and make changes
without obligation to notify any person of such revisions or changes.

*/

#ifndef _RV_RTP_BUFFER_H
#define _RV_RTP_BUFFER_H

#include "rvtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    RvUint32 length;
    RvUint8*  buffer;
} RvRtpBuffer;

/***************************************************************************************
 * buffAddToBuffer
 * description:  copies buffer information from buffer-from to buffer-to
 * parameters:
 *    to     - the pointer to the destination buffer .
 *    from   - the pointer to the source buffer ..
 *    offset - offset
 * returns: RV_TRUE, if successful. 
 *          RV_FALSE, no room to copy.
 ***************************************************************************************/
RvBool buffAddToBuffer(RvRtpBuffer* to, RvRtpBuffer* from, RvUint32 offset);

RvBool buffValid(RvRtpBuffer* buff,RvUint32 size);

/***************************************************************************************
 * buffCreate
 * description:  creates RvRtpBuffer object
 * parameters:
 *    data   - the pointer to memory to create from.
 *    size   - the size of buffer to create
 * returns: RvRtpBuffer object.
 ***************************************************************************************/
RvRtpBuffer buffCreate(void* data,RvInt32 size);


#ifdef __cplusplus
}
#endif

#endif /* _RV_RTP_BUFFER_H */




