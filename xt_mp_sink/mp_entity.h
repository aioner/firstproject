#ifndef MP_ENTITY_H
#define MP_ENTITY_H

#include "rtp_packet_block.h"
#include "packet_source.h"
#include "rtp_macro_block.h"
#include "packet_sink.h"
#include "recycle_fifo.h"
#include <set>
#include <boost/threadpool.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/atomic.hpp>
#include <stdio.h>

#define use_recycle_entity_pool_func__

typedef int (* FPSendReportOutput)(int nDeviceType, int nLinkRet, int nFrameType, XTSSendReport* pSendReport, void * objUserContext);

struct RcvSeg
{
    uint16_t sn;
    uint16_t time;
    long    tick;
};

struct SrcAddr
{
    bool multiplex;
    unsigned int multid;
    rv_net_address rtpaddr;
    rv_net_address rtcpaddr;
};

namespace xt_mp_sink
{
    typedef tghelper::any_byte_pool<rtp_packet_block,BYTE_POOL_BLOCK_SIZE,BYTE_POOL_EXPAND_SIZE,BYTE_POOL_INIT_SIZE>	rtp_block_pool;
    class mp_entity// : public tghelper::recycle_pool_item
    {

    public:
        typedef enum _mp_state
        {
            MP_IDLE_STATE = 0x0001,
            MP_OPEN_STATE = 0x0002,
        }mp_state;

        mp_entity(
            uint32_t source_fifo_size = MP_SOURCE_FIFO_MAX_SIZE,
            uint32_t sink_fifo_size = MP_SINK_FIFO_MAX_SIZE,
            uint32_t sndout_fifo_size = MP_SND_OUT_BUFFER_SIZE,
            uint32_t snd_out = 0
            );

        virtual ~mp_entity(void);

    public:
        void assign(){ ++m_ref; }
        void release() { --m_ref; }
        int use_count() { return m_ref; }

#ifdef use_recycle_entity_pool_func__
        //entity管理
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    private:
        static boost::recursive_mutex entity_pool_mutex_;
        static std::vector<mp_entity*> recycle_entity_pool_;
        static char m_license[128];
    public:
        static void init_recycle_entity_pool();
        static mp_entity* malloc_entity();
        static bool check_entity(const mp_entity* ptr);
        static void free_entity_all();
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif //#ifdef use_recycle_entity_pool_func__

    public:
        uint32_t mp_open(xt_mp_descriptor* mp_des,p_msink_handle handle,bool multiplex = false, uint32_t *multid = NULL, bool bOpend = false);

        uint32_t mp_active(uint32_t bActive);

        void mp_directoutput(bool direct){m_isDirectOutput = direct;}

        uint32_t mp_close();

        long mp_query_rcv_rtcp(rtcp_receive_report * rtcp);

        long mp_query_snd_rtcp(rtcp_send_report * rtcp);

        inline rtp_macro_block * alloc_macro_block(bool bTrace = false)
        {
            tghelper::recycle_pool_item * item = m_macro_pool.alloc_item(false);
            if(item != null)
                return static_cast<rtp_macro_block *>(item);
            else
                return static_cast<rtp_macro_block *>(tghelper::recycle_pool_build_item<rtp_macro_block>(&m_macro_pool, true));
        }

        inline rtp_packet_block * alloc_rtp_block()
        {
            return static_cast<rtp_packet_block *>(m_rtp_pool.try_alloc_any());
        }

        long mp_read_out_data(uint8_t * pDst,uint32_t size,block_params * param);
        long mp_read_out_data2(uint8_t * pDst,uint32_t size,block_params * param, uint32_t *frame);

		long mp_read_out_rtp(uint8_t *pDst,uint32_t size,rv_rtp_param *param);
		long mp_pump_out_rtp(void **p_rtp_block);

        long mp_manual_send_rtcp_sr(uint32_t pack_size,uint32_t pack_ts);

        long mp_manual_send_rtcp_rr(uint32_t ssrc,uint32_t local_ts,uint32_t ts, uint32_t sn);

        long mp_rtcp_send_dar( uint32_t rate, uint32_t level);

        long rtcp_send_fir();

        long mp_add_rtp_remote_address(mp_address_descriptor * address);

        long mp_del_rtp_remote_address(mp_address_descriptor * address);
        long mp_add_mult_rtp_remote_address( mp_address_descriptor * address, unsigned int multid );

        long mp_clear_rtp_remote_address();

        long mp_add_rtcp_remote_address( mp_address_descriptor * address );
        long mp_add_mult_rtcp_remote_address( mp_address_descriptor * address, unsigned int multid );

        long mp_del_rtcp_remote_address( mp_address_descriptor * address );

        long mp_clear_rtcp_remote_address();

        //一级流水任务，数据入SOURCE FIFO
        void pump_rtp_in(rv_handler hrv);

        void pump_rtp_in(RV_IN void *buf, RV_IN uint32_t buf_len, RV_IN rv_rtp_param *p, RV_IN rv_net_address *address);

        void pump_rtcp_in(rv_handler hrv,uint32_t ssrc);

        //二级流水任务，source与sink数据交换
        void mp_task_data_switcher();

        //三级流水任务，数据输出
        void caster_data_out();

        //四级流水，数据发送
        void send_data_out();

        //查询SR报告
        long mp_get_xtsr(int &nDeviceType, int &nLinkRet, int &nFrameType, XTSSendReport *rtcp);
		//设置丢包重传参数
		static int mp_set_resend(int resend,int wait_resend, int max_resend, int vga_order);

        int  RegistSendReportEvent(FPSendReportOutput fpSendReportOutput, void *objUserContext);

    public:
        void pump_rtp_in_sj( rv_handler hrv );
        void pump_rtp_in_sj( RV_IN void *buf, RV_IN uint32_t buf_len, RV_IN rv_rtp_param *p, RV_IN rv_net_address *address );
        void mp_task_data_switcher_sj();
     private:
        int m_nIncoming;
        int m_syncPackets;
        int m_jarlessPackets;
        int m_jPackets;
        int m_waitFrames;
        uint16_t _lastseq;
        uint16_t m_port;
    public:
        //事件回调函数
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        static void OnRtpReceiveEvent(rv_handler hrv,rv_context context);
        static void OnRtcpSendEvent(rv_handler hrv,rv_context context,uint32_t ssrc,uint8_t *rtcpPack,uint32_t size);
        static void OnRtcpReceiveEvent(rv_handler hrv,rv_context context,uint32_t ssrc,uint8_t *rtcpPack,uint32_t size,
			uint8_t *ip,uint16_t port,rv_bool multiplex,uint32_t multid);
        /* userData in bites!, not in 4 octet words */
        static rv_bool OnRtcpAppEventHandler_CB(RV_IN rv_handler hrv,RV_IN rv_context context,RV_IN uint8_t subtype,
            RV_IN uint32_t ssrc,RV_IN uint8_t* name,RV_IN uint8_t* userData,RV_IN uint32_t userDataLen);
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        static void mp_setdemux_handler(rv_context func);
        bool get_multinfo(bool *multiplex, uint32_t *multid);
        void _clear();
    private:
        static void _construct_rv_address(int8_t * ip,uint16_t port,rv_net_address * rv_address);
        inline bool _close_session()
        {
            return  RV_ADAPTER_TRUE == close_session(&m_handle) ? true : false;
        }
        uint32_t _transfer_video_byteblock();
        void _post_caster_task();

    public:
        //xy 527
        uint32_t m_ssrc;

        long m_lastFrameMarkerSN;//上一个frame帧的sn

        //byte_pool
        rtp_block_pool	m_rtp_pool;

        //macro_pool
        tghelper::recycle_pool m_macro_pool;

        //source
        rtp_packet_source m_source;

        //sink
        rtp_packet_sink m_sink;

        //state
        //mp_state m_state;
        boost::atomic<mp_state> m_state;

        //active mp
        //bool m_bActive;
        boost::atomic_bool m_bActive;

        //session handler
        rv_handler_s m_handle;

        //lock
        //boost::recursive_mutex m_mutex;
        boost::recursive_mutex m_member_variable_mutex; //资源锁保护不具备线程安全的成员变量

        //流水线串行运行锁，仅对需要进行串行化的MP中使用
        //boost::timed_mutex
        boost::timed_mutex m_mssrc_task_mutex;
        boost::timed_mutex m_msink_task_mutex;
        boost::timed_mutex m_mforword_task_mutex;

        //回调输出fifo
        tghelper::recycle_queue m_rtp_fifo;

		boost::mutex m_mutex;//pop和push时加锁
        //receive rtcp fifo
        recycle_fifo<rtcp_receive_report> m_rcv_rtcp_fifo;
        //send rtcp fifo
        recycle_fifo<rtcp_send_report> m_snd_rtcp_fifo;
        //测试fifo
        tghelper::recycle_queue m_debug_fifo;
        //send out fifo
        tghelper::recycle_queue m_snd_out_fifo;

        //rtp ssrc
        uint32_t m_rtp_ssrc;
        //rtcp ssrc
        uint32_t m_rtcp_ssrc;

        unsigned char m_fir_seq;

        //output video callback
        ReceiveFrameEvent_CB m_pRcvFrameCB;

        //output audio callback
        ReceiveRtpEvent_CB m_pRcvRtpCB;

        //report event callback
        ReportEvent_CB m_pRptEventCB;

        //用户上下文
        void * m_pContext;

        //数据输出模式
        trans_mode m_mode;

        //是否直接以包抛出上层组帧
        bool m_isDirectOutput;

        //记录RTCP报告生成方式
        bool m_bManualRtcp;
        //RcvSeg m_rcvSeg;

        boost::atomic<int> m_nFrame;
        //////////////////////////////////////////////////////////////////////////
        // 数据重发
        static boost::atomic<int> m_nReSendCfg;
		static bool m_bSetReSend;

        // 手动丢包
        boost::atomic<int> m_nLostCfg;

        uint16_t m_lastSn;
        bool m_bInitLast;
        inline void ClearReSend();

        // 已请求队列
        std::vector<RcvSeg> m_listAck;

        // 计算丢失包
        inline void CalLost(std::vector<uint16_t> &listLost, uint16_t sn1, uint16_t sn2);

        // SN比较
        inline bool IsSmaller(uint16_t sn1, uint16_t sn2);

        // 计算差值
        inline uint16_t CalDif(uint16_t sn1, uint16_t sn2);

        inline bool IsReSend(rv_rtp_param &param);

        // RTP会话保护  m_handle
        boost::recursive_mutex m_mLost;
        //////////////////////////////////////////////////////////////////////////

        // 是否有sr from appmsg
        boost::atomic_bool m_bManualSR_cfg;

        XTSR m_xtsr;

        FPSendReportOutput m_funcSR;

        void *m_puserdata;
        bool m_bMultiplex;
        uint32_t m_multID;

        // 发送端地址
        SrcAddr m_srcaddr;

        // 添加发送地址时间
        unsigned long m_tmSrcAddr;

        // 添加发送地址时间间隔
        unsigned long m_tmSrcAddrInterval;

        // 添加发送端地址
        void add_srcaddr();

        // 更新发送端地址
        void update_srcaddr_rtp(rv_net_address &addr);
        void update_srcaddr_rtcp(rv_net_address &addr);

        //////////////////////////////////////////////////////////////////////////
        static boost::recursive_mutex m_mEntity;
        static std::set<mp_handle> m_setSinkHandleMrg;
        static void add_entity(void *entity);
        static bool is_valid(void *entity);
        static void del_entity(void *entity);
        static int size_entity();
        static boost::recursive_mutex m_mEntityClose;
        //////////////////////////////////////////////////////////////////////////
        //xt_mp_descriptor m_des;
        uint16_t m_last_rtp_sn;
        //////////////////////////////////////////////////////////////////////////

        // 数据校验开关
        boost::atomic<int> m_rcheck_sum_cfg;

        // 强制RR时间
        unsigned long m_tForceLevel;
        unsigned long m_tForceRR;

        //引用计数
        boost::atomic<int> m_ref;


		////////////////////////////////////////////////////////////////
		class sn_cmp
		{
		public:
			bool operator()(uint16_t sn1,uint16_t sn2)
			{
				if (sn1<sn2 && (sn2-sn1)<MP_SN_JUDGE_CONDITION)
				{
					return true;
				}
				else if (sn1>sn2 && (sn1-sn2)>MP_SN_JUDGE_CONDITION)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
		};
		rtp_block *m_block_pool;//组帧索引指针，保证无序数据索引有序
		int m_block_index;
		int do_resend(bool resend,rv_rtp_param param);
		void erase_front_frame();
		int check_jarless(uint16_t sn);
		int deal_rtp_packet(uint16_t in_sn,bool in_mark,bool resend,rtp_block* block);

		typedef std::map<uint16_t,rtp_block *,sn_cmp> rtp_packet_t;
		typedef std::map<uint16_t,rtp_block *,sn_cmp>::iterator itr_rtp_packet_t;
		std::map<uint16_t,rtp_block *,sn_cmp> map_rtp_packets;
		int wait_frames;
		int lost_packet_count;
		bool m_pump_flags;
    };
}

#endif
