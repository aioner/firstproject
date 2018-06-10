/************************************************************************
 File Name     : rvansi.c
 Description   :
************************************************************************
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

#include "rvccore.h"
#include "rvansi.h"


#ifdef RV_ANSI_USECUSTOMSTRINGFORMATTING

/********************************************************************************************
 * RvVsscanf
 * This function implements an ANSI vsscanf style function.
 * INPUT   : s      - The string to which to read from.
 *           format - A sprintf style format string.
 * OUTPUT  : arg    - A variable argument list.
 * RETURN  : The number of characters read, or negative if an error occurred.
 */
static RvInt RvVsscanf(
    IN RvChar* s,
    IN const RvChar* format,
    OUT va_list arg)
{
    return -1;  /* Return an error until implemented */
}


/********************************************************************************************
 * RvVsprintf
 * This function implements an ANSI vsprintf style function.
 * INPUT   : s      - The string to which to write.
 *           format - A sprintf style format string.
 *           arg    - A variable argument list.
 * OUTPUT  : None
 * RETURN  : The number of characters written, or negative if an error occurred.
 */
RVCOREAPI RvInt RVCALLCONV RvVsprintf(
    IN RvChar* s,
    IN const RvChar* format,
    IN va_list arg)
{
    return -1;  /* Return an error until implemented */
}


/********************************************************************************************
 * RvSprintf
 * This function implements an ANSI sprintf style function.
 * INPUT   : s      - The string to which to write.
 *           format - A sprintf style format string.
 *           arg    - A variable argument list.
 * OUTPUT  : None
 * RETURN  : The number of characters written, or negative if an error occurred.
 */
RVCOREAPI RvInt RVCALLCONV RvSprintf(
    IN RvChar* s,
    IN const RvChar* format, ...)
{
    RvInt ret;
    va_list arg;

    va_start(arg, format);
    ret = RvVsprintf(s, format, arg);
    va_end(arg);

    return ret;
}


/********************************************************************************************
 * RvSscanf
 * This function implements an ANSI sprintf style function.
 * INPUT   : s      - The string to which to read.
 *           format - A sprintf style format string.
 *           arg    - A variable argument list.
 * OUTPUT  : None
 * RETURN  : The number input items converted, or EOF if an error occured.
 */
RVCOREAPI RvInt RVCALLCONV RvSscanf(
    IN RvChar* s,
    IN const RvChar* format, ...)
{
    int ret;
    va_list arg;

    va_start(arg, format);
    ret = RvVsscanf(s, format, arg);
    va_end(arg);

    return ret;
}


/********************************************************************************************
 * RvVsnprintf
 * This function implements an ANSI sprintf style function.
 * INPUT   : s      - The string to which to write.
 *           len    - length of the string.
 *           format - A sprintf style format string.
 *           arg    - A variable argument list.
 * OUTPUT  : None
 * RETURN  : The number input items converted, or EOF if an error occured.
 */
RVCOREAPI
RvInt RVCALLCONV RvVsnprintf(
    IN RvChar* s,
    IN RvSize_t len,
    IN const RvChar* format,
    IN va_list arg)
{
    return vsnprintf(s, len, format, arg);
}

#endif /* RV_ANSI_USECUSTOMSTRINGFORMATTING */



#if RV_ANSI_FORCE_OUR || (RV_OS_TYPE == RV_OS_TYPE_VXWORKS) || (RV_OS_TYPE == RV_OS_TYPE_PSOS)  || \
    (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN && RV_TOOL_TYPE == RV_TOOL_TYPE_GNU) || \
    (RV_OS_TYPE == RV_OS_TYPE_TRU64) || (RV_OS_TYPE == RV_OS_TYPE_MOPI)

#if ((RV_OS_TYPE == RV_OS_TYPE_VXWORKS) && (RV_OS_VERSION < RV_OS_VXWORKS_2_2)) || \
	(RV_OS_TYPE == RV_OS_TYPE_PSOS)
#define isc_int64_t             RV_SIGNED_LONG_TYPE
#define isc_uint64_t            RV_UNSIGNED_LONG_TYPE
#define ISC_PRINT_QUADFORMAT    "l"

#else
#define isc_int64_t             RV_SIGNED_INT64_TYPE
#define isc_uint64_t            RV_UNSIGNED_INT64_TYPE

#if (RV_OS_TYPE == RV_OS_TYPE_TRU64)
#define ISC_PRINT_QUADFORMAT
#else
#define ISC_PRINT_QUADFORMAT    "ll"
#endif

#endif

#define INSIST(c)               if (!(c)) return (-1)
#define CROAK(c)                return(-1)
#define REQUIRE                 INSIST

/*
 * Return length of string that would have been written if not truncated.
 */

int RvSnprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = RvVsnprintf(str, size, format, ap);
	va_end(ap);

	return (ret);
}

/*
 * Return length of string that would have been written if not truncated.
 */

/*
* Copyright (C) 2004  Internet Systems Consortium, Inc. ("ISC")
* Copyright (C) 1999-2001, 2003  Internet Software Consortium.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
* REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
* AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
* INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
* LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
* OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
* PERFORMANCE OF THIS SOFTWARE.
*/
int RvVsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    int h;
    int l;
    int q;
    int alt;
    int zero;
    int left;
    int plus;
    int space;
    int neg;
    isc_int64_t tmpi;
    isc_uint64_t tmpui;
    unsigned long width;
    unsigned long precision;
    unsigned int length;
    char buf[1024];
    char c;
    void *v;
    char *save = str;
    const char *cp;
    const char *head;
    int count = 0;
    int pad;
    int zeropad;
    int dot;
    double dbl;
#ifdef HAVE_LONG_DOUBLE
    long double ldbl;
#endif
    char fmt[32];

    INSIST(str != NULL);
    INSIST(format != NULL);

    while (*format != '\0') {
        if (*format != '%') {
            if (size > 1) {
                *str++ = *format;
                size--;
            }
            count++;
            format++;
            continue;
        }
        format++;

        /*
         * Reset flags.
         */
        dot = neg = space = plus = left = zero = alt = h = l = q = 0;
        width = precision = 0;
        head = "";
        length = pad = zeropad = 0;

        for(;;) {
            if (*format == '#') {
                alt = 1;
                format++;
            } else if (*format == '-') {
                left = 1;
                zero = 0;
                format++;
            } else if (*format == ' ') {
                if (!plus)
                    space = 1;
                format++;
            } else if (*format == '+') {
                plus = 1;
                space = 0;
                format++;
            } else if (*format == '0') {
                if (!left)
                    zero = 1;
                format++;
            } else
                break;
        } 

        /*
         * Width.
         */
        if (*format == '*') {
            width = va_arg(ap, int);
            format++;
        } else if (isdigit((unsigned char)*format)) {
            char *e;
            width = strtoul(format, &e, 10);
            format = e;
        }

        /*
         * Precision.
         */
        if (*format == '.') {
            format++;
            dot = 1;
            if (*format == '*') {
                precision = va_arg(ap, int);
                format++;
            } else if (isdigit((unsigned char)*format)) {
                char *e;
                precision = strtoul(format, &e, 10);
                format = e;
            }
        }

        switch (*format) {
        case '\0':
            continue;
        case '%':
            if (size > 1) {
                *str++ = *format;
                size--;
            }
            format++;
            count++;
            break;
        case 'q':
            q = 1;
            format++;
            goto doint;
        case 'h':
            h = 1;
            format++;
            goto doint;
        case 'l':
            l = 1;
            format++;
            if (*format == 'l') {
                q = 1;
                format++;
            }
            goto doint;
        case 'n':
        case 'i':
        case 'd':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
        doint:
            if (precision != 0)
                zero = 0;
            switch (*format) {
            case 'n':
                if (h) {
                    short int *p;
                    p = va_arg(ap, short *);
                    REQUIRE(p != NULL);
                    *p = (short int)(str - save);
                } else if (l) {
                    long int *p;
                    p = va_arg(ap, long *);
                    REQUIRE(p != NULL);
                    *p = str - save;
                } else {
                    int *p;
                    p = va_arg(ap, int *);
                    REQUIRE(p != NULL);
                    *p = str - save;
                }
                break;
            case 'i':
            case 'd':
                if (q)
                    tmpi = va_arg(ap, isc_int64_t);
                else if (l)
                    tmpi = va_arg(ap, long int);
                else
                    tmpi = va_arg(ap, int);
                if (tmpi < 0) {
                    head = "-";
                    tmpui = -tmpi;
                } else {
                    if (plus)
                        head = "+";
                    else if (space)
                        head = " ";
                    else
                        head = "";
                    tmpui = tmpi;
                }
                sprintf(buf, "%" ISC_PRINT_QUADFORMAT "u",
                    tmpui);
                goto printint;
            case 'o':
                if (q)
                    tmpui = va_arg(ap, isc_uint64_t);
                else if (l)
                    tmpui = va_arg(ap, long int);
                else
                    tmpui = va_arg(ap, int);
                sprintf(buf,
                    alt ? "%#" ISC_PRINT_QUADFORMAT "o"
                        : "%" ISC_PRINT_QUADFORMAT "o",
                    tmpui);
                goto printint;
            case 'u':
                if (q)
                    tmpui = va_arg(ap, isc_uint64_t);
                else if (l)
                    tmpui = va_arg(ap, unsigned long int);
                else
                    tmpui = va_arg(ap, unsigned int);
                sprintf(buf, "%" ISC_PRINT_QUADFORMAT "u",
                    tmpui);
                goto printint;
            case 'x':
                if (q)
                    tmpui = va_arg(ap, isc_uint64_t);
                else if (l)
                    tmpui = va_arg(ap, unsigned long int);
                else
                    tmpui = va_arg(ap, unsigned int);
                if (alt) {
                    head = "0x";
                    if (precision > 2)
                        precision -= 2;
                }
                sprintf(buf, "%" ISC_PRINT_QUADFORMAT "x",
                    tmpui);
                goto printint;
            case 'X':
                if (q)
                    tmpui = va_arg(ap, isc_uint64_t);
                else if (l)
                    tmpui = va_arg(ap, unsigned long int);
                else
                    tmpui = va_arg(ap, unsigned int);
                if (alt) {
                    head = "0X";
                    if (precision > 2)
                        precision -= 2;
                }
                sprintf(buf, "%" ISC_PRINT_QUADFORMAT "X",
                    tmpui);
                goto printint;
            printint:
                if (precision != 0 || width != 0) {
                    length = strlen(buf);
                    if (length < precision)
                        zeropad = precision - length;
                    else if (length < width && zero)
                        zeropad = width - length;
                    if (width != 0) {
                        pad = width - length -
                              zeropad - strlen(head);
                        if (pad < 0)
                            pad = 0;
                    }
                }
                count += strlen(head) + strlen(buf) + pad +
                     zeropad;
                if (!left) {
                    while (pad > 0 && size > 1) {
                        *str++ = ' ';
                        size--;
                        pad--;
                    }
                }
                cp = head;
                while (*cp != '\0' && size > 1) {
                    *str++ = *cp++;
                    size--;
                }
                while (zeropad > 0 && size > 1) {
                    *str++ = '0';
                    size--;
                    zeropad--;
                }
                cp = buf;
                while (*cp != '\0' && size > 1) {
                    *str++ = *cp++;
                    size--;
                }
                while (pad > 0 && size > 1) {
                    *str++ = ' ';
                    size--;
                    pad--;
                }
                break;
            default:
                break;
            }
            break;
        case 's':
            cp = va_arg(ap, char *);
            REQUIRE(cp != NULL);

            if (precision != 0) {
                /*
                 * cp need not be NULL terminated.
                 */
                const char *tp;
                unsigned long n;

                n = precision;
                tp = cp;
                while (n != 0 && *tp != '0')
                    n--, tp++;
                length = precision - n;
            } else {
                length = strlen(cp);
            }
            if (width != 0) {
                pad = width - length;
                if (pad < 0)
                    pad = 0;
            }
            count += pad + length;
            if (!left)
                while (pad > 0 && size > 1) {
                    *str++ = ' ';
                    size--;
                    pad--;
                }
            if (precision != 0)
                while (precision > 0 && *cp != '\0' &&
                       size > 1) {
                    *str++ = *cp++;
                    size--;
                    precision--;
                }
            else
                while (*cp != '\0' && size > 1) {
                    *str++ = *cp++;
                    size--;
                }
            while (pad > 0 && size > 1) {
                *str++ = ' ';
                size--;
                pad--;
            }
            break;
        case 'c':
            c = (char)va_arg(ap, int);
            if (width > 0) {
                count += width;
                width--;
                if (left) {
                    *str++ = c;
                    size--;
                }
                while (width-- > 0 && size > 1) {
                    *str++ = ' ';
                    size--;
                }
                if (!left && size > 1) {
                    *str++ = c;
                    size--;
                }
            } else {
                count++;
                if (size > 1) {
                    *str++ = c;
                    size--;
                }
            }
            break;
        case 'p':
            v = va_arg(ap, void *);
            sprintf(buf, "%p", v);
            length = strlen(buf);
            if (precision > length)
                zeropad = precision - length;
            if (width > 0) {
                pad = width - length - zeropad;
                if (pad < 0)
                    pad = 0;
            }
            count += length + pad + zeropad;
            if (!left)
                while (pad > 0 && size > 1) {
                    *str++ = ' ';
                    size--;
                    pad--;
                }
            cp = buf;
            if (zeropad > 0 && buf[0] == '0' &&
                (buf[1] == 'x' || buf[1] == 'X')) {
                if (size > 1) {
                    *str++ = *cp++;
                    size--;
                }
                if (size > 1) {
                    *str++ = *cp++;
                    size--;
                }
                while (zeropad > 0 && size > 1) {
                    *str++ = '0';
                    size--;
                    zeropad--;
                }
            }
            while (*cp != '\0' && size > 1) {
                *str++ = *cp++;
                size--;
            }
            while (pad > 0 && size > 1) {
                *str++ = ' ';
                size--;
                pad--;
            }
            break;
        case 'D':   /*deprecated*/
            CROAK("use %ld instead of %D");
        case 'O':   /*deprecated*/
            CROAK("use %lo instead of %O");
        case 'U':   /*deprecated*/
            CROAK("use %lu instead of %U");

        case 'L':
#ifdef HAVE_LONG_DOUBLE
            l = 1;
#else
            CROAK("long doubles are not supported");
#endif
            /*FALLTHROUGH*/
        case 'e':
        case 'E':
        case 'f':
        case 'g':
        case 'G':
            if (!dot)
                precision = 6;
            /*
             * IEEE floating point.
             * MIN 2.2250738585072014E-308
             * MAX 1.7976931348623157E+308
             * VAX floating point has a smaller range than IEEE.
             *
             * precisions > 324 don't make much sense.
             * if we cap the precision at 512 we will not
             * overflow buf.
             */
            if (precision > 512)
                precision = 512;
            sprintf(fmt, "%%%s%s.%lu%s%c", alt ? "#" : "",
                plus ? "+" : space ? " " : "",
                precision, l ? "L" : "", *format);
            switch (*format) {
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
#ifdef HAVE_LONG_DOUBLE
                if (l) {
                    ldbl = va_arg(ap, long double);
                    sprintf(buf, fmt, ldbl);
                } else
#endif
                {
                    dbl = va_arg(ap, double);
                    sprintf(buf, fmt, dbl);
                }
                length = strlen(buf);
                if (width > 0) {
                    pad = width - length;
                    if (pad < 0)
                        pad = 0;
                }
                count += length + pad;
                if (!left)
                    while (pad > 0 && size > 1) {
                        *str++ = ' ';
                        size--;
                        pad--;
                    }
                cp = buf;
                while (*cp != ' ' && size > 1) {
                    *str++ = *cp++;
                    size--;
                }
                while (pad > 0 && size > 1) {
                    *str++ = ' ';
                    size--;
                    pad--;
                }
                break;
            default:
                continue;
            }
            break;
        default:
            continue;
        }
        format++;
    }
    if (size > 0)
        *str = '\0';
    return (count);
}


#else
int prevent_warning_of_ranlib_has_no_symbols_rvansi=0;
#endif /* (RV_OS_TYPE == RV_OS_TYPE_VXWORKS) */

