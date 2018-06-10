/******************************************************************************
Filename    :rvbase64.c
Description :b64 encoding decoding routines..

  ******************************************************************************
  Copyright (c) 2005 RADVision Inc.
  ************************************************************************
  NOTICE:
  This document contains information that is proprietary to RADVision LTD.
  No part of this publication may be reproduced in any form whatsoever
  without written prior approval by RADVision LTD..

    RADVision LTD. reserves the right to revise this publication and make
    changes without obligation to notify any person of such revisions or
    changes.
    ******************************************************************************
Author:Rafi Kiel
******************************************************************************/
#include <stdlib.h>
#include "rvtypes.h"
#include "rvbase64.h"

/*
 Encoding table
*/
static const RvChar encTable[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 Decoding table
*/
static const RvInt decTable[] =
{
/*  0  */ -2,
/*  1  */ -2,
/*  2  */ -2,
/*  3  */ -2,
/*  4  */ -2,
/*  5  */ -2,
/*  6  */ -2,
/*  7  */ -2,
/*  8  */ -2,
/*  9  */ -2,
/* 10  */ -2,
/* 11  */ -2,
/* 12  */ -2,
/* 13  */ -2,
/* 14  */ -2,
/* 15  */ -2,
/* 16  */ -2,
/* 17  */ -2,
/* 18  */ -2,
/* 19  */ -2,
/* 20  */ -2,
/* 21  */ -2,
/* 22  */ -2,
/* 23  */ -2,
/* 24  */ -2,
/* 25  */ -2,
/* 26  */ -2,
/* 27  */ -2,
/* 28  */ -2,
/* 29  */ -2,
/* 30  */ -2,
/* 31  */ -2,
/* 32  */ -2,
/* 33 !*/ -2,
/* 34 "*/ -2,
/* 35 #*/ -2,
/* 36 $*/ -2,
/* 37 %*/ -2,
/* 38 &*/ -2,
/* 39 '*/ -2,
/* 40 (*/ -2,
/* 41 )*/ -2,
/* 42 **/ -2,
/* 43 +*/ 62,
/* 44 ,*/ -2,
/* 45 -*/ -2,
/* 46 .*/ -2,
/* 47 / */ 63,
/* 48 0*/ 52,
/* 49 1*/ 53,
/* 50 2*/ 54,
/* 51 3*/ 55,
/* 52 4*/ 56,
/* 53 5*/ 57,
/* 54 6*/ 58,
/* 55 7*/ 59,
/* 56 8*/ 60,
/* 57 9*/ 61,
/* 58 :*/ -2,
/* 59 ;*/ -2,
/* 60 <*/ -2,
/* 61 =*/ -1,
/* 62 >*/ -2,
/* 63 ?*/ -2,
/* 64 @*/ -2,
/* 65 A*/  0,
/* 66 B*/  1,
/* 67 C*/  2,
/* 68 D*/  3,
/* 69 E*/  4,
/* 70 F*/  5,
/* 71 G*/  6,
/* 72 H*/  7,
/* 73 I*/  8,
/* 74 J*/  9,
/* 75 K*/ 10,
/* 76 L*/ 11,
/* 77 M*/ 12,
/* 78 N*/ 13,
/* 79 O*/ 14,
/* 80 P*/ 15,
/* 81 Q*/ 16,
/* 82 R*/ 17,
/* 83 S*/ 18,
/* 84 T*/ 19,
/* 85 U*/ 20,
/* 86 V*/ 21,
/* 87 W*/ 22,
/* 88 X*/ 23,
/* 89 Y*/ 24,
/* 90 Z*/ 25,
/* 91 [*/ -2,
/* 92 \*/ -2,
/* 93 ]*/ -2,
/* 94 ^*/ -2,
/* 95 _*/ -2,
/* 96 `*/ -2,
/* 97 a*/ 26,
/* 98 b*/ 27,
/* 99 c*/ 28,
/*100 d*/ 29,
/*101 e*/ 30,
/*102 f*/ 31,
/*103 g*/ 32,
/*104 h*/ 33,
/*105 i*/ 34,
/*106 j*/ 35,
/*107 k*/ 36,
/*108 l*/ 37,
/*109 m*/ 38,
/*110 n*/ 39,
/*111 o*/ 40,
/*112 p*/ 41,
/*113 q*/ 42,
/*114 r*/ 43,
/*115 s*/ 44,
/*116 t*/ 45,
/*117 u*/ 46,
/*118 v*/ 47,
/*119 w*/ 48,
/*120 x*/ 49,
/*121 y*/ 50,
/*122 z*/ 51,
/*123 {*/ -2,
/*124 |*/ -2,
/*125 }*/ -2,
/*126 ~*/ -2,
/*127  */ -2
};




/********************************************************************************************
 * rvEncodeB64
 * Performs encode B64 operation of inLen bytes in inTxt buffer.
 * INPUT   : 
 *           inTxt - the buffer to be encoded.
 *           inLen - the length of buffer to be encoded
 *           outTxt - this buffer is used as encoding destination
 *           outLen - size of 'outTxt' buffer
 *           
 * RETURN  : length of encoded buffer or -1 if fails.
 */
RVCOREAPI
RvInt rvEncodeB64(
                     IN RvUint8* inTxt,   
                     IN RvInt inLen,      
                     INOUT RvUint8* outTxt, 
                     IN RvInt outLen)    
{
    RvUint8 *in, *out, inS[3] = {0,0,0};
    RvInt l, testLen = outLen;

    outLen = 0; /* here we'll accumulate the lenght of output buffer */
    in = inTxt;
    out = outTxt;
    l = 3;
    while (inLen)
    {
        if (testLen < 4)
            /* not enough room for output */
            return -1;

        testLen -= 4;

        if (inLen < 3)
        {
            /* if there are less than three bytes left in the input
               we'll use the 'inS' array (initially filled with 0) */
            inS[0] = *in; /* the inLen is at least 1 so take the first byte
                             from the input */
            if (inLen == 2)    /* if the inLen is 2 take the second byte from input */
                inS[1] = in[1];
                            /* the third (and may be the second) byte will be 0 */
            in = inS;
            l = inLen;
        }

        *out++ = encTable[ in[0] >> 2 ];
        *out++ = encTable[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];

        /* if we do not have three bytes now ('l' is 1 or 2) we will fill last two
           (or one) bytes of quartet with '=' character */
        *out++ = (RvUint8) (l > 1 ? encTable[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
        *out++ = (RvUint8) (l > 2 ? encTable[ in[2] & 0x3f ] : '=');

        outLen += 4;
        in += l;
        inLen -= l;
    }
    return outLen;
}

/********************************************************************************************
 * rvDecodeB64
 * Performs decode B64 operation of inLen bytes in inTxt buffer.
 * INPUT   : 
 *           inTxt - the buffer to be decoded.
 *           inLen - the length of buffer to be decoded
 *           outTxt - this buffer is used as decoding destination
 *           outLen - size of 'outTxt' buffer
 *           
 * RETURN  : number of used bytes in the 'outTxt' or -1 if function fails
 */
RVCOREAPI
RvInt rvDecodeB64(
                  IN RvUint8* inTxt,  
                  IN RvInt inLen,     
                  INOUT RvUint8* outTxt, 
                  IN RvInt outLen)
{
    RvUint8 in[4], *out;
    RvChar c;
    RvInt i, testLen = outLen;

    out = outTxt;
    outLen = 0;

    while (inLen)
    {
        for (i=0; i<4 && inLen;  i++, inTxt++, inLen--)
        {
            c = (RvChar) decTable[*inTxt];
            if (c == (RvChar)-2)
                return -1;
            if (c == (RvChar)-1)
                break;
            in[i] = c;
        }

        if (testLen < (i-1))
            /* not enough room for output */
            return -1;
        testLen -= (i-1);

        *out++ = (RvUint8) (in[0] << 2 | in[1] >> 4);
        if (i > 2)
        {
            *out++ = (RvUint8) (in[1] << 4 | in[2] >> 2);
            if (i == 4)
                *out++ = (RvUint8) (((in[2] << 6) & 0xc0) | in[3]);
        }

        outLen += i-1;

        if (i < 4)
            break;
    }

    return outLen;
}

#ifdef TEST_RV64
void TestB64Function()
{
    char tIn[1000], tOut[2000], t3[1000],t[10];
    int i1,i2,cnt;

    memset(tIn,0,sizeof(tIn));
    memset(tOut,0,sizeof(tOut));
    memset(t3,0,sizeof(t3));
    for (cnt = 0; cnt < 250; cnt ++)
    {
        sprintf(t,"%03d",cnt);
        strcat(tIn,t);
    }

    for (cnt = 1; cnt <= 1000; cnt++)
    {
        i1 = rvEncodeB64(tIn,cnt,tOut,sizeof(tOut));
        if (i1 == -1)
            exit(0);
        i2 = rvDecodeB64(tOut,i1,t3,cnt);
        if (i2 != cnt)
            exit(0);
        if (strncmp(tIn,t3,cnt))
            exit(0);
    }
}
#endif
