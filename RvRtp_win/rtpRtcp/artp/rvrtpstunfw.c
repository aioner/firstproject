/************************************************************************
 File Name	   : rvrtpstunfw.h
 Description   : header file for STUN/FW traversal for SIP
***********************************************************************
        Copyright (c) 2005 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..

RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/
#include "rvrtpstunfw.h"
#include "rtputil.h"
#include "RtcpTypes.h"
#include "RtpProfileRfc3550.h"

#ifdef __cplusplus
extern "C" {
#endif
            
#if(RV_LOGMASK != RV_LOGLEVEL_NONE)   
#define RTP_SOURCE      (rtpGetSource(RVRTP_RTP_MODULE))
#define rvLogPtr        (rtpGetSource(RVRTP_RTP_MODULE))
/*
static  RvRtpLogger      rtpLogManager = NULL;
#define logMgr          (RvRtpGetLogManager(&rtpLogManager),((RvLogMgr*)rtpLogManager))
*/
#else
#define logMgr          (NULL)
#define rvLogPtr        (NULL)
#endif
#include "rtpLogFuncs.h"


#ifdef __RTP_OVER_STUN__

#undef FUNC_NAME
#define FUNC_NAME(name) "RvRtp" #name

/***************************************************************************
 * RvRtpSendRawData
 * ------------------------------------------------------------------------
 * General: Send user buffer on the socket of RTP session.
 *          (Can be used for passing of RTP through STUN protocol)
 * ------------------------------------------------------------------------
 * Arguments:
 * Input:   hRTP             - A handle to the RTP session object.
 *          buffer           - pointer to the buffer,which contains the message
 *          buffLen          - length of the message in the buffer (in bytes)
 *
 * Output: none
 * -------------------------------------------------------------------------
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 * Note: Remote RTP address have to be set before to calling this function
 *       See RvRtpAddRemoteAddress().
 ***************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSendRawData(
       IN  RvRtpSession     hRTP,
       IN  RvUint8*         buffer,
       IN  RvSize_t         buffLen)
{
    RvRtpSessionInfo*  s             = (RvRtpSessionInfo *)hRTP;
    RvStatus           status        = RV_ERROR_UNKNOWN;
    RvAddress*         destAddress   = NULL;
    RTPLOG_ENTER(SendRawData);

   if (NULL == s)
   {
       RTPLOG_ERROR_LEAVE(SendRawData, "RTP session is not opened");
       return RV_ERROR_NULLPTR;
   }

		/* Send packet to all remote addresses */
   destAddress = RvRtpAddressListGetNext(&s->addressList, NULL);
   while (destAddress != NULL) 
   {        
       status = RvTransportSendBuffer(s->transport, buffer, buffLen,
                    destAddress, 0 /*options*/, NULL/*pSent*/);
       if (status >= 0)
       {
            RTPLOG_DEBUG((RTP_SOURCE, "RvRtpSendRawData: packet was sent"));
       }
       else
       {
            RTPLOG_DEBUG((RTP_SOURCE, "RvRtpSendRawData: packet was not sent, status =%d", status));
       }
	   destAddress = RvRtpAddressListGetNext(&s->addressList, destAddress);
   }
   RTPLOG_LEAVE(SendRawData);
   return status;
}


/***************************************************************************
 * RvRtpReadRawData()
 * ------------------------------------------------------------------------
 * General: Reads raw message on RTP session socket
 * ------------------------------------------------------------------------
 * Arguments:
 * input:  hRTP          - Handle of the RTP session.
 *         buffer        - Pointer to buffer, to which raw UDP message will be placed
 *         buffSize      - The maximal size of buffer
 *        
 * output: pMessageLen               - pointer to the read message length
 *         pIsPeekMessageSupported   - Sets RV_TRUE, if RvRtpRawReadData()
 *                                     Peeks at the incoming data. The data is copied
 *                                     into the buffer but is not removed from the input queue.
 *                                     Sets RV_FALSE - otherwise (buffer is removed from the input queue).
 *                                      
 *         pRemoteAddress            - the pointer to the address of message sender
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpReadRawData(
    IN  RvRtpSession     hRTP,
    IN  RvUint8*         buffer,
    IN  RvSize_t         buffSize,
    OUT RvSize_t*        pMessageLen,
    OUT RvNetAddress*    pRemoteAddress)
{
    RvRtpSessionInfo*  s             = (RvRtpSessionInfo *)hRTP;
    RvStatus           res           = RV_ERROR_UNKNOWN;
    
    RTPLOG_ENTER(RawReadData);
    
    if (NULL == s)
    {
        RTPLOG_ERROR_LEAVE(RawReadData, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }
    res = RvTransportReceiveBuffer(s->transport, (RvUint8*)buffer,
            buffSize, 0/*options*/, (RvAddress*)pRemoteAddress, pMessageLen);
    if (res != RV_OK)
    {
        if (res == RTP_DESTINATION_UNREACHABLE)
        {
            RTPLOG_LEAVE(RawReadData);
        }
        else
        {
            
            RTPLOG_ERROR_LEAVE(RawReadData, "RvTransportReceiveBuffer failed");
        }
        return res;
    }
    RTPLOG_LEAVE(RawReadData);
    return res;
}


/************************************************************************************
 * RvRtpParseRawData()
 * description: This routine parse [and decrypt, if needed] RTP message and
 *              and sets the header of the RTP message.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *        remAddressPtr - pointer to the remote address needed for SRTP
 * output: p    - A struct of RTP param,contain the fields of RTP header.
 *         buf  - Filled buffer with decrypted data (if decryption is needed)
 * return value: If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpParseRawReadData(
		IN    RvRtpSession  hRTP,
		INOUT void*         buf,
		IN    RvInt32       len,
		IN    RvNetAddress* remAddressPtr,
        OUT   RvRtpParam*   p)
{
    RvRtpSessionInfo*	s = (RvRtpSessionInfo *)hRTP;
    RvStatus			res = RV_ERROR_UNKNOWN;
    
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpParseRawReadData"));
	
    if (s==NULL || s->profilePlugin == NULL || remAddressPtr == NULL)
    {
		RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpParseRawReadData: RTP session is not opened"));
    }
	else {

		if (
/**h.e
			s->profilePlugin != NULL &&
			s->profilePlugin->funcs != NULL &&
**/
			s->profilePlugin->funcs->readXRTP != NULL)
		{
			res = s->profilePlugin->funcs->readXRTP(s->profilePlugin, hRTP, buf, len, RV_FALSE, p, remAddressPtr);  
		}
	}
	
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpParseRawReadData"));
    return res;
}

/***************************************************************************
 *                              R T C P                                    *
 ***************************************************************************/
#undef FUNC_NAME
#define FUNC_NAME(name) "RvRtcp" #name

/***************************************************************************
 * RvRtcpSendRawData
 * ------------------------------------------------------------------------
 * General: Send user buffer on the socket of RTCP session.
 *          (Can be used for passing of RTCP through STUN protocol
 * ------------------------------------------------------------------------
 * Arguments:
 * Input:   hRTCP            - A handle to the RTP session object.
 *          buffer           - pointer to the buffer,which contains the message
 *          buffLen          - length of the message in the buffer (in bytes)
 *
 * Output: none
 * -------------------------------------------------------------------------
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 * Note: Remote RTCP address have to be set before to calling this function
 *       See RvRtcpAddRemoteAddress().
 ***************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSendRawData(
       IN  RvRtcpSession    hRTCP,
       IN  RvUint8*         buffer,
       IN  RvSize_t         buffLen)
{
    rtcpSession*       s             = (rtcpSession*) hRTCP;
    RvStatus           status        = RV_ERROR_UNKNOWN;
    RvAddress*         destAddress   = NULL;
    
    RTPLOG_ENTER(SendRawData);

   if (NULL == s)
   {
       RTPLOG_ERROR_LEAVE(SendRawData, "RTCP session is not opened");
       return RV_ERROR_NULLPTR;
   }

		/* Send packet to all remote addresses */
   destAddress = RvRtpAddressListGetNext(&s->addressList, NULL);
   while (destAddress != NULL) 
   {        
       status = RvTransportSendBuffer(s->transport, buffer, buffLen,
                    destAddress, 0 /*options*/, NULL/*pSent*/);
       if (status >= 0)
       {
            RTPLOG_DEBUG((RTP_SOURCE, "RvRtcpSendRawData: packet was sent"));
       }
       else
       {
            RTPLOG_DEBUG((RTP_SOURCE, "RvRtcpSendRawData: packet was not sent, status =%d", status));
       }
	   destAddress = RvRtpAddressListGetNext(&s->addressList, destAddress);
   }
   RTPLOG_LEAVE(SendRawData);
   return status;
}


/***************************************************************************
 * RvRtcpSetRawBufferReceivedEventHandler
 * ------------------------------------------------------------------------
 * General: Sets callback for receiving of raw data on the RTCP session socket
 * ------------------------------------------------------------------------
 * Arguments:
 * input: hRTP          - Handle of the RTP session.
 *        callback      - Pointer to the callback function that is called each time a
 *                        new packet arrives to the RTP session's socket.
 *        context       - The parameter is an application handle that identifies the
 *                        particular RTP session. The application passes the handle to
 *                        the callback.
 * output: none.
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetRawBufferReceivedEventHandler(
       IN  RvRtcpSession     hRTCP,
       IN  RvRtcpRawBufferReceived_CB callback,
       IN  void* context)
{
    rtcpSession*       s             = (rtcpSession*) hRTCP;
    RTPLOG_ENTER(SetRawBufferReceivedEventHandler);
    
    if (NULL == s)
    {
        RTPLOG_ERROR_LEAVE(SetRawBufferReceivedEventHandler, "RTCP session is not opened");
        return RV_ERROR_NULLPTR;
    }
    s->rtcpRawReadEventHandler = callback;
    s->rtcpRawReadContext      = context;

    RTPLOG_LEAVE(SetRawBufferReceivedEventHandler);
    return RV_OK;
}

#endif /* __RTP_OVER_STUN__ */

#ifdef __cplusplus
}
#endif


