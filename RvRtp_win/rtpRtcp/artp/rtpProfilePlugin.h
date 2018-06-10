/************************************************************************
 File Name	   : rtpProfilePlugin.h
 Description   : scope: Private
                 profile interface functions in order to perform the
                 following RTP profiles
                 1) RTP  RFC 3550 
                 2) SRTP RFC 3711
*************************************************************************
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

#ifndef __RTP_PROFILE_PLUGIN_H__
#define __RTP_PROFILE_PLUGIN_H__

#include "rvaddress.h"
#include "rvrtpbuffer.h"
#include "rtp.h"
#include "rvrtpnatfw.h"

#ifdef __cplusplus
extern "C" {
#endif


/* forward declaration */
struct __RtpProfilePlugin;

/************************************************************************************
 * RtpProfilePluginReadXRTPPacketCB
 * description: This callback is called, when plugin have to
 *                 read the XRTP message and set the header of the XRTP message.
 * input: plugin - pointer to this plugin
 *        hRTP  - Handle of the XRTP session.
 *        buf   - Pointer to buffer containing the XRTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *        isPeekMessage - TRUE, if RvSocketPeekMessage is supported
 * output: p              - A struct of RTP param, contains the fields of RTP header.
 *         remAddressPtr  - remote address 
 * return value: If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/

typedef RvInt32  (*RtpProfilePluginReadXRTPPacketCB)(
        struct __RtpProfilePlugin* plugin,
        IN   RvRtpSession          hRTP,
        IN   void *                buf,
        IN   RvInt32               len,
        IN   RvBool                isPeekMessage, 
        OUT  RvRtpParam *          p,
        OUT  RvNetAddress*         remAddressPtr);

/************************************************************************************
 * RtpProfilePluginWriteXRTPPacketCB
 * description: this callback is called, when plugin have to send the XRTP packet.
 * input: plugin - pointer to this plugin
 *        hRTP   - Handle of the RTP session.
 *        buf    - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len    - Length in bytes of buf.
 *        p      - A struct of RTP param.
 * output: none.
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/

typedef RvInt32  (*RtpProfilePluginWriteXRTPPacketCB)(
        IN  RvRtpSession	hRTP,
		IN  RvInt32			ssrc,
        IN  void *          buf,
        IN  RvInt32         len,
        IN  RvRtpParam*     p);

/************************************************************************************
 * RtpProfilePluginXrtcpRawReceiveCB
 * (Private function)
 * description: this callback is called, when plugin have to receive 
 *              the RTCP compound packet filled buffer.
 * input: 
 *        hRCTP            - RTCP session handle.
 *        bytesToReceive   - The maximal length to receive.
 *        isPeekMessage    - RV_TRUE, if RvSocketPeekMessage is supported
 * output:
 *        pHRTP            - pointer to RTCP session handle..
 *                           in case of multiplexing returns pointer on actual session
 *        buf              - Pointer to buffer for receiving a message.
 *        bytesReceivedPtr - pointer to received RTCP size
 *        remoteAddressPtr - Pointer to the remote address 
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
typedef RvStatus  (*RtpProfilePluginXrtcpRawReceiveCB)(
        struct __RtpProfilePlugin* plugin,    
        INOUT RvRtcpSession  hRCTP ,
        INOUT RvRtpBuffer*   buf,
        IN    RvSize_t       bytesToReceive,
        IN    RvBool         isPeekMessage,
        OUT   RvSize_t*      bytesReceivedPtr,
        OUT   RvAddress*     remoteAddressPtr);

/************************************************************************************
 * RtpProfilePluginXrtcpRawSendCB
 * description: this callback is called, when plugin have to send
 *              the XRTCP compound packet filled buffer.
 * input: plugin           - pointer to this plugin 
 *        hRTCP            - Handle of the RTCP session.
 *        bufPtr           - Pointer to buffer containing the RCTP packet.
 *        DataLen          - Length in bytes of RTCP data buf.
 *        DataCapacity     - data capacity of RTCP buffer
 *        sentLenPtr       - for bandwith calculation only
 * output: none.
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
typedef RvStatus  (*RtpProfilePluginXrtcpRawSendCB)(
        struct __RtpProfilePlugin* plugin,                                   
        IN    RvRtcpSession hRTCP,
        IN    RvUint8*      bufPtr,
        IN    RvInt32       DataLen,
        IN    RvInt32       DataCapacity,
        INOUT RvUint32*     sentLenPtr);

/************************************************************************************
 * RtpProfilePluginRemoveRemoteAddressCB
 * description: this callback is called, when plugin have to remove
 *              a destination for the master local
 *              source from the session.
 *              Translators that need to remove destinations for other
 *              sources should use the rvSrtpForwardDestinationRemove function. (Future)
 * input: plugin       - pointer to this plugin 
 *        hRTP         - Handle of the RTP session.
 *        address      - pointer to destination address
 * output: (-)
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
typedef RvStatus  (*RtpProfilePluginRemoveRemoteAddressCB)(
    IN struct __RtpProfilePlugin* plugin,  
    IN RvRtpSession               session,
    IN RvNetAddress*              address);

/************************************************************************************
 * RtpProfilePluginAddRemoteAddressCB
 * description: this callback is called, when plugin have to add destination to the session.
 * input: plugin       - pointer to this plugin 
 *        hRTP         - Handle of the RTP session.
 *        destType     - TRUE for RTP, FALSE for RTCP
 *        address      - pointer to destination address
 * output: (-)
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
typedef RvStatus  (*RtpProfilePluginAddRemoteAddressCB)(
    IN struct __RtpProfilePlugin* plugin,
    IN RvRtpSession               session,
    IN RvBool                     destType, /* TRUE for RTP, FALSE for RTCP */
    IN RvNetAddress*              address);

/************************************************************************************
 * RtpProfilePluginReleaseCB
 * description: this callback is called, when plugin have to release data related to the
 *              plugin when session is closing.
 * input: plugin           - pointer to this plugin 
 *        hRTP             - Handle of the RTP session.
 * output: none.
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
typedef void  (*RtpProfilePluginReleaseCB)(
        struct __RtpProfilePlugin* plugin,                                   
        IN    RvRtpSession hRTP);

/***************************************************************************************
 * RtpProfilePluginCallbacks: 
 * description this type contains list of callbacks, needed to specific profile
 ****************************************************************************************/
typedef struct __RtpProfilePluginCallbacks
{
    RtpProfilePluginReadXRTPPacketCB            readXRTP;
    RtpProfilePluginWriteXRTPPacketCB           writeXRTP;
    RtpProfilePluginXrtcpRawReceiveCB           xrtcpRawReceive;
    RtpProfilePluginXrtcpRawSendCB              xrtcpRawSend;
    RtpProfilePluginRemoveRemoteAddressCB       removeRemAddress;
    RtpProfilePluginAddRemoteAddressCB          addRemAddress;
    RtpProfilePluginReleaseCB                   release; 

} RtpProfilePluginCallbacks;

/***************************************************************************************
 * RtpProfilePlugin :
 * description:
 *   This interface is used to allow to call corresponding to RTP or SRTP
 *   profile functions.
 *   In order to use this interface the user MUST implement all of the
 *    callbacks: from the above type
 ****************************************************************************************/
typedef struct __RtpProfilePlugin
{
    RtpProfilePluginCallbacks*                  funcs;
    void*                                       userData;
} RtpProfilePlugin;

/***************************************************************************************
 * RtpProfilePluginConstruct
 * description:  This method constructs a RtpProfilePlugin. All of
 *               the callbacks must be suppled for this plugin to work.
 * parameters:
 *    plugin - the RvRtpEncryptionPlugIn object.
 *   userData - the user data associated with the object.
 *   callbacks - callbacks related to the plugin
 * returns: A pointer to the object, if successful. NULL, otherwise.
 ***************************************************************************************/
RVAPI
RtpProfilePlugin* RVCALLCONV RtpProfilePluginConstruct(
     RtpProfilePlugin*                          plugin,
     void*								        userData,
     const RtpProfilePluginCallbacks*           callbacks);

/***************************************************************************************
 * RtpProfilePluginDestruct
 * description:  This method destructs a RtpProfilePlugin. 
 * parameters:
 *    plugin - the RvRtpEncryptionPlugIn object.
 * returns: none
 ***************************************************************************************/
RVAPI
void RVCALLCONV RtpProfilePluginDestruct(
     IN RtpProfilePlugin*  plugin);

#ifdef __cplusplus
}
#endif
    
#endif /* __RTP_PROFILE_PLUGIN_H__ */
