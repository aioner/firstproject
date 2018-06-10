#include "tcp_session_server.h"
#include "tcp_session_server_impl.h"
#include <stdio.h>
#include <stdarg.h>
#include <boost/detail/lightweight_thread.hpp>
#include <boost/bind.hpp>

namespace 
{
	class service_thread_t
	{
	public:
		service_thread_t()
			:service_(),
			pthread_()
		{}

		~service_thread_t()
		{
			stop();
		}

		void start()
		{
			boost::detail::lw_thread_create(pthread_, boost::bind(&service_thread_t::service_worker, this));
		}

		bool is_starting() const
		{
			return (NULL != pthread_);
		}

		void stop()
		{
			if (is_starting())
			{
				pthread_t pth = pthread_;
				service_.stop();
				pthread_join(pth, NULL);
				pthread_ = NULL;
			}
		}

		xt_tcp::service_t &get_service()
		{
			return service_;
		}
	private:
		void service_worker()
		{
			service_.run();
		}

		xt_tcp::service_t service_;
		pthread_t pthread_;
	};
}


service_thread_t *g_service_thread = NULL;
boost::intrusive_ptr<tcp_session_server::tcp_session_server_mgr> g_server_mgr;

void TCP_SVR_PRINT(const xt_log_level ll,
				   const char* format,
				   ...)
{
	if (g_server_mgr&&
		g_server_mgr->tcp_config&&
		g_server_mgr->tcp_config->m_server_param.print_cb)
	{
		char context[4096] = {0};

		va_list arg;
		va_start(arg, format);
		vsnprintf(context, sizeof(context)-1, format, arg);
		va_end(arg);

		g_server_mgr->tcp_config->m_server_param.print_cb("tcp_svr", ll, context);
	}	
}

TCP_SESSION_SERVER_API void  tcp_session_server_start(tcp_session_server_param_t *pServer_Param)
{
	g_service_thread = new service_thread_t;
	g_service_thread->start();
	g_server_mgr.reset(new tcp_session_server::tcp_session_server_mgr); 
	g_server_mgr->set_server_param(pServer_Param);
	g_server_mgr->start(g_service_thread->get_service(), pServer_Param->ip,pServer_Param->listen_port);
}


TCP_SESSION_SERVER_API void tcp_session_server_stop()
{
	if (g_server_mgr != NULL)
	{
		g_server_mgr->on_close();

		g_server_mgr.reset();
	}
	
	if( NULL != g_service_thread)
	{
		g_service_thread->stop();
		delete g_service_thread;
		g_service_thread = NULL;
	}

}

