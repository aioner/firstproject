#include "SpecialLine.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include<boost/asio/io_service.hpp>
#include <boost/asio.hpp>
#include "xt_ping/icmp_ping.h"
#include <boost/thread/lock_guard.hpp>
#include "Router_config.h"
#include "XTRouterLog.h" 

#define  STAT_ON_LINE 1 //����
#define  STAT_BREAK 0  //����

//static
SpecialLine SpecialLine::m_ins;

////////////////////////////////////////////////////////////////
//���� �� ���� SpecialLine
//����    �ܡ� ����
//����������� void
//����������� void
//����    �š� ����ר�Ҳ�
//����    �ߡ� XT_songlei
//���� �� �š� V1.0
//����    �ڡ�2014��6��12�� ������
//����    ʷ��
//........
////////////////////////////////////////////////////////////////
SpecialLine::SpecialLine(void)
{
    m_iPingPacklen = 32;
    m_iPingtimeout = 1000;

    m_lstLineInfo.clear();
    m_lstBreakLine.clear();

    m_bUpate = false;
}


////////////////////////////////////////////////////////////////
//���� �� ���� SpecialLine
//����    �ܡ� ����
//����������� void
//����������� void
//����    �š� ����ר�Ҳ�
//����    �ߡ� XT_songlei
//���� �� �š� V1.0
//����    �ڡ�2014��6��12�� ������
//����    ʷ��
//........
////////////////////////////////////////////////////////////////
SpecialLine::~SpecialLine(void)
{

}

////////////////////////////////////////////////////////////////
//���� �� ���� CheckState
//����    �ܡ� ���ר��״̬
//����������� const char* strIP-:Ŀ��IP
//����������� void
//���� �� ֵ�� int -:0-:���� -1-������
//����    �š� ����ר�Ҳ�
//����    �ߡ� XT_songlei
//���� �� �š� V1.0
//����    �ڡ�2014��6��11�� ������
//����    ʷ��
//........
////////////////////////////////////////////////////////////////
int SpecialLine::CheckState(const char* strIP)
{ 
    m_iPingPacklen = config::instance()->check_len(32);
    m_iPingtimeout = config::instance()->check_timeout(1000);

    net_state_type net;
    (void)ping(net,strIP,m_iPingtimeout,m_iPingPacklen);

    return net.net_state;
}

////////////////////////////////////////////////////////////////
//���� �� ���� GetCheckTime()
//����    �ܡ� ��ȡ��ǰʱ��
//����������� void
//����������� void
//���� �� ֵ�� std::string -��ʱ���ַ���
//����    �š� ����ר�Ҳ�
//����    �ߡ� XT_songlei
//���� �� �š� V1.0
//����    �ڡ�2014��6��11�� ������
//����    ʷ��
//........
////////////////////////////////////////////////////////////////
std::string SpecialLine::GetCheckTime()
{
    //%04d-%02d-%02d %02d:%02d:%02d
    std::string strTime = to_iso_extended_string(boost::posix_time::second_clock::local_time());
    std::string::size_type uiPos = strTime.find("T");
    if (uiPos != std::string::npos)
    {
        strTime.replace(uiPos,1,std::string(" "));
    }

    return strTime;
}

////////////////////////////////////////////////////////////////
//���� �� ���� ChangeRouterSet
//����    �ܡ� ���½�������
//����������� const char *pstrRouterSet-��XML��ʽ��������Ϣ
//             const long nSize-:��ϢXML���ĳ���
//����������� void
//���� �� ֵ�� void
//����    �š� ����ר�Ҳ�
//����    �ߡ� XT_songlei
//���� �� �š� V1.0
//����    �ڡ�2014��6��11�� ������
//����    ʷ��
//........
////////////////////////////////////////////////////////////////
void SpecialLine::ChangeRouterSet(const char *pstrRouterSet, const long nSize)
{	

    xtXml xmlTmp;
    xmlTmp.LoadXMLStr(pstrRouterSet);
    boost::lock_guard<boost::detail::spinlock> lock(m_xmlSPLineInfoMutex);
    m_xmlSPLineInfo = xmlTmp;
    m_bUpate = true;

    WRITE_LOG(XTROUTER_ZL,ll_info,
        "************************�յ����ķ�������ר����Ϣ*******************************\n%s",xmlTmp.GetXMLStrEx());

}


////////////////////////////////////////////////////////////////
//���� �� ���� Check
//����    �ܡ� ר�߼��
//����������� void
//����������� CString& strRetSPLine
//���� �� ֵ�� void
//����    �š� ����ר�Ҳ�
//����    �ߡ� XT_songlei
//���� �� �š� V1.0
//����    �ڡ�2014��6��11�� ������
//����    ʷ��
//........
////////////////////////////////////////////////////////////////
void SpecialLine::Check(std::string& strRetSPLine)
{
    //������
    strRetSPLine.clear();

    //�������ķ�������ר����Ϣ
    Analyze();

    if (m_lstLineInfo.empty())
    {
        return;
    }

    //����XML
    xtXml xmlSPLineStatInfo;
    xmlSPLineStatInfo.LoadXMLStr( "<Info type=\"RouterStat\" Num=\"0\"></Info>");


    xtXmlNodePtr HeadInfo = xmlSPLineStatInfo.getRoot();	

    if (HeadInfo.IsNull())
    {
        return;
    }

    HeadInfo.SetAttribute("type","RouterStat");

    m_lstBreakLine.clear();
    std::string strTime;
    int iStat = -1;
    int iNum = 0;
    xtXmlNodePtr Info;
    std::list<SPLINEINFO>::iterator itr = m_lstLineInfo.begin();
    for (; m_lstLineInfo.end() != itr; ++itr)
    {
        //���ר��״̬
        iStat = CheckState(itr->m_strSZIP.c_str()) < 0 ? STAT_BREAK : STAT_ON_LINE;

        //���״̬û�з����仯���÷���
        if (iStat == itr->m_iStat && -1 != itr->m_iStat )
        {
            continue;
        }

        Info = HeadInfo.NewChild("Inf");		

        // SIDS:ר���е�Դ����IDS
        Info.SetAttribute("SIDS",itr->m_strSIDS.c_str());

        // SIP�����Ͷ�ԭʼIP
        Info.SetAttribute("SIP",itr->m_strSIP.c_str());

        //SZIP�����Ͷ�ר��IP
        Info.SetAttribute("SZIP",itr->m_strSZIP.c_str());

        // DIDS: ר���е�Ŀ������IDS
        Info.SetAttribute("DIDS",itr->m_strDIDS.c_str());

        //DIP�����ն�ԭʼIP
        Info.SetAttribute("DIP",itr->m_strDIP.c_str());

        //DZIP�����ն�ר��IP
        Info.SetAttribute("DZIP",itr->m_strDZIP.c_str());

        //Stat������״̬ (0:���ߣ�1������)
        itr->m_iStat = iStat;
        Info.SetAttribute("stat",itr->m_iStat);

        //Time�����һ��״̬�仯ʱ�� (��-��-�գ�ʱ:��:��) ��Ƶ�������ϵ�ʱ�䣨�Ա��ѯ��
        strTime = GetCheckTime();
        Info.SetAttribute("Time",strTime.c_str());

        ++iNum;

        //�ռ��Ѿ����ߵ�ר��
        if (0 == iStat)
        {
            m_lstBreakLine.push_back(*itr);
        }		
    }

    HeadInfo.SetAttribute("Num",iNum);

    //����
    strRetSPLine = xmlSPLineStatInfo.GetXMLStrEx();


    WRITE_LOG(XTROUTER_ZL,ll_info,
        "************************������ר��״̬��Ϣ*******************************\n%s",strRetSPLine.c_str());
}


////////////////////////////////////////////////////////////////
//���� �� ���� GetLocalLineCfg
//����    �ܡ� ��ȡ����ר��������Ϣ
//����������� void
//����������� CString& strLocalCfg -:ר��������Ϣ
//���� �� ֵ�� void
//����    �š� ����ר�Ҳ�
//����    �ߡ� XT_songlei
//���� �� �š� V1.0
//����    �ڡ�2014��6��19�� ������
//����    ʷ��
//........
////////////////////////////////////////////////////////////////
void SpecialLine::GetLocalLineCfg(std::string& strLocalCfg)
{
    std::list<std::string> lstIP;
    GetLocalIpList(lstIP);

    xtXml xmlReqRouterInfo;
    xtXmlNodePtr Ctrl = xmlReqRouterInfo.AddRoot("<Ctrl/>");
    if (Ctrl.IsNull())
    {
        return;
    }

    Ctrl.SetAttribute("type","ReqRouterInfo");
    int iNum = lstIP.size();
    Ctrl.SetAttribute("Num",iNum);


    std::list<std::string>::iterator itr = lstIP.begin();
    xtXmlNodePtr Inf;
    for(;lstIP.end() != itr; ++itr)
    {
        Inf = Ctrl.NewChild("Inf");
        if (Inf.IsNull())
        {
            continue;
        }

        Inf.SetAttribute("IP",itr->c_str());

    }

    strLocalCfg = xmlReqRouterInfo.GetXMLStrEx();

    WRITE_LOG(XTROUTER_ZL,ll_info,
        "************************�����ı���IP��Ϣ*******************************\n%s",strLocalCfg.c_str());
}


////////////////////////////////////////////////////////////////
//���� �� ���� GetLocalIpList 
//����    �ܡ� ��ȡ����IP�б�
//����������� void
//����������� std::list<std::string>& lstIP
//���� �� ֵ�� void
//����    �š� ����ר�Ҳ�
//����    �ߡ� XT_songlei
//���� �� �š� V1.0
//����    �ڡ�2014��6��19�� ������
//����    ʷ��
//........
////////////////////////////////////////////////////////////////
#ifdef _LINUX64
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#endif
void SpecialLine::GetLocalIpList(std::list<std::string>& lstIP)
{
#ifndef _LINUX64
    boost::asio::io_service service;
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
    boost::asio::ip::tcp::resolver resolver(service);
    boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);
    boost::asio::ip::tcp::resolver::iterator end;
    while (end != it)
    {
        boost::asio::ip::tcp::endpoint  p = *it;
        if (p.address().is_v4())
        {
            lstIP.push_back(p.address().to_v4().to_string());
        }
        it++;
    }
#else
    int i = 0;
    int sockfd;
    struct ifconf ifconf;
    unsigned char buf[512];
    struct ifreq* ifreq;
    char mac_addr[30];

    //��ʼ��ifconf
    ifconf.ifc_len = 512;
    ifconf.ifc_buf = (__caddr_t)buf;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    ioctl(sockfd, SIOCGIFCONF, &ifconf); //��ȡ���нӿ���Ϣ

    //������һ��һ���Ļ�ȡIP��ַ
    ifreq = (struct ifreq*)buf;
    for(i = (ifconf.ifc_len / sizeof(struct ifreq)); i > 0; i--) 
    {
        printf("name = [%s]\n", ifreq->ifr_name);
        printf("local addr = [%s]\n", inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr));
        //        printf("local netmask = [%s]\n", inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_netmask))->sin_addr));
        lstIP.push_back(inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr));

        ifreq++;
    }
#endif
}

////////////////////////////////////////////////////////////////
//���� �� ���� Analyze()
//����    �ܡ� ����XML��ר������
//����������� void
//����������� void
//���� �� ֵ�� void
//����    �š� ����ר�Ҳ�
//����    �ߡ� XT_songlei
//���� �� �š� V1.0
//����    �ڡ�2014��7��4�� ������
//����    ʷ��
//........
////////////////////////////////////////////////////////////////
void SpecialLine::Analyze()
{
    //ֻ�����ķ�����ר����Ϣ�Ž��н���
    if (!m_bUpate)
    {
        return;
    }

    //�����Ѿ���������XML
    m_bUpate = false;

    xtXml xmlRouterStat;
    {
        boost::lock_guard<boost::detail::spinlock> lock(m_xmlSPLineInfoMutex);
        xmlRouterStat = m_xmlSPLineInfo;
    }

    xtXmlNodePtr Ctrl = xmlRouterStat.getRoot();	

    if (Ctrl.IsNull())
    {
        return;
    }

    std::string strtype = Ctrl.GetAttribute("type");
    if ( 0 != strtype.compare("SetRouterInfo"))
    {
        WRITE_LOG(XTROUTER_ZL,ll_info,"���ķ�������[%s]ר����Ϣ�ݲ�����\n",strtype.c_str());
        return;		
    }

    //�����ʷ��Ϣ
    m_lstLineInfo.clear();

    SPLINEINFO SPLine;

    //���ݴ���
    xtXmlNodePtr Inf = Ctrl.GetFirstChild("Inf");
    for( ;!Inf.IsNull(); Inf = Inf.NextSibling("Inf"))
    { 
        //SIDS:  ר���е�Դ����IDS
        SPLine.m_strSIDS = Inf.GetAttribute("SIDS");

        //SName��ר���е�Դ��������
        SPLine.m_strSName = Inf.GetAttribute("SName");

        //SIP��  ���Ͷ�ԭʼIP
        SPLine.m_strSIP = Inf.GetAttribute("SIP");

        //SZIP�����Ͷ�ר��IP
        SPLine.m_strSZIP = Inf.GetAttribute("SZIP");

        //DIDS:  ר���е�Ŀ������IDS
        SPLine.m_strDIDS = Inf.GetAttribute("DIDS");

        //DName��ר���е�Ŀ����������
        SPLine.m_strDName = Inf.GetAttribute("DName");

        //DIP��  ���ն�ԭʼIP
        SPLine.m_strDIP = Inf.GetAttribute("DIP");

        //DZIP�����ն�ר��IP
        SPLine.m_strDZIP = Inf.GetAttribute("DZIP");

        //�ռ�ר����Ϣ
        m_lstLineInfo.push_back(SPLine);

    }

}

////////////////////////////////////////////////////////////////
//���� �� ���� GetBreakSPLine
//����    �ܡ� ��ȡ����ר��
//����������� void
//����������� std::list<SPLINEINFO>& lstBreakLine
//���� �� ֵ�� void
//����    �š� ����ר�Ҳ�
//����    �ߡ� XT_songlei
//���� �� �š� V1.0
//����    �ڡ�2014��7��5�� ������
//����    ʷ��
//........
////////////////////////////////////////////////////////////////
void SpecialLine::GetBreakSPLine(std::list<SPLINEINFO>& lstBreakLine)
{
    lstBreakLine.clear();
    lstBreakLine = m_lstBreakLine;
}

