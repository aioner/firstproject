/*

NOTICE:
This document contains information that is proprietary to RADVISION LTD..
No part of this publication may be reproduced in any form whatsoever without
written prior approval by RADVISION LTD..

RADVISION LTD. reserves the right to revise this publication and make changes
without obligation to notify any person of such revisions or changes.

*/

#include "rvstdio.h"
#include "rvrtpbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

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
RvBool buffAddToBuffer(RvRtpBuffer* to, RvRtpBuffer* from, RvUint32 offset)
{
    if (from->length + offset <= to->length)
    {
        memcpy((RvUint8*)to->buffer + offset, from->buffer, from->length);

        return RV_TRUE;
    }

    return RV_FALSE;
}

RvBool buffValid(RvRtpBuffer* buff, RvUint32 size)
{
    return (size <= buff->length  &&  buff->buffer);
}

/***************************************************************************************
 * buffCreate
 * description:  creates RvRtpBuffer object
 * parameters:
 *    data   - the pointer to memory to create from.
 *    size   - the size of buffer to create
 * returns: RvRtpBuffer object.
 ***************************************************************************************/
RvRtpBuffer buffCreate(void* data,RvInt32 size)
{
    RvRtpBuffer buff;

    buff.buffer = (unsigned char*)data;
    buff.length = size;
    return buff;
}


#ifdef __cplusplus
}
#endif



