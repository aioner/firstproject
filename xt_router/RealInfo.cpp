#include "RealInfo.h"
#include "h_xtmediaserver.h"
#include "XTEngine.h"

CRealInfo CRealInfo::m_obj;

CRealInfo::CRealInfo(void)
{
    m_bCenterDbCommandShowFlg = false;
    m_bResponseToCenterShowFlg = false;
    m_bSigalingExecRetShowFlg = false;
}

CRealInfo::~CRealInfo(void)
{
}

void CRealInfo::GetConnectInfo(std::list<info_mgr::INFO_TRANS>& lstConnectInfo)
{
    lstConnectInfo.clear();

    uint32_t connect_num=xt_get_cur_connect_num();
    if (connect_num < 1)
    {
        return;
    }

    connect_info_t* info = new connect_info_t[connect_num];
    if (!info)
    {
        return;
    }

    ::xt_get_connect_info(info,connect_num);

    info_mgr::INFO_TRANS tmp;
    for(uint32_t index=0; index < connect_num; ++index)
    {
        //产生时间
        ::strncpy(tmp.m_pszCreateTime,info[index].m_pszCreateTime,MAX_STR_SIZE);

        tmp.m_lChID = info[index].m_lChID;

        //目标IP	
        ::strncpy(tmp.m_pszDestIP,info[index].m_pszDestIP,MAX_STR_SIZE);

        //目标端口
        tmp.m_lDestPort = info[index].m_lDestPort;

        tmp.m_lSendPort = info[index].m_lSendPort;

        //ssrc	
        tmp.m_uiSsrc = info[index].m_uiSsrc;

        //累计发包量
        tmp.m_uiPackets = info[index].m_uiPackets;

        //累计发送字节数（KB）	
        tmp.m_uiOctets = info[index].m_uiOctets;

        //累计丢包量	
        tmp.m_uiCumulativeLost = info[index].m_uiCumulativeLost;
		
		//往返时延
		tmp.m_urtt = info[index].m_urtt;

        //丢包率
        tmp.m_uiFractionLost = info[index].m_uiFractionLost;

        //网络抖动ms
        tmp.m_uiJitter = info[index].m_uiJitter;

        //复用标识
        tmp.m_bDestMultiplex = info[index].m_bDestMultiplex;

        //复用ID
        tmp.m_uiDestMultid = info[index].m_uiDestMultid;

        tmp.m_bSendMultiplex = info[index].m_bSendMultiplex;
        tmp.m_uiSendMultid = info[index].m_uiSendMultid;

        //协议
        tmp.m_usProtocol = info[index].m_usProtocol;
        tmp.srcno = info[index].srcno;

        lstConnectInfo.push_back(tmp);
    }

    if (info)
    {
        delete[] info;
        info=NULL;
    }
}

void CRealInfo::GetPlayInfo(std::list<info_mgr::INFO_PLAY>& lstPlayInfo)
{
    lstPlayInfo.clear();
    std::list<src_info> lstSrc;
    XTEngine::instance()->get_all_src(lstSrc);

    info_mgr::INFO_PLAY playInfo;
    std::list<src_info>::iterator itrsrc = lstSrc.begin();
    for (; lstSrc.end() != itrsrc; ++itrsrc)
    {
        //创建时间
        playInfo.m_strBiuldTime = info_mgr::ToStrMicrosecByPtime(itrsrc->create_time);

        //设备类型
        playInfo.m_iType = itrsrc->device.db_type;

        //设备IDS
        playInfo.m_sDevIDS = itrsrc->device.dev_ids; 

        //设备通道号
        playInfo.m_lDevCh = itrsrc->device.dev_chanid;

        //设备码流类型
        playInfo.m_lDevStrmTyp = itrsrc->device.dev_strmtype;

        //点播url
        playInfo.m_sDBUrl = itrsrc->device.db_url;

        //点播通道
        playInfo.m_lDBCh = itrsrc->device.db_chanid;

        //流标识
        playInfo.m_lStreamId = itrsrc->device.strmid;
        playInfo.m_Srcno = itrsrc->srcno;
        playInfo.m_Dev_handle = itrsrc->device.dev_handle;

        //流数量
        playInfo.m_iTracknum = itrsrc->device.tracknum;

        //系统头
        playInfo.m_lKey = itrsrc->device.key_len;

        //帧数
        playInfo.m_lFrames = itrsrc->frames;

        //端口
        playInfo.m_lPort = itrsrc->device.db_port;

        //转发服务通道
        XTEngine::instance()->get_main_chanid(itrsrc->srcno,playInfo.m_iTransChannel);

        lstPlayInfo.push_back(playInfo);
    }

}

void CRealInfo::SetRealShowFlg(const info_mgr::INFO_TYPE ulInfoType,const bool bIsRealShowFlg/* = true*/)
{

    switch (ulInfoType)
    {

    case info_mgr::INFO_CENTER_DB_COMMAND_EVENT:
        {
            boost::unique_lock<boost::recursive_mutex> lock(m_CenterDbCommandFlgMutex);
            m_bCenterDbCommandShowFlg = bIsRealShowFlg;
            break;
        }

    case info_mgr::INFO_RESPONSE_TO_CENTER_EVET_ID:
        {
            boost::unique_lock<boost::recursive_mutex> lock(m_ResponseToCenterFlgMutex);
            m_bResponseToCenterShowFlg = bIsRealShowFlg;
            break;
        }

    case info_mgr::INFO_SIGNALING_EXEC_RESULT_ID:
        {
            boost::unique_lock<boost::recursive_mutex> lock(m_SigalingExecRetFlgMutex);
            m_bSigalingExecRetShowFlg = bIsRealShowFlg;
            break;
        }
    default:
        {
            break;
        }

    }
}

void CRealInfo::PostCenterDbCommandEventInfo(const info_mgr::INFO_CENTERDBCOMMANDEVENT& infoRealPlayEvent)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_PlayRealEventMutex);
    boost::unique_lock<boost::recursive_mutex> lock_1(m_CenterDbCommandFlgMutex);	
    if (m_bCenterDbCommandShowFlg)
    {
        m_listRealPlayEvent.push_back(infoRealPlayEvent);
    }
}

void CRealInfo::GetCenterDbCommandEventInfo(std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>& listRealPlayEvent)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_PlayRealEventMutex);

    listRealPlayEvent = m_listRealPlayEvent;
    m_listRealPlayEvent.clear();	
}

void CRealInfo::PostRealResponseToCenterEventInfo(const info_mgr::INFO_RESPONSETOCENTER& infoResponseToCenterEvent)
{

    boost::unique_lock<boost::recursive_mutex> lock(m_ResponseToCenterMutex);
    boost::unique_lock<boost::recursive_mutex> lock_1(m_ResponseToCenterFlgMutex);	
    if (m_bResponseToCenterShowFlg)
    {
        m_listRealResponseToCenterEvent.push_back(infoResponseToCenterEvent);
    }

}
void CRealInfo::GetRealResponseToCenterEventInfo(std::list<info_mgr::INFO_RESPONSETOCENTER>& listRealResponseToCenterEvent)
{

    boost::unique_lock<boost::recursive_mutex> lock(m_ResponseToCenterMutex);
    listRealResponseToCenterEvent = m_listRealResponseToCenterEvent;
    m_listRealResponseToCenterEvent.clear();
}

//信令执行结果
void CRealInfo::PostRealSigalingExecRet(const info_mgr::INFO_SIGNALINGEXECRESULT& info)
{

    boost::unique_lock<boost::recursive_mutex> lock(m_SigalingExecRuslutMutex);
    boost::unique_lock<boost::recursive_mutex> lock_1(m_SigalingExecRetFlgMutex);
    if (m_bSigalingExecRetShowFlg)
    {
        m_lstRealSigalingExecRuslut.push_back(info);
    }

}

void CRealInfo::GetRealSigalingExecRet(std::list<info_mgr::INFO_SIGNALINGEXECRESULT>& lstSigalingExecRuslut)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_SigalingExecRuslutMutex);
    lstSigalingExecRuslut = m_lstRealSigalingExecRuslut;
    m_lstRealSigalingExecRuslut.clear();

}