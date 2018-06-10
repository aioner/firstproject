///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：msink_rv_rtp.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月26日
// 内容描述：媒体数据宿 -- rv adapter绑定型 RTP包
///////////////////////////////////////////////////////////////////////////////////////////

#include "msink_rv_rtp.h"
#include <../rv_adapter/rv_api.h>
#include "mp_caster.h"
#include "mp_caster_config.h"
#include "XTDemuxMan.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "bc_mp.h"

#ifdef _ANDROID
#include "xt_config_cxx.h"
#include <android/log.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif//#ifndef MAX_PATH

extern void writeLogV(int nLogLevel, const char* szLogName,const char* szLogFmt, va_list vArgList);
extern void writeLog(int nLevel, const char* szLogName, const char* szLogFmt, ...);

std::map<rv_rtp,msink_rv_rtp*> g_mapRtp;

namespace xt_mp_caster
{
msink_rv_rtp::msink_rv_rtp() :
m_rtp_pool(),
m_fifo(MSINK_RV_RTP_FIFO_SIZE, MSINK_RV_RTP_FIFO_OVERLAPPED),
m_active(false),
m_viewer_nums(0),
m_nReSend(1),
m_pSink_RtcpCB(NULL),
m_pAppMsgCB(NULL),
m_pRtcpRawCB(NULL),
m_sr_fifo(MSINK_RV_RTP_SR_FIFO_SIZE, MSINK_RV_RTP_SR_FIFO_OVERLAPPED),
m_rr_fifo(MSINK_RV_RTP_RR_FIFO_SIZE, MSINK_RV_RTP_RR_FIFO_OVERLAPPED),
m_multiplex(MP_FALSE),
m_multiplexID(0)
#ifdef _USE_RTP_TRAFFIC_SHAPING
,m_traffic_shaping()
#endif
#ifdef _USE_RTP_SEND_CONTROLLER
,m_rtcp_observer()
#endif //_USE_RTP_SEND_CONTROLLER
{
    set_object_id((EMP_OBJECT_ID)(EMP_OBJ_SINK + RV_RTP_MSINK));
    m_bReady = false;
	m_last_frame_in_rtp_sn = 0;
}

msink_rv_rtp::~msink_rv_rtp()
{
    close();
}

void msink_rv_rtp::recycle_alloc_event()
{
    m_active = false;
    m_bReady = false;
}

void msink_rv_rtp::recycle_release_event()
{
    close();
}

bool msink_rv_rtp::open(rv_net_address local_address,
                        bool manual_rtcp,
                        bool bActive,
                        uint8_t multicast_rtp_ttl,
                        uint8_t multicast_rtcp_ttl,
                        mp_bool multiplex,
                        uint32_t *multID
#ifdef _USE_RTP_SEND_CONTROLLER
                        ,rtcp_observer_t *rtcp_observer
#endif
                        )
{
    m_file_path = "";

#ifdef _ANDROID
    m_nReSend = xt_config::router_module::get<int>("config.caster_cfg.resend",1);
    m_open_pri = xt_config::router_module::get<int>("config.caster_cfg.open_pri",MSINK_RV_RTP_TRANSMIT_BUFFER_SIZE);
    int sndbuf = xt_config::router_module::get<int>("config.caster_cfg.sndbuf",MSINK_RV_RTP_TRANSMIT_BUFFER_SIZE);
#else
    m_nReSend = config::_()->resend(1);
    m_open_pri = config::_()->pri_pkt(0);
    int sndbuf = config::_()->sndbuf(MSINK_RV_RTP_TRANSMIT_BUFFER_SIZE);
#endif

    m_sr_fifo.clear();
    m_rr_fifo.clear();

    bool bRet = false;
    do
    {
        if (m_bReady) break;
        //调用rv_adapter open_session
        ::construct_rv_handler(&m_hrv);
        rv_session_descriptor descriptor_session;
        ::construct_rv_session_descriptor(&descriptor_session);
        descriptor_session.onRtpRcvEvent = msink_rv_rtp::OnRtpReceiveEvent_AdapterFunc;
        descriptor_session.onRtcpSndSREvent = msink_rv_rtp::OnRtcpSendHandler_AdapterFunc;
        descriptor_session.onRtcpRcvSRRREvent = msink_rv_rtp::OnRtcpReceiveHandler_AdapterFunc;
        memcpy(&descriptor_session.local_address, &local_address, sizeof(rv_net_address));
        descriptor_session.context = this;
        descriptor_session.user_context = NULL;
        descriptor_session.manual_rtcp = (manual_rtcp) ? RV_ADAPTER_TRUE : RV_ADAPTER_FALSE;
        descriptor_session.multicast_rtp_ttl = multicast_rtp_ttl;
        descriptor_session.multicast_rtcp_ttl = multicast_rtcp_ttl;

        m_multiplex = multiplex;

        if (multiplex)
        {
            bool ret = XTDemuxMan::instance()->open_session(&descriptor_session, &m_multiplexID, &m_hrv);
            if (!ret)
            {
                break;
            }

            if (multID)
            {
                *multID = m_multiplexID;
            }

            g_mapRtp[m_hrv.hrtp] = this;
            setdemux_caster_handler((void*)RtpDemuxEventHandler);
        }
        else
        {
            if (RV_ADAPTER_FALSE == ::open_session(&descriptor_session, &m_hrv))
            {
                break;
            }
        }

        ::set_manual_rtcp(&m_hrv, (RV_ADAPTER_TRUE == descriptor_session.manual_rtcp));

        rv_bool r = false;
        do 
        {
            r = ::set_rtp_transmit_buffer_size(&m_hrv, sndbuf);
            if (!r)
            {
                sndbuf -= 1024*1024;
            }
        } while (!r && sndbuf >0);

        msink_rv_rtp *sink = this;

        RtcpSetAppEventHandler(&m_hrv, msink_rv_rtp::OnRtcpAppEventHandler_CB);
        rtcp_set_raw_eventhandler(&m_hrv, msink_rv_rtp::OnRtcpRawBufferReceived_CB);

        m_manReSend.setSink(sink);

#ifdef _USE_RTP_TRAFFIC_SHAPING
        if (caster::self()->use_traffic_shapping())
        {
#ifdef _ANDROID
            uint32_t max_flow_num = xt_config::router_module::get<uint32_t>("config.caster_cfg.traffic_shaping.max_flow_num", 1024);
            __android_log_print(ANDROID_LOG_ERROR, "xt_config", "config.caster_cfg.traffic_shaping.max_flow_num(%u)", max_flow_num);
            uint16_t num_of_windows = xt_config::router_module::get<uint16_t>("config.caster_cfg.traffic_shaping.num_of_windows", 7);
            __android_log_print(ANDROID_LOG_ERROR, "xt_config", "config.caster_cfg.traffic_shaping.num_of_windows(%u)", num_of_windows);
            uint16_t calc_ms_priod = xt_config::router_module::get<uint16_t>("config.caster_cfg.traffic_shaping.calc_ms_priod", 500);
            __android_log_print(ANDROID_LOG_ERROR, "xt_config", "config.caster_cfg.traffic_shaping.calc_ms_priod(%u)", calc_ms_priod);
#else
            uint32_t max_flow_num = 1024;
            uint16_t num_of_windows = 7;
            uint16_t calc_ms_priod = 500;
#endif 

            m_traffic_shaping.reset(new traffic_shaping_wrapper_t(1024U * 1024 * 1024, max_flow_num, num_of_windows, calc_ms_priod));
            m_traffic_shaping->register_flow_out_callback(this);
        }
#endif

#ifdef _USE_RTP_SEND_CONTROLLER
        m_rtcp_observer.reset(rtcp_observer);
#endif

        m_bReady = true;
        m_active = bActive;
		m_last_frame_in_rtp_sn = 0;
        bRet = true;
    } while (false);

    return bRet;
}

void msink_rv_rtp::close()
{
    g_mapRtp.erase(m_hrv.hrtp);
    //zzx 20140402
    //////////////////////////////////////////////////////////////////////////
    ::memset(&m_iPkt, 0, sizeof(PktInfo));
    //////////////////////////////////////////////////////////////////////////

    m_manReSend.clrSeg();
    m_manReSend.setSink(NULL);

    if (!m_bReady) return;

    if (m_multiplex)
    {
        XTDemuxMan::instance()->close_session(&m_hrv);
        ::construct_rv_handler(&m_hrv);
    }
    else
    {
        ::close_session(&m_hrv);
        ::construct_rv_handler(&m_hrv);
    }

    m_fifo.clear();
    m_sr_fifo.clear();
    m_rr_fifo.clear();

    m_bReady = false;
    m_active = false;
	m_last_frame_in_rtp_sn = 0;
}

bool msink_rv_rtp::active(bool bActive)
{
    if (!m_bReady) return false;
    m_active = bActive;
    if (!bActive)
    {
        m_fifo.clear();
        m_sr_fifo.clear();
        m_rr_fifo.clear();

        m_nSID = -1;

        m_manReSend.clrSeg();
        m_manReSend.setSink(NULL);
    }

    //zzx 20140402
    //////////////////////////////////////////////////////////////////////////
    ::memset(&m_iPkt, 0, sizeof(PktInfo));
    //////////////////////////////////////////////////////////////////////////

    return true;
}

bool msink_rv_rtp::pump_rtp_in(rtp_block *rtp)
{
    bool bRet = false;
    do
    {
#if (MP_CASTER_PARAM_CHECK)
        if (!rtp) break;
#endif
        if (!m_bReady) break;
        if (!m_active) break;
        //RTP包头信息提取
        tghelper::byte_block * bind_block = rtp->get_bind_block();
        if (!bind_block)
            ::rv_rtp_unpack(&m_hrv, rtp->get_raw(), rtp->payload_totalsize(), &(rtp->m_rtp_param));
        else
            ::rv_rtp_unpack(&m_hrv, bind_block->get_raw(), bind_block->payload_totalsize(), &(rtp->m_rtp_param));
        bRet = m_fifo.push(rtp);
    } while (false);
    return bRet;
}

bool msink_rv_rtp::pump_mrtp_in(rtp_mblock *mrtp)
{
    bool bRet = false;
    do
    {
#if (MP_CASTER_PARAM_CHECK)
        if (!mrtp) break;
#endif
        if (!m_bReady) break;
        if (!m_active) break;
        bRet = true;
        uint16_t frame_sn = mrtp->m_rtp_param.sequenceNumber;
        rtp_block * rtp = static_cast<rtp_block *>(mrtp->pop_byte_block());
        if (rtp && mrtp->m_bFrameInfo)
        {
            rtp->m_bFrameInfo = true;
            ::memcpy(&rtp->m_infoFrame, &mrtp->m_infoFrame, sizeof(XTFrameInfo));
        }

        rtp->m_priority = mrtp->m_priority;
        rtp->m_use_ssrc = mrtp->m_use_ssrc;
        rtp->m_ssrc = mrtp->m_ssrc;
        while (rtp)
        {
            rtp->set_rtp_param(&mrtp->m_rtp_param);
            rtp->ser_rtp_prority(mrtp->m_priority);
            rtp->m_rtp_param.sequenceNumber = frame_sn;
            frame_sn++;

            if (0 == mrtp->size())
            {
                //最后一个包, RTP包头mark标记为1
                rtp->m_rtp_param.marker = RV_ADAPTER_TRUE;
            }
            if (!m_fifo.push(rtp))
            {
                //缓冲区容量不足
                rtp->release();
                bRet = false;
                break;
            }

            rtp->release();
            rtp = static_cast<rtp_block *>(mrtp->pop_byte_block());
        }

    } while (false);
    return bRet;
}

void msink_rv_rtp::pump_mrtp_out(rtp_mblock *mrtp)
{
	do
	{
#if (MP_CASTER_PARAM_CHECK)
		if (!mrtp) break;
#endif
		if (!m_bReady) break;
		if (!m_active) break;
		uint16_t frame_sn = m_last_frame_in_rtp_sn/*mrtp->m_rtp_param.sequenceNumber*/;
		m_last_frame_in_rtp_sn += mrtp->get_container().size();
		rtp_block * rtp = static_cast<rtp_block *>(mrtp->pop_byte_block());
		if (rtp && mrtp->m_bFrameInfo)
		{
			rtp->m_bFrameInfo = true;
			::memcpy(&rtp->m_infoFrame, &mrtp->m_infoFrame, sizeof(XTFrameInfo));
		}

		rtp->m_priority = mrtp->m_priority;
		rtp->m_use_ssrc = mrtp->m_use_ssrc;
		rtp->m_ssrc = mrtp->m_ssrc;
		while (rtp)
		{
			rtp->set_rtp_param(&mrtp->m_rtp_param);
			rtp->ser_rtp_prority(mrtp->m_priority);
			rtp->m_rtp_param.sequenceNumber = frame_sn;
			frame_sn++;

			if (0 == mrtp->size())
			{
				//最后一个包, RTP包头mark标记为1
				rtp->m_rtp_param.marker = RV_ADAPTER_TRUE;
			}

			write_to_rv_adapter(rtp);

			if (m_nReSend <= 0)
			{
				rtp->release();
			}

			rtp->release();
			rtp = static_cast<rtp_block *>(mrtp->pop_byte_block());
		}

	} while (false);
}
void msink_rv_rtp::pump_rtp_out()
{
    //RTP数据写出至rv_adapter
    if (!m_bReady) return;
    if (!m_active)
    {
        //暂停模式下，缓冲区清空
        m_fifo.clear();
        return;
    }

    rtp_block * rtp = static_cast<rtp_block *>(m_fifo.pop(false));
    while(rtp)
    {
        write_to_rv_adapter(rtp);

        if (m_nReSend <= 0)
        {
            rtp->release();
        }

        rtp = static_cast<rtp_block *>(m_fifo.pop(false));

        //boost::this_thread::sleep(boost::posix_time::millisec(1));
    }
}

void msink_rv_rtp::write_to_rv_adapter(rtp_block *rtp, bool bReSend/* = false*/)
{
#if (MP_CASTER_PARAM_CHECK)
    if (!rtp) return;
#endif

    rtp->m_resend = bReSend;//该标记的意思是如果调用该接口发送重传包，那么不重复加入缓冲队列

#ifdef _USE_RTP_TRAFFIC_SHAPING
    if (NULL != m_traffic_shaping.get())
    {
        rtp->assign();
        m_traffic_shaping->flow_in(rtp);
    }
    else
    {
        internel_write_to_rv_adapter(rtp);
    }
#else
    internel_write_to_rv_adapter(rtp);
#endif
}

bool msink_rv_rtp::read_rtcp_sr(rtcp_send_report *sr)
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

void msink_rv_rtp::RtpDemuxEventHandler(void*   hDemux,void*  context)
{
    char buf[2048];
    uint32_t buf_len = 2048;
    rv_rtp rtpH;
    rv_rtp_param p;
    rv_net_address address;

    bool ret = false;
    try
    {
        ret = read_demux_rtp(hDemux, buf, buf_len, &rtpH, NULL, &p, &address);
    }
    catch (...)
    {
        ret = false;
    }

    if (g_mapRtp.find(rtpH) == g_mapRtp.end())
    {
        return;
    }

    msink_rv_rtp *self = (msink_rv_rtp*)g_mapRtp[rtpH];
    if (self && xt_mp_caster::bc_mp::m_raddr_cb)
    {
        xt_mp_caster::bc_mp::m_raddr_cb(self->m_mp, &address);
    }
}

bool msink_rv_rtp::read_rtcp_rr(rtcp_receive_report *rr)
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
uint8_t dummy_msink_rv_rtp_buffer[MP_BLOCK_POOL_SIZE];

void msink_rv_rtp::OnRtpReceiveEvent_AdapterFunc(
    RV_IN rv_handler  hrv,
    RV_IN rv_context context)
{   
    rv_rtp_param p;
    rv_net_address addr;
    ::memset(&p, 0, sizeof(p));
    rv_bool ret = ::read_rtp(hrv, dummy_msink_rv_rtp_buffer, sizeof(dummy_msink_rv_rtp_buffer), &p,&addr);

    if (!ret)
    {
        return;
    }

    msink_rv_rtp * sink = (msink_rv_rtp *)context;
    if (sink && xt_mp_caster::bc_mp::m_raddr_cb)
    {
        xt_mp_caster::bc_mp::m_raddr_cb(sink->m_mp,&addr);
    }
    {
        return;
    }
}

void msink_rv_rtp::OnRtcpSendHandler_AdapterFunc(
    RV_IN rv_handler hrv,
    RV_IN rv_context context,
    RV_IN uint32_t ssrc,
    RV_IN uint8_t *rtcpPack,
    RV_IN uint32_t size)
{
}

void msink_rv_rtp::OnRtcpReceiveHandler_AdapterFunc(
    RV_IN rv_handler hrv,
    RV_IN rv_context context,
    RV_IN uint32_t ssrc,
    RV_IN uint8_t *rtcpPack,
    RV_IN uint32_t size,
	RV_IN uint8_t *ip,
	RV_IN uint16_t port,
	RV_IN rv_bool multiplex,
	RV_IN uint32_t multid)
{
    //提取RTCP报告，更新RTCP报告记录
    rv_rtcp_info info, info_local;
    ::memset(&info, 0, sizeof(info));
    ::memset(&info_local, 0, sizeof(info_local));

    uint32_t localSSRC = ::get_rtcp_ssrc(hrv);
    ::get_rtcp_sourceinfo(hrv, localSSRC, &info_local);
    ::get_rtcp_sourceinfo(hrv, ssrc, &info);

    msink_rv_rtp * sink = (msink_rv_rtp *)context;
    if (!sink) return;

    //防止sink被释放
    caster::share_lock();

    if (sink->m_bReady && sink->isAcitve())
    {
        // 抛出RTCP
        //////////////////////////////////////////////////////////////////////////
        if (sink->m_pSink_RtcpCB)
        {
            sink->m_pSink_RtcpCB(ssrc, info_local, info,ip,port,multiplex,multid);
        }
        //////////////////////////////////////////////////////////////////////////

        if (info_local.sr.valid)
        {
            rtcp_sr *sr =
                static_cast<rtcp_sr *>(caster::self()->m_objpools.forceAllocObject(EMP_OBJ_RTCP_SR));
            if (sr)
            {
                sr->assign();
                sr->write_srinfo(localSSRC, info_local.sr);
                sink->m_sr_fifo.push(sr);
                sr->release();
            }
        }

        if (info.rrFrom.valid)
        {
#ifdef _USE_RTP_SEND_CONTROLLER
            if (NULL != sink->m_rtcp_observer.get() && (0 != info.rrFrom.sequenceNumber) && (0 != info.rrFrom.lSR) && (0 != info.rrFrom.dlSR))
            {
                rv_net_address address;
                construct_rv_net_address(&address);
                if (RV_ADAPTER_TRUE == get_rtcp_ssrc_remote(hrv, ssrc, &address))
                {
                    sink->m_rtcp_observer->on_rtcp_receive(localSSRC, ssrc, info.rrFrom, info.rtt, address);
                }
            }
#endif  //_USE_RTP_SEND_CONTROLLER

            rtcp_rr *rr =
                static_cast<rtcp_rr *>(caster::self()->m_objpools.forceAllocObject(EMP_OBJ_RTCP_RR));
            if (rr)
            {
                rr->assign();
                rr->write_rrinfo(ssrc, info.rrFrom);
                sink->m_rr_fifo.push(rr);
                rr->release();
            }
            writeLog(0,"caster_rr","sequenceNumber[%u] fractionLost[%u] cumulativeLost[%u] jitter[%u] dlSR[%u] lSR[%u] ssrc[%u]",
                info.rrFrom.sequenceNumber,info.rrFrom.fractionLost,
                info.rrFrom.cumulativeLost,info.rrFrom.jitter,info.rrFrom.dlSR,info.rrFrom.lSR,ssrc);
        }
    }

    caster::share_unlock();
}

rv_bool msink_rv_rtp::OnRtcpAppEventHandler_CB(
    RV_IN  rv_handler	 hrv,
    RV_IN  rv_context	 context,
    RV_IN  uint8_t        subtype,
    RV_IN  uint32_t       ssrc,
    RV_IN  uint8_t*       name,
    RV_IN  uint8_t*       userData,
    RV_IN  uint32_t       userDataLen)
{  
    msink_rv_rtp * sink = (msink_rv_rtp *)context;
    if (!sink)
    {
        return false;
    }

    //防止sink被释放
    caster::share_lock();

    switch (subtype)
    {
    case 1://NACK
        {
            if (sink->m_nReSend>0 && sink->m_active && userData && userDataLen>=4)
            {
                uint32_t sn;
                ::memcpy(&sn, userData, sizeof(uint32_t));
                writeLog(0,"caster_app","subtype[%d] ssrc[%x] name[%d] sn[%d]", subtype, ssrc, *name, sn);

                sink->m_manReSend.reSend(sn);
            }
            break;
        }
    case 2://DAR
        {
            if (sink->m_pAppMsgCB)
            {
                sink->m_pAppMsgCB(sink->m_nSID, subtype, ssrc, name, userData, userDataLen);
            }

            break;
        }
    case 3://NACKS
        {
            if (sink->m_nReSend>0 && sink->m_active && userData && userDataLen>=4)
            {
                uint32_t snsize= userDataLen;

                uint32_t *p = reinterpret_cast<uint32_t *>(userData);
                for (uint32_t i=0;i< snsize; i++)
                {
                    uint32_t sn = p[i];
                    sink->m_manReSend.reSend(sn);
                }
            }
        }
    default:
        {
            break;
        }
    }

    caster::share_unlock();

    return true;
}

rv_bool msink_rv_rtp::OnRtcpRawBufferReceived_CB(
    RV_IN  rv_handler	 hrv,
    RV_IN  rv_context	 context,
    RV_IN uint8_t *buffer,
    RV_IN uint32_t buffLen,
    RV_OUT rv_net_address *remoteAddress,
    RV_OUT rv_bool *pbDiscardBuffer)
{  
    msink_rv_rtp * sink = (msink_rv_rtp *)context;
    if (!sink)
    {
        return false;
    }

    //防止sink被释放
    caster::share_lock();

    if (sink->m_pRtcpRawCB)
    {
        sink->m_pRtcpRawCB(sink, buffer, buffLen, remoteAddress, pbDiscardBuffer);
    }

    caster::share_unlock();

    return true;
}

void msink_rv_rtp::set_file_path(const char *file)
{
    if (!file)
    {
        m_file_path = "";
    }
    else
    {
        m_file_path = file;
    }
}

uint32_t bitfieldSet(
                     uint32_t    value,
                     uint32_t    bitfield,
                     int32_t         nStartBit,
                     int32_t         nBits)
{
    uint32_t mask = (1 << nBits) - 1;

    return (value & ~(mask << nStartBit)) +
        ((bitfield & mask) << nStartBit);
}
uint32_t ConvertHostToNetwork32(uint32_t host) 
{
    uint32_t hostByte = (uint8_t)host;

    hostByte = (((uint32_t)(((uint8_t*)(&(host)))[3])) | 
        (((uint32_t)(((uint8_t*)(&(host)))[2])) << 8) | 
        (((uint32_t)(((uint8_t*)(&(host)))[1])) << 16) | 
        (((uint32_t)(((uint8_t*)(&(host)))[0])) << 24));

    return hostByte;
}
void ConvertToNetwork(void *src, void *dest ,int32_t pos, int32_t n)
{
    int32_t i;
    for (i = pos; i < pos + n; ++i)
        ((uint32_t*)dest)[i] = ConvertHostToNetwork32(((int32_t*)src)[i]);
}
void msink_rv_rtp::write_file(rtp_block *rtp)
{
    if (m_file_path.empty())
    {
        return;
    }

    uint8_t * raw_data = 0;
    uint32_t  raw_len  = 0;
    uint32_t off_set = 0;

    tghelper::byte_block * bind_block = rtp->get_bind_block();
    if (bind_block)
    {
        raw_data = bind_block->get_raw();
        raw_len  = bind_block->payload_size();
        off_set = bind_block->payload_offset();
    }
    else
    {
        raw_data = rtp->get_raw();
        raw_len  = rtp->payload_size();
        off_set = rtp->payload_offset();
    }

    //*//////////////////////////////////////////////////////////////////////////
    rv_rtp_param *p = &(rtp->m_rtp_param);
    uint32_t header[16] = {0};
    uint32_t header_n[16] = {0};
    // sets the fields inside RTP message/
    header[0]=0;
    header[0]=bitfieldSet(header[0],2,30,2);                // protocol version 2
    header[0]=bitfieldSet(header[0],p->paddingBit,29,1);	// padding bit if exist
    header[0]=bitfieldSet(header[0],p->extensionBit,28,1);	// extension bit if exist
    header[0]=bitfieldSet(header[0],p->marker,23,1);
    header[0]=bitfieldSet(header[0],p->payload,16,7);
    header[0]=bitfieldSet(header[0],p->sequenceNumber,0,16);
    header[1]=p->timestamp;
    header[2]=get_rtp_ssrc(&m_hrv);;

    /*if (p->extensionBit)
    {
    header[3] = 0;
    header[3] = bitfieldSet(header[3], p->extensionLength,  0,16);
    header[3] = bitfieldSet(header[3], p->extensionProfile,16,16);
    if (p->extensionLength>0)
    {
    uint32_t count = 0;
    for (count=0;count<p->extensionLength;count++)
    {
    header[4+count] = p->extensionData[count];
    }
    }
    ConvertToNetwork(header, 0, 4 + p->extensionLength);
    }
    else*/
    {
        // converts an array of 4-byte integers from host format to network format
        ConvertToNetwork(header, header_n, 0, 3);
    }
    //////////////////////////////////////////////////////////////////////////*/

    FILE *pF = ::fopen(m_file_path.c_str(), "ab");
    if (pF)
    {
        uint32_t len_data = 12+raw_len;
        ::fwrite(&len_data, sizeof(len_data), 1, pF);
        ::fwrite(header_n, 12, 1, pF);
        ::fwrite(raw_data+off_set, raw_len, 1, pF);
        ::fclose(pF);
    }

}

void msink_rv_rtp::rv_write_rtp(rtp_block *rtp)
{
	//不使用sink库中sn号，否则对端收到后会认为丢包，而sink库中的丢包此处以无能无力
	rtp->m_rtp_param.sequenceNumber = m_last_frame_in_rtp_sn++;
	rtp->m_rtp_param.sSrc = get_rtp_ssrc(&m_hrv);
#ifdef _USE_RTP_TRAFFIC_SHAPING
	if (NULL != m_traffic_shaping.get())
	{
		rtp->assign();
		m_traffic_shaping->flow_in(rtp);
	}
	else
	{
		::write_rtp(&m_hrv, rtp->get_raw(), rtp->payload_totalsize(), &(rtp->m_rtp_param));
	}
#else
	::write_rtp(&m_hrv, rtp->get_raw(), rtp->payload_totalsize(), &(rtp->m_rtp_param));
#endif
	if (m_nReSend>0 && m_bReady && m_active)
	{
		//丢包重传内存拷贝保存，因为要修改sink库堆内存中数据
		rtp_block *rtp2 = m_rtp_pool.force_alloc_any();
		if (rtp2)
		{
			rtp2->assign();
			rtp2->m_bFrameInfo = false;

			//rtp->read(rtp2->get_raw(), rtp2->size());
			rtp2->copy(rtp);

			//rtp2->set_params(rtp->m_rtp_param.len-rtp->m_rtp_param.sByte,rtp->m_rtp_param.sByte);
			rtp2->set_rtp_param(&(rtp->m_rtp_param));
		}
		m_manReSend.addSeg(rtp2);
	}
}
void msink_rv_rtp::internel_write_to_rv_adapter(rtp_block *rtp)
{
    if (rtp && !rtp->m_resend)
    {
        write_file(rtp);
    }

    uint8_t * raw_data = 0;
    uint32_t  raw_len  = 0;
    uint8_t data[5120];

    tghelper::byte_block * bind_block = rtp->get_bind_block();
    if (bind_block)
    {
        raw_data = bind_block->get_raw();
        raw_len  = bind_block->payload_totalsize();
    }
    else
    {
        raw_data = rtp->get_raw();
        raw_len  = rtp->payload_totalsize();
    }

    if (rtp->m_use_ssrc)
    {
        rtp->m_rtp_param.sSrc = rtp->m_ssrc;
    }
    else
    {
        rtp->m_rtp_param.sSrc = get_rtp_ssrc(&m_hrv);
    }

    if (rtp->m_resend)
    {
    }
    else if (rtp->m_bFrameInfo)
    {
        rtp->m_rtp_param.extensionBit = true;

        if(m_open_pri > 0)
        {
            rtp->m_rtp_param.extensionLength = 4;
            rtp->m_rtp_param.extensionData = rtp->m_exHead;
            rtp->m_exHead[0] = rtp->m_infoFrame.verify;
            rtp->m_exHead[1] = rtp->m_infoFrame.frametype;
            rtp->m_exHead[2] = rtp->m_infoFrame.datatype;
            rtp->m_exHead[3] = rtp->m_priority;

            ::memcpy(data, raw_data+rtp->m_rtp_param.sByte, rtp->payload_size());
            ::memcpy(raw_data+rtp->m_rtp_param.sByte+20, data, rtp->payload_size());

            rtp->m_rtp_param.sByte += 20;
            rtp->set_params(rtp->payload_size(), rtp->m_rtp_param.sByte);
        }
        else
        {
            rtp->m_rtp_param.extensionLength = 3;
            rtp->m_rtp_param.extensionData = rtp->m_exHead;
            rtp->m_exHead[0] = rtp->m_infoFrame.verify;
            rtp->m_exHead[1] = rtp->m_infoFrame.frametype;
            rtp->m_exHead[2] = rtp->m_infoFrame.datatype;

            ::memcpy(data, raw_data+rtp->m_rtp_param.sByte, rtp->payload_size());
            ::memcpy(raw_data+rtp->m_rtp_param.sByte+16, data, rtp->payload_size());

            rtp->m_rtp_param.sByte += 16;
            rtp->set_params(rtp->payload_size(), rtp->m_rtp_param.sByte);
        }
    }
    else if (m_open_pri > 0)
    {
        rtp->m_rtp_param.extensionBit = true;
        rtp->m_rtp_param.extensionLength = 1;
        rtp->m_rtp_param.extensionData = rtp->m_exHead;
        rtp->m_exHead[0] = rtp->m_priority;

        ::memcpy(data, raw_data+rtp->m_rtp_param.sByte, rtp->payload_size());
        ::memcpy(raw_data+rtp->m_rtp_param.sByte+8, data, rtp->payload_size());

        rtp->m_rtp_param.sByte += 8;
        rtp->set_params(rtp->payload_size(), rtp->m_rtp_param.sByte);
    }

	::write_rtp(&m_hrv, raw_data, rtp->payload_totalsize(), &(rtp->m_rtp_param));

    if (m_nReSend>0 && m_bReady && m_active)
    {
        if (!rtp->m_resend)
        {
            m_manReSend.addSeg(rtp);
        }
    }
}

#ifdef _USE_RTP_TRAFFIC_SHAPING
void msink_rv_rtp::on_flow_out(void *flow)
{
    rtp_block *rtp = static_cast<rtp_block *>(flow);
    if (NULL != rtp)
    {
		//::write_rtp(&m_hrv, rtp->get_raw(), rtp->payload_totalsize(), &(rtp->m_rtp_param));
        internel_write_to_rv_adapter(rtp);
    }
}
#endif
}
