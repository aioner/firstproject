/************************************************************************
 File Name	   :   bitfield.c  --  Bit-field manipulation
 Description   : scope: Private
                 Misc. routines for bit field manipulation.
*************************************************************************/
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

#ifndef __BITFIELD_H
#define __BITFIELD_H

#include "rvtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
**  == bitfieldSet() ==                                                    **
**                                                                         **
**  Sets a sequence of bits in an integer to a specified value.            **
**                                                                         **
**  PARAMETERS:                                                            **
**      value      The integer value to work on.                           **
**                                                                         **
**      bitfield   The bit sequence to insert into the integer.            **
**                                                                         **
**      nStartBit  This value specifies the list-segnificant bit index     **
**                 for the bit sequence.  e.g. if nStartBit is set to 1,   **
**                 and nBits is set to 3, the bit sequence will occupy     **
**                 bits 1 through 4.                                       **
**                                                                         **
**      nBits      The number of bits in the bit sequence.                 **
**                                                                         **
**  RETURNS:                                                               **
**      The new integer value.                                             **
**                                                                         **
**=========================================================================*/
RvUint32 bitfieldSet(
    IN  RvUint32    value,
    IN  RvUint32    bitfield,
    IN  RvInt32         nStartBit,
    IN  RvInt32         nBits);

/*===========================================================================
**  == bitfieldGet() ==                                                    **
**                                                                         **
**  Retreives the sequence of bits in an integer.                          **
**                                                                         **
**  PARAMETERS:                                                            **
**      value      The integer value to work on.                           **
**                                                                         **
**      nStartBit  This value specifies the list-segnificant bit index     **
**                 of the bit sequence.  e.g. if nStartBit is set to 1,    **
**                 and nBits is set to 3, the bit sequence in bits 1       **
**                 through 4 is used.                                      **
**                                                                         **
**      nBits      The number of bits in the bit sequence.                 **
**                                                                         **
**  RETURNS:                                                               **
**      Returns the bit sequence.  The sequence will occupy bits           **
**      0..(Bits-1).                                                       **
**                                                                         **
**=========================================================================*/
RvUint32 bitfieldGet(
    IN  RvUint32    value,
    IN  RvInt32         nStartBit,
    IN  RvInt32         nBits);

/*===========================================================================
**  == bitfieldGet2() ==                                                    **
**                                                                         **
**  Retreives the sequence of bits in an half integer.                     **
**                                                                         **
**  PARAMETERS:                                                            **
**      value      The integer value to work on.                           **
**                                                                         **
**      nStartBit  This value specifies the list-segnificant bit index     **
**                 of the bit sequence.  e.g. if nStartBit is set to 1,    **
**                 and nBits is set to 3, the bit sequence in bits 1       **
**                 through 4 is used.                                      **
**                                                                         **
**      nBits      The number of bits in the bit sequence.                 **
**                                                                         **
**  RETURNS:                                                               **
**      Returns the bit sequence.  The sequence will occupy bits           **
**      0..(Bits-1).                                                       **
**                                                                         **
**=========================================================================*/
RvUint16 bitfieldGet2(
    IN  RvUint16	value,
    IN  RvUint32		nStartBit,
    IN  RvUint32		nBits);

/*===========================================================================
 **  == bitfieldSet2() ==                                                   **
 **                                                                         **
 **  Sets a sequence of bits in an half integer to a specified value.       **
 **                                                                         **
 **  PARAMETERS:                                                            **
 **      value      The integer value to work on.                           **
 **                                                                         **
 **      bitfield   The bit sequence to insert into the integer.            **
 **                                                                         **
 **      nStartBit  This value specifies the list-segnificant bit index     **
 **                 for the bit sequence.  e.g. if nStartBit is set to 1,   **
 **                 and nBits is set to 3, the bit sequence will occupy     **
 **                 bits 1 through 4.                                       **
 **                                                                         **
 **      nBits      The number of bits in the bit sequence.                 **
 **                                                                         **
 **  RETURNS:                                                               **
 **      The new integer value.                                             **
 **                                                                         **
 **=========================================================================*/
RvUint16 bitfieldSet2(
    IN  RvUint16	value,
    IN  RvUint16	bitfield,
    IN  RvUint32  nStartBit,
    IN  RvUint32	nBits);

#define rvBitfieldAdjustToNextByte(bitOffset) (*(bitOffset)) = ((*(bitOffset))+7) & ~RvUint32Const(7)

RvUint32 rvBitfieldRead(RvUint8*   bufferPtr, 
						RvSize_t* bitOffset,  
						RvSize_t  sizeInBits); /* size in bits <= 32 bits */

RvInt32 rvBitfieldReadSigned(RvUint8*   bufferPtr, 
							 RvSize_t* bitOffset,  
							 RvSize_t  sizeInBits); /* size in bits <= 32 bits */

void rvBitfieldWrite(RvUint8*   bufferPtr, 
					 RvSize_t* bitOffset,  
					 RvSize_t  sizeInBits, /* size in bits <= 32 bits */
					 RvUint32  value); 
#ifdef __cplusplus
}
#endif

#endif /* __BITFIELD_H */

