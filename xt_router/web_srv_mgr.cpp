#include "web_srv_mgr.h"
#ifdef _USE_WEB_SRV_
web_srv_mgr web_srv_mgr::self_;

#include "mml/cmd_manager.h"
int web_srv_mgr::wev_srv_cb(char* cmd, int cmdlength, void *handle)
{
    std::string result;
    command_manager_t::instance()->parse_cmd(cmd, result);
    ::xtxkWebSrvWrite(result.c_str(), result.length());
    return 0;
}
void web_srv_mgr::init(unsigned short port/*=8140*/)
{
    ::xtxkWebSrvRegistCallback(wev_srv_cb);
    ::xtxkWebSrvSetPort(port);
    ::xtxkWebSrvStart();
}
void web_srv_mgr::uninit()
{
    ::xtxkWebSrvStop();
}
void web_srv_mgr::write_log(const std::string& str)
{
    ::xtxkWebSrvPrintStr(str.c_str(),str.size());
}

#endif//_USE_WEB_SRV_