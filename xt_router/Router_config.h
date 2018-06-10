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
        LINK_NA = -1,    //��Ч����
        LINK_ON = 0,     //������
        LINK_XTCENTER,   //����˽������
        LINK_CCS,       //����CCS
        LIPK_SC,   //SIP ������Ʒ���Ԫ
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

	//rtsp�������
	unsigned int rtsp_srv_check_timer_interval(unsigned int val_default);
	unsigned int rtsp_srv_time_out_interval(unsigned int val_default);

    //base cfg
    //���߼�⿪��Ĭ�Ͽ�
    int break_monitor_onoff(int val_default);

    //���߼��ʱ����
    uint32_t break_monitor_time_interval(int val_default);

    //״̬��ؿ���
    int status_monitor_switch(int val_default);

    //״̬���Ƶ��
    int status_monitor_frequency(int val_default);
    //ping����С
    int check_len(int val_default);

    //ping��ʱ
    int check_timeout(int val_default);

    //link_center:�������� 0:������ 1:˽������ 2:ccs
    config_type::link_center_type link_center(config_type::link_center_type val_default);

    //���µ㲥����ʹ��link_type
    std::string xt_dbtype(const std::string& val_default);

    //����ת����
    int copy_send(int val_default);

    /*
    0:tcp+tcp/1:xtmsg+rtp/2:xtmsg+rtp mul/3:xtmsg+rtp/4:xtmsg+rtp mul/
    5:xtmsg+rtp demux/6:rtsp+rtp std/7:rtsp+rtp/8:rtsp+rtp demux/9:tcp_rtp_std/10:xmpp+rtp_std/
    11:xmpp+rtp_pri/12:xmpp+rtp_pri_demux/13:udp+rtp_pri/14:udp+rtp_pri_demux/15:˽����UDPЭ�� RTP�ಥ/
    16:˽����UDPЭ�� RTP�ಥ/17:˽����UDPЭ�� ��׼RTP ������/18:˽����UDPЭ�� ��׼RTP�� �ಥ
    */
    int get_link_type(int val_default);

    //ת��ͨ����
    int chan_num(int val_default);

    //���ط���ip
    std::string local_sndip(const std::string& val_default);

    int use_strmtype_param(int val_default);

    //���ط��Ͷ˿�
    int snd_port(int val_default);

    //���͸��ÿ���
    int demux_s(int val_default);

    //�鲥��ʼ��ַ
    std::string mul_ip(const std::string& val_default);

    //tcp�������&����˿�
    int tcp_listenport(int val_default);

    //˽�лỰЭ�̼����˿�
    int xtmsg_listenport(int val_default);

    //rtsp�ỰЭ�̼����˿�
    int rtsp_listenport(int val_default);

    int udp_listenport(int val_default);

    //TCP ����ע��
    bool regist(bool val_default);
    int regist_bind(int val_default);
    std::string regist_ip(const std::string& val_default);
    int regist_port(int val_default);
    int regist_listenprt(int val_default);

    //��׼RTPת������
    bool std_rtp(bool val_default);

    int sink_perch(int val_default);

    //system cfg 
    //center_host:ccs����������
    std::string cneter_host(const std::string& val_default);

    //local_ip:����IP
    //std::string local_ip(const std::string& val_default);

    //center_port:����port/ccs port
    int center_port(int val_default);

    //local_name:���ط�����
    std::string local_name(const std::string& val_default);

    //login_password:��¼ccs����
    std::string login_password(const std::string& val_default);

    //login_res_id:��¼ccs������ʱʹ�õ���Դid	
    std::string login_res_id(const std::string& val_default);

    std::string local_ip(const std::string& val_default);

    //access ����
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
    //��ȡlinux�º�̨���񿪹�
    bool get_daemon_config(bool val_default);
    /////////////////////////////////////////////////////////////////
    //local_mac : ���������ַ
    std::string local_mac(const std::string& val_default);

    //network_interface_guid: ����guid
    std::string network_interface_guid(const std::string& val_default);

    //device_status_index : �豸����
    int device_status_index(int val_default);

    //device_status_code : �豸���
    std::string device_status_code(const std::string& val_default);

    //device_status_contact : �豸��ϵ�绰
    std::string device_status_contact(const std::string& val_default);

    //device_status_manufactory: �豸��������
    std::string device_status_manufactory(const std::string& val_default);

    //device_status_software_version : ����汾
    std::string device_status_software_version(const std::string& val_default);

    //device_status_hardware_version : Ӳ���汾
    std::string device_status_hardware_version(const std::string& val_default);

    //device_status_xh : �豸�ͺ�
    std::string device_status_xh(const std::string& val_default);

    //device_status_type:�豸����
    int device_status_type(int val_default);
    // web����˿�
    int get_web_server_port(int val_default);

    //device_status_temperature_file : ָ��LINUXϵͳCPU�¶��ļ�
    std::string device_status_temperature_file(const std::string& val_default);

    //��������
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

    //SIP�������
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
    // �ļ�·��
    std::string m_fPath;

    std::string systemset_xml_path_;
    bool systemset_xml_valid;
    xtXml systemset_xml_; 

    //��Ϊȫ�־�̬��������ʼ����һ�γ�ʼ��д����������ȫ�Ƕ�����
    static config self;
};

#endif //_XT_ROUTER_CONFIG_H_INCLUDED
