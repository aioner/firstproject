#include "rvtypes.h"
#include "rverror.h"
#include "rvccore.h"
#include "rvthread.h"
#include "rvglobals.h"
#include "rvglobalindexes.h"
#include <string.h>

#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_OS_VERSION >= RV_OS_SYMBIAN_9)
static RvGlDataStruct gsGlobalsDataArray[RV_MAX_GL_DATA_INDEX];
static RvBool         gsGlobalDataStarted = RV_FALSE;
#else
#include "rvsymbgldatasrv.h"
#endif

/********************************************************************************************
 * RvStartGlobalDataServices
 * 
 * Initializes global data services
 *
 * INPUT   : None
 * OUTPUT  : started - tells whether service was initialized currently or in a 
 *                     previous call to the function;
 * RETURN  : the data pointer of NULL if fails
 */
RvStatus RvStartGlobalDataServices(OUT RvBool* started)
{
#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_OS_VERSION >= RV_OS_SYMBIAN_9)
    *started = RV_FALSE;
    if (gsGlobalDataStarted)
      return RV_OK;
    memset(gsGlobalsDataArray,0,sizeof(RvGlDataStruct)*RV_MAX_GL_DATA_INDEX);
    *started = gsGlobalDataStarted = RV_TRUE;
    return RV_OK;
#else
    return RvSymbianStartGlobalDataServices(started);
#endif
}


/********************************************************************************************
 * RvStartGlobalDataServices
 * 
 * Stops global data services
 *
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : None
 */
void RvStopGlobalDataServices(void)
{
#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN) && (RV_OS_VERSION < RV_OS_SYMBIAN_9)
    RvSymbianStopGlobalDataServices();
#endif
}

/********************************************************************************************
 * RvCreateGlobalData
 * Instruct Symbian global data server to create the global data structure for specific index
 * INPUT   : 
 *              index - index within thread vars array for the newly created structure
 *              crtF - function that creates and initializes the global data structure
 *              usrData - pointer to arbitrary data (will be passed to crtF)
 *              dstrF - function that destroys the global data structure
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI RvStatus RvCreateGlobalData(IN int index, IN RvGlobalDataCreateFunc crtF, IN void *usrData, 
                            IN RvGlobalDataDestroyFunc dstrF)
{
#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN) && (RV_OS_VERSION < RV_OS_SYMBIAN_9)
    return RvSymbCreateGlobalData(index,crtF,usrData,dstrF);
#else
    if (gsGlobalsDataArray[index].iGlobalData != NULL)
    {
        gsGlobalsDataArray[index].iRefCount++;
        return RV_OK;
    }
    gsGlobalsDataArray[index].iGlobalData = (*crtF)(index,usrData);
    if (gsGlobalsDataArray[index].iGlobalData == NULL)
        return RV_ERROR_UNKNOWN;
    gsGlobalsDataArray[index].iDestroyFunc = dstrF;
    gsGlobalsDataArray[index].iRefCount = 1;
    gsGlobalsDataArray[index].iIndex = index;
    return RV_OK;
#endif    
}

/********************************************************************************************
 * RvDestroyGlobalData
 * Instructs global data server to destroy the global data structure for specific index
 * INPUT   : 
 *              index - index within thread vars array for the destroyed structure
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI RvStatus RvDestroyGlobalData(IN int index)
{
#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN) && (RV_OS_VERSION < RV_OS_SYMBIAN_9)
    return RvSymbDestroyGlobalData(index);
#else
    if (gsGlobalsDataArray[index].iGlobalData == NULL)
        return RV_ERROR_UNKNOWN;

    gsGlobalsDataArray[index].iRefCount --;

    if (gsGlobalsDataArray[index].iRefCount)
        return RV_OK;
    
    if (gsGlobalsDataArray[index].iDestroyFunc)
        (*(gsGlobalsDataArray[index].iDestroyFunc))(index,gsGlobalsDataArray[index].iGlobalData);
            memset((void*)(gsGlobalsDataArray+index),0,sizeof(RvGlDataStruct));
    return RV_OK;
#endif
}


/********************************************************************************************
 * RvGetGlobalDataPtr
 * 
 * Fetches the pointer to the global data structure
 *
 * INPUT   :  
 *           indx - the index of the global data structure
 * OUTPUT  : None.
 * RETURN  : the data pointer of NULL if fails
 */
RVCOREAPI void* RvGetGlobalDataPtr(IN int index)
{
#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN) && (RV_OS_VERSION < RV_OS_SYMBIAN_9)
    return RvSymbGetGlPtr(index);
#else
    return gsGlobalsDataArray[index].iGlobalData;
#endif
}





















