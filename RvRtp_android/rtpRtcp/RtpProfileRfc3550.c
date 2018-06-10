/************************************************************************
 File Name	   : RtpProfileRfc3550.c
 Description   : scope: Private
                 implementation of RtpProfilePlugin for RTP (RFC 3550)
*************************************************************************/
/***********************************************************************
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

#include "rtputil.h"
#include "RtpProfileRfc3550.h"
#include "RtcpProfileRfc3550.h"
#include "rvrtplogger.h"
#include "rtcp.h" /* for RvRtcpSessionSetParam */
#include "rvrtpnatfw.h"

#define RTP_DESTINATION_UNREACHEBLE  RV_ERROR_UNKNOWN

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
#undef FUNC_NAME
#define FUNC_NAME(name) "RtpProfileRfc3550" #name


#ifdef __cplusplus
extern "C" {
#endif

extern RvRtpInstance rvRtpInstance;
static RtpProfilePluginCallbacks RtpProfileRfc3550Callbacks =
{
        RtpProfileRfc3550Read,
        RtpProfileRfc3550Write,
        RtpProfileRfc3550RtcpRawReceive,
        RtpProfileRfc3550RtcpRawSend,
        /*RtpProfilePluginRemoveRemoteAddressCB*/      NULL,
        /*RtpProfilePluginRemoveRemoteAddressesAllCB*/ NULL,
        RtpProfileRfc3550Release
};
/************************************************************************************
* RtpProfileRfc3550Read
* description: This routine sets the header of the RTP message.
* input: 
*        plugin - pointer to this plugin
*        hRTP  - Handle of the RTP session.
*        buf   - Pointer to buffer containing the RTP packet with room before first
*                payload byte for RTP header.
*        len   - Length in bytes of buf.
*
* output: p            - A struct of RTP param,contain the fields of RTP header.
*        remAddressPtr - pointer to the remote address
* return value: If no error occurs, the function returns the non-negative value.
*                Otherwise, it returns a negative value.
***********************************************************************************/
RvInt32  RtpProfileRfc3550Read(
        IN  struct __RtpProfilePlugin* plugin,
		IN  RvRtpSession               hRTP,
		IN  void *                     buf1,
		IN  RvInt32                    len,
        IN  RvBool                     isPeekMessage,
		OUT RvRtpParam*                p,
		OUT RvNetAddress*              remAddressPtr)
{
    RvRtpSessionInfo* s  = (RvRtpSessionInfo *)hRTP;
    RvAddress*        remoteAddressPtr = (RvAddress*) remAddressPtr;
    RvStatus          res = RV_ERROR_UNKNOWN;
    RvUint8*          bufferPtr = (RvUint8*) buf1;
    RvSize_t          messageSize = 0;
    RvUint32		  forwardBytes = 0;
	RvUint8           convertedBufferPtr[32]; /* temporary storage for RTP packet's header. */
	
    RV_UNUSED_ARG(plugin);
    RTPLOG_ENTER(Read); 
    
    if (isPeekMessage)
    {   
        res = RvTransportReceiveBuffer(s->transport, bufferPtr,
                (RvSize_t)len, 0/*options*/, (RvAddress*)remAddressPtr, &messageSize);
        if (res == RTP_DESTINATION_UNREACHEBLE)
        {
        /* error of sending from socket to an unreachable destination.
        Since used the same socket for sending and receiving, if sending is failed,
        here we receive the sending error. In this case logging will be bombed by error messages.
            This is the design defect, but it allow to use 1 socket per RTP session instead of two.*/ 
            /* RTPLOG_ERROR_LEAVE(Read, "RvSocketReceiveBuffer(). The sending destination is unreachable."); 
            it is intentionally omitted not to produce printings loops */
            
            RTPLOG_LEAVE(Read);  
            return res; 
        }
        if (res != RV_OK)
        {
            RTPLOG_ERROR_LEAVE(Read, "RvTransportReceiveBuffer() Read failed failed.");  
            return res;
        }    
        
        p->len = (RvInt32)messageSize;  
    }
#ifdef __RTP_OVER_STUN__
    else
	{	
		if (p->len == 0)
		{
			p->len = len;  
		}
    }
#endif

#ifdef __H323_NAT_FW__
    if (s->demux != NULL)
    {
        forwardBytes = RvRtpNatMultiplexIdSize();
    }
#endif
    if (p->len < (RvInt32)(forwardBytes + RvRtpGetHeaderLength()))
	{
		RvAddressDestruct(&remoteAddress);
        if (p->len != 0)
        {        
            RTPLOG_ERROR_LEAVE(Read, "This packet is probably corrupted or session is closed in blocking mode.");		
        }
        /* nothing to read: false alarm */
        return RV_ERROR_UNKNOWN;
	}
#ifdef __H323_NAT_FW__
    if (s->demux != NULL)
    {  
        bufferPtr	+= forwardBytes;
        p->len		-= forwardBytes;
        messageSize	-= forwardBytes;
    }
#endif
	if(s->encryptionPlugInPtr != NULL)
    {
        RvRtpEncryptionData encryptionData;
        RvKey               decryptionKey;
		
        RTPLOG_DEBUG((RTP_SOURCE,"RtpProfileRfc3550Read: Decrypting"));
        RvKeyConstruct(&decryptionKey);
        RvRtpEncryptionDataConstruct(&encryptionData);
		
        /* Setup the Encryption data */
        RvRtpEncryptionDataSetIsRtp(&encryptionData, RV_TRUE);
        RvRtpEncryptionDataSetIsEncrypting(&encryptionData, RV_FALSE);
        RvRtpEncryptionDataSetMode(&encryptionData, s->encryptionMode);
        RvRtpEncryptionDataSetLocalAddress(&encryptionData, NULL);
        RvRtpEncryptionDataSetRemoteAddress(&encryptionData, remAddressPtr);
		
        if(RvRtpEncryptionModeIsPlainTextRtpHeader(s->encryptionMode))
        {
            /* Extract the RTP header from the packet */
			/* !!!           rvRtpHeaderUnserialize(headerPtr, bufferPtr);*/
			memcpy(convertedBufferPtr, bufferPtr, RvRtpGetHeaderLength());
			ConvertFromNetwork(convertedBufferPtr, 0, 3);
			
			/* If the SSRC is ours and the is our IP, we should ignore this packet - it probably came
			in as a multicast packet we sent out. */
			if ( (s->sSrc == ((RvUint32*)convertedBufferPtr)[2]) && isMyIP(remoteAddressPtr) )
			{
				RvAddressDestruct(&remoteAddress);
				RvKeyDestruct(&decryptionKey);
				RvRtpEncryptionDataDestruct(&encryptionData);
                RTPLOG_ERROR_LEAVE(Read, "failed.");						
				return RV_ERROR_UNKNOWN;
			}
			res = RvRtpUnpack(hRTP, bufferPtr, len, p);
			
            RvRtpEncryptionDataSetRtpHeader(&encryptionData, p);
			bufferPtr += RvRtpGetHeaderLength();
			messageSize -= RvRtpGetHeaderLength();

        }
        else
        {
            RvRtpEncryptionDataSetRtpHeader(&encryptionData, NULL);
        }
		
        if(s->encryptionKeyPlugInPtr != NULL)
        {
            RvRtpEncryptionKeyPlugInGetKey(s->encryptionKeyPlugInPtr, &encryptionData, &decryptionKey);
        }
		
        /* Decrypt the data */
        RvRtpEncryptionPlugInDecrypt(s->encryptionPlugInPtr,
			bufferPtr,
			bufferPtr,
			(RvUint32)messageSize,
			&encryptionData, &decryptionKey);
		
        if(!RvRtpEncryptionModeIsPlainTextRtpHeader(s->encryptionMode))
        {
            /* Extract the RTP header from the packet */
			/* !!!           rvRtpHeaderUnserialize(headerPtr, bufferPtr);*/
			/* Copy the received packet's header to temporary storage and convert it to host order */
			memcpy(convertedBufferPtr, bufferPtr, RvRtpGetHeaderLength());
			ConvertFromNetwork(convertedBufferPtr, 0, 3);
			
			/* If the SSRC is ours and the is our IP, we should ignore this packet - it probably came
			in as a multicast packet we sent out. */
			if ( (s->sSrc == ((RvUint32*)convertedBufferPtr)[2]) && isMyIP(remoteAddressPtr) )
			{
				RvAddressDestruct(&remoteAddress);				
				RvKeyDestruct(&decryptionKey);
				RvRtpEncryptionDataDestruct(&encryptionData);
                RTPLOG_ERROR_LEAVE(Read, "failed.");	
				return RV_ERROR_UNKNOWN;
			}
            /* Extract the RTP header from the packet */
            /*rvRtpHeaderUnserialize(headerPtr, bufferPtr);*/
			res = RvRtpUnpack(hRTP, bufferPtr, len, p);
        }
		
		RvKeyDestruct(&decryptionKey);
        RvRtpEncryptionDataDestruct(&encryptionData);
    }
    else
    {
	    RvAddressDestruct(&remoteAddress);	
		/* Copy the received packet's header to temporary storage and convert it to host order */
		memcpy(convertedBufferPtr, bufferPtr, 12);
		ConvertFromNetwork(convertedBufferPtr, 0, 3);
		
		/* If the SSRC is ours and the is our IP, we should ignore this packet - it probably came
		in as a multicast packet we sent out. */
		if ( (s->sSrc == ((RvUint32*)convertedBufferPtr)[2]) && isMyIP(remoteAddressPtr) )
        {
            RTPLOG_ERROR_LEAVE(Read, "failed.");	
            return RV_ERROR_UNKNOWN;
        }
        /* Extract the RTP header from the packet */
		res = RvRtpUnpack(hRTP, bufferPtr, len, p);
    }
#ifdef __H323_NAT_FW__
    if (s->demux != NULL)
    {        
        p->len      += forwardBytes;
        p->sByte    += forwardBytes;
    }
#endif
    RTPLOG_LEAVE(Read);
    return res;
}

/************************************************************************************
 * RtpProfileRfc3550Write
 * description: this function is called, when plugin have to send the XRTP packet.
 * input: hRTP   - Handle of the RTP session.
 *		  ssrc   - The SSRC to insert into packet 
 *        buf    - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len    - Length in bytes of buf.
 *        p      - A struct of RTP param.
 * output: none.
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 * IMPORTANT NOTE: in case of encryption according to RFC 3550 (9. Security) 
 *   p.len must contain the actual length of buffer, which must be
 *   more then len, because of encryption padding
 ***********************************************************************************/
#define RTP_WORK_BUFFER_SIZE (1600)
RvInt32  RtpProfileRfc3550Write(
        IN  RvRtpSession	hRTP,
		IN  RvInt32			ssrc,		
        IN  void *			buf,
        IN  RvInt32			len,
        IN  RvRtpParam*		p)
{
    RvStatus                res;
	RvUint8                 workBuffer[RTP_WORK_BUFFER_SIZE];
	RvRtpEncryptionData     encryptionData;
	RvKey                   encryptionKey;
    RvRtpSessionInfo*       s                     = (RvRtpSessionInfo *)hRTP;
	RvUint8*                bufferPtr             = (RvUint8*) buf;
	RvUint32				headerSize            = 0;
    RvInt8                  padding               = 0;
	RvUint32                dataSize              = len-p->sByte;
    RvInt32                 UserAllocationSize    = p->len; /* if encription is used interface feature */
	RvAddress*              destAddress           = NULL;
    RvBool                  bDestAddressAvailable = RV_FALSE;
	RvUint8*				bufferToSend = (RvUint8*)buf;
	
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpWrite"));

	if (NULL == s)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "NULL session handle or session is not opened"));        
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpWrite"));
		return RV_ERROR_NULLPTR;
    }
	
    p->paddingBit = RV_FALSE; /* for security purpose only */
    
	/* Get the number of padding bytes required  */
	/* Note: Perform this after adding CSRCs to header so RvRtpGetHeaderLength */
	/* gives the correct value  */
	if(s->encryptionPlugInPtr != NULL)
	{
        RTPLOG_DEBUG((RTP_SOURCE, "RvRtpWrite: encrypting the RTP packet"));
		RvKeyConstruct(&encryptionKey);
		RvRtpEncryptionDataConstruct(&encryptionData);
		/* Setup the Encryption data */
		RvRtpEncryptionDataSetIsRtp(&encryptionData, RV_TRUE);
		RvRtpEncryptionDataSetIsEncrypting(&encryptionData, RV_TRUE);
		RvRtpEncryptionDataSetMode(&encryptionData, s->encryptionMode);
		RvRtpEncryptionDataSetLocalAddress(&encryptionData, (RvNetAddress*) &rvRtpInstance.rvLocalAddress);
		if(RvRtpEncryptionModeIsPlainTextRtpHeader(s->encryptionMode))
			RvRtpEncryptionDataSetRtpHeader(&encryptionData, p);
		else
			RvRtpEncryptionDataSetRtpHeader(&encryptionData, NULL);
	
		headerSize = RvRtpGetHeaderLength() 
                     + p->extensionBit*sizeof(RvUint32)*(p->extensionLength+1); 
                                             /* IN the FUTURE: RvRtpGetHeaderLength p 
												must be adjusted to number of csrcs */
		
		if(!RvRtpEncryptionModeIsNoPadding(s->encryptionMode))
        {
            RvUint8 blockSize;
            
            blockSize = (RvUint8) RvRtpEncryptionPlugInGetBlockSize(s->encryptionPlugInPtr);
            
            if(RvRtpEncryptionModeIsPlainTextRtpHeader(s->encryptionMode))
                padding = (RvUint8) (blockSize - (dataSize % blockSize));
            else
                padding = (RvUint8) (blockSize - ((dataSize + headerSize) % blockSize));
            
            if(padding == blockSize)
                padding = 0;
            RTPLOG_DEBUG((RTP_SOURCE, "RvRtpWrite: padding = %d, blockSize = %d",	padding, blockSize));
        }
	}
    p->paddingBit = (padding != 0) ? RV_TRUE: RV_FALSE;
    
    res = RvRtpPackEx(hRTP, ssrc, (RvUint8*)buf + RvRtpNatMultiplexIdSize(), len, p);

	/* Encrypt the data */
	if(s->encryptionPlugInPtr != NULL)
	{
		/* Add any required padding */
		if(padding > 0)
		{
			if(UserAllocationSize-len >= padding)
			{
				
				/* Add the padding to the buffer */
				len += (padding - 1);
				
				/* Write the amount of padding as the last byte */
				bufferPtr[len] = padding;
                RTPLOG_DEBUG((RTP_SOURCE,  "RvRtpWrite: setting padding in last byte of packet"));
			}
			else
            {
                p->len = len + padding; /* requred length */
                RvRtpEncryptionDataDestruct(&encryptionData);
                RvKeyDestruct(&encryptionKey);	
                RTPLOG_ERROR_LEAVE(Write, "Not enough room at end of RTP buffer to add required padding.");
                return -1;
            }
		}
		
		/* Make sure the work buffer is large enough for the current packet */
		if(sizeof(workBuffer) < (RvUint32)len)
        {            
            RvRtpEncryptionDataDestruct(&encryptionData);
            RvKeyDestruct(&encryptionKey);
            RTPLOG_ERROR_LEAVE(Write, "Not enough room for working buffer.");            
            return -2;
        }	
		/* Set up the work buffer data position */
/*		rvDataBufferSetDataPosition(&thisPtr->workBuffer,0,rvDataBufferGetLength(bufferPtr));*/
		
		/* Copy the unencrypted header to the work buffer*/
		if(RvRtpEncryptionModeIsPlainTextRtpHeader(s->encryptionMode))
			memcpy(workBuffer+RvRtpNatMultiplexIdSize(), bufferPtr+RvRtpNatMultiplexIdSize(), headerSize);
	}
    if (res >= 0)
    {	
        RtpNatAddress* natAddressPtr = NULL;
		/* Send packet to all remote addresses */
		destAddress = RvRtpAddressListGetNext(&s->addressList, NULL);
		bDestAddressAvailable = (destAddress!=NULL);
		while (destAddress != NULL) 
		{        
            natAddressPtr = (RtpNatAddress*) destAddress;
			if(s->encryptionPlugInPtr != NULL)
			{	
				/* Set the remote address in the encryption data */
				RvRtpEncryptionDataSetRemoteAddress(&encryptionData, (RvNetAddress*)destAddress);
				
				if(s->encryptionKeyPlugInPtr != NULL)
				{
					RvRtpEncryptionKeyPlugInGetKey(s->encryptionKeyPlugInPtr, &encryptionData, &encryptionKey);
				}
				
				/* Encrypt the buffer */
				if(RvRtpEncryptionModeIsPlainTextRtpHeader(s->encryptionMode))
				{
					/* Encrypt the data */
					RvRtpEncryptionPlugInEncrypt(s->encryptionPlugInPtr,
						bufferPtr + headerSize  + RvRtpNatMultiplexIdSize(), /* without header */
						workBuffer + headerSize + RvRtpNatMultiplexIdSize(),
						dataSize + padding,
						&encryptionData, &encryptionKey);
				}
				else
				{
					RvRtpEncryptionPlugInEncrypt(s->encryptionPlugInPtr,
						bufferPtr + RvRtpNatMultiplexIdSize(), /* with header */
						workBuffer + RvRtpNatMultiplexIdSize(),
						dataSize + headerSize + padding,
						&encryptionData, &encryptionKey);
				}
				bufferToSend = (RvUint8*)workBuffer;
			}
			/* send UDP data through the specified socket to the remote host.*/
            res = RtpSendPacket(s->transport, (RvUint8*)bufferToSend, padding, p, natAddressPtr);
			if (res != RV_OK)
            {
              RTPLOG_ERROR((RTP_SOURCE, "RvRtpWrite: packet was NOT sent."));
            }
			destAddress = RvRtpAddressListGetNext(&s->addressList, destAddress);
		}
    }
    if(s->encryptionPlugInPtr != NULL)
    {
        RvRtpEncryptionDataDestruct(&encryptionData);
        RvKeyDestruct(&encryptionKey);
    }
	
    if (!bDestAddressAvailable)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "No destination to send."));
        res = RV_ERROR_UNKNOWN;
    }
	
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpWrite"));
    return res;
}

/************************************************************************************
 * RtpProfileRfc3550Release
 * description: this callback is called, when plugin have to release data related to the
 *              plugin when session is closing.
 * input: plugin           - pointer to this plugin 
 *        hRTP             - Handle of the RTP session.
 * output: none.
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
void  RtpProfileRfc3550Release(
        struct __RtpProfilePlugin* profilePlugin,                                   
        IN    RvRtpSession hRTP)
{
    RtpProfileRfc3550*      rfc3550Plugin = (RtpProfileRfc3550*) profilePlugin->userData;
    
    RtpProfileRfc3550Destruct(rfc3550Plugin);
    RV_UNUSED_ARG(hRTP);
}
/***************************************************************************************
 * RtpProfileRfc3550Construct
 * description:  This method constructs a RtpProfilePlugin. All of
 *               the callbacks must be suppled for this plugin to work.
 * parameters:
 *    plugin - the RvRtpEncryptionPlugIn object.
 *   userData - the user data associated with the object.
 * returns: A pointer to the object, if successful. NULL, otherwise.
 ***************************************************************************************/
RtpProfileRfc3550* RtpProfileRfc3550Construct(
     IN RtpProfileRfc3550* rtpPlugin)
{
    RtpProfilePluginConstruct(
        &rtpPlugin->plugin,
        rtpPlugin,
        &RtpProfileRfc3550Callbacks);
   return rtpPlugin;
}

/***************************************************************************************
 * RtpProfilePluginDestruct
 * description:  This method destructs a RtpProfilePlugin. 
 * parameters:
 *    plugin - the RvRtpEncryptionPlugIn object.
 * returns: none
 ***************************************************************************************/
void RtpProfileRfc3550Destruct(
     IN RtpProfileRfc3550*  plugin)
{
   RtpProfilePluginDestruct(&plugin->plugin);
}



#ifdef __cplusplus
}
#endif
    

