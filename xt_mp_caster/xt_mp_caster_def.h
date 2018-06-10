///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：xt_mp_caster_def.h
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：兴图新科公司mp广播服务库
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef XT_MP_CASTER_DEFINE_
#define XT_MP_CASTER_DEFINE_

#include<stdint.h>

#ifdef XT_MP_CASTER_EXPORTS
#include <xt_mp_def.h>
#include <rv_adapter/rv_def.h>
#else
#include <xt_mp_def.h>
#include <rv_adapter/rv_def.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct caster_descriptor_
    {
        uint32_t thread_nums;				//caster 线程池配置
        uint32_t rv_adapter_thread_nums;	//rv_adapter线程池配置
        uint32_t numberOfSessions;			//复用会话数
        mp_bool use_traffic_shapping;       //是否开启流量整形
    } caster_descriptor;

    //mp类型描述
    typedef enum mp_type_
    {
        META_MP,			//基类使用类型
        BROADCAST_MP,		//广播型MP 1:N
        PROXY_MP,			//代理型MP 1:N
        MIXER_MP,			//混合型MP M:N
        TRANSLATOR_MP,		//转换型MP 1:1
    } mp_type;

    //mssrc类型描述
    typedef enum mssrc_type_
    {
        META_MSSRC,
        FRAME_MSSRC,
        RTP_MSSRC,
        RV_RTP_MSSRC,		//radvision 协议栈适配ssrc
    } mssrc_type;

    //msink类型描述
    typedef enum msink_type_
    {
        META_MSINK,
        RTP_MSINK,
        STREAM_RTP_MSINK,
        MEMORY_MSINK,
        RV_RTP_MSINK,		//radvision 协议栈适配sink
    } msink_type;

    //流类型描述
    typedef enum stream_mode_
    {
        STD_RTP_STREAM = 0,  //标准RTP流
        PS_RTP_STREAM,       //国标GB28181，PS流
    } stream_mode;

    typedef struct bc_mp_descriptor_	//广播型MP描述符
    {
        //rv_adapater 配置信息
        //	rv_adapater要求的RTP接收地址，建议设置为偶数
        //	默认开启的RTCP接收地址为RTP地址端口+1
        //	manual_rtcp可以考虑开启后关闭rtcp输出，
        //	因为这会引起额外的带宽开销
        rv_net_address local_address;
        mp_bool manual_rtcp;
        //media stream配置信息
        uint32_t max_bandwidth;		// 媒体的平均带宽	(unit. bytes/s), 0 -- 表示关闭带宽整形
        mssrc_type ssrc_type;		// 媒体源类型
        mp_bool active_now;			// 立刻激活标志

        uint8_t msink_multicast_rtp_ttl;			//组播输出RTP的TTL值
        uint8_t msink_multicast_rtcp_ttl;			//组播输出RTCP的TTL值

        mp_bool multiplex;                      //端口复用模式
    } bc_mp_descriptor;

    typedef struct proxy_mp_descriptor_	//代理型MP描述符
    {
        //mssrc_rv_rtp
        rv_net_address listen_address;
        mp_bool mssrc_manual_rtcp;
        //msink_rv_rtp
        rv_net_address local_address;
        mp_bool msink_manual_rtcp;
        //media stream配置信息
        uint32_t max_bandwidth;		// 媒体的平均带宽	(unit. bytes/s), 0 -- 表示关闭带宽整形
        mp_bool active_now;			// 立刻激活标志

        //组播相关设置
        mp_bool mssrc_multicast_rtp_opt;				//是否接收组播RTP数据
        rv_net_address listen_mulitcast_rtp_address;	//组播RTP监听地址
        mp_bool mssrc_multicast_rtcp_opt;				//是否接收组播RTCP数据
        rv_net_address listen_mulitcast_rtcp_address;	//组播RTCP监听地址

        uint8_t msink_multicast_rtp_ttl;			//组播输出RTP的TTL值
        uint8_t msink_multicast_rtcp_ttl;			//组播输出RTCP的TTL值

        mp_bool multiplex;                      //端口复用模式
    } proxy_mp_descriptor;

    typedef struct mixer_mp_descriptor_	//混合型MP描述符
    {
        //unsupported
    } mixer_mp_descriptor;

    typedef struct tanslator_mp_descriptor_	//转换型MP描述符
    {
        //unsupported
    } tanslator_mp_descriptor;


    typedef struct rtp_sink_descriptor_
    {
        rv_net_address rtp_address;		//rtp输出地址
        mp_bool rtcp_opt;				//rtcp功能有效选项
        rv_net_address rtcp_address;	//rtcp输出地址
        mp_bool multiplex;				//是否端口复用
        uint32_t multiplexID;			//复用ID
    } rtp_sink_descriptor;

    typedef struct stream_rtp_sink_descriptor_
    {
        //unsupported
    } stream_rtp_sink_descriptor;

    typedef void (*MemorySinkEventHandler_CB)(
        MP_IN mp_h		hmp,			//产生数据输出mp句柄
        MP_IN msink_h	hsink,			//产生数据输出的msink句柄
        MP_IN mp_context context,		//用户上下文数据
        MP_IN uint8_t * buf,			//数据空间
        MP_IN uint32_t size);			//数据空间大小

    typedef void (*MemorySinkEventHandlerEx_CB)(
        MP_IN mp_h		hmp,			//产生数据输出mp句柄
        MP_IN msink_h	hsink,			//产生数据输出的msink句柄
        MP_IN mp_context context,		//用户上下文数据
        MP_IN msink_block buf			//数据空间采用byte_block输出，用户可自由支配内存块,但不能转移所有权
        );

    typedef struct memory_sink_descriptor_
    {
        /*
        设置了onMemorySinkExEvent后，msink将仅使用该回调接口
        */
        MemorySinkEventHandler_CB	onMemorySinkEvent;		//事件回调接口
        MemorySinkEventHandlerEx_CB onMemorySinkExEvent;	//事件回调接口，优先级高于onMemorySinkEvent
        mp_context context;
    } memory_sink_descriptor;

    typedef struct _XTFrameInfo
    {
        unsigned int verify;
        unsigned int frametype;     //帧数据类型，表示为H264，H265，AAC等其中一种
        unsigned int datatype;      //设备类型
        unsigned int streamtype;    //流类型，0为标准RTP流，1为国标PS流，后续再增加
    }XTFrameInfo;

    typedef void (*raddr_cb)(void *hmp,rv_net_address *addr);
#ifdef __cplusplus
}
#endif

#endif
