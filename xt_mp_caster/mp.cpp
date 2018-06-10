///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：mp.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：媒体数据处理单元
///////////////////////////////////////////////////////////////////////////////////////////

#include "mp.h"
#include <../rv_adapter/rv_api.h>
#include "msink_memory.h"
#include "msink_rtp.h"
#include "msink_rv_rtp.h"
#include "mssrc_frame.h"
#include "mssrc_rtp.h"
#include "mssrc_rv_rtp.h"
#include "bc_mp.h"

namespace xt_mp_caster
{
    ///////////////////////////////////////////////////////////////////////////////////////////
    // mp_pools
    mp_object *mp_pools::forceAllocObject(EMP_OBJECT_ID object_id)
    {
        mp_object * object = 0;
        do 
        {
            if (!mp_object::validate(object_id)) break;
            tghelper::recycle_pool * pool = m_objpools.get_pools(object_id);
            if (!pool) 
            {
                m_objpools.add_pool(object_id);
                pool = m_objpools.get_pools(object_id);
            }
            object = static_cast<mp_object *>(pool->alloc_item());
            if (!object)
            {
                //强制分配
                //TODO::等待子类实现后完善此处
                switch (object_id)
                {
                case (EMP_OBJ_SSRC + FRAME_MSSRC):
                    object = static_cast<mp_object *>(
                        tghelper::recycle_pool_build_item<mssrc_frame>(pool, false));
                    break;
                case (EMP_OBJ_SSRC + RTP_MSSRC):
                    object = static_cast<mp_object *>(
                        tghelper::recycle_pool_build_item<mssrc_rtp>(pool, false));
                    break;
                case (EMP_OBJ_SSRC + RV_RTP_MSSRC):
                    object = static_cast<mp_object *>(
                        tghelper::recycle_pool_build_item<mssrc_rv_rtp>(pool, false));
                    break;
                case (EMP_OBJ_SINK + RTP_MSINK):
                    object = static_cast<mp_object *>(
                        tghelper::recycle_pool_build_item<msink_rtp>(pool, false));
                    break;
                case (EMP_OBJ_SINK + STREAM_RTP_MSINK):
                    object = 0; //尚未实现
                    break;
                case (EMP_OBJ_SINK + MEMORY_MSINK):
                    object = static_cast<mp_object *>(
                        tghelper::recycle_pool_build_item<msink_memory>(pool, false));
                    break;
                case (EMP_OBJ_SINK + RV_RTP_MSINK):
                    object = static_cast<mp_object *>(
                        tghelper::recycle_pool_build_item<msink_rv_rtp>(pool, false));
                    break;
                case (EMP_OBJ_MP + BROADCAST_MP):
                    object = static_cast<mp_object *>(
                        tghelper::recycle_pool_build_item<bc_mp>(pool, false));
                    break;
                case (EMP_OBJ_MP + PROXY_MP):
                    object = 0;
                    break;
                case (EMP_OBJ_MP + MIXER_MP):
                    object = 0;
                    break;
                case (EMP_OBJ_MP + TRANSLATOR_MP):
                    object = 0;
                    break;
                case EMP_OBJ_RTCP_SR:
                    object = static_cast<mp_object *>(
                        tghelper::recycle_pool_build_item<rtcp_sr>(pool, false));
                    break;
                case EMP_OBJ_RTCP_RR:
                    object = static_cast<mp_object *>(
                        tghelper::recycle_pool_build_item<rtcp_rr>(pool, false));
                    break;
                }
            }

        } while (false);
        return object;
    }

    void mp_pools::buildObjectPools()
    {
        m_objpools.add_pool(EMP_OBJ_SSRC + FRAME_MSSRC);
        m_objpools.add_pool(EMP_OBJ_SSRC + RTP_MSSRC);
        m_objpools.add_pool(EMP_OBJ_SSRC + RV_RTP_MSSRC);

        m_objpools.add_pool(EMP_OBJ_SINK + RTP_MSINK);
        m_objpools.add_pool(EMP_OBJ_SINK + STREAM_RTP_MSINK);
        m_objpools.add_pool(EMP_OBJ_SINK + MEMORY_MSINK);
        m_objpools.add_pool(EMP_OBJ_SINK + RV_RTP_MSINK);

        m_objpools.add_pool(EMP_OBJ_MP + BROADCAST_MP);
        m_objpools.add_pool(EMP_OBJ_MP + PROXY_MP);
        m_objpools.add_pool(EMP_OBJ_MP + MIXER_MP);
        m_objpools.add_pool(EMP_OBJ_MP + TRANSLATOR_MP);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // mp
    bool mp::add_mssrc(mssrc * hmssrc) 	
    { 
        bool bRet = false;
        do 
        {
            if (!hmssrc) break;
            bRet = true;
            hmssrc->assign();
            m_mssrcs.push_back(hmssrc);
        } while (false);
        return bRet;
    }
    bool mp::del_mssrc(mssrc * hmssrc)
    {
        bool bRet = false;
        do 
        {
            if (!hmssrc) break;
            bRet = true;
            m_mssrcs.remove(hmssrc);
            hmssrc->release();
        } while (false);
        return bRet;
    }
    void mp::clear_mssrc()
    {
        std::list<mssrc *>::iterator it;
        for (it = m_mssrcs.begin(); it != m_mssrcs.end(); it++)
        {
            (*it)->release();
        }
        m_mssrcs.clear();
    }

    bool mp::add_msink(msink * hmsink)
    {
        bool bRet = false;
        do 
        {
            if (!hmsink) break;
            bRet = true;
            hmsink->assign();
            m_msinks.push_back(hmsink);
        } while (false);
        return bRet;
    }
    bool mp::del_msink(msink * hmsink)
    {
        bool bRet = false;
        do 
        {
            if (!hmsink) break;
            bRet = true;
            m_msinks.remove(hmsink);
            hmsink->release();
        } while (false);
        return bRet;
    }
    void mp::clear_msink()
    {
        std::list<msink *>::iterator it;
        msink_rv_rtp* sink = (msink_rv_rtp*)m_msinks.front();
        if (sink)
        {
            sink->close();
        }
        
        for (it = m_msinks.begin(); it != m_msinks.end(); it++)
        {
            (*it)->release();
        }
        m_msinks.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // rtcp_sr
    void rtcp_sr::recycle_alloc_event()
    {
        //清空包头描述
        memset(&m_srinfo, 0, sizeof(rv_rtcp_srinfo));
    }

    void rtcp_sr::write_srinfo(uint32_t ssrc, const rv_rtcp_srinfo &srinfo)
    {
        memcpy(&m_srinfo, &srinfo, sizeof(rv_rtcp_srinfo));
        m_ssrc = ssrc;
    }
    void rtcp_sr::read_srinfo(uint32_t &ssrc, rv_rtcp_srinfo &srinfo)
    {
        memcpy(&srinfo, &m_srinfo, sizeof(rv_rtcp_srinfo));
        ssrc = m_ssrc;
    }
    void rtcp_sr::read_rtcp_send_report(rtcp_send_report &sr)
    {
        sr.ssrc = m_ssrc;
        sr.mNTPtimestamp = m_srinfo.mNTPtimestamp;
        sr.lNTPtimestamp = m_srinfo.lNTPtimestamp;
        sr.timestamp = m_srinfo.timestamp;
        sr.packets = m_srinfo.packets;
        sr.octets = m_srinfo.octets;
    }
    void rtcp_sr::read_rtcp_send_report(rtcp_send_report *sr)
    {
        if (!sr) return;
        sr->ssrc = m_ssrc;
        sr->mNTPtimestamp = m_srinfo.mNTPtimestamp;
        sr->lNTPtimestamp = m_srinfo.lNTPtimestamp;
        sr->timestamp = m_srinfo.timestamp;
        sr->packets = m_srinfo.packets;
        sr->octets = m_srinfo.octets;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // rtcp_rr
    void rtcp_rr::recycle_alloc_event()
    {
        //清空包头描述
        memset(&m_rrinfo, 0, sizeof(rv_rtcp_rrinfo));
    }

    void rtcp_rr::write_rrinfo(uint32_t ssrc, const rv_rtcp_rrinfo &rrinfo)
    {
        memcpy(&m_rrinfo, &rrinfo, sizeof(rv_rtcp_rrinfo));
        m_ssrc = ssrc;
    }
    void rtcp_rr::read_rrinfo(uint32_t &ssrc, rv_rtcp_rrinfo &rrinfo)
    {
        memcpy(&rrinfo, &m_rrinfo, sizeof(rv_rtcp_rrinfo));
        ssrc = m_ssrc;
    }
    void rtcp_rr::read_rtcp_receive_report(rtcp_receive_report &rr)
    {
        rr.ssrc = m_ssrc;
        rr.fractionLost = m_rrinfo.fractionLost;
        rr.cumulativeLost = m_rrinfo.cumulativeLost;
        rr.sequenceNumber = m_rrinfo.sequenceNumber;
        rr.jitter = m_rrinfo.jitter;
        rr.lSR = m_rrinfo.lSR;
        rr.dlSR = m_rrinfo.dlSR;
    }
    void rtcp_rr::read_rtcp_receive_report(rtcp_receive_report *rr)
    {
        if (!rr) return;
        rr->ssrc = m_ssrc;
        rr->fractionLost = m_rrinfo.fractionLost;
        rr->cumulativeLost = m_rrinfo.cumulativeLost;
        rr->sequenceNumber = m_rrinfo.sequenceNumber;
        rr->jitter = m_rrinfo.jitter;
        rr->lSR = m_rrinfo.lSR;
        rr->dlSR = m_rrinfo.dlSR;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////
    // misc

}