/***********************************************************************
Filename   : rv64ascii.c
Description: functions for converting 64 bit numbers to ASCII and back
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
#include "rv64ascii.h"
#include "rvansi.h"
#include <string.h>

/* These functions Deal with differences and bugs in printing 64 bit numbers. */

/********************************************************************************************
 * Rv64AsciiInit
 * Initializes the rv64ascii module. Must be called once (and
 * only once) before any other functions in the module are called.
 * INPUT   :
 * OUTPUT  :
 * RETURN  : RV_OK on success.
 */
RvStatus Rv64AsciiInit(void)
{
    return RV_OK;
}


/********************************************************************************************
 * Rv64AsciiEnd
 * Shuts down the rv64ascii module. Must be called once (and
 * only once) when no further calls to this module will be made.
 * INPUT   :
 * OUTPUT  :
 * RETURN  : RV_OK on success.
 */
RvStatus Rv64AsciiEnd(void)
{
    return RV_OK;
}


/********************************************************************************************
 * Rv64UtoA
 * Converts an unsigned 64 bit integer (RvUint64) to a decimal string.
 * INPUT   : num64              - The unsigned 64 bit number to be converted.
 * OUTPUT  : buf                - The buffer where resulting string will be placed
 * RETURN  : A pointer to buf if successful otherwise NULL.
 * NOTE:  The buffer (buf) should be at least RV_64TOASCII_BUFSIZE bytes long
 *        or the end of the buffer may be overwritten.
 */
RVCOREAPI RvChar * RVCALLCONV Rv64UtoA(
    IN RvUint64 num64,
    OUT RvChar *buf)
{
#if (RV_64TOASCII_TYPE == RV_64TOASCII_MANUAL)
    RvChar *cptr, tmpbuf[RV_64TOASCII_BUFSIZE];
    RvInt32 temp;
    RvSize_t size;
#endif

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(buf == NULL) return NULL;
#endif

#if (RV_64TOASCII_TYPE == RV_64TOASCII_STANDARD)
    RvSprintf(buf, "%llu", num64);
#endif
#if (RV_64TOASCII_TYPE == RV_64TOASCII_WIN32)
    _ui64toa(num64, buf, 10);
#endif
#if (RV_64TOASCII_TYPE == RV_64TOASCII_MANUAL)
    /* sprintf is broken, have to do it manually */
    cptr = &tmpbuf[RV_64TOASCII_BUFSIZE - 1];
    *cptr = '\0';
    size = 1;
    do{
        cptr -= 1;
        temp = RvUint64ToRvInt32(RvUint64Mod(num64, RvUint64Const(0,10)));
        num64 = RvUint64Div(num64, RvUint64Const(0,10));
        *cptr = (RvChar)('0' + (RvChar)temp);
        size++;
    } while(RvUint64IsGreaterThan(num64, RvUint64Const(0,0)));
    memcpy(buf, cptr, size);
#endif

    return buf;
}


/********************************************************************************************
 * Rv64toA
 * Converts an signed 64 bit integer (RvInt64) to a decimal string.
 * INPUT   : num64              - The unsigned 64 bit number to be converted.
 * OUTPUT  : buf                - The buffer where resulting string will be placed
 * RETURN  : A pointer to buf if successful otherwise NULL.
 * NOTE:  The buffer (buf) should be at least RV_64TOASCII_BUFSIZE bytes long
 *        or the end of the buffer may be overwritten.
 */
RVCOREAPI RvChar * RVCALLCONV Rv64toA(
    IN RvInt64 num64,
    OUT RvChar *buf)
{
#if (RV_64TOASCII_TYPE == RV_64TOASCII_MANUAL)
    RvChar *cptr;
    RvUint64 tmpnum64;
#endif

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(buf == NULL) return NULL;
#endif

#if (RV_64TOASCII_TYPE == RV_64TOASCII_STANDARD)
    RvSprintf(buf, "%lld", num64);
#endif
#if (RV_64TOASCII_TYPE == RV_64TOASCII_WIN32)
    _i64toa(num64, buf, 10);
#endif
#if (RV_64TOASCII_TYPE == RV_64TOASCII_MANUAL)
    /* sprintf is broken, have to do it manually */
    /* Put the sign in and send the rest as unsigned */
    cptr = buf;
    if(RvInt64IsLessThan(num64, RvInt64Const(0,0,0))) {
        *cptr = '-';
        num64 = RvInt64Mul(num64, RvInt64Const(-1,0,1));
        cptr += 1;
    } 
    
    tmpnum64 = RvUint64FromRvInt64(num64);
    Rv64UtoA(tmpnum64, cptr);
#endif

    return buf;
}


/********************************************************************************************
 * Rv64UtoHex
 * Converts an unsigned 64 bit integer (RvUint64) to a hexadecimal string.
 * INPUT   : num64              - The unsigned 64 bit number to be converted.
 * OUTPUT  : buf                - The buffer where resulting string will be placed
 * RETURN  : A pointer to buf if successful otherwise NULL.
 * NOTE:  The buffer (buf) should be at least RV_64TOHEX_BUFSIZE bytes long
 *        or the end of the buffer may be overwritten.
 */
RVCOREAPI RvChar * RVCALLCONV Rv64UtoHex(
    IN RvUint64 num64,
    OUT RvChar *buf)
{
#if (RV_64TOASCII_TYPE == RV_64TOASCII_MANUAL)
    RvUint32 msb, lsb;
    int shiftnum;
#endif

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(buf == NULL) return NULL;
#endif

#if (RV_64TOASCII_TYPE == RV_64TOASCII_STANDARD)
    RvSprintf(buf, "%llx", num64);
#endif
#if (RV_64TOASCII_TYPE == RV_64TOASCII_WIN32)
    RvSprintf(buf, "%I64x", num64);
#endif

#if (RV_64TOASCII_TYPE == RV_64TOASCII_MANUAL)
    /* sprintf is broken, have to do it in pieces */
    shiftnum = 32; /* compiler workaround */
    msb = RvUint64ToRvUint32(RvUint64ShiftRight(num64, shiftnum));
    lsb = RvUint64ToRvUint32(num64);
    if(msb > 0) {
        RvSprintf(buf, "%x%08x", msb, lsb);
    } else RvSprintf(buf,"%x",lsb);
#endif

    return buf;
}

/******************************************************************************
 * RvAto64
 * Converts an NULL-terminated string to signed 64 bit integer (RvInt64).
 * INPUT   : str - The string to be converted.
 * RETURN  : 64 bit integer
 */
RVCOREAPI void RVCALLCONV RvAto64(IN RvChar* str, OUT RvInt64* pNum64)
{
#if (RV_64TOASCII_TYPE == RV_64TOASCII_STANDARD)
    /* atoll() is provided by GNU libc6 and later.
       For now we don't recognize the version of libc.
       Therefore use sscanf() in order to ensure wide compatibility.
    *pNum64 = atoll((const RvChar*)(str));
    */
    RvSscanf(str, "%lld", pNum64);
#endif

#if (RV_64TOASCII_TYPE == RV_64TOASCII_WIN32)
    *pNum64 = _atoi64((const RvChar*)(str));
#endif

#if (RV_64TOASCII_TYPE == RV_64TOASCII_MANUAL)
    RvChar*  pStr = str;
    RvInt64  i64res;
    RvInt64  i64ten;

    i64ten = RvInt64Const(1,0,10);
    i64res = RvInt64Const(0,0,0);

    if (*pStr == '-')
        pStr++;

    while (*pStr != '\0')
    {
        i64res = RvInt64Mul(i64res, i64ten);
        i64res = RvInt64Add(i64res, RvInt64FromRvChar((RvChar)(*pStr-'0')));
        pStr++;
    }

    if (str[0] == '-')
    {
        i64res = RvInt64Mul(i64res, RvInt64Const(-1,0,1));
    }
    *pNum64 = i64res;
#endif
}

