#include "xt_session_sip.h"
#include "h_xtmediaserver.h"
#include <stdio.h>
#include <algorithm>
#include "common.h"
#include <boost/asio.hpp>
#include "XTRtp.h"
#include "XTSrc.h"

extern MS_CFG ms_cfg;

/*const char g_sdp[] = 
"v=0\r\n"
"o=- 1431483883707745 1 IN IP4 172.16.4.131\r\n"
"s=RTSP/RTP stream from IPNC\r\n"
"i=2?videoCodecType=H.264\r\n"
"t=0 0\r\n"
"a=tool:LIVE555 Streaming Media v2010.07.29\r\n"
"a=type:broadcast\r\n"
"a=control:*\r\n"
"a=range:npt=0-\r\n"
"a=x-qt-text-nam:RTSP/RTP stream from IPNC\r\n"
"a=x-qt-text-inf:2?videoCodecType=H.264\r\n"
"m=video 0 RTP/AVP 96\r\n"
"c=IN IP4 172.16.4.131\r\n"
"b=AS:12000\r\n"
"a=rtpmap:96 H264/90000\r\n"
"a=fmtp:96 packetization-mode=1;profile-level-id=64001F;max-mbps=35000;max-fs:3600;max-smbps=395500;sprop-parameter-sets=Z2QAKK2EBUViuKxUdCAqKxXFYqOhAVFYrisVHQgKisVxWKjoQFRWK4rFR0ICorFcVio6ECSFITk8nyfk/k/J8nm5s00IEkKQnJ5Pk/J/J+T5PNzZprQC0Dn88qQAAAMB4AAAcIGBAAPoAAARlBve+F4RCNQAAAAB,aO48sA==\r\n"
"a=framerate:30.0\r\n"//帧率
"a=control:track1\r\n"
"m=audio 0 RTP/AVP 96\r\n"
"c=IN IP4 172.16.4.131\r\n"
"b=AS:256\r\n"
"a=rtpmap:96 MPEG4-GENERIC/16000/2\r\n"
"a=fmtp:96 streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1410\r\n"
"a=control:track2\r\n";//*/

xt_session_sip xt_session_sip::self_;
xt_session_sip::xt_session_sip()
:sip_(XT_SIP_INVALID_HANDLE)
,ptz_cb_(NULL)
,picupdate_cb_(NULL)
,message_cb_(NULL)
,info_cb_(NULL)
,igrmconfreq_cb_(NULL)
,video_adjust_cb_(NULL)
,get_dev_info_cb_(NULL)
,point_index_operation_cb_(NULL)
,register_srv_ret_cb_(NULL)
,regist_status_(false)
{
    register_info_.handle =	 XT_SIP_INVALID_HANDLE;
}

xt_session_sip::~xt_session_sip()
{ 
}

int xt_session_sip::start(const start_sip_port_t& sip_port, const start_timer_t& timer, const char *domain, const char *username, const char *password)
{
    DEBUG_LOG("xt_session_sip::start start!\n");
    if (start_)
    {
        DEBUG_LOG("xt_session_sip::start again!\n");
        DEBUG_LOG("xt_session_sip::start end!\n");
        return 0;
    }

    xt_sip_log_init(XT_SIP_LOG_NONE, "xt_sip_test", "xt_sip_test.log");

    xt_sip_options_t options = { 0 }; 
    options.flags = XT_SIP_TRAITS_CLIENT_INVITE | XT_SIP_TRAITS_SERVER_INVITE | XT_SIP_TRAITS_PAGER_MESSAGE |XT_SIP_TRAITS_CLIENT_REGISTER;
    options.domain = domain;
    options.user = username;
    options.pwd = password;
    options.realm = domain;

    options.tcp_port = 0;
    options.udp_port  = 0;
    if (sip_port.protocol & TRANS_PROTOCOL_TCP)
    {
        options.tcp_port = sip_port.transport;
    }

    if (sip_port.protocol & TRANS_PROTOCOL_UDP)
    {
        options.udp_port = sip_port.transport;
    }
    if (options.udp_port == 0  && options.tcp_port == 0)
    {

        DEBUG_LOG("sip start fail! tcp port and udp port is 0!\n");
        return -1;
    }

    xt_sip_server_invite_callback_t server_invite_cb = { 0 };
    server_invite_cb.on_offer = on_server_invite_offer;
    server_invite_cb.on_connected_confirmed = on_server_invite_connected_confirmed;
    server_invite_cb.on_offer_required = on_server_invite_offer_required;
    server_invite_cb.on_answer = on_server_invite_answer;
    server_invite_cb.on_terminated = on_sip_server_invite_terminated;
    server_invite_cb.on_info = on_server_invite_info;
    server_invite_cb.on_info_response = on_server_invite_info_response;
    server_invite_cb.on_message = on_server_invite_message;
    server_invite_cb.on_message_response = on_server_invite_message_response;

    options.server_invite_callback = &server_invite_cb;
    options.server_invite_callback_ctx = static_cast<void*>(this);

    xt_sip_server_message_callback_t server_message_cb = { 0 };
    server_message_cb.on_arrived = on_server_message_arrived;

    options.server_message_callback = &server_message_cb;
    options.server_message_callback_ctx = static_cast<void*>(this);

    DEBUG_LOG("xt_sip_create start\n");


    xt_sip_ext_options_t ext_optins = {0};
    ext_optins.default_session_time = timer.default_session_time_interval;
    ext_optins.default_registration_retry_time = timer.default_registration_retry_time_interval;
    ext_optins.tls_port = sip_port.tls_port;
    ext_optins.dtls_port = sip_port.dtls_port;

    xt_sip_status_t ret_code = ::xt_sip_create(&options, &ext_optins, &this->sip_);

    if (ret_code != XT_SIP_STATUS_OK)
    {

        DEBUG_LOG("xt_sip_create fail ret_code[%d]\n",ret_code);
        return -1;
    }

    DEBUG_LOG("xt_session_sip::start:ret_code[%d] domain[%s] username[%s] password[%s] realm[%s] tcp_port[%d] udp_port[%d] session_time[%d] tls_port[%d] dtls_port[%d]\n",
        ret_code,options.domain,options.user,options.pwd,options.realm,options.tcp_port,options.udp_port,ext_optins.default_session_time,ext_optins.tls_port,ext_optins.dtls_port);

    start_ = true;
    regist_status_ = false;

    DEBUG_LOG("xt_session_sip::start end!\n");


    return 0;
}

void xt_session_sip::stop()
{

    DEBUG_LOG("xt_session_sip::stop start!\n");
    if (start_)
    {
        start_ = false;
        ::xt_sip_destroy(sip_);
    }
    else
    {
        DEBUG_LOG("xt_session_sip::stop: no start_sip regist fail\n");
    }

    DEBUG_LOG("xt_session_sip::stop end!\n");


}

void xt_session_sip::
regist(const regist_timer_t& timer, const sip_channel_t& chtype,const char *target,const char* sdp, uint32_t sdp_len)
{
    DEBUG_LOG("xt_session_sip::regist start!111111111111111111111111........\n");
    DEBUG_LOG("xt_session_sip::regist start!\n");
    if (!start_)
    {
        DEBUG_LOG("regist: no start_sip regist fail\n");
        return;
    }

    unregist();

    register_info_.timer.expires = timer.expires;
    register_info_.timer.link_time_interval = timer.link_time_interval;
    register_info_.timer.regist_retry_time_interval = timer.regist_retry_time_interval;
    if (target)
    {
        ::strcpy(register_info_.target, target);
    }
    else
    {
        ::strcpy(register_info_.target, "");
    }

    register_info_.sdp_len = sdp_len;
    if (sdp)
    {
        ::strcpy(register_info_.sdp, sdp);
    }
    else
    {
        ::strcpy(register_info_.sdp, "");
        register_info_.sdp_len = 0;
    }

    register_info_.channel_type.type = chtype.type;

    register_info_.channel_type.delay = chtype.delay;
    register_info_.channel_type.bandwidth = chtype.bandwidth;
    register_info_.channel_type.mtu = chtype.mtu;
    register_info_.channel_type.packetloss = chtype.packetloss;

    regist_impl();
    DEBUG_LOG("xt_session_sip::regist end!\n");
}

void xt_session_sip::unregist()
{
    if (regist_status_)
    {
        DEBUG_LOG("unregist():xt_sip_client_register_remove_all\n");
        ::xt_sip_client_register_remove_all(register_info_.handle);

        DEBUG_LOG("unregist():xt_sip_heartbeat_remove_target\n");
        ::xt_sip_heartbeat_remove_target(sip_,register_info_.target);        

        DEBUG_LOG("unregist():wait_regist_removed\n");
        wait_regist_removed();

        DEBUG_LOG("unregist(): xt_sip_client_register_handle_delete\n");
        if (XT_SIP_INVALID_HANDLE != register_info_.handle)
        {
            ::xt_sip_client_register_handle_delete(register_info_.handle);
            register_info_.handle = XT_SIP_INVALID_HANDLE;
        }
        regist_status_ = false;
    }
}

void xt_session_sip::regist_impl()
{
    xt_sip_client_register_request_t request;
    request.target = register_info_.target;
    request.expires = register_info_.timer.expires;//过期时间 服务器过期时间和本地刷新时间发送重新注册时间间隔

    //信道描述
    xt_sip_channel_category_t sip_ch;
    sip_ch.bandwidth = register_info_.channel_type.bandwidth;
    sip_ch.delay = register_info_.channel_type.delay;
    sip_ch.mtu = register_info_.channel_type.mtu;
    sip_ch.packetloss = register_info_.channel_type.packetloss;
    sip_ch.type = register_info_.channel_type.type;
    request.channel = &sip_ch;

    if ( 0 == register_info_.sdp_len)
    {
        request.sdp = NULL;
        request.length = 0;
    }
    else
    {
        request.sdp = register_info_.sdp;
        request.length = register_info_.sdp_len;
    }

    xt_sip_client_register_callback_t cicall = { 0 };

    cicall.on_removed = on_client_register_removed;
    cicall.on_request_retry = on_client_register_request_retry;
    cicall.on_response = on_client_register_response;

    xt_sip_status_t ret_code = ::xt_sip_make_client_register(this->sip_,NULL,&request, NULL,&cicall, this, 3000);
    if (XT_SIP_STATUS_OK == ret_code)
    {
    }

    DEBUG_LOG("xt_session_sip::func_regist |xt_sip_make_client_register register_ret[%d] target[%s] expires[%d] options_time[%d] mtu[%d]\n",
        ret_code,request.target,request.expires,register_info_.timer.link_time_interval,request.channel->mtu);
}

void xt_session_sip::del_options()
{
    xt_sip_status_t ret_code = ::xt_sip_heartbeat_remove_target(this->sip_,register_info_.target);
    DEBUG_LOG("del_options xt_sip_heartbeat_remove_target[%s] ret_code[%d]\n",register_info_.target,ret_code);

}
void xt_session_sip::add_options()
{
    xt_sip_status_t ret_code = ::xt_sip_heartbeat_add_target(
        sip_, register_info_.target,register_info_.timer.link_time_interval, register_info_.timer.link_time_interval, on_xt_sip_heartbeat_not_pong_cb, this);

    DEBUG_LOG("add_options xt_sip_heartbeat_add_target[%s] ret_code[%d]\n",register_info_.target,ret_code);
}

void xt_session_sip::update_register_stat(const bool status)
{
    regist_status_ = status;
}

void xt_session_sip::
on_client_invite_terminated(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, xt_sip_invite_terminated_reason_t reason)
{
    DEBUG_LOG("on_client_invite_terminated \n");
}

void xt_session_sip::
on_client_register_response(void *ctx, xt_sip_client_register_handle_t h, xt_sip_msg_handle_t msg, uint8_t success)
{
    DEBUG_LOG("on_client_register_response  success[%d]\n",success);
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("on_client_register_response error\n");
        return;
    }

    if (NULL != psip->register_srv_ret_cb_)
    {
        //获取from
        xt_sip_buf_handle_t from = psip->xt_sip_msg_get_from(msg);
        if (XT_SIP_INVALID_HANDLE != from)
        {
            std::string target;
            target.assign((const char*)psip->xt_sip_buf_get_data(from),psip->xt_sip_buf_get_len(from));
            psip->xt_sip_buf_handle_delete(from);
            psip->register_srv_ret_cb_(target.c_str(),success);
        }
    }

    //注册成功更新注册句柄
    if (0 < success)
    {
        psip->update_register_stat(true);
        psip->upate_client_register_handle(h);
        DEBUG_LOG("on_client_register_response upate_client_register_handle\n");
        psip->add_options();
    }
    else
    {
        psip->update_register_stat(false);
        psip->del_options();
    }
}

//注消成功反馈
void xt_session_sip::
on_client_register_removed(void *ctx, xt_sip_client_register_handle_t h, xt_sip_msg_handle_t msg)
{ 
    DEBUG_LOG("on_client_register_removed \n");

    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("on_client_register_response error psip is empty!\n");
        return;
    }

	DEBUG_LOG("on_client_register_removed: clear_send_data_all start...\n");
	psip->clear_send_data_all();
	DEBUG_LOG("on_client_register_removed: clear_send_data_all end.\n");

    psip->notify_regist_removed();
}


//regist 失败响应 请求重试时间 -1为不重试
int xt_session_sip::
on_client_register_request_retry(void *ctx, xt_sip_client_register_handle_t h, int retrysec, xt_sip_msg_handle_t msg)
{
    DEBUG_LOG("on_client_register_request_retry \n");
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("on_client_register_response error psip is empty!\n");
        return -1;
    }

    psip->update_register_stat(false);
    return psip->get_regist_retry_time();
}

//option 
int8_t xt_session_sip::on_xt_sip_heartbeat_not_pong_cb(void *ctx, const char *target, uint32_t)
{

    DEBUG_LOG("xt_sip_heartbeat_not_pong_cb: target[%s]\n",target);
    int8_t ret_code = 0;
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("xt_sip_heartbeat_not_pong_cb:psip error psip  is empty!\n");
        return -1;
    }

    do
    {
        DEBUG_LOG("xt_sip_heartbeat_not_pong_cb xt_sip_client_register_request_refresh start...\n");
        xt_sip_status_t ret_regist_status = psip->xt_sip_client_register_request_refresh();
        DEBUG_LOG("xt_sip_heartbeat_not_pong_cb xt_sip_client_register_request_refresh end: ret_regist_status[%d]!\n",ret_regist_status);

        DEBUG_LOG("on_xt_sip_heartbeat_not_pong_cb: clear_send_data_all start...\n");
        psip->clear_send_data_all();
        DEBUG_LOG("on_xt_sip_heartbeat_not_pong_cb: clear_send_data_all end.\n");
    } while (0);

    return 1;
}

void xt_session_sip::
on_server_invite_offer(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len)
{
    DEBUG_LOG("on_server_invite_offer start!\n");
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("on_offer:psip error psip  is empty!\n");
        return;
    }

    int oper_code = 0;
    do 
    {
        if (XT_SIP_INVALID_HANDLE == msg)
        {
            DEBUG_LOG("on_offer:psip error msg is empty!\n");
            oper_code=-1;
            break;
        }

        session_info_t session;
        session.creat_time = common::GetCurTimeSecondValue();

        //获取call id
        xt_sip_buf_handle_t cid = psip->xt_sip_msg_get_call_id(msg);
        if (XT_SIP_INVALID_HANDLE == cid)
        {
            oper_code = -5;
            break;
        }

		const char * data = static_cast<char*>(psip->xt_sip_buf_get_data(cid));
		uint32_t len_data = psip->xt_sip_buf_get_len(cid);
		session.call_id.assign(data, len_data);	
		psip->xt_sip_buf_handle_delete(cid);

		//获取资源优先级
		session.res_pri = "cmd.0";
		printf("Resource-Priority------------------------1111\n");
		xt_sip_buf_handle_t Resource_Priority = ::xt_sip_msg_get_extension_header(msg, "Resource-Priority");
		if (XT_SIP_INVALID_HANDLE != Resource_Priority)
		{
			printf("Resource-Priority------------------------\n");
			const char * data1 = static_cast<char*>(psip->xt_sip_buf_get_data(Resource_Priority));
			uint32_t len_data1 = psip->xt_sip_buf_get_len(Resource_Priority);
			session.res_pri.assign(data1, len_data1);	
			psip->xt_sip_buf_handle_delete(Resource_Priority);
			printf("Resource-Priority:%s------------------------\n", session.res_pri.c_str());
		}

		//检查优先级，仅支持一路呼叫
		if (!psip->is_exist_sip_session(session) && psip->check_pri(session.res_pri) < 0 )
		{
			oper_code = -9;
			break;
		}

        //获取from
        xt_sip_buf_handle_t from = psip->xt_sip_msg_get_from(msg);
        if (XT_SIP_INVALID_HANDLE == from)
        {
            oper_code = -6;
            break;
        }
        session.peerurl.assign((const char*)psip->xt_sip_buf_get_data(from),psip->xt_sip_buf_get_len(from));
        psip->xt_sip_buf_handle_delete(from);

        //获取streamid
        if (!sdp ||::strlen(sdp) < 1 || len >= LEN_SDP)
        {
            DEBUG_LOG("on_offer:sdp error\n");
            oper_code=-2;
            break;
        }

        int streamid = psip->get_strmid(sdp, len);
        if (streamid>=NUM_STREAM)
        {
            DEBUG_LOG("on_offer:streamid error\n");
            oper_code=-3;
            break;
        }
        if (streamid < 0)
        {
            streamid = 0;
        }
        session.srcno = streamid;

        //设置接收sdp到会话
        ::memset(session.sdp, 0, LEN_SDP);
        ::memcpy(session.sdp, sdp, len);
        session.len_sdp = len;

        xt_sdp::sdp_session_t xsdp_rcv;

        psip->sdp_parse(sdp,len,xsdp_rcv);

        psip->video_bandwidth_set(xsdp_rcv);

        psip->video_fmtp_set(xsdp_rcv);

		uint32_t mux_update_flag=0;

        //更新复用标识
        int ret1 = psip->update_rtpport_mux(sdp, len, psip->sdp_[streamid], sizeof(psip->sdp_[streamid])-1, session);

        switch(ret1)
        {
        case 0:
            xt_set_key(streamid, psip->sdp_[streamid], sizeof(psip->sdp_[streamid])-1, 172);
			mux_update_flag = 1;
            DEBUG_LOG("update_rtpport_mux sussessful!\n");
            break;
        case 1:
            DEBUG_LOG("on_offer:Not need to update_rtpport_mux...\n");
            break;
        default:
            DEBUG_LOG("on_offer:update_rtpport_mux error ret[%d]\n",ret1);
            break;
        }

        if (!psip->is_exist_sip_session(session))
        {
            //该接口用于新的INVITE时，如果SDP有分辨率字段，则设置分辨率
            psip->modify_video_resolution(session,streamid,sdp,len);
        }


        std::string sdp_ack;
        int ret = psip->sdp_compare(sdp, len, psip->sdp_[streamid], sizeof(psip->sdp_[streamid])-1, sdp_ack, session);
        if (ret < 0)
        {
            DEBUG_LOG("on_offer:sdp_compare error ret[%d]\n",ret);
            oper_code=-4;
            break;
        } 

        //RE-INVITE
        if (psip->is_exist_sip_session(session))
        {
            psip->pro_re_inivte(session,streamid,sdp,len, mux_update_flag);
        }
        else
        {
            psip->add_sip_session(session);
        }

        DEBUG_LOG("on_offer:xt_sip_server_invite_provide_answer start!\n");
        xt_sip_status_t ret_stat = psip->xt_sip_server_invite_provide_answer(h,sdp_ack.c_str(),sdp_ack.length());
        if (XT_SIP_STATUS_OK != ret_stat)
        {
            DEBUG_LOG("on_offer:xt_sip_server_invite_provide_answer error\n");
            break;
        }
        DEBUG_LOG("on_offer:xt_sip_server_invite_provide_answer end!\n");

        //所有操作都完成
        oper_code = 1;

    } while (false);

    //能力级配置失败拒绝远端操作
    if (oper_code <0)
    {
        DEBUG_LOG("on_offer:xt_sip_server_invite_reject\n");
        psip->xt_sip_server_invite_reject(h);
        DEBUG_LOG("xt_sip_server_invite_provide_answer :Err xt_sip_server_invite_reject errode[%d] \n",oper_code);
    }

    DEBUG_LOG("on_server_invite_offer end!");
}

void xt_session_sip::on_server_invite_offer_required(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg)
{
    DEBUG_LOG("on_server_invite_offer_required start!\n");
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("on_server_invite_answer:psip error psip  is empty!\n");
        return;
    }

    int streamid = 0;
    if (strlen(psip->sdp_full_[streamid]) > 0)
    {
        psip->xt_sip_server_invite_provide_offer(h, psip->sdp_full_[streamid], sizeof(psip->sdp_full_[streamid])-1);
    }
    else
    {
        psip->xt_sip_server_invite_provide_offer(h, psip->sdp_[streamid], sizeof(psip->sdp_[streamid])-1);
    }

    DEBUG_LOG("on_server_invite_offer_required end!\n");
}

void xt_session_sip::on_server_invite_answer(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len)
{
    DEBUG_LOG("on_server_invite_answer start!\n");
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("on_server_invite_answer:psip error psip  is empty!\n");
        return;
    }

    int oper_code = 0;
    do 
    {
        if (XT_SIP_INVALID_HANDLE == msg)
        {
            DEBUG_LOG("on_server_invite_answer:psip error msg is empty!\n");
            oper_code=-1;
            break;
        }

        session_info_t session;
        session.creat_time = common::GetCurTimeSecondValue();

        //获取call id
        xt_sip_buf_handle_t cid = psip->xt_sip_msg_get_call_id(msg);
        if (XT_SIP_INVALID_HANDLE == cid)
        {
            oper_code = -5;
            break;
        }

        const char * data = static_cast<char*>(psip->xt_sip_buf_get_data(cid));
        uint32_t len_data = psip->xt_sip_buf_get_len(cid);
        session.call_id.assign(data, len_data);	
        psip->xt_sip_buf_handle_delete(cid);

		//获取资源优先级
		session.res_pri = "cmd.0";
		xt_sip_buf_handle_t Resource_Priority = ::xt_sip_msg_get_extension_header(msg, "Resource-Priority");
		if (XT_SIP_INVALID_HANDLE != Resource_Priority)
		{
			const char * data1 = static_cast<char*>(psip->xt_sip_buf_get_data(Resource_Priority));
			uint32_t len_data1 = psip->xt_sip_buf_get_len(Resource_Priority);
			session.res_pri.assign(data1, len_data1);	
			psip->xt_sip_buf_handle_delete(Resource_Priority);
		}

		//检查优先级，仅支持一路呼叫
		if (!psip->is_exist_sip_session(session) && psip->check_pri(session.res_pri) < 0 )
		{
			oper_code = -9;
			break;
		}

        //获取from
        xt_sip_buf_handle_t from = psip->xt_sip_msg_get_from(msg);
        if (XT_SIP_INVALID_HANDLE == from)
        {
            oper_code = -6;
            break;
        }
        session.peerurl.assign((const char*)psip->xt_sip_buf_get_data(from),psip->xt_sip_buf_get_len(from));
        psip->xt_sip_buf_handle_delete(from);

        //获取streamid
        if (!sdp ||::strlen(sdp) < 1 || len >= LEN_SDP)
        {
            DEBUG_LOG("on_server_invite_answer:sdp error\n");
            oper_code=-2;
            break;
        }
        int streamid = psip->get_strmid(sdp, len);
        if (streamid>=NUM_STREAM)
        {
            DEBUG_LOG("on_server_invite_answer:streamid error\n");
            oper_code=-3;
            break;
        }
        if (streamid < 0)
        {
            streamid = 0;
        }
        session.srcno = streamid;

        //设置接收sdp到会话
        ::memset(session.sdp, 0, LEN_SDP);
        ::memcpy(session.sdp, sdp, len);
        session.len_sdp = len;

        xt_sdp::sdp_session_t xsdp_rcv;

        psip->sdp_parse(sdp,len,xsdp_rcv);

        psip->video_bandwidth_set(xsdp_rcv);

        psip->video_fmtp_set(xsdp_rcv);

        //更新复用标识
        int ret1 = psip->update_rtpport_mux(sdp, len, psip->sdp_[streamid], sizeof(psip->sdp_[streamid])-1, session);

        switch(ret1)
        {
        case 0:
            xt_set_key(streamid, psip->sdp_[streamid], sizeof(psip->sdp_[streamid])-1, 172);
            DEBUG_LOG("update_rtpport_mux sussessful!!!\npsip->sdp_[streamid:%d] :\n%s\n",
                streamid,psip->sdp_[streamid]);
            break;
        case 1:
            DEBUG_LOG("on_server_invite_answer:Not need to update_rtpport_mux...\n");
            break;
        default:
            DEBUG_LOG("on_server_invite_answer:update_rtpport_mux error ret[%d]\n",ret1);
            break;
        }

        if (!psip->is_exist_sip_session(session))
        {
            //该接口用于新的INVITE时，如果SDP有分辨率字段，则设置分辨率
            psip->modify_video_resolution(session,streamid,sdp,len);
        }

        std::string sdp_ack;
        int ret = psip->sdp_compare(sdp, len, psip->sdp_[streamid], sizeof(psip->sdp_[streamid])-1, sdp_ack, session);
        if (ret < 0)
        {
            DEBUG_LOG("on_server_invite_answer:sdp_compare error ret[%d]\n",ret);
            oper_code=-4;
            break;
        } 

        psip->add_sip_session(session);

        for (std::size_t i=0; i<session.tracks.size();++i)
        {
            if (session.tracks[i].port != 0)
            {
                //负载更新
                DEBUG_LOG("xt_set_payload src[%d] name[%s] payload[%d]\n", session.srcno, session.tracks[i].trackname,session.tracks[i].payload);
                ::xt_set_payload(session.srcno, session.tracks[i].trackname, session.tracks[i].payload,true);

                DEBUG_LOG("xt_add_send src[%d] name[%s] ip[%s] port[%d]\n", session.srcno, session.tracks[i].trackname, session.tracks[i].ip, session.tracks[i].port);
                ::xt_add_send(session.srcno, session.tracks[i].trackname, session.tracks[i].ip, session.tracks[i].port, 
                    session.tracks[i].demux, session.tracks[i].demuxid);
            }
        }

        if (psip->picupdate_cb_)
        {
            int oprcode = 0;
            char fail_case[2048]={0};
            psip->picupdate_cb_(session.srcno,oprcode,fail_case);
        }

        //所有操作都完成
        oper_code = 1;

    } while (false);

    DEBUG_LOG("on_server_invite_answer end!");
}

/************************************************************************/
/*add by qinchao 2015.09.28
//! 当server端接收到呼叫时修改分辨率
//! \param [session] session_info_t结构体，没有用处，暂时保留与历史接口一致
//! \param [strmid] 流id号，一般主码流为0，子码流为1
//! \param [sdp] 呼叫端发过来的sdp字符串数组
//! \param [len] 呼叫端发过来的sdp字符串数组长度
//! \returns void
*/
/************************************************************************/
void xt_session_sip::
modify_video_resolution(const session_info_t &session,const int strmid,const char *sdp, uint32_t len)
{
    DEBUG_LOG("on_offer:modify_video_resolution...\n");
    video_adjust_param_t params = {"\0","\0","\0"};

    // modify the video resolution framerate max_br.
    if (-1 < construct_video_adjust_param(params,session,sdp,len))
    {
        //如果回调函数存在，并且分辨率字符串已经获取到（不为“\0”）,就调用视频设置回调
        if (video_adjust_cb_)
        {
            DEBUG_LOG("on_offer:modify video resolution:[%s],framerate:[%s],max_br:[%s],strmid:[%d]\n",
                params.resolution, params.framerate, params.max_br, strmid);
            video_adjust_cb_(params,strmid);
        }
    }
}

void xt_session_sip::
pro_re_inivte(const session_info_t &new_session,const int strmid,const char *sdp, uint32_t len, uint32_t mux_flag)
{
    DEBUG_LOG("on_offer:RE-INVITE\n");
    int ret_code = -1;
    int old_srcno=0;
    bool old_demux = false;
    unsigned short old_port=0;
    unsigned int old_demuxid=0;
    std::string old_ip;
    std::string old_trackname;

    int new_srcno=0;
    bool new_demux = false;
    unsigned short new_port=0;
    unsigned int new_demuxid=0;
    std::string new_ip;
    std::string new_trackname;
    int new_payload = -1;
    video_adjust_param_t params;

    std::vector<track_info_t>::iterator itr;

    // update media config
    if (-1 < construct_video_adjust_param(params,new_session,sdp,len))
    {
        DEBUG_LOG("on_offer:psip->video_adjust_cb_ start....\n");
        if (video_adjust_cb_)
        {
            DEBUG_LOG("on_offer:streamid[%d] framerate[%s] max_br[%s]\n",strmid,params.framerate,params.max_br);
            video_adjust_cb_(params,strmid);
            DEBUG_LOG("on_offer:psip->video_adjust_cb_ end\n");
        }
    }

    // update media stream
    session_info_t old_session;
    ret_code = find_sip_session(new_session.call_id, old_session);
    if (ret_code < 0)
    {
        DEBUG_LOG("re_inivte| find_sip_session fail!");
        return;
    }

    for (std::size_t ipos=0;ipos<new_session.tracks.size();++ipos)
    {
        new_srcno = new_session.srcno;
        new_ip.assign(new_session.tracks[ipos].ip,LEN_NODE);
        new_port = new_session.tracks[ipos].port;
        new_demux = new_session.tracks[ipos].demux;
        new_demuxid = new_session.tracks[ipos].demuxid;
        new_trackname.assign(new_session.tracks[ipos].trackname,LEN_NODE);
        new_payload = new_session.tracks[ipos].payload;

        bool find = false;
        for(itr=old_session.tracks.begin(); itr!=old_session.tracks.end();++itr)
        {
            track_info_t &track = *itr;
            old_srcno = old_session.srcno;
            old_ip.assign(track.ip,LEN_NODE);
            old_port = track.port;
            old_demux = track.demux;
            old_demuxid = track.demuxid;
            old_trackname.assign(track.trackname,LEN_NODE);

            // update exist stream
            if (new_trackname == old_trackname)
            {
                find = true;

                if (0 == new_port)
                {
                    DEBUG_LOG("re_inivte port na! on nathig! new_port[%d]\n",new_port);
                    break;
                }

                //非复用下端口变化或者复用下复用id变化时
                if (new_port != old_port
                    || ( (new_demuxid != old_demuxid) && old_demux && new_demux) || (old_demux != new_demux) )
                {
                    //更新目标信息
                    DEBUG_LOG("re_inivte|update exist stream| update send dst start...\n\n");

					int ret = 0;
					if(!mux_flag)
					{
						ret = ::xt_del_send(old_srcno, old_trackname.c_str(),old_ip.c_str(), old_port,old_demux, old_demuxid);

						DEBUG_LOG("re_inivte|update exist stream|xt_del_send ret[%d] srcno[%d] trackname[%s] ip[%s] port[%u] demux[%d] demuxid[%u]\n",
							ret,old_srcno,old_trackname.c_str(), old_ip.c_str(), old_port,old_demux,old_demuxid);
					}

                    //负载更新 
                    ret = ::xt_set_payload(new_srcno, new_trackname.c_str(), new_payload,true);

                    DEBUG_LOG("re_inivte|update exist stream|xt_set_payload srcno[%d] name[%s] payload[%d]\n",new_srcno, new_trackname.c_str(),new_payload);

                    ret = ::xt_add_send(new_srcno, new_trackname.c_str(),new_ip.c_str(), new_port, new_demux, new_demuxid);

                    DEBUG_LOG("re_inivte|update exist stream|xt_add_send ret[%d] srcno[%d] name[%s] ip[%s] port[%u] demux[%d] demuxid[%u]\n\n",
                        ret,new_srcno, new_trackname.c_str(),new_ip.c_str(), new_port,new_demux,new_demuxid);
                }
                else
                {
                    DEBUG_LOG("re_inivte|update exist stream|no change srcno[%d] name[%s] ip[%s] port[%u] demux[%d] demuxid[%u]\n\n",
                        new_srcno, new_trackname.c_str(),new_ip.c_str(), new_port,new_demux,new_demuxid);
                }

                old_session.tracks.erase(itr);
                break;
            }//if ( new_trackname == old_trackname)
        }//for(int index=0; index<old_session.track_num&&index<MAX_TRACK; ++index)

        // add new stream
        if (!find)
        {
            //负载更新 
            int ret = ::xt_set_payload(new_srcno, new_trackname.c_str(), new_payload,true);

            DEBUG_LOG("re_inivte|add new stream|xt_set_payload srcno[%d] name[%s] payload[%d]\n\n",new_srcno, new_trackname.c_str(),new_payload);

            ret = ::xt_add_send(new_srcno, new_trackname.c_str(),new_ip.c_str(), new_port, new_demux, new_demuxid);

            DEBUG_LOG("re_inivte|add new stream|xt_add_send ret[%d] srcno[%d] name[%s] ip[%s] port[%u] demux[%d] demuxid[%u]\n\n",
                ret,new_srcno, new_trackname.c_str(),new_ip.c_str(), new_port,new_demux,new_demuxid);
        }
    }//for (int ipos=0;ipos<new_session.track_num&&ipos<MAX_TRACK;++ipos) 

    // del exist stream
    for(itr=old_session.tracks.begin(); itr!=old_session.tracks.end();++itr)
    {
        track_info_t &track = *itr;
        old_srcno = old_session.srcno;
        old_ip.assign(track.ip,LEN_NODE);
        old_port = track.port;
        old_demux = track.demux;
        old_demuxid = track.demuxid;
        old_trackname.assign(track.trackname,LEN_NODE);

		if(!mux_flag)
		{
			int ret = ::xt_del_send(old_srcno, old_trackname.c_str(),old_ip.c_str(), old_port,old_demux, old_demuxid);

			DEBUG_LOG("re_inivte|del exist stream|xt_del_send ret[%d] srcno[%d] trackname[%s] ip[%s] port[%u] demux[%d] demuxid[%u]\n\n",
				ret,old_srcno,old_trackname.c_str(), old_ip.c_str(), old_port,old_demux,old_demuxid);
		}
    }

    //update session
    del_sip_session(old_session.call_id);
    add_sip_session(new_session);
}

void xt_session_sip::
on_server_invite_connected_confirmed(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg)
{
    DEBUG_LOG("on_server_invite_connected_confirmed start!\n");
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("on_server_invite_connected_confirmed:psip error psip is empty!\n");
        return;
    }

    do 
    {
        if (XT_SIP_INVALID_HANDLE == msg)
        {
            DEBUG_LOG("on_server_invite_connected_confirmed:psip error msg is empty!\n");
            break;
        }

        std::string call_id;
        xt_sip_buf_handle_t cid = psip->xt_sip_msg_get_call_id(msg);
        if (XT_SIP_INVALID_HANDLE == cid)
        {
            break;
        }
        const char * data = static_cast<char*>(psip->xt_sip_buf_get_data(cid));
        if (NULL == data)
        {
            DEBUG_LOG("on_server_invite_connected_confirmed:cid error\n");
            break;
        }

        uint32_t len_data = psip->xt_sip_buf_get_len(cid);
        call_id.assign(data,len_data);
        psip->xt_sip_buf_handle_delete(cid);

        session_info_t session;
        int ret = psip->find_sip_session(call_id, session);
        if (ret < 0)//反向sdp
        {
            break;
        }

        for (std::size_t i=0; i<session.tracks.size();++i)
        {
            if (session.tracks[i].port != 0)
            {
                //负载更新
                DEBUG_LOG("xt_set_payload src[%d] name[%s] payload[%d]\n", session.srcno, session.tracks[i].trackname,session.tracks[i].payload);
                ::xt_set_payload(session.srcno, session.tracks[i].trackname, session.tracks[i].payload,true);

                DEBUG_LOG("xt_add_send src[%d] name[%s] ip[%s] port[%d]\n", session.srcno, session.tracks[i].trackname, session.tracks[i].ip, session.tracks[i].port);
                ::xt_add_send(session.srcno, session.tracks[i].trackname, session.tracks[i].ip, session.tracks[i].port, 
                    session.tracks[i].demux, session.tracks[i].demuxid);
            }
        }

        if (psip->picupdate_cb_)
        {
            int oprcode = 0;
            char fail_case[2048]={0};
            psip->picupdate_cb_(session.srcno,oprcode,fail_case);
        }

    } while (0);

    DEBUG_LOG("on_server_invite_connected_confirmed end!\n");
}

void xt_session_sip::
on_sip_server_invite_terminated(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, xt_sip_invite_terminated_reason_t reason)
{
    DEBUG_LOG("on_sip_server_invite_terminated\n");
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("on_sip_server_invite_terminated:psip error psip  is empty!\n");
        return;
    }

    do 
    {
        if (XT_SIP_INVALID_HANDLE == msg)
        {
            DEBUG_LOG("on_sip_server_invite_terminated:psip error msg is empty!\n");
            break;
        } 

        std::string call_id;
        xt_sip_buf_handle_t cid = psip->xt_sip_msg_get_call_id(msg);
        if (XT_SIP_INVALID_HANDLE == cid)
        {
            break;
        }
        const char * data = static_cast<char*>(psip->xt_sip_buf_get_data(cid));
        uint32_t len_data = psip->xt_sip_buf_get_len(cid);
        call_id.assign(data,len_data);
        psip->xt_sip_buf_handle_delete(cid);

        session_info_t session;
        int ret = psip->find_sip_session(call_id, session);
        if (ret < 0)
        {
            break;
        }

        for (std::size_t i=0;i<session.tracks.size();++i)
        {
//             if (psip->find_sip_session_multicast(session.tracks[i]) > 1)
//             {
//                 continue;
//             }
            if (session.tracks[i].port != 0)
            {
                int ret = ::xt_del_send(session.srcno, session.tracks[i].trackname,
                    session.tracks[i].ip, session.tracks[i].port,session.tracks[i].demux, session.tracks[i].demuxid);
                if (ret < 0)
                {
                    DEBUG_LOG("on_sip_server_invite_terminated | xt_del_send fail srcno[%d],trackanme[%s] ip[%s] port[%d]",
                        session.srcno,session.tracks[i].trackname,session.tracks[i].ip,session.tracks[i].port );
                    continue;
                }
            }
        }
        psip->del_sip_session(call_id);

    } while (0);

    DEBUG_LOG("on_sip_server_invite_terminated end\n");
}

void xt_session_sip::
on_server_message_arrived(void *ctx, xt_sip_server_message_handle_t h, xt_sip_msg_handle_t msg)
{
    DEBUG_LOG("on_server_message_arrived start!\n");
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("on_server_message_arrived:psip error psip is empty!\n");
        return;
    }

    xt_sip_status_t ret_stat = XT_SIP_STATUS_OK;
    std::string respones_ctx;
    dev_flag_t dev_flag;

    ::memset(&dev_flag,0,sizeof(dev_flag_t));

    do
    {
        if (XT_SIP_INVALID_HANDLE == msg)
        {
            DEBUG_LOG("on_server_message_arrived:psip error msg is empty!\n");
            ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
            break;
        }

        xt_sip_buf_handle_t buf = psip->xt_sip_msg_get_content_body(msg);
        if (XT_SIP_INVALID_HANDLE == buf)
        {
            ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
            break;
        }

        const char * data = static_cast<char*>(psip->xt_sip_buf_get_data(buf));
        uint32_t len_data = psip->xt_sip_buf_get_len(buf);

        if (psip->message_cb_)
        {
            DEBUG_LOG("on_server_message_arrived :psip->message_cb_!\n");
            psip->message_cb_(data, len_data);
        }

        //获取from
        std::string from;
        xt_sip_buf_handle_t from_buf = psip->xt_sip_msg_get_from(msg);
        if (XT_SIP_INVALID_HANDLE == from_buf)
        {
            ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
            break;
        }
        from.assign((const char*)psip->xt_sip_buf_get_data(from_buf), psip->xt_sip_buf_get_len(from_buf));
        psip->xt_sip_buf_handle_delete(from_buf);

        xtXml xml;
        xml.LoadXMLStr(data);
        psip->xt_sip_buf_handle_delete(buf);

        xtXmlNodePtr root = xml.getRoot();
        if (root.IsNull())
        {
            ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
            break;
        }

        DEBUG_LOG("on_server_message_arrived :[%s]\nn",xml.GetXMLStrEx());

        const char *root_v = root.GetName();
        if (::strcmp(root_v,"Control")==0)
        {
            xtXmlNodePtr command = xml.getNode(root, "commandname");
            if (command.IsNull())
            {
                ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                break;
            }

            xtXmlNodePtr seq = xml.getNode(root, "seq");
            if (!seq.IsNull())
            {
                const char *seq_v = xml.getValue(seq);
                if (seq_v)
                { 
                    ::strcpy(dev_flag.seq,seq_v);
                }
            }

            //云镜控制
            const char *commandname = xml.getValue(command);
            if (::strcmp(commandname,"ptzctrl")==0 ||
                ::strcmp(commandname,"\"ptzctrl\"")==0)
            { 
                ret_stat = psip->ptzctrl(xml, root);
            }//ptzctrl

            //设备预置点
            if (::strcmp(commandname,"pointindexoperation")==0 ||
                ::strcmp(commandname,"\"pointindexoperation\"")==0)
            {
                ret_stat = psip->pointindexoperation(xml, root);
            }//pointindexoperation

            char fail_case[2048]="xt";
            if (ret_stat == XT_SIP_STATUS_OK)
            {				
                psip->construct_operator_response_ctx(respones_ctx, ret_stat, 200, dev_flag.seq, fail_case);
            }
            else
            {
                psip->construct_operator_response_ctx(respones_ctx, ret_stat, 401, dev_flag.seq, fail_case);
            }
        }//control
        else if (::strcmp(root_v,"Request")==0)
        {
            xtXmlNodePtr command = xml.getNode(root, "commandname");
            if (command.IsNull())
            {
                ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                break;
            }

            sipmsg_ifrmconfreq freq;
            ::memset(&freq, 0, sizeof(sipmsg_ifrmconfreq)) ;

            xtXmlNodePtr seq = xml.getNode(root, "seq");
            if (!seq.IsNull())
            {
                const char *seq_v = xml.getValue(seq);
                if (seq_v)
                { 
                    freq.seq = ::atoi(seq_v);
                    ::strcpy(dev_flag.seq,seq_v);
                }
            }

            xtXmlNodePtr devname = xml.getNode(root, "devname");
            if (!devname.IsNull())
            {
                const char *devname_v = xml.getValue(devname);
                if (devname_v)
                { 
                    ::strcpy(freq.devname, devname_v);
                    ::strcpy(dev_flag.devname,devname_v);
                }
            }

            xtXmlNodePtr devid = xml.getNode(root, "devid");
            if (!devid.IsNull())
            {
                const char *devid_v = xml.getValue(devid);
                if (devid_v)
                { 
                    ::strcpy(freq.devid, devid_v);
                    ::strcpy(dev_flag.devid,devid_v);
                }
            }

            xtXmlNodePtr groupid = xml.getNode(root, "groupid");
            if (!groupid.IsNull())
            {
                const char *groupid_v = xml.getValue(groupid);
                if (groupid_v)
                { 
                    ::strcpy(freq.groupid, groupid_v);
                    ::strcpy(dev_flag.groupid,groupid_v);
                }
            }

            const char *commandname = xml.getValue(command);

            //设置I帧间隔
            if (0 == ::strcmp(commandname,"iframeconfreq") ||
                0 == ::strcmp(commandname,"\"iframeconfreq\""))
            {
                int strmid = 0;
                xtXmlNodePtr iframedelta = xml.getNode(root, "iframedelta");
                if (iframedelta.IsNull())
                {
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }

                const char *iframedelta_v = xml.getValue(iframedelta);
                if (iframedelta_v)
                { 
                    freq.iframedelta = ::atoi(iframedelta_v);
                    ret_stat = psip->iframeconfreq(&dev_flag, freq, respones_ctx);
                }                
                else
                {
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }
            }//iframeconfreq

            //修改码率
            if (0 == ::strcmp(commandname,"coderate") ||
                0 == ::strcmp(commandname,"\"coderate\""))
            {
                int strmid = 0;
                xtXmlNodePtr coderate = xml.getNode(root, "iframedelta");
                if (coderate.IsNull())
                {					
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }

                const char *coderate_v = xml.getValue(coderate);
                if (coderate_v)
                {
                    ret_stat = psip->coderate(&dev_flag, strmid, coderate_v, respones_ctx);
                }
                else
                {
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }
            }//coderate  

            //修改分辨率
            if (0 == ::strcmp(commandname,"resolution") ||
                0 == ::strcmp(commandname,"\"resolution\""))
            {
                int strmid = 0;
                xtXmlNodePtr resolution = xml.getNode(root, "iframedelta");
                if (resolution.IsNull())
                {					
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }

                const char *resolution_v = xml.getValue(resolution);
                if (resolution_v)
                {
                    ret_stat = psip->resolution(&dev_flag, strmid, resolution_v, respones_ctx);
                }
                else
                {
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }
            }//resolution      

            //获取设备状态
            if (0 == ::strcmp(commandname,"getdeviceinforeq") ||
                0 == ::strcmp(commandname,"\"getdeviceinforeq\""))
            {
                ret_stat = psip->getdeviceinforeq(&dev_flag, respones_ctx);
            }//getdeviceinforeq

            //add by songlei 20150625
            //获取会话状态
            if (0 == ::strcmp(commandname,"sessioninforeq") ||
                0 == ::strcmp(commandname,"\"sessioninforeq\""))
            {
                ret_stat = psip->sessioninforeq(&dev_flag, respones_ctx);
            }//sessioninforeq			
        }

        if (ret_stat == XT_SIP_STATUS_OK)
        {
            ret_stat = psip->xt_sip_server_message_accept(h);
            if (XT_SIP_STATUS_OK != ret_stat)
            {
                DEBUG_LOG("on_server_message_arrived:xt_sip_server_message_accept error ret_stat[%d]\n",ret_stat);
            }
        }
        else
        {
            psip->xt_sip_server_message_reject(h);
            DEBUG_LOG("on_server_message_arrived :Err xt_sip_server_message_reject ret_stat[%d] \n",ret_stat);
        }

        xt_sip_client_message_request_t	request = { 0 };
        request.target = from.c_str();
        request.content_type = "application/command+xml";
        if (!respones_ctx.empty())
        {
            request.content = respones_ctx.c_str();
            request.content_length = respones_ctx.length();

			xt_sip_client_message_callback_t cb;
			cb.on_response = on_client_message_response;
            ret_stat = psip->xt_sip_make_client_message(psip->sip_, NULL, &request, NULL, &cb, NULL);
            if (XT_SIP_STATUS_OK != ret_stat)
            {
                DEBUG_LOG("on_server_message_arrived:xt_sip_make_client_message fail ret_stat[%d]\n",ret_stat);
            }
        }

    } while (false);

    DEBUG_LOG("on_server_message_arrived :end!\n");
}

void xt_session_sip::
on_server_invite_info(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg)
{
    DEBUG_LOG("on_server_invite_info start!\n");
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip  )
    {
        DEBUG_LOG("on_server_invite_info:psip error psip is empty!\n");
        return;
    }

    do 
    { 
        if (XT_SIP_INVALID_HANDLE == msg)
        {
            DEBUG_LOG("on_server_invite_info:msg error msg is emtpy!\n");
            break;
        }

        xt_sip_status_t ret_stat = XT_SIP_STATUS_OK; 

        psip->xt_sip_server_invite_accept_NIT(h,200,NULL, NULL, 0);

        std::string call_id;
        xt_sip_buf_handle_t cid = psip->xt_sip_msg_get_call_id(msg);
        if (XT_SIP_INVALID_HANDLE == cid)
        {
            break;
        }
        const char * data1 = static_cast<char*>(psip->xt_sip_buf_get_data(cid));
        uint32_t len_data1 = psip->xt_sip_buf_get_len(cid);
        call_id.assign(data1,len_data1);
        psip->xt_sip_buf_handle_delete(cid);

        session_info_t session;
        int ret = psip->find_sip_session(call_id, session);

        xt_sip_buf_handle_t buf = psip->xt_sip_msg_get_content_body(msg);
        if (XT_SIP_INVALID_HANDLE == buf)
        {
            break;
        }
        const char * data = static_cast<char*>(psip->xt_sip_buf_get_data(buf));
        uint32_t len_data = psip->xt_sip_buf_get_len(buf);

        if (ret==0 && psip->info_cb_)
        {
            psip->info_cb_(session.srcno, data, len_data);
        }

        xtXml xml;
        xml.LoadXMLStr(data);
        psip->xt_sip_buf_handle_delete(buf);

        xtXmlNodePtr root = xml.getRoot();
        if (root.IsNull())
        {
            break;
        }

        const char *val = root.GetName()/*xml.getValue(root)*/;
        if (::strcmp(val,"media_control")==0)
        {
            xtXmlNodePtr vc_primitive = xml.getNode(root, "vc_primitive");
            if (!vc_primitive.IsNull())
            {
                xtXmlNodePtr to_encoder = xml.getNode(vc_primitive, "to_encoder");
                if (!to_encoder.IsNull())
                {
                    //强制I帧
                    xtXmlNodePtr picture_fast_update = xml.getNode(to_encoder, "picture_fast_update");
                    if (!to_encoder.IsNull())
                    {
                        std::string call_id;
                        xt_sip_buf_handle_t cid = psip->xt_sip_msg_get_call_id(msg);
                        const char * data2 = static_cast<char*>(psip->xt_sip_buf_get_data(cid));
                        if (NULL == data2)
                        {
                            DEBUG_LOG("on_server_invite_info:cid error\n");
                            break;
                        }

                        uint32_t len_data2 = psip->xt_sip_buf_get_len(cid);
                        call_id.assign(data2,len_data2);
                        psip->xt_sip_buf_handle_delete(cid);

                        session_info_t session;
                        int ret = psip->find_sip_session(call_id, session);
                        if (ret < 0)
                        {
                            break;
                        }

                        DEBUG_LOG("on_server_invite_info:picupdate_cb_ start...\n");
                        if (psip->picupdate_cb_)
                        {
                            DEBUG_LOG("on_server_invite_info:picupdate_cb_ invited!...\n");
                            int oprcode = 0;
                            char fail_case[2048]={0};
                            psip->picupdate_cb_(session.srcno,oprcode,fail_case);
                            if (oprcode < 0 )
                            {                                
                                psip->xt_sip_server_invite_reject_NIT(h,401);
                            }
                            else
                            {
                                psip->xt_sip_server_invite_reject_NIT(h,200);
                            }                            
                        }
                    }
                }
            }
        }//picupdate

    } while (0);

    DEBUG_LOG("on_server_invite_info end!\n");
}

void xt_session_sip::
on_server_invite_info_response(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success)
{
    DEBUG_LOG("on_server_invite_info_response \n");
}


void xt_session_sip::
on_server_invite_message(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg)
{
    DEBUG_LOG("on_server_invite_message start!\n");
    xt_session_sip *psip = static_cast<xt_session_sip *>(ctx);
    if (NULL == psip)
    {
        DEBUG_LOG("on_server_message_arrived:psip error psip is empty!\n");
        return;
    }

    xt_sip_status_t ret_stat = XT_SIP_STATUS_OK;
    std::string respones_ctx;
    dev_flag_t dev_flag;

    ::memset(&dev_flag,0,sizeof(dev_flag_t));

    do
    {
        if (XT_SIP_INVALID_HANDLE == msg)
        {
            DEBUG_LOG("on_server_message_arrived:psip error msg is empty!\n");
            ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
            break;
        }

        xt_sip_buf_handle_t buf = psip->xt_sip_msg_get_content_body(msg);
        if (XT_SIP_INVALID_HANDLE == buf)
        {
            ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
            break;
        }

        const char * data = static_cast<char*>(psip->xt_sip_buf_get_data(buf));
        uint32_t len_data = psip->xt_sip_buf_get_len(buf);

        if (psip->message_cb_)
        {
            DEBUG_LOG("on_server_message_arrived :psip->message_cb_!\n");
            psip->message_cb_(data, len_data);
        }

        //获取from
        std::string from;
        xt_sip_buf_handle_t from_buf = psip->xt_sip_msg_get_from(msg);
        if (XT_SIP_INVALID_HANDLE == from_buf)
        {
            ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
            break;
        }
        from.assign((const char*)psip->xt_sip_buf_get_data(from_buf), psip->xt_sip_buf_get_len(from_buf));
        psip->xt_sip_buf_handle_delete(from_buf);

        xtXml xml;
        xml.LoadXMLStr(data);
        psip->xt_sip_buf_handle_delete(buf);

        xtXmlNodePtr root = xml.getRoot();
        if (root.IsNull())
        {
            ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
            break;
        }

        DEBUG_LOG("on_server_message_arrived :[%s]\nn",xml.GetXMLStrEx());

        const char *root_v = root.GetName();
        if (::strcmp(root_v,"Control")==0)
        {
            xtXmlNodePtr command = xml.getNode(root, "commandname");
            if (command.IsNull())
            {
                ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                break;
            }

            xtXmlNodePtr seq = xml.getNode(root, "seq");
            if (!seq.IsNull())
            {
                const char *seq_v = xml.getValue(seq);
                if (seq_v)
                { 
                    ::strcpy(dev_flag.seq,seq_v);
                }
            }

            //云镜控制
            const char *commandname = xml.getValue(command);
            if (::strcmp(commandname,"ptzctrl")==0 ||
                ::strcmp(commandname,"\"ptzctrl\"")==0)
            { 
                ret_stat = psip->ptzctrl(xml, root);
            }//ptzctrl

            //设备预置点
            if (::strcmp(commandname,"pointindexoperation")==0 ||
                ::strcmp(commandname,"\"pointindexoperation\"")==0)
            {
                ret_stat = psip->pointindexoperation(xml, root);
            }//pointindexoperation

            char fail_case[2048]="xt";
            if (ret_stat == XT_SIP_STATUS_OK)
            {				
                psip->construct_operator_response_ctx(respones_ctx, ret_stat, 200, dev_flag.seq, fail_case);
            }
            else
            {
                psip->construct_operator_response_ctx(respones_ctx, ret_stat, 401, dev_flag.seq, fail_case);
            }			
        }//control
        else if (::strcmp(root_v,"Request")==0)
        {
            xtXmlNodePtr command = xml.getNode(root, "commandname");
            if (command.IsNull())
            {
                ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                break;
            }

            sipmsg_ifrmconfreq freq;
            ::memset(&freq, 0, sizeof(sipmsg_ifrmconfreq)) ;

            xtXmlNodePtr seq = xml.getNode(root, "seq");
            if (!seq.IsNull())
            {
                const char *seq_v = xml.getValue(seq);
                if (seq_v)
                { 
                    freq.seq = ::atoi(seq_v);
                    ::strcpy(dev_flag.seq,seq_v);
                }
            }

            xtXmlNodePtr devname = xml.getNode(root, "devname");
            if (!devname.IsNull())
            {
                const char *devname_v = xml.getValue(devname);
                if (devname_v)
                { 
                    ::strcpy(freq.devname, devname_v);
                    ::strcpy(dev_flag.devname,devname_v);
                }
            }

            xtXmlNodePtr devid = xml.getNode(root, "devid");
            if (!devid.IsNull())
            {
                const char *devid_v = xml.getValue(devid);
                if (devid_v)
                { 
                    ::strcpy(freq.devid, devid_v);
                    ::strcpy(dev_flag.devid,devid_v);
                }
            }

            xtXmlNodePtr groupid = xml.getNode(root, "groupid");
            if (!groupid.IsNull())
            {
                const char *groupid_v = xml.getValue(groupid);
                if (groupid_v)
                { 
                    ::strcpy(freq.groupid, groupid_v);
                    ::strcpy(dev_flag.groupid,groupid_v);
                }
            }

            const char *commandname = xml.getValue(command);

            //设置I帧间隔
            if (0 == ::strcmp(commandname,"iframeconfreq") ||
                0 == ::strcmp(commandname,"\"iframeconfreq\""))
            {
                int strmid = 0;
                xtXmlNodePtr iframedelta = xml.getNode(root, "iframedelta");
                if (iframedelta.IsNull())
                {
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }

                const char *iframedelta_v = xml.getValue(iframedelta);
                if (iframedelta_v)
                { 
                    freq.iframedelta = ::atoi(iframedelta_v);
                    ret_stat = psip->iframeconfreq(&dev_flag, freq, respones_ctx);
                }                
                else
                {
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }
            }//iframeconfreq

            //修改码率
            if (0 == ::strcmp(commandname,"coderate") ||
                0 == ::strcmp(commandname,"\"coderate\""))
            {
                int strmid = 0;
                xtXmlNodePtr coderate = xml.getNode(root, "iframedelta");
                if (coderate.IsNull())
                {					
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }

                const char *coderate_v = xml.getValue(coderate);
                if (coderate_v)
                {
                    ret_stat = psip->coderate(&dev_flag, strmid, coderate_v, respones_ctx);
                }
                else
                {
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }
            }//coderate  

            //修改分辨率
            if (0 == ::strcmp(commandname,"resolution") ||
                0 == ::strcmp(commandname,"\"resolution\""))
            {
                int strmid = 0;
                xtXmlNodePtr resolution = xml.getNode(root, "iframedelta");
                if (resolution.IsNull())
                {					
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }

                const char *resolution_v = xml.getValue(resolution);
                if (resolution_v)
                {
                    ret_stat = psip->resolution(&dev_flag, strmid, resolution_v, respones_ctx);
                }
                else
                {
                    ret_stat = XT_SIP_STATUS_ERR_BAD_PARAM;
                    break;
                }
            }//resolution      

            //获取设备状态
            if (0 == ::strcmp(commandname,"getdeviceinforeq") ||
                0 == ::strcmp(commandname,"\"getdeviceinforeq\""))
            {
                ret_stat = psip->getdeviceinforeq(&dev_flag, respones_ctx);
            }//getdeviceinforeq

            //add by songlei 20150625
            //获取会话状态
            if (0 == ::strcmp(commandname,"sessioninforeq") ||
                0 == ::strcmp(commandname,"\"sessioninforeq\""))
            {
                ret_stat = psip->sessioninforeq(&dev_flag, respones_ctx);
            }//sessioninforeq
        }

        if (ret_stat == XT_SIP_STATUS_OK)
        {
            ret_stat = psip->xt_sip_server_invite_accept_NIT(h, 200, "application/command+xml", "", 0);
            if (XT_SIP_STATUS_OK != ret_stat)
            {
                DEBUG_LOG("on_server_message_arrived:xt_sip_server_message_accept error ret_stat[%d]\n",ret_stat);
            }
        }
        else
        {
            psip->xt_sip_server_invite_reject_NIT(h, 401);
            DEBUG_LOG("on_server_message_arrived :Err xt_sip_server_message_reject ret_stat[%d] \n",ret_stat);
        }

        xt_sip_client_message_request_t	request = { 0 };
        request.target = from.c_str();
        request.content_type = "application/command+xml";
        if (!respones_ctx.empty())
        {
            request.content = respones_ctx.c_str();
            request.content_length = respones_ctx.length();
            ret_stat = psip->xt_sip_client_invite_message(h, request.content_type, request.content, request.content_length);
            if (XT_SIP_STATUS_OK != ret_stat)
            {
                DEBUG_LOG("on_server_message_arrived:xt_sip_make_client_message fail ret_stat[%d]\n",ret_stat);
            }
        }

    } while (false);

    DEBUG_LOG("on_server_invite_message :end!\n");
}
void xt_session_sip::
on_server_invite_message_response(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success)
{
    DEBUG_LOG("on_server_invite_message_response \n");
}

int xt_session_sip::get_strmid(const char *sdp, unsigned short len)
{
    int streamid = -1;
    xt_sdp::parse_buffer_t pb(sdp, len); 
    xt_sdp::sdp_session_t xsdp;
    try
    {
        xsdp.parse(pb);
    }
    catch(...)
    {
        return -1;
    }
    const std::list<std::string>& vals = xsdp.attribute_helper_.get_values("streamid");
    if (vals.size() > 0)
    {
        std::list<std::string>::const_iterator itr = vals.begin();
        streamid = ::atoi((*itr).c_str());
    }
    return streamid;
}

int xt_session_sip::get_media_info(const char *sdp, unsigned short len, session_info_t &session)
{
    if ( NULL == sdp || 0 == ::strlen(sdp))
    {
        DEBUG_LOG("get_media_info err sdp is empty!\n");
        return -1;
    }
    xt_sdp::parse_buffer_t pb(sdp, len); 
    xt_sdp::sdp_session_t xsdp;
    try
    {
        xsdp.parse(pb);
    }
    catch(...)
    {
        return -1;
    }

    bool demux = false;
    unsigned int demuxid = 0;
    std::list<std::string> lst_vale;
    if (xsdp.attribute_helper_.exists("rtpport-mux") && xsdp.attribute_helper_.exists("muxid"))
    {
        lst_vale = xsdp.attribute_helper_.get_values("muxid");
        if (!lst_vale.empty())
        {
            demux = true;
            std::string str_mux = lst_vale.front();
            lst_vale.clear();
            demuxid = ::atoi(str_mux.c_str());
        }
    }

    sdp_session_t::medium_container_t &media = xsdp.media_;
    sdp_session_t::medium_container_t::iterator itr = media.begin();
    for (int i=0;itr!=media.end()&&i<MAX_TRACK;++itr,++i)
    {
        if (itr->exists("rtpport-mux") || xsdp.attribute_helper_.exists("rtpport-mux"))
        {
            lst_vale = itr->get_values("muxid");
            if (!lst_vale.empty())
            {
                demux = true;
                std::string str_mux = lst_vale.front();
                lst_vale.clear();
                demuxid = ::atoi(str_mux.c_str());
            }
            DEBUG_LOG("get_media_info:Media:demux[%d] demuxid[%u]\n",demux,demuxid);
        }
        else
        {
            demux = false;
            demuxid = 0; 
        }

        sdp_session_t::medium_t &medium = *itr;
        std::string ip1 = xsdp.connection_.address().c_str();
        std::string ip2 = xsdp.origin_.address().c_str();
        std::string ip3 = "";
        std::string ip4 = "";
        if (medium.connections_.size() > 0)
        {
            ip3 = (*(medium.connections_.begin())).address();
        }
        if (medium.session_)
        {
            ip4 = medium.session_->connection_.address();
        }

        std::string ip_session = ip3;
        if (ip3.length() > 0)
        {
            ip_session = ip3;
        }
        else if (ip4.length() > 0)
        {
            ip_session = ip4;
        }
        else if (ip1.length() > 0)
        {
            ip_session = ip1;
        }
        else if (ip2.length() > 0)
        {
            ip_session = ip2;
        }

        track_info_t track;
        ::strcpy(track.trackname, medium.name().c_str());
        ::strcpy(track.ip, ip_session.c_str());
        track.port = medium.port();
        track.demux = demux;
        track.demuxid = demuxid;        
        track.multicast = medium.multicast();
        session.tracks.push_back(track);
    }

    session.username = xsdp.origin_.user();
    session.ip = xsdp.connection_.address(); 
    session.sessionid = xsdp.origin_.session_id();
    return 0;
}

void xt_session_sip::set_sdp(unsigned short index, const char *sdp, unsigned short len)
{
    if (index < NUM_STREAM && sdp && len < LEN_SDP && start_)
    {
        ::memset(sdp_[index], 0, LEN_SDP);
        ::memcpy(&sdp_[index], sdp, len);
        DEBUG_LOG("xt_session_sip::set_sdp index[%d] sdp[%s] len[%d]\n",index,sdp,len);
    }
}

void xt_session_sip::set_sdp_full(unsigned short index, const char *sdp, unsigned short len)
{
    if (index < NUM_STREAM && sdp && len < LEN_SDP && start_)
    {
        ::memset(sdp_full_[index], 0, LEN_SDP);
        ::memcpy(sdp_full_[index], sdp, len);
        DEBUG_LOG("xt_session_sip::set_sdp_full index[%d] sdp[%s] len[%d]\n",index,sdp,len);
    }
}

bool xt_session_sip::is_exist_sip_session(const session_info_t &session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_info_mutex_);
    if (session_infos_.find(session.call_id)!=session_infos_.end())
    {
        return true;
    }

    return false;
}

void xt_session_sip::add_sip_session(const session_info_t &session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_info_mutex_);

    if (session_infos_.find(session.call_id)!=session_infos_.end())
    {
        session_infos_.erase(session.call_id);
    }
    session_infos_[session.call_id] = session;
}

void xt_session_sip::del_sip_session(std::string call_id)
{
    boost::unique_lock<boost::shared_mutex> lock(session_info_mutex_);
    session_infos_.erase(call_id);
}

int xt_session_sip::find_sip_session(std::string call_id, session_info_t &session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_info_mutex_);

    if (session_infos_.find(call_id) == session_infos_.end())
    {
        return -1;
    }
    session = session_infos_[call_id];

    return 0;
}

int xt_session_sip::get_sip_session(session_info_t& out_session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_info_mutex_);
    if (session_infos_.empty())
    {
        return -1;
    }

    std::map<std::string,session_info_t>::iterator itr = session_infos_.begin();
    for (;session_infos_.end() != itr; ++itr)
    {
        out_session = itr->second;
    }

    return 0;
}

int xt_session_sip::find_sip_session_multicast(const track_info_t &track)
{
    boost::unique_lock<boost::shared_mutex> lock(session_info_mutex_);

    int find = 0;

    std::map<std::string,session_info_t>::iterator itr = session_infos_.begin();
    for (;itr!=session_infos_.end();++itr)
    {
        session_info_t &session = itr->second;
        for (std::size_t i=0;i<session.tracks.size();++i)
        {
            track_info_t &t = session.tracks[i];
            if (t.multicast && 
                t.trackname == track.trackname&&
                t.ip == track.ip && 
                t.port == track.port &&
                t.demux == track.demux &&
                t.demuxid == track.demuxid)
            {
                find += 1;
            }
        }
    }

    return find;
}

std::string xt_session_sip::get_medium_desc(std::string format, sdp_session_t::medium_t &medium)
{
    std::string key = "rtpmap";
    std::string desc = "";
    std::map<std::string, std::list<std::string> > &m = medium.attribute_helper_.attributes_;
    if (m.find(key) == m.end())
    {
        return desc;
    }

    std::list<std::string> &d = m[key];
    std::list<std::string>::iterator itr = d.begin();
    for (;itr!=d.end();++itr)
    {
        std::string &f = *itr;
        std::string t1,t2;
        std::string::size_type p = f.find_first_not_of(' ');
        f = f.substr(p);

        p = f.find_first_of(' ');
        t1 = f.substr(0, p);
        t2 = f.substr(p);

        if (t1 == format)
        {
            desc = t2;
            break;
        }
    }

    return desc;
}
void xt_session_sip::update_medium_desc_recv(sdp_session_t::medium_t &medium, std::string format1, std::string format2)
{
    std::string key1 = "rtpmap";
    std::string key2 = "fmtp";
    std::list<std::pair<std::string, std::string> > &m = medium.attribute_helper_.attribute_list_;
    std::list<std::pair<std::string, std::string> >::iterator itr = m.begin();
    for (;itr!=m.end();++itr)
    {
        std::pair<std::string, std::string> &v = *itr;
        if (v.first==key1 || v.first==key2)
        {
            std::string &f = v.second;
            std::string t1,t2;
            std::string::size_type p = f.find_first_not_of(' ');
            f = f.substr(p);

            p = f.find_first_of(' ');
            t1 = f.substr(0, p);
            t2 = f.substr(p);

            if (t1 == format1)
            {
                f = format2 + t2;
            }
        }
    }

    return;
}
void xt_session_sip::update_medium_desc_snd(sdp_session_t::medium_t &medium, std::string format1, std::string format2)
{
    std::string key1 = "rtpmap";
    std::string key2 = "fmtp";
    std::map<std::string, std::list<std::string> > &m = medium.attribute_helper_.attributes_;
    if (m.find(key1) != m.end())
    {
        std::list<std::string> &d = m[key1];
        std::list<std::string>::iterator itr = d.begin();
        for (;itr!=d.end();++itr)
        {
            std::string &f = *itr;
            std::string t1,t2;
            std::string::size_type p = f.find_first_not_of(' ');
            f = f.substr(p);

            p = f.find_first_of(' ');
            t1 = f.substr(0, p);
            t2 = f.substr(p);

            if (t1 == format1)
            {
                f = format2 + " " + t2;
                break;
            }
        }
    }

    if (m.find(key2) != m.end())
    {
        std::list<std::string> &d = m[key2];
        std::list<std::string>::iterator itr = d.begin();
        for (;itr!=d.end();++itr)
        {
            std::string &f = *itr;
            std::string t1,t2;
            std::string::size_type p = f.find_first_not_of(' ');
            f = f.substr(p);

            p = f.find_first_of(' ');
            t1 = f.substr(0, p);
            t2 = f.substr(p);

            if (t1 == format1)
            {
                f = format2 + " " + t2;
                break;
            }
        }
    }

    return;
}

bool xt_session_sip::desc_compare(const std::string& desc1,const std::string& desc2)
{
    bool ret_code = false;
    do 
    {
        if (desc1.length() < 0)
        {
            break;
        }

        std::string d1 = desc1;
        std::string d2 = desc2;
        std::transform(desc2.begin(), desc2.end(), d2.begin(), ::toupper);
        std::transform(desc1.begin(), desc1.end(), d1.begin(), ::toupper);

        std::string::size_type pos = d2.find(d1);
        if (pos != std::string::npos)   
        {
            DEBUG_LOG("medium compare success:desc recv[%s] send[%s]\n", d1.c_str(), d2.c_str());
            ret_code = true;
            break;
        }
        else
        {
            DEBUG_LOG("medium compare fail:desc recv[%s] send[%s]\n", d1.c_str(), d2.c_str());
            ret_code = false;
            break;
        }

    } while (0);

    return ret_code;
}

bool xt_session_sip::medium_compare(sdp_session_t::medium_t &medium_recv, sdp_session_t::medium_t &medium_snd, int &payload)
{
    bool ret = false;

    const std::list<std::string>& formats_recv = medium_recv.formats();
    std::list<std::string>::const_iterator itf_recv = formats_recv.begin();
    for (;itf_recv!=formats_recv.end();++itf_recv)
    {
        std::string f_recv = *itf_recv;
        int payload_recv = ::atoi(f_recv.c_str());
        std::string desc_recv = "";
        // if (payload_recv >= 96)
        {
            desc_recv = get_medium_desc(*itf_recv, medium_recv);
        }

        const std::list<std::string>& formats_snd = medium_snd.formats();
        std::list<std::string>::const_iterator itf_snd = formats_snd.begin();
        for (;itf_snd!=formats_snd.end();++itf_snd)
        {
            std::string f_snd = *itf_snd;
            int payload_snd = ::atoi(f_snd.c_str());
            std::string desc_snd = "";
            //if (payload_snd >= 96)
            {
                desc_snd = get_medium_desc(f_snd, medium_snd);
            }

            bool find_fmt = false;
            if (payload_recv>=96 || payload_snd>=96)
            {
                find_fmt = desc_compare(desc_recv,desc_snd);
            }
            else if(payload_recv == payload_snd)
            {
                //find_fmt = true;
                find_fmt = desc_compare(desc_recv,desc_snd);
            }
            else
            {
                DEBUG_LOG("medium compare fail:desc recv[%s] send[%s]\n", desc_recv.c_str(), desc_snd.c_str());
            }

            if (find_fmt)
            {
                ret = true;

                payload = payload_recv;

                medium_recv.formats_.clear();
                medium_recv.formats_.push_back(f_recv);

                medium_recv.attribute_helper_.clear();
                medium_recv.attribute_helper_ = medium_snd.attribute_helper_;

                update_medium_desc_recv(medium_recv, f_snd, f_recv);
                update_medium_desc_snd(medium_snd, f_snd, f_recv);

                break;
            } 
        }

        if (ret)
        {
            break;
        }
    }
    return ret;
}

//add by songlei 20150714
const bool xt_session_sip::rtpmap_compare(sdp_session_t::medium_t &medium_recv, sdp_session_t::medium_t &medium_snd, int &payload) const
{
    bool ret = false;
    do 
    {
        std::map<std::string, std::list<std::string> >::iterator itr_rtpmap_snd_lst = medium_snd.attribute_helper_.attributes_.find("rtpmap");
        if (medium_snd.attribute_helper_.attributes_.end() == itr_rtpmap_snd_lst)
        {
            break;
        }

        std::map<std::string, std::list<std::string> >::iterator itr_rtpmap_recv_lst = medium_recv.attribute_helper_.attributes_.find("rtpmap");
        if (medium_recv.attribute_helper_.attributes_.end() == itr_rtpmap_recv_lst)
        {
            break;
        }

        std::list<std::string>::iterator itr_rtpmap_snd = itr_rtpmap_snd_lst->second.begin();
        for(;itr_rtpmap_snd_lst->second.end() != itr_rtpmap_snd; ++itr_rtpmap_snd)
        {
            rtpmap_value_t rtpmap_snd;
            if (!parse_sdp_attribute_rtpmap_value(itr_rtpmap_snd->c_str(),rtpmap_snd))
            {
                continue;
            }
            bool find_rtpmap = false;
            std::list<std::string>::iterator itr_rtpmap_recv = itr_rtpmap_recv_lst->second.begin();
            for(;itr_rtpmap_recv_lst->second.end() != itr_rtpmap_recv; ++itr_rtpmap_recv)
            {
                rtpmap_value_t rtpmap_recv;
                if (!parse_sdp_attribute_rtpmap_value(itr_rtpmap_recv->c_str(),rtpmap_recv))
                {
                    continue;
                } 
                if (  0 == ::strcmp(rtpmap_snd.codec_name,rtpmap_recv.codec_name)
                    && rtpmap_snd.rtp_timestamp_frequency == rtpmap_recv.rtp_timestamp_frequency
                    && rtpmap_snd.num_channels == rtpmap_recv.num_channels)
                {
                    find_rtpmap = true;
                    payload = rtpmap_recv.rtpmap_payload_format;
                    break;
                }
            }

            if (find_rtpmap)
            {
                ret = true;
                medium_recv.formats_.clear();
                std::ostringstream os1;
                os1<<payload;
                medium_recv.add_format(os1.str());
                medium_recv.attribute_helper_.clear();
                medium_recv.attribute_helper_ = medium_snd.attribute_helper_;

                std::list<std::string> lst_vale;
                //更新rtpmap 与 fmtp的负载类型
                if ( medium_recv.attribute_helper_.exists("rtpmap"))
                {
                    lst_vale = medium_recv.attribute_helper_.get_values("rtpmap");
                    if (!lst_vale.empty())
                    {
                        std::string rtpmap_v = lst_vale.front();
                        lst_vale.clear();
                        if (!rtpmap_v.empty())
                        {
                            std::string::size_type pos =rtpmap_v.find(" ");
                            if (std::string::npos != pos)
                            {
                                rtpmap_v.replace(0,pos,os1.str());
                            }
                            medium_recv.clear_attribute("rtpmap");
                            medium_recv.add_attribute("rtpmap",rtpmap_v);
                        }
                    }

                }
                if ( medium_recv.attribute_helper_.exists("fmtp"))
                {
                    lst_vale = medium_recv.attribute_helper_.get_values("fmtp");
                    if (!lst_vale.empty())
                    {
                        std::string fmtp_v = lst_vale.front();
                        if (!fmtp_v.empty())
                        {
                            std::string::size_type pos =fmtp_v.find(" ");
                            if (std::string::npos != pos)
                            {
                                fmtp_v.replace(0,pos,os1.str());
                            }
                            medium_recv.clear_attribute("fmtp");
                            medium_recv.add_attribute("fmtp",fmtp_v);
                        }
                    }
                }

                break;
            }
        }

    } while (0);

    return ret;

}
/************************************************************************/
/*add by qinchao 2015.09.29
//! 重启rtp服务（该接口用于在动态通过INVITE更新复用标识时才使用）
//! \returns 成功返回0，失败返回-1
*/
/************************************************************************/
int xt_session_sip::rtp_server_restart(void)
{
    int ret = XTRtp::instance()->uninit();
    if(ret < 0)
    {
        printf("XTRtp::instance()->uninit failed!!!.....\n");
    }

    unsigned int num_ch2 = 0;
    unsigned int num_std = 0;

    if (ms_cfg.sink_single)
    {
        num_ch2 = 2*ms_cfg.num_chan;
    }
    if (ms_cfg.snd_std_rtp)
    {
        num_std = 2*ms_cfg.num_chan;
    }
    unsigned long rtp_chan_num = ms_cfg.num_chan+num_ch2+num_std;

    ret = XTRtp::instance()->init(rtp_chan_num, ms_cfg.ip, ms_cfg.snd_start_port, ms_cfg.demux, ms_cfg.sink_single);
    if(ret == 0)
    {
        DEBUG_LOG("xt_session_sip::rtp_server_restart successful!...\n");
    }

    return ret;
}

/************************************************************************/
/*add by qinchao 2015.10.08
//! 函数用于将SDP解析出来到sdp_session_t中
//! \param [*sdp] 接传入的SDP数组
//! \param [len_rcv]  SDP长度

//! \param [&xsdp] 解析出的SDP
//! \returns          成功返回0，失败返回负数
*/
/************************************************************************/
int xt_session_sip::sdp_parse(const char *sdp, uint32_t len, xt_sdp::sdp_session_t &xsdp)
{
    int ret_code = 0;
    do
    {
        if (NULL == sdp)
        {
            ret_code = -1;
            break;
        }
        //前置条件无SDP直接返回错误
        if (::strlen(sdp) < 1 || len <= 0 )
        {
            ret_code = -1;
            break;
        }

        xt_sdp::parse_buffer_t pb_sdp(sdp, len); 

        try
        {
            xsdp.parse(pb_sdp);
        }
        catch(...)
        {
            ret_code = -3;
            break;
        }
    } while (false);

    return ret_code;
}

/************************************************************************/
/*add by qinchao 2015.10.09
//! 将SESSION里的带宽信息取出通过回调传递给编码器
//! \param [&xsdp] session对象
//! \returns       成功返回0，失败返回负数
*/
/************************************************************************/
int xt_session_sip::video_bandwidth_set(xt_sdp::sdp_session_t &xsdp)
{
    std::list<sdp_session_t::bandwidth_t> ls_value;
    std::string s_modifier;
    unsigned long s_kbs=0;

    ls_value = xsdp.bandwidths();

    if(!ls_value.empty())
    {
        s_modifier = ls_value.front().modifier();
    }

    if(strcmp(s_modifier.c_str(), "AS")==0)
    {
        s_kbs = (ls_value.front().kbs()-64)*1024;
    }
    else if(strcmp(s_modifier.c_str(), "TIAS")==0)
    {
        s_kbs = ls_value.front().kbs() - 64*1024;
    }


    sdp_session_t::medium_container_t &media_rcv = xsdp.media_;
    sdp_session_t::medium_container_t::iterator itr_rcv = media_rcv.begin();

    //遍历SDP消息体中m字段块的每一个媒体信息，如果有复用标识，is_recv_mux置为true
    for (itr_rcv = media_rcv.begin();itr_rcv != media_rcv.end();++itr_rcv)
    {
        sdp_session_t::medium_t &medium_rcv = *itr_rcv;
        if(strcmp(medium_rcv.name().c_str(), "video") == 0)
        {
            ls_value = medium_rcv.bandwidths();
            if(!ls_value.empty())
            {
                s_modifier = ls_value.front().modifier();
            }
            if(strcmp(s_modifier.c_str(), "TIAS")==0)
            {
                s_kbs = ls_value.front().kbs();
            }
            else if(strcmp(s_modifier.c_str(), "AS")==0)
            {
                s_kbs = ls_value.front().kbs()*1024;
            }
        }
    }

    DEBUG_LOG("video_bandwidth_set : TIAS:[%u].......\n",s_kbs);
    if(sipmsg_bandwidth_cb_ && (s_kbs !=0))
    {
        sipmsg_bandwidth_cb_(s_kbs);
    }

    ls_value.clear();

    return 0;
}

/************************************************************************/
/*add by qinchao 2015.10.09
//! 将SESSION里的fmtp信息取出通过回调传递给编码器
//! \param [&xsdp] session对象
//! \returns       成功返回0，失败返回负数
*/
/************************************************************************/
int xt_session_sip::video_fmtp_set(xt_sdp::sdp_session_t &xsdp)
{
    sdp_session_t::medium_container_t &media_rcv = xsdp.media_;
    sdp_session_t::medium_container_t::iterator itr_rcv;
    std::list<std::string> lst_vale;
    std::string recv_fmtp;

    //遍历SDP消息体中m字段块的每一个媒体信息，如果有复用标识，is_recv_mux置为true
    for (itr_rcv = media_rcv.begin();itr_rcv != media_rcv.end(); ++itr_rcv)
    {
        sdp_session_t::medium_t &medium_rcv = *itr_rcv;
        if(strcmp(medium_rcv.name().c_str(), "video") == 0)
        {
            lst_vale = medium_rcv.attribute_helper_.get_values("fmtp");
            if (!lst_vale.empty())
            {
                recv_fmtp = lst_vale.front();
            }
        }
    }

    DEBUG_LOG("video_fmtp_set:%s........\n",recv_fmtp.c_str());
    if(sipmsg_profile_cb_ && (!recv_fmtp.empty()))
    {
        sipmsg_profile_cb_(recv_fmtp.c_str());
    }

    lst_vale.clear();

    return 0;

}


/************************************************************************/
/*add by qinchao 2015.09.29
//! 新增功能通过INVITE动态更改设备复用标识
//! \param [*sdp_rcv] 接收到的SDP（呼叫或者点播方的SDP）
//! \param [len_rcv]  SDP长度
//! \param [*sdp_snd] 发送端的SDP（标识设备自己的SDP）
//! \param [len_snd]  SDP长度
//! \param [&session] 会话结构体，暂时没有使用
//! \returns          0:更改复用标识成功   1:双方复用标识相同，无需更改复用标识。 负数：失败
*/
/************************************************************************/
int xt_session_sip::update_rtpport_mux(const char *sdp_rcv, uint32_t len_rcv, const char *sdp_snd, uint32_t len_snd, session_info_t &session)
{
    DEBUG_LOG("update_rtpport_mux start...\n");
    int ret_code = -5;
    do 
    {
        if (NULL == sdp_rcv || NULL == sdp_snd)
        {
            ret_code = -1;
            break;
        }
        //前置条件无SDP直接返回错误
        if (::strlen(sdp_rcv) < 1 || len_rcv <= 0 )
        {
            DEBUG_LOG("sdp_compare fail recv sdp is empty len_rcv[%d]!\n",len_rcv);
            ret_code = -1;
            break;
        }
        if (::strlen(sdp_snd) < 1 || len_snd <= 0)
        {
            DEBUG_LOG("sdp_compare fail send sdp is empty len_snd[%d]!\n",len_snd);
            ret_code = -2;
            break;
        }

        xt_sdp::parse_buffer_t pb_rcv(sdp_rcv, len_rcv); 
        //xt_sdp::parse_buffer_t pb_snd(sdp_snd, len_snd); 
        xt_sdp::sdp_session_t xsdp_rcv;
        //xt_sdp::sdp_session_t xsdp_snd;

        try
        {
            xsdp_rcv.parse(pb_rcv);
            //xsdp_snd.parse(pb_snd);
        }
        catch(...)
        {
            ret_code = -3;
            break;
        }

        bool is_recv_mux = false;
        bool is_snd_mux = false;
        unsigned int recv_demuxid = 0;

        //判断SDP会话消息头中是否带有包含复用标识，如果有is_recv_mux置为true
        if ( xsdp_rcv.attribute_helper_.exists("rtpport-mux") && xsdp_rcv.attribute_helper_.exists("muxid"))
        {
            is_recv_mux = true;
        }

        sdp_session_t::medium_container_t &media_rcv = xsdp_rcv.media_;
        sdp_session_t::medium_container_t::iterator itr_rcv;
        //遍历SDP消息体中m字段块的每一个媒体信息，如果有复用标识，is_recv_mux置为true
        for (itr_rcv = media_rcv.begin();itr_rcv != media_rcv.end(); ++itr_rcv)
        {
            sdp_session_t::medium_t &medium_rcv = *itr_rcv;
            if ( ( xsdp_rcv.attribute_helper_.exists("rtpport-mux") || medium_rcv.attribute_helper_.exists("rtpport-mux") ) && 
                medium_rcv.attribute_helper_.exists("muxid") )
            {
                is_recv_mux = true;
            }
        }

        //从配置里面取出发送端（即设备自己）复用变量
        is_snd_mux = ms_cfg.demux;

        //如果接收端和发送端复用方式不相等，通过重启rtp服务，并更改复用方式与接收端SDP复用方式一致
        if(is_recv_mux != is_snd_mux)
        {
            ms_cfg.demux = is_recv_mux;

            //重启rtp服务
            int ret = rtp_server_restart();
            if (ret < 0)
            {
                DEBUG_LOG("rtp_server_restart fail!!!\n");
                ret_code = -4;
            }
            else
            {
                ret_code = 0;
            }
        }
        //接收端SDP里的复用标识，与发送端复用标识一样，直接跳出，不再处理SDP
        else
        {
            ret_code = 1;
            break;
        }

    } while (false);

    return ret_code;

}

int xt_session_sip::sdp_compare(const char *sdp_rcv, uint32_t len_rcv, const char *sdp_snd, uint32_t len_snd, std::string &sdp, session_info_t &session)
{
    DEBUG_LOG("sdp_compare start...\n");

    printf("sdp_snd--------------\n%s-----------------------\n", sdp_snd);
    printf("sdp_rcv--------------\n%s-----------------------\n", sdp_rcv);

    int ret_code = -5;
    do 
    {
        //清空输出参数
        sdp.clear();
        if (NULL == sdp_rcv || NULL == sdp_snd)
        {
            ret_code = -1;
            break;
        }
        //前置条件无SDP直接返回错误
        if (::strlen(sdp_rcv) < 1 || len_rcv <= 0 )
        {
            DEBUG_LOG("sdp_compare fail recv sdp is empty len_rcv[%d]!\n",len_rcv);
            ret_code = -1;
            break;
        }
        if (::strlen(sdp_snd) < 1 || len_snd <= 0)
        {
            DEBUG_LOG("sdp_compare fail send sdp is empty len_snd[%d]!\n",len_snd);
            ret_code = -2;
            break;
        }

        xt_sdp::parse_buffer_t pb_rcv(sdp_rcv, len_rcv); 
        xt_sdp::parse_buffer_t pb_snd(sdp_snd, len_snd); 
        xt_sdp::sdp_session_t xsdp_rcv;
        xt_sdp::sdp_session_t xsdp_snd;

        try
        {
            xsdp_rcv.parse(pb_rcv);
            xsdp_snd.parse(pb_snd);
        }
        catch(const std::exception& e)
        {
            printf("sdp parsed failed.%s\n", e.what());
            printf("len[%d] sdp[%s]\n", len_snd, sdp_snd);
      
            ret_code = -3;
            break;
        }

        xt_sdp::sdp_session_t xsdp_response;
        xsdp_response = xsdp_snd;
        xsdp_response.media_.clear();

        //头域
        std::list<std::string> lst_vale;
        bool is_recv_mux = false;
        unsigned int recv_demuxid = 0;
        if ( xsdp_rcv.attribute_helper_.exists("rtpport-mux") && xsdp_rcv.attribute_helper_.exists("muxid"))
        {
            lst_vale = xsdp_rcv.attribute_helper_.get_values("muxid");
            if (!lst_vale.empty())
            {
                std::string recv_dumxid_v = lst_vale.front();
                lst_vale.clear();
                if (!recv_dumxid_v.empty())
                {
                    recv_demuxid = ::atoi(recv_dumxid_v.c_str());
                }
            }
            xsdp_rcv.attribute_helper_.clear_attribute("muxid");
            is_recv_mux = true;
        }

        /*
        *本地管理组播地址：
        *239.0.0.0-239.255.255.255
        *用户可用的组播地址(临时组地址);
        *224.0.2.0-238.255.255.255
        */
        std::string rcv_ip;
        bool is_multicast = false;
        DEBUG_LOG("sdp_compare: chaeck multcast!\n");
        if (xt_sdp::ipv4 == xsdp_rcv.connection_.address_type())
        {
            std::string cip = xsdp_rcv.connection_.address();

            if (!cip.empty())
            {
                rcv_ip = cip;
                DEBUG_LOG("sdp_compare: chaeck multcast cip[%s]!\n",cip.c_str());
                if (is_multicast_ipv4(cip))
                {
                    is_multicast = true;
                    xsdp_response.connection_ = xsdp_rcv.connection_;
                    DEBUG_LOG("sdp_compare: multcast cip[%s] is multcast!\n",cip.c_str());
                }
            }
            else
            {
                DEBUG_LOG("std_compare: in sdp the c of session is empty use c of m!\n");
            }
        }

        session.username = xsdp_rcv.origin_.user();
        session.ip = xsdp_rcv.connection_.address(); 
        session.sessionid = xsdp_rcv.origin_.session_id();

        sdp_session_t::medium_container_t &media_rcv = xsdp_rcv.media_;
        sdp_session_t::medium_container_t &media_snd = xsdp_snd.media_;
        sdp_session_t::medium_container_t::iterator itr_snd;
        for (itr_snd = media_snd.begin(); media_snd.end()!=itr_snd; ++itr_snd)
        {
            sdp_session_t::medium_t &medium_snd = *itr_snd;
            std::string name_snd = medium_snd.name();

            sdp_session_t::medium_container_t::iterator itr_rcv;
            for (itr_rcv = media_rcv.begin(); itr_rcv != media_rcv.end(); ++itr_rcv)
            {
                sdp_session_t::medium_t &medium_rcv = *itr_rcv;                
                std::string name_rcv = medium_rcv.name();

                if (name_rcv == name_snd)
                {
                    do
                    {
                        //ZB规范
                        //标识应使用非激活模式。在一个非激活的媒体流中，没有媒体数据被发送。可以在会话或媒体层使用。
                        medium_rcv.attribute_helper_.clear_attribute("sendrecv");
                        medium_rcv.attribute_helper_.clear_attribute("recvonly");
                        medium_rcv.attribute_helper_.clear_attribute("sendonly");
                        if (!medium_rcv.attribute_helper_.exists("inactive"))
                        {
                            medium_rcv.attribute_helper_.add_attribute("inactive");
                        }

                        std::list<xt_sdp::sdp_session_t::connection_t> &conns = medium_rcv.get_medium_connections();
                        std::list<xt_sdp::sdp_session_t::connection_t>::iterator itr_c;
                        for (itr_c = conns.begin(); itr_c!=conns.end(); ++itr_c)
                        {
                            xt_sdp::sdp_session_t::connection_t &c = *itr_c;
                            if (xt_sdp::ipv4 == c.address_type())
                            {
                                rcv_ip = c.address();
                                if (!is_multicast_ipv4(c.address()))
                                {
                                    c.set_address(ms_cfg.ip);
                                }
                                else
                                {
                                    is_multicast = true;
                                }
                            }
                        }

                        //*ZB规范要求
                        //*RTP端口复用属性，用于标识RTP端口复用机制。
                        //*请求方必须在会话层携带该属性以有流复用标识属性表示能够支持H.460.19定义的RTP
                        //*端口复用机制，响应方必须在响应消息中确认是否采用该机制。若响应方在会话层也带有
                        //*端口复用属性(rtpport-mux)和流复用标识(muxid)，则表示会话应采用RTP端口复用机制进行传输
                        //*在RTP端口复用情况下，音频码流、视频码流的复用ID(muxid)应由会话发起方和会话响应方
                        //*产生，SDP请求或应答的音、视频媒体描述应采用相同的媒体端口。
                        if ( ( xsdp_rcv.attribute_helper_.exists("rtpport-mux") || medium_rcv.attribute_helper_.exists("rtpport-mux") )
                            && medium_rcv.attribute_helper_.exists("muxid") )
                        {
                            is_recv_mux = true;
                            lst_vale = medium_rcv.attribute_helper_.get_values("muxid");                      
                            if (!lst_vale.empty())
                            {
                                std::string recv_dumxid_v = lst_vale.front();
                                lst_vale.clear();
                                if (!recv_dumxid_v.empty())
                                {
                                    recv_demuxid = ::atoi(recv_dumxid_v.c_str());
                                }
                            }
                        }
                        if ( ( xsdp_snd.attribute_helper_.exists("rtpport-mux") || medium_snd.attribute_helper_.exists("rtpport-mux") ) 
                            && medium_snd.attribute_helper_.exists("muxid") )
                        {
                            //ZB规范复用标识要加在session里
                            if (!xsdp_response.attribute_helper_.exists("rtpport-mux"))
                            {
                                xsdp_response.attribute_helper_.add_attribute("rtpport-mux");
                            }
                        }

                        int payload = 96;
                        if (!rtpmap_compare(medium_rcv, medium_snd, payload))
                        {
							xsdp_response.media_.push_back(medium_rcv);
                            break;
                        }
                        else
                        {
                            ret_code = 1;
                            //////////////////////////////////////////////////////////////
                            track_info_t track;
                            ::strncpy(track.trackname, name_rcv.c_str(),LEN_NODE);
                            ::strncpy(track.ip, rcv_ip.c_str(),LEN_NODE);
                            track.port = medium_rcv.port();
                            track.demux = is_recv_mux;
                            track.demuxid = recv_demuxid;
                            track.payload = payload;
                            track.multicast = is_multicast;
                            session.tracks.push_back(track);
                            //////////////////////////////////////////////////////////////

                            if (!is_multicast)
                            {
                                medium_rcv.port_ = medium_snd.port_;
                            }
                            medium_rcv.attribute_helper_.clear_attribute("sendrecv");
                            medium_rcv.attribute_helper_.clear_attribute("recvonly");
                            medium_rcv.attribute_helper_.clear_attribute("inactive");
                            if (!medium_rcv.attribute_helper_.exists("sendonly"))
                            {
                                medium_rcv.attribute_helper_.add_attribute("sendonly");
                            }

							// 匹配成功
							xsdp_response.media_.push_back(medium_rcv);
                        }
                    } while (0);

                    break;//一个m匹配结束跳出

                } //end if (name_rcv == name_snd)

            }//end for (itr_rcv = media_rcv.begin(); itr_rcv != media_rcv.end(); ++itr_rcv)

        }//end for (;itr_snd!=media_snd.end();++itr_snd)

        //生成回传sdp
        //xsdp_response.media_ = xsdp_rcv.media_;
        try
        {
            std::ostringstream oss;
            xsdp_response.encode(oss);
            sdp = oss.str();
            DEBUG_LOG("send request my_send_sdp:\n%s\n",sdp.c_str());
        }
        catch(...)
        {
            ret_code = -4;
        }
    } while (false);
    DEBUG_LOG("sdp_compare end!\n");
    return ret_code;
}

void xt_session_sip::set_sipmsg_ptz_cb(sipmsg_ptz_cb cb)
{
    ptz_cb_ = cb;
}

void xt_session_sip::set_sipmsg_picupdate_cb(sipmsg_picupdate_cb cb)
{
    picupdate_cb_ = cb;
}

void xt_session_sip::set_sipmsg_cb(sipmsg_cb cb)
{
    message_cb_ = cb;
}

void xt_session_sip::set_sipinfo_cb(sipinfo_cb cb)
{
    info_cb_ = cb;
}

void xt_session_sip::set_sipmsg_ifrmconfreq_cb(sipmsg_ifrmconfreq_cb cb)
{
    igrmconfreq_cb_ = cb;
}

void xt_session_sip::set_sip_video_adjust_cb(sip_video_adjust_cb cb)
{
    video_adjust_cb_ =cb;
}

void xt_session_sip::set_sip_get_dev_info_cb(sip_get_dev_info_cb cb)
{
    get_dev_info_cb_ = cb;
}

void xt_session_sip::set_sip_point_index_operation_cb(sip_point_index_operation_cb cb)
{
    //point_index_operation_cb_ = cb;
}

void xt_session_sip::set_sip_register_srv_ret_info_cb(sip_register_srv_ret_info_cb cb)
{
    register_srv_ret_cb_ = cb;
}

void xt_session_sip::set_sipmsg_bandwidth_cb(sipmsg_bandwidth_cb cb)
{
    sipmsg_bandwidth_cb_ = cb;
}

void xt_session_sip::set_sipmsg_profile_cb(sipmsg_profile_cb cb)
{
    sipmsg_profile_cb_ = cb;
}

//add by songlei
int xt_session_sip::
construct_video_adjust_param(video_adjust_param_t& outparams,
                             const session_info_t& insession,const char *insdp,const int insdp_len)
{
    int ret_code = 0;
    do 
    {
        ::memset(outparams.framerate,0,64);
        ::memset(outparams.max_br,0,64);
        ::memset(outparams.resolution,0,64);

        if (NULL == insdp)
        {
            ret_code = -1;
            break;
        }
        if (::strlen(insdp) < 1)
        {
            ret_code = -1;
            break;
        }
        xt_sdp::parse_buffer_t pb(insdp, insdp_len); 
        xt_sdp::sdp_session_t xsdp;

        try
        {
            xsdp.parse(pb);;
        }
        catch(...)
        {
            ret_code = -1;
            break;
        }

        std::string fmtp;
        std::string framerate;
        std::string resolution;
        std::list<std::string> lsta;
        xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
        for (;xsdp.media_.end() != itr; ++itr)
        {
            if ( 0 != itr->name_.compare("video"))
            {
                continue;
            }

            if (itr->attribute_helper_.exists("fmtp"))
            {
                lsta = itr->attribute_helper_.get_values("fmtp");
                if (!lsta.empty())
                {
                    fmtp = lsta.front();
                }
                lsta.clear();
            }

            if (itr->attribute_helper_.exists("framerate"))
            {
                lsta = itr->attribute_helper_.get_values("framerate");
                if (!lsta.empty())
                {
                    framerate = lsta.front(); 
                    ::memcpy(outparams.framerate,framerate.c_str(),64);
                }
                lsta.clear();
            }

            if (itr->attribute_helper_.exists("resolution"))
            {
                lsta = itr->attribute_helper_.get_values("resolution");
                if (!lsta.empty())
                {
                    resolution = lsta.front();
                    ::memcpy(outparams.resolution,resolution.c_str(),64);
                }
                lsta.clear();
            }
        }

        //解析fmtp属性 live555 code
        if (fmtp.empty())
        {
            ret_code = -2;
            break;
        }
        char sdpbuf[1024]={0};
        ::memcpy(sdpbuf,fmtp.c_str(),fmtp.length());

        const char* sdpLine = &sdpbuf[0];

        while (::isdigit(*sdpLine)) ++sdpLine;

        unsigned const sdpLineLen = ::strlen(sdpLine);
        char nameStr[1024] ;
        char valueStr[1024];

        //码率
        std::string max_br("max-br");

        while (*sdpLine != '\0' && *sdpLine != '\r' && *sdpLine != '\n') 
        {
            ::sscanf(sdpLine, " %[^=; \t\r\n] = %[^; \t\r\n]", nameStr, valueStr);
            if ( 0 == max_br.compare(nameStr))
            {
                ::memcpy(outparams.max_br,valueStr,64);
            }
            while (*sdpLine != '\0' && *sdpLine != '\r' && *sdpLine != '\n' && *sdpLine != ';') ++sdpLine;
            while (*sdpLine == ';') ++sdpLine;
        }
    }while(0);

    return ret_code;
}

//add by songlei
bool xt_session_sip::is_multicast_ipv4(const std::string& ipv4)
{
    boost::asio::ip::address_v4 ip_v4;
    try
    {
        ip_v4 = boost::asio::ip::address_v4::from_string(ipv4);
    }catch(...)
    {
        DEBUG_LOG("is_multicast_ipv4 fail ip[%s]\n",ipv4.c_str());
        return false;
    }

    return ip_v4.is_multicast();
}

int xt_session_sip::get_dev_info(dev_info_t& out_devinfo)
{
    ::memset(&out_devinfo,0,sizeof(dev_info_t));
    if (get_dev_info_cb_)
    {
        return get_dev_info_cb_(out_devinfo);
    }
    return -1;
}

//add by songlei 20150625
void xt_session_sip::construct_devinfo_response(std::string& response,const dev_flag_t& dflg)
{
    do
    {
        response.clear();
        dev_info_t devinfo;
        int ret = get_dev_info(devinfo);
        if (ret < 0)
        {
            break;
        }

        xtXml res_xml;
        res_xml.LoadXMLStr("<Response></Response>");
        xtXmlNodePtr Response = res_xml.getRoot();
        if (Response.IsNull())
        {
            break;
        }

        xtXmlNodePtr commandname = Response.NewChild("commandname");
        if (!commandname.IsNull())
        {
            commandname.SetValue("getdeviceinfores");
        }

        xtXmlNodePtr seq = Response.NewChild("seq");
        if (!seq.IsNull())
        {
            seq.SetValue(dflg.seq);
        }

        xtXmlNodePtr devname = Response.NewChild("devname");
        if (!devname.IsNull())
        {
            devname.SetValue(devinfo.devname);
        }

        xtXmlNodePtr devid = Response.NewChild("devid");
        if (!devid.IsNull())
        {
            devid.SetValue(dflg.devid);
        }

        xtXmlNodePtr groupid = Response.NewChild("groupid");
        if (!groupid.IsNull())
        {
            groupid.SetValue(dflg.groupid);
        }

        session_info_t session;
        const int find_ret = get_sip_session(session);

        xtXmlNodePtr type = Response.NewChild("type");
        if (!type.IsNull())
        {
            meida_type_t mtype = media_na;
            if (0 <= find_ret)
            {
                mtype = parse_media_type(session);
            }
            if (mtype == meida_video_and_adio)
            {
                type.SetValue("0");
            }
            else if (mtype == meida_video)
            {
                type.SetValue("1");
            }
            else if (mtype == meida_audio)
            {
                type.SetValue("2");
            }
            else
            {
                type.SetValue("-1");
            }
        }

        xtXmlNodePtr vendername = Response.NewChild("vendername");
        if (!vendername.IsNull())
        {
            vendername.SetValue(devinfo.vendername);
        }

        xtXmlNodePtr deviceversion = Response.NewChild("deviceversion");
        if (!deviceversion.IsNull())
        {
            deviceversion.SetValue(devinfo.deviceversion);
        }

        xtXmlNodePtr sessionstate = Response.NewChild("sessionstate");
        if (!sessionstate.IsNull())
        {
            if (find_ret<0)
            {
                sessionstate.SetValue("0");
            }
            else
            {
                sessionstate.SetValue("1");
            }
        }

        xtXmlNodePtr sessiontime = Response.NewChild("sessiontime");
        if (!sessiontime.IsNull())
        {
            if (0 > find_ret)
            {
                sessiontime.SetValue("0s");
            }
            else
            {
                common::time_duration_t sessiontd = common::GetCurTimeSecondValue() - session.creat_time;
                std::ostringstream os;
                os<<sessiontd.total_seconds()<<"s";
                sessiontime.SetValue(os.str().c_str());
            }
        }

        xtXmlNodePtr peerurl = Response.NewChild("peerurl");
        if (!peerurl.IsNull())
        {
            peerurl.SetValue(session.peerurl.c_str());
        }

        response.assign(res_xml.GetXMLStrEx());
    } while(0);
}

//add by songlei 20150625
void xt_session_sip::construct_session_info_response(std::string& response,const dev_flag_t& dflg)
{
    do
    {
        response.clear();
        xtXml res_xml;
        res_xml.LoadXMLStr("<Response></Response>");

        xtXmlNodePtr Response = res_xml.getRoot();
        if (Response.IsNull())
        {
            break;
        }
        xtXmlNodePtr commandname = Response.NewChild("commandname");
        if (!commandname.IsNull())
        {
            commandname.SetValue("sessioninfores");
        }

        xtXmlNodePtr seq = Response.NewChild("seq");
        if (!seq.IsNull())
        {
            seq.SetValue(dflg.seq);
        }

        xtXmlNodePtr devname = Response.NewChild("devname");
        if (!devname.IsNull())
        {
            devname.SetValue(dflg.devname);
        }

        xtXmlNodePtr devid = Response.NewChild("devid");
        if (!devid.IsNull())
        {
            devid.SetValue(dflg.devid);
        }

        xtXmlNodePtr groupid = Response.NewChild("groupid");
        if (!groupid.IsNull())
        {
            groupid.SetValue(dflg.groupid);
        }

        //获取会话
        session_info_t session;
        const int find_ret = get_sip_session(session);
        if (find_ret < 0)
        {
            DEBUG_LOG("construct_session_info_response | get_sip_session fail\n");
            break;
        }

        xtXmlNodePtr type = Response.NewChild("type");
        if (!type.IsNull())
        {
            meida_type_t mtype = parse_media_type(session);

            if (mtype == meida_video_and_adio)
            {
                type.SetValue("0");
            }
            else if (mtype == meida_video)
            {
                type.SetValue("1");

            }
            else if (mtype == meida_audio)
            {
                type.SetValue("2");
            }
            else
            {
                type.SetValue("-1");
            }
        }

        /*
        信道类型satellite/mobile/microwave/shortwave/fixed
        分别对应卫星、移动、微波、超短波、固定网络信道
        */
        xtXmlNodePtr channeltype = Response.NewChild("channeltype");
        if (!channeltype.IsNull())
        {
            std::string chtype = channel_type_to_str(register_info_.channel_type.type);
            channeltype.SetValue(chtype.c_str());
        }

        xtXmlNodePtr bandwidth = Response.NewChild("bandwidth");
        if (!bandwidth.IsNull())
        {
            dev_info_t devinfo;
            int ret = get_dev_info(devinfo);
            if (ret < 0)
            {
                bandwidth.SetValue("0");
            }
            else
            {
                bandwidth.SetValue(devinfo.bandwidth); 
            }
        }

        xtXmlNodePtr delay = Response.NewChild("delay");
        if (!delay.IsNull())
        {
            int idelay = get_delay(session);
            std::ostringstream osdelay;
            osdelay << idelay;
            delay.SetValue(osdelay.str().c_str());
        }

        double dlossaudio = 0;
        int get_ret = get_packet_loss(dlossaudio,session,"audio");
        if (0 <= get_ret)
        {
            xtXmlNodePtr packetlossaudio = Response.NewChild("packetlossaudio");
            if (!packetlossaudio.IsNull())
            {
                std::ostringstream oslossaudio;
                oslossaudio<<dlossaudio;
                packetlossaudio.SetValue(oslossaudio.str().c_str());
            }
        }

        double dlossvideo = 0;
        get_ret = get_packet_loss(dlossvideo,session);
        if (0 <= get_ret)
        {
            xtXmlNodePtr packetlossvideo = Response.NewChild("packetlossvideo");
            if (!packetlossvideo.IsNull())
            {
                std::ostringstream osloosvideo;
                osloosvideo<<dlossvideo;
                packetlossvideo.SetValue(osloosvideo.str().c_str());
            }
        }
        response.assign(res_xml.GetXMLStrEx());
    }while(0);
}

meida_type_t xt_session_sip::parse_media_type(const session_info_t& session)
{
    bool is_video = false;
    bool is_audio = false;
    for (std::size_t i=0; i<session.tracks.size(); ++i)
    {
        if (0 == ::strcmp(session.tracks[i].trackname,"video"))
        {
            is_video = true;
        }
        else if (0 == ::strcmp(session.tracks[i].trackname,"audio"))
        {
            is_audio = true; 
        }
    }

    if (is_video && is_audio)
    {
        return meida_video_and_adio;
    }

    if (is_video && !is_audio)
    {
        return meida_video;
    }

    if (!is_video && is_audio)
    {
        return meida_audio; 
    }

    return media_na;
}

int xt_session_sip::get_packet_loss(double& out_loss,const session_info_t& session,const std::string& comdition/*="video"*/)
{
    for (std::size_t i=0; i<session.tracks.size(); ++i)
    {
        if (0 == comdition.compare(session.tracks[i].trackname))
        {
            rtp_sink_comdition_t cdt;
            int ret = XTSrc::instance()->get_chanid(cdt.chanid,session.srcno,session.tracks[i].trackname);
            if (ret< 0)
            {
                return -1;
            }
            ::strncpy(cdt.ip,session.tracks[i].ip,LEN_NODE);
            cdt.multiplex_flag = session.tracks[i].demux;
            cdt.muxid = session.tracks[i].demuxid;
            cdt.port = session.tracks[i].port;

            Rtp_Sink sink;
            XTRtp::instance()->get_sink_by_comdition(sink,cdt);

            rv_rtcp_rrinfo &rrinfo = sink.recieve.rrFrom;
            out_loss = rrinfo.fractionLost/0xFF;
            return 0;
        }
    }

    //未找到对应的流
    return -2;
}

int xt_session_sip::get_delay(const session_info_t& session,const std::string& comdition/*="video"*/)
{
    /*
    由于传输库暂时无法计算时延故暂时发0，
    发送者的时间与接收者的时间不在一个统计范畴内
    以后可通个修改caster库获取粗略时延
    */
    return 0;
    /* for (int i=0; i<session.track_num; ++i)
    {
    if (0 == comdition.compare(session.tracks[i].trackname))
    {
    rtp_sink_comdition_t cdt;
    ::strncpy(cdt.ip,session.tracks[i].ip,LEN_NODE);
    cdt.multiplex_flag = session.tracks[i].demux;
    cdt.muxid = session.tracks[i].demuxid;
    cdt.port = session.tracks[i].port;

    Rtp_Sink sink;
    XTRtp::instance()->get_sink_by_comdition(sink,cdt);

    rv_rtcp_rrinfo &rrinfo = sink.recieve.rrFrom;
    return rrinfo.dlSR;
    }
    }

    return 0;*/

}

void xt_session_sip::construct_operator_response_ctx(std::string& out_ctx,int opr_ret_code,int response_code,const char* seq,const char* fail_case)
{
    do 
    {
        out_ctx.clear();
        xtXml res_xml;
        res_xml.LoadXMLStr("<Response></Response>");

        xtXmlNodePtr Response = res_xml.getRoot();
        if (Response.IsNull())
        {
            break;
        }

        xtXmlNodePtr commandname = Response.NewChild("commandname");
        if (!commandname.IsNull())
        {
            commandname.SetValue("optstatus");
        }

        xtXmlNodePtr response_ = Response.NewChild("response");
        if (!response_.IsNull())
        {
            std::ostringstream os1;
            os1<<response_code;
            response_.SetValue(os1.str().c_str());
        }

        xtXmlNodePtr seq1 = Response.NewChild("seq");
        if (!seq1.IsNull())
        {
            seq1.SetValue(seq);
        }

        if (opr_ret_code < 0)
        {
            xtXmlNodePtr error = Response.NewChild("error");
            if (!error.IsNull() && !fail_case)
            {
                error.SetValue(fail_case);
            }
        }

        out_ctx.assign(res_xml.GetXMLStrEx());

    } while (0);
}

const std::string xt_session_sip::channel_type_to_str(const uint32_t& chtype) const
{
    std::stringstream os;
    if (XT_SIP_CHAN_SATELLITE & chtype)
    {
        os<<"satellite";
    }

    if (XT_SIP_CHAN_MOBILE & chtype)
    {
        os <<"/";
        os <<"mobile";
    }

    if (XT_SIP_CHAN_MICROWARE & chtype)
    {
        os <<"/";
        os <<"microwave";
    }

    if (XT_SIP_CHAN_SHORTWARE & chtype )
    {
        os <<"/";
        os <<"shortwave";
    }

    if (XT_SIP_CHAN_FIXED & chtype)
    {
        os <<"/";
        os << "fixed"; 
    }

    if (XT_SIP_CHAN_NONE & chtype)
    {
        //os<<"na";
    }

    return os.str();
}

void xt_session_sip::notify_regist_removed()
{
    boost::unique_lock<boost::recursive_mutex>lock(regist_removed_mutex_);
    regist_removed_condition_.notify_one();
}
void xt_session_sip::wait_regist_removed(uint32_t outtime/*=10*/)
{
    boost::xtime now;
    boost::xtime_get(&now, boost::TIME_UTC_);
    now.sec += outtime;
    boost::unique_lock<boost::recursive_mutex>lock(regist_removed_mutex_);
    regist_removed_condition_.timed_wait(regist_removed_mutex_, now);
}

void xt_session_sip::clear_send_data_all()
{
    boost::unique_lock<boost::shared_mutex> lock(session_info_mutex_);
    std::map<std::string,session_info_t>::iterator itr = session_infos_.begin();

    while(session_infos_.end() != itr)
    {
        for(std::size_t index=0; index < itr->second.tracks.size(); ++index)
        { 
            int ret = ::xt_del_send(itr->second.srcno, itr->second.tracks[index].trackname,itr->second.tracks[index].ip,
                itr->second.tracks[index].port,itr->second.tracks[index].demux, itr->second.tracks[index].demuxid);

            if (ret < 0)
            {
                DEBUG_LOG("clear_send_data_all | xt_del_send fail srcno[%d],trackanme[%s] ip[%s] port[%d]",
                    itr->second.srcno,itr->second.tracks[index].trackname,itr->second.tracks[index].ip,itr->second.tracks[index].port );
                continue;
            }
        }

        itr++;
    }

    session_infos_.clear();
}

const bool xt_session_sip::parse_sdp_attribute_rtpmap_value(char const* sdp_rtpmap_value,rtpmap_value_t& out) const
{
    // Check for a "a=rtpmap:<fmt> <codec>/<freq>" line:
    // (Also check without the "/<freq>"; RealNetworks omits this)
    // Also check for a trailing "/<numChannels>".
    if (4 == ::sscanf(sdp_rtpmap_value, "%u %[^/]/%u/%u", &out.rtpmap_payload_format, out.codec_name, &out.rtp_timestamp_frequency, &out.num_channels)
        || 3 == ::sscanf(sdp_rtpmap_value, "%u %[^/]/%u", &out.rtpmap_payload_format, out.codec_name, &out.rtp_timestamp_frequency)
        || 2 == ::sscanf(sdp_rtpmap_value, "%u %s", &out.rtpmap_payload_format, out.codec_name))
    {
        return true;

    }
    return false;
}

xt_sip_status_t xt_session_sip::ptzctrl(xtXml &xml, xtXmlNodePtr &root)
{
    xt_sip_status_t ret_stat = XT_SIP_STATUS_OK; 

    sipmsg_ptz ptz;
    ::memset(&ptz, 0, sizeof(sipmsg_ptz));

    ::strcpy(ptz.commandname, "ptzctrl");

    xtXmlNodePtr operation = xml.getNode(root, "operation");
    if (!operation.IsNull())
    {
        const char *operation_v = xml.getValue(operation);
        if (operation_v)
        {
            ::strcpy(ptz.operation, operation_v);
        }
    }

    xtXmlNodePtr seq = xml.getNode(root, "seq");
    if (!seq.IsNull())
    {
        const char *seq_v = xml.getValue(seq);
        if (!seq.IsNull())
        {
            ::strcpy(ptz.seq, seq_v);
        }
    }

    xtXmlNodePtr devname = xml.getNode(root, "devname");
    if (!devname.IsNull())
    {
        const char *devname_v = xml.getValue(devname);
        if (devname_v)
        {
            ::strcpy(ptz.devname, devname_v);
        }
    }

    xtXmlNodePtr devid = xml.getNode(root, "devid");
    if (!devid.IsNull())
    {
        const char *devid_v = xml.getValue(devid);
        if (devid_v)
        {
            ::strcpy(ptz.devid, devid_v);
        }
    }

    xtXmlNodePtr camera = xml.getNode(root, "camera");
    if (!camera.IsNull())
    {
        xtXmlNodePtr zoomctrl = xml.getNode(camera, "zoomctrl");
        if (!zoomctrl.IsNull())
        {
            const char *zoomctrl_v = xml.getValue(zoomctrl);
            if (zoomctrl_v)
            {
                ::strcpy(ptz.zoomctrl, zoomctrl_v);
            }
        }

        xtXmlNodePtr focusctrl = xml.getNode(camera, "focusctrl");
        if (!focusctrl.IsNull())
        {
            const char *focusctrl_v = xml.getValue(focusctrl);
            if (focusctrl_v)
            {
                ::strcpy(ptz.focusctrl, focusctrl_v);
            }
        }

        xtXmlNodePtr aperturectrl = xml.getNode(camera, "aperturectrl");
        if (!aperturectrl.IsNull())
        {
            const char *aperturectrl_v = xml.getValue(aperturectrl);
            if (aperturectrl_v)
            {
                ::strcpy(ptz.aperturectrl, aperturectrl_v);
            }
        }
    }

    xtXmlNodePtr servo = xml.getNode(root, "servo");
    if (!servo.IsNull())
    {
        xtXmlNodePtr direction = xml.getNode(servo, "direction");
        if (!direction.IsNull())
        {
            const char *direction_v = xml.getValue(direction);
            if (direction_v)
            {
                ::strcpy(ptz.direction, direction_v);
            }
        }

        xtXmlNodePtr speed = xml.getNode(servo, "speed");
        if (!speed.IsNull())
        {
            const char *speed_v = xml.getValue(speed);
            if (speed_v)
            {
                ::strcpy(ptz.speed, speed_v);
            }
        }
    }

    DEBUG_LOG("ptz_cb_ start...\n");
    if (ptz_cb_)
    {
        DEBUG_LOG("ptz_cb_ zoomctrl[%s] focusctrl[%s] aperturectrl[%s] direction[%s] speed[%s]\n",
            ptz.zoomctrl,ptz.focusctrl,ptz.aperturectrl,ptz.direction,ptz.speed);
        ptz_cb_(&ptz);
    }
    else
    {
        ret_stat = XT_SIP_STATUS_ERR_NOT_SUPPORTED;
    }

    return ret_stat;
}

xt_sip_status_t xt_session_sip::pointindexoperation(xtXml &xml, xtXmlNodePtr &root)
{
    xt_sip_status_t ret_stat = XT_SIP_STATUS_OK;
    point_index_operation_t pio;

    xtXmlNodePtr operation = xml.getNode(root, "operation");
    if (!operation.IsNull())
    {
        const char *operation_v = xml.getValue(operation);
        if (!operation.IsNull())
        {
            ::strcpy(pio.operation, operation_v);
        }
    }

    xtXmlNodePtr seq = xml.getNode(root, "seq");
    if (!seq.IsNull())
    {
        const char *seq_v = xml.getValue(seq);
        if (!seq.IsNull())
        {
            ::strcpy(pio.seq, seq_v);
        }
    }

    xtXmlNodePtr devname = xml.getNode(root, "devname");
    if (!devname.IsNull())
    {
        const char *devname_v = xml.getValue(devname);
        if (devname_v)
        {
            ::strcpy(pio.devname, devname_v);
        }
    }

    xtXmlNodePtr devid = xml.getNode(root, "devid");
    if (!devid.IsNull())
    {
        const char *devid_v = xml.getValue(devid);
        if (devid_v)
        {
            ::strcpy(pio.devid, devid_v);
        }
    }

    xtXmlNodePtr pointindexvalue = xml.getNode(root, "pointindexvalue");
    if (!pointindexvalue.IsNull())
    {
        const char *pointindexvalue_v = xml.getValue(pointindexvalue);
        if (pointindexvalue_v)
        {
            ::strcpy(pio.pointindexvalue, pointindexvalue_v);
        }
    }

    DEBUG_LOG("on_server_message_arrived: point_index_operation_cb_ start....\n");
    if (point_index_operation_cb_)
    {
        point_index_operation_cb_(pio);
        DEBUG_LOG("on_server_message_arrived: point_index_operation_cb_ end\n");
    }
    else
    {
        ret_stat = XT_SIP_STATUS_ERR_NOT_SUPPORTED;
    }

    return ret_stat;
}
xt_sip_status_t xt_session_sip::iframeconfreq(dev_flag_t *dev_flag, sipmsg_ifrmconfreq &freq, std::string &respones_ctx)
{
    xt_sip_status_t ret_stat = XT_SIP_STATUS_OK;
    int oprcode = 0;

    //设置I帧间隔
    DEBUG_LOG("on_server_message_arrived :psip->igrmconfreq_cb_ start...!\n");
    if (igrmconfreq_cb_)
    {
        DEBUG_LOG("on_server_message_arrived :psip->igrmconfreq_cb_!\n");

        char fail_case[2048]={0};
        igrmconfreq_cb_(&freq, oprcode, fail_case);

        if (oprcode < 0)
        {
            ret_stat = XT_SIP_STATUS_ERR_UNKNOWN;
            construct_operator_response_ctx(respones_ctx,oprcode, 401, dev_flag->seq,fail_case);
        }
        else
        {
            construct_operator_response_ctx(respones_ctx, oprcode, 200, dev_flag->seq, NULL);
        }
    }
    else
    {
        ret_stat = XT_SIP_STATUS_ERR_NOT_SUPPORTED;
    }

    return ret_stat;
}
xt_sip_status_t xt_session_sip::coderate(dev_flag_t *dev_flag, int strmid, const char *coderate, std::string &respones_ctx)
{
    xt_sip_status_t ret_stat = XT_SIP_STATUS_OK;
    video_adjust_param_t params = {"\0","\0","\0"};
    strcpy(params.max_br,coderate);

    //修改码率
    DEBUG_LOG("on_server_message_arrived :psip->video_adjust_cb_ start...!\n");
    if (video_adjust_cb_)
    {
        DEBUG_LOG("on_server_message_arrived :psip->video_adjust_cb_!\n");
        int oprcode = 0;
        char fail_case[2048]={0};
        DEBUG_LOG("on_server_message_arrived:streamid[%d] max_br[%s]\n",strmid,params.max_br);
        video_adjust_cb_(params, strmid);
        DEBUG_LOG("on_server_message_arrived:psip->video_adjust_cb_ end\n");
    }
    else
    {
        ret_stat = XT_SIP_STATUS_ERR_NOT_SUPPORTED;
    }

    return ret_stat;
}
xt_sip_status_t xt_session_sip::resolution(dev_flag_t *dev_flag, int strmid, const char *resolution, std::string &respones_ctx)
{
    xt_sip_status_t ret_stat = XT_SIP_STATUS_OK;
    video_adjust_param_t params = {"\0","\0","\0"};
    strcpy(params.resolution,resolution);

    //修改分辨率
    DEBUG_LOG("on_server_message_arrived :psip->video_adjust_cb_ start...!\n");
    if (video_adjust_cb_)
    {
        DEBUG_LOG("on_server_message_arrived :psip->video_adjust_cb_!\n");
        char fail_case[2048]={0};
        DEBUG_LOG("on_server_message_arrived:streamid[%d] resolution[%s]\n",strmid,params.resolution);
        video_adjust_cb_(params,strmid);
        DEBUG_LOG("on_server_message_arrived:psip->video_adjust_cb_ end\n");
    }
    else
    {
        ret_stat = XT_SIP_STATUS_ERR_NOT_SUPPORTED;
    }

    return ret_stat;
}
xt_sip_status_t xt_session_sip::getdeviceinforeq(dev_flag_t *dev_flag, std::string &respones_ctx)
{
    xt_sip_status_t ret_stat = XT_SIP_STATUS_OK;

    construct_devinfo_response(respones_ctx, *dev_flag);

    return ret_stat;
}

xt_sip_status_t xt_session_sip::sessioninforeq(dev_flag_t *dev_flag, std::string &respones_ctx)
{
    xt_sip_status_t ret_stat = XT_SIP_STATUS_OK;

    construct_session_info_response(respones_ctx, *dev_flag);

    return ret_stat;
}

void xt_session_sip::on_client_message_response(void *ctx, xt_sip_client_message_handle_t h, xt_sip_msg_handle_t msg, uint8_t success)
{
	return;
}

int xt_session_sip::check_pri(std::string res_pri)
{
	boost::unique_lock<boost::shared_mutex> lock(session_info_mutex_);
	if (session_infos_.empty())
	{
		return 0;
	}

	std::map<std::string,session_info_t>::iterator itr = session_infos_.begin();
	for (;session_infos_.end() != itr; ++itr)
	{
		session_info_t &s = itr->second;
		std::string p1 = res_pri;
		std::string p2 = s.res_pri;
		std::string::size_type pos1 = p1.find('.');
		std::string::size_type pos2 = p2.find('.');
		int pri1 = 0;
		int pri2 = 0;
		if (pos1 != std::string::npos )
		{
			p1 = p1.substr(pos1, (p1.length()-pos1));
			pri1 = ::atoi(p1.c_str());
		}
		if (pos2 != std::string::npos )
		{
			p2 = p2.substr(pos2, (p2.length()-pos2));
			pri2 = ::atoi(p2.c_str());
		}

		printf("------------------------p1:%d p2:%d--------------------------------\n", pri1, pri2);
		if (pri1 < pri2)
		{
			return -1;
		}
	}

	return 0;
}