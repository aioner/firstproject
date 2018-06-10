//
//create by songlei 20160316
//
#ifndef DPS_DEVICE_ACCESS_MGR_H__
#define DPS_DEVICE_ACCESS_MGR_H__
#include <vector>
#include <string>
#include <stdint.h>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include "MediaDevInterface.h"
typedef POUTPUTREALDATA access_data_output_cb_t;

//设备接入的设备操作句柄
typedef long dev_handle_t;
#define DEV_HANDLE_NA -1

typedef struct _device_link
{
    dev_handle_t link;
    std::string ids;
    long dev_chanid;
    long dev_strmtype;
    bool active;
    long strmid;
}device_link_t,*ptr_device_link_t;

class dps_device_access_mgr : boost::noncopyable
{
public:
    static dps_device_access_mgr* _(){return &my_;}
    static long init();
    static void term();

    // 开启点播
    long  start_capture(int device_type, char* url, long channel, int media_type, void* user_data,access_data_output_cb_t pfnDataCB, int port = 8000, char* szUser = "", char* szPassword = "",int link_type = 0);

    // 停止点播
    int stop_capture(dev_handle_t handle);

    //设置注册消息回调
    long set_regist_callback(regist_call_back_t func);

    //获取SDP
    long get_sdp_by_handle(dev_handle_t oper_handle,const char* sdp,long& length,long& data_type);
    long get_data_type_by_handle(dev_handle_t oper_handle);

    long rtcp_request_iframe(long link);
private:
    static dps_device_access_mgr my_;

};
#endif // #ifndef DPS_DEVICE_ACCESS_MGR_H__
