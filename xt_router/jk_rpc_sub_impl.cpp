#include "center_common_types.h"
#include "SpecialLine.h"
#include "XTRouter.h"
#include "router_task.h"
#include "InfoMgr.h"
#include "XTRouterLog.h"
#include "pri_jk_engine.h"


void JkMainClientRpcStub::OnLinkServerEvent(rpc::control_t&, LinkServerEvent_RequestType *request)
{
    std::cout<<"Test Rpc JkMainClientRpcStub::OnLinkServerEvent recved!"<<std::endl;
    if (!request)
    {
        return;
    }
    DEBUG_LOG(DBLOGINFO,ll_info," OnLinkServerEvent |num[%d] flag[%d]",request->get_num(),request->get_flag());

    // 从中心注销
    if (0 == request->get_flag())
    {
        //停止所有点播
        router_task_request_mgr::stop_all();
    }

    /*
    info_mgr::INFO_LINKSERVEREVENT info;
    info.m_strTime = info_mgr::GetCurTime();
    info.m_lsNum = request->get_num();
    info.m_lbz = request->get_flag();

    CInfoMgr::instance()->PostEventInfo(info_mgr::INFO_LINK_SEVER_EVENT,&info);*/
}

void JkMainClientRpcStub::OnUserLoginOrLogoutEvent(rpc::control_t&, UserLoginOrLogoutEvent_RequestType *request)
{
    if (!request)
    {
        return;
    }
    DEBUG_LOG(DBLOGINFO,ll_info,"OnUserLoginOrLogoutEvent |m_strName[%s] m_strIPS[%s] m_strIDS[%s] flag[%d]",
        request->get_name().c_str(),request->get_ips().c_str(),request->get_ids().c_str(),request->get_flag());

    // 状态监控用
    if (86 == request->get_type())
    {
        pri_jk_engine::_()->set_gateway(request->get_ids(), request->get_ips());
    }

    if (16 == request->get_type())
    {
        //保存中心通道的交换ids
        pri_jk_engine::_()->set_center_ids(request->get_ids());
#ifdef USE_SplecialLine_func
        //向中心定阅专线功能
        std::string  strInf;
        SpecialLine::instance()->GetLocalLineCfg(strInf);
        pri_jk_engine::_()->send_transparent_cmd_to_center(request->get_ids(), "", strInf);
#endif //#ifdef USE_SplecialLine_func
    }

    //设备下线处理
    if (0 == request->get_flag())
    {
        DEBUG_LOG(DBLOGINFO,ll_info,"dev logout stop ids[%s]\n",request->get_ids().c_str());
        //router_task_request_mgr::stop_play(request->get_ids());
        router_task_request_mgr::device_logout_pro(request->get_ids());
    }

#ifdef USE_INFO_MGR_
    //保存记录
    info_mgr::INFO_LOGINORLOGOUTCENTEREVNT infoEvent;
    infoEvent.m_strTime = info_mgr::GetCurTime();
    infoEvent.m_strIDS = request->get_ids();
    infoEvent.m_strName = request->get_name();
    infoEvent.m_strIPS = request->get_ips();
    infoEvent.m_strRes1 = request->get_str_res1();
    infoEvent.m_strRes2 = request->get_str_res1();
    infoEvent.m_lType = request->get_type();	
    infoEvent.m_lFlag = request->get_flag();
    infoEvent.m_lRes1 = request->get_res1();
    infoEvent.m_lRes2 = request->get_res2();

    CInfoMgr::instance()->PostEventInfo(info_mgr::INFO_LOGIN_OR_LOGOUT_CENTER_EVNT_ID,&infoEvent);
#endif //#ifdef USE_INFO_MGR_

}

void JkMainClientRpcStub::OnDBImageCenterEvent(rpc::control_t&, DBImageCenterEvent_RequestType *request)
{
	WRITE_LOG(DBLOGINFO,ll_info, "%s", "OnDBImageCenterEvent");
    if (!request)
    {
        return;
    }

    //////////////////////////////////////////////////////////////////////////
    long ctrl = request->get_flag();				// 中心指令
    const std::string& dev_ids = request->get_ids();			// 设备IDS	
    long dev_chid = request->get_ch();				// 设备通道号
    long dev_strmtype = request->get_dest_aport();	// 设备码流类型
    const std::string& db_ip = request->get_ips();				// 点播IP 
    long db_chanid = request->get_res1();			// 点播通道	
    long dev_type = request->get_local_vport();		// 设备类型
    long chanid = request->get_dest_vport();		// 指定转发服务通道号
    const std::string& ondb_sn = request->get_str_res1();	// 设备新IDS
    long dev_chid_new = request->get_res1();		// 设备新通道号
    long link_type = request->get_dbmode();			// 连接类型
    //////////////////////////////////////////////////////////////////////////
    DEBUG_LOG(DBLOGINFO,ll_info,
        "OnDBImageCenter:中心指令:中心指令[%d]设备IDS[%s]设备通道号[%d]设备码流类型[%d]点播IP[%s]点播通道[%d]设备类型[%d]指定转发服务通道号[%d] ondb_sn[%s]设备新通道号[%d]连接类型[%d]",
        ctrl,dev_ids.c_str(),dev_chid,dev_strmtype,db_ip.c_str(),db_chanid,dev_type,chanid,ondb_sn.c_str(),dev_chid_new,link_type);

    switch (ctrl)
    {
    case CT_STARTDB:
        {
            router_task_request_mgr::start_play(ondb_sn,dev_ids, dev_chid, dev_strmtype, "0.0.0.0",db_ip, db_chanid, dev_type, chanid, link_type);
            break;
        }

    case CT_UPDATEIDS:
        {
            router_task_request_mgr::update_ids(dev_ids, dev_chid, dev_strmtype, ondb_sn, dev_chid_new);
            break;
        }

    case CT_STOPDB:
        {
            router_task_request_mgr::stop_play(ondb_sn,dev_ids, dev_chid, dev_strmtype);
            break;
        }

    case CT_STOPALLDB:
        {
            router_task_request_mgr::stop_all();
            break;
        }
    default:
        break;
    }
}

//中心发过来专线的配置信息
void JkMainClientRpcStub::OnGetMsgEvent(rpc::control_t&, OnGetMsgEvent_RequestType *request)
{
    std::string strRouterSet = request->get_info();
    long lInfoSize = request->get_info_size();

    //保存专线配置信息
    SpecialLine::instance()->ChangeRouterSet(strRouterSet.c_str(),lInfoSize);
}

//rpc server端挂了之后会调用这个函数
void JkMainClientRpcStub::on_process_exit()
{
    std::cout<<"JKMainClientProxy exit!"<<std::endl;
    DEBUG_LOG(DBLOGINFO,ll_info,"JKMainClientProxy exit!");
}

//通知交换自己的ids
void JkMainClientRpcStub::OnTellLocalIDSEvent(rpc::control_t&, OnTellLocalIDSEvent_RequestType *request)
{

    if (NULL != request)
    {
        DEBUG_LOG(DBLOGINFO,ll_info,"OnTellLocalIDSEvent:ids[%s] sRes1[%s] iRes1[%d]",request->get_local_ids().c_str(),request->get_str_res1().c_str(),request->get_ires1()); 
        pri_jk_engine::_()->set_local_ids(request->get_local_ids());

        std::string ids = pri_jk_engine::_()->get_local_ids();
        router_task_request_mgr::regist(ids, CXTRouter::Instance()->m_config.local_ip, CXTRouter::Instance()->m_config.regist_bind,
            CXTRouter::Instance()->m_config.regist_ip, CXTRouter::Instance()->m_config.regist_port);
    }
}

//透传指令
void JkMainClientRpcStub::OnTransparentCommandEvent(rpc::control_t&,OnTransparentCommandEvent_RequestType *rq)
{
    if (NULL != rq)
    {
        std::string src_ids = rq->get_ids();
        std::string cmd = rq->get_command(); 
        DEBUG_LOG(DBLOGINFO,ll_info,"OnTransparentCommandEvent: ids[%s] cmd[%s]",src_ids.c_str(),cmd.c_str());
        router_task_request_mgr::transparent_cmd_pro(src_ids,cmd);
    }
}