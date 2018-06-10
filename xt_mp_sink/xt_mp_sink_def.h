#ifndef XT_MPSINK_DEF_H
#define XT_MPSINK_DEF_H

#include <stdint.h>
#include <rv_adapter/rv_def.h>
#include <xt_mp_def.h>

#ifdef __cplusplus
extern "C" {
#endif

	#define null    0
	typedef void*	xt_handle;
	typedef int32_t xt_bool;

	typedef enum _report_type
	{
		DROP_IDLE_NMARK_NTS				= 10,
		DROP_IDLE_MARK_TSERROR1			= 11,
		DROP_IDLE_NMARK_TSERROR2		= 12,
		DROP_NREADY_NMARK_NSEQ			= 13,
		DROP_NREADY_MARK_NSEQ			= 14,
		DROP_NREADY_MARK_HEAD_TSERROR	= 15,
		DROP_NREADY_MARK_NHEAD_TSERROR	= 16,
		DROP_NREADY_MARK_NHEAD_TSERROR1	= 17,
		DROP_READY_NMARK_NSEQ			= 18,
		DROP_READY_MARK_NSEQ			= 19,
		DROP_READY_MARK_HEAD_TSERROR	= 20,
		DROP_READY_MARK_NHEAD_TSERROR	= 21,
		DROP_READY_MARK_NHEAD_TSERROR1	= 22
	}report_type;

	typedef enum _trans_mode
	{
		MP_MEMORY_MSINK	= 0x0020,
		MP_RV_RTP_MSINK	= 0x0021,
		MP_BOTH_MSINK		= 0x0022
	}trans_mode;

	typedef struct _mp_address_descritpor
	{
		int8_t		ip_address[32];
		uint16_t	port;
	}mp_address_descriptor;

	typedef mp_address_descriptor mp_address;

	typedef struct _block_params
	{
		uint32_t timestamp;
		uint32_t payload_type;
		uint32_t ssrc;
		uint32_t size;
	} block_params;

	typedef struct _sink_init_descriptor
	{
		uint32_t rv_thread_num;
		uint32_t post_thread_num;
		uint32_t sink_thread_num;
	}sink_init_descriptor;

	typedef void (*ReceiveFrameEvent_CB)(mp_handle hmp,void * context);
	typedef void (*ReceiveRtpEvent_CB)(mp_handle hmp,void * context);
    typedef void (*ReportReceiveEvent_CB)(const rtcp_send_report *sr, void *context);
    typedef void (*ReportSendEvent_CB)(const rtcp_receive_report *rr, void *context);
	typedef void (*ReportEvent_CB)(uint32_t count,uint32_t size,uint32_t type,void * context);

	typedef struct _msink_handle
	{
		mp_handle	h_mp;
		uint16_t	rtp_ssrc;
		uint16_t	rtcp_ssrc;
	}msink_handle,*p_msink_handle;

	typedef struct xt_mp_descriptor_
	{
		xt_mp_descriptor_()
			: manual_rtcp(0)
			, is_direct_output(0)
			, context(null)
			, rtp_multi_cast_opt(0)
			, rtcp_multi_cast_opt(0)
			, rtp_multi_cast_ttl(128)
			, rtcp_multi_cast_ttl(128)
			, mode(MP_MEMORY_MSINK){}
		mp_address	local_address;
		xt_bool		manual_rtcp;
		xt_bool		is_direct_output;
		xt_bool		rtp_multi_cast_opt;
		mp_address	rtp_multi_cast_address;
		uint8_t		rtp_multi_cast_ttl;
		xt_bool		rtcp_multi_cast_opt;
		mp_address	rtcp_multi_cast_address;
		uint8_t		rtcp_multi_cast_ttl;
		void*		context;
		trans_mode	mode;
		ReceiveFrameEvent_CB	onReceiveDataEvent;
		ReceiveRtpEvent_CB		onReceiveRtpEvent;
        ReportReceiveEvent_CB   onReportReceiveEvent;
        ReportSendEvent_CB      onReportSendEvent;
		//ReportEvent_CB			onReportEvent;
	}xt_mp_descriptor;

	typedef struct _XTFrameInfo
	{
		unsigned int verify;
		unsigned int frametype;
		unsigned int datatype;
	}XTFrameInfo;

#ifdef __cplusplus
}
#endif

#endif
  //XT_MPSINK_DEF_H
