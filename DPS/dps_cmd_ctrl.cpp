//
//create by songlei 20160316
//
#include "dps_cmd_ctrl.h"
#include "dps_ch_mgr.h"
#include "dps_data_send_mgr.h"

dps_cmd_ctrl dps_cmd_ctrl::my_;

void dps_cmd_ctrl::run()
{
    register_cmd();
    cli_.run(std::cin, std::cout, framework::core::get_thread_pool());
}

void dps_cmd_ctrl::register_cmd()
{
    cli_.register_cmd("help", help, "show dps help infos!");
    cli_.register_cmd("cls", cls, "clear screen!");
    cli_.register_cmd("spi", spi, "show cur dps play device and transmit channle infos!");
    cli_.register_cmd("scoi", scoi, "show dest play dps rtcp infos!");
    cli_.register_cmd("sdp", sdp, "show transmit channle sdp info!")->add_options()
        ("ch,h", framework::cli::value<int>()->default_value(-1), "transmit channle!");
}

void dps_cmd_ctrl::help(const framework::cli::command_description_t& cmd, const framework::cli::variables_map_t& vm, std::ostream& os)
{
    os<<std::endl;
    _()->cli_.print(os);
}

void  dps_cmd_ctrl::cls(const framework::cli::command_description_t& cmd, const framework::cli::variables_map_t& vm, std::ostream& os)
{
#ifdef _WIN32
    ::system("cls");
#else
    ::system("clear");
#endif //#ifdef _WIN32
}

void dps_cmd_ctrl::spi(const framework::cli::command_description_t& cmd, const framework::cli::variables_map_t& vm, std::ostream& os)
{
    os<<std::endl;
    dps_ch_mgr::dps_dev_s_handle_container_t s_hadles;
    dps_ch_mgr::_()->get_all_s_handle(s_hadles);
    dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_hadles.begin();
    for ( ;s_hadles.end() != itr; ++itr)
    {
        dps_dev_s_handle_t s_handle = *itr;
        if ( NULL == s_handle) continue;

        os<<"fn:"<<s_handle->get_recv_frame_nums();
        if (s_handle->get_src())
        {
            os<<"|transmit_ch:"<<s_handle->get_src()->transmit_ch_;
            os<<"|srcno:"<<s_handle->get_src()->srcno_;
        }
        os<<"|dev_handle:"<<s_handle->dev_handle();
        os<<"|ip:"<<s_handle->get_device().ip;
        os<<"|port:"<<s_handle->get_device().port;
        os<<"|dev_ch:"<<s_handle->get_device().dev_ch;
        os<<"|stream_type:"<<s_handle->get_device().stream_type;
        os<<"|time:"<<s_handle->get_create_time();
        os<<std::endl;
    }

}

void dps_cmd_ctrl::scoi(const framework::cli::command_description_t& cmd, const framework::cli::variables_map_t& vm, std::ostream& os)
{
    os<<std::endl;
    dps_data_send_mgr::connect_info_container_t connect_infos;
    dps_data_send_mgr::_()->get_connect_info(connect_infos);

    dps_data_send_mgr::connect_info_container_itr_t itr = connect_infos.begin();
    for(;connect_infos.end() != itr; ++itr)
    {
        os<<"srcno:"<<itr->srcno;
        os<<"|reality_ch:"<<itr->m_lChID;
        os<<"|send_port:"<<itr->m_lSendPort;
        //目标IP
        os<<"|dst_ip:"<<itr->m_pszDestIP;
        //目标端口
        os<<"|dst_port:"<<itr->m_lDestPort;
        //协议
        os<<"|protocl:"<<itr->m_usProtocol;
        //ssrc
        os<<"|ssrc:"<<itr->m_uiSsrc;
        //累计发包量
        os<<"|send_packets:"<<itr->m_uiPackets;
        //累计发送字节数（KB）
        os<<"| send_octets(KB):"<<itr->m_uiOctets;
        //累计丢包量
        os<<"|cumulative_lost:"<<itr->m_uiCumulativeLost;
        //丢包率
        os<<"|fraction_lost:"<< itr->m_uiFractionLost;
        //网络抖动ms
        os<<"|jitter:"<<itr->m_uiJitter;
        //复用标识
        os<<"|dst_mux_flag:"<<itr->m_bDestMultiplex;
        //复用ID
        os<<"|dst_mux_id:"<<itr->m_uiDestMultid;
        os<<"|send_mux_flag:"<<itr->m_bSendMultiplex;
        os<<"|send_mux_id:"<<itr->m_uiSendMultid;
        //产生时间
        os<<"|time:"<<itr->m_pszCreateTime;
        os<<std::endl;
    }
}

void dps_cmd_ctrl::sdp(const framework::cli::command_description_t& cmd, const framework::cli::variables_map_t& vm, std::ostream& os)
{
    if (vm.count("ch"))
    {
        os<<std::endl;
        int ch = vm["ch"].as<int>();
        dps_ch_mgr::dps_dev_s_handle_container_t s_handles;
        dps_ch_mgr::_()->get_s_handle_by_ch(ch,s_handles);

        dps_ch_mgr::dps_dev_s_handle_container_itr_t itr = s_handles.begin();
        for ( ;s_handles.end() != itr; ++itr)
        {
            dps_dev_s_handle_t s_handle = *itr;
            if ( NULL == s_handle) continue;
            os<<"ip:"<<s_handle->get_device().ip;
            os<<"|port:"<<s_handle->get_device().port;
            os<<"|dev_ch:"<<s_handle->get_device().dev_ch;
            os<<"|stream_type:"<<s_handle->get_device().stream_type;
            os<<std::endl<<"["<<std::endl<<s_handle->get_sdp()<<"]"<<std::endl;
        }
    }

}
