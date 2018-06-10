//
//create by songlei 20160316
//
#ifndef DPS_CORE_DISPATCH_H__
#define DPS_CORE_DISPATCH_H__
#include "dps_device_access_mgr.h"
#include "dps_ch_mgr.h"
#include "dps_data_send_mgr.h"

class dps_core_dispatch : boost::noncopyable
{
public:
    static dps_core_dispatch* _(){return &my_;}
private:
    //数据抛出回调
    static long __stdcall data_out_cb(long handle, unsigned char* data, long len, long frame_type,long data_type,void* user_data, long time_stamp,unsigned long nSSRC);
    static int __stdcall send_media_data_cb(int srcno, int trackid, unsigned char *buff, unsigned long len, int frame_type, long device_type, uint32_t in_time_stamp);

    //异步
    static void __stdcall play_asyn_cb(const dps_dev_s_handle_t s_handle);

    //异步
    static void __stdcall stop_asyn_cb(const dps_dev_s_handle_t s_handle);

    //同步
    static void __stdcall stop_syn_cb(const dps_dev_s_handle_t s_handle);

public:
    void play_asyn(const dps_dev_s_handle_t s_handle);
    void stop_asyn(const dps_dev_s_handle_t s_handle);
public:
    int start();
    int stop();
private:
    static dps_core_dispatch my_;
};

#endif // #ifndef DPS_CORE_DISPATCH_H__