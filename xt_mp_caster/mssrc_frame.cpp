///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：mssrc_frame.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月26日
// 内容描述：媒体数据源 -- 帧类型
///////////////////////////////////////////////////////////////////////////////////////////

#include "mssrc_frame.h"

namespace xt_mp_caster
{
    mssrc_frame::mssrc_frame() : 
m_fifo(MSSRC_FRAME_FIFO_SIZE, MSSRC_FRAME_FIFO_OVERLAPPED), 
m_active(false)
{
    set_object_id((EMP_OBJECT_ID)(EMP_OBJ_SSRC + FRAME_MSSRC));
    m_bReady = false;
}
mssrc_frame::~mssrc_frame()
{
    close();
}

void mssrc_frame::recycle_alloc_event()	
{
    m_active = false;
    m_bReady = false;
}
void mssrc_frame::recycle_release_event()
{
    close();
}

bool mssrc_frame::open(bool bActive)
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
void mssrc_frame::close()
{
    if (!m_bReady) return;
    m_bReady = false;
    m_active = false;
    m_fifo.clear();
}
bool mssrc_frame::active(bool bActive)			
{
    if (!m_bReady) return false;
    m_active = bActive;
    if (!bActive) m_fifo.clear();
    return true;
}
bool mssrc_frame::pump_frame_in(rtp_mblock *mrtp)
{
    bool bRet = false;
    do 
    {
        if (!mrtp) break;
        if (!m_bReady) break;
        if (!m_active) break;
        bRet = m_fifo.push(mrtp);
    } while (false);
    return bRet;
}

rtp_mblock *mssrc_frame::pump_frame_out()
{
    rtp_mblock *mrtp = 0;
    do 
    {
        if (!m_bReady) break;
        if (!m_active) break;
        mrtp = static_cast<rtp_mblock *>(m_fifo.pop(false));
    } while (false);
    return mrtp;
}

uint32_t mssrc_frame::pump_frames_out(msink_rv_rtp * hmsink, rtp_mblock *mrtp_in,uint32_t canSendSize)
{
	uint32_t loadSize = 0;
	do 
	{
		if (!m_bReady) break;
		if (!m_active) break;
#ifdef USE_POST_TASK
		rtp_mblock *mrtp = static_cast<rtp_mblock *>(m_fifo.pop(false));
#else
		rtp_mblock *mrtp = mrtp_in;
#endif
		while(mrtp)
		{
			uint32_t mrtp_size = mrtp->size() * MP_PSEUDO_RTP_HEAD_SIZE + mrtp->payload_size();
			loadSize += mrtp_size;
			hmsink->pump_mrtp_out(mrtp);
			(mrtp)->release();
			if (loadSize >= canSendSize) break;
#ifdef USE_POST_TASK
			mrtp = static_cast<rtp_mblock *>(m_fifo.pop(false));
#else
			break;
#endif
		}

	} while (false);
	return loadSize;
}
}
