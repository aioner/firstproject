/***********************************************************************
Filename   : rvportrange.c
Description: port range implementation.
************************************************************************
                Copyright (c) 2001 RADVISION Inc.
************************************************************************
NOTICE:
This document contains information that is proprietary to RADVISION LTD.
No part of this publication may be reproduced in any form whatsoever
without written prior approval by RADVISION LTD..

RADVISION LTD. reserves the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
************************************************************************/
#include "rvmemory.h"
#include "rvtimestamp.h"
#include "rvrandomgenerator.h"
#include "rvportrange.h"

/* Lets make error codes a little easier to type */
#define RvPortRangeErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_PORTRANGE, (_e))

/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvPrangeLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->prangeSource, p);}
#else
#define RvPrangeLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvPrangeLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->prangeSource, p);}
#else
#define RvPrangeLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvPrangeLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->prangeSource, p);}
#else
#define RvPrangeLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif


/********************************************************************************************
 * RvPortRangeSourceConstruct - Constructs PortRange module log source.
 *
 * Constructs log source to be used by common core when printing log from the
 * PortRange module. This function is applied per instance of log manager.
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvPortRangeSourceConstruct(
    IN RvLogMgr *logMgr)
{
    RvStatus result = RV_OK;

#if (RV_NET_TYPE != RV_NET_NONE)
    result = RvLogSourceConstruct(logMgr, &logMgr->prangeSource, "PORT", "Port range");
#else
    RV_UNUSED_ARG(logMgr);
#endif

    return result;
}


#if (RV_NET_TYPE != RV_NET_NONE)

/********************************************************************************************
 * RvPortRangeConstruct
 *
 * purpose : Create a port-range object.
 * input   : fromPort   - Port to start from in the range
 *           toPort     - Last port in the range
 * output  : portRange  - Object to construct
 * return  : RV_OK on success, other on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus RVCALLCONV  RvPortRangeConstruct(
    IN  RvUint       fromPort,
    IN  RvUint       toPort,
    IN  RvLogMgr*    logMgr,
    OUT RvPortRange* portRange)
{
    RvStatus            status = RV_OK;
    RvRandomGenerator   randGen;
    RvRandom            randVal;
    RvUint16            startingPort;

    RvPrangeLogEnter((&logMgr->prangeSource, "RvPortRangeConstruct(portRange=%p)", portRange));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (portRange == NULL) {
        RvPrangeLogError((&logMgr->prangeSource, "RvPortRangeConstruct: NULL"));
        return RvPortRangeErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    portRange->logMgr = logMgr;
    if (logMgr != NULL)
        portRange->prangeSource = &logMgr->prangeSource;
    else
        portRange->prangeSource = NULL;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if ((toPort > RvUintConst(0xffff)) || (fromPort > toPort)) {
        RvPrangeLogError((&logMgr->prangeSource, "RvPortRangeConstruct: Range(from=%d; to=%d)", fromPort, toPort));
        return RvPortRangeErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    portRange->fromPort = (RvUint16)fromPort;
    portRange->toPort = (RvUint16)toPort;
    portRange->numberOfFreePorts = toPort - fromPort + 1;

    RvRandomGeneratorConstruct(&randGen,
        (RvRandom)RvInt64ToRvUint32(RvInt64ShiftRight(RvTimestampGet(logMgr), 16)));
    RvRandomGeneratorGetValue(&randGen, &randVal);
    startingPort = (RvUint16)(randVal % portRange->numberOfFreePorts);
    RvRandomGeneratorDestruct(&randGen);

    status = RvLockConstruct(logMgr, &portRange->lock);
    if (status != RV_OK)
    {
        RvPrangeLogError((&logMgr->prangeSource, "RvPortRangeConstruct: RvLockConstruct=%d", status));
        return status;
    }

#if (RV_PORTRANGE_TYPE == RV_PORTRANGE_SLIM)
    portRange->next = portRange->fromPort + startingPort;

#elif (RV_PORTRANGE_TYPE == RV_PORTRANGE_FAST)
    /* Allocate linked list of ports */
    status = RvMemoryAlloc(NULL, sizeof(RvUint16) * portRange->numberOfFreePorts,
        NULL,(void**)&portRange->range);
    if (status != RV_OK)
    {
        RvLockDestruct(&portRange->lock, logMgr);
        RvPrangeLogError((&logMgr->prangeSource, "RvPortRangeConstruct: RvMemoryAlloc"));
        return status;
    }

    portRange->nextToAllocate = startingPort;
    if (startingPort == 0)
        portRange->lastToAllocate = (RvUint16)(portRange->numberOfFreePorts-1);
    else
        portRange->lastToAllocate = (RvUint16)(startingPort-1);

    {
        RvUint i;

        /* Fill in the linked list with all of the ports within the range */
        for (i = 0; i < portRange->numberOfFreePorts; i++)
            portRange->range[i] = (RvUint16)(i + 1);
        portRange->range[portRange->numberOfFreePorts-1] = 0;
    }
#endif

    RvPrangeLogLeave((&logMgr->prangeSource, "RvPortRangeConstruct(portRange=%p)", portRange));

    return status;
}


/********************************************************************************************
 * RvPortRangeDestruct
 *
 * purpose : Kill a port-range object.
 * input   : portRange  - Object to destruct
 * output  : None
 * return  : RV_OK on success, other on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus RVCALLCONV RvPortRangeDestruct(
    IN RvPortRange* portRange)
{
    RvStatus res = RV_OK;
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (portRange == NULL)
        return RvPortRangeErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(portRange->prangeSource, (portRange->prangeSource, "RvPortRangeDestruct(portRange=%p)", portRange));

#if (RV_PORTRANGE_TYPE == RV_PORTRANGE_FAST)
    res = RvMemoryFree(portRange->range,NULL);
#endif

    RvLockDestruct(&portRange->lock, portRange->logMgr);

    RvLogLeave(portRange->prangeSource,
        (portRange->prangeSource, "RvPortRangeDestruct(portRange=%p; res=%d)", portRange, res));

    return res;
}


/********************************************************************************************
 * RvPortRangeGetRange
 *
 * purpose : Returns a range of ports the port-range object deals with
 * input   : portRange  - Port range object to use
 * output  : fromPort   - Port we're starting from
 *           toPort     - Last port in range
 * return  : RV_OK on success, other on failure
 ********************************************************************************************/
RvStatus RvPortRangeGetRange(
    IN  RvPortRange*    portRange,
    OUT RvUint*         fromPort,
    OUT RvUint*         toPort)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (portRange == NULL)
        return RvPortRangeErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(portRange->prangeSource, (portRange->prangeSource, "RvPortRangeGetRange(portRange=%p)", portRange));

    if (fromPort != NULL)
        *fromPort = (RvUint)portRange->fromPort;
    if (toPort != NULL)
        *toPort = (RvUint)portRange->toPort;

    RvLogLeave(portRange->prangeSource, (portRange->prangeSource, "RvPortRangeGetRange(portRange=%p)", portRange));

    return RV_OK;
}


/********************************************************************************************
 * RvPortRangeGetNumberOfFreePorts
 *
 * purpose : Returns the number of free ports in port range
 * input   : portRange  - Port range object to use
 * output  : freePorts  - The number of free ports
 * return  : RV_OK on success, other on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus RVCALLCONV RvPortRangeGetNumberOfFreePorts(
    IN  RvPortRange*    portRange,
    OUT RvUint*         freePorts)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((portRange == NULL) || (freePorts == NULL))
        return RvPortRangeErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(portRange->prangeSource, (portRange->prangeSource, "RvPortRangeGetNumberOfFreePorts(portRange=%p)", portRange));

    RvLockGet(&portRange->lock, NULL);
    *freePorts = portRange->numberOfFreePorts;
    RvLockRelease(&portRange->lock, NULL);

    RvLogLeave(portRange->prangeSource, (portRange->prangeSource, "RvPortRangeGetNumberOfFreePorts(portRange=%p)=%d", portRange, *freePorts));

    return RV_OK;
}


/********************************************************************************************
 * RvPortRangeGetPort
 *
 * purpose : Returns a port from the port range
 * input   : portRange  - Port range object to use
 * output  : port       - Free port for the application's use
 * return  : RV_OK on success, other on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus RVCALLCONV RvPortRangeGetPort(
    IN  RvPortRange*    portRange,
    OUT RvUint*         port)
{
    RvStatus status = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((portRange == NULL) || (port == NULL))
        return RvPortRangeErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(portRange->prangeSource, (portRange->prangeSource, "RvPortRangeGetPort(portRange=%p)", portRange));

    RvLockGet(&portRange->lock, NULL);
    if (portRange->numberOfFreePorts > 0)
    {
#if (RV_PORTRANGE_TYPE == RV_PORTRANGE_FAST)
        /* We have some vacant ports - give next one away */
        *port = portRange->fromPort + portRange->nextToAllocate;

        /* Fix the list of ports in range */
        portRange->nextToAllocate = portRange->range[portRange->nextToAllocate];

#elif (RV_PORTRANGE_TYPE == RV_PORTRANGE_SLIM)
        /* Try the next port and continue (round robin) */
        *port = (RvUint)portRange->next;
        if (portRange->next < portRange->toPort)
            portRange->next++;
        else
            portRange->next = portRange->fromPort;
#endif
        portRange->numberOfFreePorts--;
    }
    else
    {
        /* No more free ports... */
        status = RvPortRangeErrorCode(RV_ERROR_OUTOFRESOURCES);
        RvLogError(portRange->prangeSource,
            (portRange->prangeSource, "RvPortRangeGetPort(portRange=%p): No more free ports", portRange));
    }
    RvLockRelease(&portRange->lock, NULL);

    RvLogLeave(portRange->prangeSource,
        (portRange->prangeSource, "RvPortRangeGetPort(portRange=%p)=%d", portRange, *port));

    return status;
}


/********************************************************************************************
 * RvPortRangeReleasePort
 *
 * purpose : Returns the port into the port range
 * input   : portRange      - Port range object to use
 *           portToRelease  - Port to return to the port range
 * output  : None
 * return  : RV_OK on success, other on failure
 ********************************************************************************************/
RVCOREAPI
RvStatus RVCALLCONV RvPortRangeReleasePort(
    IN  RvPortRange*    portRange,
    IN  RvUint          portToRelease)
{
    RvStatus status = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (portRange == NULL)
        return RvPortRangeErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(portRange->prangeSource,
        (portRange->prangeSource, "RvPortRangeReleasePort(portRange=%p,%d)", portRange, portToRelease));

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if ((portToRelease < portRange->fromPort) || (portToRelease > portRange->toPort)) {
        RvLogError(portRange->prangeSource,
            (portRange->prangeSource, "RvPortRangeReleasePort(portRange=%p): Range(port=%d)",
             portRange, portToRelease));
        return RvPortRangeErrorCode(RV_ERROR_OUTOFRANGE);
    }
#endif

    RvLockGet(&portRange->lock, NULL);
#if (RV_PORTRANGE_TYPE == RV_PORTRANGE_FAST)
    {
        RvUint16 portIndex;

        /* Calculate index of this released port */
        portIndex = (RvUint16)(portToRelease - portRange->fromPort);

        if (portRange->numberOfFreePorts > 0)
        {
            /* We have some vacant ports - add this one after the last one */
            portRange->range[portRange->lastToAllocate] = portIndex;
        }
        else
        {
            /* No ports at all - add as first one (and last at the same time) */
            portRange->nextToAllocate = portRange->lastToAllocate = portIndex;
        }

        portRange->lastToAllocate = portIndex;
        portRange->numberOfFreePorts++;
    }

#elif (RV_PORTRANGE_TYPE == RV_PORTRANGE_SLIM)
    /* Slim port range feature ignores the release of ports and just increases the number of
       free ports */
    portRange->numberOfFreePorts++;
#endif
    RvLockRelease(&portRange->lock, NULL);

    RvLogLeave(portRange->prangeSource, (portRange->prangeSource, "RvPortRangeReleasePort(portRange=%p)", portRange));

    return status;
}

#endif /* (RV_NET_TYPE != RV_NET_NONE) */
