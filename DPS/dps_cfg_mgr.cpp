//
//create by songlei 20160316
//
#include "dps_cfg_mgr.h"
#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#ifdef _DEBUG
# pragma comment(lib,"xt_xml_d.lib")
#pragma message("Auto Link xt_xml_d.lib")
#else
# pragma comment(lib,"xt_xml.lib")
#pragma message("Auto Link xt_xml.lib")
#endif // end #ifdef _DEBUG
#define CFG_FILE_PATH "D:/NetMCSet/"
#else
#define CFG_FILE_PATH "/etc/xtconfig/d/netmcset/"
#endif // end #ifdef _WIN32

dps_cfg_mgr dps_cfg_mgr::my_;

int dps_cfg_mgr::loading()
{
    int ret_code = -1;
    do 
    {
        dps_cfg_fpath.assign(CFG_FILE_PATH"dps_cfg.xml");
        if (dps_cfg_fpath.empty()) break;
        dps_cfg_valid = dps_cfg_xml.open(dps_cfg_fpath.c_str());
		if (dps_cfg_valid == false)
		{
			return -1;
		}

        system_set_fpath.assign(CFG_FILE_PATH"systemset.xml");
        if (system_set_fpath.empty()) break;
        systme_set_valid = system_set_xml.open(system_set_fpath.c_str());
		if (systme_set_valid == false)
		{
			return -2;
		}

        ret_code = 0;
    } while (0);
    return ret_code;

}
int dps_cfg_mgr::unload()
{
    int ret_code = -1;
    do 
    {
        ret_code = 0;
    } while (0);
    return ret_code;
}

link_center_type_t dps_cfg_mgr::link_center(const link_center_type_t default_val)
{
    uint64_t val = dps_get_cfg_node_value<uint64_t>("system","link_center",-1);
    if (-1 == val) return default_val;
    link_center_type_t ret_center_type = LINK_NA;
    switch(val)
    {
    case 0:
        {
            ret_center_type = LINK_ON;
            break;
        }
    case 1:
        {
            ret_center_type = LINK_XTCENTER;
            break;
        }
    case 2:
        {
            ret_center_type = LINK_CCS;
            break;
        }
    case 3:
        {
            ret_center_type = LIPK_SC;
            break;
        }
    default:
        {
            ret_center_type = LINK_NA;
            break;
        }
    }
    return ret_center_type;
}

uint16_t dps_cfg_mgr::link_type(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","link_type",default_val);
}

uint32_t dps_cfg_mgr::chan_num(const uint32_t default_val)
{
    return dps_get_cfg_node_value<uint32_t>("system","chan_num",default_val);
}

std::string dps_cfg_mgr::local_bind_ip(const std::string& default_val)
{
    return dps_get_cfg_node_value<std::string>("system","local_bind_ip",default_val);
}

uint16_t dps_cfg_mgr::send_port(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","send_port",default_val);
}

std::string dps_cfg_mgr::mul_ip(const std::string& default_val)
{
    return dps_get_cfg_node_value<std::string>("system","mul_ip",default_val);
}

bool dps_cfg_mgr::is_demux_s(const bool default_val)
{
    return dps_get_cfg_node_value<bool>("system","is_demux_s",default_val);
}

uint16_t dps_cfg_mgr::xtmsg_listenport(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","xtmsg_listenport",default_val);
}

uint16_t dps_cfg_mgr::rtsp_listenport(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","rtsp_listenport",default_val);
}

uint16_t dps_cfg_mgr::udp_listenport(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","udp_listenport",default_val);
}

bool dps_cfg_mgr::is_std_rtp(const bool default_val)
{
    return dps_get_cfg_node_value<bool>("system","is_std_rtp",default_val);
}

bool dps_cfg_mgr::is_regist(const bool default_val)
{
    return dps_get_cfg_node_value<bool>("system","is_regist",default_val);
}

std::string dps_cfg_mgr::regist_ip(const std::string& default_val)
{
    return dps_get_cfg_node_value<std::string>("system","regist_ip",default_val);
}

uint16_t dps_cfg_mgr::regist_port(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","regist_port",default_val);
}

bool dps_cfg_mgr::is_sink_perch(const bool default_val)
{
    return dps_get_cfg_node_value<bool>("system","is_sink_perch",default_val);
}

uint32_t dps_cfg_mgr::replay_time_interval(const uint32_t default_val)
{
    return dps_get_cfg_node_value<uint32_t>("system","replay_time_interval",default_val);
}

uint32_t dps_cfg_mgr::break_monitor_time_interval(const uint32_t default_val)
{
    return dps_get_cfg_node_value<uint32_t>("system","break_monitor_time_interval",default_val);
}

long dps_cfg_mgr::rtp_recv_port_num(const long default_val)
{
    return dps_get_cfg_node_value<long>("system","rtp_recv_port_num",default_val);
}

long dps_cfg_mgr::rtp_recv_start_port(const long default_val)
{
    return dps_get_cfg_node_value<long>("system","rtp_recv_start_port",default_val);
}

uint16_t dps_cfg_mgr::udp_session_bind_port(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","udp_session_bind_port",default_val);
}

uint16_t dps_cfg_mgr::udp_session_heartbit_proid(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","udp_session_heartbit_proid",default_val);
}

uint16_t dps_cfg_mgr::udp_session_request_try_count(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","udp_session_request_try_count",default_val);
}

uint16_t dps_cfg_mgr::udp_session_request_one_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","udp_session_request_one_timeout",default_val);
}

uint16_t dps_cfg_mgr::tcp_session_bind_port(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","tcp_session_bind_port",default_val);
}

uint16_t dps_cfg_mgr::tcp_session_connect_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","tcp_session_connect_timeout",default_val);
}

uint16_t dps_cfg_mgr::tcp_session_login_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<>("system","",default_val);
}

uint16_t dps_cfg_mgr::tcp_session_play_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","tcp_session_play_timeout",default_val);
}

uint16_t dps_cfg_mgr::tcp_session_stop_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","tcp_session_stop_timeout",default_val);
}

uint16_t dps_cfg_mgr::rtsp_session_connect_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","rtsp_session_connect_timeout",default_val);
}

uint16_t dps_cfg_mgr::rtsp_session_describe_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","rtsp_session_describe_timeout",default_val);
}

uint16_t dps_cfg_mgr::rtsp_session_setup_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","rtsp_session_setup_timeout",default_val);
}

uint16_t dps_cfg_mgr::rtsp_session_play_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","rtsp_session_play_timeout",default_val);
}

uint16_t dps_cfg_mgr::rtsp_session_pause_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","rtsp_session_pause_timeout",default_val);
}

uint16_t dps_cfg_mgr::rtsp_session_teardown_timeout(const uint16_t default_val)
{
    return dps_get_cfg_node_value<uint16_t>("system","rtsp_session_teardown_timeout",default_val);
}

bool SortDevList(const device_t &d1, const device_t &d2)
{
	if (d1.transmit_ch < d2.transmit_ch)
	{
		return true;
	}
	else if (d1.transmit_ch == d2.transmit_ch)
	{
		if (d1.stream_type < d2.stream_type)
		{
			return true;
		}
	}

	return false;
}

void dps_cfg_mgr::loading_dev_list(dps_dev_list_container_t& dev_lst)
{
    do 
    {
        dev_lst.clear();
        xtXmlNodePtr node_dps_cfg = dps_cfg_xml.getRoot();
        if (node_dps_cfg.IsNull()) break;

        xtXmlNodePtr node_dev_list = dps_cfg_xml.getNode(node_dps_cfg, "device_list");
        if (node_dev_list.IsNull()) break;
        device_t tmp;
        for(xtXmlNodePtr dev = node_dev_list.GetFirstChild("dev") ;!dev.IsNull(); dev = dev.NextSibling("dev"))
        {
            tmp.transmit_ch = boost::lexical_cast<dps_ch_t>(dev.GetAttribute("transmit_ch"));
            tmp.dev_type = boost::lexical_cast<long>(dev.GetAttribute("type"));
            std::strncpy(tmp.ip,dev.GetAttribute("ip"),DPS_MAX_IP_LEN);
            tmp.port = boost::lexical_cast<uint16_t>(dev.GetAttribute("port"));
            std::strncpy(tmp.usr,dev.GetAttribute("usr"),DPS_MAX_USR_NAME);
            std::strncpy(tmp.password,dev.GetAttribute("password"),DPS_MAX_PASSWORD);
            tmp.dev_ch = boost::lexical_cast<long>(dev.GetAttribute("ch"));
            tmp.stream_type = boost::lexical_cast<long>(dev.GetAttribute("stream_type"));
            int link_type = boost::lexical_cast<int>(dev.GetAttribute("link_type"));
            tmp.link_type = link_type_cast<int>(link_type);
            dev_lst.push_back(tmp);
        }
		std::sort(dev_lst.begin(), dev_lst.end(), SortDevList);

    } while (0);
}

//systemet
///////////////////////////////////////////////////////////////////
std::string dps_cfg_mgr::localip(const std::string& default_val)
{
//#ifdef _WIN32
//    char strTemp[256] = {0};
//    ::GetPrivateProfileString("system", "LocalIP", default_val.c_str(), strTemp,sizeof(strTemp),"d:/NetMCSet/systemset.ini");
//    return strTemp;
//#else
    return systme_set_get_cfg_node_value<std::string>("system","localip",default_val);
//#endif // end #ifdef _WIN32

}
std::string dps_cfg_mgr::centerip(const std::string& default_val)
{
   // char strTemp[256] = {0};
    //::GetPrivateProfileString("system", "CenterIP", default_val.c_str(), strTemp,sizeof(strTemp),"d:/NetMCSet/systemset.ini");
    //return strTemp;
    return systme_set_get_cfg_node_value<std::string>("system","centerip",default_val);
}
uint16_t dps_cfg_mgr::linkcenterport(const uint16_t default_val)
{
    //return  ::GetPrivateProfileInt("system", "LinkCenterPort", default_val,"d:/NetMCSet/systemset.ini");
    return systme_set_get_cfg_node_value<uint16_t>("system","linkcenterport",default_val);
}
uint16_t dps_cfg_mgr::linkcenterid(const uint16_t default_val)
{
    //return ::GetPrivateProfileInt("system", "LinkCenterID", default_val,"d:/NetMCSet/systemset.ini");
    return systme_set_get_cfg_node_value<uint16_t>("system","linkcenterid",default_val);
}
uint16_t dps_cfg_mgr::center_link_type(const uint16_t default_val)
{
	return systme_set_get_cfg_node_value<uint16_t>("system","center_link_type",default_val);
	//return ::GetPrivateProfileInt("system", "CenterLinkType",default_val,"d:/NetMCSet/systemset.ini");
}
///////////////////////////////////////////////////////////////////
