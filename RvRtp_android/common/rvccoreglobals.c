#include "rvtypes.h"
#include "rverror.h"
#include "rvccore.h"
#include "rvthread.h"
#include "rvccoreglobals.h"

#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_OS_VERSION >= RV_OS_SYMBIAN_9)
static RvCCoreGlobals gsCCGlobals;
#else
#endif


/********************************************************************************************
 * RvCCoreInitializeGlobals
 * 
 * Creates and initializes the global data structure of common core
 *
 * INPUT   :  
 *           indx - the index of the global data structure (always RV_CCORE_GLOBALS_INDEX)
 *           usrData - not used
 *           
 * OUTPUT  : None.
 * RETURN  : the data pointer of NULL if fails
 */

static const RvThreadAttr gsRvDefaultThreadAttr = RV_THREAD_ATTRIBUTE_DEFAULT;

void* RvCCoreInitializeGlobals(int index, void* usrData)
{
    RvCCoreGlobals *gl;

    RV_UNUSED_ARG(index);
    RV_UNUSED_ARG(usrData);

#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)
#if (RV_OS_VERSION < RV_OS_SYMBIAN_9)    
    gl = RvSymbianAllocateGlobalData(NULL);    
#else
    gl = RvSymbianAllocateGlobalData(&gsCCGlobals);    
#endif
#else
    gl = &gsCCGlobals;
    gl->osSpecificGlobals = NULL;
#endif

    gl->_RvCCoreInitCount = 0;

#if (RV_LOCK_TYPE != RV_LOCK_NONE)
    {
        const RvLockAttr gcRvDefaultLockAttr = RV_LOCK_ATTRIBUTE_DEFAULT;
        gl->_RvDefaultLockAttr = gcRvDefaultLockAttr;
    }
#endif    

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    gl->_rvLogInsideTextAny = RV_FALSE;
#endif

    gl->_logThreadNum = 0;

#if (RV_MUTEX_TYPE != RV_MUTEX_NONE)
    /* define the default attributes since they may be a structure */
    {
        const RvMutexAttr gcRvDefaultMutexAttr = RV_MUTEX_ATTRIBUTE_DEFAULT;
        gl->_RvDefaultMutexAttr = gcRvDefaultMutexAttr;
    }
#endif

#if (RV_SEMAPHORE_TYPE != RV_SEMAPHORE_NONE)
    /* define the default attributes since they may be a structure */
    gl->_RvDefaultSemaphoreAttr = RV_SEMAPHORE_ATTRIBUTE_DEFAULT;
#endif

    gl->_RvDefaultThreadAttr  = gsRvDefaultThreadAttr;

    gl->_RvCBaseInitCount = 0;
    
#if (RV_HOST_HAS_STATIC_ADDR == RV_YES)    
    /* List of host addresses - allocated statically */
    gl->_rvHostLocalNumOfIps = RV_HOST_MAX_ADDRS;
    gl->_rvHostLocalListInitialized = RV_FALSE;    
#endif    


#if ((RV_SELECT_TYPE == RV_SELECT_SELECT) || \
    (RV_SELECT_TYPE == RV_SELECT_POLL) || \
    (RV_SELECT_TYPE == RV_SELECT_DEVPOLL) || \
    (RV_SELECT_TYPE == RV_SELECT_EPOLL) || \
    (RV_SELECT_TYPE == RV_SELECT_KQUEUE)  || \
    (RV_SELECT_TYPE == RV_SELECT_SYMBIAN))
    /* Maximum number of file descriptors to allocate operating systems */
    gl->_rvSelectMaxDescriptors = 0;    
#endif

    gl->_rvSelectTlsVarIndex = 0;

    gl->_tosSupported = RV_FALSE;

    gl->_rvDefaultAlloc = rvConstDefaultAlloc;

	gl->_gsStatusTLS = 0;
    return gl;
}


/********************************************************************************************
 * RvCCoreDestroyGlobals
 * 
 * Destroys the global data structure of common core
 *
 * INPUT   :  
 *           indx - the index of the global data structure (always RV_CCORE_GLOBALS_INDEX)
 *           glDataPtr - pointer returned by RvCCoreInitializeGlobals
 *           
 * OUTPUT  : None.
 * RETURN  : the data pointer of NULL if fails
 */
void RvCCoreDestroyGlobals(int index, void* glDataPtr)
{
    RV_UNUSED_ARG(index);
#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN)
    RV_UNUSED_ARG(glDataPtr);
#else
    RvSymbianDestroyGlobalData(glDataPtr);
#endif
}
