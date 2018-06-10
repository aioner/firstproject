///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：mssrc_rv_rtp.cpp
// 创 建 者：汤戈
// 创建时间：2012年04月05日
// 内容描述：媒体数据源 -- rv_rtp包类型
///////////////////////////////////////////////////////////////////////////////////////////

#include "mssrc_rv_rtp.h"
#include <../rv_adapter/rv_api.h>
#include "mp_caster.h"
#include "mp_caster_config.h"

namespace xt_mp_caster
{
    mssrc_rv_rtp::mssrc_rv_rtp() : 
m_fifo(MSSRC_RV_RTP_FIFO_SIZE, MSSRC_RV_RTP_FIFO_OVERLAPPED), 
m_active(false),
m_sr_fifo(MSSRC_RV_RTP_SR_FIFO_SIZE, MSSRC_RV_RTP_SR_FIFO_OVERLAPPED),	
m_rr_fifo(MSSRC_RV_RTP_RR_FIFO_SIZE, MSSRC_RV_RTP_RR_FIFO_OVERLAPPED),
m_mp(0),
m_rtp_pool(0)
{
    set_object_id((EMP_OBJECT_ID)(EMP_OBJ_SSRC + RV_RTP_MSSRC));
    m_bReady = false;
}

mssrc_rv_rtp::~mssrc_rv_rtp()
{
    close();
}

void mssrc_rv_rtp::recycle_alloc_event()	
{
    m_active = false;
    m_bReady = false;
    m_mp = 0;
    m_rtp_pool = 0;
}
void mssrc_rv_rtp::recycle_release_event()
{
    close();
}

bool mssrc_rv_rtp::open(
                        mp* hmp,
                        inner::rtp_pool *rtp_pool,
                        rv_net_address local_address,
                        bool manual_rtcp,
                        bool bActive,
                        bool bMulticastRTP,
                        rv_net_address multicast_rtp_address,
                        bool bMulticastRTCP,
                        rv_net_address multicast_rtcp_address)								
{
    bool bRet = false;
    do 
    {
        if (m_bReady) break;
        if (!hmp) break;
        if (!rtp_pool) break;
        m_mp = hmp;
        m_rtp_pool = rtp_pool;
        //调用rv_adapter open_session
        ::construct_rv_handler(&m_hrv);
        rv_session_descriptor descriptor_session;
        ::construct_rv_session_descriptor(&descriptor_session);
        descriptor_session.onRtpRcvEvent = mssrc_rv_rtp::OnRtpReceiveEvent_AdapterFunc;
        //descriptor_session.onRtcpSndSREvent = mssrc_rv_rtp::OnRtcpSendHandler_AdapterFunc;
        //descriptor_session.onRtcpRcvSRRREvent = mssrc_rv_rtp::OnRtcpReceiveHandler_AdapterFunc;
        descriptor_session.onRtcpSndSREvent = 0;
        descriptor_session.onRtcpRcvSRRREvent = 0;
        memcpy(&descriptor_session.local_address, &local_address, sizeof(rv_net_address));
        descriptor_session.context = this; 
        descriptor_session.manual_rtcp = (manual_rtcp) ? RV_ADAPTER_TRUE : RV_ADAPTER_FALSE;
        descriptor_session.multicast_rtp_address_opt = (bMulticastRTP) ? RV_ADAPTER_TRUE : RV_ADAPTER_FALSE;
        memcpy(&descriptor_session.multicast_rtp_address, &multicast_rtp_address, sizeof(rv_net_address));
        descriptor_session.multicast_rtcp_address_opt = (bMulticastRTCP) ? RV_ADAPTER_TRUE : RV_ADAPTER_FALSE;
        memcpy(&descriptor_session.multicast_rtcp_address, &multicast_rtcp_address, sizeof(rv_net_address));	
        if (RV_ADAPTER_FALSE == ::open_session(&descriptor_session, &m_hrv)) 
        {
            m_mp = 0;
            m_rtp_pool = 0;
            break;
        }
        ::set_manual_rtcp(&m_hrv, (RV_ADAPTER_TRUE == descriptor_session.manual_rtcp));
        ::set_rtp_receive_buffer_size(&m_hrv, MSSRC_RV_RTP_RECEIVE_BUFFER_SIZE);

        m_bReady = true;
        m_active = bActive;
        bRet = true;
    } while (false);
    return bRet;
}
void mssrc_rv_rtp::close()							
{
    if (!m_bReady) return;
    //调用rv_adapter close_session
    ::close_session(&m_hrv);
    ::construct_rv_handler(&m_hrv);
    m_bReady = false;
    m_active = false;
    m_fifo.clear();
    m_sr_fifo.clear();
    m_rr_fifo.clear();
    m_mp = 0;
    m_rtp_pool = 0;
}
bool mssrc_rv_rtp::active(bool bActive)			
{
    if (!m_bReady) return false;
    m_active = bActive;
    if (!bActive) m_fifo.clear();
    return true;
}
bool mssrc_rv_rtp::pump_rtp_in(rtp_block *rtp)
{
    bool bRet = false;
    do 
    {
        if (!rtp) break;
        bRet = m_fifo.push(rtp);
    } while (false);
    return bRet;
}
rtp_block *mssrc_rv_rtp::pump_rtp_out()
{
    rtp_block *rtp = 0;
    do 
    {
        if (!m_bReady) break;
        if (!m_active) break;
        rtp = static_cast<rtp_block *>(m_fifo.pop(false));
    } while (false);
    return rtp;
}

uint32_t mssrc_rv_rtp::pump_rtps_out(std::list<rtp_block *> &resultBlocks, uint32_t canSendSize)
{
    uint32_t loadSize = 0;
    do 
    {
        if (!m_bReady) break;
        if (!m_active) break;
        rtp_block *rtp = static_cast<rtp_block *>(m_fifo.pop(false));
        while(rtp)
        {
            if (rtp->get_bind_block())
                loadSize += rtp->get_bind_block()->payload_totalsize();
            else
                loadSize += rtp->payload_totalsize();
            resultBlocks.push_back(rtp);
            if (loadSize >= canSendSize) break;
            rtp = static_cast<rtp_block *>(m_fifo.pop(false));
        }
    } while (false);
    return loadSize;
}

bool mssrc_rv_rtp::read_rtcp_sr(rtcp_send_report *sr)
{
    bool bRet = false;
    do 
    {
        if (!m_bReady) break;
        if (!sr || m_sr_fifo.size()<=0) break;
        rtcp_sr *sr_info = static_cast<rtcp_sr *>(m_sr_fifo.pop(false));
        if (!sr_info) break;
        sr_info->read_rtcp_send_report(sr);
        bRet = true;
    } while (false);
    return bRet;
}

bool mssrc_rv_rtp::read_rtcp_rr(rtcp_receive_report *rr)
{
    bool bRet = false;
    do 
    {
        if (!m_bReady) break;
        if (!rr || m_rr_fifo.size()<=0) break;
        rtcp_rr *rr_info = static_cast<rtcp_rr *>(m_rr_fifo.pop(false));
        if (!rr_info) break;
        rr_info->read_rtcp_receive_report(rr);
        bRet = true;
    } while (false);
    return bRet;
}

//静态回调函数区域
uint8_t dummy_mssrc_rv_rtp_buffer[MP_BLOCK_POOL_SIZE];
void mssrc_rv_rtp::OnRtpReceiveEvent_AdapterFunc(
    RV_IN rv_handler  hrv, 
    RV_IN rv_context context)
{
    rv_rtp_param p;
    ::construct_rv_rtp_param(&p);
    //读取数据，并推送至缓冲区
    bool bRcvSuccess = false;
    //防止ssrc被释放
    //caster::share_lock();

    do 
    {
        mssrc_rv_rtp * rv_ssrc = (mssrc_rv_rtp *)context;
        if (!rv_ssrc) break;
        if (!rv_ssrc->m_mp) break;
        if (!rv_ssrc->m_rtp_pool) break;
        rtp_block *rtpBlock = rv_ssrc->m_rtp_pool->try_alloc_any(false);
        if (!rtpBlock) break;
        bRcvSuccess = true;
        rtpBlock->assign();
        rv_net_address addr;
        ::memset(&addr, 0, sizeof(addr));
        if (RV_ADAPTER_TRUE == ::read_rtp(hrv, rtpBlock->get_raw(), rtpBlock->size(), &p,&addr))
        {
            rtpBlock->set_params(p.len - MP_PSEUDO_RTP_HEAD_SIZE, MP_PSEUDO_RTP_HEAD_SIZE);
            rtpBlock->set_rtp_param(&p);
            rv_ssrc->pump_rtp_in(rtpBlock);
            rtpBlock->release();
        }
        else
        {
            rtpBlock->release();
            break;
        }

        //产生任务调度过程
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        caster->m_engine->post_task(rv_ssrc->m_mp, ECASTER_TASK_MSSRC);
    } while (true);

    //caster::share_unlock();

    if (!bRcvSuccess) 
    {
        while(true)
        {
            rv_net_address addr;
            ::memset(&addr, 0, sizeof(addr));
            //拉空RV_ADAPTER的内部缓冲区
            if (RV_ADAPTER_FALSE == 
                ::read_rtp(hrv, dummy_mssrc_rv_rtp_buffer, sizeof(dummy_mssrc_rv_rtp_buffer), &p,&addr))
                break;
        }
        return;
    }
}
void mssrc_rv_rtp::OnRtcpSendHandler_AdapterFunc(
    RV_IN rv_handler hrv,
    RV_IN rv_context context,
    RV_IN uint32_t ssrc,
    RV_IN uint8_t *rtcpPack,
    RV_IN uint32_t size)
{
    //哑处理方式，仅保留回调形式
}
void mssrc_rv_rtp::OnRtcpReceiveHandler_AdapterFunc(
    RV_IN rv_handler hrv,
    RV_IN rv_context context,
    RV_IN uint32_t ssrc,
    RV_IN uint8_t *rtcpPack,
    RV_IN uint32_t size)
{
    //提取RTCP报告，更新RTCP报告记录
    rv_rtcp_info info, info_local;
    uint32_t localSSRC = ::get_rtcp_ssrc(hrv);
    ::get_rtcp_sourceinfo(hrv, localSSRC, &info_local);
    ::get_rtcp_sourceinfo(hrv, ssrc, &info);

    mssrc_rv_rtp * rv_ssrc = (mssrc_rv_rtp *)context;
    if (!rv_ssrc) return;
    //防止sink被释放
    //caster::share_lock();

    if (rv_ssrc->m_bReady)
    {
        if (info_local.sr.valid)
        {
            rtcp_sr *sr = 
                static_cast<rtcp_sr *>(caster::self()->m_objpools.forceAllocObject(EMP_OBJ_RTCP_SR));		
            if (sr) 
            {
                sr->assign();
                sr->write_srinfo(localSSRC, info_local.sr);
                rv_ssrc->m_sr_fifo.push(sr);
                sr->release();
            }
        }

        if (info.rrFrom.valid)
        {
            rtcp_rr *rr = 
                static_cast<rtcp_rr *>(caster::self()->m_objpools.forceAllocObject(EMP_OBJ_RTCP_RR));		
            if (rr) 
            {
                rr->assign();
                rr->write_rrinfo(ssrc, info.rrFrom);
                rv_ssrc->m_rr_fifo.push(rr);
                rr->release();
            }
        }
    }

    //caster::share_unlock();
}
}
