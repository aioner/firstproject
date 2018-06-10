#include "sink_config.h"
#ifdef _WIN32
#include "utility/winapi.h"
#else
#include "utility/function.h"
#endif

using namespace utility;

sink_config sink_config::self;

sink_config::sink_config(void)
:m_fPath("/etc/xtconfig/d/netmcset/xtrouter_config.xml")
,m_valid(false)
{
#ifdef _WIN32
m_fPath = "D:/NetMCSet/xtrouter_config.xml";
#endif
	m_valid = m_config.open(m_fPath.c_str());
}

sink_config::~sink_config(void)
{
}

xtXmlNodePtr sink_config::get_cfg()
{
	xtXmlNodePtr root = m_config.getRoot();
	if (root.IsNull())
	{
		return root;
	}

	xtXmlNodePtr rtp = m_config.getNode(root, "sink_cfg");

	return rtp;
}

int sink_config::log_level(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"log_level");
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

int sink_config::frame_cache(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"frame_cache");
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

int sink_config::pkt_cache(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"pkt_cache");
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

int sink_config::resend(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"resend");
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

int sink_config::lost(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"lost");
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

int sink_config::tm_readd_rmt(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"tm_readd_rmt");
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

int sink_config::wait_resend(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"wait_resend");
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

int sink_config::max_resend(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"max_resend");
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

int sink_config::vga_order(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"vga_order");
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

int sink_config::jarless_packets(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"jarless_packets");
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

int sink_config::sync_packets(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"sync_packets");
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

int sink_config::wait_frames(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"wait_frames");
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

int sink_config::check_sum(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"check_sum");
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

int sink_config::rtp_heart(int val_default)
{
	xtXmlNodePtr node = m_config.getNode(get_cfg(),"rtp_heart");
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

