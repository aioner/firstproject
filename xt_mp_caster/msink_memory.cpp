///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：msink_memory.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月26日
// 内容描述：媒体数据宿 -- 内存接口型
///////////////////////////////////////////////////////////////////////////////////////////

#include "msink_memory.h"

namespace xt_mp_caster
{
    msink_memory::msink_memory() : 
m_fifo(MSINK_MEMORY_FIFO_SIZE, MSINK_MEMORY_FIFO_OVERLAPPED), 
m_active(false)
{
    set_object_id((EMP_OBJECT_ID)(EMP_OBJ_SINK + MEMORY_MSINK));
    m_bReady = false;
}
msink_memory::~msink_memory()
{
    close();
}

void msink_memory::recycle_alloc_event()	
{
    m_active = false;
    m_bReady = false;
}
void msink_memory::recycle_release_event()
{
    close();
}

bool msink_memory::open(memory_sink_descriptor *descriptor, bool bActive)								
{
    bool bRet = false;
    do 
    {
        if (m_bReady) break;
        memcpy(&m_descriptor, descriptor, sizeof(memory_sink_descriptor));
        m_bReady = true;
        m_active = bActive;
        bRet = true;
    } while (false);
    return bRet;
}
void msink_memory::close()							
{
    if (!m_bReady) return;
    m_bReady = false;
    m_active = false;
    m_fifo.clear();
    memset(&m_descriptor, 0, sizeof(memory_sink_descriptor));
}
bool msink_memory::active(bool bActive)			
{
    if (!m_bReady) return false;
    m_active = bActive;
    if (!bActive) m_fifo.clear();
    return true;
}
bool msink_memory::pump_rtp_in(rtp_block *rtp)
{
    bool bRet = false;
    do 
    {
        if (!rtp) break;
        if (!m_bReady) break;
        if (!m_active) break;
        bRet = m_fifo.push(rtp);
    } while (false);
    return bRet;
}

void msink_memory::pump_rtp_out(mp * hmp)
{
    mp_h_s hmp_s;
    hmp_s.hmp = hmp;
    msink_h_s hmsink_s;
    hmsink_s.hmsink = this;
    rtp_block * rtp = 0;
    do 
    {
        if (!m_bReady) break;
        if (!m_active) break;
        if (!m_descriptor.onMemorySinkEvent && !m_descriptor.onMemorySinkExEvent) break;
        while(true)
        {
            rtp = static_cast<rtp_block *>(m_fifo.pop(false));
            if (!rtp) break;
            if (m_descriptor.onMemorySinkExEvent)
            {
                rtp->assign();

                (*m_descriptor.onMemorySinkExEvent)(&hmp_s, &hmsink_s, m_descriptor.context, rtp);
            }
            else if (m_descriptor.onMemorySinkEvent)
            {
                (*m_descriptor.onMemorySinkEvent)(&hmp_s, &hmsink_s, m_descriptor.context, 
                    rtp->get_raw(), rtp->payload_totalsize());
            }
            rtp->release();
        }

    } while (false);
}
}