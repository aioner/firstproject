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

#ifdef NEED_WIN32_WRITEV
int writev(int s, IOVEC *iov, int iovcnt)
{
    int nBytesSent;

    if (WSASend(s, iov, iovcnt, (LPDWORD)&nBytesSent, 0, NULL, NULL) == SOCKET_ERROR)
        nBytesSent = -1;

    return nBytesSent;
}
#endif

#endif /* RV_DNS_ARES */
