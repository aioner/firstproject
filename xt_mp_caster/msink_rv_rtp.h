///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：msink_rv_rtp.h
// 创 建 者：汤戈
// 创建时间：2012年03月26日
// 内容描述：媒体数据宿 -- rv adapter绑定型 RTP包
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MSINK_RV_RTP_ENTITY_
#define MSINK_RV_RTP_ENTITY_

#include "mp_caster_config.h"
#include "mp.h"
#include "xt_mp_caster_def.h"
#include <../rv_adapter/rv_def.h>
#include <boost/thread/mutex.hpp>
#include "ReSendMan.h"
#include "rtp_packet_block.h"
#ifdef _USE_RTP_TRAFFIC_SHAPING
#include "traffic_shaping.h"
#endif  //_USE_RTP_TRAFFIC_SHAPING

#ifdef _USE_RTP_SEND_CONTROLLER
#include "send_side_controller.h"
#endif

#define MAX_RTP_PACK	10240

typedef void (*pSink_RtcpCB)(uint32_t ssrc, rv_rtcp_info &send, rv_rtcp_info &recieve,
							 uint8_t *ip,uint16_t port, rv_bool multiplex,uint32_t multid);
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

struct PktInfo
{
    unsigned long count;
    double interval;
    unsigned long tm;
};

namespace xt_mp_caster
{
    /*
    具备一个fifo的实体类
    1、open/close成对调用确保实体有效，其中基类m_bReady变量标识open/close调用状态
    2、active/recycle_alloc_event/recycle_release_event引发m_active的状态变化
    3、pump_frame_in/pump_frame_out是数据的写入和读取接口，并受控于m_active状态

    */
    class msink_rv_rtp : public msink
#ifdef _USE_RTP_TRAFFIC_SHAPING
    , public flow_out_callback_t
#endif	//_USE_RTP_TRAFFIC_SHAPING
    {
        friend class bc_mp;
    public:
        msink_rv_rtp();
        virtual ~msink_rv_rtp();

    public:
        bool open(rv_net_address local_address,
            bool manual_rtcp,
            bool bActive,
            uint8_t multicast_rtp_ttl,
            uint8_t multicast_rtcp_ttl,
            mp_bool multiplex,
            uint32_t *multID = NULL
#ifdef _USE_RTP_SEND_CONTROLLER
            ,rtcp_observer_t *rtcp_observer = NULL
#endif
            );
        void close();
        bool active(bool bActive);
        inline bool isAcitve() { return m_active; }

        bool pump_rtp_in(rtp_block *rtp);				//rtp数据包写入
		void pump_rtp_out();							//rtp数据包推出至rv_adapter
        bool pump_mrtp_in(rtp_mblock *mrtp);			//mrtp解封形成rtp数据包写入
		void pump_mrtp_out(rtp_mblock *mrtp);           //mrtp解封形成rtp数据包推出至rv_adapter  
		uint16_t m_last_frame_in_rtp_sn;//使用外部计数
        virtual bool do_task()
        {
            pump_rtp_out();
            return true;
        }

        inline void inc_viewer()
        {
            boost::mutex::scoped_lock lock(m_viewer_mutex);
            m_viewer_nums++;
        }

        inline void dec_viewer()
        {
            boost::mutex::scoped_lock lock(m_viewer_mutex);
            if (m_viewer_nums > 0)	m_viewer_nums--;
        }

        inline uint32_t get_viewer() 
        {
            return m_viewer_nums; 
        }

        bool read_rtcp_sr(rtcp_send_report *sr);
        bool read_rtcp_rr(rtcp_receive_report *rr);

        //更新丢包重传开关 add by songlei 20150708
        inline void update_resend_flag(const int resend_flag)
        {
            m_nReSend = resend_flag;
        }

    public:
        virtual void recycle_alloc_event();
        virtual void recycle_release_event();

        static void OnRtpReceiveEvent_AdapterFunc(
            RV_IN rv_handler  hrv,
            RV_IN rv_context context);
        static void RtpDemuxEventHandler(
            RV_IN void*   hDemux,
            RV_IN void*  context);
        static void RV_CALLCONV OnRtcpSendHandler_AdapterFunc(
            RV_IN rv_handler hrv,
            RV_IN rv_context context,
            RV_IN uint32_t ssrc,
            RV_IN uint8_t *rtcpPack,
            RV_IN uint32_t size);
        static void RV_CALLCONV OnRtcpReceiveHandler_AdapterFunc(
            RV_IN rv_handler hrv,
            RV_IN rv_context context,
            RV_IN uint32_t ssrc,
            RV_IN uint8_t *rtcpPack,
            RV_IN uint32_t size,
			RV_IN uint8_t *ip,
			RV_IN uint16_t port,
			RV_IN rv_bool multiplex,
			RV_IN uint32_t multid);

        static rv_bool RV_CALLCONV OnRtcpAppEventHandler_CB(
            RV_IN  rv_handler	 hrv,
            RV_IN  rv_context	 context,
            RV_IN  uint8_t        subtype,
            RV_IN  uint32_t       ssrc,
            RV_IN  uint8_t*       name,
            RV_IN  uint8_t*       userData,
            RV_IN  uint32_t       userDataLen); /* userData in bites!, not in 4 octet words */

        static rv_bool RV_CALLCONV OnRtcpRawBufferReceived_CB(
            RV_IN  rv_handler	 hrv,
            RV_IN  rv_context	 context,
            RV_IN uint8_t *buffer,
            RV_IN uint32_t buffLen,
            RV_OUT rv_net_address *remoteAddress,
            RV_OUT rv_bool *pbDiscardBuffer);

        //具体推送数据至rv_adapter
        void write_to_rv_adapter(rtp_block *rtp, bool bReSend = false);

    public:
        tghelper::recycle_queue m_fifo;
        rv_net_address m_local_address;
        rv_handler_s m_hrv;
        mp_bool m_multiplex;
        uint32_t m_multiplexID;	// 复用ID
        bool m_active;
        boost::mutex m_viewer_mutex;
        uint32_t m_viewer_nums;
        tghelper::recycle_queue m_sr_fifo;
        tghelper::recycle_queue m_rr_fifo;

        // 流量整形
        //TrafficShaping m_trafficShaping;

        // 数据重发
        ReSendMan	m_manReSend;

		//是否开启重传
        int m_nReSend;

		//是否开启包优先级
		int m_open_pri;

        mp *m_mp;

		std::string m_file_path;

    public:
        // 上抛RTCP报告回调
        pSink_RtcpCB m_pSink_RtcpCB;

        // 通道号
        long m_nSID;

        // 包发送信息
        PktInfo m_iPkt;

        // Rtcp回调
        void setSink_RtcpCB(pSink_RtcpCB func){m_pSink_RtcpCB = func;}

        // 上抛APP Message
        pRtcpAppMsgCB m_pAppMsgCB;
        void setSink_AppMsgCB(pRtcpAppMsgCB func){m_pAppMsgCB = func;}

        pRtcpRawDataCB m_pRtcpRawCB;
        void rtcp_set_rawcb(pRtcpRawDataCB cb){m_pRtcpRawCB = cb;}

		void set_file_path(const char *file);

		void write_file(rtp_block *rtp);

        int m_scheck_sum;

    public:
		void internel_write_to_rv_adapter(rtp_block *rtp);
		inner::rtp_pool m_rtp_pool; //内存池，管理由拷贝复制产生的rtp_block
		void rv_write_rtp(rtp_block *rtp);
#ifdef _USE_RTP_TRAFFIC_SHAPING
        void on_flow_out(void *flow);

        std::auto_ptr<traffic_shaping_wrapper_t> m_traffic_shaping;
#endif

#ifdef _USE_RTP_SEND_CONTROLLER
        std::auto_ptr<rtcp_observer_t> m_rtcp_observer;
#endif
    };
}

#endif
