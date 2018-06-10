///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_def.h
// 创 建 者：汤戈
// 创建时间：2012年03月16日
// 内容描述：radvsion ARTP协议栈适配器
// radvision协议栈内部数据结构的转换定义
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef RADVISION_ADAPTER_DEFINE_
#define RADVISION_ADAPTER_DEFINE_

#include<stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//parameter attribute
#define RV_IN
#define RV_INOUT
#define RV_OUT
#define RV_CALLCONV
#define RV_ADAPTER_TRUE		1
#define RV_ADAPTER_FALSE	0
#define RV_NULL				0

typedef const void * rv_h;
typedef void * rv_context;
typedef rv_h rv_rtp;
typedef rv_h rv_rtcp;
typedef int32_t rv_bool;

typedef struct rv_handler_s_
{
	RV_INOUT rv_rtp	hrtp;
	RV_INOUT rv_rtcp hrtcp;
	RV_INOUT uint32_t hthread;
	//保存回调函数关系字段
	RV_INOUT rv_context context;
	RV_IN void *user_context;
	RV_IN rv_context	onRtpRcvEvent;
	RV_IN rv_context	onRtcpSndSREvent;
	RV_IN rv_context	onRtcpRcvSRRREvent;
	RV_IN rv_context	onRtcpAppEvent;
	RV_IN rv_context	onRtcpRawEvent;

	rv_handler_s_()
	{ 
		hrtp = 0;
		hrtcp = 0;
		hthread = 0;
		context = 0;
		user_context = 0;
		onRtpRcvEvent = 0;
		onRtcpSndSREvent = 0;
		onRtcpRcvSRRREvent = 0;
		onRtcpAppEvent = 0;
		onRtcpRawEvent = 0;
	}
} rv_handler_s;
typedef rv_handler_s * rv_handler;

typedef struct rv_compoundAppMessage_
{
	uint8_t  subtype;		/* 5 least significant bits are used */
	uint8_t  name[4];		/* 4 octet name */
	uint8_t* userData;		/* multiple by 4 octet user data */
	uint32_t userDataLength;
} RtcpAppMessage;

typedef struct rv_adapter_descriptor_
{
	RV_IN uint32_t thread_nums;
} rv_adapter_descriptor;


//radvision协议栈内部数据结构的转换定义
typedef struct rv_net_ipv4_
{
	RV_INOUT uint32_t ip;		/* 4 byte IP address, network format */
	RV_INOUT uint16_t port;		/* 2 byte port number, host format   */
} rv_net_ipv4;

typedef struct rv_net_ipv6_
{
	RV_INOUT uint8_t ip[16];     /* 16 byte IP address, network format */
	RV_INOUT uint16_t port;      /* 2 byte port number, host format    */
	RV_INOUT uint32_t scopeId;   /* 4 bytes of interface for a scope   */
} rv_net_ipv6;

typedef struct rv_net_address_
{
	RV_INOUT uint8_t address[32];
} rv_net_address;

typedef struct rv_rtp_param_
{
	RV_INOUT  uint32_t    timestamp;         /* RTP timestamp */
	RV_INOUT  rv_bool	  marker;            /* RTP marker bit */
	RV_INOUT  uint8_t     payload;           /* RTP payload type */

	RV_OUT     uint32_t    sSrc;              /* SSRC of received RTP packet */
	RV_OUT     uint16_t    sequenceNumber;    /* sequence number of received rtp packet */
	RV_OUT     uint16_t    extSequenceNumber; /* sequence number of received rtp packet extended by number of cycles. see RFC 3550 A.1*/
	RV_OUT     int32_t     sByte;             /* offset that points on actual beginning of the RTP payload */
	RV_INOUT   int32_t     len;               /* length of receiving payload */
											  /* when sending packet with encryption used
											  len is the size of buffer for sending.
											  Encryption may add additional padding.
											  SRTP may add authentication and MKI information to a packet.*/
	/* The following fields are not present in OLD API and should not be filled if RVRTP_OLD_CONVENTION_API is defined */
	RV_INOUT   rv_bool     extensionBit;      /* TRUE, if extension is present */
	RV_INOUT   uint16_t    extensionProfile;  /* application-depended profile */
	RV_INOUT   uint16_t    extensionLength;   /* the length in 4-byte integers of the extensionData field, 0 is valid */
	RV_INOUT   uint32_t*   extensionData;     /* pointer to the extension data */

	RV_IN      rv_bool      paddingBit;        /* for internal usage only */
} rv_rtp_param;


typedef struct rv_rtcp_srinfo_
{
    rv_bool     valid;			/* RV_ADAPTER_TRUE if this struct contains valid information. */
    uint32_t    mNTPtimestamp;	/* Most significant 32bit of NTP timestamp */
    uint32_t    lNTPtimestamp;	/* Least significant 32bit of NTP timestamp */
    uint32_t    timestamp;		/* RTP timestamp */
    uint32_t    packets;		/* Total number of RTP data packets transmitted by the sender
								   since transmission started and up until the time this SR packet
								   was generated. */
    uint32_t    octets; /* The total number of payload octets (not including header or padding */
} rv_rtcp_srinfo;

typedef struct rv_rtcp_rrinfo_
{
    rv_bool     valid;			/* RV_ADAPTER_TRUE if this struct contains valid information. */
    uint32_t    fractionLost;	/* The fraction of RTP data packets from source specified by
                                   SSRC that were lost since previous SR/RR packet was sent. */
    uint32_t    cumulativeLost; /* Total number of RTP data packets from source specified by
                                   SSRC that have been lost since the beginning of reception. */
    uint32_t    sequenceNumber; /* Sequence number that was received from the source specified
                                   by SSRC. */
    uint32_t    jitter;			/* Estimate of the statistical variance of the RTP data packet inter
								   arrival time. */
    uint32_t    lSR;			/* The middle 32 bits of the NTP timestamp received. */
    uint32_t    dlSR;			/* Delay since the last SR. */
} rv_rtcp_rrinfo;

typedef struct rv_rtcp_info_
{
    rv_bool      selfNode;		/* RV_ADAPTER_TRUE if this structure contains information about a source
								   created by this session. In this case, only sr and cname fields
								   are relevant in the structure. */
    rv_rtcp_srinfo  sr;			/* Sender report information. For selfNode==RV_TRUE, this field contains
								   the report that will be sent to other sources. Otherwise, this is the
								   last report received from remote sources. */
    rv_rtcp_rrinfo  rrFrom;		/* Receiver report information. The last, if any, receiver report
								   received from the source identified by the ssrc parameter about the
								   source that created this session. */
    rv_rtcp_rrinfo  rrTo;		/* Local receiver report information. Information about the source
								   identifier by the ssrc parameter about the source that created this
								   session. */
    char        cname[255];		/* cname of the source that is identified by the ssrc parameter */

    uint32_t rtt;
} rv_rtcp_info;

/* structure for one APP RTCP message */
typedef struct rv_rtcp_appmessage_
{
	uint8_t  subtype;			/* 5 least significant bits are used */
	uint8_t  name[4];			/* 4 octet name */
	uint8_t* userData;			/* multiple by 4 octet user data */
	uint32_t userDataLength;
} rv_rtcp_appmessage;

typedef enum rv_rtcp_parameters_
{
	RVRTCP_RTPCLOCKRATE = 0,	/* The enumeration of the clock rate of the sampling. */
								/* Typically for audio = 8000 Hz or 16000 Hz. */
								/* Typically for video = 9000 Hz. */
	RVRTCP_RTP_PAYLOADTYPE,		/*
									The enumeration of the dynamic payload type,
									an integer from 0-127,defined as as follows:

									0-34—standard points
									96-127—dynamic points
									35-71—unassigned
									77-95—unassigned
									1-2, 72-76—reserved
								*/
} rv_rtcp_parameters;

typedef void (*rtpDemuxEventHandler)(void*   hDemux,void*  context);

typedef void (*RtpReceiveEventHandler_CB)(
	RV_IN  rv_handler  hrv,
	RV_IN  rv_context context);

typedef void (RV_CALLCONV *RtcpSendHandler_CB)(
	RV_IN rv_handler hrv,
	RV_IN rv_context context,
	RV_IN uint32_t ssrc,
	RV_IN uint8_t *rtcpPack,
	RV_IN uint32_t size);

typedef void (RV_CALLCONV *RtcpReceivedHandler_CB)(
	RV_IN rv_handler hrv,
	RV_IN rv_context       context,
	RV_IN uint32_t         ssrc,
	RV_IN uint8_t          *rtcpPack,
	RV_IN uint32_t         size,
	RV_IN uint8_t          *ip,
	RV_IN uint16_t         port,
	RV_IN rv_bool          multiplex,
	RV_IN uint32_t         multid);

typedef rv_bool (RV_CALLCONV *RtcpAppEventHandler_CB)(
	RV_IN  rv_handler	 hrv,
	RV_IN  rv_context	 context,
	RV_IN  uint8_t        subtype,
	RV_IN  uint32_t       ssrc,
	RV_IN  uint8_t*       name,
	RV_IN  uint8_t*       userData,
	RV_IN  uint32_t       userDataLen); /* userData in bites!, not in 4 octet words */

typedef rv_bool (RV_CALLCONV *RtcpRawBufferReceived_CB)(
	RV_IN  rv_handler	 hrv,
	RV_IN  rv_context	 context,
	RV_IN uint8_t *buffer,
	RV_IN uint32_t buffLen,
	RV_OUT rv_net_address *remoteAddress,
	RV_OUT rv_bool *pbDiscardBuffer);

typedef struct rv_session_descriptor_
{
	RV_IN rv_net_address local_address;			//单播监听地址
	RV_IN rv_bool manual_rtcp;
	RV_IN rv_bool multicast_rtp_address_opt;
	RV_IN rv_net_address multicast_rtp_address;		//rtp组播监听地址
	RV_IN rv_bool multicast_rtcp_address_opt;
	RV_IN rv_net_address multicast_rtcp_address;	//rtcp组播监听地址
	//组播输出TTL设置
	RV_IN uint8_t multicast_rtp_ttl;				//rtp组播输出TTL
	RV_IN uint8_t multicast_rtcp_ttl;				//rtcp组播输出TTL
	RV_IN RtpReceiveEventHandler_CB		onRtpRcvEvent;
	RV_IN RtcpSendHandler_CB			onRtcpSndSREvent;
	RV_IN RtcpReceivedHandler_CB		onRtcpRcvSRRREvent;
	RV_IN rv_context context;
	RV_IN void *user_context;
} rv_session_descriptor;

#ifdef __cplusplus
}
#endif

#endif
