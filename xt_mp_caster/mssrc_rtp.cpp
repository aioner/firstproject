///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：mssrc_rtp.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月26日
// 内容描述：媒体数据源 -- rtp包类型
///////////////////////////////////////////////////////////////////////////////////////////

#include "mssrc_rtp.h"

namespace xt_mp_caster
{
    mssrc_rtp::mssrc_rtp() : 
m_fifo(MSSRC_RTP_FIFO_SIZE, MSSRC_RTP_FIFO_OVERLAPPED), 
m_active(false)
{
    set_object_id((EMP_OBJECT_ID)(EMP_OBJ_SSRC + RTP_MSSRC));
    m_bReady = false;
}
mssrc_rtp::~mssrc_rtp()
{
    close();
}

void mssrc_rtp::recycle_alloc_event()	
{
    m_active = false;
    m_bReady = false;
}
void mssrc_rtp::recycle_release_event()
{
    close();
}

bool mssrc_rtp::open(bool bActive)								
{
    bool bRet = false;
    do 
    {
        if (m_bReady) break;
        m_bReady = true;
        m_active = bActive;
        bRet = true;
    } while (false);
    return bRet;
}
void mssrc_rtp::close()							
{
    if (!m_bReady) return;
    m_bReady = false;
    m_active = false;
    m_fifo.clear();
}
bool mssrc_rtp::active(bool bActive)			
{
    if (!m_bReady) return false;
    m_active = bActive;
    if (!bActive) m_fifo.clear();
    return true;
}
bool mssrc_rtp::pump_rtp_in(rtp_block *rtp)
{
    bool bRet = false;
    do 
    {
        if (!rtp) break;
        bRet = m_fifo.push(rtp);
    } while (false);
    return bRet;
}
rtp_block *mssrc_rtp::pump_rtp_out()
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

uint32_t mssrc_rtp::pump_rtps_out(msink_rv_rtp * hmsink, uint32_t canSendSize)
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
            //resultBlocks.push_back(rtp);
			hmsink->pump_rtp_in(rtp);	//推送失败只能丢弃
			(rtp)->release();
            if (loadSize >= canSendSize) break;
            rtp = static_cast<rtp_block *>(m_fifo.pop(false));
        }
    } while (false);
    return loadSize;
}
}
