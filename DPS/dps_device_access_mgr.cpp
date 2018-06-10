//
//create by songlei 20160316
//
#include "dps_device_access_mgr.h"
#include <boost/thread/lock_guard.hpp>
#include <dps_cfg_mgr.h>
#include "dps_common_type_def.h"

dps_device_access_mgr dps_device_access_mgr::my_;
#ifdef _WIN32
#ifdef _DEBUG
# pragma comment(lib,"MediaDevice2.0_d.lib")
#pragma message("Auto Link ediaDevice2.0_d.lib")
#else
# pragma comment(lib,"MediaDevice2.0.lib")
#pragma message("Auto Link MediaDevice2.0.lib")
#endif // end #ifdef _DEBUG
#endif // end #ifdef _WIN32

long dps_device_access_mgr::init()
{
    long ret_code = -1;
    do 
    {
        ret_code = ::StartMediadevice();;
        if (ret_code < 0) break;

        xt_client_cfg_t cfg;
        std::string recv_ip = dps_cfg_mgr::_()->local_bind_ip("0.0.0.0");
        std::strncpy(cfg.udp_session_bind_ip,recv_ip.c_str(),DPS_MAX_IP_LEN);
        cfg.udp_session_bind_port = dps_cfg_mgr::_()->udp_session_bind_port(19001);
        cfg.udp_session_heartbit_proid = dps_cfg_mgr::_()->udp_session_heartbit_proid(20);
        cfg.udp_session_request_try_count = dps_cfg_mgr::_()->udp_session_request_try_count(4);
        cfg.udp_session_request_one_timeout = dps_cfg_mgr::_()->udp_session_request_one_timeout(5000);
        std::strncpy(cfg.tcp_session_bind_ip,recv_ip.c_str(),DPS_MAX_IP_LEN);
        cfg.tcp_session_bind_port = dps_cfg_mgr::_()->tcp_session_bind_port(0);
        cfg.tcp_session_connect_timeout = dps_cfg_mgr::_()->tcp_session_connect_timeout(3000);
        cfg.tcp_session_login_timeout = dps_cfg_mgr::_()->tcp_session_login_timeout(3000);
        cfg.tcp_session_play_timeout = dps_cfg_mgr::_()->tcp_session_play_timeout(2000);
        cfg.tcp_session_stop_timeout = dps_cfg_mgr::_()->tcp_session_stop_timeout(2000);
        cfg.rtsp_session_connect_timeout = dps_cfg_mgr::_()->rtsp_session_connect_timeout(3000);
        cfg.rtsp_session_describe_timeout = dps_cfg_mgr::_()->rtsp_session_describe_timeout(3000);
        cfg.rtsp_session_setup_timeout = dps_cfg_mgr::_()->rtsp_session_setup_timeout(3000);
        cfg.rtsp_session_play_timeout = dps_cfg_mgr::_()->rtsp_session_play_timeout(2000);
        cfg.rtsp_session_pause_timeout = dps_cfg_mgr::_()->rtsp_session_pause_timeout(2000);
        cfg.rtsp_session_teardown_timeout = dps_cfg_mgr::_()->rtsp_session_teardown_timeout(1000);
        long start_port = dps_cfg_mgr::_()->rtp_recv_start_port(16000);
        long portNum = dps_cfg_mgr::_()->rtp_recv_port_num(1000);

        ret_code = ::InitializeDeviceEx(-1,cfg, start_port, portNum);
        if (ret_code < 0) break;

        ret_code = 0;
    } while (0);
    return ret_code;
}

void dps_device_access_mgr::term()
{
    ::UnInitializeDevice(-1,NULL);
    ::EndMediadevice();
}

long  dps_device_access_mgr::start_capture(int device_type, char* url, long channel, int media_type, void* user_data, access_data_output_cb_t pfnDataCB, int port, char* szUser, char* szPassword,int link_type)
{
    return ::StartDeviceCapture(url, port, device_type, channel, user_data, pfnDataCB, szUser, szPassword, link_type, media_type, 0);
}

int dps_device_access_mgr::stop_capture(dev_handle_t handle)
{
    if (handle < 0) return 0;
    return ::StopDeviceCapture(handle);
}

long dps_device_access_mgr::get_data_type_by_handle(dev_handle_t oper_handle)
{
    if (oper_handle < 0) return -1;
    return ::GetDataType(oper_handle);
}
long dps_device_access_mgr::get_sdp_by_handle(dev_handle_t oper_handle,const char* sdp,long& length,long& data_type)
{
    long ret_code = -1;
    do
    {
        if (oper_handle < 0) break;
        ret_code = ::GetSDP(oper_handle,(uint8_t*)sdp,(long&)length);
        if (ret_code < 0 || length <= 0)
        {
            ret_code = -2;
            break;
        }
        data_type = ::GetDataType(oper_handle);
        if (data_type < 0)
        {
            ret_code = -3;
            break;
        }
        ret_code = 1;
    }while(0);
    return ret_code;
}

long dps_device_access_mgr::rtcp_request_iframe(long link)
{
    return ::md_request_iframe(link);
}

long dps_device_access_mgr::set_regist_callback(regist_call_back_t func)
{
    ::md_set_regist_callback(9,func);
    return 0;
}
