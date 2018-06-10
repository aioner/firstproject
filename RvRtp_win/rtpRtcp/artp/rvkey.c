/************************************************************************
 File Name     : key.c
 Description   :
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
************************************************************************/

#include "rvccore.h"

#if (RV_SECURITY == RV_YES)
#include "rvtypes.h"
#include "rvkey.h"
/*#include "rvmd5.h"*/
#include <ctype.h>

#define RV_KEY_MAXIMUMPASSPHRASELENGTH 256


static RvUint8 rvUint8BitMask[9] = {0x00,0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF};
#define rvUint8GetLSB(_s,_i)    ((_s)&&rvUint8BitMask[(_i)])
#define rvUint8GetMSB(_s,_i)    ((_s >> (8 - (_i)))&&rvUint8BitMask[(_i)])

RVAPI
RvBool RVCALLCONV RvKeyEqual(const RvKey* thisPtr, const RvKey* bPtr)
{
    const RvUint8* a;
    const RvUint8* b;
    const RvUint8* end;
    int            remainder;

    /* Check bit size */
    if(thisPtr->bitLength != bPtr->bitLength)
        return RV_FALSE;

    /* Check full bytes */
    a = thisPtr->material;
    b = bPtr->material;
    end = &thisPtr->material[(thisPtr->bitLength/8)];
    while(a != end)
        if(*a++ != *b++)
            return RV_FALSE;

    /* Check remaining bits */
    remainder = thisPtr->bitLength%8;
    if((remainder != 0) && (rvUint8GetLSB(*a,remainder) != rvUint8GetLSB(*b,remainder)))
        return RV_FALSE;

    return RV_TRUE;
}
#if 0
/* From RFC1890. Assumes ASCII character set. */
RvBool rvKeySetValueFromPassPhrase(RvKey* thisPtr, const char* passPhrase, size_t bitLength)
{
        char        hash[RV_MD5_HASHLENGTH];
        int         byteLength = ((bitLength + 7)/8);
        const char* start = passPhrase;
        const char* end   = passPhrase + strlen(passPhrase) - 1;
        char        canonicalPassPhrase[RV_KEY_MAXIMUMPASSPHRASELENGTH];
        char*       i = canonicalPassPhrase;
        char        prev;
        int         k;

        thisPtr->bitLength = bitLength;

        /* Generate a canonical pass phrase. Trim off leading and trailing white space,
           convert all characters to lowercase and replace one or more contiguous
           white space characters by a single space. */

        /* Trim off white space from ends */
        while(isspace((RvInt) *start)) start++;
        while(isspace((RvInt) *end)) end--;

        /* Crunch out redundant white spaces and convert to lowercase */
        prev = ' ';
        while(start <= end)
        {
            char c = *start++;

            /* Convert all white spaces to 0x20 */
            if(isspace((RvInt) c))
                c = ' ';

            if(((c == ' ')&&(prev != ' '))||(c != ' '))
            {
                /* Check for overflow */
                if(i == (canonicalPassPhrase + RV_KEY_MAXIMUMPASSPHRASELENGTH))
                    return RV_FALSE;

                /* Copy character to canonical pass phrase */
                *i++ = (RvInt8) tolower(c);
                prev = c;
            }
        }

        /* Hashed the normalized string */
        rvMd5(canonicalPassPhrase, i-canonicalPassPhrase, hash);

        /* Copy hashed string into the key */
        for(k = 0; k < byteLength/RV_MD5_HASHLENGTH; k++)
            memcpy(thisPtr->material + k*RV_MD5_HASHLENGTH, hash, RV_MD5_HASHLENGTH);

        if((byteLength%RV_MD5_HASHLENGTH) != 0)
            memcpy(thisPtr->material + k*RV_MD5_HASHLENGTH, hash, byteLength%RV_MD5_HASHLENGTH);

        return RV_TRUE;
}
#endif

#else

int prevent_warning_of_ranlib_has_no_symbols_artp_rvkey=0;

#endif /* RV_SECURITY */
