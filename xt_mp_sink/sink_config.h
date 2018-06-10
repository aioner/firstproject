#ifndef _XT_ROUTER_CONFIG_H_INCLUDED
#define _XT_ROUTER_CONFIG_H_INCLUDED

#include "utility/noncopyable.h"
#include <string>
#include "xtXml.h"

class sink_config:private utility::noncopyable
{
private:
	sink_config(void);
	~sink_config(void);

public:
	static sink_config* inst()
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

	xtXmlNodePtr get_cfg();
	
	int log_level(int val_default);
	int frame_cache(int val_default);
	int pkt_cache(int val_default);
	int resend(int val_default);
	int tm_readd_rmt(int val_default);
	int wait_resend(int val_default);
	int max_resend(int val_default);
	int vga_order(int val_default);
	int jarless_packets(int val_default);
	int sync_packets(int val_default);
	int wait_frames(int val_default);
	int check_sum(int val_default);
	int lost(int val_default);
	int rtp_heart(int val_default);

private:
	// xml
	xtXml m_config;

	// ÎÄ¼þÂ·¾¶
	std::string m_fPath;

	bool m_valid;

	static sink_config self;
};

#endif //_XT_ROUTER_CONFIG_H_INCLUDED
