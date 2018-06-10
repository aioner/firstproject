#include "dps_jk_dispatch_mgr.h"
#include "jk_task_pro.h"
#include "dps_cfg_mgr.h"


#ifdef _WIN32
#ifdef _DEBUG
# pragma comment(lib,"JKMainClient.lib")
#pragma message("Auto Link JKMainClient.lib")
#else
# pragma comment(lib,"JKMainClient.lib")
#pragma message("Auto Link JKMainClient.lib")
#endif // end #ifdef _DEBUG
#endif // end #ifdef _WIN32

#define Center_TYPE     0x03  //设备代理服务器
dps_jk_dispatch_mgr dps_jk_dispatch_mgr::my_;

// 通知本机的IDS 
void dps_jk_dispatch_mgr::on_tell_local_ids(void* pUser, char* LocalIDS, char* sRes1, long iRes1)
{
    on_tell_local_ids_task *ptr_task = new on_tell_local_ids_task(LocalIDS,sRes1,iRes1);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

// 通知本机的IDS  
void dps_jk_dispatch_mgr::on_link_server(void* pUser, long sNum, long bz)
{
    on_link_server_task * ptr_task = new on_link_server_task(sNum,bz);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

// 链路连接成功 
void dps_jk_dispatch_mgr::on_user_in_out(void* pUser, char* sIDS, char* sName, long sType, char* sIPS, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2)
{
    on_user_in_out_task * ptr_task = new on_user_in_out_task(sIDS,sName,sType,sIPS,bz,iRes1,iRes2,sRes1,sRes2);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//专线保障
void dps_jk_dispatch_mgr::on_event_get_msg(void* pUser, char* sSrcIDS, char* sData,long nDataLen, long nOrderbz)
{
    on_event_get_msg_task * ptr_task = new on_event_get_msg_task(sSrcIDS,sData,nDataLen,nOrderbz);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//通知服务器，本机的某个图像被点播 
void dps_jk_dispatch_mgr::on_dbimage_center(void* pUser, char* sIDS, long sCH, long successbz, char* fIDS, char* fIPS, long DBMode, long localVPort,long localAPort, long destVPort, long destAPort, long iRes1, char* sRes1)
{
    on_dbimage_center_task * ptr_task = new on_dbimage_center_task(sIDS,sCH,successbz,fIDS,fIPS,DBMode,localVPort,localAPort,destVPort,destAPort,iRes1,sRes1);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//透传指令
void dps_jk_dispatch_mgr::on_transparent_command_cb(void* pUser, char* fIDS, char* sCommands)
{
    on_transparent_command_cb_task* ptr_task = new on_transparent_command_cb_task(fIDS,sCommands);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//控制服务器录象
void __stdcall dps_jk_dispatch_mgr::on_client_record( void* pUser, char* sIDS, long sCH, long bz )
{
    on_client_record_task* ptr_task = new on_client_record_task(sIDS,sCH,bz);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//控制服务器报警点布防
void __stdcall dps_jk_dispatch_mgr::on_client_alarm( void* pUser, char* sIDS, long sCH, long bz )
{
    on_client_alarm_task* ptr_task = new on_client_alarm_task(sIDS,sCH,bz);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//控制服务器启动移动侦测
void __stdcall dps_jk_dispatch_mgr::on_client_motion( void* pUser, char* sIDS, long sCH, long bz )
{
    on_client_motion_task* ptr_task = new on_client_motion_task(sIDS,sCH,bz);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//调整画面质量
void __stdcall dps_jk_dispatch_mgr::on_client_image( void* pUser, char* sIDS, long sCH, long bz, long value )
{
    on_client_image_task* ptr_task = new on_client_image_task(sIDS,sCH,bz,value);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//要求dIDS代表的DVS发送方位角、仰角、云台光圈、焦距请求
void __stdcall dps_jk_dispatch_mgr::on_ask_angelcamerazt( void* pUser, long OP, char* dIDS, long dCH, char* sRes1, long iRes1 )
{
    on_ask_angelcamerazt_task* ptr_task = new on_ask_angelcamerazt_task(OP,dIDS,dCH,sRes1,iRes1);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//要求强制出I帧
void __stdcall dps_jk_dispatch_mgr::on_client_capture_iframe( void* pUser, char* sIDS, long sCH )
{
    on_client_capture_iframe_task* ptr_task = new on_client_capture_iframe_task(sIDS,sCH);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//OSD设置响应事件
void __stdcall dps_jk_dispatch_mgr::on_client_osd( void* pUser,char* sIDS, long sCH, char* osdName, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2 )
{
    on_client_osd_task* ptr_task = new on_client_osd_task(sIDS,sCH,osdName,bz,iRes1,iRes2,sRes1,sRes2);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//控制云台
void __stdcall dps_jk_dispatch_mgr::on_client_yt( void* puser, char* sIDS, long sCH, long Op )
{
    on_client_yt_task* ptr_task = new on_client_yt_task(sIDS,sCH,Op);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//云台控制响应事件
void __stdcall dps_jk_dispatch_mgr::on_client_ytex( void* pUser, char* dIDS, long dCH, long op, long val, long iRes1, char* sRes1 )
{
    on_client_ytex_task* ptr_task = new on_client_ytex_task(dIDS,dCH,op,val,iRes1,sRes1);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//选择预置点
void __stdcall dps_jk_dispatch_mgr::on_client_select_point( void* pUser, char* sIDS, long sCH, long cNum )
{
    on_client_select_point_task* ptr_task = new on_client_select_point_task(sIDS,sCH,cNum);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//设置预置点
void __stdcall dps_jk_dispatch_mgr::on_client_set_point( void* pUser, char* sIDS, long sCH, long cNum, char* pName )
{
    on_client_set_point_task* ptr_task = new on_client_set_point_task(sIDS,sCH,cNum,pName);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//接受透明数据
void __stdcall dps_jk_dispatch_mgr::on_transparent_data( void* pUser, char* fIDS, long iCmd, char* Buf, long len )
{
    on_transparent_data_task* ptr_task = new on_transparent_data_task(fIDS,iCmd,Buf,len);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//分组设备状态改变触发
void __stdcall dps_jk_dispatch_mgr::on_group_device_state_change( void* pUser,char* sGroupIDS, char* sDeviceIDS, char* sDeviceName, char* sDeviceIPS, long iType, long iIsOnline, long iRes1, char* sRes1 )
{
    on_group_device_state_change_task* ptr_task = new on_group_device_state_change_task(sGroupIDS,sDeviceIDS,sDeviceName,sDeviceIPS,iType,iIsOnline,iRes1,sRes1);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

//校时
void __stdcall dps_jk_dispatch_mgr::on_check_time( void* pUser, long iYear, long iMonth, long iDate, long iWeek, long iHour, long iMin, long iSec, long iRes1, long iRes2 )
{
    on_check_time_task* ptr_task = new on_check_time_task(iYear,iMonth,iDate,iWeek,iHour,iMin,iSec,iRes1,iRes2);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

void  dps_jk_dispatch_mgr::SetCallBackOnTellLocalIDS(OnTellLocalIDS CallBack ,void* pUser)
{
    ::SetCallBackOnTellLocalIDS(CallBack,pUser);
}

void  dps_jk_dispatch_mgr::SetCallBackOnLinkServer(OnLinkServer CallBack ,void* pUser)
{
    ::SetCallBackOnLinkServer(CallBack,pUser);
}

void  dps_jk_dispatch_mgr::SetCallBackOnUserInOut(OnUserInOut CallBack ,void* pUser)
{
    ::SetCallBackOnUserInOut(CallBack,pUser);
}

void  dps_jk_dispatch_mgr::SetCallBackOnDBImageCenter(OnDBImageCenter CallBack ,void* pUser)
{
    ::SetCallBackOnDBImageCenter(CallBack,pUser);
}

void  dps_jk_dispatch_mgr::SetCallBackOnEventGetMsg(OnEventGetMsg CallBack ,void* pUser)
{
    ::SetCallBackOnEventGetMsg(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnTransparentCommand(OnTransparentCommand CallBack ,void* pUser)
{
    ::SetCallBackOnTransparentCommand(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnClientRecord( OnClientRecord CallBack,void* pUser )
{
    ::SetCallBackOnClientRecord(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnClientAlarm( OnClientAlarm CallBack,void* pUser )
{
    ::SetCallBackOnClientAlarm(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnClientMotion( OnClientMotion CallBack,void* pUser )
{
    ::SetCallBackOnClientMotion(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnClientImage( OnClientImage CallBack,void* pUser )
{
    ::SetCallBackOnClientImage(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnAskAngelCameraZt( OnAskAngelCameraZt CallBack,void* pUser )
{
    ::SetCallBackOnAskAngelCameraZt(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnClientCaptureIFrame( OnClientCaptureIFrame CallBack,void* pUser )
{
    ::SetCallBackOnClientCaptureIFrame(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnOnClientOSD( OnClientOSD CallBack,void* pUser )
{
    ::SetCallBackOnOnClientOSD(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnClientYT( OnClientYT CallBack,void* pUser )
{
    ::SetCallBackOnClientYT(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnClientYTEx( OnClientYTEx CallBack,void* pUser )
{
    ::SetCallBackOnClientYTEx(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnClientSelectPoint( OnClientSelectPoint CallBack,void* pUser )
{
    ::SetCallBackOnClientSelectPoint(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnClientSetPoint( OnClientSetPoint CallBack,void* pUser )
{
    ::SetCallBackOnClientSetPoint(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnTransparentData( OnTransparentData CallBack,void* pUser )
{
    ::SetCallBackOnTransparentData(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnGroupDeviceStateChange( OnGroupDeviceStateChange CallBack,void* pUser )
{
    ::SetCallBackOnGroupDeviceStateChange(CallBack,pUser);
}

void dps_jk_dispatch_mgr::SetCallBackOnCheckTime( OnCheckTime CallBack,void* pUser )
{
    ::SetCallBackOnCheckTime(CallBack,pUser);
}

// 设置调用控件的主窗口句柄，用于控件内部处理各种网络消息
void dps_jk_dispatch_mgr::SetMainHwnd( long mhwnd)
{
    ::SetMainHwnd(mhwnd);
}

// 停止连接服务器 
void dps_jk_dispatch_mgr::SetServerInfo( short sNum,const char* sIPS, long sPort)
{
    ::SetServerInfo(sNum,sIPS,sPort);
}

// 登陆时设置校验密码
void dps_jk_dispatch_mgr::StartLinkServer( short sNum)
{
    ::StartLinkServer(sNum);
}

//停止连接服务器
void dps_jk_dispatch_mgr::StopLinkServer( short sNum)
{
    ::StopLinkServer(sNum);
}

//设置本机类型，并分配IDS 
void dps_jk_dispatch_mgr::NewSetLocalType( long sType)
{
    ::NewSetLocalType(sType);	
}

//登陆时设置校验密码
long dps_jk_dispatch_mgr::CheckPassword( long nType,const char* Name,const char* Mima,const char* sRes1, long iRes2)
{
    return ::CheckPassword( nType,  Name, Mima, sRes1,iRes2);
}

// 从JK得到DVR的登陆用户名和密码
void dps_jk_dispatch_mgr::GetLoginInfo(const char* sIDS, char* sName, char* sPassword, char* iPort, char* sRes1, char* iRes1)
{
    ::GetLoginInfo(sIDS,sName,sPassword,iPort,sRes1,iRes1);
}

void dps_jk_dispatch_mgr::GetLoginInformation(const char* sIDS, char* szName, char* szPassword, long *plPort, char* szRes1, long *plRes1)
{
    ::GetLoginInformation(sIDS,szName,szPassword,plPort,szRes1,plRes1);
}
//发送透明字符串给本级、下级
long dps_jk_dispatch_mgr::SendTransparentCommand(const char* sIDS,const char* sIPS,const char* sCommands)
{
    return ::SendTransparentCommand(sIDS,sIPS,sCommands);
}

//通知中心图像点播信息（视频交换机专用）
void dps_jk_dispatch_mgr::SetVideoCenterPlayID(const char* strSID, long lDvrChID, long lVCenterChID, long iRes1,const char* sRes1)
{
    ::SetVideoCenterPlayID(strSID,lDvrChID,lVCenterChID,iRes1,sRes1);
}

void dps_jk_dispatch_mgr::SetVideoCenterPlayIDEx(const char* strSID, long lDvrChID, long lVCenterChID, long iRes1,const char* sRes1,const char* sExInfo)
{
    ::SetVideoCenterPlayIDEx(strSID,lDvrChID,lVCenterChID,iRes1,sRes1,sExInfo);
}

//设置本级登陆时注册到中心的名字和IP地址
void dps_jk_dispatch_mgr::SendInfo(const char* myName,const char* myIPS)
{
    ::SendInfo(myName, myIPS); 
}

void dps_jk_dispatch_mgr::SendDARReq(const char* sIDS,long sCH,	long iCodeType,	 long iRes1,const char* sRes1)
{
    ::SendDARReq(sIDS,sCH,iCodeType,iRes1,sRes1); 
}	

void dps_jk_dispatch_mgr::SetLinkType(long sNum , long sType)
{
    ::SetLinkType(sNum,sType);
}

bool dps_jk_dispatch_mgr::link_center()
{
    SetMainHwnd(0);

    NewSetLocalType(Center_TYPE);
    CheckPassword(0x1000, "", "", "", server_num_);//将交换机的交换能力提交给中心
    //0：dricplay  1:tcp 2:udx
    SetLinkType(centerid_,center_link_type_);
    SetServerInfo((short)centerid_, center_ip_.c_str(), center_port_);
    SendInfo(local_name_.c_str(), local_ip_.c_str());
    StartLinkServer((short)centerid_);
    return true;
}
void dps_jk_dispatch_mgr::exit_center()
{
    ::StopLinkServer((short)centerid_);
}

//加载配置
void dps_jk_dispatch_mgr::loading_cfg()
{
    center_link_type_ = dps_cfg_mgr::_()->center_link_type(1);
    centerid_ = dps_cfg_mgr::_()->linkcenterid(1);
    center_port_ = dps_cfg_mgr::_()->linkcenterport(0);
    server_num_ = dps_cfg_mgr::_()->chan_num(64);
    local_ip_ = dps_cfg_mgr::_()->localip("0.0.0.0");
    center_ip_ = dps_cfg_mgr::_()->centerip("0.0.0.0");
}

void dps_jk_dispatch_mgr::init()
{
    ::Initialize();
    SetCallBackOnTellLocalIDS(on_tell_local_ids,this);
    SetCallBackOnLinkServer(on_link_server,this);
    SetCallBackOnUserInOut(on_user_in_out,this);
    SetCallBackOnDBImageCenter(on_dbimage_center,this);
    SetCallBackOnEventGetMsg(on_event_get_msg,this);
    SetCallBackOnTransparentCommand(on_transparent_command_cb,this);
    SetCallBackOnClientRecord(on_client_record,this);
    SetCallBackOnClientAlarm(on_client_alarm,this);
    SetCallBackOnClientMotion(on_client_motion,this);
    SetCallBackOnClientImage(on_client_image,this);
    SetCallBackOnAskAngelCameraZt(on_ask_angelcamerazt,this);
    SetCallBackOnClientCaptureIFrame(on_client_capture_iframe,this);
    SetCallBackOnOnClientOSD(on_client_osd,this);
    SetCallBackOnClientYT(on_client_yt,this);
    SetCallBackOnClientYTEx(on_client_ytex,this);
    SetCallBackOnClientSelectPoint(on_client_select_point,this);
    SetCallBackOnClientSetPoint(on_client_set_point,this);
    SetCallBackOnTransparentData(on_transparent_data,this);
    SetCallBackOnGroupDeviceStateChange(on_group_device_state_change,this);
    SetCallBackOnCheckTime(on_check_time,this);
}
void dps_jk_dispatch_mgr::uninit()
{  
    ::UnInitialize();
}
void dps_jk_dispatch_mgr::start()
{
    if (LINK_XTCENTER == dps_cfg_mgr::_()->link_center(LINK_ON))
    {
        loading_cfg();
        init();
        link_center();
    }
}
void dps_jk_dispatch_mgr::stop()
{
    if (LINK_XTCENTER == dps_cfg_mgr::_()->link_center(LINK_ON))
    {
        exit_center();
        uninit();
    }
}
