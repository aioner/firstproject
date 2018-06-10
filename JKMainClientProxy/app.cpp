#include "JkMainClientRpcServer.h"
#include "rpc/rpc_message_queue.h"
#include "rpc/rpc_server.h"
#include"config.h"
#include "JKMainClientLog.h"
#include "tools.h"

// add vesion code 
const char* XT_JK_LIB_INFO = "XT_Lib_Version: V_XT_JK_1.00.1118.0";

bool g_run = true;
rpc::message_queue::service_t g_service;
rpc::message_queue::server_channel_t g_channel(g_service);
rpc::server_t g_server;
JkMainClientRpcProxy *g_pProxy = NULL;
boost::thread* g_pThread = NULL;

void cmd_print(FILE *_File, char *text)
{
    using namespace boost::posix_time;

    ptime now = microsec_clock::local_time();
    std::string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());

    fprintf(_File, "%s %s", now_str.c_str(), text);
}

#ifdef _WIN32
void proxy_exit();
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
            proxy_exit();
            RetCode = TRUE;
            break;
        }
    default:
        break;
    }
    return RetCode;
}
#endif //#ifdef _WIN32

void proxy_exit()
{
    g_run = false;

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"work thread exit...");
    g_pThread->join();


    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"exit center......");
    g_pProxy->ExitCenter();

    //反初始化JK
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"uninit_jk ......");
    g_pProxy->uninit_jk();

    if (g_pProxy)
    {
        delete g_pProxy;
        g_pProxy = NULL;
    }

    if (g_pThread)
    {
        delete g_pThread;
        g_pThread = NULL;
    }
    std::cout<< get_cur_time_microsec()<<"JKMainClientProxy stop end!"<<std::endl;
    CJKMainClientLog::_()->un_log_sys();
}

void service_run_thread()
{
    while(g_run)
    {
        g_service.run_one();
        if (NULL == g_pProxy)
        {
            continue;
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(THREAD_CHECK_FREQUENCY));
    }
}

void loop_pro();
int init_jk();
int main(int argc,char* argv[])
{
#ifdef _WIN32
    ::SetConsoleCtrlHandler(&HandlerRoutine, TRUE);
#endif//#ifdef OS_WINDOWS

#ifndef _WIN32
    bool is_daemon = config::_()->get_daemon_config(false);
    if (is_daemon)
    {
        ::daemon(1,0);
    }
#endif // #ifndef _WIN32_

    //注册日志系统
    std::cout<<"regist_log_sys"<<std::endl;
    CJKMainClientLog::_()->regist_log_sys();
    do 
    {
        if (0 > init_jk())
        {
            break;
        }

        if (config::_()->auto_start_router(false))
        {
            if (tools_mgr::_()->is_exist_process(XTRouter_PROCESS_NAME))
            {
                if (!tools_mgr::_()->kill_process(XTRouter_PROCESS_NAME))
                {
                    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_error,"%s process is exist not kill!",XTRouter_PROCESS_NAME);
                    break;
                }
            }
            if (!tools_mgr::_()->start_process(XTRouter_PROCESS_NAME))
            {
                WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_error,"start %s fail!",XTRouter_PROCESS_NAME);
                break;
            }
        }

#ifdef _WIN32
        loop_pro();
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
            loop_pro();
        }
#endif // #ifdef _WIN32
    } while (false);//end do
    return 0;
}

int init_jk()
{
    int ret_code = -1;
    do 
    {
        WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"bind jkmainocxsvr msg queue size[%d]",MSG_QUEUE_SIZE);
        std::cout<<get_cur_time_microsec()<<" |bind jkmainocxsvr msg queue size:"<<MSG_QUEUE_SIZE<<std::endl;

        if (!g_channel.bind(JK_MSG_QUEUE_SERVER, 16, MSG_QUEUE_SIZE))
        {
            WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"bind jkmainocxsvr fail");
            break;
        }
        g_server.set_channel(&g_channel);

        if (NULL == g_pProxy)
        {
            g_pProxy = new JkMainClientRpcProxy;
            if (g_pProxy == NULL)
            {
                WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"new JkMainClientRpcProxy fail!");
                break;
            }

            WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info, "init jkclient start...");
            std::cout<<get_cur_time_microsec()<<" |init jkclient start..."<<std::endl;
            //初始化JK
            g_pProxy->init_jk();

            WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info, "init jkclient end...");
            std::cout<<get_cur_time_microsec()<<" |init jkclient end..."<<std::endl;
        }

        g_server.register_proxy(g_pProxy);

        WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"JKMainClientProxy run...");
        std::cout<<get_cur_time_microsec()<<" |JKMainClientProxy run..."<<std::endl;
        g_pThread = new boost::thread(&service_run_thread);
        if (NULL == g_pThread)
        {
            WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"create work thread fail...");
            std::cout<<"create work thread fail..."<<std::endl;
            break;
        }
        ret_code = 0;

        WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"JKMainClientProxy init finish.");
        std::cout<<get_cur_time_microsec()<<" |JKMainClientProxy init finish."<<std::endl;
    } while (false);
    return ret_code;
}

void loop_pro()
{
    while(true)
    {
        std::string cmd;
        std::cout<<">>";
        if (!std::getline(std::cin, cmd))
        {
            std::cout<<"std::getline fail!"<<std::endl;
            break;
        }

        if ("exit" == cmd)
        {
            proxy_exit();
            break;
        }
        else if ("cls" == cmd)
        {
            ::system("cls");
        }
        else if ("clear" == cmd)
        {
            ::system("clear");
        }
        else if ("rpc" == cmd)
        {
            g_pProxy->OnLinkServerJkEvent(1,1);
        }
        else if ("sip" == cmd)
        {
            g_pProxy->OnTransparentCommandJkEvent("songlei","jkproxy");
        }
        else
        {
            std::cout<<"command invalid!\n"<<std::endl;
        }
    }
}
