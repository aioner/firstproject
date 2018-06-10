#include "JkMainClientRpcServer.h"
#include "JKMainClientLog.h"
#include <iosfwd>

bool JkMainClientRpcProxy::SetServerInfo(rpc::control_t&, SetServerInfo_RequestType *request, SetServerInfo_ResponseType *, rpc::closure_t *)
{
    std::ostringstream oss;
   oss << "SetServerInfo:num=" << request->get_num() << ",ip=" << request->get_IPS() << ",port=" << request->get_port() << std::endl;
   WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());

    m_jk.SetServerInfo(request->get_num(), request->get_IPS().c_str(), request->get_port());
    return true;
}

bool JkMainClientRpcProxy::SetMainHwnd(rpc::control_t&, SetMainHwnd_RequestType *request, SetMainHwnd_ReponseType *, rpc::closure_t *)
{
    std::ostringstream oss;
    oss << "SetMainHwnd:hwnd=" << request->get_hwnd() << std::endl;
     WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());

    m_jk.SetMainHwnd(request->get_hwnd());
    return true;
}

bool JkMainClientRpcProxy::StartLinkServer(rpc::control_t&, StartLinkServer_RequestType *request, StartLinkServer_ResponseType *, rpc::closure_t *)
{
    std::ostringstream oss;
    oss << "StartLinkServer:num=" << request->get_num() << std::endl;

     WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    m_jk.StartLinkServer(request->get_num());
    return true;
}

bool JkMainClientRpcProxy::StopLinkServer(rpc::control_t&, StopLinkServer_RequestType *request, StopLinkServer_ResponseType *, rpc::closure_t *)
{
    std::ostringstream oss;
   oss << "StopLinkServer:num=" << request->get_num() << std::endl;

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    m_jk.StopLinkServer(request->get_num());
    return true;
}

bool JkMainClientRpcProxy::NewSetLocalType(rpc::control_t&, NewSetLocalType_RequestType *request, NewSetLocalType_ReponseType *, rpc::closure_t *)
{
    std::ostringstream oss;
    oss << "NewSetLocalType:type=" << request->get_type() << std::endl;
    
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    m_jk.NewSetLocalType(request->get_type());
    return true;
}

bool JkMainClientRpcProxy::CheckPassword(rpc::control_t&, CheckPassword_RequestType *request, CheckPassword_ResponseType *response, rpc::closure_t *)
{
    long result = m_jk.CheckPassword(request->get_type(), request->get_name().c_str(), request->get_pwd().c_str(), request->get_res1().c_str(), request->get_res2());
    response->set_result(result);

    std::ostringstream oss;
    oss << "CheckPassword:type=" << request->get_type() <<",name=" << request->get_name() << ",pwd=" << request->get_pwd()
        << ",res1=" << request->get_res1() << ",res2=" << request->get_res2() << ",result=" << result << std::endl;

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    return true;
}

bool JkMainClientRpcProxy::SendInfo(rpc::control_t&, SendInfo_RequestType *request, SendInfo_ResponseType *, rpc::closure_t *)
{
    std::ostringstream oss;
    oss << "SendInfo:name=" << request->get_name() << ",ip=" << request->get_IPS() << std::endl;

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    m_jk.SendInfo(request->get_name().c_str(), request->get_IPS().c_str());
    return true;
}

bool JkMainClientRpcProxy::GetLoginInfo(rpc::control_t&, GetLoginInfo_RequestType *request, GetLoginInfo_ResponseType *response, rpc::closure_t *)
{
    char name[512]="";
    char pwd[512]="";
    char str_res1[512]="";
    long port =-1;
    long res2 = -1;
    m_jk.GetLoginInformation(request->get_ids().c_str(), name, pwd, &port,str_res1,&res2);
    response->set_name(name);
    response->set_pwd(pwd);
    response->set_res1(str_res1);
    response->set_port(port);
    response->set_res2(res2);

    std::ostringstream oss;
    oss << "GetLoginInfo:ids=" << request->get_ids() <<",name=" << response->get_name() << ",pwd" << response->get_pwd() 
        << ",port=" << response->get_port() << ",res1=" << response->get_res1() << ",res2=" << response->get_res2() << std::endl;
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    return true;
}

bool JkMainClientRpcProxy::SendDARReq(rpc::control_t&, SendDARReq_RequestType *request, SendDARReq_ResponseType *, rpc::closure_t *)
{
    std::ostringstream oss;
    oss << "SendDARReq:ids=" << request->get_ids() << ",chid=" << request->get_ch() << ",ctype=" << request->get_code_type() 
        << ",res1=" << request->get_res1() << ",res2=" << request->get_res2() << std::endl;
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    
    m_jk.SendDARReq(request->get_ids().c_str(), request->get_ch(), request->get_code_type(), request->get_res1(), request->get_res2().c_str());
    return true;
}

bool JkMainClientRpcProxy::SendTransparentCommand(rpc::control_t&, SendTransparentCommand_RequestType *request, SendTransparentCommand_ResponseType *response, rpc::closure_t *)
{
    long result = m_jk.SendTransparentCommand(request->get_ids().c_str(), request->get_ips().c_str(), request->get_cmds().c_str());
    response->set_result(result);

    std::ostringstream oss;
    oss << "SendTransparentCommand:ids=" << request->get_ids() << ",ip=" << request->get_ips() << ",cmds=" << request->get_cmds() << ",result=" << result << std::endl;

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    return true;
}

bool JkMainClientRpcProxy::SetVideoCenterPlayID(rpc::control_t&,  SetVideoCenterPlayID_RequestType *request, SetVideoCenterPlayID_ResponseType *, rpc::closure_t *)
{
    std::ostringstream oss;
    oss << "SetVideoCenterPlayID:ids=" << request->get_ids() << ",dvr_chid=" << request->get_dvr_chid() << ",vcenter_chid=" << request->get_vcenter_chid()
        << ",res1=" << request->get_res1() << ",strres1=" << request->get_str_res1() << std::endl;

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    m_jk.SetVideoCenterPlayID(request->get_ids().c_str(), request->get_dvr_chid(), request->get_vcenter_chid(), request->get_res1(), request->get_str_res1().c_str());
    return true;
}

bool JkMainClientRpcProxy::SetVideoCenterPlayIDEx(rpc::control_t&,  SetVideoCenterPlayIDEx_RequestType *request, SetVideoCenterPlayIDEx_ResponseType *, rpc::closure_t *)
{
    std::ostringstream oss;
    oss << "SetVideoCenterPlayIDEx:ids=" << request->get_ids() << ",dvr_chid=" << request->get_dvr_chid() << ",vcenter_chid=" << request->get_vcenter_chid()
        << ",res1=" << request->get_res1() << ",strres1=" << request->get_str_res1() <<",exinfo="<<request->get_sExInfo() << std::endl;

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    m_jk.SetVideoCenterPlayIDEx(request->get_ids().c_str(), request->get_dvr_chid(), request->get_vcenter_chid(), request->get_res1(), request->get_str_res1().c_str(),request->get_sExInfo().c_str());
    return true;
}

void JkMainClientRpcProxy::OnLinkServerJkEvent(long sNum, long bz)
{
    std::ostringstream oss;
    oss << "OnLinkServerJkEvent:sNum=" << sNum << ",bz=" << bz << std::endl;

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());

    LinkServerEvent_RequestType request;
    request.set_num(sNum);
    request.set_flag(bz);

    rpc::control_t control;
    OnLinkServerEvent(control, &request);
}

void JkMainClientRpcProxy::OnUserInOutJkEvent(char* sIDS, char* sName, long sType, char* sIPS, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2)
{
    std::ostringstream oss;
    oss << "OnUserInOutJkEvent:sIDS=" << sIDS << ",sName=" << sName << ",sType=" << sType << ",sIPS=" << sIPS << ",bz=" << bz 
        << ",iRes1="<< iRes1 << ",sRes1=" << sRes1 << ",sRes2=" << sRes2 << std::endl;


    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    UserLoginOrLogoutEvent_RequestType request;

    request.set_ids(sIDS);
    request.set_name(sName);
    request.set_type(sType);
    request.set_ips(sIPS);
    request.set_flag(bz);
    request.set_res1(iRes1);
    request.set_res2(iRes2);
    request.set_str_res1(sRes1);
    request.set_str_res2(sRes2);

    rpc::control_t control;
    OnUserLoginOrLogoutEvent(control, &request);
}

void JkMainClientRpcProxy::OnDBImageCenterJkEvent(char* sIDS, long sCH, long successbz, char* fIDS, char* fIPS, long DBMode, long localVPort, long localAPort, long destVPort, long destAPort, long iRes1, char* sRes1)
{
    std::ostringstream oss;
    oss << "OnDBImageCenterJkEvent:sIDS=" << sIDS << ",sCH=" << sCH << ",bz=" << successbz << ",fIDS=" << fIDS << ",fIP=" << fIPS 
        << ",dbMode="<< DBMode << ",localVPort=" << localVPort << ",localAPort=" << localAPort << ",destVPort=" << destVPort 
        << ",destAPort=" << destAPort << ",iRes1=" << iRes1<< ",sRes1=" << sRes1<<std::endl;

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());

    DBImageCenterEvent_RequestType request;
    request.set_ids(sIDS);
    request.set_ch(sCH);
    request.set_flag(successbz);
    request.set_fids(fIDS);
    request.set_ips(fIPS);
    request.set_dbmode(DBMode);
    request.set_local_vport(localVPort);
    request.set_local_aport(localAPort);
    request.set_dest_vport(destVPort);
    request.set_dest_aport(destAPort);
    request.set_res1(iRes1);
    request.set_str_res1(sRes1);

    rpc::control_t control;
    OnDBImageCenterEvent(control, &request);
}

void JkMainClientRpcProxy::OnGetMsgJkEvent( char* sSrcIDS, char* sData, long nDataLen, long nOrderbz)
{
    std::ostringstream oss;
    oss << "OnGetMsgJkEvent:pstrSrcIDS=" << sSrcIDS << ",pstrInfo=" << sData << ",nInfoSize=" << nDataLen << ",nType=" << nOrderbz <<std::endl;

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());

    OnGetMsgEvent_RequestType request;
    request.set_src_ids(sSrcIDS);
    request.set_info(sData);
    request.set_info_size(nDataLen);
    request.set_type(nOrderbz);

    rpc::control_t control;
    OnGetMsgEvent(control, &request);
}

void JkMainClientRpcProxy::OnTellLocalIDSJkEvent(char* LocalIDS, char* sRes1, long iRes1)
{
    std::ostringstream oss;
    oss << "OnTellLocalIDSJkEvent:LocalIDS=" << LocalIDS << ",sRes1=" << sRes1 << ",iRes1=" << iRes1 <<std::endl;
    
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());

    OnTellLocalIDSEvent_RequestType request;
    request.set_local_ids(LocalIDS);
    request.set_str_res1(sRes1);
    request.set_ires1(iRes1);

    rpc::control_t control;
    OnTellLocalIDSEvent(control, &request);
}

void JkMainClientRpcProxy::OnTransparentCommandJkEvent(char* fIDS, char* sCommands)
{
    std::ostringstream oss;
    oss << "OnTransparentCommandJkEvent:fIDS=" << fIDS<<"sCommands"<<sCommands<<std::endl;
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    //songlei

    OnTransparentCommandEvent_RequestType request;
    request.set_ids(fIDS);
    request.set_command(sCommands);

    rpc::control_t ctrl; 
    OnTransparentCommandEvent(ctrl,&request); 

}

void JkMainClientRpcProxy::on_client_connect()
{
    bool result = LinkCenter();

    std::ostringstream oss;
    oss << "on_client_connect:LinkCenter result" << result <<std::endl;
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
}

void JkMainClientRpcProxy::on_client_disconnect(bool is_exception)
{
    ExitCenter();

    std::ostringstream oss;
    oss << "on_client_disconnect:ExitCenter" <<std::endl;
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());

}

