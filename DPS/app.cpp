//
//create by songlei 20160316
//
#include <iostream>
#include <string>
#include "dps_core_dispatch.h"
#include "dps_cmd_ctrl.h"

const char* XT_DPS_LIB_INFO = "XT_Lib_Version: V_XT_DPS_4.26.2016.032400";

void exit_pro()
{
    std::cout<<"exit_pro()..."<<std::endl;

    std::cout<<"dps_core_dispatch::_()->stop()..."<<std::endl;
    dps_core_dispatch::_()->stop();

    std::cout<<"framework::core::trem()..."<<std::endl;
    framework::core::trem();

    std::cout<<"dps_data_send_mgr::_()->uninit()..."<<std::endl;
    dps_data_send_mgr::_()->uninit();

    std::cout<<"dps_device_access_mgr::_()->term()..."<<std::endl;
    dps_device_access_mgr::_()->term();
}

#ifdef _WIN32
#include <Windows.h>
static BOOL WINAPI handler_routine(DWORD dw)
{
    BOOL ret_code = FALSE;
    switch (dw)
    {
    case CTRL_CLOSE_EVENT:
    case CTRL_C_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        {
            exit_pro();
            ret_code = TRUE;
            break;
        }
    default:
        break;
    }
    return ret_code;
}
#endif //#ifdef _WIN32

int main(int argc,char** argv)
{
    do
    {
        framework::core::start();
        std::cout<<"framework::core::start()..."<<std::endl;
        int ret_code = dps_core_dispatch::_()->start();
        if (ret_code < 0)
        {
            std::cout<<"dps_core_dispatch::_()->start() fail!"<<std::endl;
			break;
        }
#ifdef _WIN32
        ::SetConsoleCtrlHandler(&handler_routine, TRUE);
#endif//#ifdef _WIN32
        std::cout<<"DPS run..."<<std::endl;
        dps_cmd_ctrl::_()->run();
    } while (0);
    exit_pro();
    return 0;
}