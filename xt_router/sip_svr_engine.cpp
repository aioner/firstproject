#include "sip_svr_engine.h"
#include "Router_config.h"
#include "sip_svr_task.h"
#include "XTEngine.h"

#include "XTRouterLog.h"

sip_svr_engine sip_svr_engine::self_;
long sip_svr_engine::id_ = 1;
xt_sip_client_invite_callback_t sip_svr_engine::client_invite_cb=
{
	on_client_invite_failure,
	on_client_invite_200ok,
	on_client_invite_offer,
	on_client_invite_bye,
	on_client_invite_info,
	on_client_invite_info_response,
	on_client_invite_message,
	on_client_invite_message_response,
	on_client_msg_prev_post
};
sip_svr_engine::sip_svr_engine():
sip_hanle_(XT_SIP_INVALID_HANDLE)
{
}

sip_svr_engine::~sip_svr_engine()
{
}

//回调
//option 失败响应
int8_t sip_svr_engine::on_xt_sip_heartbeat_not_pong_cb(void *ctx, const char *target,uint32_t count)
{
	sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
	if (NULL == sip_engine)
	{
		return -1;
	}

	WRITE_LOG(DBLOGINFO,ll_info,"option 失败响应 target[%s]",target);
	printf("on_xt_sip_heartbeat_not_pong_cb target[%s]\n",target);
	xt_sip_invite_terminated_reason_t reason = XT_SIP_TERMINATED_REASON_TIMEOUT;
	if (count > 3)
	{
		invite_bye_task* ptr_task = new invite_bye_task(target,reason);
		if (NULL != ptr_task)
		{
			ptr_task->process_event();
		}
	}

	return 0;
}

void sip_svr_engine::del_options(const char* target)
{
	xt_sip_status_t ret_code = ::xt_sip_heartbeat_remove_target(sip_hanle_,target);
	WRITE_LOG(DBLOGINFO,ll_info,"del_options xt_sip_heartbeat_remove_target[%s] ret_code[%d]",target,ret_code);
}
void sip_svr_engine::add_options(const char* target)
{
	xt_sip_status_t ret_code = ::xt_sip_heartbeat_add_target(
		sip_hanle_, target,cfg_info_.link_keep_alive_time_interval, cfg_info_.link_keep_alive_time_interval, on_xt_sip_heartbeat_not_pong_cb, this);

	WRITE_LOG(DBLOGINFO,ll_info,"add_options xt_sip_heartbeat_add_target[%s] ret_code[%d]",target,ret_code);
}

//INVITE with sdp
void sip_svr_engine::on_sip_server_invite_with_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len)
{
    sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
    if (NULL == sip_engine)
    {
        return;
    }

    std_sip_session_t session;
    session.creat_time = boost::posix_time::second_clock::local_time();
	session.invite_type = SIP_INVITE_SERVER;

	long ret = sip_engine->parse_msg_info(msg,session.call_id,session.from,session.to);
	if (ret < 0)
	{
		return;
	}

    session.sdp_len = len;
	session.sdp = sdp;
    if (0 < session.sdp_len)
    {
        session.sdp.assign(sdp,session.sdp_len);
    }
    else
    {
        session.sdp.clear();
    }

	xt_sip_server_invite_handle_t invite_handle_ = ::xt_sip_server_invite_handle_clone(h);
	session.hserver = invite_handle_;
	session.hclient = NULL;

    WRITE_LOG(DBLOGINFO,ll_info,"call_id[%s] target[%s] to[%s] INVITE with sdp[%s]",
        session.call_id.c_str(), session.from.c_str(), session.to.c_str(), session.sdp.c_str());
	printf("server invite offer call_id[%s] target[%s] to[%s] INVITE with sdp[%s]",
		session.call_id.c_str(), session.from.c_str(), session.to.c_str(), session.sdp.c_str());
    
	if (SIP_INVITE == sip_engine->get_session_type(session.call_id))
    {
		string org_callid;
		long ret_code = sip_engine->exist_session(session,org_callid);
		if (ret_code > 0)
		{
			xt_sip_invite_terminated_reason_t reason = XT_SIP_TERMINATED_REASON_REFERRED;
			invite_bye_task* ptr_task = new invite_bye_task(org_callid,reason);
			if (NULL != ptr_task)
			{
				ptr_task->process_event();
			}
		}


		sip_engine->add_session(session);

		recv_invite_task* ptr_task = new recv_invite_task(session);
		if (NULL != ptr_task)
		{
			ptr_task->process_event();
		}
    }
    else
    {
        recv_reinvite_task* ptr_task = new recv_reinvite_task(session,h,mode_offer);
        if (NULL != ptr_task)
        {
            ptr_task->process_event();
        }
    }
}

//INVITE no sdp
void sip_svr_engine::on_sip_server_invite_no_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg)
{
}

//ACK with sdp
void sip_svr_engine::on_sip_server_invite_ack_with_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len)
{
}

//ACK no sdp
void sip_svr_engine::on_sip_server_invite_ack_no_sdp(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg)
{
	sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
	if (NULL == sip_engine)
	{
		return;
	}

	//获取call id
	std::string call_id;
	std::string from;
	std::string to;
	long ret_code = sip_engine->parse_msg_info(msg,call_id,from,to);
	if (ret_code < 0)
	{
		return;
	}
}

//收到BYE
void sip_svr_engine::on_sip_server_invite_bye(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, xt_sip_invite_terminated_reason_t reason)
{
    sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
    if (NULL == sip_engine)
    {
        return;
    }
    do 
    {
        long ret_code = 0;
        std::string call_id;
        ret_code = sip_engine->parse_call_id_by_msg(msg,call_id);
        if (ret_code < 0)
        {
            break;
        }

        WRITE_LOG(DBLOGINFO,ll_info,"call_id[%s] 收到BYE reason[%d]",call_id.c_str(), reason);

        invite_bye_task* ptr_task = new invite_bye_task(call_id,reason);
        if (NULL != ptr_task)
        {
            ptr_task->process_event();
        }
    } while (0);
}

//收到会话内的INFO
void sip_svr_engine::on_sip_server_invite_info(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg)
{
    sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
    if (NULL == sip_engine)
    {
        return;
    }
}

//发送会话内的INFO操作结果
void sip_svr_engine::on_sip_server_invite_info_response(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success)
{
    sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
    if (NULL == sip_engine)
    {
        return;
    }
}

//收到会话内的MESSAGE
void sip_svr_engine::on_sip_server_invite_message(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg)
{
    sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
    if (NULL == sip_engine)
    {
        return;
    }
}

//发送会话内的MESSAGE操作结果响应
void sip_svr_engine::on_sip_server_invite_message_response(void *ctx, xt_sip_server_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success)
{
    sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
    if (NULL == sip_engine)
    {
        return;
    }

}

//收到会话外的MESSAGE
void sip_svr_engine::on_sip_server_message_arrived(void *ctx, xt_sip_server_message_handle_t h, xt_sip_msg_handle_t msg )
{
    sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
    if (NULL == sip_engine)
    {
        return;
    }

	long oper_code = 0;
    if (oper_code < 0)
    {
        WRITE_LOG(DBLOGINFO,ll_error,"on_server_message_arrived[pares xml error!]",1);
        sip_engine->response_optstatus_fail("pares xml error!");
    }
    else
    {
        // 回复200 ok，表示接受该消息
        ::xt_sip_server_message_accept(h, 200); //针对message回复，否则message一直发送
    }
}

int sip_svr_engine::std_sip_start()
{
    std_sip_status status= SIP_STATUS_OK;
    int ret_code = 0;
    do 
    {
        status = load_cfg();
        if (status !=  SIP_STATUS_OK)
        {
            ret_code = -1;
            break;
        }

        status = start();
        if (status !=  SIP_STATUS_OK)
        {
            ret_code = -2;
            break;
        }
        ret_code=1;
    } while (0);

    return ret_code;
}
int sip_svr_engine::std_sip_stop()
{
    int ret_code=-1;
    do 
    {
        stop();
        ret_code=1;
    } while (0);

    return ret_code;
}

std_sip_status sip_svr_engine::load_cfg()
{
    std_sip_status oper_status = SIP_STATUS_OK;
    do 
    {
        //create cfg
        cfg_info_.session_keep_alive_time_interval = config::_()->sip_cfg_session_keep_alive_time_interval(90);
        cfg_info_.registration_retry_time_interval = config::_()->sip_cfg_registration_retry_time_interval(60);
        uint32_t protocl = config::_()->sip_cfg_protocol(3);
        switch(protocl)
        {
        case 0:
            {
                cfg_info_.protocol = SIP_FOR_TCP;
                break;
            } 
        case 1:
            {
                cfg_info_.protocol = SIP_FOR_UDP;
                break;
            }
        case 2:
            {
                cfg_info_.protocol = SIP_FOR_TCP_UDP;
                break;
            }
        default:
            {
                cfg_info_.protocol = SIP_FOR_NA;
                break;
            }
        }

        if (SIP_FOR_NA == cfg_info_.protocol)
        {
            oper_status = SIP_STATUS_PROTOCOL_ERR;
            break;
        }


        cfg_info_.transport = config::_()->sip_cfg_transport(5060);
        cfg_info_.tls_port = config::_()->sip_cfg_tls_port(0);
        cfg_info_.dtls_port = config::_()->sip_cfg_dtls_port(0);

        //register cfg
        cfg_info_.expires = config::_()->sip_cfg_expires(60);
        cfg_info_.link_keep_alive_time_interval = config::_()->sip_cfg_link_keep_alive_time_interval(25000);
        cfg_info_.regist_retry_time_interval = config::_()->sip_cfg_regist_retry_time_interval(60);

        cfg_info_.channle_type = config::_()->sip_cfg_channle_type(4);
        cfg_info_.net_delay = config::_()->sip_cfg_delay(0);
        cfg_info_.net_packetloss = config::_()->sip_cfg_packetloss(0);
        cfg_info_.net_mtu = config::_()->mtu(1500);
        cfg_info_.net_bandwidth = config::_()->sip_cfg_bandwidth(4096);
        cfg_info_.usre =  config::_()->sip_cfg_usre("");
        if (cfg_info_.usre.empty())
        {
            oper_status = SIP_STATUS_USRE_IS_EMPTY;
            break;
        }

        cfg_info_.pwd = config::_()->sip_cfg_password("");
        if (cfg_info_.pwd.empty())
        {
            oper_status = SIP_STATUS_PWD_IS_EMPTY;
            break;
        }

        std::list<std::string> domains;
        config::instance()->sip_cfg_get_domain(domains);
        if (domains.empty())
        {
            oper_status = SIP_STATUS_DOMAIM_IS_EMPTY;
            break;
        }

        cfg_info_.main_domain = domains.front();
        domains.pop_front();
        cfg_info_.back_domains = domains; 

		long ret = load_nodelist();
		if (ret < 0)
		{
			printf("local node list from xml config failed!\n");
			break;
		}

    } while (0);


    return oper_status;
}

std_sip_status sip_svr_engine::start()
{
    WRITE_LOG(DBLOGINFO,ll_info,"xt_session_sip::start start!\n",1);

    ::xt_sip_log_init(XT_SIP_LOG_NONE, "xt_sip_test", "xt_sip_test.log");

    xt_sip_options_t options = { 0 }; 
    options.flags = XT_SIP_TRAITS_CLIENT_INVITE | XT_SIP_TRAITS_SERVER_INVITE | XT_SIP_TRAITS_PAGER_MESSAGE |XT_SIP_TRAITS_CLIENT_REGISTER;
    options.domain = cfg_info_.main_domain.c_str();
    options.user = cfg_info_.usre.c_str();
    options.pwd = cfg_info_.pwd.c_str();
    options.realm = cfg_info_.main_domain.c_str();

    options.tcp_port = 0;
    options.udp_port  = 0;
    if (SIP_FOR_TCP == cfg_info_.protocol)
    {
        options.tcp_port = cfg_info_.transport;
    }
    if (SIP_FOR_UDP == cfg_info_.protocol)
    {
        options.udp_port = cfg_info_.transport;
    }
    if (SIP_FOR_TCP_UDP == cfg_info_.protocol)
    {
        options.tcp_port = cfg_info_.transport;
        options.udp_port = cfg_info_.transport;
    }
    if (options.udp_port == 0  && options.tcp_port == 0)
    {

        WRITE_LOG(DBLOGINFO,ll_error,"sip start fail! tcp port and udp port is 0!\n",1);
        return SIP_STATUS_PORT_IS_EMPTY;
    }

    printf("domain:%s user:%s pwd:%s realm:%s tcpport:%d udpport:%d\n",
        options.domain,options.user,options.pwd,options.realm,options.tcp_port,options.udp_port);

    xt_sip_server_invite_callback_t server_invite_cb = { 0 };
    server_invite_cb.on_offer = on_sip_server_invite_with_sdp;
    server_invite_cb.on_answer = on_sip_server_invite_ack_with_sdp;
    server_invite_cb.on_connected_confirmed = on_sip_server_invite_ack_no_sdp;
    server_invite_cb.on_terminated = on_sip_server_invite_bye;
    server_invite_cb.on_offer_required = on_sip_server_invite_no_sdp;
    server_invite_cb.on_info = on_sip_server_invite_info;
    server_invite_cb.on_info_response = on_sip_server_invite_info_response;
    server_invite_cb.on_message = on_sip_server_invite_message;
    server_invite_cb.on_message_response = on_sip_server_invite_message_response;

    options.server_invite_callback = &server_invite_cb;
    options.server_invite_callback_ctx = static_cast<void*>(this);

    xt_sip_server_message_callback_t server_message_cb = { 0 };
    server_message_cb.on_arrived = on_sip_server_message_arrived;

    options.server_message_callback = &server_message_cb;
    options.server_message_callback_ctx = static_cast<void*>(this);

    xt_sip_ext_options_t ext_optins = {0};
    ext_optins.default_session_time = cfg_info_.session_keep_alive_time_interval;
    ext_optins.default_registration_retry_time = cfg_info_.session_keep_alive_time_interval;
    ext_optins.tls_port = cfg_info_.tls_port;
    ext_optins.dtls_port = cfg_info_.dtls_port;

    xt_sip_client_digest_credential_t* ptr_dc = NULL;
    std::size_t dc_num=cfg_info_.back_domains.size();

    if (!cfg_info_.back_domains.empty())
    {
        try
        {
            ptr_dc = new xt_sip_client_digest_credential_t[dc_num];
        }
        catch (...)
        {
            return SIP_STATUS_MALLOC_MEM_FAIL;
        }

        if (ptr_dc == NULL)
        {
            return SIP_STATUS_MALLOC_MEM_FAIL;
        }

        sip_cfg_info_t::back_domain_handle_t itr = cfg_info_.back_domains.begin();
        for(int index=0;cfg_info_.back_domains.end() != itr; ++itr)
        {
            ptr_dc[index].realm = itr->c_str();
            ptr_dc[index].user = cfg_info_.usre.c_str();;
            ptr_dc[index].pwd = cfg_info_.pwd.c_str();
        }
    }
    ext_optins.ext_client_digest_credential_num = dc_num;
    ext_optins.ext_client_digest_credential = ptr_dc;

    xt_sip_status_t ret_code = ::xt_sip_create(&options, &ext_optins, &sip_hanle_);
    if (ptr_dc)
    {
        delete[] ptr_dc;
        ptr_dc = NULL;
    }
    if (ret_code != XT_SIP_STATUS_OK)
    {

        WRITE_LOG(DBLOGINFO,ll_error,"xt_sip_create fail ret_code[%d]",ret_code);
        return SIP_STATUS_CREATE_FAIL;
    }

    WRITE_LOG(DBLOGINFO,ll_info,"xt_session_sip::start:ret_code[%d] domain[%s] username[%s] password[%s] realm[%s] tcp_port[%d] udp_port[%d] session_time[%d] tls_port[%d] dtls_port[%d]",
        ret_code,options.domain,options.user,options.pwd,options.realm,options.tcp_port,options.udp_port,ext_optins.default_session_time,ext_optins.tls_port,ext_optins.dtls_port);

    return SIP_STATUS_OK;
}

void sip_svr_engine::stop()
{

    WRITE_LOG(DBLOGINFO,ll_error,"xt_session_sip::stop start!\n",1);
    ::xt_sip_destroy(sip_hanle_);
    WRITE_LOG(DBLOGINFO,ll_error,"xt_session_sip::stop end!\n",1);
}

void sip_svr_engine::on_client_invite_failure(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg)
{
	printf("on_client_invite_failure\n");
	sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
	if (NULL == sip_engine)
	{
		return;
	}

	do 
	{
		std::string call_id;
		long ret_code = sip_engine->parse_call_id_by_msg(msg,call_id);
		if (ret_code < 0)
		{
			break;
		}

		xt_sip_invite_terminated_reason_t reason = XT_SIP_TERMINATED_REASON_LOCAL_CANCEL;
		WRITE_LOG(DBLOGINFO,ll_info,"call_id[%s] invite failure reason[%d]",call_id.c_str(), reason);
		
		invite_bye_task* ptr_task = new invite_bye_task(call_id,reason);
		if (NULL != ptr_task)
		{
			ptr_task->process_event();
		}
	} while (0);
}

long sip_svr_engine::parse_msg_info(xt_sip_msg_handle_t msg,string &callid,string &from,string &to)
{
	long ret_code = parse_call_id_by_msg(msg,callid);
	if (ret_code < 0)
	{
		return ret_code;
	}
	ret_code = parse_from_by_msg(msg,from);
	if (ret_code < 0)
	{
		return ret_code;
	}
	ret_code = parse_to_by_msg(msg,to);
	if (ret_code < 0)
	{
		return ret_code;
	}

	return ret_code;
}

long sip_svr_engine::one_part_sdp(const string src_sdp, string &part_sdp,bool rdir)
{
	long oper_code = 0;
	do 
	{
		if (part_sdp.empty())
		{
			oper_code = -4;
			break;
		}

		sdp_direction_t sdp_dir = sip_svr_engine::_()->parse_sdp_direction(src_sdp);

		xt_sdp::parse_buffer_t pb_200_ok(part_sdp.c_str(),part_sdp.length());
		xt_sdp::sdp_session_t xsdp_200_ok; 
		try
		{
			xsdp_200_ok.parse(pb_200_ok);
		}
		catch(...)
		{
			oper_code = -2;
			break;
		}
		if ( dir_recvonly == sdp_dir)
		{
			xt_sdp::sdp_session_t::medium_container_t::iterator itr;
			for(itr = xsdp_200_ok.media_.begin(); xsdp_200_ok.media_.end() != itr;)
			{
				if (itr->attribute_helper_.exists(rdir?"recvonly":"sendonly"))
				{
					xsdp_200_ok.media_.erase(itr++);
				}
				else
				{
					++itr;
				}
			}
		}
		else if (dir_sendonly == sdp_dir)
		{
			xt_sdp::sdp_session_t::medium_container_t::iterator itr;
			for(itr = xsdp_200_ok.media_.begin(); xsdp_200_ok.media_.end() != itr;)
			{
				if (itr->attribute_helper_.exists(rdir?"sendonly":"recvonly"))
				{
					xsdp_200_ok.media_.erase(itr++);
				}
				else
				{
					++itr;
				}
			}
		}
		else if (dir_sendrecv != sdp_dir)
		{
			oper_code = -5;
			break;
		}

		try
		{
			std::ostringstream oss;
			xsdp_200_ok.encode(oss);
			part_sdp.assign(oss.str());
		}
		catch(...)
		{
			oper_code = -4;
			break;
		}
	} while (0);

	return oper_code;
}

long sip_svr_engine::push_media_streams(std_sip_session_t src_session, std_sip_session_t dst_session)
{
	long oper_code = 0;
	do 
	{
		std::string reason;

		std::string recv_sdp;
		std::string send_sdp;//以对端回复sdp的能力级来组帧以及转发
		long ret_code = XTEngine::_()->sip_transform_call_sdp_ex(recv_sdp,send_sdp,src_session.sdp);
		if (ret_code < 0)
		{
			oper_code  = -1;
			break;
		}
		//保存组帧sdp
		dev_handle_t recv_dev_handle = src_session.dev_handle;
		//通过该入口设置包含音频视频sdp到数据接收库
		ret_code = XTEngine::_()->save_sdp_to_access(recv_dev_handle,send_sdp.c_str(),send_sdp.length());
		if (ret_code < 0)
		{
			oper_code = -3;
			reason.assign("save sdp to access lib fail!");
			break;
		}
		WRITE_LOG(DBLOGINFO,ll_info,"switchset send_sdp[%s]",send_sdp.c_str());

		//已经在创建接收机制的时候分配
		long strmid = media_device::_()->get_strmid_by_dev_handle(recv_dev_handle); //获取空闲流标记id，音频视频为同一路流，配置支持128路流
		if (strmid < 0)
		{
			oper_code = -5;
			reason.assign("get_free_strmid fail!");
			break;
		}
		///////////////////////////////////////////////////////////////////////////////////
		int trans_srcno = src_session.srcno;

		//保存sdp到转发库//set_key
		ret_code = XTEngine::_()->save_sdp_to_srv(trans_srcno,send_sdp.c_str(),send_sdp.length(),9);
		if (ret_code < 0)
		{
			oper_code=-10;
			break;
		}

		ret_code = XTEngine::_()->update_access_info_of_src(trans_srcno,strmid,recv_dev_handle);
		if (ret_code < 0)
		{
			reason.assign("pro dst rtpid upate_src fail!");
			oper_code = -11;
			break;
		}

		//取出目标的接收sdp
		ret_code = XTEngine::_()->sip_transform_call_sdp_ex(recv_sdp,send_sdp,dst_session.sdp);
		if (ret_code < 0)
		{
			oper_code = -12;
			break;
		}
		ret_code = sip_svr_engine::_()->add_send_dst(trans_srcno,recv_sdp);
		if (ret_code < 0)
		{
			oper_code = -13;
			reason.assign("add_send_dst fail!");
			break;
		}
	} while (0);

	return oper_code;
}
long sip_svr_engine::combine_c_to_media(const string sdp_m, const string sdp_c,string &dst_sdp)
{
	long oper_code = 0;
	do 
	{

		long ret_code = 0;
		xt_sdp::parse_buffer_t pb_c(sdp_c.c_str(), sdp_c.length());
		xt_sdp::sdp_session_t xsdp_c;
		try
		{
			xsdp_c.parse(pb_c);

		}
		catch(...)
		{
			oper_code = -1;
			break;
		}

		xt_sdp::parse_buffer_t pb_m(sdp_m.c_str(), sdp_m.length());
		xt_sdp::sdp_session_t xsdp_m;
		try
		{
			xsdp_m.parse(pb_m);
		}
		catch(...)
		{
			oper_code = -2;
			break;
		}

		//必须修改音视频接收端口
		xsdp_m.origin_ = xsdp_c.origin_;
		xt_sdp::sdp_session_t::medium_container_t::iterator itr_m = xsdp_m.media_.begin();
		for (;xsdp_m.media_.end() != itr_m; ++itr_m)
		{
			xt_sdp::sdp_session_t::medium_container_t::iterator itr_c = xsdp_c.media_.begin();
			for (;xsdp_c.media_.end() != itr_c;++itr_c)
			{
				if (!itr_c->connections_.empty())
				{
					xsdp_m.connection_.address_ = xsdp_c.connection_.address_;
				}
				//情况：sdpc中有4个方向并且收发端口不一样，sdpm中方向为sendrecv时，取sdpc中recvonly作为sdpm中sendrecv端口
				if (itr_c->name_ == itr_m->name_ && 
					(itr_m->attribute_helper_.exists("sendrecv") || itr_m->attribute_helper_.exists("recvonly")) &&
					itr_c->attribute_helper_.exists("recvonly"))
				{
					itr_m->connections_ = itr_c->connections_;
					itr_m->port_ = itr_c->port_;
				}

				if (itr_c->name_ == itr_m->name_ && 
					itr_m->attribute_helper_.exists("sendonly") &&
					itr_c->attribute_helper_.exists("sendonly"))
				{
					itr_m->connections_ = itr_c->connections_;
					itr_m->port_ = itr_c->port_;
				}
			}
		}

		try
		{
			std::ostringstream oss;
			xsdp_m.encode(oss);
			dst_sdp.assign(oss.str());
		}
		catch(...)
		{
			oper_code = -3;
			break;
		}
	} while (0);
	
	return oper_code;
}

long sip_svr_engine::parse_uri(const string uri,node_info_t &node_i)
{
	long oper_code = 0;
	do 
	{
		string stemp = uri;
		int len = stemp.length();
		int posb = stemp.rfind(':');
		if (len <=0 || posb <=0 || len-posb-1 <= 0)
		{
			oper_code = -1;
			break;
		}
		node_i.port = stemp.substr(posb+1,len-posb-1);
		int posa = stemp.find('@');
		if (posa <=0 || posb-1-posa <=0||posa -4 <=0)
		{
			oper_code = -2;
			break;
		}
		node_i.ip = stemp.substr(posa+1,posb-1-posa);
		node_i.ids = stemp.substr(4,posa-4);
	} while (0);
	

	return oper_code;
}
//初始发起uri为ids为最终目标ids，ip和port为下一个节点的ip和port
long sip_svr_engine::find_next_ids(string from_uri,string local_uri,const string dst_uri,std::string &next_uri)
{
	long oper_code = 0;
	do 
	{
		long ret_code=0;
		if (from_uri.empty() || local_uri.empty() || dst_uri.empty())
		{
			oper_code = -1;
			break;
		}
		node_info_t tmp_node;
		ret_code  = parse_uri(from_uri,tmp_node);
		if (ret_code < 0)
		{
			oper_code =-2;
			break;
		}
		string from_ids = tmp_node.ids;
		ret_code = parse_uri(local_uri,tmp_node);
		if (ret_code<0)
		{
			oper_code=-3;
			break;
		}
		string local_ids = tmp_node.ids;
		ret_code = parse_uri(dst_uri,tmp_node);
		if (ret_code<0)
		{
			oper_code = -4;
			break;
		}
		string dst_ids = tmp_node.ids;

		next_uri.clear();
		vector<node_info_t> node_path;
		list< vector<node_info_t> >::iterator itr;
		for (itr = m_node_list.begin();itr!=m_node_list.end();itr++)
		{
			node_path = *itr;
			int count = node_path.size();
			int from_pos = -1;
			int local_pos = -1;
			int dst_pos = -1;
			for (int i=0;i<count;i++)
			{
				if (node_path[i].ids == from_ids)
				{
					from_pos = i;
				}
				else if (node_path[i].ids == local_ids)
				{
					local_pos = i;
				}
				else if (node_path[i].ids == dst_ids)
				{
					dst_pos = i;
				}
				else
				{
					continue;
				}
			}
			if (from_pos >= 0 && local_pos > 0 && dst_pos >= 0)
			{
				node_info_t next_node = from_pos < dst_pos?node_path[local_pos+1]:node_path[local_pos-1];
				next_uri = "sip:"+next_node.ids+"@"+next_node.ip+":"+next_node.port;
				break;
			}
		}
		if (next_uri.empty())
		{
			oper_code = -5;
			break;
		}
	} while (0);
	

	return oper_code;
}

long sip_svr_engine::load_nodelist()
{
	long oper_code = 0;
	do 
	{
		long ret_code = 0;
		xtXmlNodePtr root = config::instance()->m_config.getRoot();
		if (root.IsNull())
		{
			oper_code = -1;
			break;
		}
		xtXmlNodePtr sip_cfg_node = config::instance()->m_config.getNode(root, "sip_cfg");
		xtXmlNodePtr localuri_node = config::instance()->m_config.getNode(sip_cfg_node,"localuri");
		const char *localuri = config::instance()->m_config.getValue(localuri_node);
		//sip:xtrouter@172.16.5.19:5020
		node_info_t local_node;
		ret_code = parse_uri(string(localuri),local_node);
		if (ret_code < 0)
		{
			oper_code =-2;
			break;
		}
		m_local_uri = "sip:"+local_node.ids+"@"+local_node.ip+":"+local_node.port;
		//load node list
		xtXmlNodePtr node_list = config::instance()->m_config.getNode(sip_cfg_node,"node_list");
		std::string cfg_val;
		xtXmlNodePtr ctx = node_list.GetFirstChild("node_uri");
		for( ;!ctx.IsNull(); ctx = ctx.NextSibling("node_uri"))
		{
			const char *val = config::instance()->m_config.getValue(ctx);
			if (NULL == val)
			{
				continue;
			}
			cfg_val.clear();
			cfg_val.assign(val);

			node_info_t node_i;
			vector<node_info_t> node_path;
			int pos = -1;
			while ((pos = cfg_val.find('='))> 0)
			{
				ret_code = parse_uri(cfg_val.substr(0,pos),node_i);
				if (ret_code < 0)
				{
					oper_code = -3;
					break;
				}
				cfg_val.erase(0, pos+1);
				node_path.push_back(node_i);
			}
			ret_code = parse_uri(cfg_val,node_i);
			if (ret_code < 0)
			{
				oper_code = -4;
				break;
			}
			node_path.push_back(node_i);
			if (node_path.size() < 3)
			{
				oper_code = -5;
				break;
			}
			m_node_list.push_back(node_path);
		}
	} while (0);

	return oper_code;
}

void sip_svr_engine::on_client_invite_200ok(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len)
{
	sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
	if (NULL == sip_engine)
	{
		return;
	}

	std::string call_id;
	std::string target;
	std::string to;
	
	long ret_code = sip_engine->parse_msg_info(msg,call_id,target,to);
	if (ret_code < 0)
	{
		return;
	}

	WRITE_LOG(DBLOGINFO,ll_info,"call_id[%s] target[%s] to[%s] INVITE with sdp[%s]",
		call_id.c_str(), target.c_str(), to.c_str(), sdp);
	printf("client invite answer call_id[%s] target[%s] to[%s] INVITE with sdp[%s]",
		call_id.c_str(), target.c_str(), to.c_str(), sdp);

	xt_sip_client_invite_handle_t invite_handle_ =  ::xt_sip_client_invite_handle_clone(h);
	//收到客户端的200 ok时开始回复服务端并开始推流
	switchset_task* ptr_task = new switchset_task(invite_handle_,call_id,sdp,len);
	if (NULL != ptr_task)
	{
		ptr_task->process_event();
	}
}

void sip_svr_engine::on_client_invite_offer(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, const char *sdp, uint32_t len)
{
	printf("on_client_invite_offer\n");
}
void sip_svr_engine::on_client_invite_bye(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, xt_sip_invite_terminated_reason_t reason)
{
	sip_svr_engine *sip_engine = static_cast<sip_svr_engine *>(ctx);
    if (NULL == sip_engine)
    {
        return;
    }

    do 
    {
        std::string call_id;
        long ret_code = sip_engine->parse_call_id_by_msg(msg,call_id);
        if (ret_code < 0)
        {
            break;
        }

        WRITE_LOG(DBLOGINFO,ll_info,"call_id[%s] 收到BYE reason[%d]",call_id.c_str(), reason);
		//会话无论是被会话发起端或者会话接收端bye都会触发该回调，重复产生bye task
        invite_bye_task* ptr_task = new invite_bye_task(call_id,reason);
        if (NULL != ptr_task)
        {
            ptr_task->process_event();
        }
    } while (0);
}

void sip_svr_engine::on_client_invite_info(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg)
{
	printf("on_client_invite_info\n");
}
void sip_svr_engine::on_client_invite_info_response(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success)
{
	printf("on_client_invite_info_response\n");
}
void sip_svr_engine::on_client_invite_message(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg)
{
	printf("on_client_invite_message\n");
}
void sip_svr_engine::on_client_invite_message_response(void *ctx, xt_sip_client_invite_handle_t h, xt_sip_msg_handle_t msg, uint8_t success)
{
	printf("on_client_invite_message_response\n");
}
void sip_svr_engine::on_client_msg_prev_post(void *ctx, xt_sip_msg_handle_t msg)
{
	long cid = (long)ctx;
	long oper_code = 0;
	do 
	{
		std::string call_id;
		std::map<long,std_sip_session_t>::iterator itr = sip_svr_engine::_()->m_tmp_session_call.find(cid);
		if (itr!=sip_svr_engine::_()->m_tmp_session_call.end())
		{
			std_sip_session_t session = itr->second;
			long ret_code = sip_svr_engine::_()->parse_call_id_by_msg(msg,call_id);
			if (ret_code < 0)
			{
				printf("parse_call_id_by_msg failed\n");
				break;
			}
			session.call_id = call_id;
			ret_code = media_device::instance()->mod_ids_by_handle(session.dev_handle,call_id);
			if (ret_code < 0)
			{
				printf("mod_ids_by_handle call_id:%s failed\n",call_id.c_str());
				break;
			}
			ret_code = XTEngine::_()->mod_src_ids(session.srcno,call_id);
			if (ret_code < 0)
			{
				printf("mod_src_ids:%s failed\n",call_id.c_str());
				break;
			}
			sip_svr_engine::_()->m_tmp_session_call.erase(itr);
			sip_svr_engine::_()->add_session(session);
			//加入会话包活心跳，如果超时抛出回调，则清理相应媒体转发资源
			//sip_svr_engine::_()->add_options(session.to.c_str());
			//client端会话callid作为其对端会话的源
			std_sip_session_t svr_session;
			ret_code = sip_svr_engine::_()->get_session(session.recv_src_callid, svr_session);
			if (ret_code < 0)
			{
				printf("set session.recv_src_callid failed\n");
				break;
			}
			svr_session.recv_src_callid = session.call_id;
			ret_code = sip_svr_engine::_()->update_session(svr_session);
			if (ret_code < 0)
			{
				printf("update svr_session failed\n");
				break;
			}
		}
	} while (0);
}

// 根据msg解析出call_id
long sip_svr_engine::parse_call_id_by_msg(const xt_sip_msg_handle_t msg,std::string& call_id)
{
    long ret_code = 0;
    do 
    {
        //获取call id
        xt_sip_buf_handle_t cid = ::xt_sip_msg_get_call_id(msg);
        if (XT_SIP_INVALID_HANDLE == cid)
        {
            ret_code = -1;
            break;
        }
        const char * cid_data = static_cast<char*>(::xt_sip_buf_get_data(cid));
        if (NULL == cid_data)
        {
            ::xt_sip_buf_handle_delete(cid);
            ret_code = -2;
            break;
        }
        call_id.assign(cid_data,::xt_sip_buf_get_len(cid));
        ::xt_sip_buf_handle_delete(cid);
        ret_code=1;
    } while (0);

    return ret_code;
}
//根据msg解析出from
long sip_svr_engine::parse_from_by_msg(const xt_sip_msg_handle_t msg,std::string& from)
{
    long ret_code = 0;
    do 
    {
        xt_sip_buf_handle_t from_buf = ::xt_sip_msg_get_from(msg);
        if (XT_SIP_INVALID_HANDLE == from_buf)
        {
            ret_code = -1;
            break;
        }

        const char* from_data = static_cast<char*>(::xt_sip_buf_get_data(from_buf));
        if (NULL == from_data)
        {
            ::xt_sip_buf_handle_delete(from_buf);
            ret_code = -2;
            break;
        }
        from.assign(from_data,::xt_sip_buf_get_len(from_buf));
        ::xt_sip_buf_handle_delete(from_buf);

        ret_code = 1;
    } while (0);
    return ret_code;

}

//根据msg解析出to
long sip_svr_engine::parse_to_by_msg(const xt_sip_msg_handle_t msg,std::string& to)
{
    long ret_code = 0;
    do 
    {
        xt_sip_buf_handle_t tobuf = ::xt_sip_msg_get_to(msg);
        if (XT_SIP_INVALID_HANDLE == tobuf)
        {
            ret_code = -1;
            break;
        }
        const char* todata = static_cast<char*>(::xt_sip_buf_get_data(tobuf));
        if (NULL == todata)
        {
            ::xt_sip_buf_handle_delete(tobuf);
            ret_code = -2;
            break;
        }
        to.assign(todata,::xt_sip_buf_get_len(tobuf));
        ::xt_sip_buf_handle_delete(tobuf);

        ret_code = 1;
    } while (0);
    return ret_code;
}

//根据msg解析出content body
long sip_svr_engine::parse_content_body_by_msg(const xt_sip_msg_handle_t msg,std::string& ctx)
{
    long ret_code = 0;
    do 
    {
        xt_sip_buf_handle_t buf = ::xt_sip_msg_get_content_body(msg);
        if (XT_SIP_INVALID_HANDLE == buf)
        {
            ret_code = -1;
            break;
        }
        const char * data = static_cast<char*>(::xt_sip_buf_get_data(buf));
        if (NULL == data)
        {
            ret_code = -2;
            break;
        }
        uint32_t len_data = ::xt_sip_buf_get_len(buf);
        if (len_data <= 0)
        {
            ret_code = -3;
            break;
        }
        ctx.assign(data,len_data);
        ::xt_sip_buf_handle_delete(buf);

        ret_code = 1;
    } while (0);
    return ret_code;
}

// 回复opt ok
void sip_svr_engine::response_optstatus_ok()
{
    do 
    {
        std::string msg_ctx;
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
            std::ostringstream os;
            os<<200;
            response_.SetValue(os.str().c_str());
        }

        xtXmlNodePtr seq1 = Response.NewChild("seq");
        if (!seq1.IsNull())
        {
            std::ostringstream os;
            os<<1;
            seq1.SetValue(os.str().c_str());
        }
        msg_ctx.assign(res_xml.GetXMLStrEx());

        xt_sip_client_message_response_t tmp_response1 = { 0 };
        xt_sip_client_message_request_t	request1 = { 0 };
        request1.target ;//= /*from.c_str();*/ ;
        request1.content_type = "application/command+xml";
        request1.content = msg_ctx.c_str();
        request1.content_length = msg_ctx.length();
        if (XT_SIP_STATUS_OK != ::xt_sip_make_client_message(sip_hanle_, NULL, &request1, &tmp_response1, NULL, NULL,3000))
        {
            WRITE_LOG(DBLOGINFO,ll_error,"response_optstatus_ok:xt_sip_make_client_message fail",1);
        }

    } while (0);

}

//回复opt fail
void sip_svr_engine::response_optstatus_fail(const std::string& fail_case)
{
    do 
    {
        std::string msg_ctx;
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
            std::ostringstream os;
            os<<401;
            response_.SetValue(os.str().c_str());
        }

        xtXmlNodePtr seq1 = Response.NewChild("seq");
        if (!seq1.IsNull())
        {
            std::ostringstream os;
            os<<1;
            seq1.SetValue(os.str().c_str());
        }

        xtXmlNodePtr error = Response.NewChild("error");
        if (!error.IsNull())
        {
            error.SetValue(fail_case.c_str());
        }

        msg_ctx.assign(res_xml.GetXMLStrEx());
        xt_sip_client_message_response_t tmp_response1 = { 0 };
        xt_sip_client_message_request_t	request1 = { 0 };
        request1.target ;//= /*from.c_str();*/ ;
        request1.content_type = "application/command+xml";
        request1.content = msg_ctx.c_str();
        request1.content_length = msg_ctx.length();
        if (XT_SIP_STATUS_OK != ::xt_sip_make_client_message(sip_hanle_, NULL, &request1, &tmp_response1, NULL, NULL,3000))
        {
            WRITE_LOG(DBLOGINFO,ll_error,"response_optstatus_fail:xt_sip_make_client_message fail",1);
        }

    } while (0);

}

//回复chan status
void sip_svr_engine::response_chanstatus(char* ch_status)
{
    /*
    <Notify>
    <commandname>"chanstatus"</commandname>
    <seq>1</seq>
    <devname>"member1"</devname>
    <devid>"00000000001"</devid>
    <groupid>"123"</groupid>
    <channelstatus>establish/cleanup</channelstatus>
    <chanid>1</chanid>
    </Notify>
    */
    std::string msg_ctx;
    xtXml res_xml;
    res_xml.LoadXMLStr("<Notify></Notify>");
    xtXmlNodePtr Notify = res_xml.getRoot();
    if (Notify.IsNull())
    {
        return;
    }

    xtXmlNodePtr commandname = Notify.NewChild("commandname");
    if (!commandname.IsNull())
    {
        commandname.SetValue("chanstatus");
    }

    xtXmlNodePtr seq = Notify.NewChild("seq");
    if (!seq.IsNull())
    {
        std::ostringstream os;
        os<<1;
        seq.SetValue(os.str().c_str());
    }

    xtXmlNodePtr devname = Notify.NewChild("devname");
    if (!devname.IsNull())
    {
        devname.SetValue("xtrouter");
    }

    xtXmlNodePtr groupid = Notify.NewChild("groupid");
    if (!groupid.IsNull())
    {
        std::ostringstream os;
        os<<123;
        groupid.SetValue(os.str().c_str());
    }

    xtXmlNodePtr channelstatus = Notify.NewChild("channelstatus");
    if (!channelstatus.IsNull())
    {
        channelstatus.SetValue("ch_status");
    }

    xtXmlNodePtr chanid = Notify.NewChild("chanid");
    if (!chanid.IsNull())
    {
        std::ostringstream os;
        os<<0;
        chanid.SetValue(os.str().c_str());
    }
    msg_ctx.assign(res_xml.GetXMLStrEx());
    xt_sip_client_message_response_t tmp_response1 = { 0 };
    xt_sip_client_message_request_t	request1 = { 0 };
    request1.target ;//= /*from.c_str();*/ ;
    request1.content_type = "application/command+xml";
    request1.content = msg_ctx.c_str();
    request1.content_length = msg_ctx.length();
    if (XT_SIP_STATUS_OK != ::xt_sip_make_client_message(sip_hanle_, NULL, &request1, &tmp_response1, NULL, NULL,3000))
    {
        WRITE_LOG(DBLOGINFO,ll_error,"response_chanstatus:xt_sip_make_client_message fail",1);
    }
}

long sip_svr_engine::exist_session(const std_sip_session_t session,string &org_callid)
{
	boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
	map<string,std_sip_session_t>::iterator itr = std_sip_session_mgr_lst.begin();
	for (;itr!=std_sip_session_mgr_lst.end();itr++)
	{
		if (itr->second.from == session.from && itr->second.to == session.to)
		{
			org_callid = itr->second.call_id;
			return 1;
		}
	}

	return -1;
}

//会话管理
void sip_svr_engine::add_session(const std_sip_session_t& session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_handle_t itr = std_sip_session_mgr_lst.find(session.call_id);
    if ( std_sip_session_mgr_lst.end() == itr)
    {
        std_sip_session_mgr_lst[session.call_id] = session;
    }
}
void sip_svr_engine::del_session(const std_sip_session_t& session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_handle_t itr = std_sip_session_mgr_lst.find(session.call_id);
    if ( std_sip_session_mgr_lst.end() != itr)
    {
        std_sip_session_mgr_lst.erase(itr++);
    }
}
void sip_svr_engine::del_session(const std::string& call_id)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_handle_t itr = std_sip_session_mgr_lst.find(call_id);
    if ( std_sip_session_mgr_lst.end() != itr)
    {
        std_sip_session_mgr_lst.erase(itr++);
    }
}

long sip_svr_engine::end_session(std_sip_session_t session)
{
	long oper_code = 0;
	do 
	{
		sip_svr_engine::_()->del_session(session);
		if (session.invite_type == SIP_INVITE_CLIENT&&session.hclient)
		{
			::xt_sip_client_invite_end(session.hclient);
			::xt_sip_client_invite_handle_delete(session.hclient);
		}
		else if (session.invite_type == SIP_INVITE_SERVER&&session.hserver)
		{
			::xt_sip_server_invite_end(session.hserver);
			::xt_sip_server_invite_handle_delete(session.hserver);
		}

		long ret_code = XTEngine::_()->destroy_src(session.srcno);
		if (ret_code < 0 )
		{
			oper_code = -2;
			printf("pro bye destroy_src fail!");
			break;
		}

		ret_code = XTEngine::_()->rtp_close_recv(session.dev_handle);
		if (ret_code < 0)
		{
			oper_code = -3;
			break;
		}
	} while (0);
	

	return 0;
}

long sip_svr_engine::get_session(const std::string& call_id,std_sip_session_t& session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_handle_t itr = std_sip_session_mgr_lst.find(call_id);
    if ( std_sip_session_mgr_lst.end() != itr)
    {
        session = itr->second;
        return 0;
    }
    return -1;
}
long sip_svr_engine::get_session_ids(std::string ids_to,std_sip_session_t &session)
{
	boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
	sip_session_mgr_container_handle_t itr = std_sip_session_mgr_lst.begin();
	for ( ;std_sip_session_mgr_lst.end() != itr;itr++)
	{
		if (itr->second.ids_to == ids_to)
		{
			session = itr->second;
			return 0;
		}
	}
	return -1;
}

long sip_svr_engine::update_session(const std_sip_session_t& session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_handle_t itr = std_sip_session_mgr_lst.find(session.call_id);
    if ( std_sip_session_mgr_lst.end() == itr)
    {
        return -1;
    }

    itr->second = session;
    return 1;
}


std_sip_session_type_t sip_svr_engine::get_session_type(const std::string& call_id)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_handle_t itr = std_sip_session_mgr_lst.find(call_id);
    if ( std_sip_session_mgr_lst.end() != itr)
    {
        return SIP_REINVITE;
    }
    return SIP_INVITE;
}

//根据sdp解析sdp的方向
sdp_direction_t sip_svr_engine::parse_sdp_direction(const std::string& sdp)
{
    sdp_direction_t ret_dir = dir_na;
    do 
    {
        xt_sdp::parse_buffer_t sdp_buffer(sdp.c_str(),sdp.length());
        xt_sdp::sdp_session_t sdp_ctx;
        try
        {
            sdp_ctx.parse(sdp_buffer);
        }
        catch(...)
        {
            printf("parse_sdp_direction catch\n");
            break;
        }

        bool is_exists_sendonly = false;
        bool is_exists_recvonly = false;
        bool is_exists_sendrecv = false;

        //写在媒体里以媒体里的为准
        xt_sdp::sdp_session_t::medium_container_t::iterator itr;
        for(itr = sdp_ctx.media_.begin(); sdp_ctx.media_.end() != itr; ++itr)
        {
            if (itr->attribute_helper_.exists("sendonly"))
            {
                is_exists_sendonly = true;
            }

            if (itr->attribute_helper_.exists("recvonly"))
            {
                is_exists_recvonly = true;
            }

            if (itr->attribute_helper_.exists("sendrecv"))
            {
                is_exists_sendrecv = true;
            }
        }

        //写在session里
        if (!is_exists_sendonly && !is_exists_recvonly && !is_exists_sendrecv)
        {
            if (sdp_ctx.attribute_helper_.exists("sendonly"))
            {
                is_exists_sendonly = true;
            }

            if (sdp_ctx.attribute_helper_.exists("recvonly"))
            {
                is_exists_recvonly = true;
            }

            if (sdp_ctx.attribute_helper_.exists("sendrecv"))
            {
                is_exists_sendrecv = true;
            }
        }

        //双向
        if ( (is_exists_recvonly && is_exists_sendonly) || is_exists_sendrecv)
        {
            ret_dir = dir_sendrecv;
            break;
        }

        //单向接收
        if (is_exists_recvonly && !is_exists_sendonly)
        {
            ret_dir = dir_recvonly;
            break;
        }

        //单向发送
        if (is_exists_sendonly && !is_exists_recvonly)
        {
            ret_dir = dir_sendonly;
            break;
        }


    } while (0);

    return ret_dir;
}

//功能调度接口
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
long sip_svr_engine::add_send_dst(const int srcno,const std::string& dst_sdp)
{
    long oper_code=0;
    do 
    {
		long ret_code = 0;
        xt_sdp::parse_buffer_t pb(dst_sdp.c_str(), dst_sdp.length());
        xt_sdp::sdp_session_t xsdp;
        try
        {
            xsdp.parse(pb);

        }
        catch(...)
        {
			oper_code = -1;
			break;
        }

        svr_info svr[MAX_TRACK];
        int tracknum = xsdp.media_.size();
        ret_code = media_server::get_svr_info(svr,tracknum,srcno);
		if (ret_code < 0)
		{
			oper_code = -2;
			break;
		}
        std::list<std::string> lsta;
        bool is_send = false;
        xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
        for (;xsdp.media_.end() != itr; ++itr)
        {
            rtp_dst_info_t send_dst;

            if (itr->connections_.empty())
            {
                send_dst.dst_ip = xsdp.connection_.address_;

            }
            else
            {
                xt_sdp::sdp_session_t::connection_t c = itr->connections_.front();
                send_dst.dst_ip = c.address();
            }

            send_dst.dst_port = (unsigned short)itr->port_;

            for(int i=0; i<tracknum; ++i)
            {

                if (0 != itr->name_.compare(svr[i].trackname))
                {
                    continue;
                }

                send_dst.trackid = svr[i].trackid;

                if (itr->attribute_helper_.exists("rtpport-mux"))
                {
                    send_dst.dst_demux = true;
                    lsta = itr->attribute_helper_.get_values("muxid");
                    if (!lsta.empty())
                    {
                        send_dst.dst_demuxid = ::str_to_num<int>(lsta.front().c_str());
                    }
                    lsta.clear();
                }
                else
                {
                    send_dst.dst_demux = false;
                    send_dst.dst_demuxid = 0;
                }

                WRITE_LOG(DBLOGINFO,ll_info,"sip_svr_engine::add_send_dst srcno[%d] trackid[%d] send_ip[%s] sendport[%d] demux[%d] muxid[%d]",
                    srcno,send_dst.trackid,send_dst.dst_ip.c_str(),send_dst.dst_port,send_dst.dst_demux,send_dst.dst_demuxid);

                ret_code = media_server::add_send(srcno,send_dst.trackid,send_dst.dst_ip.c_str(),send_dst.dst_port,send_dst.dst_demux,send_dst.dst_demuxid);
                if (ret_code < 0)
                {
                    continue;
                }

                is_send = true;
            }

        }
        //没有任何流被推送出去
        if (!is_send)
        {
            WRITE_LOG(DBLOGINFO,ll_error,"没有任何流被推送出去",1);
            oper_code = -5;
            break;
        }

        oper_code=1;
    } while (0);
    return oper_code;
}

long sip_svr_engine::del_send_dst(const int srcno,const std::string& dst_sdp)
{
	long ret_code=0;
	do 
	{
		xt_sdp::parse_buffer_t pb(dst_sdp.c_str(), dst_sdp.length());
		xt_sdp::sdp_session_t xsdp;
		try
		{
			xsdp.parse(pb);
		}
		catch(...)
		{
			printf("del_send_dst::parse sdp catch\n");
			ret_code = -10;
			break;
		}

		svr_info svr[MAX_TRACK];
		int tracknum = xsdp.media_.size();
		ret_code = media_server::get_svr_info(svr,tracknum,srcno);
		std::list<std::string> lsta;

		xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
		for (;xsdp.media_.end() != itr; ++itr)
		{
			rtp_dst_info_t send_dst;

			if (itr->connections_.empty())
			{
				send_dst.dst_ip = xsdp.connection_.address_;

			}
			else
			{
				xt_sdp::sdp_session_t::connection_t c = itr->connections_.front();
				send_dst.dst_ip = c.address();
			}

			send_dst.dst_port = (unsigned short)itr->port_;

			for(int i=0; i<tracknum; ++i)
			{

				if (0 != itr->name_.compare(svr[i].trackname))
				{
					continue;
				}

				send_dst.trackid = svr[i].trackid;

				if (itr->attribute_helper_.exists("rtpport-mux"))
				{
					send_dst.dst_demux = true;
					lsta = itr->attribute_helper_.get_values("muxid");
					if (!lsta.empty())
					{
						send_dst.dst_demuxid = ::str_to_num<int>(lsta.front().c_str());
					}
					lsta.clear();
				}
				else
				{
					send_dst.dst_demux = false;
					send_dst.dst_demuxid = 0;
				}

				WRITE_LOG(DBLOGINFO,ll_info,"common_ctrl_msg_mgr::del_send_dst srcno[%d] trackid[%d] send_ip[%s] sendport[%d] demux[%d] muxid[%d]",
					srcno,send_dst.trackid,send_dst.dst_ip.c_str(),send_dst.dst_port,send_dst.dst_demux,send_dst.dst_demuxid);

				ret_code = media_server::del_send(srcno,send_dst.trackid,send_dst.dst_ip.c_str(),send_dst.dst_port,send_dst.dst_demux,send_dst.dst_demuxid);
				if (ret_code < 0)
				{
					continue;
				}
			}

		}

		ret_code=1;
	} while (0);
	return ret_code;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
