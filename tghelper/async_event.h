///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����async_event.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��17��
// �����������첽�¼�����
//
// 1�����нӿ����Ϊ���̰߳�ȫ
// 2��async_event��Ϊ�첽�����¼�������ڣ����������
//		virtual void do_event()	���������¼��ľ���ִ������
//		virtual bool wait_event(uint32_t ms_timeout) ���������������ȡ�ķ�ʽ
// 3��async_event_pools��Ϊ�첽�¼����������ڲ������ɸ�recycle_pool����
//
// �޶���־
// [2012-03-17]		���������汾
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
			EVENT_IDLE,		//����״̬��δʹ��
			EVENT_WAITTING,	//�ȴ��¼���Ӧ
			EVENT_READY,	//�¼���Ӧ�ź�
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
			if (inner::EVENT_WAITTING == m_state)		//�ͷŹ����¼��ȴ��߳�
			{
				transit(inner::EVENT_IDLE);
				m_cond.notify_one();
			}
			else
				m_state = inner::EVENT_IDLE;
		}
		//����������¼�
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
					if (!bRet) break;	//��ʱ�¼�����
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
		uint32_t m_event_id;		//������Դ˹滮��ͬ���¼�ֵ
		boost::mutex m_mutex;
		boost::condition_variable m_cond;
		inner::EAsyncEventState m_state;
		
	};
}

#endif