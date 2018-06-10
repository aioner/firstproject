//
//create by songlei 20160316
//

#include "dps_task_pro.h"
#include "dps_stream_monitor.h"

#define PLAY_TRY_TIME 10
#define PLAY_TRY_COUNT 300

const unsigned char g_sdp_hc[] = {0x34, 0x48, 0x4b, 0x48, 0xfe, 0xb3, 0xd0, 0xd6, 0x08, 0x03,
                                  0x04, 0x20, 0x00, 0x00, 0x00, 0x00, 0x03, 0x10, 0x01, 0x10,
                                  0x01, 0x10, 0x10, 0x00, 0x80, 0x3e, 0x00, 0x00, 0xc0, 0x02,
                                  0x40, 0x02, 0x11, 0x10, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00};
uint32_t dev_play_task::run()
{
    do
    {
        if (dev_handle_ < 0)
        {
            device_t dev = s_handle_->get_device();
            dev_handle_ = dps_device_access_mgr::_()->start_capture(
                dev.dev_type, dev.ip, dev.dev_ch, dev.stream_type, s_handle_->context(),recv_data_cb_,dev.port,dev.usr,dev.password,dev.link_type);

            if (dev_handle_ < 0)
            {
                dps_replay_mgr::_()->post(s_handle_);
                break;
            }
            else
            {
                s_handle_->set_dev_handle(dev_handle_);
                s_handle_->set_state(STATE_OPEN);
            }
        }

		std::cout<<"play success:"<<"ip:"<<s_handle_->get_device().ip<<" transmit_ch:"<<s_handle_->get_device().transmit_ch<<" dev_type:"<<s_handle_->get_device().dev_type<<" dev_ch:"<<s_handle_->get_device().dev_ch
            <<" codce_type:"<<s_handle_->get_device().stream_type<<std::endl;

        create_src_task* ptr_create_src_task = new create_src_task(s_handle_,send_data_cb_);
        if (NULL != ptr_create_src_task)
        {
            ptr_create_src_task->process_event();
        }
    } while (0);
    return 0;
}

uint32_t create_src_task::run()
{
    do 
    {
        if (s_handle_->src_is_create()) break;
        char sdp[DPS_MAX_SDP_LEN] = {0};
        long sdp_len = DPS_MAX_SDP_LEN;
        long data_type = -1;
        long get_ret = dps_device_access_mgr::_()->get_sdp_by_handle(s_handle_->dev_handle(), sdp, sdp_len,data_type);
        if(get_ret < 0 || sdp_len < 1)
        {
            if (PLAY_TRY_COUNT <= try_count_++)
            {
                if (DEV_HC == s_handle_->get_device().dev_type)
                {
                    sdp_len = sizeof(g_sdp_hc);
                    std::memcpy(sdp,g_sdp_hc,sdp_len);
                }
                else
                {
                    break;
                }
            }
            else
            {
                return PLAY_TRY_TIME;
            }
        }
        
        src_info_t src(sdp,sdp_len,data_type,s_handle_->get_device().transmit_ch);
        srcno_t srcno = dps_data_send_mgr::_()->create_src_ex(src);
        src.srcno_ = srcno;
        dps_data_send_mgr::_()->set_key_data(src.srcno_,src.sdp_,src.sdp_len_,src.data_type_);

        s_handle_->set_sdp(sdp);
        s_handle_->add_src_info(src);
        s_handle_->set_send_data_cb(send_data_cb_);

		std::cout<<"create src success:"<<"ip:"<<s_handle_->get_device().ip<<" transmit_ch:"<<s_handle_->get_device().transmit_ch<<" dev_type:"<<s_handle_->get_device().dev_type<<" dev_ch:"<<s_handle_->get_device().dev_ch
            <<" codce_type:"<<s_handle_->get_device().stream_type<<" srcno:"<<srcno<<std::endl;
    } while (0);
    return 0;
}

uint32_t dev_stop_task::run()
{
    s_handle_->stop_capture();
    return 0;
}
