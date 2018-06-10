/***********************************************************************
Filename   : rvoscomp.c
Description: Complementary OS functions
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

#include "rvccore.h"

#if (RV_DNS_TYPE == RV_DNS_ARES)
#include "ares.h"
#include "rvoscomp.h"

#if !defined(NEED_MANUAL_WRITEV)

#ifdef NEED_WIN32_WRITEV
#include <errno.h>

int writev(int s, IOVEC *iov, int iovcnt)
{
    DWORD nBytesSent = 0;

    if (WSASend(s, iov, iovcnt, &nBytesSent, 0, NULL, NULL) == SOCKET_ERROR) {
        if(WSAGetLastError() != WSAEWOULDBLOCK) {
            return -1;
        }

        _set_errno(EAGAIN);
    }

    return (int)nBytesSent;
}
#else   /* NEED_WIN32_WRITEV */
int prevent_warning_of_ranlib_has_no_symbols_rvoscomp=0;
#endif  /* NEED_WIN32_WRITEV */

#endif /* !defined(NEED_MANUAL_WRITEV) */

#else
int prevent_warning_of_ranlib_has_no_symbols_rvoscomp=0;
#endif /* RV_DNS_ARES */

