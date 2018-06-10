#include "rvtypes.h"
#include "rvaddress.h"
#include "rvalloc.h"

#ifndef _file_rvimsipsecprivate_h_
#define _file_rvimsipsecprivate_h_

#if (RV_IMS_IPSEC_ENABLED == RV_YES)

#include "rvimsipsec.h"

#define RV_IPSECPROTO_AH     0
#define RV_IPSECPROTO_ESP    1

#define RV_IPSECMODE_TRANSPORT  0
#define RV_IPSECMODE_TUNNEL     1

#define RV_IPSECMODE_DIR_IN     0
#define RV_IPSECMODE_DIR_OUT    1


#define IMSIPSEC_LOG_ENTER_0(f) \
    if (logMgr) {RvLogEnter(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f))}
#define IMSIPSEC_LOG_ENTER_1(f, p1) \
    if (logMgr) {RvLogEnter(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1))}
#define IMSIPSEC_LOG_ENTER_2(f, p1, p2) \
    if (logMgr) {RvLogEnter(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2))}
#define IMSIPSEC_LOG_ENTER_3(f, p1, p2, p3) \
    if (logMgr) {RvLogEnter(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3))}
#define IMSIPSEC_LOG_ENTER      IMSIPSEC_LOG_ENTER_0("")

#define IMSIPSEC_LOG_LEAVE_0(f) \
    if (logMgr) {RvLogLeave(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f))}
#define IMSIPSEC_LOG_LEAVE_1(f, p1) \
    if (logMgr) {RvLogLeave(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1))}
#define IMSIPSEC_LOG_LEAVE_2(f, p1, p2) \
    if (logMgr) {RvLogLeave(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2))}
#define IMSIPSEC_LOG_LEAVE_3(f, p1, p2, p3) \
    if (logMgr) {RvLogLeave(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3))}
#define IMSIPSEC_LOG_LEAVE_4(f, p1, p2, p3, p4) \
    if (logMgr) {RvLogLeave(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3, p4))}
#define IMSIPSEC_LOG_LEAVE      IMSIPSEC_LOG_LEAVE_0("")


#define IMSIPSEC_LOG_ERROR_0(f) \
    if (logMgr) {RvLogError(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f))}
#define IMSIPSEC_LOG_ERROR_1(f, p1) \
    if (logMgr) {RvLogError(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1))}
#define IMSIPSEC_LOG_ERROR_2(f, p1, p2) \
    if (logMgr) {RvLogError(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2))}
#define IMSIPSEC_LOG_ERROR_3(f, p1, p2, p3) \
    if (logMgr) {RvLogError(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3))}
#define IMSIPSEC_LOG_ERROR_4(f, p1, p2, p3, p4) \
    if (logMgr) {RvLogError(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3, p4))}

#define IMSIPSEC_LOG_INFO_0(f) \
    if (logMgr) {RvLogInfo(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f))}
#define IMSIPSEC_LOG_INFO_1(f, p1) \
    if (logMgr) {RvLogInfo(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1))}
#define IMSIPSEC_LOG_INFO_2(f, p1, p2) \
    if (logMgr) {RvLogInfo(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2))}
#define IMSIPSEC_LOG_INFO_3(f, p1, p2, p3) \
    if (logMgr) {RvLogInfo(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3))}
#define IMSIPSEC_LOG_INFO_4(f, p1, p2, p3, p4) \
    if (logMgr) {RvLogInfo(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3, p4))}
#define IMSIPSEC_LOG_INFO_5(f, p1, p2, p3, p4, p5) \
    if (logMgr) {RvLogInfo(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3, p4, p5))}
#define IMSIPSEC_LOG_INFO_6(f, p1, p2, p3, p4, p5, p6) \
    if (logMgr) {RvLogInfo(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3, p4, p5, p6))}
#define IMSIPSEC_LOG_INFO_7(f, p1, p2, p3, p4, p5, p6, p7) \
    if (logMgr) {RvLogInfo(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3, p4, p5, p6, p7))}

#define IMSIPSEC_LOG_DEBUG_0(f) \
    if (logMgr) {RvLogDebug(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f))}
#define IMSIPSEC_LOG_DEBUG_1(f, p1) \
    if (logMgr) {RvLogDebug(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1))}
#define IMSIPSEC_LOG_DEBUG_2(f, p1, p2) \
    if (logMgr) {RvLogDebug(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2))}
#define IMSIPSEC_LOG_DEBUG_3(f, p1, p2, p3) \
    if (logMgr) {RvLogDebug(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3))}

#define IMSIPSEC_LOG_WARN_0(f) \
    if (logMgr) {RvLogWarning(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f))}
#define IMSIPSEC_LOG_WARN_1(f, p1) \
    if (logMgr) {RvLogWarning(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1))}
#define IMSIPSEC_LOG_WARN_2(f, p1, p2) \
    if (logMgr) {RvLogWarning(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2))}
#define IMSIPSEC_LOG_WARN_3(f, p1, p2, p3) \
    if (logMgr) {RvLogWarning(&logMgr->imsIpsecSource, (&logMgr->imsIpsecSource, FUNCTION_NAME ": " f, p1, p2, p3))}

/* The message to be sent to or to be received from PF_KEY sockets.
   The PF_KEY sockets are used to configure SADB/SPDB.
   The PF_KEY message simply represents the 8-bytes aligned buffer. */
typedef union
{
	struct RvAlign_s aligningVar;
	RvUint8          buf[2048];  /* 2048 bytes should be enough to hold any configuration data */
} RvPfkeyMsg;

RvStatus rvAdIpsecInit();

RvStatus rvAdIpsecEnd();


#if (RV_OS_TYPE == RV_OS_TYPE_WINCE)

RvStatus rvAdIpsecConstruct(IN RvLogMgr* logMgr);
RvStatus rvAdIpsecDestruct(IN RvLogMgr* logMgr);

/********************************************************************************************
 * rvAdIpsecPickSpiValue
 *
 * On WinCE SPIs should not be chosen localy.
 * Instead of this, they shouls be taken from external database.
 * This function brings the value from the external database.
 *
 * INPUT:
 *  logMgr     - handle of the log manager
 *  rangeStart - start of range, where from the value should be chosen
 *  rangeStart - end of range, where from the value should be chosen
 * OUTPUT:
 *  pVal       - chosen value
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RvStatus rvAdIpsecPickSpiValue(IN RvLogMgr* logMgr, IN RvUint32 rangeStart, IN RvUint32 rangeEnd, OUT RvUint32* pVal);

/********************************************************************************************
 * rvAdIpsecFreeSpiValue
 *
 * On WinCE SPIs should not be chosen localy.
 * Instead of this, they shouls be taken from external database.
 * This function notifies the external database,
 * that the specified SPI value is not in use anymore.
 *
 * INPUT:
 *  logMgr - handle of the log manager
 *  val    - value to be freed
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RvStatus rvAdIpsecFreeSpiValue(IN RvLogMgr* logMgr, IN RvUint32 val);

#else

#define rvAdIpsecConstruct(logMgr) (RV_OK)
#define rvAdIpsecDestruct(logMgr) (RV_OK)
#define rvAdIpsecPickSpiValue(logMgr, rangeStart, rangeEnd, pVal) (RV_ERROR_NOTSUPPORTED)
#define rvAdIpsecFreeSpiValue(logMgr, val) (RV_OK)

#endif /* #if (RV_OS_TYPE != RV_OS_TYPE_WINCE) */


/********************************************************************************************
 * rvAdIPSecAddSASP
 *
 * OS specific function to add 'sapdNum' of security associations and security policies
 *
 * INPUT:
 *    sapd       - array of SA data
 * sapdNum       - number of elements in 'sapd'
 *  logMgr      - handle of the log manager for this instance 
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RvStatus rvAdIPSecAddSASP(
    IN RvImsSAData* sad, 
    IN int sapdNum, 
    IN RvLogMgr *logMgr);


/********************************************************************************************
 * rvAdIPSecDeleteSASP
 *
 * OS specific function to delete 'sapdNum' of security associations and security policies
 *
 * INPUT:
 *    sapd       - array of SA data
 * sapdNum       - number of elements in 'sapd'
 *  logMgr      - handle of the log manager for this instance 
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RvStatus rvAdIPSecDeleteSASP(
    IN RvImsSAData* sad, 
    IN int sapdNum, 
    IN RvLogMgr *logMgr);

                          

/********************************************************************************************
 * rvAdSetOSSpecific
 *
 * Set OS specific data for SA/SP manipulations
 *
 * INPUT:
 *    sad       - contains data needed for SA/SP creation;
 *
 *  logMgr      - handle of the log manager for this instance 
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RvStatus rvAdSetOSSpecific(
    IN RvImsSAData *sad, 
    IN RvLogMgr *logMgr);


/********************************************************************************************
 * rvAdIPSecCleanAll
 *
 * Removes all SA/SP set on this host.
 *
 * INPUT:
 *  logMgr      - handle of the log manager for this instance 
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RvStatus rvAdIPSecCleanAll(
    IN RvLogMgr *logMgr);

typedef enum
{
    SPIGet,
    SPIReturn,
    SPIMarkAsBusy
} RvImsSPIAction;

/********************************************************************************************
 * rvImsHandlePairsOfSPIs
 *
 * Performs different operations on SPI
 *
 * INPUT:
 *  
 *  act - the action to perform
 *  cfg - IMS configuration data
 *  logMgr - handle of the log manager for this instance 
 *
 * INOUT:
 *  spi1,spi2 - the SPIs to mark as busy
 *
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus rvImsHandlePairsOfSPIs(
    IN RvImsSPIAction act,
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
 *  spi1,spi2 - the SPIs to free
 *  peerSpi1, peerSpi2 - the SPIs used by the peer. If called by P-CSCSF can be set to zeroes.
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

#endif /*(RV_IMS_IPSEC_ENABLED == RV_YES)*/

#endif /* _file_rvimsipsecprivate_h_ */





















