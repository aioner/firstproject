///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：bc_mp.h
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：媒体数据处理单元 -- 广播型内置msink_rv_rtp
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef BRODCAST_RTP_MP_ENTITY_
#define BRODCAST_RTP_MP_ENTITY_

#include "mp_caster_config.h"
#include "mp.h"
#include "mssrc_frame.h"
#include "mssrc_rtp.h"
#include "msink_rv_rtp.h"
#include "rtp_packet_block.h"
#ifdef _USE_RTP_SEND_CONTROLLER
#include "send_side_controller.h"
#include "xt_mp_caster_api.h"
#endif

namespace xt_mp_caster
{
    struct SHeader{
        unsigned uFourCC;       //识别标志 CHUNK_HEADER_FOURCC
        unsigned uHeaderSize;   //帧头长度 sizeof(SChunkHeader)
        unsigned uMediaType;    //帧类型
        unsigned uChunkCount;   //流水序号
        unsigned uDataSize;     //数据长度(不含头结构)
        unsigned uTimeStamp;    //帧时间戳
    };

    class bc_mp : public mp
#ifdef _USE_RTP_SEND_CONTROLLER
        ,public bitrate_observer_t
#endif
    {
    public:
        bc_mp();
        virtual ~bc_mp();

    public:
        bool open(
            MP_IN bc_mp_descriptor * descriptor, 
            MP_OUT mssrc_h hmssrc, 
            MP_OUT msink_h hmsink,
            MP_OUT uint32_t *multID);

        //更新丢包重传开关 add by songlei 20150708
        void update_resend_flag(const int resend_flag);

        void close();
        virtual bool active(bool bActive);
        bool isAcitve(){ return m_active; }
		virtual bool pump_rtp_in(MP_IN mssrc *hmssrc,MP_IN void *rtp);
		//拷贝复制模式，函数执行分包动作
		virtual bool pump_frame_in(
			MP_IN mssrc * hmssrc,			//目标mssrc句柄
			MP_IN uint8_t *frame,			//数据帧缓冲区
			MP_IN uint32_t framesize,		//数据帧长度
			MP_IN bool	   frameTS_opt,		//数据帧时间戳外部设置有效性，false为内部计算时间戳(mp_caster_config.h)
			MP_IN uint32_t frameTS,			//数据帧时间戳 == 0表示内部计算时间戳
			MP_IN uint8_t  framePayload,	//数据帧伪装媒体类型 0xFF表示用缺省配置(mp_caster_config.h)
			MP_IN XTFrameInfo &info,
            MP_IN uint8_t priority,//发送数据优先级
            MP_IN bool is_std_data,
            MP_IN bool use_ssrc,
            MP_IN uint32_t ssrc);

		mp_bool pump_frame_in_aac(
								MP_IN mssrc *hmssrc,			//目标mssrc句柄
								MP_IN uint8_t *frame,			//帧数据(去除私有头)
								MP_IN uint32_t framesize,		//数据帧长度
								MP_IN uint32_t frameTS,			//时间戳(私有头获得)			  
								MP_IN uint8_t  framePayload,    // 96
                                MP_IN uint8_t priority,
                                MP_IN bool use_ssrc,
                                MP_IN uint32_t ssrc);

        mp_bool frame_unpack_aac(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu);

		mp_bool pump_frame_in_s(
								MP_IN mssrc *hmssrc,			//目标mssrc句柄
								MP_IN uint8_t *frame,			//帧数据(去除私有头)
								MP_IN uint32_t framesize,		//数据帧长度
								MP_IN uint32_t frameTS,			//时间戳(私有头获得)			  
								MP_IN uint8_t  framePayload,    // 96
                                MP_IN uint8_t priority,         // 发送数据优先级
                                MP_IN uint8_t stream_mode,	    // 发送数据流的模式，0默认标注RTP，1为PS
                                MP_IN bool use_ssrc,
                                MP_IN uint32_t ssrc);
		mp_bool pump_frame_in_hevc(
			MP_IN mssrc *hmssrc,			//目标mssrc句柄
			MP_IN uint8_t *frame,			//帧数据(去除私有头)
			MP_IN uint32_t framesize,		//数据帧长度
			MP_IN uint32_t frameTS,			//时间戳(私有头获得)			  
			MP_IN uint8_t  framePayload,	// 96
            MP_IN uint8_t priority,        // 发送数据优先级
            MP_IN bool use_ssrc,
            MP_IN uint32_t ssrc);

        bool add_rtp_sink(rtp_sink_descriptor *descriptor, msink_h hsink);
		uint16_t get_sink_sn();
        virtual bool del_msink(msink *hmsink);

        // 是否h264
        bool is_h264(uint8_t *frame, uint32_t framesize);

		mp_bool frame_unpack(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu,MP_IN uint32_t frameTS,			//时间戳(私有头获得)			  
			MP_IN uint8_t  framePayload);
        mp_bool frame_unpack_ps(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu, uint32_t frameTS);
        mp_bool pes_unpack_rtp(rtp_mblock *mrtp, uint8_t *pes_slice, uint32_t slice_size, uint32_t mtu, uint32_t frameTS);
        mp_bool frame_unpack_hevc(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu);
        mp_bool pump_frame_h264_to_ps(		
            MP_IN mssrc *hmssrc,			//目标mssrc句柄
            MP_IN uint8_t *frame,			//帧数据(去除私有头)
            MP_IN uint32_t framesize,		//数据帧长度
            MP_IN uint32_t frameTS,			//时间戳(私有头获得)			  
            MP_IN uint8_t  framePayload,    // 96
            MP_IN uint8_t priority,
            MP_IN uint8_t stream_mode,
            MP_IN bool use_ssrc,
            MP_IN uint32_t ssrc);

        void set_raddr_cb(raddr_cb cb);

		void set_file_path(const char *file);

#ifdef _USE_RTP_SEND_CONTROLLER
        void register_network_changed_callback(mp_network_changed_callback_t cb, void *ctx);
#endif

    protected:
        //执行mssrc任务动作
        //1，bwShaper = false任务由外部用户通过pump释放
        //2, bwShaper = true 任务由定时器释放
        virtual bool do_mssrc_task(uint32_t &delayMS);
        virtual bool do_msink_task();

    private:
        //do_mssrc_task的两种子任务处理过程
        //1、do_mssrc_task_frame负责处理帧型数据源的数据提取搬移
        //2、do_mssrc_task_rtp负责处理RTP包型数据源的数据提取搬移
        uint32_t do_mssrc_task_frame(
            mssrc_frame *hmssrc, msink_rv_rtp * hmsink, 
            uint32_t canSendSize);
        uint32_t do_mssrc_task_rtp(
            mssrc_rtp *hmssrc, msink_rv_rtp * hmsink, 
            uint32_t canSendSize);

        //对象出池和入池操作
        virtual void recycle_alloc_event();	
        virtual void recycle_release_event();


    private:
        bool m_active;
        uint32_t m_last_pesudo_ts;
        uint32_t m_last_pesudo_rtp_ts;
        uint16_t m_last_frame_in_rtp_sn;

#ifdef _USE_RTP_SEND_CONTROLLER
        std::auto_ptr<bitrate_controller_t> m_bitrate_controller;
        mp_network_changed_callback_t m_network_changed_callback;
        void *m_network_changed_callback_ctx;

        void on_network_change(uint32_t bitrate, uint8_t fraction_lost, uint32_t rtt);
#endif

    public:
        static raddr_cb m_raddr_cb; 
    };
}
#endif
