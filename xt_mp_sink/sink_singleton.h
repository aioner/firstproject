#ifndef	_SINK_SINGLETON_H
#define _SINK_SINGLETON_H

#include "mp_entity.h"
#include <boost/filesystem/fstream.hpp>
#include <time.h>
#include <fstream>
#include <iostream>

namespace xt_mp_sink
{

	class sink_inst : private boost::noncopyable
	{
	public:	
		static sink_inst * sink_singleton()
		{
			static sink_inst _instance;
			return &_instance;
		}		
	
		static void sink_singleton_end(){

			//utility::destruct_ptr(sink_inst::_instance);
			//sink_inst::_instance = null;
		}

		//线程池单例
		boost::threadpool::pool * tp_inst()
		{
			boost::unique_lock<boost::shared_mutex> lock(_mutex);
			return _tp;
		}		
		//投递线程单例
		boost::threadpool::pool * post_tp_inst()
		{
			boost::unique_lock<boost::shared_mutex> lock(_mutex);
			return _post_tp;
		}

		//初始化库
		int32_t mp_sink_init(sink_init_descriptor * sink_des);
		
		//终止库
		void mp_sink_end()
		{
			_tp->wait();
			utility::destruct_ptr(_tp);
			_post_tp->wait();
			utility::destruct_ptr(_post_tp);
			end_rv_adapter();
		}

	protected:
		sink_inst()
			: _tp(null)
			,m_run(true)
			,m_theart(mp_task_heart)
		{}

	public:
		~sink_inst()
		{
			m_run = false;
			m_theart.join();
		}
	public:
		boost::threadpool::pool *	_tp;
		boost::threadpool::pool *	_post_tp;

		boost::shared_mutex	_mutex;

	public:
		// 心跳包处理
		//////////////////////////////////////////////////////////////////////////
		boost::thread m_theart;
		std::map<mp_entity*,mp_entity*> m_map_ents;
		boost::shared_mutex	m_ment;	//资源锁
		bool m_run;

		static void mp_task_heart();

		void add_ent(mp_entity* ent);
		void del_ent(mp_entity* ent);
		//////////////////////////////////////////////////////////////////////////		
	};
}

#endif // _SINK_SINGLETON_H
