/***********************************************************************
        Copyright (c) 2002 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..

RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/

#ifndef __RV_RTP_INTERFACE_H
#define __RV_RTP_INTERFACE_H

#include "rvtypes.h"
#include "rvrtpconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	RVRTP_OLD_CONVENTION_API

/* rtp.h */
#define RvRtpSession			HRTPSESSION
#define RvRtcpSession			HRTCPSESSION
#define RvRtpParam				rtpParam

#define RvRtpSetRTCPSession     rtpSetRTCPSession
#define RvRtpSessionInfo		rtpSession
#define RvRtpEventHandler_CB	LPRTPEVENTHANDLER
#define RvRtpInit				rtpInit
#define RvRtpEnd				rtpEnd
#define RvRtpGetAllocationSize  rtpGetAllocationSize
#define RvRtpClose				rtpClose
#define RvRtpGetSSRC			rtpGetSSRC
#define RvRtpSetEventHandler	rtpSetEventHandler
#define RvRtpWrite				rtpWrite
#define RvRtpOLDPack		    rtpPack
#define RvRtpPack		        rtpPack

#define RvRtpUnpack				rtpUnpack
#define RvRtpRead				rtpRead
#define RvRtpReadEx				rtpReadEx
#define RvRtpGetPort			rtpGetPort
#define RvRtpGetVersion			rtpGetVersion
#define RvRtpGetVersionNum		rtpGetVersionNum
#define RvRtpGetRTCPSession		rtpGetRTCPSession
#define RvRtpGetHeaderLength	rtpGetHeaderLength
#define RvRtpRegenSSRC			rtpRegenSSRC
#define RvRtpResume				rtpResume
#define RvRtpUseSequenceNumber	rtpUseSequenceNumber
#define RvRtpSetReceiveBufferSize rtpSetReceiveBufferSize
#define RvRtpSetTransmitBufferSize rtpSetTransmitBufferSize
#define RvRtpSetTrasmitBufferSize rtpSetTrasmitBufferSize
#define RvRtpGetAvailableBytes rtpGetAvailableBytes

/* rtcp.h */
#define RvRtcpEventHandler_CB			LPRTCPEVENTHANDLER
#define RvRtcpSSRCENUM_CB				LPSSRCENUM
#define RvRtcpInit						rtcpInit
#define RvRtcpEnd						rtcpEnd
#define RvRtcpSetRTCPRecvEventHandler	rtcpSetRTCPRecvEventHandler
#define RvRtcpGetAllocationSize			rtcpGetAllocationSize
#define RvRtcpClose						rtcpClose
#define RvRtcpStop						rtcpStop
#define RvRtcpRTPPacketRecv				rtcpRTPPacketRecv
#define RvRtcpRTPPacketSent				rtcpRTPPacketSent
#define RvRtcpGetPort					rtcpGetPort
#define RvRtcpCheckSSRCCollision		rtcpCheckSSRCCollision
#define RvRtcpEnumParticipants			rtcpEnumParticipants
#define RvRtcpGetSourceInfo				rtcpGetSourceInfo
#define RvRtcpGetSSRC                   rtcpGetSSRC
#define RvRtcpSetSSRC					rtcpSetSSRC
#define RvRtcpGetEnumNext				rtcpGetEnumNext
#define RvRtcpGetEnumFirst				rtcpGetEnumFirst
#define RvRtcpINFO                      RTCPINFO
#define RvRtcpRRINFO                    RTCPRRINFO

/* payload.h */
#define RV_RTP_PAYLOAD_PCMU			PCMU
#define RV_RTP_PAYLOAD_G7231		G7231
#define RV_RTP_PAYLOAD_PCMA			PCMA
#define RV_RTP_PAYLOAD_G722			G722
#define RV_RTP_PAYLOAD_G728			G728
#define RV_RTP_PAYLOAD_G729			G729
#define RV_RTP_PAYLOAD_H261			H261
#define RV_RTP_PAYLOAD_H263			H263
#define RvRtpPayloadType			payload_e
	
#define RvRtpPCMUPack				rtpPCMUPack
#define RvRtpPCMUUnpack				rtpPCMUUnpack
#define RvRtpPCMUGetHeaderLength	rtpPCMUGetHeaderLength

#define RvRtpPCMAPack				rtpPCMAPack
#define RvRtpPCMAUnpack				rtpPCMAUnpack
#define RvRtpPCMAGetHeaderLength	rtpPCMAGetHeaderLength

#define RvRtpG722Pack				rtpG722Pack
#define RvRtpG722Unpack				rtpG722Unpack
#define RvRtpG722GetHeaderLength	rtpG722GetHeaderLength

#define RvRtpG728Pack				rtpG728Pack
#define RvRtpG728Unpack				rtpG728Unpack
#define RvRtpG728GetHeaderLength	rtpG728GetHeaderLength

#define RvRtpG729Pack				rtpG729Pack
#define RvRtpG729Unpack				rtpG729Unpack
#define RvRtpG729GetHeaderLength	rtpG729GetHeaderLength
	
#define RvRtpG7231Pack				rtpG7231Pack
#define RvRtpG7231Unpack			rtpG7231Unpack
#define RvRtpG7231GetHeaderLength	rtpG7231GetHeaderLength
	
#define RvRtpPayloadH261			H261param
#define RvRtpH261Pack				rtpH261Pack
#define RvRtpH261Unpack				rtpH261Unpack
#define RvRtpH261GetHeaderLength    rtpH261GetHeaderLength

#define RvRtpPayloadH263			H263param
#define RvRtpH263Pack				rtpH263Pack
#define RvRtpH263Unpack				rtpH263Unpack
#define RvRtpH263GetHeaderLength    rtpH263GetHeaderLength

#define RvRtpPayloadH263a			H263aparam
#define RvRtpH263aPack				rtpH263aPack
#define RvRtpH263aUnpack			rtpH263aUnpack
#define RvRtpH263aGetHeaderLength   rtpH263aGetHeaderLength

#define RVRTP_DTMF_EVENT_0			rtpDtmfEvent_0
#define RVRTP_DTMF_EVENT_1			rtpDtmfEvent_1
#define RVRTP_DTMF_EVENT_2			rtpDtmfEvent_2
#define RVRTP_DTMF_EVENT_3			rtpDtmfEvent_3
#define RVRTP_DTMF_EVENT_4			rtpDtmfEvent_4
#define RVRTP_DTMF_EVENT_5			rtpDtmfEvent_5
#define RVRTP_DTMF_EVENT_6			rtpDtmfEvent_6
#define RVRTP_DTMF_EVENT_7			rtpDtmfEvent_7
#define RVRTP_DTMF_EVENT_8			rtpDtmfEvent_8
#define RVRTP_DTMF_EVENT_9			rtpDtmfEvent_9
#define RVRTP_DTMF_EVENT_ASTERISK	rtpDtmfEvent_asterisk
#define RVRTP_DTMF_EVENT_POUND		rtpDtmfEvent_pound
#define RVRTP_DTMF_EVENT_A			rtpDtmfEvent_A
#define RVRTP_DTMF_EVENT_B			rtpDtmfEvent_B
#define RVRTP_DTMF_EVENT_C			rtpDtmfEvent_C
#define RVRTP_DTMF_EVENT_D			rtpDtmfEvent_D
#define RVRTP_DTMF_EVENT_NOEVENT	rtpDtmfEvent_NoEvent
#define RvRtpDtmfEvent				rtpDtmfEvent

#define RvRtpDtmfEventParams		rtpDtmfEventParams
#define RvRtpDtmfTonesParams		rtpDtmfTonesParams

#define RvRtpDtmfEventPack			rtpDtmfEventPack
#define RvRtpDtmfEventUnpack		rtpDtmfEventUnpack
#define RvRtpDtmfEventGetHeaderLength rtpDtmfEventGetHeaderLength

#define RvRtpDtmfTonesPack			rtpDtmfTonesPack
#define RvRtpDtmfTonesUnpack		rtpDtmfTonesUnpack

#define RvRtpDtmfTonesGetByIndex	rtpDtmfTonesGetByIndex


#define RVRTP_ANNEXQ_PROCEDURES_START_ACTION			rtpAnnexQProceduresStartAction
#define RVRTP_ANNEXQ_PROCEDURES_CONTINUE_ACTION			rtpAnnexQProceduresContinueAction
#define RVRTP_ANNEXQ_PROCEDURES_STOP_ACTION				rtpAnnexQProceduresStopAction
#define RVRTP_ANNEXQ_PROCEDURES_SELECT_VIDEO_SOURCE		rtpAnnexQProceduresSelectVideoSource
#define RVRTP_ANNEXQ_PROCEDURES_VIDEO_SOURCE_SWITCHED	rtpAnnexQProceduresVideoSourceSwitched
#define RVRTP_ANNEXQ_PROCEDURES_STORE_AS_PRESET			rtpAnnexQProceduresStoreAsPreset
#define RVRTP_ANNEXQ_PROCEDURES_ACTIVATE_PRESET			rtpAnnexQProceduresActivatePreset

#define RVRTP_ANNEXQ_MOVE_DISABLE				rtpAnnexQMoveDisable
#define RVRTP_ANNEXQ_MOVE_LEFT_DOWN_OUT			rtpAnnexQMoveLeftDownOut
#define RVRTP_ANNEXQ_MOVE_RIGHT_UP_IN			rtpAnnexQMoveRightUpIn
#define RvRtpAnnexQProcedures					rtpAnnexQProcedures

#define	RvRtpAnnexQMoveCamera					rtpAnnexQMoveCamera

#define RvRtpAnnexQActions						rtpAnnexQActions

#define RvRtpAnnexQParam						rtpAnnexQParam
#define RvRtpAnnexQMessagePack				rtpAnnexQMessagePack
#define RvRtpAnnexQMessageUnpack			rtpAnnexQMessageUnpack



#endif /* RVRTP_OLD_CONVENTION_API */

#ifdef __cplusplus
}
#endif

#endif /* __RV_RTP_INTERFACE_H */


