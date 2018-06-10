#ifndef _JK_MAIN_CLIENT_OCX_RPC_SERVER_H_INCLUDED
#define _JK_MAIN_CLIENT_OCX_RPC_SERVER_H_INCLUDED
#include "JkMainClientRpcTypes.h"
#include "rpc/rpc_proxy.h"
#include "JkMainClient.h"

class JkMainClientRpcProxy : 
    public rpc::proxy_t<JkMainClientRpcProxy>, 
    public JkMainClientOcxRpcInterfaces<JkMainClientRpcProxy>,
    public JkMainClient
{
public:
    bool SetServerInfo(rpc::control_t&, SetServerInfo_RequestType *, SetServerInfo_ResponseType *, rpc::closure_t *);
    bool SetMainHwnd(rpc::control_t&, SetMainHwnd_RequestType *, SetMainHwnd_ReponseType *, rpc::closure_t *);
    bool StartLinkServer(rpc::control_t&, StartLinkServer_RequestType *, StartLinkServer_ResponseType *, rpc::closure_t *);
    bool StopLinkServer(rpc::control_t&, StopLinkServer_RequestType *, StopLinkServer_ResponseType *, rpc::closure_t *);
    bool NewSetLocalType(rpc::control_t&, NewSetLocalType_RequestType *, NewSetLocalType_ReponseType *, rpc::closure_t *);
    bool CheckPassword(rpc::control_t&, CheckPassword_RequestType *, CheckPassword_ResponseType *, rpc::closure_t *);
    bool SendInfo(rpc::control_t&, SendInfo_RequestType *, SendInfo_ResponseType *, rpc::closure_t *);
    bool GetLoginInfo(rpc::control_t&, GetLoginInfo_RequestType *, GetLoginInfo_ResponseType *, rpc::closure_t *);
    bool SendDARReq(rpc::control_t&, SendDARReq_RequestType *, SendDARReq_ResponseType *, rpc::closure_t *);
    bool SendTransparentCommand(rpc::control_t&, SendTransparentCommand_RequestType *, SendTransparentCommand_ResponseType *, rpc::closure_t *);
    bool SetVideoCenterPlayID(rpc::control_t&,  SetVideoCenterPlayID_RequestType *, SetVideoCenterPlayID_ResponseType *, rpc::closure_t *);
    bool SetVideoCenterPlayIDEx(rpc::control_t&,  SetVideoCenterPlayIDEx_RequestType *, SetVideoCenterPlayIDEx_ResponseType *, rpc::closure_t *);

    RPC_PROXY_EVENT_IMPL(binary, OnLinkServerEvent, LinkServerEvent_RequestType)
    RPC_PROXY_EVENT_IMPL(binary, OnUserLoginOrLogoutEvent, UserLoginOrLogoutEvent_RequestType)
    RPC_PROXY_EVENT_IMPL(binary, OnDBImageCenterEvent, DBImageCenterEvent_RequestType) 
    RPC_PROXY_EVENT_IMPL(binary, OnGetMsgEvent, OnGetMsgEvent_RequestType)
    RPC_PROXY_EVENT_IMPL(binary, OnTellLocalIDSEvent, OnTellLocalIDSEvent_RequestType)
    RPC_PROXY_EVENT_IMPL(binary, OnTransparentCommandEvent, OnTransparentCommandEvent_RequestType)

    void OnLinkServerJkEvent(long sNum, long bz);
    void OnUserInOutJkEvent(char* sIDS, char* sName, long sType, char* sIPS, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2);
    void OnDBImageCenterJkEvent(char* sIDS, long sCH, long successbz, char* fIDS, char* fIPS, long DBMode, long localVPort, long localAPort, long destVPort, long destAPort, long iRes1, char* sRes1);
    void OnGetMsgJkEvent( char* sSrcIDS, char* sData, long nDataLen, long nOrderbz);
    void OnTellLocalIDSJkEvent(char* LocalIDS, char* sRes1, long iRes1);
    void OnTransparentCommandJkEvent(char* fIDS, char* sCommands);

    void on_client_connect();
    void on_client_disconnect(bool is_exception);
};

#endif //_JK_MAIN_CLIENT_OCX_RPC_SERVER_H_INCLUDED
