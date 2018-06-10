#include "sip_svr_task.h"
#include "XTEngine.h"
#include "rtpid_mgr.h"

#include "XTRouterLog.h"

uint32_t recv_invite_task::run()
{
    long oper_code = 0;
   
    do 
    {
		std::string ok_200_with_sdp;
		int srcno = -1;
		dev_handle_t dev_handle = -1;
        //创建接收机制与转发源，返回接收机制与转发源创建时返回组合sdp
        long ret_code = XTEngine::_()->sip_2_sip_create_free_transmit_channel(session_.call_id,session_.call_id,-1,0,0,false,ok_200_with_sdp,srcno,dev_handle);
        if (ret_code < 0)
        {
            oper_code = -1;
            break;
        }

		session_.srcno = srcno;
		session_.dev_handle = dev_handle;
		//删除与请求sdp相同方向的媒体
		ret_code = sip_svr_engine::_()->one_part_sdp(session_.sdp,ok_200_with_sdp,true);
		if (ret_code < 0)
		{
			oper_code = -2;
			printf("one_part_sdp failed\n");
			break;
		}
		//更新session中收发sdp信息
		session_.router_sdp = ok_200_with_sdp;//保留修改前sdp
		ret_code = sip_svr_engine::_()->update_session(session_);
		if (ret_code < 0)
		{
			oper_code = -3;
			break;
		}

		//查询是否重复点播，向下一级客户端发出invite
		std_sip_session_t src_session;
		ret_code = sip_svr_engine::_()->get_session_ids(session_.to, src_session);
		if (ret_code < 0)
		{
			long id = sip_svr_engine::_()->id_++;
			send_invite_task *ptr_task = new send_invite_task(sip_svr_engine::_()->sip_hanle_,id,session_);
			if (ptr_task!=NULL)
			{
				ptr_task->process_event();
			}
		}
		else
		{
			//1.重复呼叫直接拒绝会话
			sdp_direction_t svr_dir = sip_svr_engine::_()->parse_sdp_direction(session_.sdp);
			if (svr_dir !=  dir_recvonly)
			{
				oper_code = -4;
				break;
			}
			//2.重复点播添加转发地址
			ret_code = sip_svr_engine::_()->add_send_dst(src_session.srcno,session_.sdp);
			if (ret_code < 0)
			{
				oper_code = -5;
				break;
			}

			src_session.trans_callid_container.push_back(session_.call_id);
			ret_code = sip_svr_engine::_()->update_session(src_session);
			if (ret_code < 0)
			{
				oper_code = -6;
				break;
			}

			xt_sip_status_t status = ::xt_sip_server_invite_provide_answer(session_.hserver,ok_200_with_sdp.c_str(),ok_200_with_sdp.length());
			if (status != XT_SIP_STATUS_OK)
			{
				oper_code = -7;
				break;
			}
		}
    } while (0);

	xt_sip_status_t status = XT_SIP_STATUS_OK;
	if (oper_code < 0)
	{
		WRITE_LOG(DBLOGINFO,ll_error,"invite_task::run() oprater fail call_id[%s] oper_code[%d]",session_.call_id.c_str(),oper_code);
		//拒绝
		status = ::xt_sip_server_invite_reject(session_.hserver,488);
		session_.invite_type = SIP_INVITE_CANCEL;
		sip_svr_engine::_()->end_session(session_);
	}
	
    delete this;
    return 0;
}

uint32_t send_invite_task::run()
{
	long oper_code = 0;
	std_sip_session_t session_;
	do 
	{
		std::string ok_200_with_sdp;
		int srcno = -1;
		dev_handle_t dev_handle = -1;

		string tmp_callid;
		std::string call_id;
		char buff[32];
		sprintf(buff,"20160512%d",id_);
		call_id.assign(buff);
		//创建接收机制与转发源，返回接收机制与转发源创建时返回组合sdp
		long ret_code = XTEngine::_()->sip_2_sip_create_free_transmit_channel(tmp_callid,tmp_callid,-1,0,0,false,ok_200_with_sdp,srcno,dev_handle);
		if (ret_code < 0)
		{
			oper_code = -1;
			break;
		}

		
		session_.creat_time = boost::posix_time::second_clock::local_time();
		session_.invite_type = SIP_INVITE_CLIENT;
		session_.hclient = NULL;
		session_.hserver = NULL;

		session_.call_id = call_id;
		session_.recv_src_callid = (dst_session_.call_id);
		//以发起invite的目标作为本次会话的源ids
		session_.ids_to = dst_session_.to;

		session_.dev_handle = dev_handle;
		session_.srcno = srcno;
		//保留与目标方向相同的媒体
		ret_code = sip_svr_engine::_()->one_part_sdp(dst_session_.sdp, ok_200_with_sdp, false);
		if (ret_code < 0)
		{
			printf("one_part_sdp failed\n");
			oper_code  =-2;
			break;
		}

		session_.router_sdp = ok_200_with_sdp;

		std::string invite_sdp;
		ret_code = sip_svr_engine::_()->combine_c_to_media(dst_session_.sdp,ok_200_with_sdp,invite_sdp);
		if (ret_code < 0)
		{
			printf("comine send invite sdp failed\n");
			oper_code = -3;
			break;
		}
		xt_sip_client_invite_request_t request={0};
		if(!invite_sdp.empty())
		{
			request.sdp = invite_sdp.c_str();
			request.length = invite_sdp.length();
		}
		else
		{
			printf("invite sdp is empty\n");
			oper_code=-4;
			break;
		}

		//此处修改from和target，to
		string local_uri = sip_svr_engine::_()->get_local_uri();
		string next_uri;
		ret_code = sip_svr_engine::_()->find_next_ids(dst_session_.from,local_uri,dst_session_.to,next_uri);
		if (ret_code < 0)
		{
			printf("find next ids failed\n");
			oper_code = -5;
			break;
		}
		request.target = next_uri.c_str();//requst uri
		request.from = local_uri.c_str();//local uri

		//to字段保留最终目标ids
		int pos = dst_session_.to.find('@');
		string ids = dst_session_.to.substr(4,pos-4);
		int len_uri = next_uri.length();
		pos  = next_uri.find('@');
		string tmp = next_uri.substr(pos,len_uri-pos);
		string ids_to = "sip:"+ids+tmp;
		request.to = ids_to.c_str();//to uri
		request.resource_priority = "cmd.3";

		session_.from = request.from;
		session_.to = request.to;

		xt_sip_client_invite_callback_t client_invite_cb = sip_svr_engine::_()->get_client_invite_cb();
		//get_client_invite_cb

		xt_sip_status_t status = xt_sip_make_client_invite(sip_svr_engine::_()->sip_hanle_,
			NULL, &request, NULL, &client_invite_cb, (void*)id_,30*1000);
		if(XT_SIP_STATUS_OK != status)
		{
			printf("sip make client invite fail\n");
			oper_code = -6;
			break;
		}

		if(!(sip_svr_engine::_()->m_tmp_session_call.insert(std::make_pair(id_,session_)).second))
		{
			printf("insert tmp_session make_pair failed\n");
			oper_code = -7;
			break;
		}
	} while (0);

	if (oper_code<0)
	{
		sip_svr_engine::_()->end_session(dst_session_);
		sip_svr_engine::_()->end_session(session_);
	}

	delete this;
	return 0;
}

uint32_t recv_reinvite_task::run()
{
    do 
    {
    } while (0);
    delete this;
    return 0;
}

uint32_t ack_with_sdp_task::run()
{
	do 
	{
	} while (0);
    delete this;
    return 0;
}

uint32_t switchset_task::run()
{
	long oper_code=0;
	std_sip_session_t src_session;
	std_sip_session_t dst_session;
	do 
	{
		long ret_code = sip_svr_engine::_()->get_session(call_id_,src_session);
		if (ret_code < 0)
		{
			oper_code=-1;
			break;
		}

		src_session.sdp = sdp_;
		src_session.sdp_len = len_;
		src_session.hclient = client_handle_;
		src_session.hserver = NULL;
		ret_code = sip_svr_engine::_()->update_session(src_session);
		if (ret_code < 0)
		{
			oper_code=-2;
			break;
		}

		xt_sip_status_t status = XT_SIP_STATUS_OK;
		status = ::xt_sip_client_invite_provide_answer(client_handle_,NULL,0);
		if (status!=XT_SIP_STATUS_OK)
		{
			oper_code=-3;
			break;
		}

		//此处只可能只第一个，因为只有第一源才会去拉流，否则为重复点播
		ret_code = sip_svr_engine::_()->get_session(src_session.recv_src_callid,dst_session);
		if (ret_code < 0)
		{
			oper_code=-4;
			break;
		}

		string ok_200_with_sdp;//对端的能力级组合交换接收机制的接收端口，回复到server端
		ret_code = sip_svr_engine::_()->combine_c_to_media(sdp_,dst_session.router_sdp,ok_200_with_sdp);
		if (ret_code<0)
		{
			oper_code=-5;
			break;
		}
		status = ::xt_sip_server_invite_provide_answer(dst_session.hserver,ok_200_with_sdp.c_str(),ok_200_with_sdp.length());
		if (status != XT_SIP_STATUS_OK)
		{
			oper_code=-6;
			break;
		}
		WRITE_LOG(DBLOGINFO,ll_info,"call_id[%s] ok_200_with_sdp[%s]",dst_session.call_id.c_str(),ok_200_with_sdp.c_str());
		sdp_direction_t svr_dir = sip_svr_engine::_()->parse_sdp_direction(dst_session.sdp);
		if (svr_dir == dir_sendrecv)
		{
			ret_code = sip_svr_engine::_()->push_media_streams(src_session,dst_session);
			if (ret_code < 0)
			{
				printf("push_media_streams failed\n");
				oper_code=-7;
				break;
			}

			ret_code = sip_svr_engine::_()->push_media_streams(dst_session,src_session);
			if (ret_code < 0)
			{
				printf("push_media_streams failed\n");
				oper_code=-8;
				break;
			}
		}
		else if (svr_dir == dir_sendonly)
		{
			oper_code = sip_svr_engine::_()->push_media_streams(dst_session,src_session);
			if (ret_code < 0)
			{
				printf("push_media_streams failed\n");
				oper_code=-9;
				break;
			}
		}
		else if (svr_dir == dir_recvonly)
		{
			ret_code = sip_svr_engine::_()->push_media_streams(src_session,dst_session);
			if (ret_code < 0)
			{
				printf("push_media_streams failed\n");
				oper_code=-10;
				break;
			}
		}
		
	} while (0);

	if (oper_code<0)
	{
		sip_svr_engine::_()->end_session(src_session);
		sip_svr_engine::_()->end_session(dst_session);
	}
	
    delete this;
    return 0;
}

//terminate invite
uint32_t invite_bye_task::run()
{
	long oper_code=0;
	do 
	{
		//清理自身会话相关转发资源
		std_sip_session_t session;
		long ret_code = sip_svr_engine::_()->get_session(call_id_,session);
		if (ret_code < 0)
		{
			//printf("invite bye session already do bye\n");
			oper_code=-1;
			break;
		}
		//结束自身会话
		ret_code = sip_svr_engine::_()->end_session(session);
		if (ret_code < 0)
		{
			oper_code = -2;
			printf("end session fail!");
			break;
		}

		if (session.invite_type == SIP_INVITE_CLIENT)
		{
			//清理关联会话转发资源
			std_sip_session_t src_session;
			ret_code = sip_svr_engine::_()->get_session(session.recv_src_callid,src_session);
			if (ret_code<0)
			{
				oper_code=-3;
				break;
			}
			//发送bye
			ret_code = sip_svr_engine::_()->end_session(src_session);
			if (ret_code < 0)
			{
				oper_code = -4;
				printf("end session fail!");
				break;
			}
		}
		else if (session.invite_type == SIP_INVITE_SERVER)
		{

			//如果对端转发源已没有转发目标，则将其结束掉
			std_sip_session_t src_session;
			ret_code =sip_svr_engine::_()->get_session(session.recv_src_callid,src_session);
			if (ret_code<0)
			{
				oper_code=-5;
				break;
			}
			if (src_session.trans_callid_container.size() == 0)
			{
				//发送bye
				ret_code = sip_svr_engine::_()->end_session(src_session);
				if (ret_code < 0)
				{
					oper_code = -6;
					printf("end session fail!");
					break;
				}
			}
		}

		//清理重复点播转发资源
		vector<string>::iterator itr = session.trans_callid_container.begin();
		for (;itr != session.trans_callid_container.end();++itr)
		{
			std_sip_session_t trans_session;
			ret_code = sip_svr_engine::_()->get_session(*itr,trans_session);
			if (ret_code < 0)
			{
				continue;
			}
			ret_code = sip_svr_engine::_()->end_session(trans_session);
			if (ret_code < 0)
			{
				oper_code = -7;
				printf("end session fail!");
			}
		}
	} while (0);
	
    delete this;
    return 0;
}
