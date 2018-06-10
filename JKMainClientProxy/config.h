#ifndef _XT_CONFIG_H_INCLUDED
#define _XT_CONFIG_H_INCLUDED

#include "utility/noncopyable.h"
#include <string>
#include "xtXml.h"


namespace config_type
{
    enum link_center_type
    {
        LINK_NA = -1,    //无效连接
        LINK_ON = 0,     //不连接
        LINK_XTCENTER,   //连接私有中心
        LINK_CCS,       //连接CCS
    };
}


class config:private utility::noncopyable
{
private:
    config(void);
    ~config(void);

public:
    static config* instance()
    {
        return &self;
    }

    static config* _()
    {
        return &self;
    }

    const std::string& get_config_path() const
    {
        return m_fPath;
    }

    bool valid() const
    {
        return m_valid;
    }

    xtXmlNodePtr get_router();
    xtXmlNodePtr get_system();
    xtXmlNodePtr get_client();
    xtXmlNodePtr get_center();
    xtXmlNodePtr get_log_level();

    //base cfg
    //断线检测开关默认开
    int break_monitor(int val_default);

    //断线检测频率默认60000ms检测一次
    int break_monitor_frequency(int val_default);

    //断线检测时长
    int break_monitor_time(int val_default);

    //ping包大小
    int check_len(int val_default);

    //ping超时
    int check_timeout(int val_default);

    //link_center:连接中心 0:不连接 1:私有中心 2:ccs
    config_type::link_center_type link_center(config_type::link_center_type val_default);

    //以下点播类型使用link_type
    std::string xt_dbtype(const std::string& val_default);

    //复制转发数
    int copy_send(int val_default);

    //点播方式(0:tcp+tcp/1:xtmsg+rtp/2:xtmsg+rtp mul
    //3:xtmsg+rtp/4:xtmsg+rtp mul/5:xtmsg+rtp demux/6:rtsp+rtp std/7:rtsp+rtp/8:rtsp+rtp demux)
    int link_type(int val_default);

    //转发通道数
    int chan_num(int val_default);

    //本地发送ip
    std::string local_sndip(const std::string& val_default);

    //本地发送端口
    int snd_port(int val_default);

    //发送复用开关
    int demux_s(int val_default);

    //组播起始地址
    std::string mul_ip(const std::string& val_default);

    //tcp传输监听&传输端口
    int tcp_listenprt(int val_default);

    //私有会话协商监听端口
    int xtmsg_listenprt(int val_default);

    //rtsp会话协商监听端口
    int rtsp_listenprt(int val_default);

    int udp_listenprt(int val_default);

    //TCP 反向注册
    bool regist(bool val_default);
    int regist_bind(int val_default);
    std::string regist_ip(const std::string& val_default);
    int regist_port(int val_default);
    int regist_listenprt(int val_default);

    //标准RTP转发开关
    bool std_rtp(bool val_default);

    int sink_perch(int val_default);

    //system cfg 
    //center_host:ccs服务器名称
    std::string cneter_host(const std::string& val_default);

    //local_ip:本地IP
    std::string local_ip(const std::string& val_default);

    //center_port:中心port/ccs port
    int center_port(int val_default);

    //local_name:本地服务名
    std::string local_name(const std::string& val_default);

    //login_password:登录ccs密码
    std::string login_password(const std::string& val_default);

    //login_res_id:登录ccs服务器时使用的资源id	
    std::string login_res_id(const std::string& val_default);

    std::string center_ip(std::string val_default);
    int center_id(int val_default);
    long center_link_type(long val_default);

    bool auto_start_router(const bool val_default);

    xtXmlNodePtr get_system_set();

    bool is_write_JKMainClientLog(const bool val_default);

    bool get_daemon_config(bool val_default);

private:
    // xml
    xtXml m_config;
    xtXml m_system_config;
    xtXml m_set_config;
    // 文件路径
    std::string m_fPath;
    std::string m_set_path;
    std::string m_system_path;
    bool m_valid;

    //作为全局静态变量来初始化，一次初始化写操作，后面全是读操作
    static config self;
};

#endif //_XT_CONFIG_H_INCLUDED
