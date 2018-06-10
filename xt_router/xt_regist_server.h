#ifndef XT_REGIST_SERVER_H_INCLUDE__
#define XT_REGIST_SERVER_H_INCLUDE__
#include <string>
#include <vector>
#include <stdint.h>
#include <boost/thread.hpp>

#include "h_xtmediaserver.h"

class xt_regist_server
{
public:
    struct  xt_regist_client_t
    {
        char	ids[MAX_STR_SIZE];			//IDS
        unsigned	usr_data;		//用户数据
        bool		regist;			//是否注册
        char ip[MAX_STR_SIZE];             //IP
        unsigned  port;
    };
public:
    xt_regist_server()
    {
    };
    
    ~xt_regist_server()
    {

    };

public:
    static xt_regist_server* instance(){return &m_regist_server;}
    bool init();
    bool uinit();

    bool add_regist_client( const char *ip,unsigned port ,const uint8_t *data,int len);
    bool del_regist_client(const char *ip,unsigned port );
    bool get_regist_client_by_ids(const char*ids,xt_regist_client_t *client);
    bool get_regist_client_by_ip(const char *ip,xt_regist_client_t *client);
    bool set_regist_client_by_ids(const char*ids,char *ip,unsigned port);
    bool set_regist_client_by_ip(const char*ids,char *ip,unsigned port);

private:
    static xt_regist_server m_regist_server;
   
    static long  MEDIASERVER_STDCALL xt_regist_calllback(const char *ip, uint16_t port, const uint8_t *data, uint32_t length);

    std::vector<xt_regist_client_t> m_regist_client;


};

#endif//XT_REGIST_SERVER_H_INCLUDE__