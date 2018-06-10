#include "rvtypes.h"

#ifndef _file_rvimsipsecpublic_h_
#define _file_rvimsipsecpublic_h_

#if (RV_IMS_IPSEC_ENABLED == RV_YES)

/*
 *	An application may optionally provide an extension data to be used when creating SA/SPs.
 *  This enum lists currently supported extension data types. The values of this enum are used
 *  as values for iExtType field of RvImsAppExtensionData structure (defined below). Each value of
 *  iExtType impacts the way the value of iExtData is considered.
 */
typedef enum
{
   /* Transport mode SP rule may need to be created over exitsing tunnel mode Security Policy.
    * When this is true the tunnel addresses need to be known when transport mode SPs are
    * created.
    * When the iExtType is one of RvImsAppExtLinuxOutboundTunnelData or RvImsAppExtLinuxInboundTunnelData
    * the iExtData must point to the instance of 'struct sadb_x_ipsecrequest' followed by two instances of
    * 'struct sockaddr'. The sadb_x_ipsecrequest must contain data for tunnel mode policy
    * (IPPROTO_ESP/IPSEC_MODE_TUNNEL/...). The sockaddr instances must contain the source and destination
    * addreses of IPSec tunnel.
    * RvImsAppExtLinuxOutboundTunnelData value is used to configure outbound SP while RvImsAppExtLinuxInboundTunnelData
    * is used for inbound SPs.
    * All above is currently supported for ReadHat Linux only. */
    RvImsAppExtLinuxOutboundTunnelData          = 1,
    RvImsAppExtLinuxInboundTunnelData           = 2,

   /* Defines the priority of created SecurityPolicies.
    * The iExtData must point to 32 bits unsigned integer containing the priority value.
    * Priority will be used for OUTBOUND or INBOUND rules depending on whether RvImsAppExtLinuxOutboundPolicyPriority
    * or RvImsAppExtLinuxInboundPolicyPriority is used for iExtType.
    * All above is currently supported for ReadHat Linux only (kernels starting with 2.6.6) */
   RvImsAppExtLinuxOutboundPolicyPriority       = 3,
   RvImsAppExtLinuxInboundPolicyPriority        = 4
} RvImsAppExtensionType;


/*
 *	An application may optionally provide an extension data to be used when creating SA/SP.
 *  This struct contains this extension data.
 */
typedef struct
{
    RvImsAppExtensionType   iExtType;
    void*                   iExtData;
    RvSize_t                iExtLen;
} RvImsAppExtensionData;


#endif /*(RV_IMS_IPSEC_ENABLED == RV_YES)*/

#endif /*_file_rvimsipsecpublic_h_*/
