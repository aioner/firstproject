#ifndef WEB_SRV_MGR_H__
#define WEB_SRV_MGR_H__

#ifdef _USE_WEB_SRV_

#ifdef _WIN32
# pragma comment(lib,"xtxkWebSrv.lib")
#else
#endif //#ifdef _WIN32
#include <boost/noncopyable.hpp>
#include <string>
#include "xtxkWebSrv.h"

class web_srv_mgr : boost::noncopyable
{
public:
    static web_srv_mgr* _(){return &self_;}
    void init(unsigned short port=8140);
    void uninit();
    void write_log(const std::string& str);
private:
    static int XT_WEB_SERVER_STDCALL  wev_srv_cb(char* cmd, int cmdlength, void *handle);
    static web_srv_mgr self_;
};

#endif //#  ifdef _USE_WEB_SRV_
#endif //#ifndef WEB_SRV_MGR_H__