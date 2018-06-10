#include "XTTcp.h"

#ifdef USE_TCP_SERVER_
XTTcp XTTcp::self;
XTTcp::XTTcp(void)
:m_pServer(NULL)
,m_serverPort(20000)
,m_numChan(0)
{
}

XTTcp::~XTTcp(void)
{
}

void CALLBACK BackState(BYTE MODE,char*addr,int port,BYTE state)
{ 
}

int XTTcp::init(unsigned long num_chan, string ip, unsigned short server_port)
{
    utility::unique_lock<utility::shared_mutex> lock(m_mutex); 

    m_pServer = XT_Create_Server();
    if (!m_pServer)
    {
        return -1;
    }

    m_pServer->XT_ServerSetOnState(BackState);

    m_serverPort = server_port;
    m_pServer->XT_ServerSetNetPort(server_port, 0);

    XT_SERVER_VIDEOINFO videoinfo;
    (void)memset(&videoinfo, 0, sizeof(XT_SERVER_VIDEOINFO));
    ::strcpy_s(videoinfo.m_addr, 20, ip.c_str());
    videoinfo.m_channum = num_chan;

    m_numChan = num_chan;
    m_pServer->XT_ServerStart(&videoinfo);

    return 0;
}

int XTTcp::uninit()
{
    utility::unique_lock<utility::shared_mutex> lock(m_mutex); 

    if (!m_pServer)
    {
        return -1;
    }

    m_pServer->XT_ServerStop();	
    XT_Release_Server(m_pServer);


    return 0;
}

int XTTcp::send_data(unsigned long chanid,
                     char *buff,
                     unsigned long len,
                     int frame_type,
                     long device_type)
{
    utility::shared_lock<utility::shared_mutex> lock(m_mutex); 

    if(chanid>=m_numChan)
    {
        return -1;
    }

    if(NULL != m_pServer)
    {
        m_pServer->XT_ServerWriteData(chanid, (UCHAR*)buff, len, frame_type, device_type);
    }

    return 0;
}

int XTTcp::get_link_num()
{
    utility::shared_lock<utility::shared_mutex> lock(m_mutex); 

    long nCount = 0;

    if(m_pServer)
    {
        nCount = m_pServer->XT_ServerGetCurrentConnection();
    }

    return nCount;
}

int XTTcp::get_tcp_trans_info(std::vector<connect_info_t>& vecInfo)
{
    utility::shared_lock<utility::shared_mutex> lock(m_mutex); 

    int iret_code = 0;
    if(m_pServer)
    {
        connect_info_t tcpInfo;

        std::vector<TCP_connect_info_t> vecTcpInfo;
        iret_code = m_pServer->XT_GetTcpConnetInfo(vecTcpInfo);

        std::vector<TCP_connect_info_t>::iterator itr = vecTcpInfo.begin();
        for (;vecTcpInfo.end() != itr; ++itr)
        {
            //tcpInfo.clear();

            //创建时间
            ::strncpy(tcpInfo.m_pszCreateTime,itr->m_pszCreateTime,MAX_STR_SIZE);

            //目标IP
            ::strncpy(tcpInfo.m_pszDestIP,itr->m_pszDestIP,MEDIA_SERVER_IP_LEN);

            //目标端口
            tcpInfo.m_lDestPort = itr->m_lDestPort;

            //源IP
            ::strncpy(tcpInfo.m_pszSrcIP,itr->m_pszSrcIP,MEDIA_SERVER_IP_LEN);

            //源端口
            tcpInfo.m_lSrcPort = itr->m_lSrcPort;

            //转发服务通道
            tcpInfo.m_lChID = itr->m_lChID;

            //用使用Buffer大小代丢包
            tcpInfo.m_uiOctets = itr->nAcceptLen;

            //协议
            tcpInfo.m_usProtocol = 1;//info_mgr::TCP_MODE;

            vecInfo.push_back(tcpInfo);

        }
    }
    else
    {
        iret_code = -1;
    }

    return iret_code;

}
#endif //#ifdef USE_TCP_SERVER_
