#include "config.h"
#ifdef _WIN32
#include "utility/winapi.h"
#else
#include "utility/function.h"
#endif
#ifdef _WIN32 
#include <windows.h>
#endif

using namespace utility;

config config::self;

config::config(void)
:m_valid(false),m_fPath("")
{ 
#ifdef  _WIN32
    m_fPath.assign("D:/NetMCSet/xtrouter_config.xml");
    m_set_path.assign("D:/NetMCSet/set.xml");
    m_system_path.assign("D:/NetMCSet/systemset.xml");
#else
    m_fPath.assign("/etc/xtconfig/d/netmcset/xtrouter_config.xml");
    m_set_path.assign("/etc/xtconfig/d/netmcset/set.xml");
    m_system_path.assign("/etc/xtconfig/d/netmcset/systemset.xml");
#endif//#ifdef _WIN32

    m_valid = m_config.open(m_fPath.c_str());

    m_valid = m_system_config.open( m_system_path.c_str());

    m_valid = m_set_config.open( m_set_path.c_str());
}

config::~config(void)
{
}

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

xtXmlNodePtr config::get_client()
{
    xtXmlNodePtr root = m_config.getRoot();
    if (root.IsNull())
    {
        return root;
    }

    xtXmlNodePtr system = m_config.getNode(root, "client");

    return system;
}

int config::break_monitor_frequency(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"break_monitor_frequency");
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

int config::break_monitor(int val_default)
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

int config::break_monitor_time(int val_default)
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

    xtXmlNodePtr node = m_system_config.getNode(get_system_set(),"link_center");
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

int config::link_type(int val_default)
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

int config::tcp_listenprt(int val_default)
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

int config::xtmsg_listenprt(int val_default)
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

int config::rtsp_listenprt(int val_default)
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

int config::udp_listenprt(int val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_router(),"udp_listenprt");
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

std::string config::local_ip(const std::string& val_default)
{
#ifndef _WIN32
    xtXmlNodePtr node = m_system_config.getNode(get_system_set(),"localip");
    if (node.IsNull())
    {
        return val_default;

    } 
    const char* val = m_system_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;
#else
    char strTemp[256] = "";
    ::GetPrivateProfileString("system", "LocalIP", val_default.c_str(), strTemp,sizeof(strTemp),"d:/NetMCSet/systemset.ini");
    return strTemp;
#endif
}

//local_name:本地服务名
std::string config::local_name(const std::string& val_default)
{
#ifndef _WIN32
    xtXmlNodePtr node = m_system_config.getNode(get_system_set(),"localname");
    if (node.IsNull())
    {
        return val_default;
    }

    const char* val = m_system_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return val;
#else
    char strTemp[256] = "";
    ::GetPrivateProfileString("system", "LocalName", val_default.c_str(), strTemp,sizeof(strTemp),"d:/NetMCSet/systemset.ini");
    return strTemp;
#endif
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
#ifndef _WIN32
    xtXmlNodePtr node = m_system_config.getNode(get_system_set(),"linkcenterport");
    if (node.IsNull())
    {
        return val_default;
    }

    const char *val = m_system_config.getValue(node);
    if (NULL == val)
    {
        return val_default;
    }

    return ::atoi(val);
#else
    return  ::GetPrivateProfileInt("system", "LinkCenterPort", val_default,"d:/NetMCSet/systemset.ini");
#endif
}

std::string config::center_ip(std::string val_default)
{
#ifndef _WIN32
    xtXmlNodePtr node = m_system_config.getNode(get_system_set(),"centerip");
    std::string val = m_system_config.getValue(node);
    if (val.empty())
    {
        return val_default;
    }
    return val;
#else
    char strTemp[256] = "";
    ::GetPrivateProfileString("system", "CenterIP", val_default.c_str(), strTemp,sizeof(strTemp),"d:/NetMCSet/systemset.ini");
    return strTemp;
#endif
}


xtXmlNodePtr config::get_system_set()
{
    xtXmlNodePtr root = m_system_config.getRoot();
    if (root.IsNull())
    {
        return root;
    }

    xtXmlNodePtr system = m_system_config.getNode(root, "system");

    return system;
}
//modify by wangyin
int config::center_id(int val_default)
{
#ifndef _WIN32

    xtXmlNodePtr node = m_system_config.getNode(get_system_set(),"linkcenterid");
    std::string val = m_system_config.getValue(node);
    if (val.empty())
    {
        return val_default;
    }

    return ::atoi(val.c_str());
#else
    return ::GetPrivateProfileInt("system", "LinkCenterID", val_default,"d:/NetMCSet/systemset.ini");
#endif
}

long config::center_link_type(long val_default)
{
    xtXmlNodePtr node = m_system_config.getNode(get_system_set(),"center_link_type");
    std::string val(m_system_config.getValue(node));
    if (val.empty())
    {
        return val_default;
    }

    return ::atoi(val.c_str());

}

bool config::auto_start_router(const bool val_default)
{
    xtXmlNodePtr node = m_config.getNode(get_system(),"auto_start_router");
    std::string val = m_config.getValue(node);
    if (val.empty())
    {
        return val_default;
    }

    int ret_val = ::atoi(val.c_str());

    return  ret_val > 0 ? true :  false; 
}

xtXmlNodePtr config::get_center()
{
    xtXmlNodePtr root = m_set_config.getRoot();
    if (root.IsNull())
    {
        return root;
    }

    xtXmlNodePtr center = m_set_config.getNode(root, "center");

    return center;
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

bool config::is_write_JKMainClientLog( const bool val_default )
{
    xtXmlNodePtr node = m_config.getNode(get_log_level(),"JKMainClientLog");
    std::string val = m_config.getValue(node);
    if (val.empty())
    {
        return val_default;
    }

    int ret_val = ::atoi(val.c_str());

    return  ret_val > 0 ? true :  false; 

}

bool config::get_daemon_config( bool val_default )
{

    xtXmlNodePtr node = m_config.getNode(get_router(),"daemon_JKMainClient");
    std::string val = m_config.getValue(node);
    if (val.empty())
    {
        return val_default;
    }

    int ret_val = ::atoi(val.c_str());

    return  ret_val > 0 ? true :  false; 

}



