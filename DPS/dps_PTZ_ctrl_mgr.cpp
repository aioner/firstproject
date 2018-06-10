//
//create by songlei 20160316
//
#include "dps_PTZ_ctrl_mgr.h"
#include "dps_ch_mgr.h"
#include "dps_cfg_mgr.h"

#ifdef _USE_PTZ_FUN_
#include "EDvrCtrlAttach.h"
#ifdef _WIN32
#ifdef _DEBUG
# pragma comment(lib,"EDvrCtrlAttach_d.lib")
#pragma message("Auto Link EDvrCtrlAttach_d.lib")
#else
# pragma comment(lib,"EDvrCtrlAttach.lib")
#pragma message("Auto Link EDvrCtrlAttach.lib")
#endif // end #ifdef _DEBUG
#endif // end #ifdef _WIN32
#endif // end #ifdef _USE_PTZ_FUN_

dps_PTZ_ctrl_mgr dps_PTZ_ctrl_mgr::my_;

int dps_PTZ_ctrl_mgr::init()
{
#ifdef _USE_PTZ_FUN_
    if (LINK_XTCENTER == dps_cfg_mgr::_()->link_center(LINK_ON))
    {
        ::Ctrl_Startup(0, NULL);
        ::Ctrl_ReflashDeviceList("", "", 0, State_ReflashStart);
        dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
        dps_ch_mgr::_()->get_all_s_handle(s_hndles);
        dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
        for (;s_hndles.end() != itr;++itr)
        {
            dps_dev_s_handle_t s_handle = *itr;
            ::Ctrl_ReflashDeviceList(s_handle->get_device().ip,s_handle->get_device().ip,s_handle->get_device().dev_type,State_ReflastDoing);
            ::Ctrl_ClientJoinV2(s_handle->get_device().ip,s_handle->get_device().ip,s_handle->get_device().dev_type,0,s_handle->get_device().port,s_handle->get_device().usr,s_handle->get_device().password);
        }
        ::Ctrl_ReflashDeviceList("", "", 0, State_ReflastFinish);
    }
#endif //#ifdef _USE_PTZ_FUN_
    return 0;

}

void dps_PTZ_ctrl_mgr::uninit()
{
#ifdef _USE_PTZ_FUN_
    ::Ctrl_Cleanup();
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_clientrecord( const char* szIDS, int nChannel, int bEnable )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_ClientRecord(szIDS, nChannel, bEnable);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_clientalarm( const char* szIDS, int nChannel, int bEnable )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_ClientAlarm(szIDS, nChannel, bEnable);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_clientmotion( const char* szIDS, int nChannel, int bEnable )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_ClientMotion(szIDS, nChannel, bEnable);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_clientimage( const char* szIDS, int nChannel, int eCtrlEffect, int nValue )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_ClientImage(szIDS, nChannel, eCtrlEffect, nValue);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_captureiframe( const char* szIDS, int nChannel )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_CaptureIFrame(szIDS, nChannel);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_setcameraname( const char* szIDS, int nChannel, const char* szName )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_SetCameraName(szIDS, nChannel, szName);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_clientptz( const char* szIDS, int nChannel, int ePtzAct )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_ClientPtz(szIDS, nChannel, ePtzAct);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_clientptzspeed( const char* szIDS, int nChannel, int ePtzAct, int nSpeed )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_ClientPtzSpeed(szIDS, nChannel, ePtzAct, nSpeed);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_clientptzadvance( const char* szIDS, int nChannel, const char* szAdvCmd, int nCmdSize )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_ClientPtzAdvance(szIDS, nChannel, (char*)szAdvCmd, nCmdSize);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_clientptzpreset( const char* szIDS, int nChannel, int eCtrlPresetAct, int nPreset )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_ClientPtzPreset(szIDS, nChannel, eCtrlPresetAct, nPreset);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}

int dps_PTZ_ctrl_mgr::ctrl_setdatetime( const char* szIDS, int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec )
{
#ifdef _USE_PTZ_FUN_
    return ::Ctrl_SetDateTime(szIDS, nYear, nMonth, nDay, nHour, nMin, nSec);
#else
    return 0;
#endif //#ifdef _USE_PTZ_FUN_
}


