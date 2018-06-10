#ifndef _JK_MAIN_CLIENT_OCX_RPC_CLIENT_H_INCLUDED
#define _JK_MAIN_CLIENT_OCX_RPC_CLIENT_H_INCLUDED
#include "JkMainClientRpcTypes.h"
#include "rpc/rpc_stub.h"

class JkMainClientRpcStub : 
    public rpc::stub_t<JkMainClientRpcStub>, 
    public JkMainClientOcxRpcInterfaces<JkMainClientRpcStub>
{
public:
    RPC_STUB_FUNC_IMPL(binary, SetServerInfo, SetServerInfo_RequestType, SetServerInfo_ResponseType)
    RPC_STUB_FUNC_IMPL(binary, SetMainHwnd, SetMainHwnd_RequestType, SetMainHwnd_ReponseType)
    RPC_STUB_FUNC_IMPL(binary, StartLinkServer, StartLinkServer_RequestType, StartLinkServer_ResponseType)
    RPC_STUB_FUNC_IMPL(binary, StopLinkServer, StopLinkServer_RequestType, StopLinkServer_ResponseType)
    RPC_STUB_FUNC_IMPL(binary, NewSetLocalType, NewSetLocalType_RequestType, NewSetLocalType_ReponseType)
    RPC_STUB_FUNC_IMPL(binary, CheckPassword, CheckPassword_RequestType, CheckPassword_ResponseType)
    RPC_STUB_FUNC_IMPL(binary, SendInfo, SendInfo_RequestType, SendInfo_ResponseType)
    RPC_STUB_FUNC_IMPL(binary, GetLoginInfo, GetLoginInfo_RequestType, GetLoginInfo_ResponseType)
    RPC_STUB_FUNC_IMPL(binary, SendDARReq, SendDARReq_RequestType, SendDARReq_ResponseType)
    RPC_STUB_FUNC_IMPL(binary, SendTransparentCommand, SendTransparentCommand_RequestType, SendTransparentCommand_ResponseType)
    RPC_STUB_FUNC_IMPL(binary, SetVideoCenterPlayID, SetVideoCenterPlayID_RequestType, SetVideoCenterPlayID_ResponseType)
    RPC_STUB_FUNC_IMPL(binary, SetVideoCenterPlayIDEx, SetVideoCenterPlayIDEx_RequestType, SetVideoCenterPlayIDEx_ResponseType)

    void OnLinkServerEvent(rpc::control_t&, LinkServerEvent_RequestType *);
    void OnUserLoginOrLogoutEvent(rpc::control_t&, UserLoginOrLogoutEvent_RequestType *);
    void OnDBImageCenterEvent(rpc::control_t&, DBImageCenterEvent_RequestType *);
    void OnGetMsgEvent(rpc::control_t&, OnGetMsgEvent_RequestType *);
    void OnTellLocalIDSEvent(rpc::control_t&, OnTellLocalIDSEvent_RequestType *);
    void OnTransparentCommandEvent(rpc::control_t&,OnTransparentCommandEvent_RequestType *);

    void on_process_exit();
};

#endif //_JK_MAIN_CLIENT_OCX_RPC_CLIENT_H_INCLUDED
