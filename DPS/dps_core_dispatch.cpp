//
//create by songlei 20160316
//
#include "dps_core_dispatch.h"
#include "dps_cfg_mgr.h"
#include "dps_task_pro.h"
#include "dps_stream_monitor.h"
#include "dps_jk_dispatch_mgr.h"
#include "dps_PTZ_ctrl_mgr.h"

dps_core_dispatch dps_core_dispatch::my_;

long dps_core_dispatch::data_out_cb(long handle, unsigned char* data, long len, long frame_type,long data_type,void* user_data, long time_stamp,unsigned long nSSRC)
{
    do 
    {
        dps_dev_s_handle_t s_handle = static_cast<dps_dev_stream_t*>(user_data);
        if (DPS_DEV_S_HANDLE_NA == s_handle || NULL == data || DPS_MAX_FRAME_SIZE < len) break;
        s_handle->update_recv_frame_nums();
        s_handle->recv_data_refurbish();
        if (s_handle->is_open())
        {
            s_handle->send_media_data(data,len,frame_type,data_type,time_stamp,nSSRC);
        }
    } while (0);
    return 0;
}

int dps_core_dispatch::send_media_data_cb(int srcno, int trackid, unsigned char *buff, unsigned long len, int frame_type, long device_type, uint32_t in_time_stamp)
{
    return dps_data_send_mgr::_()->send_data_stamp(srcno,trackid,(char*)buff,len,frame_type,device_type,in_time_stamp);
}

void dps_core_dispatch::play_asyn_cb(const dps_dev_s_handle_t s_handle)
{
    dev_play_task *ptr_task = new dev_play_task(s_handle,data_out_cb,send_media_data_cb);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

void dps_core_dispatch::play_asyn(const dps_dev_s_handle_t s_handle)
{
    play_asyn_cb(s_handle);
}

void dps_core_dispatch::stop_asyn_cb(const dps_dev_s_handle_t s_handle)
{
    dev_stop_task* ptr_task = new dev_stop_task(s_handle);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

void dps_core_dispatch::stop_asyn(const dps_dev_s_handle_t s_handle)
{
    stop_asyn_cb(s_handle);
}

void dps_core_dispatch::stop_syn_cb(const dps_dev_s_handle_t s_handle)
{
    s_handle->stop_capture();
    s_handle->clear_src_info();
}

int dps_core_dispatch::start()
{
    int ret_code = -1;
    do 
    {
       std::cout<<"dps_cfg_mgr::_()->loading()..."<<std::endl;
       ret_code = dps_cfg_mgr::_()->loading();
       if (ret_code < 0)
       {
           //write log fail....
		   if (ret_code == -1)
		   {
			   printf("Error: not find the dps configuration file!\nProgram will exit......");
			   getchar();
			   break;
		   }
		   else
		   {
			   printf("Warnning: not find the systemset file, so can not link the XT center!\n");
		   }
       }

       std::cout<<"dps_replay_mgr::_()->init()..."<<std::endl;
       dps_replay_mgr::_()->init();

       std::cout<<"dps_replay_mgr::_()->init()..."<<std::endl;
       dps_break_monitor_mgr::_()->init();

       std::cout<<"dps_data_send_mgr::_()->start()..."<<std::endl;
       ret_code = dps_data_send_mgr::_()->start();
       if (ret_code < 0)
       {
           //writer log fail...
		   printf("Error: dps_data_send_mgr::_()->start() failed!\nProgram will exit......");
		   getchar();
           break;
       }

       std::cout<<"dps_device_access_mgr::_()->init()..."<<std::endl;
       long ret = dps_device_access_mgr::_()->init();
       if (ret < 0)
       {
           // write log fail ....
		   printf("Error: dps_device_access_mgr::_()->init() failed!\nProgram will exit......");
		   getchar();
           break;
       }

       std::cout<<"dps_ch_mgr::_()->init()..."<<std::endl;
       ret_code = dps_ch_mgr::_()->init();
       if (ret_code < 0)
       {
           // write log fial....
		   printf("Error: dps_ch_mgr::_()->init() failed!\nProgram will exit......");
		   getchar();
           break;
       }

       std::cout<<"dps_ch_mgr::_()->play_all_ch(play_asyn_cb)..."<<std::endl;
       dps_ch_mgr::_()->play_all_ch(play_asyn_cb);

       std::cout<<"dps_jk_dispatch_mgr::_()->start()..."<<std::endl;
       dps_jk_dispatch_mgr::_()->start();

       std::cout<<"dps_PTZ_ctrl_mgr::_()->init()..."<<std::endl;
       dps_PTZ_ctrl_mgr::_()->init();

       ret_code = 1;
    } while (0);
    return ret_code;
}
int dps_core_dispatch::stop()
{
    int ret_code = -1;
    do 
    {
        dps_jk_dispatch_mgr::_()->stop();
        dps_PTZ_ctrl_mgr::_()->uninit();
        dps_replay_mgr::_()->uninit();
        dps_break_monitor_mgr::_()->uninit();
        dps_ch_mgr::_()->stop_all_ch(stop_syn_cb);
        dps_ch_mgr::_()->uninit();
        dps_cfg_mgr::_()->unload();
        ret_code = 1;
    } while (0);
    return ret_code;
}
