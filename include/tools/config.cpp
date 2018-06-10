#include "config.h"

config config::self;

config::config(void)
:m_hMutex(NULL)
,m_fPath("D:/NetMCSet/xtrouter_config.xml")
{
	m_mapMutex.insert(mutex_type(TEXT("jkclient_000"),"D:/NetMCSet/xtrouter_config.xml"));
	m_mapMutex.insert(mutex_type(TEXT("jkclient_001"),"D:/NetMCSet/xtrouter_config1.xml"));
	m_mapMutex.insert(mutex_type(TEXT("jkclient_002"),"D:/NetMCSet/xtrouter_config2.xml"));
	m_mapMutex.insert(mutex_type(TEXT("jkclient_003"),"D:/NetMCSet/xtrouter_config3.xml"));
	m_mapMutex.insert(mutex_type(TEXT("jkclient_004"),"D:/NetMCSet/xtrouter_config4.xml"));
	m_mapMutex.insert(mutex_type(TEXT("jkclient_005"),"D:/NetMCSet/xtrouter_config5.xml"));
	m_mapMutex.insert(mutex_type(TEXT("jkclient_006"),"D:/NetMCSet/xtrouter_config6.xml"));
	m_mapMutex.insert(mutex_type(TEXT("jkclient_007"),"D:/NetMCSet/xtrouter_config7.xml"));
	m_mapMutex.insert(mutex_type(TEXT("jkclient_008"),"D:/NetMCSet/xtrouter_config8.xml"));
	map<string, string>::iterator itr = m_mapMutex.begin();
	for (;itr != m_mapMutex.end();++itr)
	{
		::SetLastError(0);
		m_hMutex = ::CreateMutex(NULL, FALSE, itr->first.c_str());
		if (::GetLastError()==ERROR_ALREADY_EXISTS)
		{
			if (m_hMutex)
			{
				::ReleaseMutex(m_hMutex);
			}
			
			continue;
		}
		else
		{
			m_fPath = itr->second;
			break;
		}
	}

	m_config.open(m_fPath.c_str());
}

config::~config(void)
{
	if (m_hMutex)
	{
		::ReleaseMutex(m_hMutex);
	}
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

int config::break_monitor(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"break_monitor");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str()); 
}

int config::break_monitor_time(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"break_monitor_time");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

int config::check_len(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"check_len");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

int config::check_timeout(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"check_timeout");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

int config::link_center(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"link_center");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

string config::xt_dbtype(string val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"local_ip");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return val;
}

int config::copy_send(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"copy_send");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

int config::link_type(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"link_type");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

int config::chan_num(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"chan_num");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

string config::local_sndip(string val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"local_sndip");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return val;
}

int config::snd_port(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"snd_port");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

int config::demux_s(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"demux_s");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

string config::mul_ip(string val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"mul_ip");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return val;
}

int config::tcp_listenprt(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"tcp_listenport");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

int config::xtmsg_listenprt(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"xtmsg_listenport");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

int config::rtsp_listenprt(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_router(),"rtsp_listenport");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}

string config::center_ip(string val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_system(),"center_ip");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return val;
}

string config::local_ip(string val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_system(),"local_ip");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return val;
}

string config::local_name(string val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_system(),"local_name");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return val;
}

int config::center_port(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_system(),"center_port");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}
int config::center_id(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_system(),"center_id");
	string val = m_config.getValue(node);
	if (val.empty())
	{
		return val_default;
	}

	return ::atoi(val.c_str());
}