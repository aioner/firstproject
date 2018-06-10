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
#include <string.h>

 
#if (RV_TLS_TYPE != RV_TLS_NONE)

#if (OPENSSL_VERSION_NUMBER == 0x0090800fL)
typedef const unsigned char** d2iUCharPP;
#else
typedef unsigned char** d2iUCharPP;
#endif
 
/* Defines TLS error code to simplify TLS module functions */
#define RvTLSErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_TLS, (_e))

/* Defines macro to retrive TLS log source pointer from the log manager */
#define RvTLSLogSrc(_m) (_m==NULL?NULL:(&(_m->TLSSource)))


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
    SSL_METHOD  *meth;
    RvInt       sslRet;
    RvStatus    stat = RV_OK;


#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsEngine == NULL) || (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineConstruct(method=%d,privKey=%p,keyType=%d,keyLen=%d,cert=%p,certLen=%d,certDepth=%d,mtx=%p,logMgr=%p,tlsEng=%p)",
        method,privKey,privKeyType,privKeyLen,cert,certLen,certDepth,mtx,logMgr,tlsEngine));

    stat = RvMutexLock(mtx,logMgr);
    if (stat != RV_OK) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSEngineConstruct: engine (0x%x) mutex lock error",
            tlsEngine));
        return stat;
    }

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
            "RvTLSEngineConstruct: engine (0x%x) invalid method (%d)",
            tlsEngine,method));
        RvMutexUnlock(mtx,logMgr);
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }

    /* Initiate OpenSSL context */
    tlsEngine->ctx = SSL_CTX_new(meth);

    if (tlsEngine->ctx == NULL) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSEngineConstruct: engine (0x%x) failed to create OpenSSL context",
            tlsEngine));
        RvMutexUnlock(mtx,logMgr);
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    if ((privKey != NULL) && (privKeyLen > 0)) {
      switch(privKeyType) {
      case RV_TLS_RSA_KEY:
        sslRet = SSL_CTX_use_PrivateKey_ASN1(EVP_PKEY_RSA,tlsEngine->ctx,
                         (unsigned char *)privKey, privKeyLen);
        if (!sslRet) {
          RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                           "RvTLSEngineConstruct: engine (0x%x) failed to load private key",
                           tlsEngine));
          RvMutexUnlock(mtx,logMgr);
          return RvTLSErrorCode(RV_TLS_ERROR_GEN);
        }
        break;
      default:
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineConstruct: engine (0x%x) wrong private key type (%d)",
                         tlsEngine,privKeyType));
        RvMutexUnlock(mtx,logMgr);
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
                         "RvTLSEngineConstruct: engine (0x%x) failed to load local certificate",
                         tlsEngine));
        RvMutexUnlock(mtx,logMgr);
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
      }
    }

    SSL_CTX_set_verify(tlsEngine->ctx, SSL_VERIFY_NONE, NULL);
    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineConstruct(method=%d,privKey=%p,keyType=%d,keyLen=%d,cert=%p,certLen=%d,certDepth=%d,mtx=%p,logMgr=%p,tlsEng=%p)=%d",
        method,privKey,privKeyType,privKeyLen,cert,certLen,certDepth,mtx,logMgr,tlsEngine,RV_OK));

    RvMutexUnlock(mtx,logMgr);

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


#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((tlsEngine == NULL) || (mtx == NULL) ||
        (cert == NULL) || (certLen <= 0)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineAddAutorityCertificate(tlsEng=%p,cert=%p,certLen=%d,mtx=%p,logMgr=%p)",
        tlsEngine,cert,certLen,mtx,logMgr));

    if (d2i_X509(&xCert,(d2iUCharPP)&cert,certLen) == NULL) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineAddAutorityCertificate: engine (0x%x), failed to convert ASN1 certificate to X509 format",
                         tlsEngine));
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    RvMutexLock(mtx,logMgr);

    if (X509_STORE_add_cert(SSL_CTX_get_cert_store(tlsEngine->ctx), xCert) != 1) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineAddAutorityCertificate: engine (0x%x), failed to convert add X509 certificate to the engine",
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

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((tlsEngine == NULL) || (mtx == NULL) ||
        (underlying == NULL) ) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
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

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((tlsEngine == NULL) || (mtx == NULL) ||
        (cert == NULL) || (certLen <= 0)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineAddCertificate(tlsEng=%p,cert=%p,certLen=%d,mtx=%p,logMgr=%p)",
        tlsEngine,cert,certLen,mtx,logMgr));

    if (d2i_X509(&xCert,(d2iUCharPP)&cert,certLen) == NULL) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineAddCertificate: engine (0x%x), failed to convert ASN1 certificate to X509 format",
                         tlsEngine));
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }


    RvMutexLock(mtx,logMgr);
    if (SSL_CTX_add_extra_chain_cert(tlsEngine->ctx,xCert) != 1) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineAddCertificate: engine (0x%x), failed to add X509 certificate to the chain",
                         tlsEngine));
        retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
        RvMutexUnlock(mtx,logMgr);
        return retCode;
    }

    RvMutexUnlock(mtx,logMgr);

    if(xCert) {
        X509_free(xCert);
    }    

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
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((tlsEngine == NULL) || (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineDestruct(tlsEng=%p,mtx=%p,logMgr=%p)",
        tlsEngine,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

    if (tlsEngine->ctx == NULL) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSEngineDestruct: engine (0x%x), failed to destruct, wrong OpenSSL context",
                         tlsEngine));
        RvMutexUnlock(mtx,logMgr);
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }

    SSL_CTX_flush_sessions(tlsEngine->ctx,0);
    SSL_CTX_free(tlsEngine->ctx);

    RvMutexUnlock(mtx,logMgr);

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
    RvStatus stat;
    SSL      *sslSession;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input */
    if ((tlsEngine == NULL) || (tlsSession == NULL) ||
        (mtxEngine == NULL) || (mtxSession == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionConstruct(tlsEng=%p,mtxEng=%p,mtxSession=%p,logMgr=%p,tlsSession=%p)",
        tlsEngine,mtxEngine,mtxSession,logMgr,tlsSession));

    stat = RvMutexLock(mtxEngine,logMgr);
    if (stat != RV_OK) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionConstruct: engine (0x%x), session (0x%x) engine lock failure",
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
						     "RvTLSSessionConstruct: engine (0x%x) session (0x%x) , general error - %s, file - %s, line - %d",
						     tlsEngine,tlsSession,err,file,line));
		}
		else {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
						     "RvTLSSessionConstruct: engine (0x%x) session (0x%x) general eError",
						     tlsEngine,tlsSession));
        }
        RvMutexUnlock(mtxEngine,logMgr);
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    RvMutexUnlock(mtxEngine,logMgr);

    stat = RvMutexLock(mtxSession,logMgr);
    if (stat != RV_OK) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionConstruct: engine (0x%x), session (0x%x) session lock failure",
            tlsEngine,tlsSession));
        return stat;
    }

    tlsSession->requiredForHandshake    = 0;
    tlsSession->requiredForTLSRead      = 0;
    tlsSession->requiredForTLSWrite     = 0;
    tlsSession->requiredForTLSShutdown  = 0;
    tlsSession->tlsEvents               = 0;
    tlsSession->bio = NULL;     /* important since when applying handshake
                                this parameter specifies if the handshake
                                was applied for the first time or as a result of
                                previouse 'will block' failure */
    tlsSession->sslSession = sslSession;
	tlsSession->renegState = RV_TLS_RENEG_NONE;
	tlsSession->stickyEvent = RV_TLS_NONE_EV;

    RvMutexUnlock(mtxSession,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionConstruct(tlsEng=%p,mtxEng=%p,mtxSession=%p,logMgr=%p,tlsSession=%p)=%d",
        tlsEngine,mtxEngine,mtxSession,logMgr,tlsSession,RV_OK));

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


    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionClientHandshake(tlsSession=%p,certCB=%p,sock=%d,mtx=%p,logMgr=%p)",
        tlsSession,certCB,tcpSock,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

	ERR_clear_error();

    /* In case handshake is applied for the first time,
    BIO object should be created and certificate verification
    should be set according to input */
    if (tlsSession->bio == NULL) {
        tlsSession->bio = BIO_new_socket(tcpSock,BIO_NOCLOSE);
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
                          "RvTLSSessionClientHandshake: session (0x%x) SSL_RET_WANT_READ failure",
                          tlsSession));
          tlsSession->requiredForHandshake |= RV_SELECT_READ;
          break;
        case SSL_ERROR_WANT_WRITE:
          RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                          "RvTLSSessionClientHandshake: session (0x%x) SSL_RET_WANT_WRITE failure",
                          tlsSession));
          tlsSession->requiredForHandshake |= RV_SELECT_WRITE;
          break;
        default:
          if (ERR_get_error_line_data((const char **)&file,
                          &line, (const char **)&err,
                          &errFlags) != 0) {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                             "RvTLSSessionClientHandshake: session (0x%x) , general error - %s, file - %s, line - %d",
                             tlsSession,err,file,line));
          }
          else {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                             "RvTLSSessionClientHandshake: session (0x%x) general eError",
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
 *                I/O operatation. Currently unused
 *                if RV_TRUE - no applicationI/O is possible during renegotiation process,
 *                e.g. next message from the peer should be handshake protocol message
 *	mtx			- Mutex that protects session structure
 *  logMgr		- log instance
 * RETURN: 
 *	RvStatus	- Success, WilllBlock or failure
 *                  
 */ 


RVCOREAPI
RvStatus RvTLSSessionRenegotiate(RvTLSSession *rvSession, RvBool forced, RvMutex *mtx, RvLogMgr *logMgr) {
	RvStatus s;

	s = RV_OK;
    RV_UNUSED_ARG(forced);
	RvMutexLock(mtx, logMgr);

	if(rvSession->renegState == RV_TLS_RENEG_NONE) {
		rvSession->renegState = RV_TLS_RENEG_NEEDED;
	}

	RvMutexUnlock(mtx, logMgr);
	return s;
}

#endif


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


    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionServerHandshake(tlsSession=%p,certCB=%p,sock=%d,mtx=%p,logMgr=%p)",
        tlsSession,certCB,tcpSock,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

	ERR_clear_error();

    /* In case handshake is applied for the first time,
    BIO object should be created and certificate check model
    should be set */
    if (tlsSession->bio == NULL) {
        tlsSession->bio = BIO_new_socket(tcpSock,BIO_NOCLOSE);
        SSL_set_bio(tlsSession->sslSession,tlsSession->bio,tlsSession->bio);

        if (certCB != NULL) {
        /* Note that when initialaizing tlsEngine
        we set default certificate check to none so
            in case clientCert is 'FALSE' nothing should be done */
            SSL_set_verify(tlsSession->sslSession,
                       SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       (int (*)(int, X509_STORE_CTX *))certCB);
        }
    }

    if ((sslRetCode = SSL_accept(tlsSession->sslSession)) <= 0) {
        /* it is supported that most failures are result of
        'will block' condition. In other cases error code
        will be replaced bu the 'generall error' */
        stat = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);

        rvTLSGetError(tlsSession,sslRetCode,mtx,&sslErrCode);
        switch(sslErrCode) {
        case SSL_ERROR_WANT_READ:
          RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                          "RvTLSSessionServerHandshake: session (0x%x) SSL_RET_WANT_WRITE failure",
                          tlsSession));
          tlsSession->requiredForHandshake |= RV_SELECT_READ;
          break;
        case SSL_ERROR_WANT_WRITE:
          RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                          "RvTLSSessionServerHandshake: session (0x%x) SSL_RET_WANT_WRITE failure",
                          tlsSession));
          tlsSession->requiredForHandshake |= RV_SELECT_WRITE;
          break;
        default:
          if (ERR_get_error_line_data((const char **)&file, &line,
                          (const char **)&err,
                          &errFlags) != 0) {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                             "RvTLSSessionServerHandshake: session (0x%x) general error - %s, file - %s, line - %d",
                             tlsSession,err,file,line));
          }
          else {
            RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                             "RvTLSSessionServerHandshake: session (0x%x) general error",
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
 *  tlsSession      - Connected TLS session
 *  receiveBuf      - receive buffer
 *  mtx             - Mutex that protects session
 *                    structure
 *  logMgr          - log instance
 *  receiveBufLen   - receive buffer maximum length
 * OUTPUT:
 *  receiveBufLen   - length of received data
 * RETURN: RvStatus - Success, WilllBlock, Pending or failure
 */
RVCOREAPI
RvStatus RvTLSSessionReceiveBuffer(
    IN    RvTLSSession  *tlsSession,
    IN    RvChar        *receiveBuf,
    IN    RvMutex       *mtx,
    IN    RvLogMgr      *logMgr,
    INOUT RvInt         *receiveBufLen)
{
    RvInt       sslRetCode;
    RvStatus    retCode = RV_OK;
    RvInt       sslErrCode  = SSL_ERROR_NONE;
    RvChar      *file;
    RvChar      *err;
    RvInt       line;
    RvInt       errFlags;
    RvInt       shutdownStatus;

#if RV_TLS_ENABLE_RENEGOTIATION

	RvBool      renegPending = RV_FALSE;

#endif

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL)    || (receiveBuf == NULL) ||
        (receiveBufLen == NULL) || (mtx == NULL)        ||
        (*receiveBufLen <= 0)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionReceiveBuffer(tlsSession=%p,receiveBuf=%p,mtx=%p,logMgr=%p,bufLen=%d)",
        tlsSession,receiveBuf,mtx,logMgr,*receiveBufLen));

    RvMutexLock(mtx,logMgr);

#if RV_TLS_ENABLE_RENEGOTIATION
    
	/* I try to perform whole renegotiation while OpenSSL library is prepared to 
	 *  accept application data. This happens only when reneg is performed during
	 *  SSL_read. On the other hand, renegotiation is not started as far as there is
	 *  data in internal read or write buffer. So this check is performed in order to start
	 *  reneg only when write buffer is empty.
	 */
	if(tlsSession->renegState == RV_TLS_RENEG_NEEDED && tlsSession->requiredForTLSWrite == 0) {
		SSL_renegotiate(tlsSession->sslSession);
		/* from my point of view, renegotiation is started. Actually, it may be started 
		 *  lately. But from this point and untile renegotiation is done
		 *  I will only raise RV_TLS_READ_EV event
		 */
		 tlsSession->renegState = RV_TLS_RENEG_STARTED;
		 tlsSession->stickyEvent = RV_TLS_READ_EV;
	}

#endif

    sslRetCode = SSL_read(tlsSession->sslSession,receiveBuf,*receiveBufLen);

#if RV_TLS_ENABLE_RENEGOTIATION

	renegPending =  SSL_renegotiate_pending(tlsSession->sslSession);
	/* Check whether we accepted renegotiation request from our peer */
	if(tlsSession->renegState <= RV_TLS_RENEG_NEEDED && renegPending) {
		tlsSession->renegState = RV_TLS_RENEG_ACCEPTED;
		tlsSession->stickyEvent = RV_TLS_READ_EV;
	} else if(tlsSession->renegState > RV_TLS_RENEG_NEEDED && !renegPending) {
		tlsSession->renegState = RV_TLS_RENEG_NONE;
		tlsSession->stickyEvent = RV_TLS_NONE_EV;
	}
    
#endif

	if (sslRetCode <= 0)
		rvTLSGetError(tlsSession,sslRetCode,mtx,&sslErrCode);

    switch(sslErrCode) {
    case SSL_ERROR_NONE:
        RvLogDebug(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionReceiveBuffer: session (0x%x) received %d bytes",
            tlsSession,sslRetCode));
        tlsSession->requiredForTLSRead = 0;
        *receiveBufLen = sslRetCode;
        break;
    case SSL_ERROR_WANT_READ:
      RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                      "RvTLSSessionReceiveBuffer: session (0x%x) SSL_RET_WANT_READ failure.",
                      tlsSession));
      tlsSession->requiredForTLSRead |= RV_SELECT_READ;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
      break;
    case SSL_ERROR_WANT_WRITE:
      RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                      "RvTLSSessionReceiveBuffer: session (0x%x) SSL_RET_WANT_WRITE failure.",
                      tlsSession));
      tlsSession->requiredForTLSRead |= RV_SELECT_WRITE;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
      break;
    case SSL_ERROR_ZERO_RETURN:     /* possible shutdown */
        RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                        "RvTLSSessionReceiveBuffer: session (0x%x) SSL_RET_ZERO_RETURN failure.",
                        tlsSession));
#if RV_TLS_ENABLE_RENEGOTIATION
        tlsSession->renegState = RV_TLS_RENEG_NONE;
        tlsSession->stickyEvent = RV_TLS_NONE_EV;
#endif
        shutdownStatus = SSL_get_shutdown(tlsSession->sslSession);
        if ( shutdownStatus & (SSL_RECEIVED_SHUTDOWN | SSL_SENT_SHUTDOWN)) {
            RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                "RvTLSSessionReceiveBuffer: session (0x%x) shutdown request received.",
                tlsSession));
            retCode = RvTLSErrorCode(RV_TLS_ERROR_SHUTDOWN);
        }
        break;
    default:
      if (ERR_get_error_line_data((const char **)&file, &line,
                      (const char **)&err, &errFlags) != 0) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSSessionReceiveBuffer: session (0x%x) general error - %s, file - %s, line - %d",
                         tlsSession,err,file,line));
      }
      else {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                        "RvTLSSessionReceiveBuffer: session (0x%x) general error.",
                        tlsSession));
      }
      tlsSession->requiredForTLSRead = 0;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
      break;
    }

    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionReceiveBuffer(tlsSession=%p,receiveBuf=%p,mtx=%p,logMgr=%p,bufLen=%d)=%d",
        tlsSession,receiveBuf,mtx,logMgr,*receiveBufLen,retCode));

    return retCode;
}


/**********************************************************************************
 * RvTLSSessionSendBuffer - sends stack message via SSL session
 *
 * Enecrypts stack message and sends SSL/TLS message with encrypted stack data to
 * the SSL session peer.
 *
 * INPUT:
 *  tlsSession      - Connected TLS session
 *  sendBuf         - buffer to send
 *  sendBufLen      - length of stack data in the sendBuf
 *  mtx             - Mutex that protects session
 *                    structure
 *  logMgr          - log instance
 * OUTPUT: none
 * RETURN:
 *  RvStatus    - Success, WilllBlock or failure
 */
RVCOREAPI
RvStatus RvTLSSessionSendBuffer(
    IN  RvTLSSession    *tlsSession,
    IN  RvChar          *sendBuf,
    IN  RvInt           sendBufLen,
    IN  RvMutex         *mtx,
    IN  RvLogMgr        *logMgr)
{
    RvInt       sslRetCode;
    RvStatus    retCode = RV_OK;
    RvInt       sslErrCode;
    RvChar      *file;
    RvChar      *err;
    RvInt       line;
    RvInt       errFlags;
    RvInt       shutdownState;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL) || (sendBuf == NULL) ||
        (sendBufLen <= 0)    || (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionSendBuffer(tlsSession=%p,sendBuf=%p,bufLen=%d,mtx=%p,logMgr=%p)",
        tlsSession,sendBuf,sendBufLen,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

    sslRetCode = SSL_write(tlsSession->sslSession,sendBuf,sendBufLen);

    rvTLSGetError(tlsSession,sslRetCode,mtx,&sslErrCode);
    switch(sslErrCode) {
    case SSL_ERROR_NONE:
      /* empty list of events required to complete SSL write */
      tlsSession->requiredForTLSWrite = 0;

#if RV_TLS_ENABLE_RENEGOTIATION
      
      if(tlsSession->renegState == RV_TLS_RENEG_NEEDED) {
        tlsSession->stickyEvent = RV_TLS_READ_EV;    
      }
#endif

      /* situation when buffer was sent partually but no
         error was returned should not happen */
      if (sendBufLen != sslRetCode) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSSessionSendBuffer: session (0x%x) sent %d out of %d bytes",
                         tlsSession,sslRetCode,sendBufLen));
        RvMutexUnlock(mtx,logMgr);
        return RvTLSErrorCode(RV_TLS_ERROR_GEN);
      }
      break;
    case SSL_ERROR_WANT_READ:
      RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                       "RvTLSSessionSendBuffer: session (0x%x) sent SSL_RET_WANT_READ failure.",
                       tlsSession));
      tlsSession->requiredForTLSWrite |= RV_SELECT_READ;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
      break;
    case SSL_ERROR_WANT_WRITE:
      RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                       "RvTLSSessionSendBuffer: session (0x%x) sent SSL_RET_WANT_WRITE failure.",
                       tlsSession));
      tlsSession->requiredForTLSWrite |= RV_SELECT_WRITE;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
      break;
    case SSL_ERROR_ZERO_RETURN:     /* possible shutdown */
        RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionSendBuffer: session (0x%x) sent SSL_RET_ZERO_RETURN failure.",
            tlsSession));
        shutdownState = SSL_get_shutdown(tlsSession->sslSession);
        if (shutdownState &(SSL_RECEIVED_SHUTDOWN|SSL_SENT_SHUTDOWN)) {
            RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                "RvTLSSessionSendBuffer: session (0x%x) shutdown request received.",
                tlsSession));
            retCode = RvTLSErrorCode(RV_TLS_ERROR_SHUTDOWN);
        }
        break;

    default:
      if (ERR_get_error_line_data((const char **)&file, &line,
                      (const char **)&err, &errFlags) != 0) {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSSessionSendBuffer: session (0x%x) general error - %s file - %s line - %d .",
                         tlsSession,err,file,line));
      }
      else {
        RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                         "RvTLSSessionSendBuffer: session (0x%x) sent general error",
                         tlsSession));
      }

      /* empty list of events required to complete SSL write */
      tlsSession->requiredForTLSWrite = 0;
      retCode = RvTLSErrorCode(RV_TLS_ERROR_GEN);
      break;
    }

    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionSendBuffer(tlsSession=%p,sendBuf=%p,bufLen=%d,mtx=%p,logMgr=%p)=%d",
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

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionShutdown(tlsSession=%p,mtx=%p,logMgr=%p)",
        tlsSession,mtx,logMgr));

    RvMutexLock(mtx,logMgr);

    sslRetCode = SSL_shutdown(tlsSession->sslSession);
    tlsSession->requiredForTLSShutdown = 0;

    switch (sslRetCode) {
    case RV_TLS_SHUTDOWN_SUCCEED_RET:
        RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionShutdown: session (0x%x) shutdown succeed",
            tlsSession));
        break;
    case RV_TLS_SHUTDOWN_WAITING_RET:
        RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSSessionShutdown: session (0x%x) shutdown incomplete",
            tlsSession));
        retCode = RvTLSErrorCode(RV_TLS_ERROR_INCOMPLETE);
        break;
    case RV_TLS_SHUTDOWN_FAILURE_RET:
        {
            RvInt       sslErrCode;

            rvTLSGetError(tlsSession,sslRetCode,mtx,&sslErrCode);
            switch(sslErrCode) {
            case SSL_ERROR_WANT_READ:
                RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                    "RvTLSSessionShutdown: session (0x%x) SSL_RET_WANT_READ failure.",
                    tlsSession));
                tlsSession->requiredForTLSShutdown |= RV_SELECT_READ;
                retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
                break;
            case SSL_ERROR_WANT_WRITE:
                RvLogInfo(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                    "RvTLSSessionShutdown: session (0x%x) SSL_RET_WANT_WRITE failure.",
                    tlsSession));
                tlsSession->requiredForTLSShutdown |= RV_SELECT_WRITE;
                retCode = RvTLSErrorCode(RV_TLS_ERROR_WILL_BLOCK);
                break;
            default:
                if (ERR_get_error_line_data((const char **)&file, &line,
                    (const char **)&err, &errFlags) != 0) {
                    RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                        "RvTLSSessionSendBuffer: session (0x%x) shutdown general error - %s file - %s line - %d",
                        tlsSession,err,file,line));
                }
                else {
                    RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
                        "RvTLSSessionShutdown: session (0x%x) shutdown general failure.",
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
                "RvTLSSessionShutdown: session (0x%x) shutdown unknown general failure.",
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

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSTranslateSelectEvents(tlsSession=%p,selEvents=%x,mtx=%p,logMgr=%p,tlsEvents=%p)",
        tlsSession,selEvents,mtx,logMgr,tlsEvents));

    /* initiate output */
    *tlsEvents = (RvTLSEvents)0;

    /* check conditions required to complete previousely
    failed event */

    RvMutexLock(mtx,logMgr);

	if(tlsSession->stickyEvent) {
		*tlsEvents = tlsSession->stickyEvent;
	} else {
		if (tlsSession->requiredForHandshake & selEvents) {
			*tlsEvents |= RV_TLS_HANDSHAKE_EV;
		}
		if (tlsSession->requiredForTLSRead & selEvents) {
			*tlsEvents |= RV_TLS_READ_EV;
		}
		if (tlsSession->requiredForTLSWrite & selEvents) {
			*tlsEvents |= RV_TLS_WRITE_EV;
		}
		if (tlsSession->requiredForTLSShutdown & selEvents) {
			*tlsEvents |= RV_TLS_SHUTDOWN_EV;
		}

		/* filter all irrelevant events */
		*tlsEvents &= tlsSession->tlsEvents;

		/* Add all direct mappings if available */
		if ((selEvents & RV_SELECT_WRITE) && (tlsSession->tlsEvents & RV_TLS_HANDSHAKE_EV)) {
			*tlsEvents |= RV_TLS_HANDSHAKE_EV;
		}
		if ((selEvents & RV_SELECT_READ) && (tlsSession->tlsEvents & RV_TLS_READ_EV)) {
			*tlsEvents |= RV_TLS_READ_EV;
		}
		if ((selEvents & RV_SELECT_WRITE) && (tlsSession->tlsEvents & RV_TLS_WRITE_EV)) {
			*tlsEvents |= RV_TLS_WRITE_EV;
		}
		if ((selEvents & RV_SELECT_WRITE) && (tlsSession->tlsEvents & RV_TLS_SHUTDOWN_EV)) {
			*tlsEvents |= RV_TLS_SHUTDOWN_EV;
		}
	}

    /* clear received events from session waiting events list */
    tlsSession->tlsEvents &= (~(*tlsEvents));

    RvMutexUnlock(mtx,logMgr);

    RvLogLeave(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSTranslateSelectEvents(tlsSession=%p,selEvents=%x,mtx=%p,logMgr=%p,tlsEvents=%x)=%d",
        tlsSession,selEvents,mtx,logMgr,*tlsEvents,RV_OK));

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
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    /* check input parameters */
    if ((tlsSession == NULL) || (selEvents == NULL) ||
        (mtx == NULL)) {
        return RvTLSErrorCode(RV_ERROR_BADPARAM);
    }
#endif


    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSTranslateTLSEvents(tlsSession=%p,tlsEvents=%x,mtx=%p,logMgr=%p,selEvents=%p)",
        tlsSession,tlsEvents,mtx,logMgr,selEvents));

    /* initiate output */
    *selEvents = 0;

    RvMutexLock(mtx,logMgr);

    if(tlsSession->stickyEvent) {
        RvSelectEvents requiredForRead = tlsSession->requiredForTLSRead;
        tlsSession->tlsEvents = RV_TLS_READ_EV;
        *selEvents = (RvSelectEvents)(requiredForRead ? requiredForRead : RV_SELECT_WRITE);
    } else {
        if (tlsEvents & RV_TLS_HANDSHAKE_EV) {
            tlsSession->tlsEvents |= RV_TLS_HANDSHAKE_EV;
            if (tlsSession->requiredForHandshake != 0) {
                *selEvents |= tlsSession->requiredForHandshake;
            }
            else {
                *selEvents |= RV_SELECT_WRITE;
            }
        }
        if (tlsEvents & RV_TLS_READ_EV) {
            tlsSession->tlsEvents |= RV_TLS_READ_EV;
            if (tlsSession->requiredForTLSRead != 0) {
                *selEvents |= tlsSession->requiredForTLSRead;
            }
            else {
                *selEvents |= RV_SELECT_READ;
            }
        }
        if (tlsEvents & RV_TLS_WRITE_EV) {
            tlsSession->tlsEvents |= RV_TLS_WRITE_EV;
            if (tlsSession->requiredForTLSWrite != 0) {
                *selEvents |= tlsSession->requiredForTLSWrite;
            }
            else {
                *selEvents |= RV_SELECT_WRITE;
            }
        }
        if (tlsEvents & RV_TLS_SHUTDOWN_EV) {
            tlsSession->tlsEvents |= RV_TLS_SHUTDOWN_EV;
            if (tlsSession->requiredForTLSShutdown != 0) {
                *selEvents |= tlsSession->requiredForTLSShutdown;
            }
            else {
                *selEvents |= RV_SELECT_WRITE;
            }
        }
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
 *  certLen     - lenght of ASN1 certificate
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
 *                allocated memory space enought for keeping the ASN1
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
 *  certLen     - lenght of ASN1 certificate
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

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSSessionGetCertificateLength(tlsSession=%p,mtx=%p,logMgr=%p,certLen=%p)",
        tlsSession,mtx,logMgr,certLen));

    RvMutexLock(mtx,logMgr);

    cert = SSL_get_peer_certificate(tlsSession->sslSession);

    if (cert == NULL) {
      *certLen = 0;
      RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
          "RvTLSSessionGetCertificateLength: session (0x%x) failed to retrieve peer certificate",
          tlsSession));
      RvMutexUnlock(mtx,logMgr);
      return RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }

    *certLen = i2d_X509(cert,NULL);

    RvMutexUnlock(mtx,logMgr);

    X509_free(cert);

    if (*certLen < 0) {
      RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
          "RvTLSSessionGetCertificate: session (0x%x) failed to convert X509 certificate to ASN1",
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
 *                allocated memory space enought for keeping the ASN1
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

  RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
      "RvTLSSessionGetCertificate(tlsSession=%p,mtx=%p,logMgr=%p,cert=%p)",
      tlsSession,mtx,logMgr,cert));

  RvMutexLock(mtx,logMgr);

  certX509 = SSL_get_peer_certificate(tlsSession->sslSession);

  if (certX509 == NULL) {
      RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
          "RvTLSSessionGetCertificate: session (0x%x) failed to retrieve peer certificate",
          tlsSession));
      RvMutexUnlock(mtx,logMgr);
      return RvTLSErrorCode(RV_TLS_ERROR_GEN);
  }

  certEnd = cert;
  if (i2d_X509(certX509,(unsigned char **)&certEnd) < 0) {
      RvLogError(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
          "RvTLSSessionGetCertificate: session (0x%x) failed to convert X509 certificate to ASN1",
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
 *  memBuf      - memory buffer used to save intermidiate strings during processing.
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
    X509V3_EXT_METHOD   *meth   = NULL;
    ASN1_VALUE          *iExtStr= NULL;

#ifdef RV_NULLCHECK
    /* Check input parameters */
    if ((tlsSession == NULL) || (name == NULL) ||
        (mtx == NULL)) {
        return _RvTLSErrorCode(RV_TLS_ERROR_GEN);
    }
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
            "RvTLSSessionCheckCertAgainstName: session (0x%x) failed to retrieve peer certificate",
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
                val = meth->i2v(meth,
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
                "RvTLSSessionCheckCertAgainstName: session (0x%x) name does not match certificate",
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

    RvLogEnter(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
        "RvTLSEngineCheckPrivateKey(tlsEngine=%p,mtx=%p,logMgr=%p)",
        tlsEngine,mtx,logMgr));

    RvMutexLock(mtx,logMgr);
    if (SSL_CTX_check_private_key(tlsEngine->ctx) != 1) {
        RvLogWarning(RvTLSLogSrc(logMgr), (RvTLSLogSrc(logMgr),
            "RvTLSEngineCheckPrivateKey: engine (0x%x), failed to add X509 certificate to the chain",
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
        X509V3_EXT_METHOD *meth;
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

        val = meth->i2v(meth, iExtStr, NULL);
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

    d->bufLeft -= altNameLen;
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
    d.bufLeft = *pbufSize;
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

#endif /* (RV_TLS_TYPE != RV_TLS_NONE) */
