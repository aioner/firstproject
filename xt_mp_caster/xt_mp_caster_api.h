///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：xt_mp_caster_api.h
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：兴图新科公司mp广播服务库
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

//库初始化
XT_MP_CASTER_API mp_bool init_mp_caster(MP_IN caster_descriptor *descriptor);
//库析构
XT_MP_CASTER_API void end_mp_caster(void);

//获取当前版本号
XT_MP_CASTER_API void get_mp_caster_version(uint32_t *v1, uint32_t *v2, uint32_t *v3);


//创建一个广播MP
XT_MP_CASTER_API mp_bool open_bc_mp(
                                    MP_IN bc_mp_descriptor* descriptor,
                                    MP_OUT mp_h hmp,						//mp实体句柄
                                    MP_OUT mssrc_h hmssrc,					//用于pump_frame_in/pump_rtp_in
                                    MP_OUT msink_h hmsink,					//用于read_rtcp_sr/read_rtcp_rr，不可调用del_sink释放
                                    MP_OUT uint32_t *multid);
//更新丢包重传开关 add by songlei 20150708
XT_MP_CASTER_API void update_resend_flag(MP_IN mp_h hmp,const int resend_flag);

//创建一个代理MP
XT_MP_CASTER_API mp_bool open_proxy_mp(
                                       MP_IN proxy_mp_descriptor* descriptor,
                                       MP_OUT mp_h hmp,						//mp实体句柄
                                       MP_OUT mssrc_h hmssrc,					//用于pump_frame_in/pump_rtp_in
                                       MP_OUT msink_h hmsink);					//用于read_rtcp_sr/read_rtcp_rr，不可调用del_sink释放
//析构指定MP
XT_MP_CASTER_API mp_bool close_mp(MP_IN mp_h hmp);
//激活或禁用指定MP，禁用的MP将不接纳外部数据输入，也不输出数据
XT_MP_CASTER_API mp_bool active_mp(MP_IN mp_h hmp, mp_bool bActive);

//对指定MP增加rtp输出接点，rtp_sink_descriptor中的地址操作使用rv_adapter中的相关函数
XT_MP_CASTER_API mp_bool add_rtp_sink(
                                      MP_IN mp_h hmp,
                                      MP_IN rtp_sink_descriptor* descriptor,
                                      MP_OUT msink_h hsink);
//对指定MP增加内存回调输出点
XT_MP_CASTER_API mp_bool add_mem_sink(
                                      MP_IN mp_h hmp,
                                      MP_IN memory_sink_descriptor* descriptor,
                                      MP_IN mp_bool bActive,
                                      MP_OUT msink_h hsink);
//对指定MP删除指定sink输出点
XT_MP_CASTER_API mp_bool del_sink(MP_IN mp_h hmp, MP_IN msink_h hsink);
//读取bc_mp的rtcp发送报告，其中hsink参数由open_bc_mp获得
XT_MP_CASTER_API mp_bool read_rtcp_sr_from_sender(
    MP_IN msink_h hsink,
    MP_OUT rtcp_send_report *sr);
//读取bc_mp的rtcp接收报告，其中hsink参数由open_bc_mp获得
XT_MP_CASTER_API mp_bool read_rtcp_rr_from_sender(
    MP_IN msink_h hsink,
    MP_OUT rtcp_receive_report *rr);
//读取rv_ssrc的rtcp发送报告，其中hssrc参数由open_xxx_mp获得
XT_MP_CASTER_API mp_bool read_rtcp_sr_from_receiver(
    MP_IN mssrc_h hssrc,
    MP_OUT rtcp_send_report *sr);
//读取rv_ssrc的rtcp接收报告，其中hsink参数由open_xxx_mp获得
XT_MP_CASTER_API mp_bool read_rtcp_rr_from_receiver(
    MP_IN mssrc_h hssrc,
    MP_OUT rtcp_receive_report *rr);
//对指定MP的MSSRC端口写入数据帧
/*
pump_frame_in函数
如果用户采用自定义frameTS_opt, frameTS, framePayload要满足rtp协议的规范填写
例如：音频PCMU framePayload = 0, frameTS_opt = MP_TRUE, 则frameTS按8000Hz采样率表达，1ms = 8 unit
例如：视频H263 framePayload = 34,frameTS_opt = MP_TRUE, 则frameTS按90000Hz采样率表达，1ms = 90 unit
库内部的默认定义参考mp_caster_config.h
*/
XT_MP_CASTER_API mp_bool pump_frame_in(
                                        MP_IN mp_h	hmp,				//目标mp句柄
                                        MP_IN mssrc_h hmssrc,			//目标mssrc句柄
                                        MP_IN uint8_t *frame,			//数据帧缓冲区
                                        MP_IN uint32_t framesize,		//数据帧长度
                                        MP_IN mp_bool  frameTS_opt,		//数据帧时间戳外部设置有效性，false为内部计算时间戳(mp_caster_config.h)
                                        MP_IN uint32_t frameTS,			//数据帧时间戳 == 0表示内部计算时间戳
                                        MP_IN uint8_t  framePayload,	//数据帧伪装媒体类型 0xFF表示用缺省配置(mp_caster_config.h)
                                        MP_IN XTFrameInfo &info,        //12字节私有头
                                        MP_IN uint8_t priority,         //发送数据优先级
                                        MP_IN bool std,                 //标准流
                                        MP_IN bool use_ssrc,            //是否制定ssrc
                                        MP_IN uint32_t ssrc);           //ssrc

XT_MP_CASTER_API mp_bool get_sink_sn(MP_IN mp_h hmp, unsigned short *sn);

//对指定MP的MSSRC端口写入RTP数据包
XT_MP_CASTER_API mp_bool pump_rtp_in(
                                     MP_IN mp_h	hmp,				//目标mp句柄
                                     MP_IN mssrc_h hmssrc,			//目标mssrc句柄
                                     MP_IN uint8_t *rtp,			//RTP完整信息包
                                     MP_IN uint32_t rtpSize);		//RTP包长度
//对指定MP的MSSRC端口写入RTP数据包，采用0拷贝模式，但用户需使用tghelper库的内存块和池系统
// 内存块形式上遵循谁申请谁释放的原则，只是利用智能指针技术，内存块提交成功后，引用计数会增加
XT_MP_CASTER_API mp_bool pump_rtp_in2(
                                      MP_IN mp_h	hmp,				//目标mp句柄
                                      MP_IN mssrc_h hmssrc,			//目标mssrc句柄
                                      MP_IN void *rtp);				//RTP完整信息包,
//外部输入byte_block数据块，库使用完返还源byte_pool
//函数调用成功，rtp的引用计数会增加，用户调用完此函数后
//需配套rtp->assign()调用rtp->release()


XT_MP_CASTER_API void setSink_RtcpCB(mp_handle sink, pSink_RtcpCB func);

XT_MP_CASTER_API void setSink_SID(mp_handle sink, long sid);

XT_MP_CASTER_API void setSink_AppMsgCB(mp_handle sink, pRtcpAppMsgCB func);

XT_MP_CASTER_API void mp_rtcp_set_rawdata_cb(mp_handle sink, pRtcpRawDataCB func);

XT_MP_CASTER_API void pump_sr_in(mp_handle sink, int nDeviceType,
                                 int nLinkRet, int nFrameType,
                                 void *pSendReport);

//zhouzx 20141016 接收到数据抛出远端地址
XT_MP_CASTER_API void set_raddr_cb(mp_h	hmp, raddr_cb cb);

// 设置文件保存路径
XT_MP_CASTER_API void mp_set_file_path(MP_IN mp_h	hmp,				//目标mp句柄
									MP_IN const char *file);		//文件保存路径

#ifdef _USE_RTP_SEND_CONTROLLER
typedef void (*mp_network_changed_callback_t)(void *ctx, mp_handle hmp, uint32_t bitrate, uint32_t fraction_lost, uint32_t rtt);
XT_MP_CASTER_API void mp_register_network_changed_callback(mp_handle hmp, mp_network_changed_callback_t cb, void *ctx);
#endif

#endif
