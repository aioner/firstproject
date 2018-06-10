///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����xt_mp_caster_api.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��23��
// ������������ͼ�¿ƹ�˾mp�㲥�����
//
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef XT_MP_CASTER_FUNC_
#define XT_MP_CASTER_FUNC_

#ifdef _OS_WINDOWS
#ifdef XT_MP_CASTER_EXPORTS
#define XT_MP_CASTER_API extern "C" __declspec(dllexport)
#define XT_MP_CASTER_CLASS __declspec(dllexport)
#else
#define XT_MP_CASTER_API extern "C" __declspec(dllimport)
#define XT_MP_CASTER_CLASS __declspec(dllimport)
#endif
#else
#define XT_MP_CASTER_API extern "C" __attribute__((visibility("default")))
#define XT_MP_CASTER_CLASS __attribute__((visibility("default")))
#endif



#include "xt_mp_caster_def.h"

typedef void (*pSink_RtcpCB)(uint32_t ssrc, rv_rtcp_info &send, rv_rtcp_info &recieve,
							 uint8_t *ip, uint16_t port, rv_bool multiplex, uint32_t multid);
typedef rv_bool (*pRtcpAppMsgCB)(
                                 uint32_t	   serverid,
                                 uint8_t        subtype,
                                 uint32_t       ssrc,
                                 uint8_t*       name,
                                 uint8_t*       userData,
                                 uint32_t       userDataLen);
typedef rv_bool (*pRtcpRawDataCB)(
                                  mp_handle sink,
                                  uint8_t *buffer,
                                  uint32_t buffLen,
                                  rv_net_address *remoteAddress,
                                  rv_bool *pbDiscardBuffer);

//���ʼ��
XT_MP_CASTER_API mp_bool init_mp_caster(MP_IN caster_descriptor *descriptor);
//������
XT_MP_CASTER_API void end_mp_caster(void);

//��ȡ��ǰ�汾��
XT_MP_CASTER_API void get_mp_caster_version(uint32_t *v1, uint32_t *v2, uint32_t *v3);


//����һ���㲥MP
XT_MP_CASTER_API mp_bool open_bc_mp(
                                    MP_IN bc_mp_descriptor* descriptor,
                                    MP_OUT mp_h hmp,						//mpʵ����
                                    MP_OUT mssrc_h hmssrc,					//����pump_frame_in/pump_rtp_in
                                    MP_OUT msink_h hmsink,					//����read_rtcp_sr/read_rtcp_rr�����ɵ���del_sink�ͷ�
                                    MP_OUT uint32_t *multid);
//���¶����ش����� add by songlei 20150708
XT_MP_CASTER_API void update_resend_flag(MP_IN mp_h hmp,const int resend_flag);

//����һ������MP
XT_MP_CASTER_API mp_bool open_proxy_mp(
                                       MP_IN proxy_mp_descriptor* descriptor,
                                       MP_OUT mp_h hmp,						//mpʵ����
                                       MP_OUT mssrc_h hmssrc,					//����pump_frame_in/pump_rtp_in
                                       MP_OUT msink_h hmsink);					//����read_rtcp_sr/read_rtcp_rr�����ɵ���del_sink�ͷ�
//����ָ��MP
XT_MP_CASTER_API mp_bool close_mp(MP_IN mp_h hmp);
//��������ָ��MP�����õ�MP���������ⲿ�������룬Ҳ���������
XT_MP_CASTER_API mp_bool active_mp(MP_IN mp_h hmp, mp_bool bActive);

//��ָ��MP����rtp����ӵ㣬rtp_sink_descriptor�еĵ�ַ����ʹ��rv_adapter�е���غ���
XT_MP_CASTER_API mp_bool add_rtp_sink(
                                      MP_IN mp_h hmp,
                                      MP_IN rtp_sink_descriptor* descriptor,
                                      MP_OUT msink_h hsink);
//��ָ��MP�����ڴ�ص������
XT_MP_CASTER_API mp_bool add_mem_sink(
                                      MP_IN mp_h hmp,
                                      MP_IN memory_sink_descriptor* descriptor,
                                      MP_IN mp_bool bActive,
                                      MP_OUT msink_h hsink);
//��ָ��MPɾ��ָ��sink�����
XT_MP_CASTER_API mp_bool del_sink(MP_IN mp_h hmp, MP_IN msink_h hsink);
//��ȡbc_mp��rtcp���ͱ��棬����hsink������open_bc_mp���
XT_MP_CASTER_API mp_bool read_rtcp_sr_from_sender(
    MP_IN msink_h hsink,
    MP_OUT rtcp_send_report *sr);
//��ȡbc_mp��rtcp���ձ��棬����hsink������open_bc_mp���
XT_MP_CASTER_API mp_bool read_rtcp_rr_from_sender(
    MP_IN msink_h hsink,
    MP_OUT rtcp_receive_report *rr);
//��ȡrv_ssrc��rtcp���ͱ��棬����hssrc������open_xxx_mp���
XT_MP_CASTER_API mp_bool read_rtcp_sr_from_receiver(
    MP_IN mssrc_h hssrc,
    MP_OUT rtcp_send_report *sr);
//��ȡrv_ssrc��rtcp���ձ��棬����hsink������open_xxx_mp���
XT_MP_CASTER_API mp_bool read_rtcp_rr_from_receiver(
    MP_IN mssrc_h hssrc,
    MP_OUT rtcp_receive_report *rr);
//��ָ��MP��MSSRC�˿�д������֡
/*
pump_frame_in����
����û������Զ���frameTS_opt, frameTS, framePayloadҪ����rtpЭ��Ĺ淶��д
���磺��ƵPCMU framePayload = 0, frameTS_opt = MP_TRUE, ��frameTS��8000Hz�����ʱ�1ms = 8 unit
���磺��ƵH263 framePayload = 34,frameTS_opt = MP_TRUE, ��frameTS��90000Hz�����ʱ�1ms = 90 unit
���ڲ���Ĭ�϶���ο�mp_caster_config.h
*/
XT_MP_CASTER_API mp_bool pump_frame_in(
                                        MP_IN mp_h	hmp,				//Ŀ��mp���
                                        MP_IN mssrc_h hmssrc,			//Ŀ��mssrc���
                                        MP_IN uint8_t *frame,			//����֡������
                                        MP_IN uint32_t framesize,		//����֡����
                                        MP_IN mp_bool  frameTS_opt,		//����֡ʱ����ⲿ������Ч�ԣ�falseΪ�ڲ�����ʱ���(mp_caster_config.h)
                                        MP_IN uint32_t frameTS,			//����֡ʱ��� == 0��ʾ�ڲ�����ʱ���
                                        MP_IN uint8_t  framePayload,	//����֡αװý������ 0xFF��ʾ��ȱʡ����(mp_caster_config.h)
                                        MP_IN XTFrameInfo &info,        //12�ֽ�˽��ͷ
                                        MP_IN uint8_t priority,         //�����������ȼ�
                                        MP_IN bool std,                 //��׼��
                                        MP_IN bool use_ssrc,            //�Ƿ��ƶ�ssrc
                                        MP_IN uint32_t ssrc);           //ssrc

XT_MP_CASTER_API mp_bool get_sink_sn(MP_IN mp_h hmp, unsigned short *sn);

//��ָ��MP��MSSRC�˿�д��RTP���ݰ�
XT_MP_CASTER_API mp_bool pump_rtp_in(
                                     MP_IN mp_h	hmp,				//Ŀ��mp���
                                     MP_IN mssrc_h hmssrc,			//Ŀ��mssrc���
                                     MP_IN uint8_t *rtp,			//RTP������Ϣ��
                                     MP_IN uint32_t rtpSize);		//RTP������
//��ָ��MP��MSSRC�˿�д��RTP���ݰ�������0����ģʽ�����û���ʹ��tghelper����ڴ��ͳ�ϵͳ
// �ڴ����ʽ����ѭ˭����˭�ͷŵ�ԭ��ֻ����������ָ�뼼�����ڴ���ύ�ɹ������ü���������
XT_MP_CASTER_API mp_bool pump_rtp_in2(
                                      MP_IN mp_h	hmp,				//Ŀ��mp���
                                      MP_IN mssrc_h hmssrc,			//Ŀ��mssrc���
                                      MP_IN void *rtp);				//RTP������Ϣ��,
//�ⲿ����byte_block���ݿ飬��ʹ���귵��Դbyte_pool
//�������óɹ���rtp�����ü��������ӣ��û�������˺�����
//������rtp->assign()����rtp->release()


XT_MP_CASTER_API void setSink_RtcpCB(mp_handle sink, pSink_RtcpCB func);

XT_MP_CASTER_API void setSink_SID(mp_handle sink, long sid);

XT_MP_CASTER_API void setSink_AppMsgCB(mp_handle sink, pRtcpAppMsgCB func);

XT_MP_CASTER_API void mp_rtcp_set_rawdata_cb(mp_handle sink, pRtcpRawDataCB func);

XT_MP_CASTER_API void pump_sr_in(mp_handle sink, int nDeviceType,
                                 int nLinkRet, int nFrameType,
                                 void *pSendReport);

//zhouzx 20141016 ���յ������׳�Զ�˵�ַ
XT_MP_CASTER_API void set_raddr_cb(mp_h	hmp, raddr_cb cb);

// �����ļ�����·��
XT_MP_CASTER_API void mp_set_file_path(MP_IN mp_h	hmp,				//Ŀ��mp���
									MP_IN const char *file);		//�ļ�����·��

#ifdef _USE_RTP_SEND_CONTROLLER
typedef void (*mp_network_changed_callback_t)(void *ctx, mp_handle hmp, uint32_t bitrate, uint32_t fraction_lost, uint32_t rtt);
XT_MP_CASTER_API void mp_register_network_changed_callback(mp_handle hmp, mp_network_changed_callback_t cb, void *ctx);
#endif

#endif
