//
//create by songlei 20160316
//
#ifndef DPS_PTZ_CTRL_MGR_H__
#define DPS_PTZ_CTRL_MGR_H__
#include <boost/noncopyable.hpp>

#define State_ReflashStart 0
#define State_ReflastDoing 1
#define State_ReflastFinish 2

class dps_PTZ_ctrl_mgr : boost::noncopyable
{
public:
    static dps_PTZ_ctrl_mgr* _(){return &my_;}

private:
    static dps_PTZ_ctrl_mgr my_;

public:
    int init();
    void uninit();
    int ctrl_clientrecord(const char* szIDS, int nChannel, int bEnable);
    int ctrl_clientalarm(const char* szIDS, int nChannel, int bEnable);
    int ctrl_clientmotion(const char* szIDS, int nChannel, int bEnable);
    int ctrl_clientimage(const char* szIDS, int nChannel, int eCtrlEffect, int nValue);
    int ctrl_captureiframe(const char* szIDS, int nChannel);
    int ctrl_setcameraname(const char* szIDS, int nChannel, const char* szName);
    int ctrl_clientptz(const char* szIDS, int nChannel, int ePtzAct);
    int ctrl_clientptzspeed(const char* szIDS, int nChannel, int ePtzAct, int nSpeed);
    int ctrl_clientptzadvance(const char* szIDS, int nChannel, const char* szAdvCmd, int nCmdSize);
    int ctrl_clientptzpreset(const char* szIDS, int nChannel, int eCtrlPresetAct, int nPreset);
    int ctrl_setdatetime(const char* szIDS, int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec);
};
#endif // end #ifndef DPS_PTZ_CTRL_MGR_H__
