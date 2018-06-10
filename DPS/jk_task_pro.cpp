//
//create by songlei 20160316
//
#include "jk_task_pro.h"
#include "dps_ch_mgr.h"
#include "dps_jk_dispatch_mgr.h"
#include "dps_PTZ_ctrl_mgr.h"

uint32_t on_tell_local_ids_task::run()
{
    dps_jk_dispatch_mgr::_()->set_local_ids(local_ids_);
    return 0;
}

uint32_t on_link_server_task::run()
{
    //此处增加实现代码
    return 0;
}

uint32_t on_user_in_out_task::run()
{
    //此处增加实现代码
    return 0;
}


uint32_t on_event_get_msg_task::run()
{
    //此处增加实现代码
    return 0;
}

uint32_t on_dbimage_center_task::run()
{
    //此处增加实现代码
    return 0;
}

//处理中心透传指令
uint32_t on_transparent_command_cb_task::run()
{
    //此处增加实现代码 获取对应通信息
    return 0;
}

uint32_t on_client_record_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(sch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        dps_PTZ_ctrl_mgr::_()->ctrl_clientrecord(s_handle->get_device().ip,0,bz_);
    }
    return 0;
}

uint32_t on_client_alarm_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(sch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        dps_PTZ_ctrl_mgr::_()->ctrl_clientalarm(s_handle->get_device().ip,0,bz_);
    }
    return 0;
}

uint32_t on_client_motion_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(sch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        dps_PTZ_ctrl_mgr::_()->ctrl_clientmotion(s_handle->get_device().ip,0,bz_);
    }
    return 0;
}

uint32_t on_client_image_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(sch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        dps_PTZ_ctrl_mgr::_()->ctrl_clientimage(s_handle->get_device().ip,0,bz_,value_);
    }
    return 0;
}

uint32_t on_ask_angelcamerazt_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(dch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        //未实现
    }
    return 0;
}

uint32_t on_client_capture_iframe_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(sch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        dps_PTZ_ctrl_mgr::_()->ctrl_captureiframe(s_handle->get_device().ip,0);
    }
    return 0;
}

uint32_t on_client_osd_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(sch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        if (bz_ == 0)
        {
            dps_PTZ_ctrl_mgr::_()->ctrl_setcameraname(s_handle->get_device().ip, 0, "");
        }
        else if (bz_ == 1)
        {
            dps_PTZ_ctrl_mgr::_()->ctrl_setcameraname(s_handle->get_device().ip, 0, osd_name_.c_str());
        } 
    }
    return 0;
}

uint32_t on_client_yt_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(sch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        dps_PTZ_ctrl_mgr::_()->ctrl_clientptz(s_handle->get_device().ip,0,op_);
    }
    return 0;
}

uint32_t on_client_ytex_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(dch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        if (op_ == 0x1000)
        {
            dps_PTZ_ctrl_mgr::_()->ctrl_clientptzadvance(s_handle->get_device().ip, 0, sres1_.c_str(), sres1_.length()+1);
        }
        else
        {
            long speed = (val_+1)*32 - 1;
            dps_PTZ_ctrl_mgr::_()->ctrl_clientptzspeed(s_handle->get_device().ip, 0, op_, speed);
        }
    }
    return 0;
}

uint32_t on_client_select_point_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(sch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        dps_PTZ_ctrl_mgr::_()->ctrl_clientptzpreset(s_handle->get_device().ip,0,3,cnum_);
    }
    return 0;
}

uint32_t on_client_set_point_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_s_handle_by_ch(sch_,s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    if(s_hndles.end() != itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        char num[9];
        num[8] = 0;
        memcpy(num, pname_.c_str(), 8);

        //if (pname_.length() > 8)
        //	pname_ += 8;
        //else
        //	pname_ = "";

        int param = atoi(num);
        int funcNo = param & 0xFFFF;
        //ADD_PRESET_POINT      = 0;  //增加预置点    1
        //DEL_PRESET_POINT      = 1;  //删除预置点    0
        //RESET_PRESET_POINT    = 2;  //修改预置点    2
        //SELECT_PRESET_POINT   = 3;
        if (funcNo == 0)
        {
            dps_PTZ_ctrl_mgr::_()->ctrl_clientptzpreset(s_handle->get_device().ip, 0, 1, cnum_);
        }
        else if (funcNo == 1)
        {
            dps_PTZ_ctrl_mgr::_()->ctrl_clientptzpreset(s_handle->get_device().ip, 0, 0, cnum_);
        }
        else if (funcNo == 2)
        {
            dps_PTZ_ctrl_mgr::_()->ctrl_clientptzpreset(s_handle->get_device().ip, 0, 2, cnum_);
        }
    }
    return 0;
}

uint32_t on_transparent_data_task::run()
{
    delete this;
    return 0;
}

uint32_t on_group_device_state_change_task::run()
{
    return 0;
}

uint32_t on_check_time_task::run()
{
    dps_ch_t transmit_ch = 0;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hndles;
    dps_ch_mgr::_()->get_all_s_handle(s_hndles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hndles.begin();
    for (;s_hndles.end() != itr; itr++)
    {
        dps_dev_s_handle_t s_handle = *itr;
        s_handle->get_device().dev_type;
        s_handle->get_device().dev_ch;
        s_handle->get_device().ip;
        s_handle->get_device().port;
        s_handle->get_device().usr;
        s_handle->get_device().password;
        dps_PTZ_ctrl_mgr::_()->ctrl_setdatetime(s_handle->get_device().ip, iyear_, imonth_, idate_, ihour_, imin_, isec_);
    }
    return 0;
}
