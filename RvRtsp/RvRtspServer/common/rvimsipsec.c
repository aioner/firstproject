/***********************************************************************
Filename   : rvimsipsec.c
Description: Interface functions for IMS IPSec support.
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
#include "rvtypes.h"
#include "rvaddress.h"
#include "rvlock.h"
#include "rvmemory.h"
#include "rvlog.h"
#include "rvtimestamp.h"

#include <string.h>
#include <time.h>

#if (RV_IMS_IPSEC_ENABLED == RV_YES)

#include "rvimsipsecprivate.h"

/********************************************************************************************
 * RvIpsecInit
 * Open the IPSEC Module.
 * INPUT   : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvIpsecInit(void)
{
    return rvAdIpsecInit();
}

/********************************************************************************************
 * RvIpsecEnd
 * Closes the IPSEC Module.
 * INPUT   : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvIpsecEnd(void)
{
    return rvAdIpsecEnd();
}

/******************************************************************************
 * RvIpsecConstruct
 *
 * Allocates and initiates a new IPsec engine.
 * In case select engine was already constructed for the current thread,
 * pointer to the existing agent will be returned.
 *
 *
 * INPUT   : logMgr         - log manager
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvIpsecConstruct(IN RvLogMgr*  logMgr)
{
    /* Get rid of "unused parameter" warning on systems,
       where the rvAdIpsecConstruct macro is mapped to nothing. */
    RV_UNUSED_ARG(logMgr);

    return rvAdIpsecConstruct(logMgr);
}

/******************************************************************************
 * RvIpsecDestruct
 *
 * Destruct a select engine
 * Not thread-safe function.
 *
 * INPUT   : logMgr - log manager
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvIpsecDestruct(IN RvLogMgr* logMgr)
{
    /* Get rid of "unused parameter" warning on systems,
       where the rvAdIpsecConstruct macro is mapped to nothing. */
    RV_UNUSED_ARG(logMgr);

    return rvAdIpsecDestruct(logMgr);
}

/********************************************************************************************
 * rvIpsecSourceConstruct
 *
 * Constructs a Log Source object for the IPSec module.
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RvStatus rvIpsecSourceConstruct(
    IN RvLogMgr* logMgr)
{
    RvStatus status = RV_OK;
    status = RvLogSourceConstruct(logMgr, &logMgr->imsIpsecSource, "IMSIPSEC", "IMS IPSec support");
    return status;
}

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
    IN RvLogMgr *logMgr)
{
#define FUNCTION_NAME "rvImsIPSecEstablishSAs"
    RvIPSecSAData *sapdp;
    
    IMSIPSEC_LOG_ENTER;

    memset(&sad->iIpSecSpecific,0,sizeof(RvImsIpSecSpecific));
    sapdp = &sad->iIpSecSpecific.iSAP[0];

    if (sad->iPeerAddress.addrtype != sad->iLclAddress.addrtype)
    {
        IMSIPSEC_LOG_ERROR_0("The source and destination address types are not the same");
        goto failure;
    }

    if (rvAdSetOSSpecific(sad,logMgr) != RV_OK)
        goto failure;
  
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)    
    {
        RvChar addr1[256],addr2[256];

        IMSIPSEC_LOG_INFO_4("Config SPIs: InClient %u, InSrv %u, OutClient %u, OutSrv %u",
            sad->iSpiInClnt,sad->iSpiInSrv,sad->iSpiOutClnt,sad->iSpiOutSrv);
   
        IMSIPSEC_LOG_INFO_4("Config PORTs: lclClient %d, lclServer %d, peerClient %d, peerServer %d",
            sad->iLclPrtClnt, sad->iLclPrtSrv, sad->iPeerPrtClnt, sad->iPeerPrtSrv);

        IMSIPSEC_LOG_INFO_2("Config Addrs: lcl %s, peer %s",
            RvAddressGetString(&sad->iLclAddress,sizeof(addr1),addr1),
            RvAddressGetString(&sad->iPeerAddress,sizeof(addr2),addr2));
    }
#endif /*#if (RV_LOGMASK != RV_LOGLEVEL_NONE)*/
        
    /* first the out bound will be created */
    sapdp->iSPI = sad->iSpiOutClnt;
    sapdp->iLclPort = sad->iLclPrtClnt;
    sapdp->iRemPort = sad->iPeerPrtSrv;
    sapdp->iDirection = RV_IPSECMODE_DIR_OUT;
        
    sapdp++;
    sapdp->iSPI = sad->iSpiOutSrv;
    sapdp->iLclPort = sad->iLclPrtSrv;
    sapdp->iRemPort = sad->iPeerPrtClnt;
    sapdp->iDirection = RV_IPSECMODE_DIR_OUT;
    
    sapdp++;
    sapdp->iSPI = sad->iSpiInClnt;
    sapdp->iLclPort = sad->iLclPrtClnt;
    sapdp->iRemPort = sad->iPeerPrtSrv;
    sapdp->iDirection = RV_IPSECMODE_DIR_IN;

    sapdp++;
    sapdp->iSPI = sad->iSpiInSrv;
    sapdp->iLclPort = sad->iLclPrtSrv;
    sapdp->iRemPort = sad->iPeerPrtClnt;
    sapdp->iDirection = RV_IPSECMODE_DIR_IN;
           
    if (rvAdIPSecAddSASP(sad,4,logMgr) != RV_OK)
    {
        IMSIPSEC_LOG_ERROR_0("rvAdIPSecAddSASP failed to add SAs");
        goto failure;        
    }
    IMSIPSEC_LOG_INFO_0("Successfully added 4 SA entries");

    IMSIPSEC_LOG_LEAVE_0("Success");
    return RV_OK;

failure:

    IMSIPSEC_LOG_LEAVE_0("Leaving without success");
    return RV_ERROR_UNKNOWN;    
#undef FUNCTION_NAME
}

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
    IN RvLogMgr *logMgr)
{
#define FUNCTION_NAME "rvImsIPSecDestroySAs"
    RvStatus retSt = RV_OK;
    IMSIPSEC_LOG_ENTER;

    if (rvAdIPSecDeleteSASP(sad,4,logMgr) != RV_OK)
        retSt = RV_ERROR_UNKNOWN;

    IMSIPSEC_LOG_LEAVE;
    return retSt;    
#undef FUNCTION_NAME
    
}

/********************************************************************************************
 * rvImsIPSecCleanAll
 *
 * Removes all SA/SP set on this host.
 * Not implemented on VxWorks.
 *
 * INPUT:
 *  logMgr      - handle of the log manager for this instance 
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
********************************************************************************************/
RVCOREAPI
RvStatus rvImsIPSecCleanAll(
    IN RvLogMgr *logMgr)
{
#define FUNCTION_NAME "rvImsIPSecCleanAll"
    RvStatus retSt = RV_OK;
    IMSIPSEC_LOG_ENTER;
    
    if (rvAdIPSecCleanAll(logMgr) != RV_OK)
        retSt = RV_ERROR_UNKNOWN;
        
    IMSIPSEC_LOG_LEAVE;
    return retSt;    
#undef FUNCTION_NAME
}


void* spiPoolPageAllocCB(RvSize_t size, void *data)
{
    void *ptr;
    RV_UNUSED_ARG(data);
    return (RvMemoryAlloc(NULL,size,NULL,&ptr)==RV_OK)?ptr:NULL;
}

void spiPoolPageFreeCB(void *ptr, void *data)
{
    RV_UNUSED_ARG(data);
    RvMemoryFree(ptr,NULL);
}



/********************************************************************************************
 * rvImsConstructCfg
 *
 * Initialize the configuratiuon of IMS
 *
 * INPUT:
 *  cfg - IMS configuration data
 *  spiRangeStart, spiRangeFinish - defines the range where the SPI numbers will be 
 *                                  allocated. 
 *  expectedSPIsNum - expected max number of security channels established by application
 *  logMgr      - handle of the log manager for this instance 
 *
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus rvImsConstructCfg(
    IN RvImsCfgData *cfg,
    IN RvUint32 spiRangeStart,
    IN RvUint32 spiRangeFinish,
    IN RvSize_t expectedSPIsNum,
    IN RvLogMgr *logMgr)
{
    RvImsSpiStruct spis;
    RvSize_t maxSz;
    RvObjPoolFuncs poolCB;
    RvRandom seed;

#define FUNCTION_NAME "rvImsConstructCfg"
    IMSIPSEC_LOG_ENTER;

    memset(cfg,0,sizeof(RvImsCfgData));
    
    cfg->iArraySz = 4;

    /*
     *	expectedSPIsNum is the expected max number of security agreements;
     *  we allocate two SPIs for each agreement and another two will be marked
     *  as busy. Thus the hash table size is 4 times bigger than then
     *  initial expectedSPIsNum value;
     */
    expectedSPIsNum *= 4; 

    while (cfg->iArraySz < expectedSPIsNum)
    {
        cfg->iArraySz <<= 1; /* each time multiply by two */
        if (cfg->iArraySz >= RV_IMS_SPI_HASH_SIZE)
        {
            cfg->iArraySz = RV_IMS_SPI_HASH_SIZE;
            break;
        }
    }

    cfg->iMsk = (cfg->iArraySz-1);

    if (RvMemoryAlloc(NULL,cfg->iArraySz*sizeof(RvImsSpiStruct*),NULL,(void**)&cfg->iArray) != RV_OK)
    {
        IMSIPSEC_LOG_ERROR_0("Failed to allocate array")
        return RV_ERROR_OUTOFRESOURCES;
     
    }        
    memset(cfg->iArray,0,cfg->iArraySz*sizeof(RvImsSpiStruct*));

    maxSz = expectedSPIsNum;
#define ITEMS_PER_PAGE 30 /* the sizeof(RvImsSpiStruct) is 24; 24*30 = 720  */
    if (maxSz < ITEMS_PER_PAGE)
        maxSz = ITEMS_PER_PAGE;

	memset(&poolCB, 0, sizeof(RvObjPoolFuncs));
	poolCB.pagealloc = spiPoolPageAllocCB;
	poolCB.pagefree  = spiPoolPageFreeCB;

    if (RvObjPoolConstruct(&spis,&spis.iPoolElement,&poolCB,sizeof(RvImsSpiStruct),
                        ITEMS_PER_PAGE,0,RV_OBJPOOL_TYPE_DYNAMIC,RV_OBJPOOL_SALVAGE_ALLOWED,
                        maxSz,0,10,&cfg->iPoolObj) == NULL)
    {
        IMSIPSEC_LOG_ERROR_0("Failed to allocate pool object")
        RvMemoryFree(cfg->iArray,NULL);
        cfg->iArray = NULL;
        return RV_ERROR_OUTOFRESOURCES;
    }
    RvLockConstruct(logMgr,&cfg->iSPIRangeLock);

    seed = RvInt64ToRvUint32(RvTimestampGet(NULL)); /* NULL - logMgr */
    RvRandomGeneratorConstruct(&cfg->iRandObj, seed);


    cfg->iSPIRangeLow = spiRangeStart;
    cfg->iSPIRangeHigh = spiRangeFinish;
    cfg->iInitialized = RV_TRUE;

    IMSIPSEC_LOG_LEAVE;    
    return RV_OK;    
#undef FUNCTION_NAME    
}

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
 *  None
 ********************************************************************************************/
RVCOREAPI
void rvImsDestructCfg(
    IN RvImsCfgData *cfg,
    IN RvLogMgr *logMgr)
{
#define FUNCTION_NAME "rvImsDestructCfg"
    IMSIPSEC_LOG_ENTER;
    if (cfg->iInitialized)
    {
        RvImsSpiStruct *s,*sn;
        RvSize_t cnt;

        for (cnt = 0; cnt<cfg->iArraySz; cnt++)
        {
            s = cfg->iArray[cnt];
            while (s)
            {
                sn = s->iNext;
                RvObjPoolReleaseItem(&cfg->iPoolObj,(void*)s);
                s = sn;
            }            
        }
        RvMemoryFree(cfg->iArray,NULL);
        RvLockDestruct(&cfg->iSPIRangeLock,logMgr);
        RvObjPoolDestruct(&cfg->iPoolObj);
        RvRandomGeneratorDestruct(&cfg->iRandObj);        
    }
    IMSIPSEC_LOG_LEAVE;    
#undef FUNCTION_NAME    
}                         

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
 *              If the *spi1 or *spi2 is set to non-zero value it will not be changed.
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus rvImsGetPairOfSPIs(
    INOUT RvUint32* spi1,
    INOUT RvUint32* spi2,
    IN RvImsCfgData *cfg,
    IN RvLogMgr *logMgr)
{
#define FUNCTION_NAME "rvImsGetPairOfSPIs"
    RvStatus retSt = RV_OK;
    IMSIPSEC_LOG_ENTER;
    retSt = rvImsHandlePairsOfSPIs(SPIGet,spi1,spi2,cfg,logMgr);    
    IMSIPSEC_LOG_LEAVE;
    return retSt;    
#undef FUNCTION_NAME
}

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
    IN RvLogMgr *logMgr)
{
#define FUNCTION_NAME "rvImsReturnSPIs"
    RvStatus retSt = RV_OK;
    IMSIPSEC_LOG_ENTER;
    retSt = rvImsHandlePairsOfSPIs(SPIReturn,&spi1,&spi2,cfg,logMgr);    
    retSt = rvImsHandlePairsOfSPIs(SPIReturn,&peerSpi1,&peerSpi2,cfg,logMgr);    
    IMSIPSEC_LOG_LEAVE;
    return retSt;    
#undef FUNCTION_NAME
}

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
    IN RvLogMgr *logMgr)
{
#define FUNCTION_NAME "rvImsSetAsBusyPairOfSPIs"
    RvStatus retSt = RV_OK;
    IMSIPSEC_LOG_ENTER;
    retSt = rvImsHandlePairsOfSPIs(SPIMarkAsBusy,&spi1,&spi2,cfg,logMgr);    
    IMSIPSEC_LOG_LEAVE;
    return retSt;    
#undef FUNCTION_NAME
}


/********************************************************************************************
 * rvImsHandleSPI
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
 *  spi - to handle
 *
 *
 * RETURNS:
 *  RV_OK on success, other values on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus rvImsHandleSPI(
    IN RvImsSPIAction act,
    INOUT RvUint32* spi,
    IN RvImsCfgData *cfg,
    IN RvLogMgr *logMgr)
{
#define FUNCTION_NAME "rvImsHandleSPI"    
    RvUint32 cnt = 0;
    RvSize_t sz,sz2,ind;
    RvImsSpiStruct *s, *sp;
	RvStatus retVal;
	RvBool bExternalVal = RV_FALSE;

    IMSIPSEC_LOG_ENTER_2("act %d, spi %d",act,*spi);        

    /* 
     * if *spi1 or *spi2 is not zero: Get operation should not act on this specific spi;
     * if *spi1 or *spi2 is zero: Return and MarkAsBusy should not act on this specific spi;
     */
    if ((*spi == 0 && (act == SPIReturn || act == SPIMarkAsBusy)) || 
        (*spi && act == SPIGet))
        return RV_OK;

    sz = cfg->iSPIRangeHigh - cfg->iSPIRangeLow + 1;
    sz2 = sz<<1;   
    
    for (;;)
    {
        if (act == SPIGet)
        {
            cnt ++;
            if (cnt > sz2)
            {
                /* we do not want to enter the endless loop with random numbers thus
                   we limit the number of tries to twice of range size */
                IMSIPSEC_LOG_ERROR_1("Failed to get the SPI number, after %d tries",cnt);            
                return RV_ERROR_OUTOFRESOURCES;
            }
			retVal = rvAdIpsecPickSpiValue(logMgr,cfg->iSPIRangeLow,cfg->iSPIRangeHigh,spi);
			if (retVal == RV_OK)
			{
				bExternalVal = RV_TRUE;
			}
			else if (retVal == RV_ERROR_NOTSUPPORTED)
			{
				/* If system doesn't support pool of SPIs, use random value */
				RvRandomGeneratorGetInRange(&cfg->iRandObj,sz,spi);
				*spi += cfg->iSPIRangeLow;
				bExternalVal = RV_FALSE;
			}
			else /*	if (retVal != RV_OK) */
			{
                IMSIPSEC_LOG_ERROR_0("rvAdIpsecPickSpiValue() failed");            
                return retVal;
			}
        }
        else 
        {        
            if (*spi < cfg->iSPIRangeLow || *spi > cfg->iSPIRangeHigh)
                return RV_OK;
        }

        /* this is hash function; we take the last bits of the *spi and use
           it as the index within the array of linked lists */
        ind = cfg->iMsk & *spi;
        s = cfg->iArray[ind];
        sp = NULL;

        while (s)
        {
            if (s->iSPI == *spi)
                break;
            sp = s;
            s = s->iNext;
        }
    
        if (s && act == SPIMarkAsBusy)
        {
            s->iRefCount++;
            return RV_OK;
        }   
    
        if (act == SPIGet || act == SPIMarkAsBusy)
        {
            if (s)
            {   /* can be here only for SPIGet */
                /* this SPI (the 'num' value) is in use,
				   free it and try another one*/
                if (bExternalVal == RV_TRUE)
                {
				    retVal = rvAdIpsecFreeSpiValue(logMgr, *spi);
                }
                continue;
            }
        
            /* add another instance of RvImsSpiStruct */
            s = (RvImsSpiStruct*)RvObjPoolGetItem(&cfg->iPoolObj);
            if (!s)
            {
                IMSIPSEC_LOG_ERROR_0("Failed to get the SPI struct from the pool");
                return RV_ERROR_OUTOFRESOURCES;
            }
            memset(s,0,sizeof(RvImsSpiStruct));
            s->iRefCount ++;
            s->iSPI = *spi;
			s->bExternal = bExternalVal;
            if (sp)
                sp->iNext = s;
            else
                cfg->iArray[ind] = s;
        }
        else if (act == SPIReturn)
        {
            if (!s)
            {
                IMSIPSEC_LOG_INFO_1("Removing the SPI(%d) that is not on the list", *spi);
                return RV_OK;
            }
            s->iRefCount--;
            if (s->iRefCount)
                return RV_OK;
        
            if (!sp)
                cfg->iArray[ind] = s->iNext;
            else
                sp->iNext = s->iNext;

			/* Release SPI value, if it was not chosen locally (randomly) */
			if (s->bExternal == RV_TRUE)
			{
                retVal = rvAdIpsecFreeSpiValue(logMgr, s->iSPI);
			}

            RvObjPoolReleaseItem(&cfg->iPoolObj,(void*)s);
        }
        break;
    }    

    return RV_OK;
#undef FUNCTION_NAME
}

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
 *  spi1,spi2 - the SPIs to handle
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
    IN RvLogMgr *logMgr)
{
#define FUNCTION_NAME "rvImsHandlePairsOfSPIs"
    RvStatus retSt;

    IMSIPSEC_LOG_ENTER_3("act %d, spi1 %d, spi2 %d",act,*spi1,*spi2);        

    RvLockGet(&cfg->iSPIRangeLock,logMgr);
    retSt = rvImsHandleSPI(act,spi1,cfg,logMgr);
    if (retSt == RV_OK)
    {
        retSt = rvImsHandleSPI(act,spi2,cfg,logMgr);
        if (retSt != RV_OK)
        {
            if (act == SPIGet)
                rvImsHandleSPI(SPIReturn,spi1,cfg,logMgr);
        }
    }
    RvLockRelease(&cfg->iSPIRangeLock,logMgr);

    IMSIPSEC_LOG_LEAVE;
    return retSt;    
#undef FUNCTION_NAME
}


#else
int prevent_warning_of_ranlib_has_no_symbols_rvimsipsec=0;
#endif /* (RV_IMS_IPSEC_ENABLED == RV_YES) */

