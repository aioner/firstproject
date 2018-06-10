#ifndef ____MPSINK_API_H____201203281613_H____
#define ____MPSINK_API_H____201203281613_H____

/*******************************************************************************
 * Created: 2012/03/29
 * File: xt_mp_sink_api.h
 * Author: renfei
 *
 * Copyright (C) xtxk Corporation
 * All rights reserved.
 *
 * Description: ���紫��ͻ��˶�̬��
 *
 ******************************************************************************/

#include<stdint.h>

#include "xt_mp_def.h"
#include "xt_mp_sink_def.h"

#ifdef _WIN32
#ifdef XT_MP_SINK_EXPORTS
#define MPSINK_API __declspec(dllexport)
#else
#define MPSINK_API __declspec(dllimport)
#endif
#else //linux
#define MPSINK_API  __attribute__((visibility("default")))
#endif

typedef int (* FPSendReportOutput)(int nDeviceType, int nLinkRet, int nFrameType, XTSSendReport* pSendReport, void * objUserContext);

#ifdef __cplusplus
extern "C" {
#endif

	//��ʼ��mp_sink��
	MPSINK_API long mp_sink_init(sink_init_descriptor * sink_des);

	//��ֹmp_sink��
	MPSINK_API long mp_sink_end();

	//��һ��mp����ʵ��
	MPSINK_API long	mp_open(xt_mp_descriptor* mp_des,p_msink_handle handle);
	MPSINK_API long mp_open_mult( xt_mp_descriptor* mp_des, p_msink_handle handle, uint32_t *multid);

	//����mp����ʵ��
	MPSINK_API long mp_active(p_msink_handle handle,uint32_t bActive = 1);

	MPSINK_API long mp_directoutput(p_msink_handle handle, bool direct);

	//�رղ���������ʵ��
	MPSINK_API long mp_close(p_msink_handle handle);

	//��ѯ�ͻ��˽��յ�rtcp����
	MPSINK_API long mp_query_rcv_rtcp(p_msink_handle handle,rtcp_receive_report * rtcp);

	//��ѯ�ͻ��˷��͵�rtcp����
	MPSINK_API long mp_query_snd_rtcp(p_msink_handle handle,rtcp_send_report * rtcp);

	//��ȡ���ݣ������ݻص���ʹ��
	MPSINK_API long mp_read_out_data( p_msink_handle handle,uint8_t * pDst,uint32_t size,block_params * param );
	MPSINK_API long mp_read_out_data2( p_msink_handle handle,uint8_t * pDst,uint32_t size,block_params * param , XTFrameInfo &frame);

	//��ȡrtp����
	MPSINK_API long mp_read_out_rtp(p_msink_handle handle,uint8_t *pDst,uint32_t size,rv_rtp_param *param);
	MPSINK_API long mp_pump_out_rtp(p_msink_handle handle,void **p_rtp_block);
	//�����ֶ����һ�η��Ͷ�rtcp����
	MPSINK_API long mp_manual_send_rtcp_sr(p_msink_handle handle,uint32_t pack_size,uint32_t pack_ts);

	//�����ֶ����һ�ν��ն�rtcp����
	MPSINK_API long mp_manual_send_rtcp_rr(p_msink_handle handle,uint32_t ssrc,uint32_t local_ts,uint32_t ts, uint32_t sn);

	//���rtp_sessionԶ�˵�ַ
	MPSINK_API long mp_add_rtp_remote_address( p_msink_handle handle,mp_address_descriptor * address );
	MPSINK_API long mp_add_mult_rtp_remote_address( p_msink_handle handle,mp_address_descriptor * address, uint32_t multid );

	//ɾ��rtp_sessionԶ�˵�ַ
	MPSINK_API long mp_del_rtp_remote_address( p_msink_handle handle,mp_address_descriptor * address );

	//���rtp_sessionԶ�˵�ַ
	MPSINK_API long mp_clear_rtp_remote_address( p_msink_handle handle);

	//���һ��rtcpԶ�˵�ַ
	MPSINK_API long mp_add_rtcp_remote_address(p_msink_handle handle,mp_address_descriptor * address);
	MPSINK_API long mp_add_mult_rtcp_remote_address(p_msink_handle handle,mp_address_descriptor * address, uint32_t multid);

	//ɾ��һ��rtcpԶ�˵�ַ
	MPSINK_API long mp_del_rtcp_remote_address(p_msink_handle handle,mp_address_descriptor * address);

	//�������rtcpԶ�˵�ַ
	MPSINK_API long mp_clear_rtcp_remote_address(p_msink_handle handle,mp_address_descriptor * address);

	//��ȡ�汾��
	MPSINK_API long mp_get_version(uint32_t * v1,uint32_t * v2,uint32_t * v3);

	// rtcp-based feedback message
	MPSINK_API long mp_rtcp_send_dar(p_msink_handle handle, uint32_t rate, uint32_t level);

	MPSINK_API long mp_directoutput_order(bool order);

	// rtcp-based feedback message
	MPSINK_API long mp_rtcp_send_fir(p_msink_handle handle);

	//��ѯSR����
	MPSINK_API long mp_get_xtsr(p_msink_handle handle, int &nDeviceType, int &nLinkRet, int &nFrameType,XTSSendReport *rtcp);
	//���ö����ش�����
	MPSINK_API int mp_set_resend(int resend,int wait_resend, int max_resend, int vga_order);
	MPSINK_API int mp_RegistSendReportEvent(p_msink_handle handle, FPSendReportOutput fpSendReportOutput, void *objUserContext);

	MPSINK_API void mp_setdemux_handler(rv_context func);
	MPSINK_API bool mp_read_demux_rtp(
		RV_IN void* demux,
		RV_IN void * buf,
		RV_IN uint32_t buf_len,
		RV_INOUT rv_rtp *rtpH,
		RV_INOUT void **context,
		RV_INOUT rv_rtp_param * p,
		RV_INOUT rv_net_address *address);
	MPSINK_API bool mp_pump_demux_rtp(
		RV_IN p_msink_handle handle,
		RV_IN void* demux,
		RV_IN void * buf,
		RV_IN uint32_t buf_len,
		RV_INOUT rv_rtp *rtpH,
		RV_INOUT void **context,
		RV_INOUT rv_rtp_param * p,
		RV_INOUT rv_net_address *address);

	MPSINK_API bool mp_get_multinfo(p_msink_handle handle, bool *multiplex, uint32_t *multid);

	MPSINK_API rv_rtp mp_query_rtp_handle(p_msink_handle h);

#ifdef __cplusplus
}
#endif

#ifdef WIN32
#ifndef XT_MP_SINK_EXPORTS
#ifndef NDEBUG
#pragma comment(lib,"xt_mp_sink_d.lib")
#else
#pragma comment(lib,"xt_mp_sink.lib")
#endif
#endif
#endif

#endif
