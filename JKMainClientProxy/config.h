#ifndef _XT_CONFIG_H_INCLUDED
#define _XT_CONFIG_H_INCLUDED

#include "utility/noncopyable.h"
#include <string>
#include "xtXml.h"


namespace config_type
{
    enum link_center_type
    {
        LINK_NA = -1,    //��Ч����
        LINK_ON = 0,     //������
        LINK_XTCENTER,   //����˽������
        LINK_CCS,       //����CCS
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
    //���߼�⿪��Ĭ�Ͽ�
    int break_monitor(int val_default);

    //���߼��Ƶ��Ĭ��60000ms���һ��
    int break_monitor_frequency(int val_default);

    //���߼��ʱ��
    int break_monitor_time(int val_default);

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

    //�㲥��ʽ(0:tcp+tcp/1:xtmsg+rtp/2:xtmsg+rtp mul
    //3:xtmsg+rtp/4:xtmsg+rtp mul/5:xtmsg+rtp demux/6:rtsp+rtp std/7:rtsp+rtp/8:rtsp+rtp demux)
    int link_type(int val_default);

    //ת��ͨ����
    int chan_num(int val_default);

    //���ط���ip
    std::string local_sndip(const std::string& val_default);

    //���ط��Ͷ˿�
    int snd_port(int val_default);

    //���͸��ÿ���
    int demux_s(int val_default);

    //�鲥��ʼ��ַ
    std::string mul_ip(const std::string& val_default);

    //tcp�������&����˿�
    int tcp_listenprt(int val_default);

    //˽�лỰЭ�̼����˿�
    int xtmsg_listenprt(int val_default);

    //rtsp�ỰЭ�̼����˿�
    int rtsp_listenprt(int val_default);

    int udp_listenprt(int val_default);

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
    std::string local_ip(const std::string& val_default);

    //center_port:����port/ccs port
    int center_port(int val_default);

    //local_name:���ط�����
    std::string local_name(const std::string& val_default);

    //login_password:��¼ccs����
    std::string login_password(const std::string& val_default);

    //login_res_id:��¼ccs������ʱʹ�õ���Դid	
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
    // �ļ�·��
    std::string m_fPath;
    std::string m_set_path;
    std::string m_system_path;
    bool m_valid;

    //��Ϊȫ�־�̬��������ʼ����һ�γ�ʼ��д����������ȫ�Ƕ�����
    static config self;
};

#endif //_XT_CONFIG_H_INCLUDED
