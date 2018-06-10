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



// <mm c="RouterChannel" t="ʱ��" n="m_strLocalIP" d="m_sIDS_Center">
// <i>
// <id>ԴIDS</id>
// <ch>Դͨ����</ch>
// <ip>ԴIP��ַ</ip>
// <dc>�㲥ͨ����</dc>
// <dt>�㲥����</dt>
// <rc>����ͨ����</rc>
// <tm>ʱ���</tm>
// <tag>״̬���</tag>
// <to>
// <i>
// <id>Ŀ��IP</id>
// <ch>ͨ��</ch>
// <tm>ʱ���</tm>
// <tag>״̬���</tag>
// <lo>������</lo>
// <de>��ʱ</de>
// <sh>����</sh>    
// </i>
// </to>
// </i>
// </mm>
void StateMonitor::Monitor( std::string &strRet , int &playedFlag)
{
    //<mm c="RouterChannel" t="ʱ��" n="m_strLocalIP" d="m_sIDS_Center">
    xtXml xmlStateInfo;
    xtXmlNodePtr mm = xmlStateInfo.AddRoot("<mm/>");
    if (mm.IsNull())
    {
        return;
    }

    mm.SetAttribute("c","RouterChannel");

    //ʱ��
    std::string strTime = info_mgr::GetCurTime();
    std::stringstream os_t_v;
    os_t_v<<info_mgr::ToUnixTimestamp( strTime.c_str() );
    mm.SetAttribute("t", os_t_v.str());

    //��ȡ����IP
    std::string val = config::instance()->local_sndip("0.0.0.0");
    mm.SetAttribute("n",val.c_str());

    //����IDS
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
    int count = 0; //Defined for ����ͨ����

    for (; lstSrc.end() != itrSrc; ++itrSrc)
    {
        // <i>
        i = mm.NewChild("i");
        if (i.IsNull())
        {
            continue;
        }

        // <id>ԴIDS</id>
        xtXmlNodePtr idNode = i.NewChild("id");
        idNode.SetValue(itrSrc->device.dev_ids.c_str());

        // <ch>Դͨ����</ch>
        char chDevChanid[48] = "";
        memset(chDevChanid,0,sizeof(chDevChanid));
        sprintf_s(chDevChanid, sizeof(chDevChanid)-1, "%ld",itrSrc->device.dev_chanid);
        xtXmlNodePtr chNode = i.NewChild("ch");
        chNode.SetValue(chDevChanid);

        // <ip>ԴIP��ַ</ip>
        xtXmlNodePtr ipNode = i.NewChild("ip");
        ipNode.SetValue(itrSrc->device.db_url.c_str());

        // <dc>�㲥ͨ����</dc>
        char chDbChanid[48] = "";
        memset(chDbChanid,0,sizeof(chDbChanid));
        sprintf_s(chDbChanid, sizeof(chDbChanid)-1, "%ld",itrSrc->device.db_chanid);
        xtXmlNodePtr dcNode = i.NewChild("dc");
        dcNode.SetValue(chDbChanid);

        // <dt>�㲥����</dt>
        char chType[48] = "";
        memset(chType,0,sizeof(chType));
        sprintf_s(chType, sizeof(chDevChanid)-1, "%ld",itrSrc->device.db_type);
        xtXmlNodePtr dtNode = i.NewChild("dt");
        dtNode.SetValue(chType);

        // <rc>����ͨ����</rc>
        char chRCcount[32] = "";
        memset(chRCcount,0,sizeof(chRCcount));
        sprintf_s(chRCcount, sizeof(chRCcount)-1, "%d",count);
        xtXmlNodePtr rcNode = i.NewChild("rc");
        rcNode.SetValue(chRCcount);

        // <tm>ʱ���</tm>
        xtXmlNodePtr tmNode = i.NewChild("tm");
        char chTM[32] = "";
        memset(chTM,0,sizeof(chTM));
        sprintf_s( chTM, sizeof(chTM)-1, "%d",info_mgr::ToUnixTimestamp( info_mgr::ToStrMicrosecByPtime(itrSrc->create_time).c_str() ) );
        tmNode.SetValue(chTM);

        // <tag>״̬���</tag>
        xtXmlNodePtr tagNode = i.NewChild("tag");
        if (itrSrc->active) tagNode.SetValue("0");
        else tagNode.SetValue("1");


        // <i>
        // <id>Ŀ��IP</id>
        // <ch>ͨ��</ch>
        // <tm>ʱ���</tm>
        // <tag>״̬���</tag>
        // <lo>������</lo>
        // <de>��ʱ</de>
        // <sh>����</sh>    
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
                if ( itrTransInfo->m_lChID == count )  //�ж��Ƿ�Ϊ��ͬ�Ľ���ͨ��
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

                    //Ŀ��IP
                    xtXmlNodePtr idNode = iINto.NewChild("id");
                    idNode.SetValue(itrTransInfo->m_pszDestIP);

                    //Ŀ��˿ں�
                    char chDestPort[32] = "";
                    memset(chDestPort,0,sizeof(chDestPort));
                    sprintf_s(chDestPort,sizeof(chDestPort)-1, "%d",itrTransInfo->m_lDestPort);
                    xtXmlNodePtr chNode = iINto.NewChild("ch");
                    chNode.SetValue(chDestPort);

                    //ʱ���
                    xtXmlNodePtr tmNode = iINto.NewChild("tm");
                    char chTM[32] = "";
                    memset(chTM,0,sizeof(chTM));
                    sprintf_s( chTM, sizeof(chTM)-1, "%d",info_mgr::ToUnixTimestamp( itrTransInfo->m_pszCreateTime ) );
                    tmNode.SetValue(chTM);

                    //״̬���
                    xtXmlNodePtr tagNode = iINto.NewChild("tag");
                    if (itrSrc->active) tagNode.SetValue("0");
                    else tagNode.SetValue("1");

                    //������
                    char chFractionLost[1024] = "";
                    memset(chFractionLost,0,sizeof(chFractionLost));
                    sprintf_s(chFractionLost, sizeof(chFractionLost)-1, "%.2f",static_cast<float>(itrTransInfo->m_uiFractionLost) );
                    xtXmlNodePtr loNode = iINto.NewChild("lo");
                    loNode.SetValue( chFractionLost );

                    //��ʱ
                    char chDLSR[1024] = "";
                    memset(chDLSR,0,sizeof(chFractionLost));
                    sprintf_s(chDLSR, sizeof(chDLSR)-1, "%d",itrTransInfo->m_uiDlSR);
                    xtXmlNodePtr deNode = iINto.NewChild("de");
                    deNode.SetValue(chDLSR);

                    //����
                    char chJitter[1024] = "";
                    memset(chJitter,0,sizeof(chFractionLost));
                    sprintf_s(chJitter, sizeof(chJitter)-1, "%d",itrTransInfo->m_uiJitter);
                    xtXmlNodePtr shNode = iINto.NewChild("sh");
                    shNode.SetValue(chJitter);

                    //break;

                }

            }
        }

        count++; //Defined for ����ͨ����
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
