#ifndef __RTCP_PLUGIN_H__
#define __RTCP_PLUGIN_H__



#include "rvrtpbuffer.h"

#ifndef __RTCP_H
RV_DECLARE_HANDLE(RvRtcpSession);
#endif



struct __rtcpSession;


typedef RvBool (*RtcpBuffAddToBuffer)(
									  IN RvRtpBuffer*  to, 
									  IN RvRtpBuffer*  from, 
									  IN RvUint32      offset);


typedef struct __rtcpInfo* (*RtcpFindSSrc)(
									  IN struct __rtcpSession *s, 
									  IN RvUint32     ssrc);


typedef struct __rtcpHeader (*RtcpMakeHeader) (
									  IN RvUint32  ssrc, 
									  IN RvUint8   count, 
									  IN RtcpType  type, 
									  IN RvUint16  dataLen);

typedef RvRtpBuffer (*RtcpBuffCreate)(
									  IN void*   data,
									  IN RvInt32 size);

typedef RvBool (*RtcpDataAddToBuffer) ( 
									  IN		void *			data, 
									  IN		RvUint32		size, 
									  IN OUT	RvRtpBuffer *	buf, 
									  IN OUT	RvUint32 *		allocated);
							  
typedef RvStatus (*RtcpAddRTCPPacketType)(
									   IN RvRtcpSession    hRTCP,
									   IN RtcpType         type,
									   IN RvUint8          subtype,
									   IN RvUint8*         name,
									   IN void*            userData,
									   IN RvUint32         userDataLength,
									   IN OUT RvRtpBuffer* buf,
									   IN OUT RvUint32*    pCurrentIndex);


typedef RvStatus (*RtcpSessionRawSend)(
									   IN    RvRtcpSession hRTCP,
									   IN    RvUint8*      bufPtr,
									   IN    RvInt32       DataLen,
									   IN    RvInt32       DataCapacity,
									   INOUT RvUint32*     sentLenPtr);



typedef RvUint64 (*RtcpGetNNTPTime)(void);


typedef RvUint32 (*RvBitfieldSet)(
					 IN  RvUint32    value,
					 IN  RvUint32    bitfield,
					 IN  RvInt32     nStartBit,
					 IN  RvInt32     nBits);
					 
typedef RvUint32 (*RvBitfieldGet)(
					 IN  RvUint32	value,
					 IN  RvInt32	nStartBit,
					 IN  RvInt32	nBits);
							 

typedef struct _RtcpMethods
{
	RtcpFindSSrc			findSSrc;	
	RtcpMakeHeader			makeHeader;
	RtcpBuffCreate			buffCreate;
	RtcpBuffAddToBuffer		buffAddToBuffer;
	RtcpDataAddToBuffer     dataAddToBuffer;
	RtcpAddRTCPPacketType	addRTCPPacketType;
	RtcpSessionRawSend		sessionRawSend;
	RtcpGetNNTPTime			getNNTPTime;
	RvTimerFunc				timerCallback;
	RvBitfieldSet			bitfieldSet;
	RvBitfieldGet			bitfieldGet;
} RtcpMethods;


#endif  /* __RTCP_PLUGIN_H__ */
