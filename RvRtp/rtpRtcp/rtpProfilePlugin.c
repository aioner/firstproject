/************************************************************************
 File Name	   : rtpProfilePlugin.c
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

#include "rtpProfilePlugin.h"

#ifdef __cplusplus
extern "C" {
#endif

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
     const RtpProfilePluginCallbacks*           callbacks)
{
    plugin->userData = userData;
    plugin->funcs = (RtpProfilePluginCallbacks*)callbacks;
    return plugin;
}

/***************************************************************************************
 * RtpProfilePluginDestruct
 * description:  This method destructs a RtpProfilePlugin. 
 * parameters:
 *    plugin - the RvRtpEncryptionPlugIn object.
 * returns: none
 ***************************************************************************************/
RVAPI
void RVCALLCONV RtpProfilePluginDestruct(
     IN RtpProfilePlugin*  plugin)
{
    plugin->userData               = NULL;
    plugin->funcs                  = NULL;
}

#ifdef __cplusplus
}
#endif

