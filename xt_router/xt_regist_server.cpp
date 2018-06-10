
#include "xt_regist_server.h"
#include "media_device.h"

xt_regist_server xt_regist_server::m_regist_server;

bool xt_regist_server::init()
{
    media_device::instance()->set_regist_callback(xt_regist_calllback);
    return true;
}

bool xt_regist_server::add_regist_client( const char *ip,unsigned port ,const uint8_t *data,int len )
{
    xt_regist_client_t regist_client_;
    regist_client_.port = port;
    strcpy(regist_client_.ip,ip);
    regist_client_.regist = true;
    memcpy(regist_client_.ids,data,len);

	// 同一个ids设备ip和port变化后重新注册更新地址
	std::vector<xt_regist_client_t>::iterator it = m_regist_client.begin();
	for (;it != m_regist_client.end();it++)
	{
		if (!strcmp(it->ids,regist_client_.ids))
		{
			strcpy((*it).ip, regist_client_.ip);
			(*it).port = regist_client_.port;

			return true;
		}
	}
    m_regist_client.push_back(regist_client_);
    return true;
}

bool xt_regist_server::del_regist_client( const char *ip,unsigned port )
{
    std::vector<xt_regist_client_t>::iterator it = m_regist_client.begin();
    for (;it != m_regist_client.end();it++)
    {
        if (strcmp(it->ip,ip) && it->port == port )
        {
            m_regist_client.erase(it);
            return true;
        }
    }
    return false;
}

bool xt_regist_server::get_regist_client_by_ids( const char*ids,xt_regist_client_t *client )
{
    std::vector<xt_regist_client_t>::iterator it = m_regist_client.begin();
    for (;it != m_regist_client.end();it++)
    {
        if (!strcmp(it->ids,ids) )
        {
            strcpy(client->ip,(*it).ip);
            client->port = (*it).port;
            strcpy(client->ids,(*it).ids);
             return true;
        }
    }
    return false;

}

bool xt_regist_server::get_regist_client_by_ip(const char *ip,xt_regist_client_t *client )
{
    std::vector<xt_regist_client_t>::iterator it = m_regist_client.begin();
    for (;it != m_regist_client.end();it++)
    {
        if (!strcmp(it->ip,ip) )
        {
            strcpy(client->ip,ip);
            client->port = (*it).port;
            strcpy(client->ids,(*it).ids);
            return true;
        }
    }
    return false;
}
bool xt_regist_server::set_regist_client_by_ids( const char*ids,char *ip,unsigned port )
{
    std::vector<xt_regist_client_t>::iterator it = m_regist_client.begin();
    for (;it != m_regist_client.end();it++)
    {
        if (!strcmp(it->ids,ids) )
        {
            strcpy((*it).ip,ip);
           (*it).port  = port;
            strcpy((*it).ids,ids);
            break;
        }
    }

	return true;
}

bool xt_regist_server::set_regist_client_by_ip( const char*ids,char *ip,unsigned port )
{
    std::vector<xt_regist_client_t>::iterator it = m_regist_client.begin();
    for (;it != m_regist_client.end();it++)
    {
        if (!strcmp(it->ip,ip) )
        {
            strcpy((*it).ids,ids);
            (*it).port  = port;
            strcpy((*it).ip,ids);
            break;
        }
    }
    return true;
}

long xt_regist_server::xt_regist_calllback(const char *ip, uint16_t port, const uint8_t *data, uint32_t length)
{
     instance()->add_regist_client(ip,port,data,length);
     return 0;
}

