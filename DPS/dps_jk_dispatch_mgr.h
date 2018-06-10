//
//create by songlei 20160316
//
#ifndef DPS_JK_DISPATCH_MGR_H__
#define DPS_JK_DISPATCH_MGR_H__
#include <boost/noncopyable.hpp>
#include <string>
#include "JKMainClient.h"

#ifdef WIN32
#else
	#define __stdcall
#endif

class dps_jk_dispatch_mgr : boost::noncopyable
{
public:
    // 通知本机的IDS 
    static void __stdcall on_tell_local_ids(void* pUser, char* LocalIDS, char* sRes1, long iRes1);

    // 通知本机的IDS  
    static void __stdcall on_link_server(void* pUser, long sNum, long bz);

    // 链路连接成功 
    static void __stdcall on_user_in_out(void* pUser, char* sIDS, char* sName, long sType, char* sIPS, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2);

    //专线保障
    static void __stdcall on_event_get_msg(void* pUser, char* sSrcIDS, char* sData,long nDataLen, long nOrderbz);

    //通知服务器，本机的某个图像被点播 
    static void __stdcall on_dbimage_center(void* pUser, char* sIDS, long sCH, long successbz, char* fIDS, char* fIPS, long DBMode, long localVPort,long localAPort, long destVPort, long destAPort, long iRes1, char* sRes1);

    //透传指令回调
    static void __stdcall on_transparent_command_cb(void* pUser, char* fIDS, char* sCommands);

    //控制服务器录象
    static void __stdcall on_client_record(void* pUser, char* sIDS, long sCH, long bz);

    //控制服务器报警点布防
    static void __stdcall on_client_alarm(void* pUser, char* sIDS, long sCH, long bz);

    //控制服务器启动移动侦测
    static void __stdcall on_client_motion(void* pUser, char* sIDS, long sCH, long bz);

    //调整画面质量
    static void __stdcall on_client_image(void* pUser, char* sIDS, long sCH, long bz, long value);

    //要求dIDS代表的DVS发送方位角、仰角、云台光圈、焦距请求
    static void __stdcall on_ask_angelcamerazt(void* pUser, long OP, char* dIDS, long dCH, char* sRes1, long iRes1);

    //要求强制出I帧
    static void __stdcall on_client_capture_iframe(void* pUser, char* sIDS, long sCH);

    //OSD设置响应事件
    static void __stdcall on_client_osd( void* pUser,char* sIDS, long sCH, char* osdName, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2);

    //控制云台
    static void __stdcall on_client_yt(void* puser, char* sIDS, long sCH, long Op);

    //云台控制响应事件
    static void __stdcall on_client_ytex(void* pUser, char* dIDS, long dCH, long op, long val, long iRes1, char* sRes1);

    //选择预置点
    static void __stdcall on_client_select_point(void* pUser, char* sIDS, long sCH, long cNum);

    //设置预置点
    static void __stdcall on_client_set_point(void* pUser, char* sIDS, long sCH, long cNum, char* pName);

    //接受透明数据
    static void __stdcall on_transparent_data(void* pUser, char* fIDS, long iCmd, char* Buf, long len);

    //分组设备状态改变触发
    static void __stdcall on_group_device_state_change( void* pUser,char* sGroupIDS, char* sDeviceIDS, char* sDeviceName, char* sDeviceIPS, long iType, long iIsOnline, long iRes1, char* sRes1);

    //校时
    static void __stdcall on_check_time(void* pUser, long iYear, long iMonth, long iDate, long iWeek, long iHour, long iMin, long iSec, long iRes1, long iRes2);

    //功能接口
public:
    static dps_jk_dispatch_mgr* _() {return &my_;}
    void start();
    void stop();
    void init();
    void uninit();

    //保存回调用
    ////////////////////////////////////////////////////////////////////
    void  SetCallBackOnTellLocalIDS(OnTellLocalIDS CallBack ,void* pUser);

    void  SetCallBackOnLinkServer(OnLinkServer CallBack ,void* pUser);

    void  SetCallBackOnUserInOut(OnUserInOut CallBack ,void* pUser);

    void  SetCallBackOnDBImageCenter(OnDBImageCenter CallBack ,void* pUser);

    void  SetCallBackOnEventGetMsg(OnEventGetMsg CallBack ,void* pUser);

    void  SetCallBackOnTransparentCommand(OnTransparentCommand CallBack ,void* pUser);

    void  SetCallBackOnClientRecord(OnClientRecord CallBack,void* pUser);

    void  SetCallBackOnClientAlarm(OnClientAlarm CallBack,void* pUser);

    void  SetCallBackOnClientMotion(OnClientMotion CallBack,void* pUser);

    void  SetCallBackOnClientImage(OnClientImage CallBack,void* pUser);

    void  SetCallBackOnAskAngelCameraZt(OnAskAngelCameraZt CallBack,void* pUser);

    void  SetCallBackOnClientCaptureIFrame(OnClientCaptureIFrame CallBack,void* pUser);

    void  SetCallBackOnOnClientOSD(OnClientOSD CallBack,void* pUser);

    void  SetCallBackOnClientYT(OnClientYT CallBack,void* pUser);

    void  SetCallBackOnClientYTEx(OnClientYTEx CallBack,void* pUser);

    void  SetCallBackOnClientSelectPoint(OnClientSelectPoint CallBack,void* pUser);

    void  SetCallBackOnClientSetPoint(OnClientSetPoint CallBack,void* pUser);

    void  SetCallBackOnTransparentData(OnTransparentData CallBack,void* pUser);

    void  SetCallBackOnGroupDeviceStateChange(OnGroupDeviceStateChange CallBack,void* pUser);

    void  SetCallBackOnCheckTime(OnCheckTime CallBack,void* pUser);
    ////////////////////////////////////////////////////////////////////

    //停止连接服务器 
    void SetServerInfo( short sNum,const char* sIPS, long sPort);

    //设置调用控件的主窗口句柄，用于控件内部处理各种网络消息
    void SetMainHwnd( long mhwnd);

    //登陆时设置校验密码
    void StartLinkServer( short sNum);

    //停止连接服务器
    void StopLinkServer( short sNum);

    //设置本机类型，并分配IDS
    void NewSetLocalType( long sType);

    //登陆时设置校验密码
    long CheckPassword( long nType,const char* Name,const char* Mima,const char* sRes1, long iRes2);

    //从JK得到DVR的登陆用户名和密码
    void GetLoginInfo(const char* sIDS, char* sName, char* sPassword, char* iPort, char* sRes1, char* iRes1);

    void GetLoginInformation(const char* sIDS, char* szName, char* szPassword, long *plPort, char* szRes1, long *plRes1);

    //视频交换机通过此方法通知中心对某个设备的点播其带宽不够，需要调整
    void SendDARReq(const char* sIDS,long sCH,	long iCodeType,	 long iRes1,const char* sRes1);

    //发送透明字符串给本级、下级
    long SendTransparentCommand(const char* sIDS,const char* sIPS,const char* sCommands);

    //通知中心图像点播信息（视频交换机专用）
    void SetVideoCenterPlayID(const char* strSID, long lDvrChID, long lVCenterChID, long iRes1,const char* sRes1);

    //通知中心图像点播信息（视频交换机专用）
    void  SetVideoCenterPlayIDEx(const char* strSID, long lDvrChID, long lVCenterChID, long iRes1,const char* sRes1,const char* sExInfo);

    //设置本级登陆时注册到中心的名字和IP地址
    void SendInfo(const char* myName,const char* myIPS);

    void SetLinkType(long sNum , long sType);

    bool link_center();
    void exit_center();
    void loading_cfg();
    void set_local_ids(const std::string& ids)
    {
        local_ids_ = ids;

    }
    const std::string& get_local_ids()const 
    {
        return local_ids_;
    }

private:
    long center_link_type_;
    long centerid_;
    long center_port_;
    long server_num_;
    std::string center_ip_;
    std::string local_name_;
    std::string local_ip_;
    std::string local_ids_;
    static dps_jk_dispatch_mgr my_;
};

#endif // #ifndef DPS_JK_DISPATCH_MGR_H__
