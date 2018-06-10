#include <iostream>
#include "XTRouter.h"
#include "Router_config.h"
#include "media_server.h"
#include "mml/cmd_manager.h"
#include "XTRouterLog.h"

// add vesion code 
const char* XT_ROUTER_LIB_INFO = "XT_Lib_Version: V_XT_Router_4.26.2016.050300";

void pro_exit()
{
    long ret;
    //退出时停止交换
    ret = CXTRouter::Instance()->StopXTRouter();
    if (ret < 0)
    {
        std::cout << "StopXTRouter fail" << std::endl;
        DEBUG_LOG(DBLOGINFO,ll_error,"StopXTRouter fail");
    }
    std::cout<<"stop framework ..."<<std::endl;
    void framework_services_destruct();
    framework_services_destruct();
    std::cout<<"stop end!"<<std::endl;
}
#ifdef _WIN32
static BOOL WINAPI HandlerRoutine(DWORD dw)
{
    BOOL RetCode = FALSE;
    switch (dw)
    {
    case CTRL_CLOSE_EVENT:
    case CTRL_C_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        {
            pro_exit();
            RetCode = TRUE;
            break;
        }
    default:
        break;
    }
    return RetCode;
}
#endif //#ifdef _WIN32

extern int main_real();
int main(int argc,char** argv)
{
#ifdef _WIN32
    ::SetConsoleCtrlHandler(&HandlerRoutine, TRUE);
#endif//#ifdef _WIN32
    CXTRouter::Instance()->regist_log_sys();
    return main_real();
}
void router_loop()
{
    bool exit_flag = false;
    std::string line;
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

        std::string result;
        switch (command_manager_t::instance()->parse_cmd(line,result))
        {
        case command_manager_t::cmd_not_exists:
            std::cerr << line << " unregistered command"<< std::endl;
            break;
        case command_manager_t::cmd_format_error:
            std::cerr << line << "command format invalid"<< std::endl;
            break;
        case command_manager_t::cmd_arguments_error:
            std::cerr << line << " command param invalid"<< std::endl;
            break;
        case command_manager_t::ok:
            break;
        case command_manager_t::exec_will_exit:
        default:
            exit_flag = true;
            break;
        }
        if (exit_flag)
        {
            break;
        }
    }
}

int main_real()
{
    bool is_daemon = config::instance()->get_daemon_config(0);

#ifndef _WIN32 
    if (is_daemon) 
    {
        daemon(1,0);
    }
#endif //_WIN32
    
   void framework_services_construct();
   framework_services_construct();

    std::cout<<"server start..."<<std::endl;
    std::string load_config_ret = config::instance()->valid() ? "success!" : "fail!";
    std::cout<< "config file [" << config::instance()->get_config_path() << "] load " << load_config_ret << std::endl;
    std::cout<<"local build send ip:"<<config::instance()->local_sndip("0.0.0.0")<<std::endl;
    std::cout<<"local build ip:"<<config::instance()->local_ip("0.0.0.0")<<std::endl;
    load_config_ret = config::instance()->valid_systemset_xml() ? "success!" : "fail!";
    std::cout<< "config file [" << config::instance()->get_systemset_path() << "] load " << load_config_ret << std::endl;

    int ret = CXTRouter::Instance()->StartXTRouter();
    if (ret < 0)
    {
        std::cout << "server start fail" << std::endl;
    }
    else
    {
        std::cout<<"server run..."<<std::endl;
#ifdef _WIN32  
            router_loop();
#else
        if (is_daemon)
        {
            while(true)
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
            }
        }
        else
        {
             router_loop();
        }
        
#endif//_WIN32
    }
    //退出时停止交换
    pro_exit();
    return 0;
}
