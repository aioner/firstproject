///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����rv_api.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��16��
// ����������radvsion ARTPЭ��ջ������
//
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef RADVISION_ADAPTER_FUNC_
#define RADVISION_ADAPTER_FUNC_
#ifdef _WIN32
#ifdef RV_ADAPTER_EXPORTS
#define RV_ADAPTER_API extern "C" __declspec(dllexport)
#define RV_ADAPTER_CLASS __declspec(dllexport)
#else
#define RV_ADAPTER_API extern "C" __declspec(dllimport)
#define RV_ADAPTER_CLASS __declspec(dllimport)
#endif
#else
#define RV_ADAPTER_API extern "C" __attribute__((visibility("default"))) 
#define RV_ADAPTER_CLASS __attribute__((visibility("default"))) 
#endif

#include "rv_def.h"
RV_ADAPTER_API rv_bool init_rv_adapter(RV_IN rv_adapter_descriptor *descriptor);
RV_ADAPTER_API void end_rv_adapter(void);

//��ȡ��ǰ�汾��
RV_ADAPTER_API void get_rv_adapter_version(uint32_t *v1, uint32_t *v2, uint32_t *v3);

/////////////////////////////////////////////////
//RTP Session��������
// hrv������û�����洢��Ԫ�������������������ڲ�ֵ
RV_ADAPTER_API rv_bool open_session(RV_IN rv_session_descriptor *descriptor, RV_OUT rv_handler hrv);
RV_ADAPTER_API rv_bool open_session2(RV_IN rv_session_descriptor *descriptor, RV_OUT rv_handler hrv);
RV_ADAPTER_API rv_bool close_session(RV_IN rv_handler hrv);
RV_ADAPTER_API rv_bool close_session2(RV_IN rv_handler hrv);

//rtp���ݷ��ͺ�����
//	���û��̵߳��õ�ֱ��д����ʽ�������ڲ�ֱ�ӵ���ARTPЭ��ջ����
//  radvision�ٷ����Ƽ���ʽ
RV_ADAPTER_API rv_bool write_rtp(
			   RV_IN rv_handler hrv,
			   RV_IN void * buf,
			   RV_IN uint32_t buf_len,
			   RV_INOUT rv_rtp_param * p);

//rtp���ݷ��ͺ�����
//	���û��̵߳��õļ��д����ʽ�������ڲ����첽�¼���ʽ��ӵ���ARTPЭ��ջ����
//  radvision�ٷ��Ƽ���ʽ
RV_ADAPTER_API rv_bool write_rtp_s(RV_IN rv_handler hrv,
				 RV_IN void * buf,
				 RV_IN uint32_t buf_len,
				 RV_INOUT rv_rtp_param * p,
				 RV_IN rv_bool async_mode);

//rtp���ݷ��ͺ�����
//	���û��̵߳��õļ��д����ʽ�������ڲ����첽�¼���ʽ��ӵ���ARTPЭ��ջ����
//  radvision�ٷ��Ƽ���ʽ
//  �˰汾��write_rtp_s�����в���������ָ�뷽ʽ�������ݣ�����ͻ�ϵͳ���ڴ����ģ��
//	���������ϣ���Ҫ����������һ��Чģʽ
//  buf�н��뺯������Զ��������ü������첽������Ϻ��������ü���
RV_ADAPTER_API rv_bool write_rtp_s2(RV_IN rv_handler hrv,
				  RV_IN void * buf,			//tghelper::byte_block
				  RV_INOUT rv_rtp_param * p,
				  RV_IN rv_bool async_mode);

//rtp���ݽ��պ�����
//	����adapter�ڲ��̲߳�����RtpReceiveEventHandler_CB�е���
//  ��ȡ���ݳ��ȱ�����rv_rtp_param��
RV_ADAPTER_API rv_bool read_rtp(
			  RV_IN rv_handler hrv,
			  RV_IN void * buf,
			  RV_IN uint32_t buf_len,
			  RV_INOUT rv_rtp_param * p,
			  RV_INOUT rv_net_address *addr);

//rtp�����ַ���Ӻ��������������鲥��ַ
RV_ADAPTER_API rv_bool add_rtp_remote_address(
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtpAddress);

//rtp�����ַɾ������������ɾ���鲥��ַ
RV_ADAPTER_API rv_bool del_rtp_remote_address(
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtpAddress);

RV_ADAPTER_API rv_bool add_rtp_mult_remote_address(
			/*
				Adds the new RTP address of the remote peer,
				or the address of a multicast group,
				or of the multi-unicast address list to which the RTP stream will be sent.
			*/
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtpAddress,
			RV_IN uint32_t multiplexID);

RV_ADAPTER_API rv_bool del_rtp_mult_remote_address(
	/*
		Removes the specified RTP address of the remote peer,
		or the address of a multicast group,
		or of the multi-unicast address list to which the RTP stream was sent.
	*/
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtpAddress,
	RV_IN uint32_t multiplexID);

//rtp�����ַ��պ���
RV_ADAPTER_API rv_bool clear_rtp_remote_address(
	RV_IN rv_handler hrv);

//rtp����鲥��ַ���ú�����
RV_ADAPTER_API rv_bool set_rtp_group_address(
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtpAddress);

//rtp����鲥TTL���ú�����
RV_ADAPTER_API rv_bool set_rtp_multicast_ttl(
	RV_IN rv_handler hrv,
	RV_IN uint8_t    ttl);

/////////////////////////////////////////////////
//RTCP Session��������
RV_ADAPTER_API rv_bool  set_rtcp_bandwidth(
						RV_IN rv_handler hrv,				/* The handle of the RTCP session */
						RV_IN uint32_t bandwidth);			/* The bandwidth for RTCP packets */

RV_ADAPTER_API rv_bool set_manual_rtcp(
					 RV_IN rv_handler hrv,
					 RV_IN rv_bool manual_rtcp);

RV_ADAPTER_API rv_bool manual_send_rtcp_sr(
						 RV_IN rv_handler hrv,				/* The handle of the RTCP session */
						 RV_IN uint32_t pack_size,			/* The number of bytes in the packet that was sent */
						 RV_IN uint32_t pack_ts);			/* The RTP timestamp from the packet that was sent */

RV_ADAPTER_API rv_bool manual_send_rtcp_rr(
						 RV_IN rv_handler hrv,				/* The handle of the RTCP session */
						 RV_IN uint32_t ssrc,				/* The synchronization source value of the participant that sent the packet */
						 RV_IN uint32_t localtimestamp,		/* The local RTP timestamp when the received packet arrived */
						 RV_IN uint32_t mytimestamp,			/* The RTP timestamp from the packet that was received */
						 RV_IN uint16_t sequence);			/* The sequence number of the packet */

RV_ADAPTER_API rv_bool force_send_rtcp_sr(
						RV_IN rv_handler hrv,				/* The handle of the RTCP session */
						RV_IN rv_bool bCompound,				/* If set to true, sends a compound report (RR + ALL SDES + APP) */
						/* If set to false, sends a receiver report only */
						RV_IN rv_rtcp_appmessage * appMessageTable, /* A pointer to an array holding all APP messages to be sent */
						RV_IN int32_t	appMessageTableEntriesNum);	/* The number of messages in appMessageTable */

RV_ADAPTER_API rv_bool force_send_rtcp_rr(
						RV_IN rv_handler hrv,				/* The handle of the RTCP session */
						RV_IN rv_bool bCompound,				/* If set to true, sends a compound report (RR + ALL SDES + APP) */
						/* If set to false, sends a receiver report only */
						RV_IN rv_rtcp_appmessage * appMessageTable, /* A pointer to an array holding all APP messages to be sent */
						RV_IN int32_t	appMessageTableEntriesNum);	/* The number of messages in appMessageTable */

RV_ADAPTER_API rv_bool set_rtcp_param(
	/*
		For RTP session with dynamic payload (with param = RVRTCP_PARAMS_RTPCLOCKRATE)
		this function should be called after the RTP session
		opening for accurate RTCP timestamp calculation.
		This call can be omitted for standard payload types.
	*/
	RV_IN rv_handler hrv,				/* The handle of the RTCP session */
	RV_IN rv_rtcp_parameters   param,	/* The type of parameter */
	RV_IN void*      data);				/* A pointer to the parameter which is defined by param */

RV_ADAPTER_API rv_bool add_rtcp_remote_address(
	/*
		Adds the new RTCP address of the remote peer,
		or of the multicast group,
		or of the multi-unicast list with elimination of address duplication
	*/
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtcpAddress);

RV_ADAPTER_API rv_bool del_rtcp_remote_address(
	/*
		Removes the specified RTCP address of the remote peer,
		or of the multicast group,
		or of the multi-unicast list with elimination of address duplication
	*/
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtcpAddress);

RV_ADAPTER_API rv_bool add_rtcp_mult_remote_address(
			/*
				Adds the new RTCP address of the remote peer,
				or of the multicast group,
				or of the multi-unicast list with elimination of address duplication
			*/
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtcpAddress,
			RV_IN uint32_t multiplexID);
RV_ADAPTER_API rv_bool del_rtcp_mult_remote_address(
	/*
		Removes the specified RTCP address of the remote peer,
		or of the multicast group,
		or of the multi-unicast list with elimination of address duplication
	*/
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtcpAddress,
	RV_IN uint32_t multiplexID);

RV_ADAPTER_API rv_bool clear_rtcp_remote_address(
	/*
		Removes all RTCP addresses of the remote peer,
		or of the multicast group,
		or of the multi-unicast list with elimination of address duplication
	*/
	RV_IN rv_handler hrv);

RV_ADAPTER_API rv_bool set_rtcp_multicast_ttl(
	/*
		Defines a multicast Time To Live (TTL) for the RTCP session (multicast sending).
	*/
	RV_IN rv_handler hrv,
	RV_IN uint8_t    ttl);

RV_ADAPTER_API rv_bool get_rtcp_sourceinfo(
			/*
				Provides information about a particular synchronization source
			*/
			RV_IN rv_handler hrv,
			RV_IN uint32_t ssrc,
			RV_OUT rv_rtcp_info * info
			);
RV_ADAPTER_API rv_bool get_rtcp_ssrc_remote(
			/*
				Provides information about a particular synchronization source
			*/
			RV_IN rv_handler hrv,  
			RV_IN uint32_t ssrc,
			RV_OUT rv_net_address *addr
			);

/////////////////////////////////////////////////
//�����������
//	��ȡ��ǰrtp��ssrc
RV_ADAPTER_API uint32_t get_rtp_ssrc(RV_IN rv_handler hrv);
//	��������rtp��ssrc�����������ɵĵ�ǰֵ
RV_ADAPTER_API uint32_t regen_rtp_ssrc(RV_IN rv_handler hrv);
//	��ȡ��ǰrtp��ssrc
RV_ADAPTER_API uint32_t get_rtcp_ssrc(RV_IN rv_handler hrv);
//	��������rtp��ssrc�����������ɵĵ�ǰֵ
RV_ADAPTER_API rv_bool set_rtcp_ssrc(RV_IN rv_handler hrv, uint32_t ssrc);
//  ��ȡϵͳĬ��rtp�̶�ͷ�ֽڳ���
RV_ADAPTER_API uint32_t get_rtp_header_lenght();
//  Sets the RTP header before sending an RTP packet
RV_ADAPTER_API rv_bool rv_rtp_pack(
				 RV_IN rv_handler	hrv,
				 RV_IN void*         buf,
				 RV_IN uint32_t      len,
				 RV_IN rv_rtp_param* p);
RV_ADAPTER_API rv_bool rv_rtp_pack_ex(
					RV_IN rv_handler	hrv,
					RV_IN uint32_t      ssrc,		/* ���滻SSRC */
					RV_IN void*         buf,
					RV_IN uint32_t      len,
					RV_IN rv_rtp_param* p);
// Gets the RTP header from a received RTP message buffer
RV_ADAPTER_API rv_bool rv_rtp_unpack(
				   RV_IN rv_handler	hrv,
				   RV_IN void*         buf,
				   RV_IN uint32_t      len,
				   RV_OUT rv_rtp_param *p);

//  ����Э��ջ���仺�����ֽڳ���
RV_ADAPTER_API rv_bool set_rtp_transmit_buffer_size(RV_IN rv_handler hrv,RV_IN uint32_t size);
//  ����Э��ջ���ջ������ֽڳ���
RV_ADAPTER_API rv_bool set_rtp_receive_buffer_size(RV_IN rv_handler hrv,RV_IN uint32_t size);

RV_ADAPTER_API void* demux_construct(RV_IN uint32_t numberOfSessions, RV_IN rv_handler hrv);
RV_ADAPTER_API void demux_deconstruct(RV_IN void* demux);

RV_ADAPTER_API rv_bool open_demux_session(RV_IN rv_session_descriptor *descriptor, RV_IN void* rtpDemux, RV_OUT uint32_t *multiplexID, RV_OUT rv_handler hrv);
RV_ADAPTER_API rv_bool close_demux_session(RV_IN rv_handler hrv);
RV_ADAPTER_API void setdemux_handler(rv_context func);
RV_ADAPTER_API void setdemux_caster_handler(rv_context func);
RV_ADAPTER_API rv_bool read_demux_rtp(
								RV_IN void* demux,
								RV_IN void * buf,
								RV_IN uint32_t buf_len,
								RV_INOUT rv_rtp *rtpH,
								RV_INOUT void **context,
								RV_INOUT rv_rtp_param * p,
								RV_INOUT rv_net_address *address);

// rtcp	 app message
//////////////////////////////////////////////////////////////////////////
RV_ADAPTER_API int RtcpSetAppEventHandler(
	RV_IN rv_handler         hrv,
	RV_IN RtcpAppEventHandler_CB pAppEventHandler);

RV_ADAPTER_API int RtcpSendApps(
								RV_IN rv_handler         hrv,
								RV_IN RtcpAppMessage* appMessageTable,
								RV_IN uint32_t appMessageTableEntriesNum,
								RV_IN bool bCompound);
//////////////////////////////////////////////////////////////////////////

// rtcp raw data
//////////////////////////////////////////////////////////////////////////
RV_ADAPTER_API rv_bool rtcp_send_rawdata(RV_IN rv_handler	hrv,
										 RV_IN void *		buf,
										 RV_IN uint32_t		buf_len);

RV_ADAPTER_API rv_bool  rtcp_set_raw_eventhandler(RV_IN rv_handler	hrv,
												  RV_IN RtcpRawBufferReceived_CB cb);
//////////////////////////////////////////////////////////////////////////

//ȫ�ֺ���
//  rv_net_ipv4 translate to rv_net_address
RV_ADAPTER_API rv_bool convert_ipv4_to_rvnet(RV_OUT rv_net_address * dst, RV_IN rv_net_ipv4 * src);
//  rv_net_ipv6 translate to rv_net_address
RV_ADAPTER_API rv_bool convert_ipv6_to_rvnet(RV_OUT rv_net_address * dst, RV_IN rv_net_ipv6 * src);
//  rv_net_address translate to rv_net_ipv4
RV_ADAPTER_API rv_bool convert_rvnet_to_ipv4(RV_OUT rv_net_ipv4 * dst, RV_IN rv_net_address * src);
//  rv_net_address translate to rv_net_ipv4
RV_ADAPTER_API rv_bool convert_rvnet_to_ipv6(RV_OUT rv_net_ipv6 * dst, RV_IN rv_net_address * src);
//  ipv4_str translate to rv_net_ipv4
RV_ADAPTER_API rv_bool rv_inet_pton4(RV_OUT rv_net_ipv4 * dst, RV_IN const char *src);
//  ��ʼ�� rv_handler/rv_net_ipv4/rv_net_ipv6/rv_net_address/rv_session_descriptor/rv_rtp_param
RV_ADAPTER_API rv_bool construct_rv_handler(RV_INOUT rv_handler hrv);
RV_ADAPTER_API rv_bool construct_rv_net_ipv4(RV_INOUT rv_net_ipv4 *address);
RV_ADAPTER_API rv_bool construct_rv_net_ipv6(RV_INOUT rv_net_ipv6 *address);
RV_ADAPTER_API rv_bool construct_rv_net_address(RV_INOUT rv_net_address *address);
RV_ADAPTER_API rv_bool construct_rv_session_descriptor(RV_INOUT rv_session_descriptor *descriptor);
RV_ADAPTER_API rv_bool construct_rv_rtp_param(RV_INOUT rv_rtp_param *param);



#endif
