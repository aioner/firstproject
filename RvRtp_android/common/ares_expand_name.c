/* Copyright 1998 by the Massachusetts Institute of Technology.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

#include "rvccore.h"

#if (RV_DNS_TYPE == RV_DNS_ARES)
#include "ares.h"
#include <stdlib.h>

#ifndef INDIR_MASK
#  define INDIR_MASK 0xc0
#endif

/* Expand an RFC1035-encoded domain name given by encoded.  The
 * containing message is given by abuf and alen.  The result given by
 * *s, which is set to a NUL-terminated allocated buffer.  *enclen is
 * set to the length of the encoded name (not the length of the
 * expanded name; the goal is to tell the caller how many bytes to
 * move forward to get past the encoded name).
 *
 * In the simple case, an encoded name is a series of labels, each
 * composed of a one-byte length (limited to values between 0 and 63
 * inclusive) followed by the label contents.  The name is terminated
 * by a zero-length label.
 *
 * In the more complicated case, a label may be terminated by an
 * indirection pointer, specified by two bytes with the high bits of
 * the first byte (corresponding to INDIR_MASK) set to 11.  With the
 * two high bits of the first byte stripped off, the indirection
 * pointer gives an offset from the beginning of the containing
 * message with more labels to decode.  Indirection can happen an
 * arbitrary number of times, so we have to detect loops.
 *
 * Since the expanded name uses '.' as a label separator, we use
 * backslashes to escape periods or backslashes in the expanded name.
 */

/* Return the length of the expansion of an encoded domain name, or
 * -1 if the encoding is invalid.
 */
int rv_ares_name_length(const unsigned char *encoded, const unsigned char *abuf, RvSize_t alen)
{
    int n = 0;
    RvSize_t offset;
    RvSize_t indir = 0;

    /* Allow the caller to pass us abuf + alen and have us check for it. */
    if (encoded == abuf + alen)
        return -1;

    while (*encoded)
    {
        if ((*encoded & INDIR_MASK) == INDIR_MASK)
        {
            /* Check the offset and go there. */
            if (encoded + 1 >= abuf + alen)
                return -1;
            offset = (*encoded & ~INDIR_MASK) << 8 | *(encoded + 1);
            if (offset >= alen)
                return -1;
            encoded = abuf + offset;

            /* If we've seen more indirects than the message length,
             * then there's a loop.
             */
            if (++indir > alen)
                return -1;
        }
        else
        {
            offset = *encoded;
            if (encoded + offset + 1 >= abuf + alen)
                return -1;
            encoded++;
            while (offset--)
            {
                n += (*encoded == '.' || *encoded == '\\') ? 2 : 1;
                encoded++;
            }
            n++;
        }
    }

    /* If there were any labels at all, then the number of dots is one
     * less than the number of labels, so subtract one.
     */
    return (n) ? n - 1 : n;
}


int rv_ares_enc_length(const unsigned char *inencoded, const unsigned char *abuf, size_t msgLen) {
    int enclen = 0;
    int curEnc = 0;
    const unsigned char *last    = abuf + msgLen;
    const unsigned char *encoded = inencoded;

    while(((curEnc = *encoded) > 0) && curEnc < 64) {
        encoded += curEnc + 1;
        if(encoded >= last) {
            return -1;
        }
    }

    enclen = (int)(encoded - inencoded);
    if(curEnc >= 64) {
        enclen += 2;
    } else {
        enclen++;
    }

    return enclen;
}

/* Expands DNS name pointed by 'encoded' into 's' 
 *
 * encoded - points to the encoded name in DNS message
 * abuf    - points to the start of DNS message
 * alen    - size of DNS message
 * s       - points to the buffer for expanded name
 * strLen  - size of buffer pointed by s
 *
 * Return value
 *   length of encoded name
 */
int rv_ares_expand_name(const unsigned char *encoded, const unsigned char *abuf,
                     RvSize_t alen, char *s, int strLen)
{
    int len, indir = 0, enclen = -1;
    char *q;
    const unsigned char *p;

    len = rv_ares_name_length(encoded, abuf, alen);
    if (len == -1 || len >= strLen)
        return -1;

    q = s;
    *q = 0; /* In case "encoded" is a NULL string */

    /* No error-checking necessary; it was all done by name_length(). */
    p = encoded;
    while (*p)
    {
        if ((*p & INDIR_MASK) == INDIR_MASK)
        {
            if (!indir)
            {
                enclen = (int)(p + 2 - encoded);
                indir = 1;
            }
            p = abuf + ((*p & ~INDIR_MASK) << 8 | *(p + 1));
        }
        else
        {
            len = *p;
            p++;
            while (len--)
            {
                if (*p == '.' || *p == '\\')
                    *q++ = '\\';
                *q++ = *p;
                p++;
            }
            *q++ = '.';
        }
    }
    if (!indir)
        enclen = (int)(p + 1 - encoded);

    /* Nuke the trailing period if we wrote one. */
    if (q > s)
        *(q - 1) = 0;

    return enclen;
}



/* Retrieve the following NAPTR fields: Flags, Service and Regexp */
int rv_ares_expand_string(const unsigned char *p, char *s, int strLen)
{
    int len, i;
    
    len = *p++;

    if (len >= strLen)
        return -1;

    for (i=0; i < len; ++i)
    {
        s[i] = p[i];
    }

    s[i] = 0;

    return len + 1;
}
#endif /* RV_DNS_ARES */
