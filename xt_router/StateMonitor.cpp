#include <stdio.h>

#include "StateMonitor.h"
#include "XTEngine.h"
#include "Router_config.h"
#include "InfoTypeDef.h"
#include "pri_jk_engine.h"

#ifndef	_WIN32
#define sprintf_s snprintf
#endif

StateMonitor StateMonitor::m_Obj;

StateMonitor::StateMonitor(void)
{
    this->strOldStatusMessage = "";
}

StateMonitor::~StateMonitor(void)
{
}



// <mm c="RouterChannel" t="时间" n="m_strLocalIP" d="m_sIDS_Center">
// <i>
// <id>源IDS</id>
// <ch>源通道号</ch>
// <ip>源IP地址</ip>
// <dc>点播通道号</dc>
// <dt>点播类型</dt>
// <rc>交换通道号</rc>
// <tm>时间戳</tm>
// <tag>状态标记</tag>
// <to>
// <i>
// <id>目的IP</id>
// <ch>通道</ch>
// <tm>时间戳</tm>
// <tag>状态标记</tag>
// <lo>丢包率</lo>
// <de>延时</de>
// <sh>抖动</sh>    
// </i>
// </to>
// </i>
// </mm>
void StateMonitor::Monitor( std::string &strRet , int &playedFlag)
{
    //<mm c="RouterChannel" t="时间" n="m_strLocalIP" d="m_sIDS_Center">
    xtXml xmlStateInfo;
    xtXmlNodePtr mm = xmlStateInfo.AddRoot("<mm/>");
    if (mm.IsNull())
    {
        return;
    }

    mm.SetAttribute("c","RouterChannel");

    //时间
    std::string strTime = info_mgr::GetCurTime();
    std::stringstream os_t_v;
    os_t_v<<info_mgr::ToUnixTimestamp( strTime.c_str() );
    mm.SetAttribute("t", os_t_v.str());

    //获取交换IP
    std::string val = config::instance()->local_sndip("0.0.0.0");
    mm.SetAttribute("n",val.c_str());

    //中心IDS
    std::string strCenterIds = pri_jk_engine::_()->get_center_ids();
    mm.SetAttribute("d",strCenterIds.c_str());


    std::list<src_info> lstSrc;
    XTEngine::instance()->get_all_src_1(lstSrc);

    if ( lstSrc.size() <= 0 )
    {
        return;
    }

    playedFlag = 1; //Set played flag to be true
    std::list<src_info>::iterator itrSrc = lstSrc.begin();
    xtXmlNodePtr i;
    int count = 0; //Defined for 交换通道号

    for (; lstSrc.end() != itrSrc; ++itrSrc)
    {
        // <i>
        i = mm.NewChild("i");
        if (i.IsNull())
        {
            continue;
        }

        // <id>源IDS</id>
        xtXmlNodePtr idNode = i.NewChild("id");
        idNode.SetValue(itrSrc->device.dev_ids.c_str());

        // <ch>源通道号</ch>
        char chDevChanid[48] = "";
        memset(chDevChanid,0,sizeof(chDevChanid));
        sprintf_s(chDevChanid, sizeof(chDevChanid)-1, "%ld",itrSrc->device.dev_chanid);
        xtXmlNodePtr chNode = i.NewChild("ch");
        chNode.SetValue(chDevChanid);

        // <ip>源IP地址</ip>
        xtXmlNodePtr ipNode = i.NewChild("ip");
        ipNode.SetValue(itrSrc->device.db_url.c_str());

        // <dc>点播通道号</dc>
        char chDbChanid[48] = "";
        memset(chDbChanid,0,sizeof(chDbChanid));
        sprintf_s(chDbChanid, sizeof(chDbChanid)-1, "%ld",itrSrc->device.db_chanid);
        xtXmlNodePtr dcNode = i.NewChild("dc");
        dcNode.SetValue(chDbChanid);

        // <dt>点播类型</dt>
        char chType[48] = "";
        memset(chType,0,sizeof(chType));
        sprintf_s(chType, sizeof(chDevChanid)-1, "%ld",itrSrc->device.db_type);
        xtXmlNodePtr dtNode = i.NewChild("dt");
        dtNode.SetValue(chType);

        // <rc>交换通道号</rc>
        char chRCcount[32] = "";
        memset(chRCcount,0,sizeof(chRCcount));
        sprintf_s(chRCcount, sizeof(chRCcount)-1, "%d",count);
        xtXmlNodePtr rcNode = i.NewChild("rc");
        rcNode.SetValue(chRCcount);

        // <tm>时间戳</tm>
        xtXmlNodePtr tmNode = i.NewChild("tm");
        char chTM[32] = "";
        memset(chTM,0,sizeof(chTM));
        sprintf_s( chTM, sizeof(chTM)-1, "%d",info_mgr::ToUnixTimestamp( info_mgr::ToStrMicrosecByPtime(itrSrc->create_time).c_str() ) );
        tmNode.SetValue(chTM);

        // <tag>状态标记</tag>
        xtXmlNodePtr tagNode = i.NewChild("tag");
        if (itrSrc->active) tagNode.SetValue("0");
        else tagNode.SetValue("1");


        // <i>
        // <id>目的IP</id>
        // <ch>通道</ch>
        // <tm>时间戳</tm>
        // <tag>状态标记</tag>
        // <lo>丢包率</lo>
        // <de>延时</de>
        // <sh>抖动</sh>    
        // </i>
        // </to>
        // </i>
        std::list<info_mgr::INFO_TRANS> lstTransInfoOut;
        CInfoMgr::instance()->GetConnectInfo(lstTransInfoOut);

        if ( lstTransInfoOut.size() > 0 )
        {
            std::list<info_mgr::INFO_TRANS>::iterator itrTransInfo = lstTransInfoOut.begin();

            for ( ; lstTransInfoOut.end() != itrTransInfo; itrTransInfo++ )
            {
                if ( itrTransInfo->m_lChID == count )  //判断是否为相同的交换通道
                {
                    // <to>
                    xtXmlNodePtr to;
                    to = i.NewChild("to");
                    if (to.IsNull())
                    {
                        continue;
                    }

                    xtXmlNodePtr iINto;
                    iINto = to.NewChild("i");
                    if (iINto.IsNull())
                    {
                        continue;
                    }

                    //目的IP
                    xtXmlNodePtr idNode = iINto.NewChild("id");
                    idNode.SetValue(itrTransInfo->m_pszDestIP);

                    //目标端口号
                    char chDestPort[32] = "";
                    memset(chDestPort,0,sizeof(chDestPort));
                    sprintf_s(chDestPort,sizeof(chDestPort)-1, "%d",itrTransInfo->m_lDestPort);
                    xtXmlNodePtr chNode = iINto.NewChild("ch");
                    chNode.SetValue(chDestPort);

                    //时间戳
                    xtXmlNodePtr tmNode = iINto.NewChild("tm");
                    char chTM[32] = "";
                    memset(chTM,0,sizeof(chTM));
                    sprintf_s( chTM, sizeof(chTM)-1, "%d",info_mgr::ToUnixTimestamp( itrTransInfo->m_pszCreateTime ) );
                    tmNode.SetValue(chTM);

                    //状态标记
                    xtXmlNodePtr tagNode = iINto.NewChild("tag");
                    if (itrSrc->active) tagNode.SetValue("0");
                    else tagNode.SetValue("1");

                    //丢包率
                    char chFractionLost[1024] = "";
                    memset(chFractionLost,0,sizeof(chFractionLost));
                    sprintf_s(chFractionLost, sizeof(chFractionLost)-1, "%.2f",static_cast<float>(itrTransInfo->m_uiFractionLost) );
                    xtXmlNodePtr loNode = iINto.NewChild("lo");
                    loNode.SetValue( chFractionLost );

                    //延时
                    char chDLSR[1024] = "";
                    memset(chDLSR,0,sizeof(chFractionLost));
                    sprintf_s(chDLSR, sizeof(chDLSR)-1, "%d",itrTransInfo->m_uiDlSR);
                    xtXmlNodePtr deNode = iINto.NewChild("de");
                    deNode.SetValue(chDLSR);

                    //抖动
                    char chJitter[1024] = "";
                    memset(chJitter,0,sizeof(chFractionLost));
                    sprintf_s(chJitter, sizeof(chJitter)-1, "%d",itrTransInfo->m_uiJitter);
                    xtXmlNodePtr shNode = iINto.NewChild("sh");
                    shNode.SetValue(chJitter);

                    //break;

                }

            }
        }

        count++; //Defined for 交换通道号
    }
    strRet = xmlStateInfo.GetXMLStrEx();
}


void StateMonitor::setOldStatusMessage(std::string strStatusMessage)
{
    if ( !strStatusMessage.empty() )
    {
        this->strOldStatusMessage = strStatusMessage;
    }
}

std::string StateMonitor::getOldStatusMessage()
{
    return this->strOldStatusMessage;
}
