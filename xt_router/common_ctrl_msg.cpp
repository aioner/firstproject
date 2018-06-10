#include "common_ctrl_msg.h"

#ifdef _USE_COMMON_CTRL_MSG_
#include "Router_config.h"
#include "std_sip_task.h"
#include "XTEngine.h"
#include "rtpid_mgr.h"
#include "XTRouterLog.h"
common_ctrl_msg_mgr common_ctrl_msg_mgr::my_obj_;

void common_ctrl_msg_mgr::onLinkServerImp_cb(int serverNum, int operateResult, const char* comment)
{
    if(operateResult == 0)
	{
		//这里包含连接中心失败，中心正常或者异常下线情况，清理已建立会话以及转发资源
		std::cout << "login sip center failed!"<< std::endl;
		_()->clear_all();
	}
    else
	{
        std::cout << "login sip center success!"<< std::endl;
	}
}

void common_ctrl_msg_mgr::onLockControlImp_cb(const char* ids, int type, int operateResult)
{

}

void common_ctrl_msg_mgr::onReceiveLockStateImp_cb(struct LockState* states, int length)
{

}

void common_ctrl_msg_mgr::onReceiveTransCommandImp_cb(const char* ids, const char* command)
{
    printf("onReceiveTransCommandImp::ids:%s\n command:%s\n", ids, command);

    long oper_code = -1;
    do 
    {
        std::string msg_ctx;
        msg_ctx.assign(command);
        WRITE_LOG(DBLOGINFO,ll_info,"on_server_message_arrived[%s]",msg_ctx.c_str());

        xtXml xml;
        xml.LoadXMLStr(msg_ctx.c_str());

        xtXmlNodePtr root = xml.getRoot();
        if (root.IsNull())
        {
            break;
        }

        const char *root_v = root.GetName();
        if (::strcmp(root_v,"Control")==0)
        {
            xtXmlNodePtr command = xml.getNode(root, "commandname");
            if (command.IsNull())
            {
                break;
            }
            const char *commandname = xml.getValue(command);
            if (::strcmp(commandname,"\"switchset\"")==0)
            {
                xtXmlNodePtr ptr_seq = xml.getNode(root, "seq");
                if (ptr_seq.IsNull())
                {
                    break;
                }
                std::string seq = xml.getValue(ptr_seq);

                xtXmlNodePtr ptr_max_dst =  xml.getNode(root, "max-dst");
                if (ptr_max_dst.IsNull())
                {
                    break;
                }

                int max_dst = INT_VALUE_NA;
                const char* max_dst_v = xml.getValue(ptr_max_dst);
                if (NULL == max_dst_v)
                {
                    break;
                }
                else
                {
                    max_dst = ::str_to_num<int>(max_dst_v);
                }

                xtXmlNodePtr ptr_src =  xml.getNode(root, "src");
                if (ptr_src.IsNull())
                {
                    break;
                }

                xtXmlNodePtr ptr_src_rtp_id = ptr_src.GetFirstChild("rtpid");
                if (ptr_src_rtp_id.IsNull())
                {
                    break;
                }
                rtp_id_t src_rtp_id = RTP_ID_VALUE_NA;
                const char* src_rtp_id_v = xml.getValue(ptr_src_rtp_id);
                if (NULL == src_rtp_id_v)
                {
                    break;
                }
                else
                {
                    //去掉前后引号
                    char tmp[32]={0x0};
                    int len = strlen(src_rtp_id_v);
                    memcpy(tmp, src_rtp_id_v+1,len-2);
                    tmp[len-2]='\0';

                    src_rtp_id = ::str_to_num<rtp_id_t>(tmp);
                }

                switchset_data_t swt_data;
                swt_data.src_rtp_id = src_rtp_id;

                xtXmlNodePtr ptr_dst =  xml.getNode(root, "dst");
                if (ptr_dst.IsNull())
                {
                    break;
                }
                xtXmlNodePtr ptr_dst_rtp_id = ptr_dst.GetFirstChild("rtpid");
                if (ptr_dst_rtp_id.IsNull())
                {
                    break;
                }
                for( ;!ptr_dst_rtp_id.IsNull(); ptr_dst_rtp_id = ptr_dst_rtp_id.NextSibling("rtpid"))
                {
                    const char *val = xml.getValue(ptr_dst_rtp_id);
                    if (NULL == val)
                    {
                        continue;
                    }
                    //去掉前后引号
                    char tmp[32]={0x0};
                    int len = strlen(val);
                    memcpy(tmp, val+1,len-2);
                    tmp[len-2]='\0';

                    rtp_id_t dst_rtp_id = ::str_to_num<rtp_id_t>(tmp);
                    swt_data.dst_rtp_ids.push_back(dst_rtp_id);
                }

                if (swt_data.dst_rtp_ids.empty())
                {
                    break;
                }

                switchset_task_sip* ptr_task = new switchset_task_sip(swt_data);
                if (NULL != ptr_task)
                {
                    ptr_task->process_event();
                }
            }
        }

        oper_code = 1;
    } while (0);

    if (oper_code < 0)
    {
        WRITE_LOG(DBLOGINFO,ll_error,"onReceiveTransCommandImp::pares xml error!", 1);
        _()->response_optstatus_fail("pares xml error!");
    }
}

void  common_ctrl_msg_mgr::onUserInOutImp_cb(struct ResourceState* states, int length)
{
    for(int i = 0; i < length; i++)
    {
        std::cout << states[i].xtids << " " << states[i].state 
            << " " << states[i].serverIDs << std::endl;
    }
}

void  common_ctrl_msg_mgr::onSendInviteImp_cb(const char* xtids, const char* sdp, const char* sessionID, int operateResult,void* extendObj)
{
    do 
    {
        if (operateResult != 1)
        {
            break;
        }

        //JK发消息
        if (NULL != extendObj)
        {

        }
        else //SIP消息
        {

        }

    } while (0);
}
void  common_ctrl_msg_mgr::onReceiveInviteImp_cb(const char* ids, const char* sdp, const char* sessionID,void* extendObj)
{
    printf("onReceiveInviteImp::ids:%s sessionID:%s\n sdp:%s  extendobj:%d\n", ids, sessionID, sdp, extendObj);

    std_sip_session_t session;
    session.creat_time = boost::posix_time::second_clock::local_time();

    //获取call id
    session.call_id.assign(sessionID);

    //一次SIP_INVITE回话对应一个sessionid，否则为SIP_REINVITE
    if (INVITE_SESSION == _()->get_session_type(session.call_id))
    {
        _()->add_session(session);
        invite_task_sip* ptr_task = new invite_task_sip(ids, sdp, session);
        if (NULL != ptr_task)
        {
            ptr_task->process_event();
        }
    }
    else
    {
        re_invite_task_sip* ptr_task = new re_invite_task_sip(ids, sdp, sessionID);
        if (NULL != ptr_task)
        {
            ptr_task->process_event();
        }
    }
}

void  common_ctrl_msg_mgr::onTerminateInviteImp_cb(const char* sessionID, int operateResult)
{

    do 
    {

    } while (0);
}

void  common_ctrl_msg_mgr::onReceiveTerminateInviteImp_cb(const char* sessionID)
{

    printf("onReceiveTerminateInviteImp\n");

    std::string call_id;
    call_id.assign(sessionID);

    invite_bye_task_sip* ptr_task = new invite_bye_task_sip(call_id);
    if (NULL != ptr_task)
    {
        ptr_task->process_event();
    }
}

void common_ctrl_msg_mgr::onReceiveACKImp_cb(const char* sessionID, const char* sdp)
{
    printf("onReceiveACKImp::sessionID:%s\n sdp:%s\n", sessionID, sdp);

    //获取call id
    std::string call_id;
    call_id.assign(sessionID);

    std::string _sdp;
    _sdp.assign(sdp);

    if (!_sdp.empty())
    {
        ack_with_sdp_task_sip* ptr_task = new ack_with_sdp_task_sip(call_id,_sdp);
        if (NULL != ptr_task)
        {
            ptr_task->process_event();
        }
    }
    else
    {
        WRITE_LOG(DBLOGINFO,ll_info,"onReceiveACKImp with no sdp", 1);
    }
}

bool common_ctrl_msg_mgr::init_ctrl_msg()
{
    long ret = load_cfg();
    if (ret < 0)
    {
        printf("load_cfg() failed:%d\n", ret);
        return false;
    }
    start();

    login();

    return true;
}

bool common_ctrl_msg_mgr::uninit_ctrl_msg()
{
    ::stopLinkServer(server_num);
    ::unInitialize();
    return true;
}

int common_ctrl_msg_mgr::load_cfg()
{
    do 
    {
        client_ip = config::instance()->local_ip("0.0.0.0");
        int transport = config::_()->sip_cfg_transport(5060);

        char tmp[1024]={0};
        sprintf(tmp, "%d", transport);
        client_port.assign(tmp);

        user_name =  config::_()->sip_cfg_usre("");
        if (user_name.empty())
        {
            break;
        }
        pass_word = config::_()->sip_cfg_password("");
        if (pass_word.empty())
        {
            break;
        }

		demux_s = config::_()->demux_s(0)?true:false;

        std::list<std::string> domains;
        config::instance()->sip_cfg_get_domain(domains);
        if (domains.empty())
        {
            break;
        }

        std::string domain = domains.front();
        domains.pop_front();
        //std::string back_domains = domains; 

        int pos = domain.find(':');
        int len = domain.length();
        server_port = atoi(domain.substr(pos+1).c_str());
        server_ip = domain.erase(pos, len-pos);

        sipid = config::_()->sip_cfg_serverids("");
        if (sipid.empty())
        {
            break;
        }
        commlib_type = config::_()->sip_cfg_commlibtype("SIP");

    } while (0);

    return 0;
}

void common_ctrl_msg_mgr::start()
{
    WRITE_LOG(DBLOGINFO,ll_info,"common_ctrl_msg_mgr::start()!\n",1);

    printf("commlibtype:%s\n", commlib_type.c_str());
    ::initialize(commlib_type.c_str());

    //::setParam("NeedRecordLog", "true");//开启通信库日志

    setCallback();
}

void common_ctrl_msg_mgr::setCallback()
{
    ::setCallbackOnLinkServer(onLinkServerImp_cb);
    ::setCallbackOnLockControl(onLockControlImp_cb);
    ::setCallbackOnReceiveLockState(onReceiveLockStateImp_cb);
    ::setCallbackOnReceiveTransCommand(onReceiveTransCommandImp_cb);
    ::setCallbackOnUserInOut(onUserInOutImp_cb);
    ::setCallbackOnSendInvite(onSendInviteImp_cb);
    ::setCallbackOnReceiveInvite(onReceiveInviteImp_cb);
    ::setCallbackOnTerminateInvite(onTerminateInviteImp_cb);
    ::setCallbackOnReceiveTerminateInvite(onReceiveTerminateInviteImp_cb);
    ::setCallbackOnReceiveACK(onReceiveACKImp_cb);
}

void common_ctrl_msg_mgr::login()
{
    printf("%s %s %s %s %s %s %d\n", user_name.c_str(), pass_word.c_str(), 
        client_ip.c_str(), client_port.c_str(),
        sipid.c_str(), server_ip.c_str(), server_port);

    server_num = 0;

    ::setParam("UserName", user_name.c_str());
    ::setParam("Password", pass_word.c_str());
    ::setParam("ClientIP", client_ip.c_str());
    ::setParam("ClientPort", client_port.c_str()); // 登陆中心
    ::startLinkServer(sipid.c_str(), server_ip.c_str(), server_port, server_num);
}


void common_ctrl_msg_mgr::answer_Invite(const char* tempSessionID, unsigned int code, const char* sdp)
{
	::answerInvite(tempSessionID, code, sdp);
}

void common_ctrl_msg_mgr::response_optstatus_ok()
{
    do 
    {
		//加一个xml头
        std::string msg_ctx="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
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

		xtXmlNodePtr seq1 = Response.NewChild("seq");
		if (!seq1.IsNull())
		{
			std::ostringstream os;
			os<<1;
			seq1.SetValue(os.str().c_str());
		}

        xtXmlNodePtr response_ = Response.NewChild("response");
        if (!response_.IsNull())
        {
            std::ostringstream os;
            os<<200;
            response_.SetValue(os.str().c_str());
        }

		msg_ctx += std::string(res_xml.GetXMLStrEx());

        ::sendTransparentCommandToCenter(msg_ctx.c_str());

    } while (0);

}
void common_ctrl_msg_mgr::response_optstatus_fail(const std::string& fail_case)
{
    do 
    {
        std::string msg_ctx="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
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

		xtXmlNodePtr seq1 = Response.NewChild("seq");
		if (!seq1.IsNull())
		{
			std::ostringstream os;
			os<<1;
			seq1.SetValue(os.str().c_str());
		}

        xtXmlNodePtr response_ = Response.NewChild("response");
        if (!response_.IsNull())
        {
            std::ostringstream os;
            os<<401;
            response_.SetValue(os.str().c_str());
        }

        xtXmlNodePtr error = Response.NewChild("error");
        if (!error.IsNull())
        {
            error.SetValue(fail_case.c_str());
        }

		msg_ctx += std::string(res_xml.GetXMLStrEx());

        ::sendTransparentCommandToCenter(msg_ctx.c_str());

    } while (0);

}

void common_ctrl_msg_mgr::response_chanstatus(char* ch_status)
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
    std::string msg_ctx="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
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

	xtXmlNodePtr devid = Notify.NewChild("devid");
	if (!devid.IsNull())
	{
		devid.SetValue(sipid.c_str());//目前写中心的sipid
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
        channelstatus.SetValue(ch_status);
    }

    xtXmlNodePtr chanid = Notify.NewChild("chanid");
    if (!chanid.IsNull())
    {
        std::ostringstream os;
        os<<0;
        chanid.SetValue(os.str().c_str());
    }
	msg_ctx +=std::string(res_xml.GetXMLStrEx());

    ::sendTransparentCommandToCenter(msg_ctx.c_str());
}

void common_ctrl_msg_mgr::clear_all()
{
	sip_session_mgr_container_iterator_t itr_sip = sip_session_mgr_lst.begin();
	for ( ;sip_session_mgr_lst.end() !=itr_sip;)
	{
		int oper_code = 0;
		std::string reason;

		std_sip_session_t session = itr_sip->second;
		rtp_id_attr_t rtp_id_attr;
		std_sip_session_t::rtp_id_container_handle_t itr_rtpid = session.send_rtpid.begin();
		for(;session.send_rtpid.end() != itr_rtpid; ++itr_rtpid)
		{
			oper_code = rtp_id_mr::_()->get_rtpid_attr(*itr_rtpid,rtp_id_attr);
			if (oper_code < 0)
			{
				oper_code = -2;
				reason.assign("pro bye get_rtp_id_attr fail!");
				//break;
			}
			//根据源id销毁转发源，删除相关回话与发送单元
			oper_code = XTEngine::_()->destroy_src(rtp_id_attr.srcno);
			if (oper_code < 0 )
			{
				oper_code = -3;
				reason.assign("pro bye destroy_src fail!");
				//break;
			}
		}

		itr_rtpid = session.recv_rtpid.begin();
		for (; session.recv_rtpid.end() != itr_rtpid; ++itr_rtpid)
		{
			oper_code = rtp_id_mr::_()->get_rtpid_attr(*itr_rtpid,rtp_id_attr);
			if (oper_code < 0)
			{
				oper_code = -2;
				reason.assign("pro bye get_rtp_id_attr fail!");
				//break;
			}
			//根据dev_handle关闭捕获数据数据，数据不会抛上来转发
			oper_code = XTEngine::_()->rtp_close_recv(rtp_id_attr.dev_handle);
		}
		//////////////////////////////////////////////////////////////////////
		//$注意：如果交换重启或者bye之后中心未清理对应rtpid资源，rtpid对应不上导致转发失败
		//rtpid管理器清理
		rtp_dst_info_t dst_info;
		itr_rtpid = session.send_rtpid.begin();
		for(;session.send_rtpid.end() != itr_rtpid; ++itr_rtpid)
		{
			rtp_id_mr::_()->del_rtp_dst_to_rtpid(*itr_rtpid, dst_info);
			rtp_id_mr::_()->free_rtpid(*itr_rtpid);
		}

		itr_rtpid = session.recv_rtpid.begin();
		for (; session.recv_rtpid.end() != itr_rtpid; ++itr_rtpid)
		{
			rtp_id_mr::_()->free_rtpid(*itr_rtpid);
		}
		//会话管理器清理
		//common_ctrl_msg_mgr::_()->del_session(session);

		sip_session_mgr_lst.erase(itr_sip++);
	}
}
//会话管理
void common_ctrl_msg_mgr::add_session(const std_sip_session_t& session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_iterator_t itr = sip_session_mgr_lst.find(session.call_id);
    if ( sip_session_mgr_lst.end() == itr)
    {
        sip_session_mgr_lst[session.call_id] = session;
    }

}
void common_ctrl_msg_mgr::del_session(const std_sip_session_t& session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_iterator_t itr = sip_session_mgr_lst.find(session.call_id);
    if ( sip_session_mgr_lst.end() != itr)
    {
        sip_session_mgr_lst.erase(itr++);
    }

}
void common_ctrl_msg_mgr::del_session(const std::string& call_id)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_iterator_t itr = sip_session_mgr_lst.find(call_id);
    if ( sip_session_mgr_lst.end() != itr)
    {
        sip_session_mgr_lst.erase(itr++);
    }

}
long common_ctrl_msg_mgr::get_session(const std::string& call_id,std_sip_session_t& session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_iterator_t itr = sip_session_mgr_lst.find(call_id);
    if ( sip_session_mgr_lst.end() != itr)
    {
        session = itr->second;
        return 0;
    }
    return -1;
}

void common_ctrl_msg_mgr::add_send_rtpid_to_session(const std::string& call_id,const rtp_id_t& rtp_id)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_iterator_t itr = sip_session_mgr_lst.find(call_id);
    if ( sip_session_mgr_lst.end() == itr)
    {
        return;
    }
    itr->second.send_rtpid.push_back(rtp_id);
}

void common_ctrl_msg_mgr::add_recv_rtpid_to_session(const std::string& call_id,const rtp_id_t& rtp_id)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_iterator_t itr = sip_session_mgr_lst.find(call_id);
    if ( sip_session_mgr_lst.end() == itr)
    {
        return;
    }
    itr->second.recv_rtpid.push_back(rtp_id);
    return;
}

long common_ctrl_msg_mgr::update_session(const std_sip_session_t& session)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_iterator_t itr = sip_session_mgr_lst.find(session.call_id);
    if ( sip_session_mgr_lst.end() == itr)
    {
        return -1;
    }

    itr->second = session;
    return 1;
}

//会话类型：invite，re-invite
sip_session_type common_ctrl_msg_mgr::get_session_type(const std::string& call_id)
{
    boost::unique_lock<boost::shared_mutex> lock(session_mgr_mutex_);
    sip_session_mgr_container_iterator_t itr = sip_session_mgr_lst.find(call_id);
    if ( sip_session_mgr_lst.end() != itr)
    {
        return REINVITE_SESSION;
    }
    return INVITE_SESSION;
}

sdp_direction_t common_ctrl_msg_mgr::parse_sdp_direction(const std::string& sdp)
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
long common_ctrl_msg_mgr::add_send_dst(const int srcno,const rtp_id_t rtpid,const std::string& dst_sdp)
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
            printf("add_send_dst::parse sdp catch\n");
            ret_code = -10;
            break;
        }

		//复用标识和复用id在会话层
		std::list<std::string> lsta;
		bool dst_demux = false;
		unsigned int dst_demuxid = 0;
		if (xsdp.exists("rtpport-mux"))
		{
			dst_demux = true;
			lsta = xsdp.get_values("muxid");
			if (!lsta.empty())
			{
				dst_demuxid = ::str_to_num<int>(lsta.front().c_str());
			}
			lsta.clear();
		}
		else
		{
			dst_demux = false;
			dst_demuxid = 0;
		}

        svr_info svr[MAX_TRACK];
        int tracknum = xsdp.media_.size();
        ret_code = media_server::get_svr_info(svr,tracknum,srcno);
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
				//媒体层端口复用属性
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
				send_dst.dst_demux = dst_demux;
				send_dst.dst_demuxid = dst_demuxid;

                WRITE_LOG(DBLOGINFO,ll_info,"common_ctrl_msg_mgr::add_send_dst srcno[%d] trackid[%d] send_ip[%s] sendport[%d] demux[%d] muxid[%d]",
                    srcno,send_dst.trackid,send_dst.dst_ip.c_str(),send_dst.dst_port,send_dst.dst_demux,send_dst.dst_demuxid);

                ret_code = media_server::add_send(srcno,send_dst.trackid,send_dst.dst_ip.c_str(),send_dst.dst_port,send_dst.dst_demux,send_dst.dst_demuxid);
                if (ret_code < 0)
                {
                    continue;
                }

                rtp_id_mr::_()->add_rtp_dst_to_rtpid(rtpid,send_dst);
                is_send = true;
            }

        }
        //没有任何流被推送出去
        if (!is_send)
        {
            printf("没有任何流被推送出去\n");
            ret_code = -5;
            break;
        }

        ret_code=1;
    } while (0);
    return ret_code;
}

long common_ctrl_msg_mgr::del_send_dst(const int srcno,const rtp_id_t rtpid,const std::string& dst_sdp)
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

                rtp_id_mr::_()->del_rtp_dst_to_rtpid(rtpid,send_dst);
            }

        }

        ret_code=1;
    } while (0);
    return ret_code;
}
#endif //#ifdef USE_COMMON_CTRL_MSG_
///////////////////////////////////////////////////////////////////////////////////////////////////////////////