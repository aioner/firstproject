#ifndef XT_REGIST_CLIENT_H__
#define XT_REGIST_CLIENT_H__
#include <string>
#include<stdint.h>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>

#ifndef CLOSE_SESSION
#include "h_xtsessionserver.h"
class xt_regist_client
{
public:
    static xt_regist_client* instance()
    {
        return &m_regist_client;
    };
    int regist(const char* regist_ids,const char* server_ip, unsigned short server_port,uint32_t millisec);

    int stop_regist(const char* server_ip, unsigned short server_port ,uint32_t millisec);
    static xt_regist_client m_regist_client;
    void  set_regist_response_callback_t(regist_response_callback_t func);
private:
	xt_regist_client();
private:
	static void xt_regist_work_thread(void *param);
	
	struct regist_info
	{
		std::string m_regist_ids;
		std::string m_server_ip;
		unsigned short m_server_port;
		uint32_t m_millisec;

		
	};
	boost::thread *xt_regist_thread;
	bool m_brun;
    static regist_info rinfo;
};
#endif

#endif//XT_REGIST_SERVER_H