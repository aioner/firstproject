///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：mp_caster.h
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：媒体数据广播器
//
// 1、所有接口类均为线程安全
// 2、采用boost库中的mutex对象实现线程同步
//
// 修订日志
// [2012-03-23]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MP_CASTER_
#define MP_CASTER_

#include "mp_caster_config.h"
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include "xt_mp_caster_def.h"
#include "mp.h"
#include "caster_engine.h"
#include "xt_av_check.h"

#ifdef _USE_RTP_TRAFFIC_SHAPING
#include "msec_timer.h"
#endif

namespace xt_mp_caster
{
    class caster
    {
        /*
        单件类形式输出接口和类工厂方法
        */
    private:

        caster();
        ~caster();

        bool init_caster(MP_IN caster_descriptor *descriptor);
        void end_caster();

    public:
        static caster * _instance;
    public:
        //全局管理函数
        // 1、本模块第一调用函数，成功返回caster句柄
        static caster *init(MP_IN caster_descriptor *descriptor);
        // 2、本模块最后一个调用函数
        static void end();
        // 3、取caster句柄
        static caster *self();
        // 4、共享加锁，确保caster句柄有效性
        static void share_lock();
        // 5、共享解锁，确保caster句柄有效性
        static void share_unlock();

        //mp类工厂方法
    public:
        bool open_bc_mp(
            MP_IN bc_mp_descriptor* descriptor, 
            MP_OUT mp_h hmp,						//mp实体句柄
            MP_OUT mssrc_h hmssrc,					//用于pump_frame_in/pump_rtp_in
            MP_OUT msink_h hmsink,					//用于read_rtcp_sr/read_rtcp_rr，不可调用del_sink释放
            MP_OUT uint32_t *multID);

        //更新丢包重传开关 add by songlei 20150708
        void update_resend_flag(MP_IN mp_h hmp,const int resend_flag);

        bool open_proxy_mp(	
            MP_IN proxy_mp_descriptor* descriptor, 
            MP_OUT mp_h hmp,						//mp实体句柄
            MP_OUT mssrc_h hmssrc,					//用于read_rtcp_sr/read_rtcp_rr
            MP_OUT msink_h hmsink);					//用于read_rtcp_sr/read_rtcp_rr，不可调用del_sink释放
        bool open_mixer_mp(	//unsupported
            MP_IN mixer_mp_descriptor* descriptor, 
            MP_OUT mp_h hmp) { return false; }
        bool open_tanslator_mp(
            MP_IN tanslator_mp_descriptor* descriptor, 
            MP_OUT mp_h hmp) { return false; }

        bool close_mp(MP_IN mp_h hmp);
        bool active_mp(MP_IN mp_h hmp, bool bActive);

        bool add_rtp_sink(
            MP_IN mp_h hmp, 
            MP_IN rtp_sink_descriptor* descriptor, 
            MP_OUT msink_h hsink);

        bool add_stream_sink(	//unsupported
            MP_IN mp_h hmp,
            MP_IN stream_rtp_sink_descriptor* descriptor,
            MP_OUT msink_h hsink){ return false; }

        bool add_mem_sink(		
            MP_IN mp_h hmp,
            MP_IN memory_sink_descriptor* descriptor,
            MP_IN bool bActive,
            MP_OUT msink_h hsink);

        bool del_sink(MP_IN mp_h hmp, MP_IN msink_h hsink);

        bool read_rtcp_sr(
            MP_IN msink_h hsink, 
            MP_OUT rtcp_send_report *sr);
        bool read_rtcp_rr(
            MP_IN msink_h hsink, 
            MP_OUT rtcp_receive_report *rr);

        bool read_rtcp_sr(
            MP_IN mssrc_h hssrc, 
            MP_OUT rtcp_send_report *sr);
        bool read_rtcp_rr(
            MP_IN mssrc_h hssrc, 
            MP_OUT rtcp_receive_report *rr);

		bool get_sink_sn(MP_IN mp_h hmp,
			MP_OUT uint16_t *sn);

        //拷贝复制模式，函数执行分包动作
        bool pump_frame_in(
            MP_IN mp_h	hmp,				//目标mp句柄
            MP_IN mssrc_h hmssrc,			//目标mssrc句柄
            MP_IN uint8_t *frame,			//数据帧缓冲区
            MP_IN uint32_t framesize,		//数据帧长度
            MP_IN bool	   frameTS_opt,		//数据帧时间戳外部设置有效性，false为内部计算时间戳(mp_caster_config.h)
            MP_IN uint32_t frameTS,			//数据帧时间戳 == 0表示内部计算时间戳
            MP_IN uint8_t  framePayload,	//数据帧伪装媒体类型 0xFF表示用缺省配置(mp_caster_config.h)
            MP_IN XTFrameInfo &info,
			MP_IN uint8_t priority,          //发送数据优先级
            bool is_std_data,
            MP_IN bool use_ssrc,
            MP_IN uint32_t ssrc);
		//对指定MP的MSSRC端口写入RTP数据包，采用0拷贝模式，但用户需使用tghelper库的内存块和池系统
		// 内存块形式上遵循谁申请谁释放的原则，只是利用智能指针技术，内存块提交成功后，引用计数会增加
		mp_bool pump_rtp_in2(
			MP_IN mp_h	hmp,				//目标mp句柄
			MP_IN mssrc_h hmssrc,			//目标mssrc句柄
			MP_IN void *rtp);				//RTP完整信息包,
		//外部输入byte_block数据块，库使用完返还源byte_pool
		//函数调用成功，rtp的引用计数会增加，用户调用完此函数后
		//需配套rtp->assign()调用rtp->release()

#ifdef _USE_RTP_TRAFFIC_SHAPING
        deadline_timer_mgr_t& get_timer_mgr();
        bool use_traffic_shapping() const;
#endif
    public:
        mp_pools m_objpools;
        caster_engine *m_engine;
    private:
        bool m_bReady;

#ifdef _USE_RTP_TRAFFIC_SHAPING
        deadline_timer_mgr_t m_timer_mgr;
        bool m_use_traffic_shapping;
#endif

        private:
            std::map<mp*, mp*> m_mEntity;
            char m_license[128];
    };
}


#endif

