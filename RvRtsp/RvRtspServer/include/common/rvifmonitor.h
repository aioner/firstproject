#include "rverror.h"

#ifndef _file_rvifmonitor_h_
#define _file_rvifmonitor_h_

#include "rvaddress.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* To enable interface monitoring, set this flag to '1' in rvusrconfig.h */
#if (RV_OS_TYPE == RV_OS_TYPE_LINUX)&& RV_USES_IFMONITOR



/* the supported interface states */
typedef enum {
    RvInterfaceUp,          /* interface is UP & RUNNING */
    RvInterfaceDown,        /* interface is DOWN */
    RvInterfaceError        /* error on the interface */
} RvInterfaceState;

typedef void* RvInterfaceMonitorHandle;


/********************************************************************************************
 * RvInterfaceMonitorOnEvent - the application provided callback
 *  Such function will be called by CommonCore interface monitor when
 *  the interface monitor changes.
 *
 * INPUT   : hIfMon         - the monitor handle created by RvInterfaceMonitorStart
 *           pUsrCtx        - the application context
 *           ifState        - the current interface state
 * OUTPUT  : None
 * RETURN  : None
 */
typedef RvStatus (*RvInterfaceMonitorOnEvent)(
    IN RvInterfaceMonitorHandle hIfMon,
    IN void* pUsrCtx,
    IN RvInterfaceState ifState);


/********************************************************************************************
 * RvInterfaceMonitorStart - Starts to monitor the network interface
 *
 * Application registers the callback to be called when the network interface state
 * changes.
 * This function must be called in the thread where the toolkit (SIP,ARTP etc) was
 * constructed.
 *
 * INPUT   : addr           - the address of the interface
 *           ifname         - the name of the interface. ("eth0" for example)
 *           One of addr or ifname must be provided.
 *           onEvent        - the callback to be called when the interface status changes
 *           pUsrCtx        - the application context
 *           logMgr         - the log manager (may be NULL)
 * OUTPUT  : phIfMon        - the monitor handle. It will be needed later for RvInterfaceMonitorStop
 *           pIfState       - if not NULL the current state of the interface will be returned.
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvInterfaceMonitorStart(
	IN RvAddress* addr,
	IN RvChar* ifname,
	IN RvInterfaceMonitorOnEvent onEvent,
	IN void* pUsrCtx,
	IN RvLogMgr *logMgr,
	OUT RvInterfaceMonitorHandle* phIfMon,
	OUT RvInterfaceState* pIfState);

/********************************************************************************************
 * RvInterfaceMonitorStop - Stops to monitor the network interface
 *
 *
 * INPUT   : hIfMon         - the monitor handle created by RvInterfaceMonitorStart
 * OUTPUT  : None
 * RETURN  : None
 */
void RvInterfaceMonitorStop(
	IN RvInterfaceMonitorHandle hIfMon);


#elif (RV_OS_TYPE == RV_OS_TYPE_WIN32)


#define RV_ADDR_MONITOR_MAX_LISTENERES    5
#define RV_ADDR_MONITOR_MAX_CURRADDRS     10


/********************************************************************************************
* RvOnAddressChange - this application provided callback will be called to notofy the application
* about the local addresses changes. This function needs to be provided when the application
* registers to monitor the host addresses using the RvInterfaceMonitorRegister API.
* Currently only IPv4 addresses are monitored.
*
* INPUT   : appCtx         - application context
*           addedAddrs     - array containing the added addresses.
*                            Please note that the memory pointed by addedAddrs is valid
*                            only during the function call and can be reclaimed as soon
*                            as the function exits.
*           addedAddrsCnt  - the number of added addresses.
*           removedAddrs   - array containing the removed addresses.
*                            Please note that the memory pointed by removedAddrs is valid
*                            only during the function call and can be reclaimed as soon
*                            as the function exits.
*           removedAddrs  - the number of removed addresses.
* RETURN  : None
*/
typedef void (*RvOnAddressChange)(
               IN       void*            appCtx,
               IN const RvAddress*       addedAddrs,
               IN       RvInt            addedAddrsCnt,
               IN const RvAddress*       removedAddrs,
               IN       RvInt            removedAddrsCnt);

#include "rvselect.h"

/********************************************************************************************
* RvInterfaceMonitorRegister - registers the application provided callback function to be
*  called when some local IP addresses are added or removed on the local host.
*
* INPUT   : pSelEng        - select engine object
*           pListener      - the callback function
*           pAppCtx        - the application context
* OUTPUT  : pListenerInd   - the listener index will be returned through this parameter, its value
*                            may be used later when RvInterfaceMonitorUnregister is called.
*                            The NULL pointer may also be given.
* RETURN  : RV_OK on success, other on failure
*/
RVCOREAPI RvStatus RVCALLCONV
RvInterfaceMonitorRegister(
    IN  RvSelectEngine*     pSelEng,
    IN  RvOnAddressChange   pListener,
    IN  void*               pAppCtx,
    OUT RvInt*              pListenerInd);

/********************************************************************************************
* RvInterfaceMonitorUnregister - unregisters the application provided callback function to be
* called when some local IP addresses are added or removed on the local host.
* The listener to be unregistered must be specified by either pAppCtx or pListenerId value.
* That is one of them (but not both) must be provided.
*
* INPUT   : pSelEng        - select engine object
*           pAppCtx        - the application context of the listener to be unregistered or NULL
*           pListenerInd   - the listener index to be unregistered or NULL
* RETURN  : RV_OK on success, other on failure
*/
RVCOREAPI RvStatus RVCALLCONV
RvInterfaceMonitorUnregister(
    IN  RvSelectEngine*         pSelEng,
    IN  void*                   pAppCtx,
    IN  RvInt*                  pListenerInd);

/* Called by select when the  change in the interfaces was detected */
void
RvInterfaceMonitorChangeDetected(
    RvSelectEngine* pSelEng);


#endif /* #elif (RV_OS_TYPE == RV_OS_TYPE_WIN32) */

#define     RV_INTERFACE_ADDRS_MAX_NUM  5
#define     RV_INTERFACE_DESCR_MAX_LEN  256

/********************************************************************************************
* RvInterfaceData
* The instance of this structure must be provided when calling RvInterfaceGetInterfaceInfoByDestination
* function.
* This structure contains data describing the host network interface.
********************************************************************************************/
typedef struct {
    RvAddress       pAddrs[RV_INTERFACE_ADDRS_MAX_NUM];     /* the array of addresses owned by the interface */
    RvInt           addrsNum;                               /* the number of addresses */
    RvUint          interfaceSpeed;                         /* the current interface speed measured in bits per second */
    RvChar          intrfaceDescrription[RV_INTERFACE_DESCR_MAX_LEN];   /* the interface textual description */
} RvInterfaceData;

/********************************************************************************************
* RvInterfaceGetInterfaceInfoByDestination - 
* Retrieves the interface data that will be used when communicating with the host having
* the pDestAddr address.
* At the moment this function can only be used on Windows OS.
*
* INPUT   : pDestAddr      - the destination address
* OUTPUT  : pInterfData    - the interface data structure to be filled.
* RETURN  : RV_OK on success, other on failure
*/
RVCOREAPI RvStatus RVCALLCONV
RvInterfaceGetInterfaceInfoByDestination(
    IN      RvAddress*          pDestAddr,
    OUT     RvInterfaceData*    pInterfData);

#define     RV_MAC_ADDR_LENGTH 6
typedef struct {
    RvUint8    Address[RV_MAC_ADDR_LENGTH];
    RvUint     AddressLen;
} RvMacAddress;

/********************************************************************************************
* RvInterfaceGetMacAddresses - 
* Retrieves the list of MAC addresses the given host has.
* At the moment this function can only be used on Windows OS.
*
* INPUT   : None
* OUTPUT  : 
*           macAddrs     - array of MacAddresses
*           pNumOfMacs   - when called must contain the size of macAddrs array
*                          after function call will contain the number of addresses returned in the
*                          macAddrs.
* RETURN  : RV_OK on success, other on failure
*/
RVCOREAPI RvStatus RVCALLCONV
RvInterfaceGetMacAddresses(
    INOUT   RvMacAddress*       macAddrs,
    INOUT   RvUint*             pNumOfMacs);


#if defined(__cplusplus)
}
#endif

#endif /*define _file_rvifmonitor_h_ */
