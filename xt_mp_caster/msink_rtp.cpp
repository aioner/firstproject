///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：msink_rtp.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月26日
// 内容描述：媒体数据宿 -- RTP包类型
///////////////////////////////////////////////////////////////////////////////////////////
#include "msink_rtp.h"
#include <../rv_adapter/rv_def.h>
#include <../rv_adapter/rv_api.h>

namespace xt_mp_caster
{
    msink_rtp::msink_rtp() : m_hrv(0)
#ifdef _USE_RTP_SEND_CONTROLLER
        ,m_bitrate_controller(NULL)
#endif
    {
        set_object_id((EMP_OBJECT_ID)(EMP_OBJ_SINK + RTP_MSINK));
        m_bReady = false;
    }
    msink_rtp::~msink_rtp()
    {

    }

    void msink_rtp::recycle_alloc_event()	
    {
        m_bReady = false;
        m_hrv = 0;
    }
    void msink_rtp::recycle_release_event()
    {
        close(m_hrv);
    }

    bool msink_rtp::open(rv_handler hrv, rtp_sink_descriptor *descriptor
#ifdef _USE_RTP_SEND_CONTROLLER
        ,bitrate_controller_t *bitrate_controller
#endif
        )
    {
        bool bRet = false;
        do 
        {
            if (m_bReady) break;
            if (!hrv) break;
            if (!descriptor) break;
            memcpy(&m_descriptor, descriptor, sizeof(rtp_sink_descriptor));

            //调用rv_adapter
            if (m_descriptor.multiplex)
            {
                ::add_rtp_mult_remote_address(hrv, &(m_descriptor.rtp_address), m_descriptor.multiplexID);
                if (m_descriptor.rtcp_opt)
                    ::add_rtcp_mult_remote_address(hrv, &(m_descriptor.rtcp_address), m_descriptor.multiplexID);
            }
            else
            {
                ::add_rtp_remote_address(hrv, &(m_descriptor.rtp_address));
                if (m_descriptor.rtcp_opt)
                    ::add_rtcp_remote_address(hrv, &(m_descriptor.rtcp_address));
            }

#ifdef _USE_RTP_SEND_CONTROLLER
            m_bitrate_controller = bitrate_controller;
            if (NULL != m_bitrate_controller)
            {
                m_bitrate_controller->add_sink(m_descriptor.rtcp_address);
            }
#endif

            m_bReady = true;
            m_hrv = hrv;
            bRet = true;
        } while (false);
        return bRet;
    }
    void msink_rtp::close(rv_handler hrv)							
    {
        if (!m_bReady) return;
        if (!hrv) return;
        //调用rv_adapter

        if (m_descriptor.multiplex)
        {
            if (m_descriptor.rtcp_opt)
                ::del_rtcp_mult_remote_address(hrv, &(m_descriptor.rtcp_address), m_descriptor.multiplexID);
            ::del_rtp_mult_remote_address(hrv, &(m_descriptor.rtp_address), m_descriptor.multiplexID);
        }
        else
        {
            if (m_descriptor.rtcp_opt)
                ::del_rtcp_remote_address(hrv, &(m_descriptor.rtcp_address));
            ::del_rtp_remote_address(hrv, &(m_descriptor.rtp_address));
        }

#ifdef _USE_RTP_SEND_CONTROLLER
        if (NULL != m_bitrate_controller)
        {
            m_bitrate_controller->del_sink(m_descriptor.rtcp_address);
        }
#endif

		m_bReady = false;
		m_hrv = 0;
	}
}
