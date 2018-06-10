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
 * The apr_vsnprintf/apr_snprintf functions are based on, and used with the
 * permission of, the  SIO stdio-replacement strx_* functions by Panos
 * Tsirigotis <panos@alumni.cs.colorado.edu> for xinetd.
 */

#ifndef BASE64_H_
#define BASE64_H_

#include<stdint.h>

namespace tghelper 
{
	/**
	 * @defgroup APR_Util_Base64 Base64 Encoding
	 * @ingroup APR_Util
	 * @{
	 */

	/* Simple BASE64 encode/decode functions.
	 * 
	 * As we might encode binary strings, hence we require the length of
	 * the incoming plain source. And return the length of what we decoded.
	 *
	 * The decoding function takes any non valid char (i.e. whitespace, \0
	 * or anything non A-Z,0-9 etc as terminal.
	 * 
	 * plain strings/binary sequences are not assumed '\0' terminated. Encoded
	 * strings are neither. But probably should.
	 *
	 */

	/**
	 * Given the length of an un-encrypted string, get the length of the 
	 * encrypted string.
	 * @param len the length of an unencrypted string.
	 * @return the length of the string after it is encrypted
	 */ 
	uint32_t base64EncodeLen( uint32_t len );

	/**
	 * Encode an EBCDIC string using base64encoding.
	 * @param dst The destination string for the encoded string.
	 * @param src The original string in plain text
	 * @param src The length of the plain text string
	 * @return the length of the encoded string
	 */ 
	uint32_t base64Encode( char *dst, const uint8_t *src, uint32_t len );

	/**
	 * Determine the maximum buffer length required to decode the plain text
	 * string given the encoded string.
	 * @param coded_src The encoded string
	 * @return the maximum required buffer length for the plain text string
	 */ 
	uint32_t base64DecodeLen( const char *src );

	/**
	 * Decode an EBCDIC string to plain text
	 * @param plain_dst The destination string for the plain text
	 * @param coded_src The encoded string 
	 * @return the length of the plain text string
	 */ 
	uint32_t base64Decode( uint8_t *dst, const char *src );

	/** @} */

}

#endif	/* !BASE64_H_ */
