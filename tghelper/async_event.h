///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：async_event.h
// 创 建 者：汤戈
// 创建时间：2012年03月17日
// 内容描述：异步事件处理
//
// 1、所有接口类均为多线程安全
// 2、async_event作为异步处理事件基类存在，子类可重载
//		virtual void do_event()	用于描述事件的具体执行内容
//		virtual bool wait_event(uint32_t ms_timeout) 用于描述结果的提取的方式
// 3、async_event_pools作为异步事件池容器，内部由若干个recycle_pool构成
//
// 修订日志
// [2012-03-17]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef ASYNC_EVENT_
#define ASYNC_EVENT_

#include<stdint.h>
#include "recycle_pool.h"
#include <boost/thread/condition_variable.hpp>

#if (_TGHELP_DEBUG_ENABLE)
	#include <iostream>
#endif


namespace tghelper
{
	namespace inner
	{
		typedef enum _EAsyncEventState
		{
			EVENT_IDLE,		//空闲状态，未使用
			EVENT_WAITTING,	//等待事件响应
			EVENT_READY,	//事件响应信号
		} EAsyncEventState;
	}

	class async_event : public recycle_pool_item
	{
	public:
		async_event() : m_event_id(0),
						m_state(inner::EVENT_IDLE)
		{

		}

		async_event(uint32_t event_id) : 
			m_event_id(event_id),
			m_state(inner::EVENT_IDLE)
		{

		}
		virtual ~async_event()
		{

		}
		inline uint32_t get_event_id() { return m_event_id; }
		inline void set_event_id(uint32_t event_id) { m_event_id = event_id; }
		virtual void recycle_alloc_event() { m_state = inner::EVENT_IDLE; }	
		virtual void recycle_release_event() 
		{
			if (inner::EVENT_WAITTING == m_state)		//释放关联事件等待线程
			{
				transit(inner::EVENT_IDLE);
				m_cond.notify_one();
			}
			else
				m_state = inner::EVENT_IDLE;
		}
		//子类可重载事件
		virtual void do_event()	
		{
			if (inner::EVENT_WAITTING == m_state)
			{
				transit(inner::EVENT_READY);
				m_cond.notify_one();
			}
		}
		void set_wait_state()
		{
			transit(inner::EVENT_WAITTING);
		}
		virtual bool wait_event(uint32_t ms_timeout = 0)
		{
			bool bRet = true;
			boost::unique_lock<boost::mutex> lock(m_mutex);
			while(inner::EVENT_WAITTING == m_state)
			{
				if (0 == ms_timeout) 
					m_cond.wait(lock);
				else
				{
					bRet = m_cond.timed_wait(lock, 
											 boost::get_system_time() + 
											 boost::posix_time::milliseconds(ms_timeout));
					if (!bRet) break;	//超时事件发生
				}
			}

			return bRet;
		}

		bool compare(async_event * event)
		{
			bool bRet = false;
			if (event) bRet = (event->m_event_id == m_event_id);
			return bRet;
		}

		bool compare(const async_event & event)
		{ return (event.m_event_id == m_event_id);	}

		bool operator ==(async_event * event)
		{ return compare(event); }

		bool operator ==(const async_event &event)
		{ return compare(event); }

	protected:
		inner::EAsyncEventState transit(inner::EAsyncEventState newState)
		{
			boost::unique_lock<boost::mutex> lock(m_mutex);
			m_state = newState;
			return m_state;
		}

	protected:
		uint32_t m_event_id;		//子类可以此规划不同的事件值
		boost::mutex m_mutex;
		boost::condition_variable m_cond;
		inner::EAsyncEventState m_state;
		
	};
}

#endif