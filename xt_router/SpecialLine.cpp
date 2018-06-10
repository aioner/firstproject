#include "SpecialLine.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include<boost/asio/io_service.hpp>
#include <boost/asio.hpp>
#include "xt_ping/icmp_ping.h"
#include <boost/thread/lock_guard.hpp>
#include "Router_config.h"
#include "XTRouterLog.h" 

#define  STAT_ON_LINE 1 //在线
#define  STAT_BREAK 0  //断线

//static
SpecialLine SpecialLine::m_ins;

////////////////////////////////////////////////////////////////
//【函 数 名】 SpecialLine
//【功    能】 构造
//【输入参数】 void
//【输出参数】 void
//【部    门】 技术专家部
//【作    者】 XT_songlei
//【版 本 号】 V1.0
//【日    期】2014年6月12日 星期四
//【历    史】
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
//【函 数 名】 SpecialLine
//【功    能】 析构
//【输入参数】 void
//【输出参数】 void
//【部    门】 技术专家部
//【作    者】 XT_songlei
//【版 本 号】 V1.0
//【日    期】2014年6月12日 星期四
//【历    史】
//........
////////////////////////////////////////////////////////////////
SpecialLine::~SpecialLine(void)
{

}

////////////////////////////////////////////////////////////////
//【函 数 名】 CheckState
//【功    能】 检测专线状态
//【输入参数】 const char* strIP-:目地IP
//【输出参数】 void
//【返 回 值】 int -:0-:在线 -1-：断线
//【部    门】 技术专家部
//【作    者】 XT_songlei
//【版 本 号】 V1.0
//【日    期】2014年6月11日 星期三
//【历    史】
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
//【函 数 名】 GetCheckTime()
//【功    能】 获取当前时间
//【输入参数】 void
//【输出参数】 void
//【返 回 值】 std::string -：时间字符串
//【部    门】 技术专家部
//【作    者】 XT_songlei
//【版 本 号】 V1.0
//【日    期】2014年6月11日 星期三
//【历    史】
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
//【函 数 名】 ChangeRouterSet
//【功    能】 更新交换配置
//【输入参数】 const char *pstrRouterSet-：XML格式的配置信息
//             const long nSize-:信息XML串的长度
//【输出参数】 void
//【返 回 值】 void
//【部    门】 技术专家部
//【作    者】 XT_songlei
//【版 本 号】 V1.0
//【日    期】2014年6月11日 星期三
//【历    史】
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
        "************************收到中心发过来的专线信息*******************************\n%s",xmlTmp.GetXMLStrEx());

}


////////////////////////////////////////////////////////////////
//【函 数 名】 Check
//【功    能】 专线检测
//【输入参数】 void
//【输出参数】 CString& strRetSPLine
//【返 回 值】 void
//【部    门】 技术专家部
//【作    者】 XT_songlei
//【版 本 号】 V1.0
//【日    期】2014年6月11日 星期三
//【历    史】
//........
////////////////////////////////////////////////////////////////
void SpecialLine::Check(std::string& strRetSPLine)
{
    //清空输出
    strRetSPLine.clear();

    //解析中心发过来的专线信息
    Analyze();

    if (m_lstLineInfo.empty())
    {
        return;
    }

    //构造XML
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
        //检测专线状态
        iStat = CheckState(itr->m_strSZIP.c_str()) < 0 ? STAT_BREAK : STAT_ON_LINE;

        //如果状态没有发生变化不用反馈
        if (iStat == itr->m_iStat && -1 != itr->m_iStat )
        {
            continue;
        }

        Info = HeadInfo.NewChild("Inf");		

        // SIDS:专线中的源中心IDS
        Info.SetAttribute("SIDS",itr->m_strSIDS.c_str());

        // SIP：发送端原始IP
        Info.SetAttribute("SIP",itr->m_strSIP.c_str());

        //SZIP：发送端专线IP
        Info.SetAttribute("SZIP",itr->m_strSZIP.c_str());

        // DIDS: 专线中的目的中心IDS
        Info.SetAttribute("DIDS",itr->m_strDIDS.c_str());

        //DIP：接收端原始IP
        Info.SetAttribute("DIP",itr->m_strDIP.c_str());

        //DZIP：接收端专线IP
        Info.SetAttribute("DZIP",itr->m_strDZIP.c_str());

        //Stat：链接状态 (0:断线，1：在线)
        itr->m_iStat = iStat;
        Info.SetAttribute("stat",itr->m_iStat);

        //Time：最后一次状态变化时间 (年-月-日，时:分:秒) 视频交换机上的时间（以便查询）
        strTime = GetCheckTime();
        Info.SetAttribute("Time",strTime.c_str());

        ++iNum;

        //收集已经断线的专线
        if (0 == iStat)
        {
            m_lstBreakLine.push_back(*itr);
        }		
    }

    HeadInfo.SetAttribute("Num",iNum);

    //反馈
    strRetSPLine = xmlSPLineStatInfo.GetXMLStrEx();


    WRITE_LOG(XTROUTER_ZL,ll_info,
        "************************检测出的专线状态信息*******************************\n%s",strRetSPLine.c_str());
}


////////////////////////////////////////////////////////////////
//【函 数 名】 GetLocalLineCfg
//【功    能】 获取本地专线配置作息
//【输入参数】 void
//【输出参数】 CString& strLocalCfg -:专线配置信息
//【返 回 值】 void
//【部    门】 技术专家部
//【作    者】 XT_songlei
//【版 本 号】 V1.0
//【日    期】2014年6月19日 星期四
//【历    史】
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
        "************************检测出的本地IP信息*******************************\n%s",strLocalCfg.c_str());
}


////////////////////////////////////////////////////////////////
//【函 数 名】 GetLocalIpList 
//【功    能】 获取本机IP列表
//【输入参数】 void
//【输出参数】 std::list<std::string>& lstIP
//【返 回 值】 void
//【部    门】 技术专家部
//【作    者】 XT_songlei
//【版 本 号】 V1.0
//【日    期】2014年6月19日 星期四
//【历    史】
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

    //初始化ifconf
    ifconf.ifc_len = 512;
    ifconf.ifc_buf = (__caddr_t)buf;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    ioctl(sockfd, SIOCGIFCONF, &ifconf); //获取所有接口信息

    //接下来一个一个的获取IP地址
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
//【函 数 名】 Analyze()
//【功    能】 解析XML到专线数据
//【输入参数】 void
//【输出参数】 void
//【返 回 值】 void
//【部    门】 技术专家部
//【作    者】 XT_songlei
//【版 本 号】 V1.0
//【日    期】2014年7月4日 星期五
//【历    史】
//........
////////////////////////////////////////////////////////////////
void SpecialLine::Analyze()
{
    //只有中心发过来专线信息才进行解析
    if (!m_bUpate)
    {
        return;
    }

    //中心已经发过来的XML
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
        WRITE_LOG(XTROUTER_ZL,ll_info,"中心发过来的[%s]专线信息暂不处理\n",strtype.c_str());
        return;		
    }

    //清空历史信息
    m_lstLineInfo.clear();

    SPLINEINFO SPLine;

    //数据处理
    xtXmlNodePtr Inf = Ctrl.GetFirstChild("Inf");
    for( ;!Inf.IsNull(); Inf = Inf.NextSibling("Inf"))
    { 
        //SIDS:  专线中的源中心IDS
        SPLine.m_strSIDS = Inf.GetAttribute("SIDS");

        //SName：专线中的源中心名称
        SPLine.m_strSName = Inf.GetAttribute("SName");

        //SIP：  发送端原始IP
        SPLine.m_strSIP = Inf.GetAttribute("SIP");

        //SZIP：发送端专线IP
        SPLine.m_strSZIP = Inf.GetAttribute("SZIP");

        //DIDS:  专线中的目的中心IDS
        SPLine.m_strDIDS = Inf.GetAttribute("DIDS");

        //DName：专线中的目的中心名称
        SPLine.m_strDName = Inf.GetAttribute("DName");

        //DIP：  接收端原始IP
        SPLine.m_strDIP = Inf.GetAttribute("DIP");

        //DZIP：接收端专线IP
        SPLine.m_strDZIP = Inf.GetAttribute("DZIP");

        //收集专线信息
        m_lstLineInfo.push_back(SPLine);

    }

}

////////////////////////////////////////////////////////////////
//【函 数 名】 GetBreakSPLine
//【功    能】 获取断线专线
//【输入参数】 void
//【输出参数】 std::list<SPLINEINFO>& lstBreakLine
//【返 回 值】 void
//【部    门】 技术专家部
//【作    者】 XT_songlei
//【版 本 号】 V1.0
//【日    期】2014年7月5日 星期六
//【历    史】
//........
////////////////////////////////////////////////////////////////
void SpecialLine::GetBreakSPLine(std::list<SPLINEINFO>& lstBreakLine)
{
    lstBreakLine.clear();
    lstBreakLine = m_lstBreakLine;
}

