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
        //����ʱ��
        ::strncpy(tmp.m_pszCreateTime,info[index].m_pszCreateTime,MAX_STR_SIZE);

        tmp.m_lChID = info[index].m_lChID;

        //Ŀ��IP	
        ::strncpy(tmp.m_pszDestIP,info[index].m_pszDestIP,MAX_STR_SIZE);

        //Ŀ��˿�
        tmp.m_lDestPort = info[index].m_lDestPort;

        tmp.m_lSendPort = info[index].m_lSendPort;

        //ssrc	
        tmp.m_uiSsrc = info[index].m_uiSsrc;

        //�ۼƷ�����
        tmp.m_uiPackets = info[index].m_uiPackets;

        //�ۼƷ����ֽ�����KB��	
        tmp.m_uiOctets = info[index].m_uiOctets;

        //�ۼƶ�����	
        tmp.m_uiCumulativeLost = info[index].m_uiCumulativeLost;
		
		//����ʱ��
		tmp.m_urtt = info[index].m_urtt;

        //������
        tmp.m_uiFractionLost = info[index].m_uiFractionLost;

        //���綶��ms
        tmp.m_uiJitter = info[index].m_uiJitter;

        //���ñ�ʶ
        tmp.m_bDestMultiplex = info[index].m_bDestMultiplex;

        //����ID
        tmp.m_uiDestMultid = info[index].m_uiDestMultid;

        tmp.m_bSendMultiplex = info[index].m_bSendMultiplex;
        tmp.m_uiSendMultid = info[index].m_uiSendMultid;

        //Э��
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
        //����ʱ��
        playInfo.m_strBiuldTime = info_mgr::ToStrMicrosecByPtime(itrsrc->create_time);

        //�豸����
        playInfo.m_iType = itrsrc->device.db_type;

        //�豸IDS
        playInfo.m_sDevIDS = itrsrc->device.dev_ids; 

        //�豸ͨ����
        playInfo.m_lDevCh = itrsrc->device.dev_chanid;

        //�豸��������
        playInfo.m_lDevStrmTyp = itrsrc->device.dev_strmtype;

        //�㲥url
        playInfo.m_sDBUrl = itrsrc->device.db_url;

        //�㲥ͨ��
        playInfo.m_lDBCh = itrsrc->device.db_chanid;

        //����ʶ
        playInfo.m_lStreamId = itrsrc->device.strmid;
        playInfo.m_Srcno = itrsrc->srcno;
        playInfo.m_Dev_handle = itrsrc->device.dev_handle;

        //������
        playInfo.m_iTracknum = itrsrc->device.tracknum;

        //ϵͳͷ
        playInfo.m_lKey = itrsrc->device.key_len;

        //֡��
        playInfo.m_lFrames = itrsrc->frames;

        //�˿�
        playInfo.m_lPort = itrsrc->device.db_port;

        //ת������ͨ��
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

//����ִ�н��
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