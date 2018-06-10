#pragma once
#include "utility/noncopyable.h"
#include "tools/xtXml.h"
#include <string>
#include <map>
#include <wtypes.h>

using namespace std;

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

	typedef pair<string, string> mutex_type;

	xtXmlNodePtr get_router();
	xtXmlNodePtr get_system();

	int break_monitor(int val_default);
	int break_monitor_time(int val_default);
	int check_len(int val_default);
	int check_timeout(int val_default);
	int link_center(int val_default);
	string xt_dbtype(string val_default);
	int copy_send(int val_default);
	int link_type(int val_default);
	int chan_num(int val_default);
	string local_sndip(string val_default);
	int snd_port(int val_default);
	int demux_s(int val_default);
	string mul_ip(string val_default);
	int tcp_listenprt(int val_default);
	int xtmsg_listenprt(int val_default);
	int rtsp_listenprt(int val_default);

	string center_ip(string val_default);
	string local_ip(string val_default);
	string local_name(string val_default);
	int center_port(int val_default);
	int center_id(int val_default);

private:
	// xml
	xtXml m_config;

	// 文件路径
    std::string m_fPath;

	// 命名内核
	map<string, string> m_mapMutex;

	HANDLE m_hMutex;

    static config self;
};
