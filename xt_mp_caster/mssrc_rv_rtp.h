///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：mssrc_rv_rtp.h
// 创 建 者：汤戈
// 创建时间：2012年04月05日
// 内容描述：媒体数据源 -- rv_rtp包类型
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MSSRC_RV_RTP_ENTITY_
#define MSSRC_RV_RTP_ENTITY_

#include "mp_caster_config.h"
#include "mp.h"
#include "xt_mp_caster_def.h"
#include <../rv_adapter/rv_def.h>

namespace xt_mp_caster
{
    /*
    具备一个fifo的实体类，内部采用rv_adpater的数据接收机制
    1、open/close成对调用确保实体有效，其中基类m_bReady变量标识open/close调用状态
    2、active/recycle_alloc_event/recycle_release_event引发m_active的状态变化
    3、pump_frame_in/pump_frame_out是数据的写入和读取接口，并受控于m_active状态
    4、该类不接受来自外部的数据推送过程
    */
    class mssrc_rv_rtp : public mssrc
    {
    public:
        mssrc_rv_rtp();
        virtual ~mssrc_rv_rtp();

    public:
        bool open(mp* hmp,
            inner::rtp_pool *rtp_pool,
            rv_net_address local_address,
            bool manual_rtcp,
            bool bActive,
            bool bMulticastRTP,
            rv_net_address multicast_rtp_address,
            bool bMulticastRTCP,
            rv_net_address multicast_rtcp_address);		//创建
        void close();											//关闭
        bool active(bool bActive);								//激活
        inline bool isAcitve() { return m_active; }

    protected:
        bool pump_rtp_in(rtp_block *rtp);			//RTP数据包写入 -- 仅提供内部数据写入

    public:
        rtp_block *pump_rtp_out();					//RTP数据包读取
        //配额提取方式RTP数据包，返回实际提取数量
        uint32_t pump_rtps_out(std::list<rtp_block *> &resultBlocks, uint32_t canSendSize);

        bool read_rtcp_sr(rtcp_send_report *sr);
        bool read_rtcp_rr(rtcp_receive_report *rr);

    protected:
        //对象池的分配和去配过程
        //1、对mssrc的fifo写入读出的m_active准入方式进行控制
        //2、去配操作会自动关闭rv_adapter的session资源
        virtual void recycle_alloc_event();
        virtual void recycle_release_event();

        static void OnRtpReceiveEvent_AdapterFunc(
            RV_IN rv_handler  hrv,
            RV_IN rv_context context);
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
            RV_IN uint32_t size);

    private:
        tghelper::recycle_queue m_fifo;
        rv_net_address m_local_address;
        rv_handler_s m_hrv;
        bool m_active;
        tghelper::recycle_queue m_sr_fifo;
        tghelper::recycle_queue m_rr_fifo;
        mp* m_mp;
        inner::rtp_pool *m_rtp_pool;
    };
}

#endif
