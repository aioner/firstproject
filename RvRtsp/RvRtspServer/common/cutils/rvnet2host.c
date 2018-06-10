/* rvnet2host.c - converst network/host organized header bytes */

/************************************************************************
        Copyright (c) 2002 RADVISION Inc. and RADVISION Ltd.
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

#include "rvtypes.h"
#include "rvnet2host.h"
#include "rvsocket.h"

#if (RV_SOCKET_TYPE == RV_SOCKET_BSD)

#if (RV_OS_TYPE == RV_OS_TYPE_OSE)
#include <inet.h>

#elif (RV_OS_TYPE != RV_OS_TYPE_WINCE) && (RV_OS_TYPE != RV_OS_TYPE_MOPI) && \
      (RV_OS_TYPE != RV_OS_TYPE_OSA)
#include <sys/types.h>
#include <netinet/in.h>
#endif

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
#include <sys/types.h>
#include <netinet/in.h>

#endif

#if (RV_ARCH_ENDIAN == RV_ARCH_LITTLE_ENDIAN)
/************************************************************************************************************************
 * RvConvertHostToNetwork64
 *
 * Converts a 64bit integer in host order to an integer in a network format.
 *
 * INPUT   :  host       : value to convert.
 * OUTPUT  :  None.
 * RETURN  :  The converted value.
 */
RVCOREAPI
RvUint64 RVCALLCONV RvConvertHostToNetwork64(RvUint64 host)
{
#if (RV_64BITS_TYPE == RV_64BITS_STANDARD)
	RvUint64 hostByte = host;
	hostByte = (((RvUint64)(((RvUint8*)(&(hostByte)))[7])) | 
		       (((RvUint64)(((RvUint8*)(&(hostByte)))[6])) << 8) | 
			   (((RvUint64)(((RvUint8*)(&(hostByte)))[5])) << 16) | 
			   (((RvUint64)(((RvUint8*)(&(hostByte)))[4])) << 24) |
	           (((RvUint64)(((RvUint8*)(&(hostByte)))[3])) << 32) | 
		       (((RvUint64)(((RvUint8*)(&(hostByte)))[2])) << 40) | 
			   (((RvUint64)(((RvUint8*)(&(hostByte)))[1])) << 48) | 
			   (((RvUint64)(((RvUint8*)(&(hostByte)))[0])) << 56));
#elif (RV_64BITS_TYPE == RV_64BITS_MANUAL)
    RvUint64 hostByte;
    hostByte.words[0] = htons(host.words[3]);
    hostByte.words[1] = htons(host.words[2]);
    hostByte.words[2] = htons(host.words[1]);
    hostByte.words[3] = htons(host.words[0]);
#endif
	return hostByte;
}

/************************************************************************************************************************
 * RvConvertHostToNetwork32
 *
 * Converts an integer in host order to an integer in a network format.
 *
 * INPUT   :  host       : value to convert.
 * OUTPUT  :  None.
 * RETURN  :  The converted value.
 */
RVCOREAPI
RvUint32 RVCALLCONV RvConvertHostToNetwork32(RvUint32 host) 
{
	RvUint32 hostByte = (RvUint32)host;

	hostByte = (((RvUint32)(((RvUint8*)(&(hostByte)))[3])) | 
		       (((RvUint32)(((RvUint8*)(&(hostByte)))[2])) << 8) | 
			   (((RvUint32)(((RvUint8*)(&(hostByte)))[1])) << 16) | 
			   (((RvUint32)(((RvUint8*)(&(hostByte)))[0])) << 24));

	return hostByte;
}


/************************************************************************************************************************
 * RvConvertHostToNetwork16
 *
 * Converts a short integer in host order to a short integer in a network format.
 *
 * INPUT   :  host       : value to convert.
 * OUTPUT  :  None.
 * RETURN  :  The converted value.
 */
RVCOREAPI
RvUint16 RVCALLCONV RvConvertHostToNetwork16(RvUint16 host)
{
    RvUint16 hostByte = host;
	
	hostByte = (RvUint16) (((RvUint16)(((RvUint8*)(&(host)))[1])) | 
						  (((RvUint16)(((RvUint8*)(&(host)))[0])) << 8));

	return hostByte;
}

#endif	/* (RV_ARCH_ENDIAN == RV_ARCH_LITTLE_ENDIAN) */


/************************************************************************************************************************
 * RvNet2Host2Network
 *
 * Converts an array of 4-byte integers from host format to network format.  The integers can
 * then be sent over the network.
 *
 * INPUT   :  buff       : A pointer to the buffer location where the array of 4-byte integers in host format are located.
 *            startIndex : The exact byte location in the buffer where the integers in host format begin.
 *            size       : The number of integers to convert.
 * OUTPUT  :  None.
 * RETURN  :  None
 */
RVCOREAPI
void RVCALLCONV RvNet2Host2Network(
    IN RvUint8  *buff,
    IN RvInt    startIndex,
    IN RvInt    size)
{
    RvInt i;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (buff == NULL)
        return;
#endif

    for (i = startIndex; i < (startIndex + size); i++)
        ((RvUint32*)buff)[i]=RvConvertHostToNetwork32(((RvUint32*)buff)[i]);
}


/***********************************************************************************************************************
 * RvNet2Host2Host
 * Converts an array of 4-byte integers from network format to host format.
 * INPUT   : buff       : A pointer to the buffer location where the array of 4-byte integers in network format are located,
 *                        and where the conversion occurs.
 *           startIndex : The exact byte location in the buffer where the integers in network format begin.
 *           size       : The number of integers to convert.
 * OUTPUT  : None.
 * RETURN  : None.
 */
RVCOREAPI
void RVCALLCONV RvNet2Host2Host(
    IN RvUint8  *buff,
    IN RvInt    startIndex,
    IN RvInt    size)
{
    RvInt i;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (buff == NULL)
        return;
#endif

    for (i = startIndex; i < (startIndex + size);i++)
        ((RvUint32*)buff)[i]= RvConvertNetworkToHost32 (((RvUint32*)buff)[i]);
}

