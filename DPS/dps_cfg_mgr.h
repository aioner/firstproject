//
//create by songlei 20160316
//
#ifndef DPS_CFG_MGR_H__
#define DPS_CFG_MGR_H__
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <stdint.h>
#include <vector>
#include "xtXml.h"
#include "dps_common_type_def.h"

typedef enum
{
    LINK_NA = -1,    // ��Ч����
    LINK_ON = 0,     // ������
    LINK_XTCENTER,   // ����˽������
    LINK_CCS,        // ����CCS
    LIPK_SC,         // SIP ������Ʒ���Ԫ
}link_center_type_t;

class dps_cfg_mgr : boost::noncopyable
{
private:
    dps_cfg_mgr():dps_cfg_xml(),dps_cfg_valid(false),dps_cfg_fpath(""),
        system_set_xml(),systme_set_valid(false),system_set_fpath("")
    {}
    ~dps_cfg_mgr(){}
private:
    xtXmlNodePtr dps_cfg_node(const char* node_name)
    {
        xtXmlNodePtr root = dps_cfg_xml.getRoot();
        if (root.IsNull())
        {
            return root;
        }
        return dps_cfg_xml.getNode(root, node_name);;
    }

    template < typename T>
    T dps_get_cfg_node_value(const char* parent_node_name,const char* sub_node_name,const T val_default)
    {
        xtXmlNodePtr parent_node;
        do
        {
            xtXmlNodePtr root = dps_cfg_xml.getRoot();
            if (root.IsNull())
            {
                parent_node = root;
                break;
            }
            parent_node = dps_cfg_xml.getNode(root, parent_node_name);
        } while (0);

        xtXmlNodePtr sub_node = dps_cfg_xml.getNode(parent_node,sub_node_name);
        if (sub_node.IsNull())
        {
            return val_default;
        }
        const char *val = dps_cfg_xml.getValue(sub_node);
        if (NULL == val)
        {
            return val_default;
        }
        return boost::lexical_cast<T>(val);
    }

public:
    static dps_cfg_mgr* _(){return &my_;}
    int loading();
    int unload();

public:
    //system
    /////////////////////////////////////////////////////////////////////////////////////
    //������������ 0:������ 1:˽������
    link_center_type_t link_center(const link_center_type_t default_val);

    //�㲥��·��ʽ
    uint16_t link_type(const uint16_t default_val);

    //֧�����ͨ����
    uint32_t chan_num(const uint32_t default_val);

    //�������ݱ��ذ�IP
    std::string local_bind_ip(const std::string& default_val);

    //�������ݱ��ؿ�ʼ�˿�
    uint16_t send_port(const uint16_t default_val);

    //�ಥIP
    std::string mul_ip(const std::string& default_val);

    //���Ͷ��Ƿ�֧�ָ���
    bool is_demux_s(const bool default_val);

    //4.25��TCP˽��Э�������˿�
    uint16_t xtmsg_listenport(const uint16_t default_val);

    //��׼rtspЭ�������˿�
    uint16_t rtsp_listenport(const uint16_t default_val);

    //˽��udpЭ�������˿�
    uint16_t udp_listenport(const uint16_t default_val);

    //�Ƿ�֧��rtp��׼��
    bool is_std_rtp(const bool default_val);

    //NAT����ע�Ὺ��
    bool is_regist(const bool default_val);

    //NAT����ע��IP
    std::string regist_ip(const std::string& default_val);

    //NAT����ע��˿�
    uint16_t regist_port(const uint16_t default_val);

    //�Ƿ�֧��NAT��͸һ��һת��
    bool is_sink_perch(const bool default_val);

    //�ص�ʱ���� ��λ���� Ĭ��100ms
    uint32_t replay_time_interval(const uint32_t default_val);

    //�������ݶ��߼��ʱ���� ��λ���� Ĭ��30000ms
    uint32_t break_monitor_time_interval(const uint32_t default_val);
    /////////////////////////////////////////////////////////////////////////////////////

    //access
    /////////////////////////////////////////////////////////////////////////////////////
    long rtp_recv_port_num(const long default_val);
    long rtp_recv_start_port(const long default_val);
    uint16_t udp_session_bind_port(const uint16_t default_val);
    uint16_t udp_session_heartbit_proid(const uint16_t default_val);
    uint16_t udp_session_request_try_count(const uint16_t default_val);
    uint16_t udp_session_request_one_timeout(const uint16_t default_val);
    uint16_t tcp_session_bind_port(const uint16_t default_val);
    uint16_t tcp_session_connect_timeout(const uint16_t default_val);
    uint16_t tcp_session_login_timeout(const uint16_t default_val);
    uint16_t tcp_session_play_timeout(const uint16_t default_val);
    uint16_t tcp_session_stop_timeout(const uint16_t default_val);
    uint16_t rtsp_session_connect_timeout(const uint16_t default_val);
    uint16_t rtsp_session_describe_timeout(const uint16_t default_val);
    uint16_t rtsp_session_setup_timeout(const uint16_t default_val);
    uint16_t rtsp_session_play_timeout(const uint16_t default_val);
    uint16_t rtsp_session_pause_timeout(const uint16_t default_val);
    uint16_t rtsp_session_teardown_timeout(const uint16_t default_val);
    /////////////////////////////////////////////////////////////////////////////////////
    typedef std::vector<device_t> dps_dev_list_container_t;
    typedef dps_dev_list_container_t::iterator dps_dev_list_container_itr_t;
    void loading_dev_list(dps_dev_list_container_t& dev_lst);


    //systemset.xml
    /////////////////////////////////////////////////////////////////////////////////////
    xtXmlNodePtr systemset_cfg_node(const char* node_name)
    {
        xtXmlNodePtr root = system_set_xml.getRoot();
        if (root.IsNull())
        {
            return root;
        }
        return system_set_xml.getNode(root, node_name);;
    }
    template < typename T>
    T systme_set_get_cfg_node_value(const char* parent_node_name,const char* sub_node_name,const T val_default)
    {
        xtXmlNodePtr parent_node;
        do
        {
            xtXmlNodePtr root = system_set_xml.getRoot();
            if (root.IsNull())
            {
                parent_node = root;
                break;
            }
            parent_node = system_set_xml.getNode(root, parent_node_name);
        } while (0);

        xtXmlNodePtr sub_node = system_set_xml.getNode(parent_node,sub_node_name);
        if (sub_node.IsNull())
        {
            return val_default;
        }
        const char *val = system_set_xml.getValue(sub_node);
        if (NULL == val)
        {
            return val_default;
        }
        return boost::lexical_cast<T>(val);
    }

    std::string localip(const std::string& default_val);
    std::string centerip(const std::string& default_val);
    uint16_t linkcenterport(const uint16_t default_val);
    uint16_t linkcenterid(const uint16_t default_val);
    uint16_t center_link_type(const uint16_t default_val);


    /////////////////////////////////////////////////////////////////////////////////////
private:
   static dps_cfg_mgr my_;
private:
   xtXml dps_cfg_xml;
   bool dps_cfg_valid;
   std::string dps_cfg_fpath;

   xtXml system_set_xml;
   std::string system_set_fpath;
   bool systme_set_valid;
};


#endif // #ifndef DPS_CFG_MGR_H__
