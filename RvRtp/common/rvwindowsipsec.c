#include "rvtypes.h"
#include "rvaddress.h"
#include "rvalloc.h"
#include "rvlog.h"
#include "rvlock.h"

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)

#if (RV_IMS_IPSEC_ENABLED == RV_YES)

#include "rvimsipsecprivate.h"

RvStatus rvAdIpsecInit()
{
    return RV_OK;
}

RvStatus rvAdIpsecEnd()
{
    return RV_OK;
}


/********************************************************************************************
 * rvAdSetOSSpecific
 *
 * Set OS specific data for SA/SP manipulations
 *
 * INPUT:
 *    sad       - contains data needed for SA/SP creation;
 *  logMgr      - handle of the log manager for this instance 
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RvStatus rvAdSetOSSpecific(
    IN RvImsSAData *sad, 
    IN RvLogMgr *logMgr)
{
    RV_UNUSED_ARG(sad);
    RV_UNUSED_ARG(logMgr);
    return RV_OK;
}


/********************************************************************************************
 * rvAdIPSecAddSASP
 *
 * OS specific function to add 'sapdNum' of security associations and security policies.
 *
 * INPUT:
 *    sad       - array of SA data
 * sapdNum       - number of elements in 'sapd'
 *  logMgr      - handle of the log manager for this instance 
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RvStatus rvAdIPSecAddSASP(
    IN RvImsSAData *sad, 
    IN int sapdNum, 
    IN RvLogMgr *logMgr)
{
    RV_UNUSED_ARG(sad);
    RV_UNUSED_ARG(sapdNum);
    RV_UNUSED_ARG(logMgr);
    return RV_OK;
}

/********************************************************************************************
 * rvAdIPSecDeleteSASP
 *
 * OS specific function to delete 'sapdNum' of security associations and security policies.
 *
 * INPUT:
 *    sad       - array of SA data
 * sapdNum       - number of elements in 'sapd'
 *  logMgr      - handle of the log manager for this instance 
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RvStatus rvAdIPSecDeleteSASP(
    IN RvImsSAData *sad, 
    IN int sapdNum, 
    IN RvLogMgr *logMgr)
{
    RV_UNUSED_ARG(sad);
    RV_UNUSED_ARG(sapdNum);
    RV_UNUSED_ARG(logMgr);
    return RV_OK;
}


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
    IN RvLogMgr *logMgr)
{
    RV_UNUSED_ARG(logMgr);
    return RV_OK;
}

#endif /*(RV_IMS_IPSEC_ENABLED == RV_YES) */

#else
int prevent_warning_of_ranlib_has_no_symbols_rvwindowsipsec=0;
#endif /*(RV_OS_TYPE == RV_OS_TYPE_LINUX && RV_OS_VERSION & RV_OS_LINUX_REDHAT)*/

