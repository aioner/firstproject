#ifndef _ICMP_PING__
#define _ICMP_PING__

//ERR_CODE
#define ICMP_PING_ERR_INIT_FLIED -1 //初化始Ping失败

#define MAX_STR 64
#include <stdint.h>
#include <string.h>

#define NET_STATE_ON_LINE  1 //在线
#define NET_STATE_OFF_LINE 0 //断线
#define NET_STATE_NA  -1 //无效

#define MTU_SIZE 1472
typedef struct _struct_net_state_ret
{
	char src_ip[MAX_STR];
	double lost;
	uint64_t rtt; //mics
	int net_state;//0:断线 1：在线 -1:无效
	long recv_packs;
	long send_packs;

	_struct_net_state_ret()
	{
		memset(src_ip,0,MAX_STR);
		lost= 0.0;
		rtt= 0;
		net_state = NET_STATE_NA;
		recv_packs = 0;
		send_packs = 0;
	}

	_struct_net_state_ret& operator=(const _struct_net_state_ret& other)
	{	
		if (this != &other)
		{
			memcpy(this->src_ip, other.src_ip, MAX_STR);
			this->lost = other.lost;
			this->net_state = other.net_state;
			this->recv_packs = other.recv_packs;
			this->send_packs = other.send_packs;
		}

		return *this;
	}
} net_state_type,*pnet_state_type;

 int ping(net_state_type& ping_ret,const char* destination,const long out_time_seconds,const long pack_num);

#endif //_ICMP_PING__