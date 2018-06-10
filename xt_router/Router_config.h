#ifndef _XT_ROUTER_CONFIG_H_INCLUDED
#define _XT_ROUTER_CONFIG_H_INCLUDED

#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <list>
#include <stdint.h>
#include "xtXml.h"

namespace config_type
{
    enum link_center_type
    {
        LINK_NA = -1,    //无效连接
        LINK_ON = 0,     //不连接
        LINK_XTCENTER,   //连接私有中心
        LINK_CCS,       //连接CCS
        LIPK_SC,   //SIP 信令控制服务单元
    };
}
typedef config_type::link_center_type center_type;

class config: boost::noncopyable
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
    xtXmlNodePtr get_access_cfg();
	xtXmlNodePtr get_snmp();
	xtXmlNodePtr get_log_level();

	//mtu
    uint32_t mtu(const uint32_t val_default);

	//log
	int log_level(const int val_default);

	//rtsp心跳监测
	unsigned int rtsp_srv_check_timer_interval(unsigned int val_default);
	unsigned int rtsp_srv_time_out_interval(unsigned int val_default);

    //base cfg
    //断线检测开关默认开
    int break_monitor_onoff(int val_default);

    //断线检测时间间隔
    uint32_t break_monitor_time_interval(int val_default);

    //状态监控开关
    int status_monitor_switch(int val_default);

    //状态监控频率
    int status_monitor_frequency(int val_default);
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

    /*
    0:tcp+tcp/1:xtmsg+rtp/2:xtmsg+rtp mul/3:xtmsg+rtp/4:xtmsg+rtp mul/
    5:xtmsg+rtp demux/6:rtsp+rtp std/7:rtsp+rtp/8:rtsp+rtp demux/9:tcp_rtp_std/10:xmpp+rtp_std/
    11:xmpp+rtp_pri/12:xmpp+rtp_pri_demux/13:udp+rtp_pri/14:udp+rtp_pri_demux/15:私有有UDP协商 RTP多播/
    16:私有有UDP协商 RTP多播/17:私有有UDP协商 标准RTP 复用流/18:私有有UDP协商 标准RTP流 多播
    */
    int get_link_type(int val_default);

    //转发通道数
    int chan_num(int val_default);

    //本地发送ip
    std::string local_sndip(const std::string& val_default);

    int use_strmtype_param(int val_default);

    //本地发送端口
    int snd_port(int val_default);

    //发送复用开关
    int demux_s(int val_default);

    //组播起始地址
    std::string mul_ip(const std::string& val_default);

    //tcp传输监听&传输端口
    int tcp_listenport(int val_default);

    //私有会话协商监听端口
    int xtmsg_listenport(int val_default);

    //rtsp会话协商监听端口
    int rtsp_listenport(int val_default);

    int udp_listenport(int val_default);

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
    //std::string local_ip(const std::string& val_default);

    //center_port:中心port/ccs port
    int center_port(int val_default);

    //local_name:本地服务名
    std::string local_name(const std::string& val_default);

    //login_password:登录ccs密码
    std::string login_password(const std::string& val_default);

    //login_res_id:登录ccs服务器时使用的资源id	
    std::string login_res_id(const std::string& val_default);

    std::string local_ip(const std::string& val_default);

    //access 配置
    /////////////////////////////////////////////////////////////////
    long rtp_recv_port_num(long val_default);
    long rtp_recv_start_port(long val_default);
    uint16_t udp_session_bind_port(uint16_t val_default);
    uint16_t udp_session_heartbit_proid(uint16_t val_default);
    uint16_t udp_session_request_try_count(uint16_t val_default);
    uint16_t udp_session_request_one_timeout(uint16_t val_default);
    uint16_t tcp_session_bind_port(uint16_t val_default);
    uint16_t tcp_session_connect_timeout(uint16_t val_default);
    uint16_t tcp_session_login_timeout(uint16_t val_default);
    uint16_t tcp_session_play_timeout(uint16_t val_default);
    uint16_t tcp_session_stop_timeout(uint16_t val_default);
    uint16_t rtsp_session_connect_timeout(uint16_t val_default);
    uint16_t rtsp_session_describe_timeout(uint16_t val_default);
    uint16_t rtsp_session_setup_timeout(uint16_t val_default);
    uint16_t rtsp_session_play_timeout(uint16_t val_default);
    uint16_t rtsp_session_pause_timeout(uint16_t val_default);
    uint16_t rtsp_session_teardown_timeout(uint16_t val_default);
    //获取linux下后台服务开关
    bool get_daemon_config(bool val_default);
    /////////////////////////////////////////////////////////////////
    //local_mac : 网卡物理地址
    std::string local_mac(const std::string& val_default);

    //network_interface_guid: 网卡guid
    std::string network_interface_guid(const std::string& val_default);

    //device_status_index : 设备索引
    int device_status_index(int val_default);

    //device_status_code : 设备编号
    std::string device_status_code(const std::string& val_default);

    //device_status_contact : 设备联系电话
    std::string device_status_contact(const std::string& val_default);

    //device_status_manufactory: 设备厂家名称
    std::string device_status_manufactory(const std::string& val_default);

    //device_status_software_version : 软件版本
    std::string device_status_software_version(const std::string& val_default);

    //device_status_hardware_version : 硬件版本
    std::string device_status_hardware_version(const std::string& val_default);

    //device_status_xh : 设备型号
    std::string device_status_xh(const std::string& val_default);

    //device_status_type:设备类型
    int device_status_type(int val_default);
    // web服务端口
    int get_web_server_port(int val_default);

    //device_status_temperature_file : 指定LINUX系统CPU温度文件
    std::string device_status_temperature_file(const std::string& val_default);

    //其它配置
    ///////////////////////////////////////////////////////////////////////////
    std::string default_sdp();

public:
    const std::string& get_systemset_path()const
    {
        return systemset_xml_path_;
    }
    bool valid_systemset_xml() const
    {
        return systemset_xml_valid;
    }
    xtXmlNodePtr get_systemset_xml_system_root();
    std::string get_local_ip_systemset(const std::string& val_default);

    //SIP相关配置
    ///////////////////////////////////////////////////////////////////////////////////////
public:
    xtXmlNodePtr get_sip_cfg();
    void sip_cfg_get_domain(std::list<std::string>& domains);
    std::string sip_cfg_commlibtype(const std::string val_default);
    std::string sip_cfg_serverids(const std::string val_default);
    std::string sip_cfg_usre(const std::string& val_default);
    std::string sip_cfg_password(const std::string& val_default);
    uint32_t sip_cfg_protocol(const uint32_t val_default);
    uint32_t sip_cfg_transport(const uint32_t val_default);
    uint32_t sip_cfg_tls_port(const uint32_t val_default);
    uint32_t sip_cfg_dtls_port(const uint32_t val_default);
    uint32_t sip_cfg_expires(const uint32_t val_default);
    uint32_t sip_cfg_session_keep_alive_time_interval(const uint32_t val_default);
    uint32_t sip_cfg_registration_retry_time_interval(const uint32_t val_default);
    uint32_t sip_cfg_link_keep_alive_time_interval(const uint32_t val_default);
    uint32_t sip_cfg_regist_retry_time_interval(const uint32_t val_default);
    uint32_t sip_cfg_channle_type(const uint32_t val_default);
    uint32_t sip_cfg_delay(const uint32_t val_default);
    uint32_t sip_cfg_packetloss(const uint32_t val_default);
    uint32_t sip_cfg_bandwidth(const uint32_t val_default);
    ///////////////////////////////////////////////////////////////////////////////////////
private:
    template <typename T>
    T get_router_sub_node_value(const char* node_name,const T val_default)
    {
        return get_node_value(get_router(),node_name,val_default);
    }
    template <typename T>
    T get_system_sub_onde_vaue(const char* node_name,const T val_default)
    {
        return get_node_value(get_system(),node_name,val_default);
    }

    template <typename T>
    T get_access_cfg_sub_onde_vaue(const char* node_name,const T val_default)
    {
        return get_node_value(get_access_cfg(),node_name,val_default);
    }

    template <typename T>
    T get_snmp_sub_onde_vaue(const char* node_name,const T val_default)
    {
        return get_node_value(get_snmp(),node_name,val_default);
    }

    template < typename T>
    T get_node_value(const xtXmlNodePtr parent_node,const char* node_name,const T val_default)
    {
        xtXmlNodePtr node = m_config.getNode(parent_node,node_name);
        if (node.IsNull())
        {
            return val_default;
        }
        const char *val = m_config.getValue(node);
        if (NULL == val)
        {
            return val_default;
        }
        return boost::lexical_cast<T>(val);
    }
public:
    // xml
    xtXml m_config;
private:
    bool m_valid;
    // 文件路径
    std::string m_fPath;

    std::string systemset_xml_path_;
    bool systemset_xml_valid;
    xtXml systemset_xml_; 

    //作为全局静态变量来初始化，一次初始化写操作，后面全是读操作
    static config self;
};

#endif //_XT_ROUTER_CONFIG_H_INCLUDED
