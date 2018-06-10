#include "ReSendMan.h"
#include <../rv_adapter/rv_api.h>
#include "mp_caster.h"
#include "msink_rv_rtp.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef _ANDROID
#include "xt_config_cxx.h"
#endif

extern void writeLogV(int nLogLevel, const char* szLogName,const char* szLogFmt, va_list vArgList);
extern void writeLog(int nLevel, const char* szLogName, const char* szLogFmt, ...);

#define MAX_RESEND_LEN	1024	//  最大缓冲长度
#define MAX_RESEND_AU	1024  	//  最大权标
#define DEFAULT_RESEND	1		//  最大权标

#ifdef _ANDROID
ReSendMan::ReSendMan(void)
:m_sink(NULL),
m_nSeg(MAX_RESEND_LEN)
,m_nMaxAu(MAX_RESEND_AU)
,m_nPackResend(DEFAULT_RESEND)
,m_size_seg(0)
{    m_nSeg = xt_config::router_module::get<int>("config.caster_cfg.ReSendLen",MAX_RESEND_LEN);
    m_nMaxAu = xt_config::router_module::get<int>("config.caster_cfg.ReSendAu",MAX_RESEND_AU);
    m_nPackResend = xt_config::router_module::get<int>("config.caster_cfg.packresend",DEFAULT_RESEND);
}
#else
ReSendMan::ReSendMan(void)
:m_sink(NULL),
m_nSeg(MAX_RESEND_LEN)
,m_nAu(MAX_RESEND_AU)
,m_nMaxAu(MAX_RESEND_AU)
,m_nPackResend(DEFAULT_RESEND)
,m_size_seg(0)
{    m_nSeg = config::_()->ReSendLen(MAX_RESEND_LEN);    m_nMaxAu = config::_()->ReSendAu(MAX_RESEND_AU);
    m_nPackResend = config::_()->packresend(DEFAULT_RESEND);
}
#endif

ReSendMan::~ReSendMan(void)
{
    clrSeg();
}

// 分组缓冲
void ReSendMan::addSeg(rtp_block *rtp)
{
    uint16_t sn = 0;
    if (!rtp)
    {
        return;
    }
    sn = rtp->m_rtp_param.sequenceNumber;

    //////////////////////////////////////////////////////////////////////////
	uint32_t header_exlen = 0;
    uint32_t offset = rtp->m_rtp_param.sByte;
    if (rtp->m_rtp_param.extensionBit && rtp->m_rtp_param.extensionLength>=1 && rtp->m_rtp_param.extensionData)
    {
        uint32_t exData[64];
        ::memcpy(exData, rtp->m_rtp_param.extensionData, 4*rtp->m_rtp_param.extensionLength);
        ::memcpy(rtp->m_exHead+1, exData, 4*rtp->m_rtp_param.extensionLength);

        rtp->m_rtp_param.extensionLength += 1;
        rtp->m_rtp_param.sByte += 4;
		header_exlen = 4;
    }
    else
    {
        rtp->m_rtp_param.extensionBit = true;
        rtp->m_rtp_param.extensionLength = 1;
        rtp->m_rtp_param.sByte += 8;
		header_exlen = 8;
    }
	//重传包头部自定义标记
    uint32_t resend = 0x00AABBCC;
    rtp->m_exHead[0] = resend;
    rtp->m_rtp_param.extensionData = rtp->m_exHead;

    uint8_t * raw_data = 0;
    tghelper::byte_block * bind_block = rtp->get_bind_block();
    if (bind_block)
    {
        raw_data = bind_block->get_raw();
    }
    else
    {
        raw_data = rtp->get_raw();
    }

    if (raw_data)
    {
        uint8_t data[5120];//数据m_rtp_param.sByte改变
        ::memcpy(data, raw_data+offset, rtp->payload_size());
        ::memcpy(raw_data+rtp->m_rtp_param.sByte, data, rtp->payload_size());
		//rtp固定头扩展长度增加，payload数据后移，实际参数保留在param中，发送时在拷贝到头部
		/*int last_index = rtp->payload_totalsize()-1;
		for (int i=0;i<rtp->payload_size();i++)
		{
			raw_data[last_index-i+header_exlen] = raw_data[last_index-i];
		}*/
    }

    rtp->set_params(rtp->payload_size(), rtp->m_rtp_param.sByte);
    //////////////////////////////////////////////////////////////////////////*/

    boost::unique_lock<boost::recursive_mutex> lock(m_mSeg);

    if (m_nSeg <= 0)
    {
        rtp->release();
    }
    else if (m_size_seg > m_nSeg)
    {
        vector<rtp_block*>::iterator itr = m_vecSegment.begin();
        rtp_block *seg = m_vecSegment[0];
        if (seg)
        {
            seg->release();
        }

        m_vecSegment.erase(itr);
        m_vecSegment.push_back(rtp);
    }
    else
    {
        m_vecSegment.push_back(rtp);
        m_size_seg += 1;
    }

    m_nAu +=1;
    if (m_nAu > m_nMaxAu)
    {
        m_nAu = m_nMaxAu;
    }
}

// 清空缓冲
void ReSendMan::clrSeg()
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mSeg);

    vector<rtp_block*>::iterator itr = m_vecSegment.begin();
    for (;itr != m_vecSegment.end();++itr)
    {
        rtp_block *rtp = *itr;
        if (rtp)
        {
            rtp->release();
        }
    }
    m_vecSegment.clear();
    m_size_seg = 0;
}

// 数据重发
void ReSendMan::reSend(uint16_t sn)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mSeg);

    if (m_nAu < m_nPackResend)
    {
        writeLog(5,"重发失败","没有权标[%d]", m_nMaxAu);
        return;
    }

    m_nAu -= m_nPackResend;

    rtp_block *seg = NULL;
    if (m_size_seg > 0)
    {
        uint16_t sn0 = m_vecSegment[0]->m_rtp_param.sequenceNumber;
        uint16_t index = ::abs(sn-sn0);
        if (index > 32767)
        {
            index = 65536 - index;
        }
        if (index < m_size_seg)
        {
            seg = m_vecSegment[index];
        }

        if (!seg)
        {
            writeLog(5,"重发失败","没有数据[%d]", sn);
        }
    }

    /*list<rtp_block*>::reverse_iterator itr = m_vecSegment.rbegin();
    for (;itr != m_vecSegment.rend();++itr)
    {
    rtp_block *tmp = *itr;
    if (tmp && tmp->m_rtp_param.sequenceNumber==sn)
    {
    seg = tmp;
    break;
    }
    }*/

    if (m_sink && seg && seg->m_rtp_param.sequenceNumber==sn)
    {
        writeLog(5,"重发","重发SN[%d]", seg->m_rtp_param.sequenceNumber);

        //((msink_rv_rtp*)m_sink)->write_to_rv_adapter(seg, true);
		//避免做一次重复检查，请求重传的包也不通过流量整形
		seg->m_resend = true;
		((msink_rv_rtp*)m_sink)->internel_write_to_rv_adapter(seg);
		//::write_rtp(&((msink_rv_rtp*)m_sink)->m_hrv, seg->get_raw(), seg->payload_totalsize(), &(seg->m_rtp_param));
    }
    else
    {
        writeLog(5,"重发失败","没有数据[%d]", sn);
        return;
    }
}
