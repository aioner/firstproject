/*
********************************************************************************
*                                                                              *
* NOTICE:                                                                      *
* This document contains information that is confidential and proprietary to   *
* RADVision LTD.. No part of this publication may be reproduced in any form    *
* whatsoever without written prior approval by RADVision LTD..                 *
*                                                                              *
* RADVision LTD. reserves the right to revise this publication and make changes*
* without obligation to notify any person of such revisions or changes.        *
********************************************************************************
*/

/*******************************************************************************
 *                              rvtransport.c
 *
 *    Author                        Date
 *    ------                        ------------
 *                                  01-July-2009
 ******************************************************************************/

/*----------------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                                */
/*----------------------------------------------------------------------------*/
#include "rvtransport.h"
#include "rvsocket.h"
#include "rvmemory.h"

#include <string.h>


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*                Abstract transport                                          */
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportConnect(
                    IN RvTransport  transp,
                    IN RvAddress*   pRemoteAddr)
{
    if (transp->iface->pfnConnect != NULL)
    {
        return (*transp->iface->pfnConnect)(transp, pRemoteAddr);
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}

RVCOREAPI
RvStatus RVCALLCONV RvTransportAccept(
                    IN  RvTransport         transp,
                    IN  RvUint32            options,
                    OUT RvTransport*        pNewTransp,
                    OUT RvAddress*          pRemoteAddr)
{
    if (transp->iface->pfnAccept != NULL)
    {
        return (*transp->iface->pfnAccept)(transp, options, pNewTransp, pRemoteAddr);
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}

RVCOREAPI
RvStatus RVCALLCONV RvTransportShutdown(IN RvTransport transp)
{
    if (transp->iface->pfnShutdown != NULL)
    {
        return (*transp->iface->pfnShutdown)(transp);
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}

RVCOREAPI
RvStatus RVCALLCONV RvTransportSendBuffer(
                    IN  RvTransport             transp,
                    IN  RvUint8*                buff,
                    IN  RvSize_t                len,
                    IN  RvAddress*              pDest,
                    IN  RvUint32                options,
                    OUT RvSize_t*               pSent)
{
    if (transp->iface->pfnSendBuffer != NULL)
    {
        return (*transp->iface->pfnSendBuffer)(
            transp, buff, len, pDest, options, pSent);
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}

RVCOREAPI
RvStatus RVCALLCONV RvTransportReceiveBuffer(
                    IN  RvTransport             transp,
                    IN  RvUint8*                buff,
                    IN  RvSize_t                len,
                    IN  RvUint32                options,
                    OUT RvAddress*              pRcvdFromAddr,
                    OUT RvSize_t*               pReceived)
{
    if (transp->iface->pfnReceiveBuffer != NULL)
    {
        return (*transp->iface->pfnReceiveBuffer)(
            transp, buff, len, options, pRcvdFromAddr, pReceived);
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}

RVCOREAPI
RvStatus RVCALLCONV RvTransportGetBytesAvailable(
    IN  RvTransport             transp,
    OUT RvSize_t*               bytesAvailable)
{
    *bytesAvailable = 0;
    if (transp->iface->pfnGetBytesAvailable != NULL)
    {
        return (*transp->iface->pfnGetBytesAvailable)(
            transp,bytesAvailable);
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}

RVCOREAPI
RvStatus RVCALLCONV RvTransportSetOption(
                    IN  RvTransport         transp,
                    IN  RvUint32            type,
                    IN  RvUint32            option,
                    IN  void*               val)
{
    if (transp->iface->pfnSetOption != NULL)
    {
        return (*transp->iface->pfnSetOption)(transp, type, option, val);
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}

RVCOREAPI
RvStatus RVCALLCONV RvTransportGetOption(
                    IN  RvTransport         transp,
                    IN  RvUint32            type,
                    IN  RvUint32            option,
                    IN  void*               val)
{
    if (transp->iface->pfnGetOption != NULL)
    {
        return (*transp->iface->pfnGetOption)(transp, type, option, val);
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}

RVCOREAPI
const RvChar* RVCALLCONV RvTransportGetType(IN  RvTransport  transp)
{
    return transp->iface->strTransportType;
}

RVCOREAPI
RvStatus RVCALLCONV RvTransportSetCallbacks(
                    IN  RvTransport             transp,
                    IN  RvTransportCallbacks*   pCallbacks)
{
    if (transp->iface->pfnSetCallbacks != NULL)
    {
        return (*transp->iface->pfnSetCallbacks)(transp, pCallbacks);
    }
    else
    {
        if (pCallbacks == NULL)
        {
            memset(&transp->callbacks, 0, sizeof(transp->callbacks));
        }
        else
        {
            transp->callbacks = *pCallbacks;
        }
        return RV_OK;
    }
}

RVCOREAPI
RvStatus RVCALLCONV RvTransportGetCallbacks(
                    IN  RvTransport             transp,
                    IN  RvTransportCallbacks*   pCallbacks)
{
    if (transp->iface->pfnGetCallbacks != NULL)
    {
        return (*transp->iface->pfnGetCallbacks)(transp, pCallbacks);
    }
    else
    {
        *pCallbacks = transp->callbacks;
        return RV_OK;
    }
}

/*******************************************************************************
 ******* Asynchronous / non-blocking functions
 *******
*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportRegisterEvent(
                    IN RvTransport        transp,
                    IN RvTransportEvents  events)
{
    if (transp->iface->pfnRegisterEvent != NULL)
    {
        return (*transp->iface->pfnRegisterEvent)(transp, events);
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}

/*******************************************************************************
 ******* Life-cycle functions
 *******
*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportAddRef(IN  RvTransport     transp)
{
    if (transp->iface->pfnAddRef != NULL)
    {
        (*transp->iface->pfnAddRef)(transp);
        return RV_OK;
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}

RVCOREAPI
RvStatus RVCALLCONV RvTransportRelease(IN  RvTransport     transp)
{
    if (transp == NULL)
    {
        return RV_ERROR_NULLPTR;
    }

    if (transp->iface->pfnRelease != NULL)
    {
        (*transp->iface->pfnRelease)(transp);
        return RV_OK;
    }
    else
    {
        return RV_ERROR_NOTSUPPORTED;
    }
}



/*----------------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS                                    */
/*----------------------------------------------------------------------------*/
