#include <string.h>
#include "tcp_session_server_impl.h"
#include <stdio.h>

using namespace tcp_session;
extern void TCP_SVR_PRINT(const xt_log_level ll,
						  const char* format,
						  ...);

void tcp_session_server::tcp_session_server_impl::send_login_stop_response( void *buf, size_t bytes_transferred )
{
	if (NULL == tcp_config_)
	{
		return;
	}

	login_msg_t login_msg;
	memset(&login_msg,0,sizeof(login_msg_t));

	memcpy(&login_msg,buf,sizeof(login_msg_t));
	char send_buf[TCP_SESSION_KEY_LEN];
	msg_header_t head_msg;

	memset(&head_msg,0,sizeof(msg_header_t ));

	if (bytes_transferred != sizeof(login_msg_t ))
	{
		on_destroy_impl();
		return;
	}

	uint32_t send_len = 0;
	switch(login_msg.op_id)
	{
	case TCP_SESSION_OPID_LOGIN:
		{
			head_msg.op_id = TCP_SESSION_OPID_MULTICAST_INFO;
			head_msg.data_type = 0;
			char multl_info[receive_buffer_max_size];
			uint16_t mul_size=0;
			tcp_config_->m_server_param.get_mul_cb(this,multl_info,&mul_size);

			send_len = create_send_data(send_buf, TCP_SESSION_KEY_LEN, head_msg,multl_info , mul_size);
			if (send_len < TCP_SESSION_KEY_LEN && send_len >0)
			{
				TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_LOGIN: rip[%s] rport[%d]", m_remote_ip, m_remote_port);	
				if (!this->send(send_buf,send_len))
				{
					TCP_SVR_PRINT(level_error, "TCP_SESSION_OPID_LOGIN resp fail: rip[%s] rport[%d]", m_remote_ip, m_remote_port);
					on_destroy_impl();
					destroy();
				}
			}
			else
			{
				TCP_SVR_PRINT(level_error, "TCP_SESSION_OPID_LOGIN resp fail(no data): rip[%s] rport[%d]", m_remote_ip, m_remote_port);
				on_destroy_impl();
				destroy();
			}
		
		}
		break;
	case TCP_SESSION_OPID_STOP:
		{
			if(NULL != tcp_config_->m_server_param.stop_link_cb)
			{	
				TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_STOP: rip[%s] rport[%d] rtp_port[%d] chan[%d] mode[%d]", 
					m_remote_ip, m_remote_port, login_msg.rtp_port, login_msg.channel,login_msg.mode);
				tcp_config_->m_server_param.stop_link_cb(this,m_local_ip,m_remote_ip,m_remote_port,login_msg.rtp_port,login_msg.channel,login_msg.mode,0,0);		
			}
			head_msg.op_id = TCP_SESSION_OPID_STOP;
			head_msg.data_type = 0;
			send_len = create_send_data(send_buf,TCP_SESSION_KEY_LEN,head_msg,(char*)buf,sizeof(msg_header_t ));
			this->send(send_buf,send_len);
		}
	}

}

void tcp_session_server::tcp_session_server_impl::send_rtp_start_play_response( void *buf, size_t bytes_transferred )
{
	if (NULL == tcp_config_)
	{
		return;
	}

	play_msg_t play_msg;
	memset(&play_msg,0,sizeof(play_msg_t));

	memcpy(&play_msg,buf,sizeof(play_msg_t));

	char	send_buf[TCP_SESSION_KEY_LEN] = {0};
	char	sdp[TCP_SESSION_KEY_LEN] = {0};
	uint32_t send_len=0;
	msg_header_t head_msg;
	memset(&head_msg,0,sizeof(msg_header_t));

	if (bytes_transferred != sizeof(play_msg_t ))
	{
		tcp_config_->m_server_param.close_connect_cb(this,m_remote_ip,m_remote_port,receive_rtp_msg_invalid);
		on_destroy_impl();
		return;
	}

	switch(play_msg.op_id_ext)
	{
		case TCP_SESSION_OPID_PLAY:
			{
				head_msg.op_id = TCP_SESSION_OPID_KEY_INFO;
				uint32_t multiplex;
				uint32_t multiplex_id;
				uint32_t rtp_port;

				tcp_config_->m_server_param.get_sdp_cb(this,m_remote_ip,m_remote_port,play_msg.channel,
							sdp,&head_msg.data_type,&head_msg.data_size,&send_len,
							&multiplex,&multiplex_id,&rtp_port);
				send_len = create_send_data(send_buf, TCP_SESSION_KEY_LEN, head_msg,sdp, head_msg.data_size);
				if (send_len>0)
				{
					if (tcp_config_->m_server_param.start_link_cb !=NULL)
					{
						TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_PLAY: rip[%s] rport[%d] rtp_port[%d] chan[%d] mode[%d]", 
							m_remote_ip, m_remote_port, play_msg.rtp_port, play_msg.channel,play_msg.mode);
						tcp_config_->m_server_param.start_link_cb(this,m_local_ip,m_remote_ip,m_remote_port,play_msg.rtp_port,
																	play_msg.channel,play_msg.mode,play_msg.ssrc,0,0);
					}

					TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_PLAY resp: rip[%s] rport[%d] len[%d] sdp[%s]", m_remote_ip, m_remote_port, send_len, sdp);
					if (!this->send(send_buf,send_len))
					{
						TCP_SVR_PRINT(level_error, "TCP_SESSION_OPID_PLAY resp fail: rip[%s] rport[%d]", m_remote_ip, m_remote_port);
						tcp_config_->m_server_param.close_connect_cb(this,m_remote_ip,m_remote_port,send_multi_info_failed);
						on_destroy_impl();
					}
				}
				else
				{
					if (tcp_config_->m_server_param.start_link_cb !=NULL)
					{
						TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_PLAY: rip[%s] rport[%d] rtp_port[%d] chan[%d] mode[%d]", 
							m_remote_ip, m_remote_port, play_msg.rtp_port, play_msg.channel,play_msg.mode);
						tcp_config_->m_server_param.start_link_cb(this,m_local_ip,m_remote_ip,m_remote_port,play_msg.rtp_port,
																	play_msg.channel,play_msg.mode,play_msg.ssrc,0,0);
					}
					memcpy(&send_buf,&head_msg,sizeof(msg_header_t));
					send_len = sizeof(msg_header_t);
					TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_PLAY resp: rip[%s] rport[%d] len[%d] sdp[%s]", m_remote_ip, m_remote_port, send_len, sdp);
					if (!this->send(send_buf,send_len))
					{
						TCP_SVR_PRINT(level_error, "TCP_SESSION_OPID_PLAY resp fail: rip[%s] rport[%d]", m_remote_ip, m_remote_port);
						tcp_config_->m_server_param.close_connect_cb(this,m_remote_ip,m_remote_port,send_multi_info_failed);
						on_destroy_impl();
					}
				}
			}
			break;
		default:
			{
				TCP_SVR_PRINT(level_error, "TCP_SESSION_OPID_PLAY ERROR: rip[%s] rport[%d]", m_remote_ip, m_remote_port);
				tcp_config_->m_server_param.close_connect_cb(this,m_remote_ip,m_remote_port,receive_rtp_msg_invalid);
				on_destroy_impl();
			}

	}

}

void tcp_session_server::tcp_session_server_impl::send_multi_start_play_response( void *buf, size_t bytes_transferred )
{
	if (NULL == tcp_config_)
	{
		return;
	}

	multiplex_play_msg_t multicast;
	memset(&multicast,0,sizeof(multiplex_play_msg_t));
	memcpy(&multicast, buf, sizeof(multiplex_play_msg_t)); 


	if (bytes_transferred != sizeof(multiplex_play_msg_t ))
	{
		tcp_config_->m_server_param.close_connect_cb(this,m_remote_ip,m_remote_port,receive_multi_msg_invalid);
		on_destroy_impl();
		return;
	}


	uint32_t send_len=0;
	char send_buf[TCP_SESSION_KEY_LEN] = {0};
	char	sdp[TCP_SESSION_KEY_LEN] = {0};

	msg_header_t head_msg;
	memset(&head_msg,0,sizeof(msg_header_t));
	switch(multicast.op_id_ext)
	{
	case TCP_SESSION_OPID_PLAY:
		{ 
			head_msg.op_id = TCP_SESSION_OPID_KEY_INFO2;
			head_msg.data_type = 0;
			head_msg.flag = 0xFF;
			head_msg.data_size = 0;
		
			uint32_t multiplex_s=0;
			uint32_t multiplex_id_s=0;
			uint32_t rtp_port=0;		

			tcp_config_->m_server_param.get_sdp_cb(this,m_remote_ip,m_remote_port,multicast.channel,
				sdp,&head_msg.data_type,&head_msg.data_size,&send_len,
				&multiplex_s,&multiplex_id_s,&rtp_port);

            if (send_len >0 )
            {
                memcpy(sdp+head_msg.data_size,&multiplex_s,4);
                send_len+=4;
                head_msg.data_size+=4;
                memcpy(sdp+head_msg.data_size,&multiplex_id_s,4);
                send_len+=4;
                head_msg.data_size+=4;
                if (!multiplex_s)
                {
                    memcpy(sdp+head_msg.data_size,&rtp_port,4);
                    send_len+=4;
                    head_msg.data_size+=4;
                }
            }

			send_len = create_send_data(send_buf, TCP_SESSION_KEY_LEN, head_msg,sdp, head_msg.data_size);

			if (send_len>0)
			{
				if (tcp_config_->m_server_param.start_link_cb !=NULL)
				{
					TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_PLAY: rip[%s] rport[%d] rtp_port[%d] chan[%d] mode[%d] demux[%d] demuxid[%x]", 
						m_remote_ip, m_remote_port, multicast.rtp_port, multicast.channel,multicast.mode, multicast.multiplex,multicast.multiplexID);

					// 20150623 modify by songlei 增加sink复用信息抛出
					tcp_config_->m_server_param.start_link_cb(
						this,m_local_ip,m_remote_ip,m_remote_port,multicast.rtp_port,multicast.channel,
						multicast.mode,multicast.ssrc,multicast.multiplex,multicast.multiplexID);
				}

				TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_PLAY resp: rip[%s] rport[%d] len[%d] sdp[%s]", m_remote_ip, m_remote_port, send_len, sdp);
				if (!this->send(send_buf,send_len))
				{
					TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_PLAY resp fail: rip[%s] rport[%d]", m_remote_ip, m_remote_port);
					tcp_config_->m_server_param.close_connect_cb(this,m_remote_ip,m_remote_port,send_multi_sdp_info_failed);
					on_destroy_impl();
				}
			}
			else
			{
				if (tcp_config_->m_server_param.start_link_cb !=NULL)
				{
					TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_PLAY: rip[%s] rport[%d] rtp_port[%d] chan[%d] mode[%d] demux[%d] demuxid[%x]", 
						m_remote_ip, m_remote_port, multicast.rtp_port, multicast.channel,multicast.mode, multicast.multiplex,multicast.multiplexID);

					// 20150623 modify by songlei 增加sink复用信息抛出
					tcp_config_->m_server_param.start_link_cb(
						this,m_local_ip,m_remote_ip,m_remote_port,multicast.rtp_port,multicast.channel,
						multicast.mode ,multicast.ssrc,multicast.multiplex,multicast.multiplexID);
				}
				memcpy(&send_buf,&head_msg,sizeof(msg_header_t));
				send_len = sizeof(msg_header_t);
				
				TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_PLAY resp: rip[%s] rport[%d] len[%d] sdp[%s]", m_remote_ip, m_remote_port, send_len, sdp);
				if (!this->send(send_buf,send_len))
				{
					TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_PLAY resp fail: rip[%s] rport[%d]", m_remote_ip, m_remote_port);
					tcp_config_->m_server_param.close_connect_cb(this,m_remote_ip,m_remote_port,send_multi_sdp_info_failed);
					on_destroy_impl();
				}

			}

			break;
		}
    case TCP_SESSION_OPID_STOP:
        {
            if (tcp_config_ != NULL )
            {
				TCP_SVR_PRINT(level_info, "TCP_SESSION_OPID_STOP: rip[%s] rport[%d] rtp_port[%d] chan[%d] mode[%d] demux[%d] demuxid[%x]", 
					m_remote_ip, m_remote_port, multicast.rtp_port, multicast.channel,multicast.mode, multicast.multiplex,multicast.multiplexID);

				tcp_config_->m_server_param.stop_link_cb(this,m_local_ip,m_remote_ip,m_remote_port,multicast.rtp_port,multicast.channel,multicast.mode,multicast.multiplex,multicast.multiplexID);	
            }
            head_msg.op_id = TCP_SESSION_OPID_STOP;
            head_msg.data_type = 0;
            send_len = create_send_data(send_buf,TCP_SESSION_KEY_LEN,head_msg,(char*)buf,sizeof(msg_header_t ));
            this->send(send_buf,send_len);
        }
        break;
	default:
		{
			TCP_SVR_PRINT(level_error, "TCP_SESSION_OPID_PLAY ERROR: rip[%s] rport[%d]", m_remote_ip, m_remote_port);
			tcp_config_->m_server_param.close_connect_cb(this,m_remote_ip,m_remote_port,receive_multi_msg_invalid);
			on_destroy_impl();
		}

	}
}
void tcp_session_server::tcp_session_server_impl::on_destroy_impl()
{
	if (mgr_)
	{
		mgr_->on_close_client(this);
	}
}

tcp_session_server::tcp_session_server_impl::~tcp_session_server_impl()
{

}

tcp_session_server::tcp_session_server_impl::tcp_session_server_impl(const tcp_config_ptr& config,tcp_session_server_mgr *mgr)
:xt_tcp::socket_t(),
tcp_config_(config),
mgr_(mgr)
{

}

void tcp_session_server::tcp_session_server_impl::on_receive(xt_tcp_status_t stat, void *buf, size_t bytes_transferred)
{
	if (TCP_SESSION_KEY_LEN < bytes_transferred)
	{		
		on_destroy_impl();
		destroy();
		return;
	}
	if (XT_TCP_STATUS_OK == stat && bytes_transferred >0)
	{
		uint16_t op_id=0;
		memcpy(&op_id, buf, 2);
		if ( TCP_SESSION_OPID_LOGIN ==op_id || TCP_SESSION_OPID_STOP == op_id)
		{
			send_login_stop_response(buf,bytes_transferred);
		}
		else if (TCP_SESSION_OPID_EXT_FLAG == op_id)
		{
			send_rtp_start_play_response(buf,bytes_transferred);
		}
		else if (TCP_SESSION_OPID_EXT2_FLAG == op_id)
		{
			send_multi_start_play_response(buf,bytes_transferred);
		}
	
		start();
	}
	else if (bytes_transferred <= 0)
	{
		char remote_ip[SERVER_IP_LEN];
		uint16_t remoter_port;
		this->remote_endpoint(remote_ip,remoter_port);
		if (NULL == tcp_config_)
		{
			return;
		}
		if ( NULL != tcp_config_->m_server_param.close_connect_cb )
		{
			TCP_SVR_PRINT(level_error, "client_disconnect: rip[%s] rport[%d]", m_remote_ip, m_remote_port);
			tcp_config_->m_server_param.close_connect_cb(this,remote_ip,remoter_port,client_disconnect);
		}
		
		on_destroy_impl();
		 
		destroy();
	}

}

bool  tcp_session_server::tcp_session_server_impl::start()
{
	this->remote_endpoint(m_remote_ip,m_remote_port);
	this->local_endpoint(m_local_ip,m_local_port);
	 return this->async_receice(buf_, receive_buffer_max_size);
}

uint32_t tcp_session_server::tcp_session_server_impl::create_send_data( char *send_data, uint32_t send_max_size, msg_header_t &head_msg, const char *data, uint32_t data_size )
{
	uint32_t send_size = get_send_size(data_size+sizeof(msg_header_t), head_msg.fill_size);	

	if(send_size>send_max_size)		//如果超出范围则返回-1
		return 0;

	if (head_msg.flag != 0xFF)
	{
		//DataHead.nFlag = Def_Flag;
	}

	head_msg.data_size = send_size - sizeof(msg_header_t);		//保存有效载荷的长度
	long copy_send = 0;

	::memcpy(send_data, &head_msg, sizeof(msg_header_t));			//拷贝数据头信息
	copy_send += sizeof(msg_header_t);

	if((NULL != data)&&(data_size>0))
	{
		::memcpy(send_data+copy_send, data, data_size);		//拷贝数据
		copy_send += data_size;
	}

	if(head_msg.fill_size>0)							//如果需要填充则进行数据填充
		::memset(send_data+copy_send, 0XFF, head_msg.fill_size);

	return send_size;
}

uint32_t tcp_session_server::tcp_session_server_impl::get_send_size( uint32_t sou_size, uint32_t &fill_size, uint32_t max_fill_size/*=FILLSIZE*/ )
{
	int mode = sou_size%max_fill_size;

	fill_size = 0;

	if(0 != mode)
	{
		int div = sou_size/max_fill_size;		
		uint32_t size = div*max_fill_size+max_fill_size;

		fill_size = size - sou_size;

		return size;		
	}

	return sou_size;
}




bool tcp_session_server::tcp_session_server_mgr::start( xt_tcp::service_t& service, const char *ip, uint16_t port )
{
	if (!this->init(service, ip, port))
	{
		TCP_SVR_PRINT(level_error, "start fail: rip[%s] rport[%d]", ip, port);
		return false;
	}

	TCP_SVR_PRINT(level_info, "start : rip[%s] rport[%d]", ip, port);
	return start_accept();
}

bool tcp_session_server::tcp_session_server_mgr::start_accept()
{
	return async_accept(xt_tcp::create_socket(get_service()));
}

void tcp_session_server::tcp_session_server_mgr::on_accept( xt_tcp_status_t stat, xt_tcp_socket_t socket )
{
	if (XT_TCP_STATUS_OK == stat)
	{
		boost::intrusive_ptr<tcp_session_server_impl> sp(new tcp_session_server_impl(tcp_config,this));
		sp->attach(socket);

		xt_tcp_linger_t opt;
		opt.onoff = 1;
		opt.linger = 0;
		sp->set_option(XT_TCP_OPT_LINGER, opt);

        //added by lichao, 20151223 增加tcp心跳保活配置
        sp->set_keepalive_opt(true, 5, 6);
		
		if (sp->start())
		{
			clients_.push_back(sp);
		}
		start_accept();

	}
	else
	{
		xt_tcp::destroy_socket(socket);
	}
}

void tcp_session_server::tcp_session_server_mgr::on_close()
{

	clients_.clear();

	this->destroy();

}
void tcp_session_server::tcp_session_server_mgr::set_server_param(tcp_session_server_param_t *server_param )
{
	tcp_config->set_server_param(server_param);
}

tcp_session_server::tcp_session_server_mgr::tcp_session_server_mgr()
{
	tcp_config = new tcp_session_server_config();
	clients_.clear();
}

tcp_session_server::tcp_session_server_mgr::~tcp_session_server_mgr()
{
}
void tcp_session_server::tcp_session_server_mgr::on_close_client(tcp_session_server_impl *client)
{
    std::vector<boost::intrusive_ptr<tcp_session_server_impl> >::iterator itr = clients_.begin();
    for (;clients_.end() != itr;)
    {
        if (client == (*itr))
        {
            clients_.erase(itr++);
            break;
        }
        else
        {
            ++itr;
        }
    }
}

void tcp_session_server::tcp_session_server_config::set_server_param( tcp_session_server_param_t *server_param )
{
	memcpy(&m_server_param, server_param, sizeof(tcp_session_server_param_t));
}

tcp_session_server::tcp_session_server_config::tcp_session_server_config()
{
	memset(&m_server_param,0,sizeof(tcp_session_server_param_t));
}
