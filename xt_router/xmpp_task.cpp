#include "xmpp_client.h"
#include "xmpp_task.h"
#include "XTRouterLog.h"

//XMPP调度部分
/////////////////////////////////////////////////////////////////////////////////////////////////
const unsigned char g_sdp_hc[] = { 0x34, 0x48, 0x4b, 0x48, 0xfe, 0xb3, 0xd0, 0xd6, 0x08, 0x03,
                                   0x04, 0x20, 0x00, 0x00, 0x00, 0x00, 0x03, 0x10, 0x01, 0x10,
                                   0x01, 0x10, 0x10, 0x00, 0x80, 0x3e, 0x00, 0x00, 0xc0, 0x02,
                                   0x40, 0x02, 0x11, 0x10, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00};
uint32_t xmpp_play_task::run()
{
#ifdef _USE_XMPP_FUNC_
    if (start_play_ret_ < 0)
    {
        dev_ids_ = iq_msg_.query_.playbody_.transmitInfo_.arg.token_; 
        dev_chanid_ = iq_msg_.query_.playbody_.transmitInfo_.arg.playChannel_; 
        dev_strmtype_ =  iq_msg_.query_.playbody_.original_.streamType_;
        play_ip_ = iq_msg_.query_.playbody_.transmitInfo_.arg.playip_;
        play_ch_ = iq_msg_.query_.playbody_.transmitInfo_.arg.playChannel_;
        play_type_ = iq_msg_.query_.playbody_.transmitInfo_.arg.playType_;
        transmit_channel_ = iq_msg_.query_.playbody_.transmitInfo_.arg.transmitChannel_;
        link_type_ = xmpp_client::instance()->to_xtrouter_linktype(iq_msg_.query_.playbody_.transmitInfo_.sessionType_,iq_msg_.query_.playbody_.transmitInfo_.arg.reuseFlag_r_);
        start_play_ret_ = XTEngine::_()->start_play("",dev_ids_, dev_chanid_, dev_strmtype_, "0.0.0.0",play_ip_, play_ch_, play_type_, transmit_channel_, link_type_,
            iq_msg_.query_.playbody_.transmitInfo_.arg.loginName_, iq_msg_.query_.playbody_.transmitInfo_.arg.loginPassword_, iq_msg_.query_.playbody_.transmitInfo_.arg.playPort_);
       int ret = XTEngine::_()->get_src_ids(dev_ids_,dev_chanid_,dev_strmtype_,src_);
       if (ret < 0)
       {
           DEBUG_LOG(DBLOGINFO,ll_error,"xmpp_play_task::run| get_src_ids fail! ret[%d] ids[%s] dev_strmtype_[%d] dev_chanid_[%d] db_ip[%s]",
               ret,dev_ids_.c_str(),dev_strmtype_,dev_chanid_,play_ip_.c_str());
       }
    }

    //点播成功后更新一次SDP
    if(-1 < start_play_ret_)
    {
        char sdp[MAX_SDP_LEN] = {0};
        long sdp_len = -1;
        long data_type = -1;
        long get_ret = media_device::_()->get_sdp_by_handle(src_.device.dev_handle,sdp,sdp_len,data_type);
        if (get_ret < 0 || sdp_len < 1)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"xmpp_play_task::run| get sdp fail try get ids[%s] dev_strmtype_[%d] dev_chanid_[%d] count[%d] db_ip[%s]",
                dev_ids_.c_str(),dev_strmtype_,dev_chanid_,get_sdp_count_,play_ip_.c_str());
            if (get_sdp_count_++ >= 300) 
            {
                if (DEV_HC == play_type_)
                {
                    sdp_len = sizeof(g_sdp_hc);
                    ::memcpy(sdp,g_sdp_hc,sdp_len);
                }
                else
                {
                    //......
                }
            }
            else
            {
                //获取SDP失败重试
                return get_sdp_try_time_;
            }
        }
        int save_ret = XTEngine::_()->save_sdp_to_srv(src_.srcno,sdp,sdp_len,data_type);
        if (save_ret < 0)
        {
            //没有sdp向CCS馈点播失败
            (void)XTEngine::_()->stop_play_by_srcno(src_.srcno);
            std::cout<<"ids:"<<dev_ids_<<" db_ip:"<< play_ip_ <<" save_ret:"<<save_ret<<" save_sdp_to_srv fail,xtrouter stop play!"<<std::endl;
            DEBUG_LOG(DBLOGINFO,ll_error,"xmpp_play_task::run| dev_ids[%s] db_ip[%s] save_ret[%d]",dev_ids_.c_str(),play_ip_.c_str(),save_ret);
            start_play_ret_ = -1;
        }
    }

    //构造反馈IQ消息
    iq_msg_.query_.playbody_.transmitInfo_.arg.playType_ = DEV_ROUTER;
    iq_msg_.query_.playbody_.transmitInfo_.sessionType_ = iq_msg_.query_.playbody_.play_.sessionType_;
    int trans_link = xmpp_client::instance()->to_xtrouter_linktype(iq_msg_.query_.playbody_.transmitInfo_.sessionType_,iq_msg_.query_.playbody_.transmitInfo_.arg.reuseFlag_r_);
    xmpp_client::instance()->get_local_net_info( iq_msg_.query_.playbody_.transmitInfo_.arg.playip_,iq_msg_.query_.playbody_.transmitInfo_.arg.playPort_,trans_link);
    iq_msg_.query_.playbody_.transmitInfo_.arg.transmitChannel_ = transmit_channel_;

    //向CSS反馈点播结果
    (void)xmpp_client::instance()->play_result_to_ccs(iq_msg_,start_play_ret_);

    delete this;
#endif //#ifdef _USE_XMPP_FUNC_
    return 0;
}

uint32_t xmpp_stop_play_task::run()
{
#ifdef _USE_XMPP_FUNC_
    uint32_t ret=0;

    long trans_ch = iq_msg_.query_.playbody_.transmitInfo_.arg.transmitChannel_;
    ret = XTEngine::instance()->stop_play_trans_chid(trans_ch);

    xmpp_client::instance()->stop_play_result_to_ccs(iq_msg_,ret);

    delete this;
#endif //#ifdef _USE_XMPP_FUNC_
    return 0;
}

uint32_t xmpp_play_inform_request_task::run()
{
#ifdef _USE_XMPP_FUNC_
    uint32_t ret=0;

    std::string local_ids = xmpp_client::instance()->get_local_jid();
    std::string local_ip = xmpp_client::instance()->get_local_ip();
    uint32_t regist_bind_port = xmpp_client::instance()->get_regist_bind_port();

    ret = media_server::regist(	local_ids,local_ip, regist_bind_port,
        iq_.query_.playbody_.transmitInfo_.arg.routerip_,iq_.query_.playbody_.transmitInfo_.arg.routerport_);

    //向CCS反馈注册结果
    xmpp_client::instance()->play_inform_replay_to_ccs(iq_,ret);

    delete this;
#endif//#ifdef _USE_XMPP_FUNC_
    return 0;
}

uint32_t xmpp_play_stop_inform_request_task::run()
{
#ifdef _USE_XMPP_FUNC_
    uint32_t ret=0;

    ret = media_server::stop_regist(iq_.query_.playbody_.transmitInfo_.arg.routerip_.c_str(),
        iq_.query_.playbody_.transmitInfo_.arg.routerport_);

    xmpp_client::instance()->stop_inform_replay_to_ccs(iq_,ret);

    delete this;
#endif //#ifdef _USE_XMPP_FUNC_
    return 0;

}

uint32_t xmpp_stop_all_task::run()
{
#ifdef _USE_XMPP_FUNC_
    int result = XTEngine::_()->stop_allplay();
    delete this;
#endif //#ifdef _USE_XMPP_FUNC_
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
