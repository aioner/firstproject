#include "Router_config.h"

#ifdef _WIN32
#include <Windows.h>
#define CFG_FILE_PATH "D:/NetMCSet/"
#else
#define CFG_FILE_PATH "/etc/xtconfig/d/netmcset/"
#endif //#ifdef _WIN32

config config::self;
config::config(void)
:m_valid(false),
systemset_xml_valid(false)
{
#ifdef _WIN32
    m_fPath.assign(CFG_FILE_PATH"xtrouter_config.xml");
    systemset_xml_path_.assign(CFG_FILE_PATH"systemset.xml");
#else
    m_fPath.assign(CFG_FILE_PATH"xtrouter_config.xml");
    systemset_xml_path_.assign(CFG_FILE_PATH"systemset.xml");
#endif//#ifdef _WIN32
    m_valid = m_config.open(m_fPath.c_str());
    systemset_xml_valid = systemset_xml_.open(systemset_xml_path_.c_str());
}

config::~config(void)
{
}
//systemset.xml
xtXmlNodePtr config::get_systemset_xml_system_root()
{
    xtXmlNodePtr root = systemset_xml_.getRoot();
    if (root.IsNull())
    {
        return root;
    }
    xtXmlNodePtr router = systemset_xml_.getNode(root, "system");
    return router;
}

#ifdef _WIN32
#include <Windows.h>
#endif //#ifdef _WIN32
std::string config::get_local_ip_systemset(const std::string& val_default)
{
#ifdef _WIN32
    char strTemp[256] = "";
    ::GetPrivateProfileString("system", "LocalIP", val_default.c_str(), strTemp,sizeof(strTemp),"d:/NetMCSet/systemset.ini");
    return strTemp;
#else
    xtXmlNodePtr node = systemset_xml_.getNode(get_systemset_xml_system_root(),"localip");
    if (node.IsNull())
    {
        return val_default;
    }
    const char* val = systemset_xml_.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return val;
#endif //#ifdef _WIN32
}

//xtrouter_config.xml
xtXmlNodePtr config::get_router()
{
    xtXmlNodePtr root = m_config.getRoot();
    if (root.IsNull())
    {
        return root;
    }

    xtXmlNodePtr router = m_config.getNode(root, "router");

    return router;
}

xtXmlNodePtr config::get_system()
{
    xtXmlNodePtr root = m_config.getRoot();
    if (root.IsNull())
    {
        return root;
    }

    xtXmlNodePtr system = m_config.getNode(root, "system");

    return system;
}
xtXmlNodePtr config::get_access_cfg()
{
    xtXmlNodePtr root = m_config.getRoot();
    if (root.IsNull())
    {
        return root;
    }

    xtXmlNodePtr access_cfg = m_config.getNode(root, "access_cfg");

    return access_cfg;

}

uint32_t config::mtu(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"mtu");
    if (node.IsNull())
    {
        return val_default;
    }
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);
}

unsigned int config::rtsp_srv_check_timer_interval(unsigned int val_default)
{
   return get_router_sub_node_value("rtsp_srv_check_timer_interval",val_default);
}
unsigned int config::rtsp_srv_time_out_interval(unsigned int val_default)
{
    return get_router_sub_node_value("rtsp_srv_time_out_interval",val_default);
}

int config::break_monitor_onoff(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"break_monitor");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val); 
}

uint32_t config::break_monitor_time_interval(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"break_monitor_time");
    if (node.IsNull())
    {
        return val_default;
    }

    const char * val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    uint32_t ret = ::atoi(val);

    return  ret <= 0 ? val_default : ret;
}

//For status monitoring wluo
int config::status_monitor_switch(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"status_monitor_switch");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);	 
}

int config::status_monitor_frequency(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"status_monitor_frequency");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val); 
}
int config::check_len(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"check_len");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

int config::check_timeout(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"check_timeout");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

/*
*/
config_type::link_center_type config::link_center(config_type::link_center_type val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"link_center");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    config_type::link_center_type ret_center_type;

    switch(::atoi(val))
    {
    case 0:
        {
            ret_center_type = config_type::LINK_ON;
            break;
        }

    case 1:
        {
            ret_center_type = config_type::LINK_XTCENTER;
            break;
        }

    case 2:
        {
            ret_center_type = config_type::LINK_CCS;
            break;
        }
    case 3:
        {
            ret_center_type = config_type::LIPK_SC;
            break;
        }

    default:
        {
            ret_center_type = config_type::LINK_NA;
            break;
        }
    }

    return ret_center_type;
}

std::string config::xt_dbtype(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"xt_dbtype");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;
}

int config::copy_send(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"copy_send");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

int config::get_link_type(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"link_type");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

int config::chan_num(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"chan_num");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

int config::use_strmtype_param(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"use_strmtype_param");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
std::string config::local_sndip(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"local_sndip");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;
}

int config::snd_port(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"snd_port");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

int config::demux_s(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"demux_s");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

std::string config::mul_ip(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"mul_ip");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;
}

int config::tcp_listenport(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"tcp_listenport");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

int config::xtmsg_listenport(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"xtmsg_listenport");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

int config::rtsp_listenport(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"rtsp_listenport");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

int config::udp_listenport(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"udp_listenport");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}

bool config::std_rtp(bool val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"std_rtp");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return (1 == ::atoi(val));
}

bool config::regist(bool val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"regist");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return (0 < ::atoi(val));
}

int config::regist_bind(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"regist_bind");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

std::string config::regist_ip(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"regist_ip");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;
}

int config::regist_port(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"regist_port");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

int config::regist_listenprt(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"regist_listenprt");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

int config::sink_perch(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"sink_perch");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

//本地上报中心IP
std::string config::local_ip(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_system(),"local_ip");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;
}

//local_name:本地服务名
std::string config::local_name(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_system(),"local_name");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}


std::string config::login_password(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_system(),"login_password");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}

std::string config::login_res_id(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_system(),"login_res_id");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}

//center_host:ccs服务器名称
std::string config::cneter_host(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_system(),"center_host");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}

//center_port:中心port/ccs port
int config::center_port(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_system(),"center_port");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}

//SIP
xtXmlNodePtr config::get_sip_cfg()
{
    xtXmlNodePtr root = m_config.getRoot();
    if (root.IsNull())
    {
        return root;
    }
    xtXmlNodePtr sip_cfg = m_config.getNode(root, "sip_cfg");
    return sip_cfg;
}

void config::sip_cfg_get_domain(std::list<std::string>& domains)
{
    domains.clear();
    xtXmlNodePtr domain = m_config.getNode(get_sip_cfg(),"domain");
    if (domain.IsNull())
    {
        return;
    }

    std::string cfg_val;
    xtXmlNodePtr ctx = domain.GetFirstChild("domain");
    for( ;!ctx.IsNull(); ctx = ctx.NextSibling("domain"))
    {
        const char *val = m_config.getValue(ctx);
        if (NULL == val)
        {
            continue;
        }
        cfg_val.clear();
        cfg_val.assign(val);
        domains.push_back(cfg_val);
    }
}

std::string config::sip_cfg_commlibtype(const std::string val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"commlibtype");
	if (node.IsNull())
	{
		return val_default;
	}

	const char* val = m_config.getValue(node);
	if (NULL == val)
	{
		return val_default;
	}

	return val;
}

std::string config::sip_cfg_serverids(const std::string val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"serverids");
	if (node.IsNull())
	{
		return val_default;
	}

	const char* val = m_config.getValue(node);
	if (NULL == val)
	{
		return val_default;
	}

	return val;
}


std::string config::sip_cfg_usre(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"user");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}
std::string config::sip_cfg_password(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"password");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}
uint32_t config::sip_cfg_protocol(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"protocol");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_transport(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"transport");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_tls_port(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"tls_port");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_dtls_port(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"dtls_port");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_expires(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"expires");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_session_keep_alive_time_interval(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"session_keep_alive_time_interval");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_registration_retry_time_interval(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"registration_retry_time_interval");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_link_keep_alive_time_interval(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"link_keep_alive_time_interval");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_regist_retry_time_interval(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"regist_retry_time_interval");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_channle_type(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"channle_type");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_delay(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"delay");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_packetloss(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"packetloss");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
uint32_t config::sip_cfg_bandwidth(const uint32_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_sip_cfg(),"bandwidth");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);
}

//access 配置
/////////////////////////////////////////////////////////////////
long config::rtp_recv_port_num(long val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"rtp_recv_port_num");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
long config::rtp_recv_start_port(long val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"rtp_recv_start_port");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::udp_session_bind_port(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"udp_session_bind_port");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::udp_session_heartbit_proid(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"udp_session_heartbit_proid");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::udp_session_request_try_count(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"udp_session_request_try_count");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::udp_session_request_one_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"udp_session_request_one_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::tcp_session_bind_port(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"tcp_session_bind_port");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::tcp_session_connect_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"tcp_session_connect_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::tcp_session_login_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"tcp_session_login_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::tcp_session_play_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"tcp_session_play_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::tcp_session_stop_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"tcp_session_stop_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::rtsp_session_connect_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"rtsp_session_connect_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::rtsp_session_describe_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"rtsp_session_describe_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::rtsp_session_setup_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"rtsp_session_setup_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::rtsp_session_play_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"rtsp_session_play_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::rtsp_session_pause_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"rtsp_session_pause_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}
uint16_t config::rtsp_session_teardown_timeout(uint16_t val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_access_cfg(),"rtsp_session_teardown_timeout");
    if (node.IsNull())
    {
        return val_default;
    } 
    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return ::atoi(val);

}

bool config::get_daemon_config( bool val_default )
{

    xtXmlNodePtr node = m_config.getNode(get_router(),"daemon");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }
    return (0 < ::atoi(val));
}
/////////////////////////////////////////////////////////////////
//Add snmp node by wluo
xtXmlNodePtr config::get_snmp()
{
    xtXmlNodePtr root = m_config.getRoot();
    if (root.IsNull())
    {
        return root;
    }

    xtXmlNodePtr snmp = m_config.getNode(root, "snmp");

    return snmp;
}

xtXmlNodePtr config::get_log_level()
{
	xtXmlNodePtr root = m_config.getRoot();
	if (root.IsNull())
	{
		return root;
	}

	xtXmlNodePtr log = m_config.getNode(root, "log");

	return log;
}

//Read snmp configure by wluo
std::string config::local_mac(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"local_mac");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;
}

std::string config::network_interface_guid(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"network_interface_guid");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}

//device_status_index : 设备索引
int config::device_status_index(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_snmp(),"device_status_index");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

//device_status_code : 设备编号
std::string config::device_status_code(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_snmp(),"device_status_code");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;
}

std::string config::device_status_manufactory(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_snmp(),"device_status_manufactory");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}

std::string config::device_status_contact(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_snmp(),"device_status_contact");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}

std::string config::device_status_software_version(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_snmp(),"device_status_software_version");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}

std::string config::device_status_hardware_version(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_snmp(),"device_status_hardware_version");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}

std::string config::device_status_xh(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_snmp(),"device_status_xh");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;

}


int config::device_status_type(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_snmp(),"device_status_type");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
}

std::string config::device_status_temperature_file(const std::string& val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_snmp(),"device_status_temperature_file");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;
}

int config::get_web_server_port( int val_default )
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"web_service_port");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);

}
//end snmp configure

#include <fstream>
std::string config::default_sdp()
{
    std::string ret_sdp("");
    do
    {
        std::ifstream sdp_file(CFG_FILE_PATH"default_sdp.sdp");
        if (sdp_file.fail()) break;
        while(!sdp_file.eof())
        {
            std::string temp;
            std::getline(sdp_file,temp);
            ret_sdp.append(temp);
            ret_sdp.append("\n");
        }
    } while (0);
    return ret_sdp;
}
int config::log_level(const int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_log_level(),"XTRouterLog");
	std::string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	int ret_val = ::atoi(val.c_str());

	return  ret_val > 0 ? true :  false; 

}

//end snmp configure

