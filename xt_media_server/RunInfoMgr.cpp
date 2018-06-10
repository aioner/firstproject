#include "RunInfoMgr.h"
#include "XTTcp.h"
#include "XTRtp.h"
#include "XTSrc.h"

CRunInfoMgr::CRunInfoMgr(void)
{}

CRunInfoMgr::~CRunInfoMgr(void)
{}
CRunInfoMgr* CRunInfoMgr::instance() 
{ 
    static CRunInfoMgr m_Obj;
    return &m_Obj;
};

int CRunInfoMgr::GetInfoConnectInfo(std::vector<connect_info_t>& vecConnectInfoData)
{
    int iRetCode = 0;

#ifdef USE_TCP_SERVER_
    //�ռ�TCP��ת����Ϣ
    iRetCode = XTTcp::instance()->get_tcp_trans_info(vecConnectInfoData);
#endif //#ifdef USE_TCP_SERVER_

    //�ռ�RTP
    std::list<Rtp_Sink> lstRTPSink;
    XTRtp::instance()->get_rtp_sink(lstRTPSink);

    connect_info_t RtcpInfo;
    rtcp_send_report sr;
    rtcp_receive_report rr;
    Rtp_Handle send_rtp;

    std::list<Rtp_Sink>::iterator itr = lstRTPSink.begin();
    for (;lstRTPSink.end() != itr; ++itr)
    {
        ::memset(&sr, 0, sizeof(sr));
        ::memset(&rr, 0, sizeof(rr));

        rv_rtcp_srinfo &srinfo = itr->send.sr;
        rv_rtcp_rrinfo &rrinfo = itr->recieve.rrFrom;

        sr.ssrc = itr->ssrc;
        sr.mNTPtimestamp = srinfo.mNTPtimestamp;
        sr.lNTPtimestamp = srinfo.lNTPtimestamp;
        sr.timestamp = srinfo.timestamp;
        sr.packets = srinfo.packets;
        sr.octets = srinfo.octets;

        rr.ssrc = itr->ssrc;
        rr.fractionLost = rrinfo.fractionLost;
        rr.cumulativeLost = rrinfo.cumulativeLost;			

        rr.sequenceNumber = rrinfo.sequenceNumber;
        rr.jitter = rrinfo.jitter * 0xF;
        rr.lSR = rrinfo.lSR;
        rr.dlSR = rrinfo.dlSR;

        //RtcpInfo.clear();

        //m_pRTCPInfo[i]->strCTime);//����ʱ��
        ::strncpy(RtcpInfo.m_pszCreateTime,common::ToStrMicrosecByPtime(itr->createtime).c_str(),MAX_STR_SIZE);

        //m_pRTCPInfo[i]->nCHIndex);//����ͨ��
        RtcpInfo.m_lChID = itr->chanid;

        //m_pRTCPInfo[i]->strAddr);//Ŀ��IP	
        //RtcpInfo.m_strDestIP = itr->ip;
        ::strncpy(RtcpInfo.m_pszDestIP,itr->ip.c_str(),MAX_STR_SIZE);

        //m_pRTCPInfo[i]->nPort);//Ŀ��˿�
        RtcpInfo.m_lDestPort = itr->port;

        //m_pRTCPInfo[i]->nRtcpSSRC);//ssrc	
        RtcpInfo.m_uiSsrc = itr->ssrc;

        //m_pRTCPInfo[i]->rtcp_sr.packets/1024.0);//�ۼƷ�����
        RtcpInfo.m_uiPackets = sr.packets/1024;

        //m_pRTCPInfo[i]->rtcp_sr.octets/1024.0);//�ۼƷ����ֽ�����KB��	
        RtcpInfo.m_uiOctets = sr.octets/1024;

        //m_pRTCPInfo[i]->rtcp_rr.cumulativeLost);//�ۼƶ�����	
        RtcpInfo.m_uiCumulativeLost = rr.cumulativeLost;

        //m_pRTCPInfo[i]->lostPertg)*100);//������
        RtcpInfo.m_uiFractionLost = rr.fractionLost/0xFF;

        //m_pRTCPInfo[i]->rtcp_rr.jitter)/90);//���綶��ms
        RtcpInfo.m_uiJitter = rr.jitter/90;

        //���ñ�ʶ
        RtcpInfo.m_bDestMultiplex = itr->multiplex;

        //����ID
        RtcpInfo.m_uiDestMultid = itr->multid;

        //Э��
        RtcpInfo.m_usProtocol = itr->mode;

        //���Ͷ˿�
        (void)XTRtp::instance()->get_rtp(itr->chanid, send_rtp);

        RtcpInfo.m_lSendPort = send_rtp.port;
        RtcpInfo.m_bSendMultiplex = send_rtp.multiplex;
        RtcpInfo.m_uiSendMultid = send_rtp.multid;

        //�ռ�
        vecConnectInfoData.push_back(RtcpInfo);

    }
    return iRetCode;

}

uint32_t CRunInfoMgr::get_cur_connect_num()
{
    return XTRtp::instance()->get_cur_sink_num();
}

int CRunInfoMgr::get_connect_info(connect_info_t out_cinfo[],uint32_t& connect_num)
{
    int ret_code = 0;

    std::list<Rtp_Sink> lst_rtp_sink;
    lst_rtp_sink.clear();
    XTRtp::instance()->get_rtp_sink(lst_rtp_sink);

    rtcp_send_report sr;
    rtcp_receive_report rr;
    Rtp_Handle send_rtp;

    int index=0;
    std::list<Rtp_Sink>::iterator itr = lst_rtp_sink.begin();
    for (;lst_rtp_sink.end() != itr && index < connect_num; ++itr)
    {
        ::memset(&sr, 0, sizeof(sr));
        ::memset(&rr, 0, sizeof(rr));

        rv_rtcp_srinfo &srinfo = itr->send.sr;
        rv_rtcp_rrinfo &rrinfo = itr->recieve.rrFrom;

        sr.ssrc = itr->ssrc;
        sr.mNTPtimestamp = srinfo.mNTPtimestamp;
        sr.lNTPtimestamp = srinfo.lNTPtimestamp;
        sr.timestamp = srinfo.timestamp;
        sr.packets = srinfo.packets;
        sr.octets = srinfo.octets;

        rr.ssrc = itr->ssrc;
        rr.fractionLost = rrinfo.fractionLost;
		if (rrinfo.cumulativeLost ^ (-1) == 0)
		{
			rrinfo.cumulativeLost = 0;//first it's value is -1
		}
        rr.cumulativeLost = rrinfo.cumulativeLost;

        rr.sequenceNumber = rrinfo.sequenceNumber;
        rr.jitter = rrinfo.jitter * 0xF;
        rr.lSR = rrinfo.lSR;
        rr.dlSR = rrinfo.dlSR;

        //����ʱ��
        ::strncpy(out_cinfo[index].m_pszCreateTime,common::ToStrMicrosecByPtime(itr->createtime).c_str(),MAX_STR_SIZE);

        //����ͨ��
        out_cinfo[index].m_lChID = itr->chanid;

        //Ŀ��IP	
        ::strncpy(out_cinfo[index].m_pszDestIP,itr->ip.c_str(),MAX_STR_SIZE);

        //Ŀ��˿�
        out_cinfo[index].m_lDestPort = itr->port;

        //ssrc	
        out_cinfo[index].m_uiSsrc = itr->ssrc;

        //�ۼƷ�����
        out_cinfo[index].m_uiPackets = sr.packets;

        //�ۼƷ����ֽ�����KB��	
        out_cinfo[index].m_uiOctets = sr.octets/1024;

        //�ۼƶ�����	
		out_cinfo[index].m_uiCumulativeLost = rr.cumulativeLost;
		
		//����ʱ��
		out_cinfo[index].m_urtt = itr->recieve.rtt;

        //������
        out_cinfo[index].m_uiFractionLost = rr.fractionLost/0xFF;

        //���綶��ms
        out_cinfo[index].m_uiJitter = rr.jitter/90;

        //���ñ�ʶ
        out_cinfo[index].m_bDestMultiplex = itr->multiplex;

        //����ID
        out_cinfo[index].m_uiDestMultid = itr->multid;

        //Э��
        out_cinfo[index].m_usProtocol = itr->mode;

        //���Ͷ˿�
        out_cinfo[index].srcno = XTSrc::instance()->find_src(itr->chanid);
        (void)XTRtp::instance()->get_rtp(itr->chanid, send_rtp);

        out_cinfo[index].m_lSendPort = send_rtp.port;
        out_cinfo[index].m_bSendMultiplex = send_rtp.multiplex;
        out_cinfo[index].m_uiSendMultid = send_rtp.multid;

        ++index;
    }

    connect_num = index;

    return ret_code;
}
