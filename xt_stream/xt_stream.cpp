// xt_stream.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <stdlib.h>
#include <string>
#include "cmd_manager.h"
#include "h_xtmediaserver.h"

void main()
{
	std::string line;
	//////////////////////////////////////////////////////////////////////////
	MS_CFG cfg;
	::memset(&cfg, 0, sizeof(MS_CFG));
	cfg.num_chan = 1;
	::strcpy(cfg.ip, "0.0.0.0");
	cfg.snd_start_port = 50000;
	cfg.demux = 0;
	::strcpy(cfg.mul_start_ip, "239.0.0.0");
	cfg.msg_liten_port = 0;
	cfg.rtsp_listen_port = 1554;
	cfg.tcp_listen_port = 0;
	cfg.snd_std_rtp = 1;
	cfg.sink_single = 0;
	cfg.udp_listen_port = 0;
	cfg.sink_single = 0;
	int r = xt_init_server(cfg);
	for (int i=1;i<10&&r<0;++i)
	{
        cfg.snd_start_port = 50000+10*i;
        cfg.rtsp_listen_port = 1554+1000*i;
        r = xt_init_server(cfg);
		std::cerr <<line << "start fail"<< std::endl;
	}
	//////////////////////////////////////////////////////////////////////////

	while (true)
	{
		std::cout<<">>";
		if (!std::getline(std::cin, line))
		{
			break;
		}

		if (line.empty())
		{
			continue;
		}

		if (command_manager_t::instance()->parse_cmd(line))
		{  
			std::cerr << line <<"invalid command"<< std::endl;
		}
	}
}
