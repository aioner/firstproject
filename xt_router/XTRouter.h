#ifndef XTROUTER_H__
#define XTROUTER_H__

#include <string>
#include <vector>
#include <stdint.h>
#include <boost/noncopyable.hpp>
#include "Router_config.h"
#include "mml/cmd_manager.h"
#define  COMMAND_DISPATTCH_FUNCTION bool
struct  router_config
{
    std::string local_ip;                 //本地地址
    std::string local_snd_ip;             //本地发送地址
    std::string mul_ip;                  //组播起始地址 
    std::string log_usre_name;         //登录Openfire用户名
    std::string log_usre_password;     //密码
    std::string log_usre_res_id;       //资源ID
    std::string log_center_name;       //中心主机名
    std::string default_sdp;           //默认全能力级接收sdp
    uint32_t   log_center_port;       //登录中心端口
    unsigned long chan_num;           //服务通道数
    unsigned short start_sndport;      //发送起始端口
    bool demux_flg_s;                      //发送复用
    bool reserve1;                      //预留
    unsigned xtmsg_listenport;         //xtmsg监听端口
    unsigned rtsp_listenport;          //rtsp监听端口
    unsigned tcp_listenport;           //tcp发送&监听端口
    unsigned regist_listenport;        //注册监听端口
    unsigned udp_listenport;           //UDP会话侦听端口 
    bool regist;                      //是否注册
    unsigned regist_bind;              //注册绑定端口
    std::string regist_ip;            //注册地址
    unsigned regist_port;              //注册端口
    bool std_rtp;                    //发送标准RTP
    bool demux_r;           //接收端复用开关
    bool use_strmtype_param; //使用strmtype
    center_type link_center_type;//连接中心类型
};

class CXTRouter : boost::noncopyable
{
private:
    static CXTRouter m_obj;
public:
    static CXTRouter* Instance()
    {
        return &m_obj;
    }
    static CXTRouter* _()
    {
        return &m_obj;
    }

    int StartXTRouter();

    void init_cfg();

    int StopXTRouter();

    void GetConfig(router_config &config);

    bool SndStdRtp() const
    {
        return m_config.std_rtp;
    }

    //命令调用函数
protected:
    //点播命令
	COMMAND_DISPATTCH_FUNCTION  Play(const command_argument_t& Args,std::string&result);
	COMMAND_DISPATTCH_FUNCTION  sdp_send(const command_argument_t& Args,std::string&result);

    //停止
    COMMAND_DISPATTCH_FUNCTION  Stop(const command_argument_t& Args,std::string&result);
    //停止
    COMMAND_DISPATTCH_FUNCTION  StopAll(const command_argument_t& Args,std::string&result);

    //连接信息
    COMMAND_DISPATTCH_FUNCTION  ShowConnectInfo(const command_argument_t& Args,std::string&result);
    COMMAND_DISPATTCH_FUNCTION  HideConnectInfo(const command_argument_t& Args,std::string &result);

    //查看交换点播信息
    COMMAND_DISPATTCH_FUNCTION  ShowPlayInfo(const command_argument_t& Args,std::string&result);
    COMMAND_DISPATTCH_FUNCTION  HidePlayInfo(const command_argument_t& Args,std::string &result);

    //查看系统头信息
    COMMAND_DISPATTCH_FUNCTION  ShowKeyInfo(const command_argument_t& Args, std::string&result);

    //帮助
    COMMAND_DISPATTCH_FUNCTION  Help(const command_argument_t& Args,std::string&result);

    //登录登出中心 信息查看
    COMMAND_DISPATTCH_FUNCTION ShowlogInfo(const command_argument_t& Args,std::string &result);

    //中心点播信令 信息查看
    COMMAND_DISPATTCH_FUNCTION ShowCenterCommand(const command_argument_t& Args,std::string &result);

    //信令执行结果 信息查看
    COMMAND_DISPATTCH_FUNCTION ShowSigalingExecRet(const command_argument_t& Args,std::string &result);

    //反馈中心事件信息
    COMMAND_DISPATTCH_FUNCTION ShowResponseInfo(const command_argument_t& Args,std::string &result);

    //LinkSever事件信息查看
    COMMAND_DISPATTCH_FUNCTION ShowLinkSeverEventInfo(const command_argument_t& Args,std::string &result);

    //清屏
    COMMAND_DISPATTCH_FUNCTION Clear(const command_argument_t& Args,std::string &result);

    COMMAND_DISPATTCH_FUNCTION AppExit(const command_argument_t& Args,std::string &result);

    //工作信息查看
    COMMAND_DISPATTCH_FUNCTION ShowBaseInfo(const command_argument_t& Args,std::string &result);

    //query information of the framework task schedulers
    COMMAND_DISPATTCH_FUNCTION QueryFrameworkTaskScheduler(const command_argument_t& Args,std::string &result);

    COMMAND_DISPATTCH_FUNCTION LogOnOff(const command_argument_t& Args,std::string&result);

    COMMAND_DISPATTCH_FUNCTION XmppPlay(const command_argument_t& Args,std::string &result);
    COMMAND_DISPATTCH_FUNCTION XmppStop(const command_argument_t& Args,std::string &result);

    COMMAND_DISPATTCH_FUNCTION xregister(const command_argument_t& Args,std::string &result);
    COMMAND_DISPATTCH_FUNCTION xstop_register(const command_argument_t& Args,std::string &result);

    COMMAND_DISPATTCH_FUNCTION get_center_name(const command_argument_t& Args,std::string &result);

    COMMAND_DISPATTCH_FUNCTION log_ccs(const command_argument_t& Args,std::string &result);

    //打开接收
    COMMAND_DISPATTCH_FUNCTION openr(const command_argument_t& Args,std::string &result);
    COMMAND_DISPATTCH_FUNCTION setsdpr(const command_argument_t& Args,std::string &result);
    COMMAND_DISPATTCH_FUNCTION close_recv(const command_argument_t& Args,std::string &result);

    //打开一个发送
    COMMAND_DISPATTCH_FUNCTION add_send(const command_argument_t& Args,std::string &result);

    //删除所有发送
    COMMAND_DISPATTCH_FUNCTION del_send_all(const command_argument_t& Args,std::string &result);

    //删除指定发送
    COMMAND_DISPATTCH_FUNCTION del_send(const command_argument_t& Args,std::string &result);

    //获取发送信息
    COMMAND_DISPATTCH_FUNCTION getsndinf(const command_argument_t& Args,std::string&result);

    //获取接收端信息
    COMMAND_DISPATTCH_FUNCTION getrecvinf(const command_argument_t& Args,std::string&result);

    //查看网关会话状态
    COMMAND_DISPATTCH_FUNCTION gws(const command_argument_t& Args,std::string&result);

    //查看接收信息
    COMMAND_DISPATTCH_FUNCTION recv(const command_argument_t& Args,std::string&result);

protected:
    void PrintPlayCommandHelp(std::string &result);
	void PrintlogCommandHelp(std::string &result);
    void PrintSkiCommandHelp(std::string &result);
    void PrintSwiCommandHelp(std::string &result);
    void PrintGetrevinfCommandHelp(std::string &result);
    void PrintGersndinfCommandHelp(std::string &result);
    void PrintStopCommandHelp(std::string &result);
    void PrintSpiCommandHelp(std::string &result);
    void PrintslsiCommandHelp(std::string &result);
    void PrintsliCommandHelp(std::string &result);
    void PrintclsComamndHelp(std::string &result);
    void PrintScoiCommandHelp(std::string &result);
private:
    void ResCommand();

    void CtrlMsgInit();
    void CtrlMsgUninit();
public:
    const router_config& get_router_cfg(){return m_config;}
    const std::string& get_local_ip() const
    {
        return m_config.local_ip;
    }
    void update_cfg_local_ip(const std::string& ip)
    {
        m_config.local_ip.assign(ip);
    }
    bool get_send_demux_flag() const
    {
        return m_config.demux_flg_s;
    }

    bool get_demux_mode_r() const
    {
        return m_config.demux_r;
    }

    const bool use_jk() const 
    { 
        return (m_config.link_center_type == config_type::LINK_XTCENTER); 
    }

    const bool use_ccs()const 
    {
        return (m_config.link_center_type == config_type::LINK_CCS); 
    }

    const bool use_sc()const
    {
        return(m_config.link_center_type == config_type::LIPK_SC);
    }
	const bool use_sc2()const
	{
		return true;
	}
    const std::string& get_default_sdp() const 
    {
        return m_config.default_sdp;
    }
public:
    void regist_log_sys();
    // 服务配置
    router_config m_config;
};
#endif //XTROUTER_H__
