/* rvtls.c - Common Core TLS functionality support module */

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
#include "rvmutex.h"
#include "rvthread.h"
#include "rvtls.h"
#include "rvnettypes.h"
#include "rvstrutils.h"
#include <string.h>
#include <ctype.h>

#if (RV_TLS_TYPE != RV_TLS_NONE)
#if (OPENSSL_VERSION_NUMBER >= 0x0090800fL)
typedef const unsigned char** d2iUCharPP;
#else
typedef unsigned char** d2iUCharPP;
#endif

/* Defines TLS error code to simplify TLS module functions */
#define RvTLSErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_TLS, (_e))

/* Defines macro to retrive TLS log source pointer from the log manager */
#define RvTLSLogSrc(_m) (_m==NULL?NULL:(&(_m->TLSSource)))

/* Maximal number of subsequent SSL_writes caused by single select WRITE event */
#define MAX_SUBSEQUENT_WRITES 5

#if 0
/* bio_err defined by OpenSSL and used as destination of error messages */
extern BIO              *bio_err = NULL;
#endif

/* list of mutexes for OpenSSL library threadsafeness */
static RvMutex          *openSSLMutexes = NULL;

/* Static functions definitions */

static void rvTLSGetError(
    IN  RvTLSSession    *tlsSession,
    IN  RvInt           sslRetCode,
    IN  RvMutex         *mtx,
    OUT RvInt           *errId);

static void rvLockingCallback(
    IN RvInt        mode,
    IN RvInt        type,
    IN RvChar       *file,
    IN RvInt        line);

static RvBool rvIsWriteEnabled(
    IN  RvTLSSession* tlsSession,
    OUT RvTLSEvents*  pPendingTlsEvents,
    OUT RvChar**      pStrWriteBlockRsn);

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
/* The following functions are not members of the Nucleus libraries.
 * OpenSSL services that call these functions are not called by us anyway...
 */
void opendir()  {}
void closedir() {}
void readdir()  {}
void sigaction(){}
void __open()   {}
void __lseek()  {}


time_t time(time_t *t)
{
    RvInt32 seconds;

    seconds = RvClockGet(NULL, NULL);

    if (t != NULL)
        *t = seconds;

    return seconds;
}
#endif  /* RV_OS_TYPE == RV_OS_TYPE_NUCLEUS */

#endif  /* RV_TLS_TYPE != RV_TLS_NONE */

#if (RV_TLS_TYPE != RV_TLS_NONE)

#if (RV_USE_RV_BIO == RV_YES)

static int rv_sock_write(BIO *b, const char *in, int inl)
{
    RvStatus ret;
    RvSize_t sentBytes;

    ret = RvSocketSendBuffer((RvSocket*)&b->num,(RvUint8*)in,inl,NULL,(RvLogMgr*)(b->ptr),&sentBytes);
    if (ret != RV_OK)
        return -1;
    if (sentBytes == 0)
    {
        BIO_set_retry_write(b);
        return -1;
    }
    return (int)(sentBytes);
}

static int rv_sock_read(BIO *b, char *out, int outl)
{
    RvStatus ret;
    RvSize_t readBytes;
    if (!out)
        return 0;

    ret = RvSocketReceiveBuffer((RvSocket*)&b->num,(RvUint8*)out,outl,(RvLogMgr*)b->ptr,&readBytes,NULL);
    if (ret != RV_OK)
        return -1;
    if (readBytes == 0)
    {
        BIO_set_retry_read(b);
        return -1;
    }
    return (int)(readBytes);
}

static int rv_sock_new(BIO *bi)
{
    bi->init=0;
    bi->num=0;
    bi->ptr=NULL;
    bi->flags=0;
    return(1);
}

static long rv_sock_ctrl(BIO *b, int cmd, long num, void *ptr)
{
    int ret;
    RV_UNUSED_ARG(b);
    RV_UNUSED_ARG(num);
    RV_UNUSED_ARG(ptr);

    switch (cmd)
    {
    case BIO_CTRL_FLUSH:
		ret=1;
        break;
    default:
        ret=0;
    }

    return ret;
}


static BIO_METHOD bio_rv_method =
{
        157645765,/*BIO_TYPE_SOCKET,*/ /* not important, we are not going to chain it */
        "rvsockets",
        rv_sock_write,
        rv_sock_read,
        NULL,
        NULL, /* sock_gets, */
        rv_sock_ctrl,
        rv_sock_new,
        NULL,
        NULL,
};

BIO *BIO_new_rvsocket(RvSocket sock, RvLogMgr *logMgr)
{
    BIO *ret;

    ret=BIO_new(&bio_rv_method);
    if (ret == NULL) return(NULL);
    ret->num = (int) sock;
    ret->ptr = (void*) logMgr;
    ret->init=1;
    return(ret);
}


#endif /* RV_USE_RV_BIO */

#endif /* (RV_TLS_TYPE != RV_TLS_NONE) */

/**********************************************************************************
 * RvTLSSourceConstruct - initiates TLS module log source
 *
 * This function should be called only once in specific process. It initiates
 * OpenSSL library.
 *
 * INPUT : none
 * OUTPUT: none
 * RETURN: RvStatus - Success or failure
 */
RvStatus RvTLSSourceConstruct(
    IN RvLogMgr* logMgr)
{
    RvStatus result = RV_OK;

#if (RV_TLS_TYPE != RV_TLS_NONE)
    result = RvLogSourceConstruct(logMgr, RvTLSLogSrc(logMgr), "TLS", "TLS/SSL module");
#else
    RV_UNUSED_ARG(logMgr);
#endif

    return result;
}


/**********************************************************************************
 * RvTLSInit - initiates OpenSSL library
 *
 * This function should be called only once in specific process. It initiates
 * OpenSSL library.
 *
 * INPUT : none
 * OUTPUT: none
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSInit(void)
{
#if (RV_TLS_TYPE != RV_TLS_NONE)
    int  mutexCnt;

    if (!SSL_library_init())
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);

    SSL_load_error_strings();

#if 0
    if (bio_err == NULL)
        bio_err = BIO_new_fp(stderr,BIO_NOCLOSE);
#endif

    /* Allocate mutexes for OpenSSL use */
    openSSLMutexes = (RvMutex *)OPENSSL_malloc(sizeof(RvMutex) * CRYPTO_num_locks());
    for (mutexCnt = 0; mutexCnt < CRYPTO_num_locks(); mutexCnt++) {
        RvMutexConstruct(NULL,&(openSSLMutexes[mutexCnt]));
    }

#if (RV_TLS_AUTO_SET_OS_CALLBACKS == RV_YES)
    /* define callback that provides OpenSSL with current thread ID */
    CRYPTO_set_id_callback((unsigned long (*)(void))RvThreadCurrentId);
    /* define lock callback for OpenSSL */
    CRYPTO_set_locking_callback((void (*)(int,int,const char *,int))rvLockingCallback);
#endif /* (RV_TLS_AUTO_SET_OS_CALLBACKS == RV_YES) */


#endif

    return RV_OK;
}

/**********************************************************************************
 * RvTLSEnd - finalize OpenSSL library usage
 *
 * This function should be called only once in specific process.
 *
 * INPUT : none
 * OUTPUT: none
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSEnd(void)
{
#if (RV_TLS_TYPE != RV_TLS_NONE)
    RvInt mutexCnt;

    if (openSSLMutexes == NULL)
        return RV_OK;

#if (RV_TLS_AUTO_SET_OS_CALLBACKS == RV_YES)
    /* remove the previously set callbacks */
    CRYPTO_set_id_callback(NULL);
    /* define lock callback for OpenSSL */
    CRYPTO_set_locking_callback(NULL);
#endif /* (RV_TLS_AUTO_SET_OS_CALLBACKS == RV_YES) */


    /* destroy OpenSSL mutexes */
    for (mutexCnt = 0; mutexCnt < CRYPTO_num_locks(); mutexCnt++) {
        RvMutexDestruct(&(openSSLMutexes[mutexCnt]),NULL);
    }

    /* Free OpenSSL mutexes memory */
    OPENSSL_free(openSSLMutexes);
    openSSLMutexes = NULL;
#endif

    return RV_OK;
}


#if (RV_TLS_TYPE != RV_TLS_NONE)

#if (RV_TLS_AUTO_SET_OS_CALLBACKS == RV_NO)

/**********************************************************************************
 * RvTLSSetLockingCallback - sets/unsets TLS locking callback to RVSN function.
 *
 * INPUT:
 *	set - if RV_TRUE the callback will be set to RadVision locking callback otherwise
 *        the callback will be removed.
 * RETURN:
 *	None.
 *
 */
RVCOREAPI
void RvTLSSetLockingCallback(RvBool set)
{
    CRYPTO_set_locking_callback((set) ? (void (*)(int,int,const char *,int))rvLockingCallback : NULL);
}

/**********************************************************************************
 * RvTLSSetThreadIdCallback - sets/unsets TLS thread ID callback to RVSN function.
 *
 * INPUT:
 *	set - if RV_TRUE the callback will be set to RadVision thread id callback otherwise
 *        the callback will be removed.
 * RETURN:
 *	None.
 *
 */
RVCOREAPI
void RvTLSSetThreadIdCallback(RvBool set)
{
    CRYPTO_set_id_callback((set) ? (unsigned long (*)(void))RvThreadCurrentId : NULL);
}


#endif /*(RV_TLS_AUTO_SET_OS_CALLBACKS == RV_YES)*/


/**********************************************************************************
 * RvTLSEngineConstruct - constructs TLS engine
 *
 * Constructs SSL context according to the input parameters and TLS module log source.
 * It fills output TLS engine structure with results.
 *
 * INPUT:
 *  method      - SSL version. Can be SSL v1, SSL v2 or TLS v1
 *  logMgr      - log instance
 *  privKey     - private key
 *  privKeyType - private key type
 *  privKeyLen  - private key length
 *  cert        - local side certificate
 *  certLen     - local side certificate length
 *  certDepth   - maximum allowed depth of certificates
 *  mtx         - mutex that protects this TLS
 *                engine memory
 * OUTPUT: tlsEngine    - TLS engine memory
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSEngineConstruct(
    IN  RvTLSMethod             method,
    IN  RvChar                  *privKey,
    IN  RvPrivKeyType           privKeyType,
    IN  RvInt                   privKeyLen,
    IN  RvChar                  *cert,
    IN  RvInt                   certLen,
    IN  RvInt                   certDepth,
    IN  RvMutex                 *mtx,
    IN  RvLogMgr                *logMgr,
    OUT RvTLSEngine             *tlsEngine)
{
    const SSL_METHOD  *meth;
    RvInt       sslRet;
    /*RvStatus    stat = RV_OK;*/


#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if (tlsEngine == NULL) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif


    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineConstruct(method=%d,privKey=%p,keyType=%d,keyLen=%d,cert=%p,certLen=%d,certDepth=%d,mtx=%p,logMgr=%p,tlsEng=%p)",
        method,privKey,privKeyType,privKeyLen,cert,certLen,certDepth,mtx,logMgr,tlsEngine));


/*  No need to lock here;
    stat = RvMutexLock(mtx,logMgr);
    if (stat != RV_OK) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSEngineConstruct: engine (%p) mutex lock error",
            tlsEngine));
        return stat;
    }
*/

    /* Initiate SSL context according to SSL version */

    switch(method) {
    case RV_TLS_SSL_V2:
        meth = SSLv2_method();
        break;
    case RV_TLS_SSL_V3:
        meth = SSLv23_method();
        break;
    case RV_TLS_TLS_V1:
        meth = TLSv1_method();
        break;
    default:
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSEngineConstruct: engine (%p) invalid method (%d)",
            tlsEngine,method));
        /*RvMutexUnlock(mtx,logMgr);*/
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }

    /* Initiate OpenSSL context */
    tlsEngine->ctx = SSL_CTX_new((SSL_METHOD*)meth);

    if (tlsEngine->ctx == NULL) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSEngineConstruct: engine (%p) failed to create OpenSSL context",
            tlsEngine));
        /*RvMutexUnlock(mtx,logMgr);*/
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    if ((privKey != NULL) && (privKeyLen > 0)) {
      switch(privKeyType) {
      case RV_TLS_RSA_KEY:
        sslRet = SSL_CTX_use_PrivateKey_ASN1(EVP_PKEY_RSA,tlsEngine->ctx,
                         (unsigned char *)privKey, privKeyLen);
        if (!sslRet) {
          RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                           "RvTLSEngineConstruct: engine (%p) failed to load private key",
                           tlsEngine));
          SSL_CTX_free(tlsEngine->ctx);
          tlsEngine->ctx = NULL;
          /*RvMutexUnlock(mtx,logMgr);*/
          return RvTLSErrorCode(RV_TLS_ERROR_GEN);
        }
        break;
      default:
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineConstruct: engine (%p) wrong private key type (%d)",
                         tlsEngine,privKeyType));
        SSL_CTX_free(tlsEngine->ctx);
        tlsEngine->ctx = NULL;
        /*RvMutexUnlock(mtx,logMgr);*/
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
      }
    }

    /* Set maximum certificate chain depth */
    if ((certDepth != RV_TLS_DEFAULT_CERT_DEPTH) && (certDepth >= 0)) {
        SSL_CTX_set_verify_depth(tlsEngine->ctx,certDepth);
    }

    if ((cert != NULL) && (certLen > 0)) {
      sslRet = SSL_CTX_use_certificate_ASN1(tlsEngine->ctx,certLen,(unsigned char *)cert);
      if (!sslRet) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineConstruct: engine (%p) failed to load local certificate",
                         tlsEngine));
        /*RvMutexUnlock(mtx,logMgr);*/
        SSL_CTX_free(tlsEngine->ctx);
        tlsEngine->ctx = NULL;
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
      }
    }

    if (mtx == NULL)
    {
        RvStatus retv;
        /* an application has not provided us with the mutex, we will create the one ourselves */
        retv  = RvMutexConstruct(logMgr,&tlsEngine->mtx);
        if (retv != RV_OK) {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                "RvTLSEngineConstruct:failed in RvMutexConstruct",tlsEngine));
            SSL_CTX_free(tlsEngine->ctx);
            tlsEngine->ctx = NULL;
            return RvTLSErrorCode(RV_TLS_ERROR_GEN);
        }
        mtx = &tlsEngine->mtx;
    }
    tlsEngine->pMtx = mtx;

    SSL_CTX_set_verify(tlsEngine->ctx, SSL_VERIFY_NONE, NULL);
    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineConstruct(method=%d,privKey=%p,keyType=%d,keyLen=%d,cert=%p,certLen=%d,certDepth=%d,mtx=%p,logMgr=%p,tlsEng=%p)=%d",
        method,privKey,privKeyType,privKeyLen,cert,certLen,certDepth,mtx,logMgr,tlsEngine,RV_OK));

    /*RvMutexUnlock(mtx,logMgr);*/

    return RV_OK;
}


/**********************************************************************************
 * RvTLSEngineAddAutorityCertificate - adds certificate authority to the engine
 *
 * INPUT:
 *  tlsEngine   - TLS engine where certificate will be added
 *  cert        - CA certificate
 *  certLen     - CA certificate length
 *  mtx         - mutex that protects this TLS
 *                engine memory
 *  logMgr      - log instance
 * OUTPUT: none
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSEngineAddAutorityCertificate(
    IN  RvTLSEngine             *tlsEngine,
    IN  RvChar                  *cert,
    IN  RvInt                   certLen,
    IN  RvMutex                 *mtx,
    IN  RvLogMgr                *logMgr)
{

    X509        *xCert = NULL;
    RvStatus    retCode = RV_OK;

    if (mtx == NULL && tlsEngine != NULL)
        /* if mutex was not provided, use the one provided when the TLS engine was constructed */
        mtx = tlsEngine->pMtx;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((tlsEngine == NULL) || (mtx == NULL) ||
        (cert == NULL) || (certLen <= 0)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineAddAutorityCertificate(tlsEng=%p,cert=%p,certLen=%d,mtx=%p,logMgr=%p)",
        tlsEngine,cert,certLen,mtx,logMgr));

    if (d2i_X509(&xCert,(d2iUCharPP)&cert,certLen) == NULL) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineAddAutorityCertificate: engine (%p), failed to convert ASN1 certificate to X509 format",
                         tlsEngine));
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    RvMutexLock(mtx,logMgr);

    if (X509_STORE_add_cert(SSL_CTX_get_cert_store(tlsEngine->ctx), xCert) != 1) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineAddAutorityCertificate: engine (%p), failed to convert add X509 certificate to the engine",
                         tlsEngine));
        retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
        RvMutexUnlock(mtx,logMgr);

        if(xCert) {
            X509_free(xCert);
        }

        return retCode;
    }

    RvMutexUnlock(mtx,logMgr);

    if(xCert) {
        X509_free(xCert);
    }


    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineAddAutorityCertificate(tlsEng=%p,cert=%p,certLen=%d,mtx=%p,logMgr=%p)=%d",
        tlsEngine,cert,certLen,mtx,logMgr,retCode));

    return retCode;
}

/**********************************************************************************
 * RvTLSEngineExpose - exposes underlying engine's SSL_CTX*
 *
 * INPUT:
 *  tlsEngine   - TLS engine where certificate will be added
 *  mtx         - mutex that protects this TLS
 *                engine memory
 *  logMgr      - log instance
 * OUTPUT:
 *  underlying  - underlyingctx engine's underlying SSL_CTX* pointer
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSEngineExpose(
    IN  RvTLSEngine             *tlsEngine,
    IN  RvMutex                 *mtx,
    IN  RvLogMgr                *logMgr,
    OUT SSL_CTX                **underlying)
{

    if (mtx == NULL && tlsEngine != NULL)
        /* if mutex was not provided, use the one provided when the TLS engine was constructed */
        mtx = tlsEngine->pMtx;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((tlsEngine == NULL) || (mtx == NULL) ||
        (underlying == NULL) ) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineExpose(tlsEng=%p,mtx=%p,logMgr=%p,underlying=%p)",
        tlsEngine,mtx,logMgr,underlying));


    RvMutexLock(mtx,logMgr);
    *underlying = tlsEngine->ctx;
    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineExpose(tlsEng=%p,mtx=%p,logMgr=%p,*underlying=%p)",
        tlsEngine,mtx,logMgr,*underlying));

    return RV_OK;
}

/**********************************************************************************
 * RvTLSEngineAddCertificate - adds certificate to the engine certificate chain
 *
 * INPUT:
 *  tlsEngine   - TLS engine where certificate will be added
 *  cert        - local side certificate
 *  certLen     - local side certificate length
 *  mtx         - mutex that protects this TLS
 *                engine memory
 *  logMgr      - log instance
 * OUTPUT: none
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSEngineAddCertificate(
    IN  RvTLSEngine             *tlsEngine,
    IN  RvChar                  *cert,
    IN  RvInt                   certLen,
    IN  RvMutex                 *mtx,
    IN  RvLogMgr                *logMgr)
{
    X509        *xCert = NULL;
    RvStatus    retCode = RV_OK;

    if (mtx == NULL && tlsEngine != NULL)
        /* if mutex was not provided, use the one provided when the TLS engine was constructed */
        mtx = tlsEngine->pMtx;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((tlsEngine == NULL) || (mtx == NULL) ||
        (cert == NULL) || (certLen <= 0)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineAddCertificate(tlsEng=%p,cert=%p,certLen=%d,mtx=%p,logMgr=%p)",
        tlsEngine,cert,certLen,mtx,logMgr));

    if (d2i_X509(&xCert,(d2iUCharPP)&cert,certLen) == NULL) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineAddCertificate: engine (%p), failed to convert ASN1 certificate to X509 format",
                         tlsEngine));
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }


    RvMutexLock(mtx,logMgr);
    if (SSL_CTX_add_extra_chain_cert(tlsEngine->ctx,xCert) != 1) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineAddCertificate: engine (%p), failed to add X509 certificate to the chain",
                         tlsEngine));
        retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
        RvMutexUnlock(mtx,logMgr);
        return retCode;
    }

    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineAddCertificate(tlsEng=%p,cert=%p,certLen=%d,mtx=%p,logMgr=%p)=%d",
        tlsEngine,cert,certLen,mtx,logMgr,retCode));

    return retCode;
}


/**********************************************************************************
 * RvTLSEngineDestruct - destructs TLS engine
 *
 * Destructs SSL context.
 *
 * INPUT:
 *  tlsEngine   - engine to be destructed
 *  mtx         - mutex that protects this TLS
 *                engine memory
 *  logMgr      - log instance
 * OUTPUT: none
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSEngineDestruct(
    IN RvTLSEngine  *tlsEngine,
    IN RvMutex      *mtx,
    IN RvLogMgr     *logMgr)
{
    if (mtx == NULL && tlsEngine != NULL)
        /* if mutex was not provided, use the one provided when the TLS engine was constructed */
        mtx = tlsEngine->pMtx;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((tlsEngine == NULL) || (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineDestruct(tlsEng=%p,mtx=%p,logMgr=%p)",
        tlsEngine,mtx,logMgr));

    /*RvMutexLock(mtx,logMgr);*/

    if (tlsEngine->ctx == NULL) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineDestruct: engine (%p), failed to destruct, wrong OpenSSL context",
                         tlsEngine));
        /*RvMutexUnlock(mtx,logMgr);*/
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }

    SSL_CTX_flush_sessions(tlsEngine->ctx,0);
    SSL_CTX_free(tlsEngine->ctx);

    /*RvMutexUnlock(mtx,logMgr);*/

    if (tlsEngine->pMtx == &tlsEngine->mtx)
        /* if this is the mutex constructed by us, we have to destruct it as well */
        RvMutexDestruct(tlsEngine->pMtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineDestruct(tlsEng=%p,mtx=%p,logMgr=%p)=%d",
        tlsEngine,mtx,logMgr,RV_OK));

    return RV_OK;
}

/**********************************************************************************
 * RvTLSSessionConstruct - Initiates TLS/SSL session
 *
 * Creates uninitialized SSL/TLS session that matches context defined by
 * input TLS engine.
 *
 * INPUT:
 *  tlsEngine   - TLS engine
 *  mtxEngine   - mutex that protects TLS
 *                engine memory
 *  mtxSession  - mutex that protects Session
 *                engine memory
 *  logMgr      - log instance
 * OUTPUT: tlsSession   - new TLS session
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSSessionConstruct(
    IN  RvTLSEngine     *tlsEngine,
    IN  RvMutex         *mtxEngine,
    IN  RvMutex         *mtxSession,
    IN  RvLogMgr        *logMgr,
    OUT RvTLSSession    *tlsSession)
{
    RvTLSSessionCfg cfg;
    cfg.bDisablePartialSend = RV_TRUE;

    return RvTLSSessionConstructEx(tlsEngine, mtxEngine, mtxSession, &cfg,
                                   logMgr, tlsSession);
}

/**********************************************************************************
 * RvTLSSessionConstructEx - Initiates TLS/SSL session
 *
 * Creates uninitialized SSL/TLS session that matches context defined by
 * input TLS engine.
 *
 * INPUT:
 *	tlsEngine	- TLS engine
 *  mtxEngine	- mutex that protects TLS
 *				  engine memory
 *  mtxSession	- mutex that protects Session
 *				  engine memory
 *  cfg         - configuration of session.
 *                If NULL, default configuration will be used.
 *  logMgr		- log instance
 * OUTPUT: tlsSession	- new TLS session
 * RETURN: RvStatus	- Success or failure
 */
RVCOREAPI
RvStatus RvTLSSessionConstructEx(
	IN  RvTLSEngine		*tlsEngine,
	IN  RvMutex			*mtxEngine,
	IN  RvMutex			*mtxSession,
	IN  RvTLSSessionCfg *cfg,
	IN  RvLogMgr		*logMgr,
	OUT RvTLSSession	*tlsSession)
{
    RvStatus stat;
    SSL      *sslSession;

    if (mtxEngine == NULL && tlsEngine != NULL)
        /* if mutex was not provided, use the one provided when the TLS engine was constructed */
        mtxEngine = tlsEngine->pMtx;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input */
    if ((tlsEngine == NULL) || (tlsSession == NULL) ||
        (mtxEngine == NULL) || (mtxSession == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionConstructEx(tlsEng=%p,mtxEng=%p,mtxSession=%p,logMgr=%p,tlsSession=%p)",
        tlsEngine,mtxEngine,mtxSession,logMgr,tlsSession));

    stat = RvMutexLock(mtxEngine,logMgr);
    if (stat != RV_OK) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionConstructEx: engine (%p), session (%p) engine lock failure",
            tlsEngine,tlsSession));
        return stat;
    }

    sslSession  = SSL_new(tlsEngine->ctx);
    if (sslSession == NULL) {
	    RvChar          *file;
	    RvChar          *err;
	    RvInt           line;
	    RvInt           errFlags;

		if (ERR_get_error_line_data((const char **)&file,
					      &line, (const char **)&err,
					      &errFlags) != 0) {
                         RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
						     "RvTLSSessionConstructEx: engine (%p) session (%p) , general error - %s, file - %s, line - %d",
						     tlsEngine,tlsSession,err,file,line));
		}
		else {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
						     "RvTLSSessionConstructEx: engine (%p) session (%p) general eError",
						     tlsEngine,tlsSession));
        }
        RvMutexUnlock(mtxEngine,logMgr);
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    RvMutexUnlock(mtxEngine,logMgr);

    if (cfg==NULL || cfg->bDisablePartialSend==RV_FALSE)
    {
        SSL_set_mode(sslSession,
                     (SSL_MODE_ENABLE_PARTIAL_WRITE |
                      SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER));
        RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionConstructEx(tlsEng=%p,mtxEng=%p,mtxSession=%p,logMgr=%p,tlsSession=%p)=0 - Partial Send was enabled",
            tlsEngine,mtxEngine,mtxSession,logMgr,tlsSession));
    }

    stat = RvMutexLock(mtxSession,logMgr);
    if (stat != RV_OK) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionConstructEx: engine (%p), session (%p) session lock failure",
            tlsEngine,tlsSession));
        return stat;
    }

    tlsSession->requiredForHandshake     = 0;
    tlsSession->requiredForTLSRead       = 0;
    tlsSession->requiredForTLSWrite      = 0;
    tlsSession->requiredForTLSShutdown   = 0;
    tlsSession->pendingTlsEvents         = 0;
    tlsSession->bio = NULL;     /* important since when applying handshake
                                this parameter specifies if the handshake
                                was applied for the first time or as a result of
                                previous 'will block' failure */
    tlsSession->sock = 0;
    tlsSession->sslSession = sslSession;
	tlsSession->renegState = RV_TLS_RENEG_NONE;
	tlsSession->bWaitingShutdownFinish = RV_FALSE;
	tlsSession->counterWrites = 0;

    RvMutexUnlock(mtxSession,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionConstructEx(tlsEng=%p,mtxEng=%p,mtxSession=%p,logMgr=%p,tlsSession=%p,sslSession=%p)=%d",
        tlsEngine,mtxEngine,mtxSession,logMgr,tlsSession,sslSession,RV_OK));

    return RV_OK;
}

/**********************************************************************************
 * RvTLSSessionDestruct - Destructs TLS/SSL session
 *
 * INPUT:
 *  tlsSession  - new TLS session
 *  mtx         - Mutex that protects session
 *                structure
 *  logMgr      - log instance
 * OUTPUT: none
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSSessionDestruct(
    IN RvTLSSession     *tlsSession,
    IN RvMutex          *mtx,
    IN RvLogMgr         *logMgr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input */
    if ((tlsSession == NULL) || (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionDestruct(tlsSession=%p,mtx=%p,logMgr=%p)",
        tlsSession,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

    /* Note that SSL session BIO is freed by
    the SSL_free and not required separate
    BIO_free function call */
    SSL_free(tlsSession->sslSession);
    tlsSession->sslSession = NULL;

    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionDestruct(tlsSession=%p,mtx=%p,logMgr=%p)=%d",
        tlsSession,mtx,logMgr,RV_OK));

    return RV_OK;
}



/**********************************************************************************
 * RvTLSSessionClientHandshake - client side SSL/TLS handshake
 *
 * Initiates client connection to a remote TLS/SSL server.
 *
 * INPUT:
 *  tlsSession  - created earlier TLS session
 *  certCB      - certificate callback. If is not NULL enables
 *                certificate check by client.
 *  tcpSock     - already connected to server TCP socket.
 *  mtx         - Mutex that protects session
 *                structure
 *  logMgr      - log instance
 * OUTPUT:
 * RETURN: RvStatus - Success, WilllBlock or failure
 */

RVCOREAPI
RvStatus RvTLSSessionClientHandshake(
    IN  RvTLSSession            *tlsSession,
    IN  RvCompareCertificateCB  certCB,
    IN  RvSocket                tcpSock,
    IN  RvMutex                 *mtx,
    IN  RvLogMgr                *logMgr)
{
    RvInt       sslRetCode;
    RvInt       sslErrCode;
    RvStatus    stat = RV_OK;
    RvChar          *file;
    RvChar          *err;
    RvInt           line;
    RvInt           errFlags;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL) || (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif


    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionClientHandshake(tlsSession=%p,certCB=%p,sock=%d,mtx=%p,logMgr=%p)",
        tlsSession,certCB,tcpSock,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

	ERR_clear_error();

    /* In case handshake is applied for the first time,
    BIO object should be created and certificate verification
    should be set according to input */
    if (tlsSession->bio == NULL) {
#if (RV_USE_RV_BIO != RV_YES)
        tlsSession->bio = BIO_new_socket((int)tcpSock,BIO_NOCLOSE);
#else
        tlsSession->bio = BIO_new_rvsocket((int)tcpSock,logMgr);/*BIO_NOCLOSE,logMgr*/
#endif
        tlsSession->sock = tcpSock;
        SSL_set_bio(tlsSession->sslSession,tlsSession->bio,tlsSession->bio);

        if (certCB != NULL) {
        /* Note that when initialaizing tlsEngine
        we set default certificate check to none so
            in case enableCertCheck is 'FALSE' nothing should be done */
            SSL_set_verify(tlsSession->sslSession,
                       SSL_VERIFY_PEER,
                       (int (*)(int, X509_STORE_CTX *))certCB);
        }
    }

    if ((sslRetCode = SSL_connect(tlsSession->sslSession)) <= 0) {
        /* it is supported that most failures are result of
        'will block' condition. In other cases error code
        will be replaced by the 'generall error' */
        stat = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);

        rvTLSGetError(tlsSession,sslRetCode,mtx,&sslErrCode);
        switch(sslErrCode) {
        case SSL_ERROR_WANT_READ:
          RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                          "RvTLSSessionClientHandshake: session (%p) SSL_RET_WANT_READ failure",
                          tlsSession));
          tlsSession->requiredForHandshake |= RV_SELECT_READ;
          break;
        case SSL_ERROR_WANT_WRITE:
          RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                          "RvTLSSessionClientHandshake: session (%p) SSL_RET_WANT_WRITE failure",
                          tlsSession));
          tlsSession->requiredForHandshake |= RV_SELECT_WRITE;
          break;
        default:
          if (ERR_get_error_line_data((const char **)&file,
                          &line, (const char **)&err,
                          &errFlags) != 0) {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                             "RvTLSSessionClientHandshake: session (%p) , general error - %s, file - %s, line - %d",
                             tlsSession,err,file,line));
          }
          else {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                             "RvTLSSessionClientHandshake: session (%p) general eError",
                             tlsSession));
          }
          tlsSession->requiredForHandshake = 0;
          /* in case of other that WONT_READ/WRITE error
             replace return code with generol error ret. code */
          stat = RvTLSErrorCode(RV_TLS_ERROR_GEN);
          break;
        }

        RvMutexUnlock(mtx,logMgr);

        return stat;
    }

    /* Handshake succeeded */
    tlsSession->requiredForHandshake = 0;
    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionClientHandshake(tlsSession=%p,certCB=%p,sock=%d,mtx=%p,logMgr=%p)=%d",
        tlsSession,certCB,tcpSock,mtx,logMgr,RV_OK));

    return RV_OK;
}


#if RV_TLS_ENABLE_RENEGOTIATION

/**********************************************************************************
 * RvTLSSessionRenegotiate - renegotiate TLS session
 *
 * INPUT:
 *	tlsSession	- Connected TLS session
 *	forced		- if RV_FALSE - renegotiation process will be completed during
 *                I/O operation. Currently unused
 *                if RV_TRUE - no applicationI/O is possible during renegotiation process,
 *                e.g. next message from the peer should be handshake protocol message
 *	mtx			- Mutex that protects session structure
 *  logMgr		- log instance
 * RETURN:
 *	RvStatus	- Success, WilllBlock or failure
 *
 */

RVCOREAPI
RvStatus RvTLSSessionRenegotiate(
            RvTLSSession *tlsSession, RvBool forced, RvMutex *mtx, RvLogMgr *logMgr)
{
    RV_UNUSED_ARG(forced);
    return RvTLSSessionRenegotiateEx(tlsSession, mtx, logMgr);
}


/**********************************************************************************
 * RvTLSSessionRenegotiateEx - renegotiate TLS session
 *
 * INPUT:
 *	tlsSession	- Connected TLS session
 *	mtx			- Mutex that protects session structure
 *  logMgr		- log instance
 * RETURN:
 *	RvStatus	- Success, WilllBlock or failure
 *
 */

RVCOREAPI
RvStatus RvTLSSessionRenegotiateEx(RvTLSSession *tlsSession, RvMutex *mtx, RvLogMgr *logMgr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL) || (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    /* In order to prevent compilation warnings for NO_LOGS SINGL_THREAD mode */
    RV_UNUSED_ARG(logMgr);

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionRenegotiate(tlsSession=%p,renegState=%d,mtx=%p,logMgr=%p)",
        tlsSession,tlsSession->renegState,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

    if (tlsSession->renegState != RV_TLS_RENEG_NONE)
    {
        RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionRenegotiate(tlsSession=%p,mtx=%p,logMgr=%p) - renegotiation in progress",
            tlsSession,mtx,logMgr));
        goto exit;
    }

    /* Don't call SSL_renegotiate() here. OpenSSL can't handle correctly
    request to renegotiate, if it will be followed by SSL_write() call.
    Therefore marks the connection as waiting for call to SSL_renegotiate().
    Handshake requests are sent and are received by OpenSSL from within
    SSL_read. Therefore renegotiation should be initiated before SSL_read.
    Progress of renegotiation should be tested after SSL_read.
    No SSL_writes should be done until the renegotiation will complete. */
    tlsSession->renegState = RV_TLS_RENEG_NEEDED;
    RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionRenegotiate(tlsSession=%p,mtx=%p,logMgr=%p) - state was changed to RENEG_NEEDED",
        tlsSession, mtx, logMgr));

exit:
    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionRenegotiate(tlsSession=%p,renegState=%d,mtx=%p,logMgr=%p)",
        tlsSession,tlsSession->renegState,mtx,logMgr));

	return RV_OK;
}

/**********************************************************************************
 * RvTLSSessionRenegotiationInProgress - check if TLS session is beiung renegotiated
 *
 * INPUT:
 *	tlsSession	- Connected TLS session
 *	mtx			- Mutex that protects session structure
 *  logMgr		- log instance
 * OUTPUT:
 *  pbInProgress - RV_TRUE if it is, RV_FALSE - otherwise
 * RETURN:
 *	RvStatus	- Success, WilllBlock or failure
 *
 */

RVCOREAPI
RvStatus RvTLSSessionRenegotiationInProgress(
            RvTLSSession *tlsSession, RvMutex *mtx, RvLogMgr *logMgr,
            OUT RvBool *pbInProgress)
{
    /* In order to prevent compilation warnings for NO_LOGS SINGL_THREAD mode */
    RV_UNUSED_ARG(logMgr);

	RvMutexLock(mtx, logMgr);
    *pbInProgress = (tlsSession->renegState != RV_TLS_RENEG_NONE) ?
                    RV_TRUE : RV_FALSE;
	RvMutexUnlock(mtx, logMgr);
	return RV_OK;
}

#endif /*#if RV_TLS_ENABLE_RENEGOTIATION*/


/**********************************************************************************
 * RvTLSSessionServerHandshake - server side SSL/TLS handshake
 *
 * Responses to client handshake request.
 *
 * INPUT:
 *  tlsSession  - created earlier TLS session
 *  tcpSock     - already accepted server TCP socket.
 *  certCB      - certification callback. If not NULL
 *                server requests client certification
 *  mtx         - Mutex that protects session
 *                structure
 *  logMgr      - log instance
 * OUTPUT:
 * RETURN: RvStatus - Success, WilllBlock or failure
 */
RVCOREAPI
RvStatus RvTLSSessionServerHandshake(
    IN  RvTLSSession            *tlsSession,
    IN  RvCompareCertificateCB  certCB,
    IN  RvSocket                tcpSock,
    IN  RvMutex                 *mtx,
    IN  RvLogMgr                *logMgr)
{
    RvInt       sslRetCode;
    RvInt       sslErrCode;
    RvStatus    stat = RV_OK;
    RvChar      *file;
    RvChar      *err;
    RvInt       line;
    RvInt       errFlags;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL) || (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif


    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionServerHandshake(tlsSession=%p,certCB=%p,sock=%d,mtx=%p,logMgr=%p)",
        tlsSession,certCB,tcpSock,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

	ERR_clear_error();

    /* In case handshake is applied for the first time,
    BIO object should be created and certificate check model
    should be set */
    if (tlsSession->bio == NULL) {
#if (RV_USE_RV_BIO != RV_YES)
        tlsSession->bio = BIO_new_socket((int)tcpSock,BIO_NOCLOSE);
#else
        tlsSession->bio = BIO_new_rvsocket((int)tcpSock,logMgr);/*logMgr,BIO_NOCLOSE*/
#endif
        tlsSession->sock = tcpSock;
        SSL_set_bio(tlsSession->sslSession,tlsSession->bio,tlsSession->bio);

        if (certCB != NULL) {
        /* Note that when initializing tlsEngine
        we set default certificate check to none so
            in case clientCert is 'FALSE' nothing should be done */
            SSL_set_verify(tlsSession->sslSession,
                       SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       (int (*)(int, X509_STORE_CTX *))certCB);
        }
    }

    if ((sslRetCode = SSL_accept(tlsSession->sslSession)) <= 0) {
        /* it is supposed that most failures are result of
        'will block' condition. In other cases error code
        will be replaced by the 'general error' */
        stat = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);

        rvTLSGetError(tlsSession,sslRetCode,mtx,&sslErrCode);
        switch(sslErrCode) {
        case SSL_ERROR_WANT_READ:
          RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                          "RvTLSSessionServerHandshake: session (%p) SSL_RET_WANT_READ failure",
                          tlsSession));
          tlsSession->requiredForHandshake |= RV_SELECT_READ;
          break;
        case SSL_ERROR_WANT_WRITE:
          RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                          "RvTLSSessionServerHandshake: session (%p) SSL_RET_WANT_WRITE failure",
                          tlsSession));
          tlsSession->requiredForHandshake |= RV_SELECT_WRITE;
          break;
        default:
          if (ERR_get_error_line_data((const char **)&file, &line,
                          (const char **)&err,
                          &errFlags) != 0) {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                             "RvTLSSessionServerHandshake: session (%p) general error - %s, file - %s, line - %d",
                             tlsSession,err,file,line));
          }
          else {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                             "RvTLSSessionServerHandshake: session (%p) general error",
                             tlsSession));
          }
          stat = RvTLSErrorCode(RV_TLS_ERROR_GEN);
          tlsSession->requiredForHandshake = 0;
          break;
        }

        RvMutexUnlock(mtx,logMgr);
        return stat;
    }
    tlsSession->requiredForHandshake = 0;

    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionServerHandshake(tlsSession=%p,certCB=%p,sock=%d,mtx=%p,logMgr=%p)=%d",
        tlsSession,certCB,tcpSock,mtx,logMgr,RV_OK));

    return RV_OK;
}


/**********************************************************************************
 * RvTLSSessionReceiveBuffer - retrieves SSL/TLS message
 *
 * Reads & decrypt SSL/TLS message. Saves retrieved message or it's part output
 * buffer
 *
 * INPUT:
 *  tlsSession       - Connected TLS session
 *  receiveBuf       - receive buffer
 *  mtx              - Mutex that protects session
 *                     structure
 *  logMgr           - log instance
 *  receiveBufLen    - receive buffer maximum length
 * OUTPUT:
 *  receiveBufLen    - length of received data
 * RETURN: RvStatus  - Success, WilllBlock, Pending or failure
 */
RVCOREAPI
RvStatus RvTLSSessionReceiveBuffer(
    IN    RvTLSSession  *tlsSession,
    IN    RvChar        *receiveBuf,
    IN    RvMutex       *mtx,
    IN    RvLogMgr      *logMgr,
    INOUT RvInt         *receiveBufLen)
{
    RvTLSEvents pendingTlsEvents = 0;
    return RvTLSSessionReceiveBufferEx(
            tlsSession, receiveBuf, mtx, logMgr, receiveBufLen, &pendingTlsEvents);
}

/**********************************************************************************
 * RvTLSSessionReceiveBufferEx - retrieves SSL/TLS message
 *
 * Reads & decrypt SSL/TLS message. Saves retrieved message or it's part output
 * buffer
 *
 * INPUT:
 *  tlsSession       - Connected TLS session
 *  receiveBuf       - receive buffer
 *  mtx              - Mutex that protects session
 *                     structure
 *  logMgr           - log instance
 *  receiveBufLen    - receive buffer maximum length
 * OUTPUT:
 *  receiveBufLen    - length of received data
 *  pendingTlsEvents - TLS events requested by application that were blocked
 *                     early and can be processed now are added to this mask.
 * RETURN: RvStatus  - Success, WilllBlock, Pending or failure
 */
RVCOREAPI
RvStatus RvTLSSessionReceiveBufferEx(
    IN    RvTLSSession  *tlsSession,
    IN    RvChar        *receiveBuf,
    IN    RvMutex       *mtx,
    IN    RvLogMgr      *logMgr,
    INOUT RvInt         *receiveBufLen,
    OUT   RvTLSEvents   *pendingTlsEvents)
{
    RvInt       sslRetCode;
    RvStatus    retCode = RV_OK;
    RvInt       sslErrCode  = SSL_ERROR_NONE;
    RvChar      *file;
    RvChar      *err;
    RvInt       line;
    RvInt       errFlags;
    RvInt       shutdownStatus;
	RvInt		sslver_rbuf_left;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL)    || (receiveBuf == NULL) ||
        (receiveBufLen == NULL) || (mtx == NULL)        ||
        (*receiveBufLen <= 0)   || (pendingTlsEvents == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionReceiveBufferEx(tlsSession=%p,receiveBuf=%p,mtx=%p,logMgr=%p,bufLen=%d)",
        tlsSession,receiveBuf,mtx,logMgr,*receiveBufLen));

    RvMutexLock(mtx,logMgr);

    /* Block call to SSL_read(), if previous SSL_write was not completed */
    if (tlsSession->requiredForTLSWrite != 0)
    {
        RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionReceiveBufferEx(tlsSession=%p) - block reading, previous write was not completed",
            tlsSession));
        /* Add TLS_READ to the waiting list in order to enable it later */
        tlsSession->pendingTlsEvents |= RV_TLS_READ_EV;
        retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
        goto exit;
    }

    /* Before call to SSL_read() check if renegotiation should be initiated.
       For more details see comments in RvTLSSessionRenegotiate(). */
#if RV_TLS_ENABLE_RENEGOTIATION
    if (tlsSession->renegState == RV_TLS_RENEG_NEEDED &&
        tlsSession->requiredForTLSWrite==0 && tlsSession->requiredForTLSRead==0)
    {
        RvBool bInProgress;
        bInProgress = SSL_renegotiate_pending(tlsSession->sslSession);

        /* Request OpenSSL for renegotiation only if there is no renegotiation
        right now, that was initiated by the other side.  */
        if (bInProgress == RV_FALSE)
        {
            SSL_renegotiate(tlsSession->sslSession);
        }

        /* Don't call handshake API: if there is an application data in receive
        buffer of session socket, the SSL_do_handshake() will try to handle it
        as a handshake data and will fail.This behavior contradicts the OpenSSL
        documentation and can be considered as a bug in OpenSSL.
        The workaround is to use silent renegotiation (renegotiation without
        explicit call to SSL_do_handshake()) and disable SSL_write() until
        renegotiation will be finished. SSL_renegotiate_pending() is used in
        order to find out if renegotiation is still in progress. */

        tlsSession->renegState = RV_TLS_RENEG_STARTED;
        RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionReceiveBufferEx(tlsSession=%p,logMgr=%p) - renegotiation was %s",
            tlsSession,logMgr,(bInProgress == RV_FALSE)?"initiated":"detected"));

        /* Ensure that SSL_write is blocked while renegotiating.
        For more details see comments in RvTLSSessionRenegotiate(). */
        tlsSession->requiredForTLSRead = RV_SELECT_READ;
    }
#endif /* #if RV_TLS_ENABLE_RENEGOTIATION */

    sslRetCode = SSL_read(tlsSession->sslSession,receiveBuf,*receiveBufLen);
	if (sslRetCode <= 0)
		rvTLSGetError(tlsSession,sslRetCode,mtx,&sslErrCode);

	if (tlsSession->sslSession->version==SSL2_VERSION)
    {
		sslver_rbuf_left = tlsSession->sslSession->s2->rbuf_left;
	}
    else
    {
		sslver_rbuf_left = tlsSession->sslSession->s3->rbuf.left;
	}

    /* After each call to SSL_read check if renegotiation was completed.
       For more details see comments in RvTLSSessionRenegotiate(). */
#if RV_TLS_ENABLE_RENEGOTIATION
    if (tlsSession->renegState == RV_TLS_RENEG_STARTED)
    {
        RvBool bWasCompleted;
        bWasCompleted = (1==SSL_renegotiate_pending(tlsSession->sslSession)) ?
                        RV_FALSE : RV_TRUE;

        /* If renegotiation was completed, reset state */
        if (bWasCompleted == RV_TRUE)
        {
	        tlsSession->renegState = RV_TLS_RENEG_NONE;

            RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                "RvTLSSessionReceiveBufferEx(tlsSession=%p,logMgr=%p) - renegotiation was completed (sslErrCode=%d, rbuf.left=%d)",
                tlsSession, logMgr, sslErrCode, sslver_rbuf_left));
        }
        else /* Renegotiation is being in progress */
        {
            RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                "RvTLSSessionReceiveBufferEx(tlsSession=%p,logMgr=%p) - renegotiation is in progress (sslErrCode=%d, rbuf.left=%d)",
                tlsSession, logMgr, sslErrCode, sslver_rbuf_left));
        }
    }
#endif /* #if RV_TLS_ENABLE_RENEGOTIATION */

    /* Workaround for cases where:
       1. Remote peer doesn't send application data after last handshake msg,
       2. RvTLSSessionReceiveBufferEx() was called when there is nothing to
          be read from socket.
       In these cases SSL_read() will be blocked on WANT_READ.
       The blocked SSL_read causes us to disable SSL_writes, as it is requested
       by the OpenSSL documentation. As a result deadlock may occur:
       local side, acting as a client, doesn't send messages due to blocked
       SSL_read; remote side, acting as a server, doesn't send messages,
       because it doesn't receive data from client.
       In order to prevent this deadlock, enable SSL_write:
       doesn't set tlsSession->requiredForTLSRead. */
    if (sslErrCode == SSL_ERROR_WANT_READ  &&  sslver_rbuf_left == 0)
    {
        *pendingTlsEvents |= RV_TLS_READ_EV;
        /* Keep SSL_write blocked during renegotiation */
#if RV_TLS_ENABLE_RENEGOTIATION
        if (tlsSession->renegState != RV_TLS_RENEG_STARTED)
#endif /* #if RV_TLS_ENABLE_RENEGOTIATION */
        {
            tlsSession->requiredForTLSRead = 0;
		}
        retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
        goto exit;
    }

    /* Perform sanity check of rbuf.left structure.
       It may contain a junk, if version of OpenSSL libraries linked to
       application doesn't match version of the OpenSSL include files used for
       compilation of Common Core */
    if (sslver_rbuf_left > 100000 || sslver_rbuf_left < 0)
    {
        RvLogExcep(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionReceiveBufferEx(tlsSession=%p,logMgr=%p): rbuf.left contains junk (%d)",
            tlsSession, logMgr, sslver_rbuf_left));
    }

    switch(sslErrCode) {
    case SSL_ERROR_NONE:
        RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionReceiveBufferEx: session (%p) received %d bytes",
            tlsSession,sslRetCode));

        /* If this SSL_read caused completion of blocked operation,
           provide application with TLS events that were pended, in order to
           enable it to register these event with select. */
        if (tlsSession->pendingTlsEvents!=0 && tlsSession->requiredForTLSRead!=0)
        {
            *pendingTlsEvents |= tlsSession->pendingTlsEvents;
        }

        tlsSession->requiredForTLSRead = 0;
        *receiveBufLen = sslRetCode;

        /* After successful read enable sub sequential writes.
           For more details see rvIsWriteEnabled(). */
        tlsSession->counterWrites = 0;

        break;

    case SSL_ERROR_WANT_READ:
      RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionReceiveBufferEx: session (%p) SSL_RET_WANT_READ failure (rbuf.left=%d)",
        tlsSession, sslver_rbuf_left));
      tlsSession->requiredForTLSRead |= RV_SELECT_READ;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
      break;

    case SSL_ERROR_WANT_WRITE:
      RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionReceiveBufferEx: session (%p) SSL_RET_WANT_WRITE failure.",
        tlsSession));
      tlsSession->requiredForTLSRead |= RV_SELECT_WRITE;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
      break;

    case SSL_ERROR_ZERO_RETURN:     /* possible shutdown */
        RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionReceiveBufferEx: session (%p) SSL_RET_ZERO_RETURN failure.",
            tlsSession));
#if RV_TLS_ENABLE_RENEGOTIATION
        tlsSession->renegState = RV_TLS_RENEG_NONE;
#endif
        shutdownStatus = SSL_get_shutdown(tlsSession->sslSession);
        if ( shutdownStatus & (SSL_RECEIVED_SHUTDOWN | SSL_SENT_SHUTDOWN)) {
            RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                "RvTLSSessionReceiveBufferEx: session (%p) shutdown request received.",
                tlsSession));
            retCode = RvTLSErrorCode(RV_TLS_ERROR_SHUTDOWN);
        }
        break;
    default:
      if (ERR_get_error_line_data((const char **)&file, &line,
                      (const char **)&err, &errFlags) != 0) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionReceiveBufferEx: session (%p) general error - %s, file - %s, line - %d",
            tlsSession,err,file,line));
      }
      else {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionReceiveBufferEx: session (%p) general error.",
            tlsSession));
      }
#if RV_TLS_ENABLE_RENEGOTIATION
      tlsSession->renegState = RV_TLS_RENEG_NONE;
#endif
      tlsSession->requiredForTLSRead = 0;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
      break;
    }

    /* Ensure that SSL_write is blocked while renegotiating.
       For more details see comments in RvTLSSessionRenegotiate(). */
#if RV_TLS_ENABLE_RENEGOTIATION
    if (tlsSession->renegState == RV_TLS_RENEG_STARTED  &&
        tlsSession->requiredForTLSRead == 0)
    {
        tlsSession->requiredForTLSRead = RV_SELECT_READ;
    }
#endif /* #if RV_TLS_ENABLE_RENEGOTIATION */

exit:
    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionReceiveBufferEx(tlsSession=%p,receiveBuf=%p,mtx=%p,logMgr=%p,bufLen=%d)=%d",
        tlsSession,receiveBuf,mtx,logMgr,*receiveBufLen,retCode));

    return retCode;
}

/**********************************************************************************
 * RvTLSSessionSendBuffer - sends stack message via SSL session
 *
 * Encrypts stack message and sends SSL/TLS message with encrypted stack data to
 * the SSL session peer.
 *
 * INPUT:
 *  tlsSession       - Connected TLS session
 *  sendBuf          - buffer to send
 *  sendBufLen       - length of stack data in the sendBuf
 *  mtx              - Mutex that protects session
 *                     structure
 *  logMgr           - log instance
 * OUTPUT: none
 * RETURN:
 *  RvStaus          - Success, WilllBlock or failure
 */
RVCOREAPI
RvStatus RvTLSSessionSendBuffer(
    IN  RvTLSSession    *tlsSession,
    IN  RvChar          *sendBuf,
    IN  RvInt           sendBufLen,
    IN  RvMutex         *mtx,
    IN  RvLogMgr        *logMgr)
{
    RvSize_t     sent;
    RvTLSEvents  pendingTlsEvents = 0;
    return RvTLSSessionSendBufferEx(tlsSession, sendBuf, sendBufLen, mtx,
                                    logMgr, &pendingTlsEvents, &sent);
}

/**********************************************************************************
 * RvTLSSessionSendBufferEx - sends stack message via SSL session
 *
 * Encrypts stack message and sends SSL/TLS message with encrypted stack data to
 * the SSL session peer.
 *
 * INPUT:
 *  tlsSession       - Connected TLS session
 *  sendBuf          - buffer to send
 *  sendBufLen       - length of stack data in the sendBuf
 *  mtx              - Mutex that protects session
 *                     structure
 *  logMgr           - log instance
 * OUTPUT: none
 *  pendingTlsEvents - TLS events requested by application that were blocked
 *                     early and can be processed now are added to this mask.
 *  pSent            - how much bytes were sent
 * RETURN:
 *  RvStaus          - Success, WilllBlock or failure
 */
RVCOREAPI
RvStatus RvTLSSessionSendBufferEx(
    IN  RvTLSSession    *tlsSession,
    IN  RvChar          *sendBuf,
    IN  RvInt           sendBufLen,
    IN  RvMutex         *mtx,
    IN  RvLogMgr        *logMgr,
    OUT RvTLSEvents     *pendingTlsEvents,
    OUT RvSize_t        *pSent)
{
    RvInt     sslRetCode;
    RvStatus  retCode = RV_OK;
    RvInt     sslErrCode;
    RvChar    *file;
    RvChar    *err;
    RvInt     line;
    RvInt     errFlags;
    RvInt     shutdownState;
    RvBool    bIsWriteEnabled;
    RvChar    *strWriteBlockRsn;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL) || (sendBuf == NULL) ||
        (sendBufLen <= 0)    || (mtx == NULL)     ||
        (pSent == NULL)      || (pendingTlsEvents == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionSendBufferEx(tlsSession=%p,sendBuf=%p,bufLen=%d,mtx=%p,logMgr=%p)",
        tlsSession,sendBuf,sendBufLen,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

    bIsWriteEnabled = rvIsWriteEnabled(tlsSession, pendingTlsEvents,
                                       &strWriteBlockRsn);
    if (bIsWriteEnabled == RV_FALSE)
    {
        RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionSendBufferEx(tlsSession=%p) - send is blocked: %s",
            tlsSession, strWriteBlockRsn));
        retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
        goto exit;
    }

    sslRetCode = SSL_write(tlsSession->sslSession,sendBuf,sendBufLen);
    if (sslRetCode <= 0)
        rvTLSGetError(tlsSession,sslRetCode,mtx,&sslErrCode);
    else
        sslErrCode = SSL_ERROR_NONE;

    switch(sslErrCode)
    {
    case SSL_ERROR_NONE:
      /* If this SSL_write caused completion of blocked operation,
         provide application with TLS events that were pended, in order to
         enable it to register these event with select. */
      if (tlsSession->requiredForTLSWrite!=0 && tlsSession->pendingTlsEvents!=0)
      {
            *pendingTlsEvents |= tlsSession->pendingTlsEvents;
      }
      /* empty list of events required to complete SSL write */
      tlsSession->requiredForTLSWrite = 0;
      tlsSession->counterWrites++;
      *pSent = (RvSize_t)sslRetCode;

      /* If only a part of message was sent, disable further SSL_writes until
         next WRITE event on select. This gives chance to application to read*/
      if (*pSent < (RvSize_t)sendBufLen)
      {
        tlsSession->counterWrites = MAX_SUBSEQUENT_WRITES;
      }
      break;
    case SSL_ERROR_WANT_READ:
      RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                       "RvTLSSessionSendBufferEx: session (%p) sent SSL_RET_WANT_READ failure.",
                       tlsSession));
      tlsSession->requiredForTLSWrite |= RV_SELECT_READ;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
      break;
    case SSL_ERROR_WANT_WRITE:
      RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                       "RvTLSSessionSendBufferEx: session (%p) sent SSL_RET_WANT_WRITE failure.",
                       tlsSession));
      tlsSession->requiredForTLSWrite |= RV_SELECT_WRITE;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
      break;
    case SSL_ERROR_ZERO_RETURN:     /* possible shutdown */
        RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionSendBufferEx: session (%p) sent SSL_RET_ZERO_RETURN failure.",
            tlsSession));
        shutdownState = SSL_get_shutdown(tlsSession->sslSession);
        if (shutdownState &(SSL_RECEIVED_SHUTDOWN|SSL_SENT_SHUTDOWN)) {
            RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                "RvTLSSessionSendBufferEx: session (%p) shutdown request received.",
                tlsSession));
            retCode = RvTLSErrorCode(RV_TLS_ERROR_SHUTDOWN);
        }
        break;

    default:
      if (ERR_get_error_line_data((const char **)&file, &line,
                      (const char **)&err, &errFlags) != 0) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSSessionSendBufferEx: session (%p) general error - %s file - %s line - %d .",
                         tlsSession,err,file,line));
      }
      else {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSSessionSendBufferEx: session (%p) sent general error %d",
                         tlsSession, sslErrCode));
      }

      /* empty list of events required to complete SSL write */
      tlsSession->requiredForTLSWrite = 0;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
      break;
    }

exit:
    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionSendBufferEx(tlsSession=%p,sendBuf=%p,bufLen=%d,mtx=%p,logMgr=%p)=%d",
        tlsSession,sendBuf,sendBufLen,mtx,logMgr,retCode));

    return retCode;
}

/**********************************************************************************
 * RvTLSShutdown - shutdown SSL/TLS session
 *
 * Sends shutdown request to the SSL/TLS connection peer
 *
 * INPUT:
 *  tlsSession      - Connected TLS session
 *  mtx             - Mutex that protects session
 *                    structure
 *  logMgr          - log instance
 * OUTPUT: none
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSSessionShutdown(
    IN  RvTLSSession    *tlsSession,
    IN  RvMutex         *mtx,
    IN  RvLogMgr        *logMgr)
{
    RvInt       sslRetCode;
    RvChar      *file;
    RvChar      *err;
    RvInt       line;
    RvInt       errFlags;
    RvStatus    retCode = RV_OK;

#define RV_TLS_SHUTDOWN_WAITING_RET 0
#define RV_TLS_SHUTDOWN_SUCCEED_RET 1
#define RV_TLS_SHUTDOWN_FAILURE_RET -1

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL) || (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionShutdown(tlsSession=%p,mtx=%p,logMgr=%p)",
        tlsSession,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

    sslRetCode = SSL_shutdown(tlsSession->sslSession);
    tlsSession->requiredForTLSShutdown = 0;

    switch (sslRetCode) {
    case RV_TLS_SHUTDOWN_SUCCEED_RET:
        RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionShutdown: session (%p) shutdown succeed",
            tlsSession));
        tlsSession->bWaitingShutdownFinish = RV_FALSE;
        break;
    case RV_TLS_SHUTDOWN_WAITING_RET:
        RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionShutdown: session (%p) shutdown incomplete",
            tlsSession));
        retCode = RvTLSErrorCode(RV_TLS_ERROR_INCOMPLETE);
        tlsSession->bWaitingShutdownFinish = RV_TRUE;
        break;
    case RV_TLS_SHUTDOWN_FAILURE_RET:
        {
            RvInt       sslErrCode;

            rvTLSGetError(tlsSession,sslRetCode,mtx,&sslErrCode);
            switch(sslErrCode) {
            case SSL_ERROR_WANT_READ:
                RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                    "RvTLSSessionShutdown: session (%p) SSL_RET_WANT_READ failure.",
                    tlsSession));
                tlsSession->requiredForTLSShutdown |= RV_SELECT_READ;
                retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
                break;
            case SSL_ERROR_WANT_WRITE:
                RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                    "RvTLSSessionShutdown: session (%p) SSL_RET_WANT_WRITE failure.",
                    tlsSession));
                tlsSession->requiredForTLSShutdown |= RV_SELECT_WRITE;
                retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
                break;
            default:
                if (ERR_get_error_line_data((const char **)&file, &line,
                    (const char **)&err, &errFlags) != 0) {
                    RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                        "RvTLSSessionShutdown: session (%p) shutdown general error - %s file - %s line - %d",
                        tlsSession,err,file,line));
                }
                else {
                    RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                        "RvTLSSessionShutdown: session (%p) shutdown general failure.",
                        tlsSession));
                }
                /* empty list of events required to complete SSL shutdown */
                tlsSession->requiredForTLSShutdown = 0;
                retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
                break;
            }
        }
        break;
    default :
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                "RvTLSSessionShutdown: session (%p) shutdown unknown general failure.",
                tlsSession));
        retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
        break;
    }

    RvMutexUnlock(mtx,logMgr); /* Note that we can't apply unlock immidiately after
                               SSL_shutdown since in continue requiredForTLSShutdown
                               of the TLS session may be changed */

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionShutdown(tlsSession=%p,mtx=%p,logMgr=%p)=%d",
        tlsSession,mtx,logMgr,retCode));

    return retCode;
}


/**********************************************************************************
 * RvTLSTranslateSelectEvents - Translates received select events to appropriate
 * TLS event
 *
 * INPUT:
 *  tlsSession      - Connected TLS session
 *  selEvents       - select events to be translated
 *  mtx             - Mutex that protects session
 *                    structure
 *  logMgr          - log instance
 * OUTPUT:
 *  tlsEvents       - translated TLS events
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSTranslateSelectEvents(
    IN  RvTLSSession    *tlsSession,
    IN  RvSelectEvents  selEvents,
    IN  RvMutex         *mtx,
    IN  RvLogMgr        *logMgr,
    OUT RvTLSEvents     *tlsEvents)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL) || (tlsEvents == NULL) ||
        (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSTranslateSelectEvents(tlsSession=%p,pendingTlsEvents=%x,requiredForRead=%x,requiredForWrite=%x,requiredForTLSShutdown=%x,requiredForHandshake=%x,renegState=%d)",
        tlsSession,tlsSession->pendingTlsEvents,tlsSession->requiredForTLSRead,
        tlsSession->requiredForTLSWrite,tlsSession->requiredForTLSShutdown,
        tlsSession->requiredForHandshake, tlsSession->renegState));

    /* initiate output */
    *tlsEvents = (RvTLSEvents)0;

    RvMutexLock(mtx,logMgr);

    /* If there is not completed operation, deduce the correspondent TLS event.
       Note only one not completed operation is possible at any moment. */
    if (tlsSession->requiredForHandshake & selEvents)
    {
	    *tlsEvents = RV_TLS_HANDSHAKE_EV;
    }
    else if (tlsSession->requiredForTLSRead & selEvents)
    {
	    *tlsEvents = RV_TLS_READ_EV;
	}
    else if (tlsSession->requiredForTLSWrite & selEvents)
    {
	    *tlsEvents = RV_TLS_WRITE_EV;
        tlsSession->counterWrites = 0;
    }
    else if (tlsSession->requiredForTLSShutdown & selEvents)
    {
        *tlsEvents = RV_TLS_SHUTDOWN_EV;
	}
    else
    /* If there is no not completed operation, maps event directly */
    {
		if ((selEvents & RV_SELECT_WRITE) && (tlsSession->pendingTlsEvents & RV_TLS_HANDSHAKE_EV))
        {
			*tlsEvents |= RV_TLS_HANDSHAKE_EV;
		}
		if ((selEvents & RV_SELECT_READ) && (tlsSession->pendingTlsEvents & RV_TLS_READ_EV))
        {
			*tlsEvents |= RV_TLS_READ_EV;
		}
		if ((selEvents & RV_SELECT_WRITE) && (tlsSession->pendingTlsEvents & RV_TLS_WRITE_EV))
        {
			*tlsEvents |= RV_TLS_WRITE_EV;
            tlsSession->counterWrites = 0;
		}
		if ( ((selEvents & RV_SELECT_WRITE) && (tlsSession->pendingTlsEvents & RV_TLS_SHUTDOWN_EV)) ||
             tlsSession->bWaitingShutdownFinish == RV_TRUE
           )
        {
			*tlsEvents |= RV_TLS_SHUTDOWN_EV;
		}
    }

    /* Clear received events from session waiting events list */
    tlsSession->pendingTlsEvents &= (~(*tlsEvents));

    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSTranslateSelectEvents(tlsSession=%p,pendingTlsEvents=%x,requiredForRead=%x,requiredForWrite=%x,requiredForTLSShutdown=%x,requiredForHandshake=%x,renegState=%d)",
        tlsSession,tlsSession->pendingTlsEvents,tlsSession->requiredForTLSRead,
        tlsSession->requiredForTLSWrite,tlsSession->requiredForTLSShutdown,
        tlsSession->requiredForHandshake, tlsSession->renegState));

    return RV_OK;
}

/**********************************************************************************
 * RvTLSTranslateTLSEvents - Translates received TLS events to appropriate
 * select event
 *
 * INPUT:
 *  tlsSession      - Connected TLS session
 *  tlsEvents       - TLS event
 *  mtx             - Mutex that protects session
 *                    structure
 *  logMgr          - log instance
 * OUTPUT: selEvents        - select events memory
 * RETURN: RvStatus - Success, WilllBlock or failure
 */
RVCOREAPI
RvStatus RvTLSTranslateTLSEvents(
    IN  RvTLSSession    *tlsSession,
    IN  RvTLSEvents     tlsEvents,
    IN  RvMutex         *mtx,
    IN  RvLogMgr        *logMgr,
    OUT RvSelectEvents  *selEvents)
{
    RvBool bBlockedOperation;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL) || (selEvents == NULL) ||
        (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSTranslateTLSEvents(tlsSession=%p,tlsEvents=%x,requiredForRead=%x,requiredForWrite=%x,requiredForTLSShutdown=%x,requiredForHandshake=%x,renegState=%d)",
        tlsSession,tlsEvents,tlsSession->requiredForTLSRead,
        tlsSession->requiredForTLSWrite, tlsSession->requiredForTLSShutdown,
        tlsSession->requiredForHandshake, tlsSession->renegState));

    /* initiate output */
    *selEvents = 0;

    RvMutexLock(mtx,logMgr);

    /* If there is not completed operation, enable only events that are
       required for completion of this operation. */
    bBlockedOperation = RV_FALSE;
    if (tlsSession->requiredForHandshake != 0)
    {
        *selEvents |= tlsSession->requiredForHandshake;
        bBlockedOperation = RV_TRUE;
    }
    else if (tlsSession->requiredForTLSRead != 0)
    {
        *selEvents |= tlsSession->requiredForTLSRead;
        bBlockedOperation = RV_TRUE;
    }
    else if (tlsSession->requiredForTLSWrite != 0)
    {
        *selEvents |= tlsSession->requiredForTLSWrite;
        bBlockedOperation = RV_TRUE;
    }
    else if (tlsSession->requiredForTLSShutdown != 0)
    {
        *selEvents |= tlsSession->requiredForTLSShutdown;
        bBlockedOperation = RV_TRUE;
    }
    else
    /* If there is no not completed operation, maps event directly */
    if (*selEvents == 0)
    {
        if (tlsEvents & RV_TLS_HANDSHAKE_EV)
        {
            *selEvents |= RV_SELECT_WRITE;
        }
        if (tlsEvents & RV_TLS_READ_EV)
        {
            *selEvents |= RV_SELECT_READ;
        }
        if (tlsEvents & RV_TLS_WRITE_EV)
        {
            *selEvents |= RV_SELECT_WRITE;
        }
        if (tlsEvents & RV_TLS_SHUTDOWN_EV)
        {
            *selEvents |= RV_SELECT_WRITE;
        }
        if (tlsSession->bWaitingShutdownFinish == RV_TRUE)
        {
            *selEvents = RV_SELECT_READ;
        }
    }

    /* Save TLS events, requested by application, for later usage.
       The 'pendingTlsEvents' storage is used on:
       1. Resume from blocked operation upon operation completion -
          'pendingTlsEvents' stores events that waited for processing;
          Note only one not completed operation is possible at any moment.
       2. On conversion of select events into TLS events - 'pendingTlsEvents'
          helps to maps RV_SELECT_WRITE event into correct TLS event.
     */
     if (bBlockedOperation == RV_TRUE)
     {  /* Add requested events to the waiting list */
        tlsSession->pendingTlsEvents |= tlsEvents;
     }
     else
     {  /* Replace waiting list with the requested events */
        tlsSession->pendingTlsEvents = tlsEvents;
     }

    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSTranslateTLSEvents(tlsSession=%p,tlsEvents=%x,mtx=%p,logMgr=%p,selEvents=%x)",
        tlsSession,tlsEvents,mtx,logMgr,*selEvents));

    return RV_OK;
}


/**********************************************************************************
 * RvTLSGetCertificateLength - retrieves ASN1 format certificate length
 * from a certificate context received by a certificate verification callback.
 * This function used to specify stack how much memory should be allocated for
 * the certificate.
 *
 * INPUT:
 *  certCtx     - Certificate context
 * OUTPUT:
 *  certLen     - length of ASN1 certificate
 * RETURN:
 *  RvStatus    - Success or failure
 */
RVCOREAPI
RvStatus RvTLSGetCertificateLength(
    IN   void  *certCtx,
    OUT  RvInt *certLen)
{
    X509 *cert;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* Check input parameters */
    if ((certCtx == NULL) || (certLen == NULL)) {
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }
#endif

    cert = X509_STORE_CTX_get_current_cert((X509_STORE_CTX *)certCtx);

    if (cert == NULL) {
        *certLen = 0;
        return RV_OK;
    }

    *certLen = i2d_X509(cert,NULL);

    if (*certLen < 0)
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);

    return RV_OK;
}


/**********************************************************************************
 * RvTLSGetCertificate - retrieves ASN1 format certificate from a certificate
 * context received by a certificate verification callback.
 *
 * INPUT:
 *  certCtx     - Certificate context
 * OUTPUT:
 *  cert        - retrieved certificate. Note that user should provide
 *                allocated memory space enough for keeping the ASN1
 *                certificate
 * RETURN:
 *  RvStatus    - Success or failure
 */
RVCOREAPI
RvStatus RvTLSGetCertificate(
    IN  void   *certCtx,
    OUT RvChar *cert)
{
    X509    *certX509;
    RvChar  *certEnd;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* Check input parameters */
    if ((certCtx == NULL) || (cert == NULL)) {
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }
#endif

    certX509 = X509_STORE_CTX_get_current_cert((X509_STORE_CTX *)certCtx);

    if (certX509 == NULL)
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);

    certEnd = cert;

    if (i2d_X509(certX509,(unsigned char **)&certEnd) < 0)
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);

    return RV_OK;
}

/**********************************************************************************
* RvTLSSessionGetCertificateFingerprint - retrieves the peer certificate fingerprint
* according to hash algorithm name provided through the 'hashName'.
*
* INPUT:
*	tlsSession		- Connected TLS session
*	mtx			- Mutex that protects session
*				  structure
*  logMgr		- log instance
*  hashName     - the name of the hash algorithm (as SHA-1,MD5 etc)
* OUTPUT:
*  fingerprint	- the calculated fingerprint will be stored here
*  fLen         - will contain the length of the calculated fingerprint,
*                 on input must contain the size of the buffer specified by 'fingerprint'
* RETURN:
*	RvStatus	- Success or failure
*/
RVCOREAPI
RvStatus RvTLSSessionGetCertificateFingerprint(
    IN  RvTLSSession	*tlsSession,
    IN  RvMutex			*mtx,
    IN  RvLogMgr		*logMgr,
    IN  RvHashFuncEnum  hash,
    OUT RvUint8         *fingerprint,
    OUT RvUint          *fLen)
{
    const EVP_MD *digest;
    X509* cert;
    int ret;
    const RvChar* hashName = RvHashFuncEnum2Str(hash,RV_FALSE,RV_FALSE);

#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
    RV_UNUSED_ARG(logMgr);
#endif

    *fLen = 0;
    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionGetCertificateFingerprint(tlsSession=%p,mtx=%p,logMgr=%p,hash=%d)",
        tlsSession,mtx,logMgr,hash));


    digest = EVP_get_digestbyname(hashName);
    if (digest == NULL)
    {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionGetCertificateFingerprint: failed to find the digest by name %s",hashName));
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    RvMutexLock(mtx,logMgr);

    cert = SSL_get_peer_certificate(tlsSession->sslSession);

    if (cert == NULL) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionGetCertificateFingerprint: session (%p) failed to retrieve peer certificate",
            tlsSession));
        RvMutexUnlock(mtx,logMgr);
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }
    ret = X509_digest(cert,digest,fingerprint,fLen);
    X509_free(cert);

    if (!ret)
    {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionGetCertificateFingerprint: session (%p) failed in X509_digest ret %d (hashname %d)",
            tlsSession,ret,hashName));
        RvMutexUnlock(mtx,logMgr);
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionGetCertificateFingerprint(tlsSession=%p,mtx=%p,logMgr=%p,hashName=%s) fLen=%d",
        tlsSession,mtx,logMgr,hashName,*fLen));

    return RV_OK;
}


/**********************************************************************************
 * RvTLSSessionGetCertificateLength - retrieves ASN1 format certificate length
 * from a session that complete handshake
 * This function used to specify stack how much memory should be allocated for
 * the certificate.
 *
 * INPUT:
 *  tlsSession      - Connected TLS session
 *  mtx         - Mutex that protects session
 *                structure
 *  logMgr      - log instance
 * OUTPUT:
 *  certLen     - length of ASN1 certificate
 * RETURN:
 *  RvStatus    - Success or failure
 */
RVCOREAPI
RvStatus RvTLSSessionGetCertificateLength(
    IN  RvTLSSession    *tlsSession,
    IN  RvMutex         *mtx,
    IN  RvLogMgr        *logMgr,
    OUT RvInt           *certLen)
{
    X509 *cert;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* Check input parameters */
    if ((tlsSession == NULL) || (certLen == NULL) ||
        (mtx == NULL)) {
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionGetCertificateLength(tlsSession=%p,mtx=%p,logMgr=%p,certLen=%p)",
        tlsSession,mtx,logMgr,certLen));

    RvMutexLock(mtx,logMgr);

    cert = SSL_get_peer_certificate(tlsSession->sslSession);

    if (cert == NULL) {
      *certLen = 0;
      RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
          "RvTLSSessionGetCertificateLength: session (%p) failed to retrieve peer certificate",
          tlsSession));
      RvMutexUnlock(mtx,logMgr);
      return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    *certLen = i2d_X509(cert,NULL);

    RvMutexUnlock(mtx,logMgr);

    X509_free(cert);

    if (*certLen < 0) {
      RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
          "RvTLSSessionGetCertificate: session (%p) failed to convert X509 certificate to ASN1",
          tlsSession));
      return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionGetCertificateLength(tlsSession=%p,mtx=%p,logMgr=%p,certLen=%d)=%d",
        tlsSession,mtx,logMgr,*certLen,RV_OK));

    return RV_OK;
}


/**********************************************************************************
 * RvTLSSessionGetCertificate - retrieves ASN1 format certificate from a session
 * after handshake is complete
 *
 * INPUT:
 *  tlsSession  - Connected TLS session
 *  mtx         - Mutex that protects session
 *                structure
 *  logMgr      - log instance
 * OUTPUT:
 *  cert        - retrieved certificate. Note that user should provide
 *                allocated memory space enough for keeping the ASN1
 *                certificate
 * RETURN:
 *  RvStatus    - Success or failure
 */
RVCOREAPI
RvStatus RvTLSSessionGetCertificate(
    IN  RvTLSSession    *tlsSession,
    IN  RvMutex         *mtx,
    IN  RvLogMgr        *logMgr,
    OUT RvChar          *cert)
{
  X509  *certX509;
  RvChar    *certEnd;


#if (RV_CHECK_MASK & RV_CHECK_NULL)
  /* Check input parameters */
  if ((tlsSession == NULL) || (cert == NULL) ||
      (mtx == NULL)) {
    return RvTLSErrorCode(RV_TLS_ERROR_GEN);
  }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
  RV_UNUSED_ARG(logMgr);
#endif

  RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
      "RvTLSSessionGetCertificate(tlsSession=%p,mtx=%p,logMgr=%p,cert=%p)",
      tlsSession,mtx,logMgr,cert));

  RvMutexLock(mtx,logMgr);

  certX509 = SSL_get_peer_certificate(tlsSession->sslSession);

  if (certX509 == NULL) {
      RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
          "RvTLSSessionGetCertificate: session (%p) failed to retrieve peer certificate",
          tlsSession));
      RvMutexUnlock(mtx,logMgr);
      return RvTLSErrorCode(RV_TLS_ERROR_GEN);
  }

  certEnd = cert;
  if (i2d_X509(certX509,(unsigned char **)&certEnd) < 0) {
      RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
          "RvTLSSessionGetCertificate: session (%p) failed to convert X509 certificate to ASN1",
          tlsSession));
      RvMutexUnlock(mtx,logMgr);
      return RvTLSErrorCode(RV_TLS_ERROR_GEN);
  }

  RvMutexUnlock(mtx,logMgr);

  X509_free(certX509);


  RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
      "RvTLSSessionGetCertificate(tlsSession=%p,mtx=%p,logMgr=%p,cert=%p)=%d",
      tlsSession,mtx,logMgr,cert,RV_OK));

  return RV_OK;
}

/**********************************************************************************
 * RvTLSSessionExpose - exposes session's underlying SSL* pointer
 *
 * INPUT:
 *  tlsSession  - Connected TLS session
 *  mtx         - Mutex that protects session
 *                structure
 *  logMgr      - log instance
 * OUTPUT:
 *  underlying  - session's underlying SSL* pointer
 * RETURN:
 *  RvStatus    - Success or failure
 */
RVCOREAPI
RvStatus RvTLSSessionExpose(
    IN  RvTLSSession    *tlsSession,
    IN  RvMutex         *mtx,
    IN  RvLogMgr        *logMgr,
    OUT SSL            **underlying)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
  /* Check input parameters */
    if ((tlsSession == NULL) || (underlying == NULL) ||
      (mtx == NULL)) {
    return RvTLSErrorCode(RV_TLS_ERROR_GEN);
  }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

  RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
      "RvTLSSessionExpose(tlsSession=%p,mtx=%p,logMgr=%p,underlying=%p)",
      tlsSession,mtx,logMgr,underlying));

  RvMutexLock(mtx,logMgr);
  *underlying = tlsSession->sslSession;
  RvMutexUnlock(mtx,logMgr);

  RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
      "RvTLSSessionExpose(tlsSession=%p,mtx=%p,logMgr=%p,*underlying=%p)=%d",
      tlsSession,mtx,logMgr,*underlying,RV_OK));

  return RV_OK;
}

/**********************************************************************************
 * RvTLSGetCertificateVerificationError - retrieves certificate verification error
 *
 * INPUT:
 *  cert        - certificate context, received by verification callback
 * OUTPUT:
 *  errString   - retrieved error string
 * RETURN: none
 */
RVCOREAPI
void RvTLSGetCertificateVerificationError(
    IN  void    *cert,
    OUT RvChar  **errString)
{
    RvInt32             err;

    err = X509_STORE_CTX_get_error((X509_STORE_CTX *)cert);

    *errString = (RvChar *)X509_verify_cert_error_string(err);
}

/**********************************************************************************
 * RvTLSSessionCheckCertAgainstName
 *
 * Validate correctness of certificate by compare value, saved under 'DNS' key
 * in the certificate to name.
 *
 * INPUT:
 *  tlsSession  - Connected TLS session
 *  name        - domain name string
 *  mtx         - Mutex that protects session structure
 *  memBuf      - memory buffer used to save intermediate strings during processing.
 *              used to ensure threadsafeness
 *  memBufLen   - size of the memBuf
 *  logMgr      - log instance
 * RETURN:
 *  RvStatus    - Success if certificate matches the name, failure in case of error
 *              or if certificate does not match the name
 */
RVCOREAPI
RvStatus RvTLSSessionCheckCertAgainstName(
    IN  RvTLSSession    *tlsSession,
    IN  RvChar          *name,
    IN  RvMutex         *mtx,
    IN  RvChar          *memBuf,
    IN  RvInt32         memBufLen,
    IN  RvLogMgr        *logMgr)
{
    X509_NAME           *subj;
    X509                *certX509 = NULL;
    RvInt               extcount;
    RvStatus            retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
    const X509V3_EXT_METHOD   *meth   = NULL;
    ASN1_VALUE          *iExtStr= NULL;

#ifdef RV_NULLCHECK
    /* Check input parameters */
    if ((tlsSession == NULL) || (name == NULL) ||
        (mtx == NULL)) {
        return _RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }
#endif
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif
    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionCheckCertAgainstName(tlsSession=%p,name=%s,mtx=%p,memBuf=%p,bufLen=%d,logMgr=%p)",
        tlsSession,name,mtx,memBuf,memBufLen,logMgr));

    RvMutexLock(mtx,logMgr);

    /* Checking the return from SSL_get_peer_certificate here is not strictly
     * necessary.  With our example programs, it is not possible for it to return
     * NULL.  However, it is good form to check the return since it can return NULL
     * if the examples are modified to enable anonymous ciphers or for the server
     * to not require a client certificate.
     */
    certX509 = SSL_get_peer_certificate(tlsSession->sslSession);

    if (certX509 == NULL) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionCheckCertAgainstName: session (%p) failed to retrieve peer certificate",
            tlsSession));
        RvMutexUnlock(mtx,logMgr);
        return retCode;
    }

    if ((extcount = X509_get_ext_count(certX509)) > 0) {
        RvInt i;

        for (i = 0;  i < extcount;  i++) {
            RvChar              *extstr;
            X509_EXTENSION       *ext;

            ext = X509_get_ext(certX509, i);
            extstr = (RvChar *)OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));

            if (!strcmp(extstr, "subjectAltName"))
            {
                RvInt                  j;
                unsigned char        *data;
                STACK_OF(CONF_VALUE) *val;
                CONF_VALUE           *nval;

                if (!(meth = X509V3_EXT_get(ext)))
                    break;

                data = ext->value->data;
                if(meth->it)
                {
                    iExtStr = ASN1_item_d2i(NULL, (d2iUCharPP)&data, ext->value->length, ASN1_ITEM_ptr(meth->it));
                }
                else
                {
                    iExtStr = meth->d2i(NULL, (d2iUCharPP)&data, ext->value->length);
                }
                val = meth->i2v((X509V3_EXT_METHOD*)meth,
                    iExtStr,
                    NULL);

                for (j = 0;  j < sk_CONF_VALUE_num(val);  j++) {
                    nval = sk_CONF_VALUE_value(val, j);
                    if (!strcmp(nval->name, "DNS") && !strcmp(nval->value, name)) {
                        retCode = RV_OK;
                        break;
                    }
                }
                if (val) {
                    sk_CONF_VALUE_pop_free(val,X509V3_conf_free);
                }
            }
            if (retCode == RV_OK) {
                RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                    "RvTLSSessionCheckCertAgainstName(tlsSession=%p,name=%s,mtx=%p,memBuf=%p,bufLen=%d,logMgr=%p)=%d",
                    tlsSession,name,mtx,memBuf,memBufLen,logMgr,retCode));
                goto clean_and_ret;
            }
        }
    }

    if ((subj = X509_get_subject_name(certX509)) &&
        X509_NAME_get_text_by_NID(subj, NID_commonName, memBuf, memBufLen-1) > 0) {
        memBuf[memBufLen-1] = '\0';
        if ((strlen(name) != strlen(memBuf)) || (memcmp(memBuf,name,strlen(name)) != 0)) {
            RvLogWarning(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                "RvTLSSessionCheckCertAgainstName: session (%p) name does not match certificate",
                tlsSession));
            goto clean_and_ret;
        }
        retCode = RV_OK;
    }

clean_and_ret:

    RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionCheckCertAgainstName(tlsSession=%p,name=%s,mtx=%p,memBuf=%p,bufLen=%d,logMgr=%p)=%d",
        tlsSession,name,mtx,memBuf,memBufLen,logMgr,retCode));

#if OPENSSL_VERSION_NUMBER > 0x000907000L
    RvMutexUnlock(mtx,logMgr);
    if (meth && meth->it)
    {
        ASN1_item_free(iExtStr, ASN1_ITEM_ptr(meth->it));
    }
    else
#endif
    {
        if (meth)
        {
            meth->ext_free(iExtStr);
        }
    }

    if(certX509) {
        X509_free(certX509);
    }


    return retCode;
}



/**********************************************************************************
 * RvTLSEngineCheckPrivateKey - checks the consistency of a private key with the
 *                             corresponding certificate loaded into engine
 *
 * INPUT:
 *  tlsEngine   - TLS engine where certificate will be added
 *  mtx         - mutex that protects this TLS
 *                engine memory
 *  logMgr      - log instance
 * OUTPUT: none
 * RETURN: RvStatus - Success or failure
 */
RVCOREAPI
RvStatus RvTLSEngineCheckPrivateKey(
    IN  RvTLSEngine             *tlsEngine,
    IN  RvMutex                 *mtx,
    IN  RvLogMgr                *logMgr)
{
    RvStatus    retCode = RV_OK;

#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    if (mtx == NULL && tlsEngine != NULL)
        /* if mutex was not provided, use the one provided when the TLS engine was constructed */
        mtx = tlsEngine->pMtx;

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineCheckPrivateKey(tlsEngine=%p,mtx=%p,logMgr=%p)",
        tlsEngine,mtx,logMgr));

    RvMutexLock(mtx,logMgr);
    if (SSL_CTX_check_private_key(tlsEngine->ctx) != 1) {
        RvLogWarning(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSEngineCheckPrivateKey: engine (%p), failed to add X509 certificate to the chain",
            tlsEngine));
        retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
        RvMutexUnlock(mtx,logMgr);
        return retCode;
    }

    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineCheckPrivateKey(tlsEngine=%p,mtx=%p,logMgr=%p)=%d",
        tlsEngine,mtx,logMgr,retCode));

    return retCode;
}

/* Visits all subjectAltName DNS names
 * Returns RV_TRUE when done
 */

typedef RvBool (*RvDNSNamesVisitorCB)(RvChar *name, void *userData);

RvStatus RvX509VisitDNSNames(X509 *cert, RvDNSNamesVisitorCB visitor, void *userData) {
    RvInt extCount;
    RvInt i;
    RvBool done = RV_FALSE;

    /* SubjectAltNames are present in X509 v3 extensions */
    extCount = X509_get_ext_count(cert);

    /* loop thru all extensions in this certificate */
    for(i = 0; (i < extCount) && !done; i++) {
        const RvChar *extName;
        X509_EXTENSION *ext;
        RvInt j;
        RvUint8 *data;
        STACK_OF(CONF_VALUE) *val;
        CONF_VALUE *nval;
        RvInt nVals;
        const X509V3_EXT_METHOD *meth;
        ASN1_VALUE *iExtStr = 0;



        /* Fetch extension name */
        ext = X509_get_ext(cert, i);
        extName = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));


        if(strcmp(extName, SN_subject_alt_name))
            continue;

        if (!(meth = X509V3_EXT_get(ext)))
            break;

        data = ext->value->data;

        if(meth->it) {
            iExtStr = ASN1_item_d2i(NULL, (d2iUCharPP)&data, ext->value->length, ASN1_ITEM_ptr(meth->it));
        } else {
            iExtStr = meth->d2i(NULL, (d2iUCharPP)&data, ext->value->length);
        }

        val = meth->i2v((X509V3_EXT_METHOD*)meth, iExtStr, NULL);
        nVals = sk_CONF_VALUE_num(val);

        for (j = 0;  j < nVals && !done;  j++) {
            RvChar *altName;

            nval = sk_CONF_VALUE_value(val, j);
            /* We're interested (by now) only in DNS alt names */
            if (strcmp(nval->name, "DNS") != 0) {
                continue;
            }
            altName = nval->value;
            done = visitor(altName, userData);
        }

        if (val) {
            sk_CONF_VALUE_pop_free(val, X509V3_conf_free);
        }

        if(meth->it) {
            ASN1_item_free(iExtStr, ASN1_ITEM_ptr(meth->it));
        } else {
            meth->ext_free(iExtStr);
        }
    }
    return RV_OK;
}

typedef struct {
    RvChar   *buf;
    RvInt32   bufLeft;
    RvSize_t  nItems;
} RvGetDNSData;

RvBool RvDNSNamesGetVisitor(RvChar *altName, void *userData) {
    RvGetDNSData *d = (RvGetDNSData *)userData;
    RvSize_t altNameLen = strlen(altName) + 1;

    d->bufLeft -= (RvInt32)altNameLen;
    /* If there left buffer space, copy new item to the buffer */
    if(d->bufLeft >= 0) {
        d->nItems++;
        strcpy(d->buf, altName);
        d->buf += altNameLen;
    }

    return RV_FALSE;
}

RVCOREAPI
RvStatus RvX509GetSubjectAltDNS(X509 *cert, RvChar *buf, RvSize_t *pbufSize, RvSize_t *pNitems) {
    RvGetDNSData d;
    RvStatus       s;

    d.buf = buf;
    d.bufLeft = (RvInt32)*pbufSize;
    d.nItems = 0;

    s = RvX509VisitDNSNames(cert, RvDNSNamesGetVisitor, &d);
    if(s != RV_OK) {
        return s;
    }

    *pNitems = d.nItems;
    *pbufSize -= d.bufLeft;
    if(d.bufLeft < 0) {
        return RV_ERROR_OUTOFRESOURCES;
    }

    return RV_OK;
}

RVCOREAPI
RvStatus RvTLSSessionGetSubjectAltDNS(RvTLSSession *sess, RvChar *buf, RvSize_t *pbufSize, RvSize_t *pNitems, RvLogMgr *logMgr) {

#define API_NAME "RvTLSSessionGetSubjectAltDNS"
#define API_FORMAT_STRING API_NAME "(tlsSession=%p, buf=%p, bufSize=%d)"
#define LOG_SRC RvTLSLogSrc(logMgr)

    X509 *cert;
    RvStatus s;

#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

    RvLogEnter(LOG_SRC, (LOG_SRC, API_FORMAT_STRING, sess, buf, *pbufSize));

    cert = SSL_get_peer_certificate(sess->sslSession);
    if(cert == 0) {
        RvLogWarning(LOG_SRC, (LOG_SRC, API_NAME ": no certificate in session %p", sess));
        *pNitems = 0;
        return RV_OK;
    }

    s = RvX509GetSubjectAltDNS(cert, buf, pbufSize, pNitems);
    if(s != RV_OK) {
        RvLogError(LOG_SRC, (LOG_SRC, API_NAME ": failed to retrieve names"));
        return s;
    }

    X509_free(cert);
    RvLogLeave(LOG_SRC, (LOG_SRC, API_NAME));
    return RV_OK;

#undef LOG_SRC
#undef API_FORMAT_STRING
#undef API_NAME
}

#if 0
/**********************************************************************************
* RvTLSSessionGetSessionId - retrieves session id
*
* INPUT:
*	IN    sess	   - Connected TLS session
*  IN    buf      - buffer that will contain zero-ended session id.
*  INOUT pBufSize - pointer to the size of buffer. On return contains requested buffer size
*  logMgr		-    log instance
* RETURN:
*	RV_OK - for success
*  RV_ERROR_OUTOFRESOURCES - not enough buffer space. In this case *pbufSize will hold the requested
*                            buffer size.
*
*/
RVCOREAPI
RvStatus RvTLSSessionGetSessionId(RvTLSSession *sess, RvChar *buf, RvSize_t *pbufSize, RvLogMgr *logMgr)

{
#define API_NAME "RvTLSSessionGetSessionId"
#define API_FORMAT_STRING API_NAME "(tlsSession=%p, buf=%p, bufSize=%d)"
#define LOG_SRC RvTLSLogSrc(logMgr)

	X509 *cert;
	RvStatus rv;
	const char *sessionId;
	unsigned int len;
	SSL_SESSION *sslSession;
#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#endif

	*pbufSize = 0;

	RvLogEnter(LOG_SRC, (LOG_SRC, API_FORMAT_STRING, sess, buf, *pbufSize));
	if (sess == NULL)
	{
		RvLogWarning(LOG_SRC, (LOG_SRC, API_NAME ": session ptr is not initialized"));
		RvLogLeave(LOG_SRC, (LOG_SRC, API_NAME));
		return RV_ERROR_NULLPTR;
	}
	cert = SSL_get_peer_certificate(sess->sslSession);
	if(cert == 0) {
		RvLogWarning(LOG_SRC, (LOG_SRC, API_NAME ": no certificate in session %p", sess));
		RvLogLeave(LOG_SRC, (LOG_SRC, API_NAME));
		return RV_OK;
	}

	sslSession = SSL_get_session(sess->sslSession);
	if (sslSession == NULL)
	{
		RvLogWarning(LOG_SRC, (LOG_SRC, API_NAME ": session ptr is not initialized"));
		rv = RV_ERROR_NULLPTR;
	}
	else /* Session is not NULL*/
	{
		sessionId = (const char *)SSL_SESSION_get_id(sslSession, &len);
		if(len > 0)
		{
			/*  copy the session id to the buffer */
			len += 1;
			if(len <= *pbufSize)
			{
				strcpy(buf, sessionId);
			}
			else
			{
				RvLogWarning(LOG_SRC, (LOG_SRC, API_NAME ": not enough buffer. The session id length is %", len));
				rv = RV_ERROR_INSUFFICIENT_BUFFER;
			}
		}
		else
		{
			RvLogWarning(LOG_SRC, (LOG_SRC, API_NAME ": failed to retrieve session id"));
			rv = RV_ERROR_UNKNOWN;
		}
		*pbufSize = len;
	}	/* END IF: Session is not NULL*/
	X509_free(cert);
	RvLogLeave(LOG_SRC, (LOG_SRC, API_NAME));
	return RV_OK;

#undef LOG_SRC
#undef API_FORMAT_STRING
#undef API_NAME
}
#endif

/**********************************************************************************
 * rvTLSGetError - retrieves ASN1 format certificate from a certificate
 * context received by a certificate verification callback.
 *
 * INPUT:
 *  tlsSession  - TLS session structure
 *  sslRetCode  - OpenSSL function return code
 *  mtx         - tls session mutex
 * OUTPUT:
 *  errId       - retrieved TLS error code
 * RETURN:
 *  none
 */
static void rvTLSGetError(
    IN  RvTLSSession    *tlsSession,
    IN  RvInt           sslRetCode,
    IN  RvMutex         *mtx,
    OUT RvInt           *errId)
{
    RvMutexLock(mtx,NULL);
    *errId = SSL_get_error(tlsSession->sslSession,sslRetCode);
    RvMutexUnlock(mtx,NULL);

    /*
    switch(*errId) {
    case SSL_ERROR_ZERO_RETURN:
        strncpy(errStr,"SSL_ERROR_ZERO_RETURN",errStrLen);
        break;
    case SSL_ERROR_WANT_READ:
        strncpy(errStr,"SSL_ERROR_WANT_READ",errStrLen);
        break;
    case SSL_ERROR_WANT_WRITE:
        strncpy(errStr,"SSL_ERROR_WANT_WRITE",errStrLen);
        break;
    case SSL_ERROR_WANT_CONNECT:
        strncpy(errStr,"SSL_ERROR_WANT_CONNECT",errStrLen);
        break;
    case SSL_ERROR_WANT_X509_LOOKUP:
        strncpy(errStr,"SSL_ERROR_WANT_X509_LOOKUP",errStrLen);
        break;
    case SSL_ERROR_SYSCALL:
        strncpy(errStr,"SSL_ERROR_SYSCALL",errStrLen);
        break;
    case SSL_ERROR_SSL:
        strncpy(errStr,"SSL_ERROR_SSL",errStrLen);
        break;
    case SSL_ERROR_NONE:
        strncpy(errStr,"SSL_ERROR_NONE",errStrLen);
        break;
    default:
        strncpy(errStr,"UNKNOWN",errStrLen);
        break;
    }
    */
}

/******************************************************************************
 * rvIsWriteEnabled - check if call to SSL_write() should be blocked due to
 *                    a number of reasons.
 *
 * INPUT:
 *	tlsSession	- Connected TLS session
 *  logMgr		- log instance
 * OUTPUT:
 *  pPendingTlsEvents - TLS events that should be handled before
 *                      write will be enabled
 *	pStrWriteBlockRsn - if write is not enabled - reason in string format
 * RETURN:
 *	RV_TRUE - if the renegotiation was completed, RV_FALSE - otherwise
 *
 */
static RvBool rvIsWriteEnabled(
    IN  RvTLSSession* tlsSession,
    OUT RvTLSEvents*  pPendingTlsEvents,
    OUT RvChar**      pStrWriteBlockRsn)
{
    /* Block SSL_write, if previous SSL_read was not completed */
    if (tlsSession->requiredForTLSRead != 0)
    {
        /* Add TLS_WRITE to the waiting list in order to enable it later */
        tlsSession->pendingTlsEvents |= RV_TLS_WRITE_EV;
        *pStrWriteBlockRsn = "previous SSL_read was not completed";
        return RV_FALSE;
    }

    /* Block SSL_write, if previous SSL_write in sequence of SSL_writes,
       called on single WRITE event, succeeded.
       This should prevent following deadlock in exchange of data between
       local and remote peer:
          1. A batch of SSL_writes, caused by processing of one WRITE event,
             fills the socket send buffer.
          2. As a result of (1) the last SSL_write is blocked on WANT_WRITE.
          3. Blocked SSL_write prevents call to SSL_read.
          4. As a result of (3) socket send buffer of the remote peer
             is not freed, causing SSL_write to be blocked on WANT_WRITE on
             remote peer.
          5. Both sides of session become to be stuck on writing. This causes
             them not to read -> not to free write buffers of opposite side.*/
    if (tlsSession->counterWrites >= MAX_SUBSEQUENT_WRITES)
    {
        /* Ask application to register to WRITE again */
        *pPendingTlsEvents |= RV_TLS_WRITE_EV;
        *pStrWriteBlockRsn = "MAX_SUBSEQUENT_WRITES was reached";
        return RV_FALSE;
    }

    return RV_TRUE;
}

/**********************************************************************************
 * rvLockingCallback - locking callback of the OpenSSL
 *
 * INPUT:
 *  mode        - operation to apply on mutex (lock or unlock )
 *  type        - mutex ID
 *  file        - OpenSSL code file name
 *  line        - OpenSSL code line number
 * OUTPUT: none
 * RETURN: none
 */
static void rvLockingCallback(
    IN RvInt        mode,
    IN RvInt        type,
    IN RvChar       *file,
    IN RvInt        line)
{
    RV_UNUSED_ARG(file);
    RV_UNUSED_ARG(line);

  if (mode & CRYPTO_LOCK) {
      RvMutexLock(&(openSSLMutexes[type]),NULL);
      return;
  }

  RvMutexUnlock(&(openSSLMutexes[type]),NULL);
}

/**********************************************************************************
* RvTlsSockSelfSignedCertAllowed - allow self-signed certificates
*   This function might be given as the certificate verification callback
*   used by RvTLSSessionClientHandshake/RvTLSSessionServerHandshake.
*   This function will modify the OpenSSL suggested verification result from 0 to 1
*   (from failure to success) if the OpenSSL error is X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT.
*   That is it overrides the default OpenSSL behavior (failure) when the peer cerificate
*   is self-signed and the key is untrusted.
*
* INPUT:
* RETURN:
*	1 if the OpenSSL error was X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT, otherwise
*   preserves the OpenSSL suggested value of 'prevErrro'.
*
*/
RVCOREAPI RvInt RvTlsSockSelfSignedCertAllowed(
    IN int  prevErrro,
    IN void *certCtx)
{
    X509_STORE_CTX *crt = (X509_STORE_CTX *)certCtx;
    int err;

    if (prevErrro == 0)
    {
        err=X509_STORE_CTX_get_error(crt);
        if (err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
            prevErrro = 1;
    }
    return prevErrro;
}



RVCOREAPI
void RvTLSEngineCfgInit(
    INOUT RvTLSEngineCfg *pCfg)
{
    memset(pCfg,0,sizeof(RvTLSEngineCfg));
    pCfg->certDepth = RV_TLS_DEFAULT_CERT_DEPTH;
    pCfg->method = RV_TLS_TLS_V1;
}

RVCOREAPI
RvStatus RvTLSEngineConstructEx(
    IN  RvTLSEngineCfg*         pTlsEngCfg,
    OUT RvTLSEngineEx**         ppTlsEng)
{
    RvStatus rvs;
    RvLogMgr *pLogMgr = (RvLogMgr*) pTlsEngCfg->pRvLogMgr;
    RvTLSEngineEx*         pTlsEng;
    RvChar                 certStr[8192]     = {'\0'};
    RvChar*                pCert = NULL;
    RvChar                 privKey[8192]     = {'\0'};
    RvChar*                pPrivKey = NULL;
    RvInt                  privKeyLen = 0, certLen = 0;

    rvs = RvMemoryAlloc(NULL,sizeof(RvTLSEngineEx),pTlsEngCfg->pRvLogMgr,(void**)ppTlsEng);
    if (rvs != RV_OK)
    {
        RvLogError(RvTLSLogSrc(pLogMgr), (RvTLSLogSrc(pLogMgr),
            "RvTLSEngineConstructEx failed to allocate memory"));
        return RV_ERROR_OUTOFRESOURCES;
    }
    pTlsEng = *ppTlsEng;
    memset(pTlsEng,0,sizeof(RvTLSEngineEx));

    rvs = RvMutexConstruct(pLogMgr,&pTlsEng->tlsEngMtx);
    if (rvs != RV_OK)
    {
        RvLogError(RvTLSLogSrc(pLogMgr), (RvTLSLogSrc(pLogMgr),
            "RvTLSEngineConstructEx failed in RvMutexConstruct"));
        RvMemoryFree(pTlsEng,pLogMgr);
        return RV_ERROR_OUTOFRESOURCES;
    }

    memcpy(&pTlsEng->tlsEngCfg,pTlsEngCfg,sizeof(*pTlsEngCfg));

    pPrivKey = pTlsEngCfg->privKey;
    privKeyLen = pTlsEngCfg->privKeyLen;

    /* load the private key if the file name provided */
    if (pPrivKey == NULL && pTlsEngCfg->privKeyFileName != NULL)
    {
        BIO                                 *inKey          = NULL;
        EVP_PKEY                            *pkey           = NULL;

        inKey=BIO_new(BIO_s_file_internal());
        if (BIO_read_filename(inKey,pTlsEngCfg->privKeyFileName) <= 0)
        {
            RvLogError(RvTLSLogSrc(pLogMgr), (RvTLSLogSrc(pLogMgr),
                "RvTLSEngineConstructEx: failed to load the private key from the file %s",pTlsEngCfg->privKeyFileName));
            goto failure;
        }
        pkey=PEM_read_bio_PrivateKey(inKey,NULL,NULL,NULL);
        pPrivKey = privKey;
        privKeyLen = i2d_PrivateKey(pkey,(RvUint8**)&pPrivKey);
        pPrivKey = privKey;
        BIO_free(inKey);
    }

    pCert = pTlsEngCfg->cert;
    certLen = pTlsEngCfg->certLen;

    /* load the certificate if the file name provided */
    if (pCert == NULL && pTlsEngCfg->certFileName != NULL)
    {
        BIO                                 *inCert         = NULL;
        X509                                *x509           = NULL;

        inCert=BIO_new(BIO_s_file_internal());
        if (BIO_read_filename(inCert,pTlsEngCfg->certFileName) <= 0)
        {
            RvLogError(RvTLSLogSrc(pLogMgr), (RvTLSLogSrc(pLogMgr),
                "RvTLSEngineConstructEx: failed to load the certificate from the file %s",pTlsEngCfg->certFileName));
            goto failure;
        }

        x509=PEM_read_bio_X509(inCert,NULL,NULL,NULL);
        pCert = certStr;
        certLen = i2d_X509(x509,(RvUint8**)&pCert);
        pCert = certStr;
        BIO_free(inCert);
    }


    rvs = RvTLSEngineConstruct(
        pTlsEngCfg->method,
        pPrivKey,
        RV_TLS_RSA_KEY,
        privKeyLen,
        pCert,
        certLen,
        pTlsEngCfg->certDepth,
        &pTlsEng->tlsEngMtx,
        pLogMgr,
        &pTlsEng->tlsEng);

    if (rvs != RV_OK)
    {
        RvLogError(RvTLSLogSrc(pLogMgr), (RvTLSLogSrc(pLogMgr),
            "RvTLSEngineConstructEx: RvTLSEngineConstruct failed",pTlsEngCfg->certFileName));
        goto failure;
    }

    return RV_OK;

failure:

    RvMutexDestruct(&pTlsEng->tlsEngMtx,pLogMgr);
    RvMemoryFree(pTlsEng,pLogMgr);

    return RV_ERROR_UNKNOWN;
}

RVCOREAPI
void RvTLSEngineDestructEx(
    IN RvTLSEngineEx*         pTlsEng)
{
    RvLogMgr* pLogMgr;

    pLogMgr = (RvLogMgr*)pTlsEng->tlsEngCfg.pRvLogMgr;
    RvTLSEngineDestruct(&pTlsEng->tlsEng,&pTlsEng->tlsEngMtx,pLogMgr);
    RvMutexDestruct(&pTlsEng->tlsEngMtx,pLogMgr);
    RvMemoryFree(pTlsEng,pLogMgr);
}




RVCOREAPI
RvStatus RvTLSGetCertificateFingerprint(
    IN    RvChar*                cert,
    IN    RvInt                  certLen,
    IN    RvChar*                certFilename,
    IN    RvHashFuncEnum         hash,
    INOUT RvChar*                fingerprint,
    INOUT RvInt*                 fingerprintLen)
{
    BIO                                 *inCert         = NULL;
    X509                                *x509           = NULL;
    RvChar                              certStr[8192]     = {'\0'};
    const EVP_MD *digest;
    const RvChar* hashTxt;
    int ret;

    if (!cert && !certFilename)
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);

    if (cert == NULL)
    {
        inCert=BIO_new(BIO_s_file_internal());
        if (BIO_read_filename(inCert,certFilename) <= 0)
        {
            return RvTLSErrorCode(RV_TLS_ERROR_GEN);
        }
        x509=PEM_read_bio_X509(inCert,NULL,NULL,NULL);
        cert = certStr;
        certLen = i2d_X509(x509,(RvUint8**)&cert);
        cert = certStr;
        BIO_free(inCert);
    }
    else {
        if (d2i_X509(&x509,(d2iUCharPP)&cert,certLen) == NULL)
        {
            return RvTLSErrorCode(RV_TLS_ERROR_GEN);
        }
    }

    hashTxt = RvHashFuncEnum2Str(hash,RV_FALSE,RV_FALSE);
    digest = EVP_get_digestbyname(hashTxt);
    if (digest == NULL)
    {
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    ret = X509_digest(x509,digest,(unsigned char*)fingerprint,(unsigned int*)fingerprintLen);
    X509_free(x509);

    if (!ret)
    {
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    return RV_OK;
}


#endif /* (RV_TLS_TYPE != RV_TLS_NONE) */

RVCOREAPI
RvHashFuncEnum RvHashFuncStr2Enum(
                                  IN const RvChar* txt)
{
    if (RvStrncasecmp(txt,"sha",3) == 0 && (txt[3] == '-' || isdigit((RvInt)txt[3])))
    {
        int offs = 3;
        if (txt[3] == '-')
            offs++;
        if (strcmp(txt+offs,"1") == 0)
            return RvHashFuncSha1;
        else if (strcmp(txt+offs,"224") == 0)
            return RvHashFuncSha224;
        else if (strcmp(txt+offs,"256") == 0)
            return RvHashFuncSha256;
        else if (strcmp(txt+offs,"384") == 0)
            return RvHashFuncSha384;
        else if (strcmp(txt+offs,"512") == 0)
            return RvHashFuncSha512;
        goto end;
    }
    else if (RvStrncasecmp(txt,"md",2) == 0)
    {
        if (strcmp(txt+2,"2") == 0)
            return RvHashFuncMd2;
        else if (strcmp(txt+2,"5") == 0)
            return RvHashFuncMd5;
        goto end;
    }
end:
    return RvHashFuncUndefined;
}

RVCOREAPI
const RvChar* RvHashFuncEnum2Str(
                                 IN RvHashFuncEnum v,
                                 IN RvBool upperCase,
                                 IN RvBool useHyphen)
{
    switch (v)
    {
    case RvHashFuncSha1:
        if (upperCase)
            return (useHyphen)?"SHA-1":"SHA1";
        else
            return (useHyphen)?"sha-1":"sha1";
    case RvHashFuncSha224:
        if (upperCase)
            return (useHyphen)?"SHA-224":"SHA224";
        else
            return (useHyphen)?"sha-224":"sha224";
    case RvHashFuncSha256:
        if (upperCase)
            return (useHyphen)?"SHA-256":"SHA256";
        else
            return (useHyphen)?"sha-256":"sha256";
    case RvHashFuncSha384:
        if (upperCase)
            return (useHyphen)?"SHA-384":"SHA384";
        else
            return (useHyphen)?"sha-384":"sha384";
    case RvHashFuncSha512:
        if (upperCase)
            return (useHyphen)?"SHA-512":"SHA512";
        else
            return (useHyphen)?"sha-512":"sha512";
    case RvHashFuncMd5:
        if (upperCase)
            return "MD5";
        else
            return "md5";
    case RvHashFuncMd2:
        if (upperCase)
            return "MD2";
        else
            return "md2";
    default:
        return "undefined";
    }
}


