#include "FuncEx.h"
#include <stdlib.h>
#include <stdio.h>
#include "SpecialLine.h"
#include "break_monitor.h"
#include "StateMonitor.h"
#include "router_task.h"
#include "center_common_types.h"
#include "InfoMgr.h"
#include "Router_config.h"
#include "XTRouterLog.h"
#include "XTRouter.h"
#include "pri_jk_engine.h"

#ifndef	_WIN32
#define sprintf_s snprintf
#endif

CFuncEx CFuncEx::m_Obj;

CFuncEx::CFuncEx(void)
{	
}

CFuncEx::~CFuncEx(void)
{
}

int CFuncEx::InitFuncEx()
{
    int iRet =0;
    do 
    {
#ifdef USE_SplecialLine_func
        //专线保障
        m_pZlineTask = new ZLineTask;
        if (!m_pZlineTask)
        {
            iRet = -1;
            break;
        }
        m_pZlineTask->signal();
#endif //#ifdef USE_SplecialLine_func

#ifdef USE_BreakMonitor_FUNC_
        //断线监测
        m_pBreakMonitor = new BreakMonitorTask;
        if (!m_pBreakMonitor)
        {
            iRet = -1;
            break;
        }
        if (break_monitor_mgr::_()->init(CXTRouter::_()->get_router_cfg().chan_num) < 0)
        {
            std::cout<<"break_monitor_mgr init fail!"<<std::endl;
        }
        else
        {
            m_pBreakMonitor->signal();
        }
#endif //#ifdef USE_BreakMonitor_FUNC_
        //状态监控
        m_pStateMonitor = new StateMonitorTask;
        if (!m_pStateMonitor)
        {
            iRet = -1;
            break;
        }
        m_pStateMonitor->signal();

    } while (0);

    return iRet;

}
int CFuncEx::UnitFuncEx()
{
    int iRet=0;

#ifdef USE_SplecialLine_func
    //专线保障
    if (m_pZlineTask)
    {
        m_pZlineTask->cancel();
        delete m_pZlineTask;
        m_pZlineTask = NULL;
    }
#endif //#ifdef USE_SplecialLine_func

#ifdef USE_BreakMonitor_FUNC_
    //断线监测
    if (m_pBreakMonitor)
    {
        m_pBreakMonitor->cancel();
        delete m_pBreakMonitor;
        m_pBreakMonitor = NULL;
    }
#endif //#ifdef USE_BreakMonitor_FUNC_
    //状态监控
    if (m_pStateMonitor)
    {
        m_pStateMonitor->cancel();
        delete m_pStateMonitor;
        m_pStateMonitor = NULL;
    }
    return iRet;

}

uint32_t BreakMonitorTask::run()
{
    if (0 < config::_()->break_monitor_onoff(0))
    {
        do
        {
            std::list<src_info> srcs;
            std::map<long,long> strmids;

            XTEngine::_()->get_all_src(srcs);
            if (srcs.empty()) break;

            std::list<src_info>::iterator itr_src;
            for (itr_src = srcs.begin(); srcs.end() != itr_src; ++itr_src)
            {
                if (!itr_src->active) 
                {
                    continue;
                }

                if (strmids.find(itr_src->device.strmid)!=strmids.end())
                {
                    strmids[itr_src->device.strmid]++;
                    continue;
                }

                if (!break_monitor_mgr::_()->get_stream_state(itr_src->device.strmid))
                {
                    if (!itr_src->active) continue;
                    DEBUG_LOG(DBLOGINFO,ll_info,
                        "断线监测检测出有没有数据的通道清除接收与转发资源并反馈中心点播失败 ids[%s] ch[%d] strmtype[%d] strmid[%d]  srcno[%d]",
                        itr_src->device.dev_ids.c_str(),itr_src->device.dev_chanid,itr_src->device.dev_strmtype,itr_src->device.strmid,itr_src->srcno);

                    router_task_request_mgr::stop_play("",itr_src->device.dev_ids,itr_src->device.dev_chanid,itr_src->device.dev_strmtype);
                }
                else
                {
                    strmids[itr_src->device.strmid] = 1;
                    break_monitor_mgr::_()->update_stream_state(itr_src->device.strmid,false);
                }
            }
        } while (0);
    }
    return config::_()->break_monitor_time_interval(30000);
}

void BreakMonitorTask::SaveCtrlInfo(const src_info& src,const long lChId,long lRet)
{
    //收集执行点播信息
    info_mgr::INFO_SIGNALINGEXECRESULT info;
    info.m_strTime = info_mgr::GetCurTime();
    info.m_strCtrlName = "断线监测停点无数据转发通道";
    info.m_strDevIds = src.device.dev_ids;

    info.m_strDbIP = src.device.db_url;
    info.m_lServerType = 9;
    info.m_lExecRet = lRet;
    info.m_lDevStrmType = src.device.dev_strmtype;
    info.m_lTransChId = lChId;
    CInfoMgr::instance()->PostEventInfo(info_mgr::INFO_SIGNALING_EXEC_RESULT_ID,&info);

}

uint32_t ZLineTask::run()
{
    //TEST
    ////////////////////////////////////////////////////
    // 		char* strRouterSetInfo =
    // 	 		"<Ctrl type=\"SetRouterInfo\" Num=\"5\">\n"
    // 	 		"<Inf SIDS=\"b1\" SName=\"广州S区\" SIP=\"172.16.9.13\" SZIP= \"172.16.9.12\" DIDS=\"x1\" DName=\"武汉A区\" DIP=\"172.16.9.231\" DZIP=\"5.0.0.1\" />\n"
    // 	 		"<Inf SIDS=\"Z2\" SName=\"昆明C区\" SIP=\"177.0.5.2\" SZIP= \"172.16.9.13\" DIDS=\"x1\" DName=\"武汉A区\" DIP=\"172.16.9.231\" DZIP=\"5.0.0.1\" />\n"
    // 	 		"<Inf SIDS=\"Z2\" SName=\"北京A区\" SIP=\"172.16.9.145\" SZIP= \"172.16.9.14\" DIDS=\"x1\" DName=\"武汉A区\" DIP=\"172.16.9.231\" DZIP=\"5.0.0.1\" />\n"
    // 	 		"<Inf SIDS=\"Z2\" SName=\"长沙D区\" SIP=\"172.16.5.58\" SZIP= \"172.16.9.15\" DIDS=\"x1\" DName=\"武汉A区\" DIP=\"172.16.9.231\" DZIP=\"5.0.0.1\" />\n"
    // 	 		"<Inf SIDS=\"Z2\" SName=\"恩施E区\" SIP=\"177.0.5.44\" SZIP= \"172.166.9.16\" DIDS=\"x1\" DName=\"武汉A区\" DIP=\"172.16.9.231\" DZIP=\"5.0.0.1\" />\n"
    // 	 		"</Ctrl>\n";
    // 	 
    // 	 	char* strRouterSetInfo =
    // 	 		"<Ctrl type=\"SetRouterInfo\" Num=\"5\">\n"
    // 	 		"<Inf SIDS=\"b1\" SName=\"广州S区\" SIP=\"172.16.9.13\" SZIP= \"172.16.9.13\" DIDS=\"x1\" DName=\"武汉A区\" DIP=\"172.16.9.231\" DZIP=\"5.0.0.1\" />\n"
    // 	 		"</Ctrl>\n";
    // 	 
    // 	 SpecialLine::instance()->ChangeRouterSet(strRouterSetInfo,1024);
    ///////////////////////////////////////////////////
    uint32_t uiTime = 10000;

    //检测专线状态
    std::string strRet;
    SpecialLine::instance()->Check(strRet);

    //给中心反馈
    if (!strRet.empty())
    {
        //清理断开专线上的点播
        ClearBreakLinePlayInfo();

        std::string strCenterIds = pri_jk_engine::_()->get_center_ids();

        if (!pri_jk_engine::_()->
            send_transparent_cmd_to_center(strCenterIds,"",strRet))
        {
            //std::cout<<"发送中心专线信息失败！"<<std::endl;
        }
    }
    return uiTime;
}

void ZLineTask::ClearBreakLinePlayInfo()
{
    //获取断线专线信息
    std::list<SPLINEINFO> lstBreakLine;
    SpecialLine::instance()->GetBreakSPLine(lstBreakLine);

    //清除点播
    std::list<SPLINEINFO>::iterator itr = lstBreakLine.begin();
    for (; lstBreakLine.end() != itr; ++itr)
    {
        WRITE_LOG(XTROUTER_ZL,ll_info,"专线断开停点专线上的点播IP[%s]",itr->m_strSZIP.c_str());

        //根据IP停点对应点播
        XTEngine::instance()->stop_play_ip(itr->m_strSZIP);
    }
}

uint32_t StateMonitorTask::run()
{
    int iSwitch     = config::instance()->status_monitor_switch(1); 

    uint32_t uiTime = config::instance()->status_monitor_frequency(10000); 

    if ( 1 == iSwitch ) {

        this->OnStateMonitor();

    }

    return uiTime;
}

int StateMonitorTask::MsgID = 1;

int StateMonitorTask::GetMsgID()
{
    if ( 1000 == MsgID )
        MsgID = 1;

    return MsgID++;
}

void StateMonitorTask::BreakDownToMany(const std::string &origMsg, std::vector<std::string> &vMsgList, int limit)
{
    if (origMsg.empty())
        return;

    std::vector<std::string>().swap(vMsgList);

    char szDividedMsg[950] = "";
    int count              = 0;  
    int dividedSum         = 0;           //Present the number of the divided fragments 
    int MsgID              = GetMsgID();  //Mark a ID number for each divided fragments.
    std::size_t MsgIndex           = 0;           //Point to the current position in the message.

    //Get divided sum.
    for ( std::size_t i = 0 ; i < origMsg.size() - 1 ; i += limit )
    {
        dividedSum++;
    }

    for ( ; MsgIndex < origMsg.size() - 1 ; MsgIndex += limit )
    {
        std::string strTemp = origMsg.substr(MsgIndex,limit);
        sprintf_s(szDividedMsg, sizeof(szDividedMsg)-1, "<mm c=\"RouterSubstr\" g=\"%d\"  s=\"%d\" d=\"%d\" n=\"%s\"></mm>",MsgID,dividedSum,++count,strTemp.c_str());

        std::string s(szDividedMsg);
        vMsgList.push_back(s);
    } 
}

bool StateMonitorTask::CheckSameMsgWithLastOne(const std::string &newMsg,const std::string &oldMsg)
{
    if ( oldMsg.empty() )
        return false;

    bool bFlag = false; 

    for ( std::size_t i = 0; i < newMsg.size() - 1; i++ )
    {
        if ( '>' == newMsg[i] ) //Match to the first '>' character,in order to remove the header of the message. 
        {
            for ( std::size_t j = 0; j < oldMsg.size() - 1; j++ )
            {
                if ( '>' == oldMsg[j] )
                {
                    if ( newMsg.substr(i+1) == oldMsg.substr(j+1) ) //Check whether the new message is the same as the old one after removing the header info.
                        return bFlag = true;

                    break;
                }			   
            }
            break; //Only match to the first '>' character at the end of the string header
            //"<mm c=\"RouterChannel\" t=\"1434335016\" n=\"172.16.2.111\" d=\"Rv00000000000000\">..."
        }
    }

    return bFlag;
}

void StateMonitorTask::FilterSpaceCharFromMsg(const std::string &origMsg , std::string &newMsg)
{
    int filterSwitch = 0;  
    for( std::size_t i = 0; i < origMsg.size() - 1; i++ )
    {
        if ( '<' == origMsg[i] )  filterSwitch = 0;

        if ( 1 == filterSwitch && ( ' ' == origMsg[i] || '\n' == origMsg[i] ) )  continue;

        if ( '>' == origMsg[i] )  filterSwitch = 1;

        newMsg.push_back(origMsg[i]);
    }
}

void StateMonitorTask::OnStateMonitor()
{
    int playedFlag           = 0;
    std::string strStateInfo = "";

    StateMonitor::instance()->Monitor(strStateInfo,playedFlag); 

    if ( strStateInfo.empty() )
        return;

    if ( 0 == playedFlag ) //No play message returns
        return;

    if ( CheckSameMsgWithLastOne( strStateInfo,StateMonitor::instance()->getOldStatusMessage() ) ) //Avoid to send same message content with last one.
        return;

    std::string strStateInfoFilter = ""; 
    FilterSpaceCharFromMsg(strStateInfo,strStateInfoFilter);

    //break down message to several fragments,each fragment is no more than 880 bytes. 
    std::vector<std::string> vMsgList; 
    BreakDownToMany(strStateInfoFilter,vMsgList);

    if ( vMsgList.empty() )
        return;

    std::string strGatewayIds = pri_jk_engine::_()->get_gateway_ids();

    std::vector<std::string>::iterator iteMsg = vMsgList.begin();
    for( ; iteMsg != vMsgList.end(); iteMsg++ )
    {
        if ( !pri_jk_engine::_()->
            send_transparent_cmd_to_center(strGatewayIds,"",*iteMsg) )
        {
            //std::cout<<"发送中心专线信息失败！"<<std::endl;
        }	
    } 

    StateMonitor::instance()->setOldStatusMessage(strStateInfo); //Set the current status message as an old one.
}
