#ifndef	TCP_SESSION_SERVER_IMPL_H_INCLUDED
#define TCP_SESSION_SERVER_IMPL_H_INCLUDED


#include "../xt_tcp/xt_tcp_wrapper.h"
#include "tcp_session_msg.h"
#include "tcp_session_server.h"
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <vector>

namespace tcp_session_server
{

	class tcp_session_server_config : public boost::intrusive_ref_counter<tcp_session_server_config>
	{
	public:
		tcp_session_server_config();
		void set_server_param(tcp_session_server_param_t *server_param);
		tcp_session_server_param_t m_server_param;

	};

	typedef boost::intrusive_ptr<tcp_session_server_config> tcp_config_ptr;

	class tcp_session_server_mgr;
	class tcp_session_server_impl: public xt_tcp::socket_t
	{
	public:
		 enum {  receive_buffer_max_size = 2048,
				 FILLSIZE = 16 ,
				 SERVER_IP_LEN=32,
		 };
		 enum
		 {
			 send_multi_info_failed=0,
			 send_stop_failed=1,
			 send_sdp_info_failed=2,
			 send_multi_sdp_info_failed=3,
			 send_multi_stop_failed=4,
			 receive_msg_invalid=5,
			 receive_rtp_msg_invalid = 6,
			 receive_multi_msg_invalid=7,
			 client_disconnect=8
		 };
		tcp_session_server_impl(const tcp_config_ptr& config,tcp_session_server_mgr *mgr);
		
		~tcp_session_server_impl();

		void on_receive(xt_tcp_status_t stat, void *buf, size_t bytes_transferred);

		void send_login_stop_response(void *buf, size_t bytes_transferred);

		void send_rtp_start_play_response(void *buf, size_t bytes_transferred);

		void send_multi_start_play_response(void *buf, size_t bytes_transferred);

		bool start();
		void on_connect(xt_tcp_status_t stat) 
		{

		}
		void on_send(xt_tcp_status_t stat, const void* buf, size_t bytes_transferred) 
		{

		}
		

	private:
		tcp_config_ptr tcp_config_;

		char buf_[receive_buffer_max_size];

		uint32_t create_send_data(char *send_data, uint32_t send_max_size, tcp_session::msg_header_t &head_msg, const char *pData, uint32_t data_size);	
		
		uint32_t get_send_size(uint32_t sou_size, uint32_t &fill_size, uint32_t max_fill_size=FILLSIZE);

		void on_destroy_impl();

		char m_remote_ip[SERVER_IP_LEN];
		uint16_t m_remote_port;
		char m_local_ip[SERVER_IP_LEN];
		uint16_t m_local_port;
		tcp_session_server_mgr	*mgr_;
	};

	class tcp_session_server_mgr:public xt_tcp::acceptor_t
	{
	public:
		bool start(xt_tcp::service_t& service, const char *ip, uint16_t port);

		void set_server_param(tcp_session_server_param_t *server_param);
		void on_close();

		void on_close_client(tcp_session_server_impl *client);
		tcp_session_server_mgr();
		~tcp_session_server_mgr();
	private:
		bool start_accept();
		void on_accept(xt_tcp_status_t stat, xt_tcp_socket_t socket);
	public:
		tcp_config_ptr tcp_config;

		std::vector< boost::intrusive_ptr<tcp_session_server_impl> >clients_;

	};



	

}
#endif//TCP_SESSION_SERVER_IMPL_H_INCLUDED