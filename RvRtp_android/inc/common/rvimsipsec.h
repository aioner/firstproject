#include "rvtypes.h"
#include "rvaddress.h"
#include "rvobjpool.h"
#include "rvrandomgenerator.h"

#include "rvimsipsecpublic.h"


#ifndef _file_rvimsipsec_h_
#define _file_rvimsipsec_h_

#if (RV_IMS_IPSEC_ENABLED == RV_YES)

/*
 *  Remarks:
 *
 *  Functions of this module are used to establish/destroy two pairs of SA/SP as described by 
 *  3GPP 33.203 standard.
 *
 *  Linux:
 *
 *
 *  The creation of SA is as using setkey command with following input:
 *  add IP1 IP2 esp SPI -m transport -u ID_NUM -E ENCR_ALG ENCR_KEY -A AUTH_ALG AUTH_KEY;
 *  If the SA is incoming there is no need to set '-u ID_NUM'
 *  The creation of SP is as using setkey command with following input:
 *  spdadd IP1[PORT1] IP2[PORT2] IP_PROTO -P DIR ipsec esp/transport//unique:ID_NUM ;
 *  The unique:ID_NUM has to be given only for outbound policies. The DIR is in or out
 *  the IP_PROTO is one of tcp, udp or any.
 *
 *  The ounbound SPs use the unique SA. The uniqueness is set by ID.
 *  The rvImsIPSecCleanAll function is supported. When creating TCP,UDP and BOTH are supported.
 *
 *
 *  Solaris:
 *
 *  The creation of SA is as using ipseckey command with following input:
 *  add esp spi SPI src IP1 sport PORT1 dst IP2 dport PORT2 
 *                   encralg ENCR_ALG encrkey ENCR_KEY authalg AUTH_ALG authkey AUTH_KEY
 *  
 *  The creation of SP is as using ipseccond command with following input for inbound SPs:
 *  { saddr IP1 sport PORT1 daddr IP2 dport PORT2 dir in }
 *          permit { encr_algs ENCR_ALG encr_auth_algs AUTH_ALG sa shared }
 *  and for outbound SPs:
 *  { saddr IP1 sport PORT1 daddr IP2 dport PORT2 dir out }
 *      apply { encr_algs ENCR_ALG encr_auth_algs AUTH_ALG sa unique }
 *
 *  There is a problem using SAs for both TCP and UDP.
 *  Uniqueness of outbound SA when used (and locked) by SP is defined by 5 tuples containing:
 *  source and destination addresses and ports and IP protocol that is TCP or UDP.
 *  That is consider the situation with outbound SP defined for both protocols 
 *  (TCP and UDP) and using unique SA.
 *  Now when the first TCP packet is encrypted for this rule the new (unused) SA has to be found 
 *  and will be locked for TCP and 
 *  for any further UDP packet this SA can't be used. 
 *  If we define SA for SP as 'shared' 
 *  the SA matching addresses only (but not ports) will be used. That is there are two outbound
 *  SAs first for Ip1:Port1 -> Ip2:Port2 and the second for Ip1:Port3 -> Ip2:Port4 and also two 
 *  outbound SPs for the same addresses and ports. Only first SA will be used for both SPs
 *  since when SP needs 'shared' SA it finds the first SA with the correspondent source and 
 *  destination addresses.
 *  Conclusion: using rvImsIPSecEstablishSAs with RvImsSAData.iProto set to RvImsProtoAny will
 *  result in using only one outbound SA for both outbound SPs.
 *  If the other side does not check the strict correspondence of inbound SA and SP (Linux case) 
 *  the communication will work otherwise (VxWorks case) will not.
 *  The rvImsIPSecCleanAll function is supported.
 *
 *
 *  VxWorks:
 *  On VxWorks it is impossible to call rvImsIPSecEstablishSAs with RvImsSAData.iProto set to 
 *  RvImsProtoAny since the spdAddTransport requires some TCP or UDP if the port numbers are given.
 *  The rvImsIPSecCleanAll function is not supported.
 *
 */

/*
 *	This struct is used to keep the data of used SPI numbers;
 *  The all SPIs in use data is kept in has that is organised as
 *  array of linked lists. Instances of struct RvImsSpiStruct are contained
 *  in linked lists.
 *  
 */
typedef struct _RvImsSpiStruct
{
    RvUint32 iSPI;                  /* the SPI number that is used */
    RvUint32 iRefCount;             /* reference count of this struct instance */
	RvBool   bExternal;             /* RV_TRUE if value for SPI was taken from
                                       external database, and was not generated
                                       locally */
    struct _RvImsSpiStruct* iNext;  /* points to the next instance in the linked list*/
    RvObjPoolElement iPoolElement;  /* needed because the instances are allocated through 
                                       the pool */
} RvImsSpiStruct;


/* 
 * this is the upper limit of SPIs hash function;
 * the application should set this number to appropriate number to make
 * search in the hash effective.
 * The has is an array of RvImsSpiStruct pointers allocated to the 
 * number not bigger than RV_IMS_SPI_HASH_SIZE.
 */
#define RV_IMS_SPI_HASH_SIZE    1024
/*
 *	IMS Ipses configuration data. Needed for SPI numbers allocation.
 */
typedef struct
{
    RvUint32 iSPIRangeLow;      
    RvUint32 iSPIRangeHigh;         /* sets the range of SPIs */
    RvLock  iSPIRangeLock;          /* the mutex used when allocating/releasing SPIs */
    RvObjPool iPoolObj;             /* the pool used for RvImsSpiStruct instances */
    RvImsSpiStruct** iArray;        /* the hash of RvImsSpiStruct instances */
    RvSize_t  iArraySz;             /* the size of hash */
    RvUint32  iMsk;                 /* this mask is used to locate the hash position given 
                                       the SPI value */
    RvRandomGenerator iRandObj;     /* random generator */
    RvBool iInitialized;            /* whether is initialized */
} RvImsCfgData;

/*
 *	To keep the crypto key (encryption and authentication)
 */
#define RV_IMS_CRYPTO_KEY_MAX_LEN   30 /* currently the longest key used is 192 bits for 3DES */
typedef struct  {
    RvUint8  iKeyValue[RV_IMS_CRYPTO_KEY_MAX_LEN];  /* the key value */
    RvUint16 iKeyBits;                              /* the key length */
} RvImsCryptoKey;


typedef enum
{
    RvImsProtoUdp,
    RvImsProtoTcp,
    RvImsProtoAny
} RvImsProtocol;

/* encryption algorithm */ 
typedef enum
{
    RvImsEncrUndefined = -1,
    RvImsEncrNull = 0, /* that is importanat that RvImsEncrNull would be zero 
                        * and the other values would go up in increasing order.
                        * The values are used as indexes in the array
                        */
    RvImsEncr3Des,
    RvImsEncrAes
} RvImsEncrAlg;

/* authentication algorithm */ 
typedef enum
{
    RvImsAuthUndefined = -1,
    RvImsAuthMd5 = 0, /* that is importanat that RvImsEncrNull would be zero 
                       * and the other values would go up in increasing order.
                       * The values are used as indexes in the array
                       */
    RvImsAuthSha1
} RvImsAuthAlg;

/* data specific to one SA */
typedef struct 
{
    RvUint          iDirection;
    RvUint32        iSPI;      
    RvUint32        iSPDIndex;
    RvUint32        iSADIndex;
    RvUint16        iLclPort;
    RvUint16        iRemPort;
} RvIPSecSAData;

#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS) 
/* 
 * on VxWorks there are two stages in IPsec administartion:
 *    SA and Mkm
 */    
typedef enum
{   
    RvImsVWUndef = 0,
    RvImsVWSACreated = 1,
    RvImsVWMkmAdded = 2
} RvImsIpsecVWState;
#endif


/*
 *	Data needed for two SA/SP pairs
 */
typedef struct
{
    RvUint              iOSEncrAlg;     /* encryption algorithm in use */
    RvUint              iOSAuthAlg;     /* authentication algorithm in use */
    RvIPSecSAData       iSAP[4];        /* this contains, local and remote port numbers
                                           SPI used for inbound and outbound SAs
                                           and the indexes of the created SAs and SPs */
#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
    RvImsIpsecVWState   iVWState[2];    /* tells the create state om VxWorks */
#endif
} RvImsIpSecSpecific;

/*
 * this data has to be provided by external application when creating IMS IPsec channel
 */
typedef struct
{
    RvImsEncrAlg        iEncrAlg;       /* the encryption algorithm */
    RvImsAuthAlg        iAuthAlg;       /* the authentication algorithm */
    RvImsProtocol       iProto;         /* wtether the IPsec channel is created for TCP, UDP or both
                                           protocols */

    RvImsCryptoKey      iAuthKey;       /* the authentication key */
    RvImsCryptoKey      iEncrKey;       /* the encryption key (not needed if iEncrAlg is RvImsEncrNull) */

    RvUint32            iSpiInClnt;     /* SPI used for Inbound Client SA 
                                           this SPI is selected locally and has to be
                                           used by the peer when transmitting packets
                                           from peer's server port to the local client port.
                                         */

    RvUint32            iSpiInSrv;      /* SPI used for Inbound Server SA 
                                           this SPI is selected locally and has to be
                                           used by the peer when transmitting packets
                                           from peer's client port to the local server port.
                                         */

    RvUint32            iSpiOutClnt;    /* SPI used for Outbound Client SA 
                                           This SPI is selected by the peer and is used
                                           locally when transmitting packets from local
                                           client port to the peer server port.
                                         */
    RvUint32            iSpiOutSrv;     /* SPI used for Outbound Server SA 
                                           This SPI is selected by the peer and is used
                                           locally when transmitting packets from local
                                           server port to the peer client port.  
                                         */

    RvUint16            iLclPrtSrv;     /* ports numbers (local and remote) */
    RvUint16            iLclPrtClnt;
    RvUint16            iPeerPrtSrv;
    RvUint16            iPeerPrtClnt;

    RvAddress           iLclAddress;    /* addresses */
    RvAddress           iPeerAddress;

    RvImsAppExtensionData*  iExtData;  /* array containing extension data */
    RvSize_t                iExtDataLen;    /* the length of extension data array */


    RvImsIpSecSpecific  iIpSecSpecific;
} RvImsSAData;

/********************************************************************************************
 * RvIpsecInit
 * Initializes the IPSEC Module.
 * INPUT   : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvIpsecInit(void);

/********************************************************************************************
 * RvIpsecEnd
 * Closes the IPSEC Module.
 * INPUT   : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvIpsecEnd(void);

/******************************************************************************
 * RvIpsecConstruct
 *
 * Allocates and initiates a new IPsec engine.
 * In case select engine was already constructed for the current thread,
 * pointer to the existing agent will be returned.
 *
 *
 * INPUT   : logMgr - log manager
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvIpsecConstruct(IN RvLogMgr* logMgr);

/******************************************************************************
 * RvIpsecDestruct
 *
 * Destruct a select engine
 * Not thread-safe function.
 *
 * INPUT   : logMgr - log manager
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvIpsecDestruct(IN RvLogMgr* logMgr);

/********************************************************************************************
 * rvIpsecSourceConstruct
 *
 * Constructs a Log Source object for the IPSec module.
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RvStatus rvIpsecSourceConstruct(
    IN RvLogMgr* logMgr);


/********************************************************************************************
 * rvImsIPSecEstablishSAs
 *
 * Establishes 4 new SA and SP entries based on 'sad'.
 *
 * INPUT:
 *    sad       - contains data needed for SA/SP creation;
 *  logMgr      - handle of the log manager for this instance
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RVCOREAPI
RvStatus rvImsIPSecEstablishSAs(
    IN RvImsSAData *sad,
    IN RvLogMgr *logMgr);

/********************************************************************************************
 * rvImsIPSecDestroySAs
 *
 * Removes 4 previously established SAs.
 *
 * INPUT:
 *    sad       - contains data needed for SA/SP creation;
 *  logMgr      - handle of the log manager for this instance
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RVCOREAPI
RvStatus rvImsIPSecDestroySAs(
    IN RvImsSAData *sad,
    IN RvLogMgr *logMgr);


/********************************************************************************************
 * rvImsIPSecCleanAll
 *
 * Removes all SA/SP set on this host.
 * This function removes *all* SA/SP on this host, not only previpusly created by us.
 * Not supported on VxWorks.
 *
 * INPUT:
 *  logMgr      - handle of the log manager for this instance
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus rvImsIPSecCleanAll(
    IN RvLogMgr *logMgr);


/********************************************************************************************
 * rvImsConstructCfg
 *
 * Initialize the configuratiuon of IMS IPsec. Needed for SPI selection functins only.
 *
 * INPUT:
 *  cfg - IMS configuration data
 *  spiRangeStart, spiRangeFinish - defines the range where the SPI numbers will be
 *                                  allocated.
 *  expectedSPIsNum - expected max number of security channels established by application.
 *  logMgr      - handle of the log manager for this instance
 *
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus rvImsConstructCfg(
    IN RvImsCfgData *cfg,           /* ims cfg data */
    IN RvUint32 spiRangeStart,      /* the lower border of SPIs range */
    IN RvUint32 spiRangeFinish,     /* the upper border of SPIs range */
    IN RvSize_t expectedSPIsNum,    /* expected max number of security agreements */
    IN RvLogMgr *logMgr);


/********************************************************************************************
 * rvImsDestructCfg
 *
 * Destroys the configuratiuon of IMS
 *
 * INPUT:
 *  cfg - IMS configuration data
 *  logMgr      - handle of the log manager for this instance 
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RVCOREAPI
void rvImsDestructCfg(
    IN RvImsCfgData *cfg, /* cfg data (contains the hash and the pool that will be deallocated) */
    IN RvLogMgr *logMgr);


/********************************************************************************************
 * rvImsGetPairOfSPIs
 *
 * Allocates pair of SPIs
 *
 * INPUT:
 *  cfg - IMS configuration data
 *  logMgr      - handle of the log manager for this instance 
 *
 * INOUT
 *  spi1,spi2 - here the allocated SPIs will be stored.
 *              If the *spi1 or *spi2 is set to non-zero value it will not be changed by the function
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus rvImsGetPairOfSPIs(
    INOUT RvUint32* spi1,
    INOUT RvUint32* spi2,
    IN RvImsCfgData *cfg,
    IN RvLogMgr *logMgr);


/********************************************************************************************
 * rvImsReturnSPIs
 *
 * Frees pair of SPIs
 *
 * INPUT:
 *  spi1,spi2 - the SPIs to free. If one of them set to 0 it will not be treated.
 *  peerSpi1, peerSpi2 - peer side SPIs to free. If one of them set to 0 it will not be treated.
 *  cfg - IMS configuration data
 *  logMgr      - handle of the log manager for this instance 
 *
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus rvImsReturnSPIs(
    IN RvUint32 spi1,
    IN RvUint32 spi2,
    IN RvUint32 peerSpi1,
    IN RvUint32 peerSpi2,
    IN RvImsCfgData *cfg,
    IN RvLogMgr *logMgr);

/********************************************************************************************
 * rvImsSetAsBusyPairOfSPIs
 *
 * Sets pair of SPIs as used
 *
 * INPUT:
 *  spi1,spi2 - the SPIs to mark as busy. If one of them set to 0 it will not be treated.
 *  cfg - IMS configuration data
 *  logMgr      - handle of the log manager for this instance 
 *
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus rvImsSetAsBusyPairOfSPIs(
    IN RvUint32 spi1,
    IN RvUint32 spi2,
    IN RvImsCfgData *cfg,
    IN RvLogMgr *logMgr);

#endif /*(RV_IMS_IPSEC_ENABLED == RV_YES)*/

#endif /*_file_rvimsipsec_h_*/
