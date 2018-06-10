/* rvtlstransport.c - Common Core TLS functionality support module */

/************************************************************************
Copyright (c) 2002 RADVISION Inc.
************************************************************************
NOTICE:
This document contains information that is proprietary to RADVISION LTD.
No part of this publication may be reproduced in any form whatsoever
without written prior approval by RADVISION LTD..

RADVISION LTD. reserves the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
************************************************************************/

#ifndef _rvtlstransport_h_
#define _rvtlstransport_h_


#include "rvmutex.h"
#include "rvtls.h"
#include "rvtransport.h"
#include "rvtransportsocket.h"
#include <string.h>

#if (RV_TLS_TYPE != RV_TLS_NONE)

typedef RvStatus (* RvTlsTransportPostHadshakeVerifyCB)(RvTransport* pTransport);

#define RV_TLS_FINGERPRINT_MAX_LEN              100

struct _RvTlsSockTransportCfg {
    RvTransportSocketCfg                    socketTransportCfg;
    RvTLSSessionCfg                         tlsSessionCfg;
    RvTLSEngine*                            pTlsEngine;
    RvMutex*                                pTlsEngineMtx;
    RvCompareCertificateCB                  onCertCB;
    RvUint32                                options;
    RvHashFuncEnum                          peerCertFingerprintHashFunc;
    RvUint8                                 peerCertFingerprint[RV_TLS_FINGERPRINT_MAX_LEN];
    RvUint                                  peerCertFingerprintLen;
};

#define RV_TLS_SOCK_PENDING_CONNS_MAX 5

extern struct RvTransportInterface gTlsSocketTransportInterface;

RVCOREAPI RvStatus RvTransportCreateTlsSocket(
    IN  RvTlsSockTransportCfg *pCfg,
    OUT RvTransport*    pTransp);

RVCOREAPI RvStatus RvTransportTlsSocketTestRemoteCertifFingerprint(
    IN  RvTransport             transp,
    IN  RvChar*                 fingerprint,
    IN  RvUint                  fingerprintLen,
    IN  RvHashFuncEnum          hash);




#endif /* (RV_TLS_TYPE != RV_TLS_NONE) */

#endif /* _rvtlstransport_h_ */
