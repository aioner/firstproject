/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* base64 encoder/decoder. Originally part of main/util.c
 * but moved here so that support/ab and apr_sha1.c could
 * use it. This meant removing the apr_palloc()s and adding
 * ugly 'len' functions, which is quite a nasty cost.
 */

#include "base64.h"
#include <stddef.h>

namespace tghelper
{

	static const char    encodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	static const uint8_t decodeTable[] =
	{
		/* ASCII table */
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
		64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
		64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	};

	uint32_t base64DecodeLen( const char *src )
	{
		register const uint8_t *ptr = (const uint8_t *)src;

		while ( decodeTable[*ptr++] < 64 );

		register const size_t cBytes = ptr - (const uint8_t *)src - 1;
		return ( ( cBytes + 3 ) / 4 ) * 3 + 1;
	}

	/* This is the same as apr_base64_decode() except on EBCDIC machines, where
	 * the conversion of the output to ebcdic is left out.
	 */
	uint32_t base64Decode( uint8_t *dst, const char *src )
	{
		register const uint8_t *sBegin = (const uint8_t *) src;

		while ( decodeTable[*src++] < 64 );
		register size_t cBytes = (const uint8_t *)src - sBegin - 1;
		register size_t bBytes = ( cBytes + 3 ) / 4 * 3;

		while ( cBytes > 4 )
		{
			*dst++ = decodeTable[sBegin[0]] << 2 | decodeTable[sBegin[1]] >> 4;
			*dst++ = decodeTable[sBegin[1]] << 4 | decodeTable[sBegin[2]] >> 2;
			*dst++ = decodeTable[sBegin[2]] << 6 | decodeTable[sBegin[3]] >> 0;
			sBegin += 4;
			cBytes -= 4;
		}

		/* Note: (cBytes == 1) would be an error, so just ingore that case */
		if ( cBytes > 1 )
		{
			*dst++ = decodeTable[sBegin[0]] << 2 | decodeTable[sBegin[1]] >> 4;
		}
		if ( cBytes > 2 )
		{
			*dst++ = decodeTable[sBegin[1]] << 4 | decodeTable[sBegin[2]] >> 2;
		}
		if ( cBytes > 3 )
		{
			*dst++ = decodeTable[sBegin[2]] << 6 | decodeTable[sBegin[3]] >> 0;
		}

		return bBytes - ( ( 4 - cBytes ) & 3 );
	}


	uint32_t base64EncodeLen( uint32_t len )
	{
		return ( ( len + 2 ) / 3 << 2 ) + 1;
	}

	/* This is the same as apr_base64_encode() except on EBCDIC machines, where
	 * the conversion of the input to ascii is left out.
	 */
	uint32_t base64Encode( char *dst, const uint8_t *src, uint32_t len )
	{
		const char *beg = dst;
		const size_t mod = len % 3;
		const size_t div = len / 3 * 3;

		for ( size_t i = 0; i < div; i += 3 )
		{
			*dst++ = encodeTable[( src[i] >> 2 ) & 0x3F];
			*dst++ = encodeTable[( ( src[i]     & 0x03 ) << 4 ) | ( ( src[i + 1] & 0xF0 ) >> 4 )];
			*dst++ = encodeTable[( ( src[i + 1] & 0x0F ) << 2 ) | ( ( src[i + 2] & 0xC0 ) >> 6 )];
			*dst++ = encodeTable[src[i + 2] & 0x3F];
		}
		if ( mod == 0 ) goto encode_end;

		*dst++ = encodeTable[( src[div] >> 2 ) & 0x3F];
		if ( mod == 1 )
		{
			*dst++ = encodeTable[( ( src[div] & 0x3 ) << 4 )];
			*dst++ = '=';
			*dst++ = '=';
		}
		else
		{
			*dst++ = encodeTable[( ( src[div]     & 0x03 ) << 4 ) | ( ( src[div + 1] & 0xF0 ) >> 4 )];
			*dst++ = encodeTable[( ( src[div + 1] & 0x0F ) << 2 )];
			*dst++ = '=';
		}

	encode_end:
		*dst++ = '\0';
		return dst - beg;
	}

}

