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
#include "rvstdio.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* This is an internal function.  Its contract is to read a line from
 * a file into a dynamically allocated buffer, zeroing the trailing
 * newline if there is one.  The calling routine may call
 * rv_ares__read_line multiple times with the same buf and bufsize
 * pointers; *buf will be reallocated and *bufsize adjusted as
 * appropriate.  The initial value of *buf should be NULL.  After the
 * calling routine is done reading lines, it should free *buf.
 */
int rv_ares__read_line(FILE *fp, char **buf, int *bufsize)
{
    char *newbuf;
    int offset = 0, len;
    int s = ARES_SUCCESS;
    char *cur;
    char *lastspace;

    if (*buf == NULL)
    {
        *buf = malloc(128);
        if (!*buf)
            return ARES_ENOMEM;
        *bufsize = 128;
    }

    for (;;)
    {
        if (!fgets(*buf + offset, *bufsize - offset, fp)) {
            s = (offset != 0) ? 0 : (ferror(fp)) ? ARES_EFILE : ARES_EOF;
            if(s == ARES_EFILE) {
                return s;
            }
            break;
        }
        len = offset + (int)(strlen(*buf + offset));
        if ((*buf)[len - 1] == '\n')
        {
            (*buf)[len - 1] = 0;
            break;
        }
        offset = len;

        /* Allocate more space. */
        newbuf = realloc(*buf, *bufsize * 2);
        if (!newbuf) {
            return ARES_ENOMEM;
        }
        *buf = newbuf;
        *bufsize *= 2;
    }

    cur = *buf;
    lastspace = 0;

    while(*cur && *cur != '#') {
        lastspace = isspace((int)*cur) ? cur : 0;
        cur++;
    }

    if(lastspace) {
        *lastspace = 0;
    } else {
        *cur = 0;
    }

    return s;
}

#endif /* RV_DNS_ARES */
