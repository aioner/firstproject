#ifndef JK_DLL_H__
#include "jk/JKMainClient.h"

class jk_dll
{
	//功能接口
public:
	void init();
	void uninit();

	//保存回调用
	////////////////////////////////////////////////////////////////////
	void  SetCallBackOnTellLocalIDS(OnTellLocalIDS CallBack ,void* pUser);

	void  SetCallBackOnLinkServer(OnLinkServer CallBack ,void* pUser);

	void  SetCallBackOnUserInOut(OnUserInOut CallBack ,void* pUser);

	void  SetCallBackOnDBImageCenter(OnDBImageCenter CallBack ,void* pUser);

	void  SetCallBackOnEventGetMsg(OnEventGetMsg CallBack ,void* pUser);

	void SetCallBackOnTransparentCommand(OnTransparentCommand CallBack ,void* pUser);
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

private:

};
#endif//#ifndef JK_DLL_H__