/************************************************************************
 File Name	   : RtpProfileRfc3550.h
 Description   : scope: Private
     declaration of inplementation of RtpProfilePlugin for RTP (RFC 3550)
*************************************************************************
************************************************************************
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

#ifndef __RTP_PROFILE_RFC_3550_H__
#define __RTP_PROFILE_RFC_3550_H__

#include "rtpProfilePlugin.h"

#ifdef __cplusplus
extern "C" {
#endif

struct __RtpProfileRfc3550;

/***************************************************************************************
 * RtpProfileRfc3550 :
 * description:
 *   implementation of RtpProfilePlugin for RTP (RFC 3550)
 ****************************************************************************************/
typedef struct __RtpProfileRfc3550
{

    RtpProfilePlugin plugin;

} RtpProfileRfc3550;

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
     RtpProfileRfc3550*                          plugin);

/***************************************************************************************
 * RtpProfilePluginDestruct
 * description:  This method destructs a RtpProfilePlugin. 
 * parameters:
 *    plugin - the RvRtpEncryptionPlugIn object.
 * returns: none
 ***************************************************************************************/
void RtpProfileRfc3550Destruct(
     IN RtpProfileRfc3550*  plugin);

/* Accessors */
#define RtpProfileRfc3550GetPlugIn(t) &(t)->plugin

/************************************************************************************
 * RtpProfileRfc3550Read
 * description: This routine sets the header of the RTP message.
 * input: 
 *        plugin - pointer to this plugin
 *        hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *        isPeekMessage - TRUE, if RvSocketPeekMessage is supported 
 * output: p            - A struct of RTP param,contain the fields of RTP header.
 *        remAddressPtr - pointer to the remote address
 * return value: If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RvInt32  RtpProfileRfc3550Read(
        IN  struct __RtpProfilePlugin* plugin,
		IN  RvRtpSession               hRTP,
		IN  void *                     buf,
		IN  RvInt32                    len,
        IN  RvBool                     isPeekMessage,
		OUT RvRtpParam*                p,
		OUT RvNetAddress*              remAddressPtr);

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
RvInt32  RtpProfileRfc3550Write(
        IN  RvRtpSession	hRTP,
		IN  RvInt32			ssrc,		
        IN  void *			buf,
        IN  RvInt32			len,
        IN  RvRtpParam*		p);

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
        IN struct __RtpProfilePlugin* profilePlugin,                                   
        IN    RvRtpSession hRTP);

#ifdef __cplusplus
}
#endif
    
#endif /* __RTP_PROFILE_RFC_3550_H__ */
