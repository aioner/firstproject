///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：recycle_pool.cpp
// 创 建 者：汤戈
// 创建时间：2011年02月27日
// 内容描述：循环内存池
//
// 修订日志
// [2011-02-27]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////
//#include "stdafx.h"
#include "tghelper/recycle_pool.h"

namespace tghelper
{
	///////////////////////////////////////////////////
	//recycle_pool_item
	int recycle_pool_item::assign()
	{
		boost::mutex::scoped_lock lock(m_mutex);
		m_ref_count++;
		return m_ref_count;
	}
	int recycle_pool_item::release()
	{
		int nRet = 0;
		boost::mutex::scoped_lock lock(m_mutex);
		if (0 < m_ref_count) m_ref_count--;
		if (0 == m_ref_count)
		{
			if (m_owner)
			{
				m_owner->release_item(this);
				nRet = 0;
			}
			else
			{
				nRet = -1;
			}
		}
		else
			nRet = m_ref_count;
		return nRet;
	}

	///////////////////////////////////////////////////
	// recycle_pool
	recycle_pool::recycle_pool(void)
	{
	}

	recycle_pool::~recycle_pool(void)
	{
	#if (_TG_DEBUG_ENABLE)
		std::cout << "~recycle_pool, freesize = " << freesize() << " usesize = " << tracesize() << std::endl; 
	#endif

		clear();
	}
	
	void recycle_pool::_add_item(recycle_pool_item *item)
	{
		if (item)
		{
			item->set_owner_flag(this);	
			item->reset_ref_count();
			m_unused.push_back(item);
		}
	}

	void recycle_pool::add_item(recycle_pool_item *item)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		_add_item(item);
	}

	void recycle_pool::_del_item(recycle_pool_item *item)
	{
		if (item)
		{
			item->set_owner_flag(0);
			item->reset_ref_count();

			std::vector<recycle_pool_item *>::iterator itr = m_unused.begin();
			for (;itr!=m_unused.end();++itr)
			{
				if (item == *itr)
				{
					m_unused.erase(itr);
					break;
				}
			}

			if (item->get_trace_flag()) 
			{
				std::vector<recycle_pool_item *>::iterator itr = m_used.begin();
				for (;itr!=m_used.end();++itr)
				{
					if (item == *itr)
					{
						m_used.erase(itr);
						break;
					}
				}
			}
		}
	}

	void recycle_pool::del_item(recycle_pool_item *item)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		_del_item(item);
	}

	void recycle_pool::mov_item(recycle_pool_item *item)
	{
		if (item)
		{
			recycle_pool * old_owner = item->get_owner_flag();
			if (old_owner) old_owner->del_item(item);
			add_item(item);
		}
	}

	recycle_pool_item * recycle_pool::_alloc_item(bool bTrace)
	{
		recycle_pool_item * item = 0;
		if (!m_unused.empty()) 
		{
			item = m_unused.front();
			m_unused.erase(m_unused.begin());
			//实时跟踪分配节点状态，引入性能开销，对于频繁申请释放的节点不建议使用该功能
			if (bTrace) m_used.push_back(item);		
			item->set_trace_flag(bTrace);
			//激活分配事件
			item->recycle_alloc_event();
		}
		return item;
	}

	recycle_pool_item * recycle_pool::alloc_item(bool bTrace)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		return _alloc_item(bTrace);
	}

	recycle_pool_item * recycle_pool::_add_alloc_fast_item(recycle_pool_item *item, bool bTrace)
	{
		if (item)
		{
			item->set_owner_flag(this);	
			item->reset_ref_count();

			if(bTrace)
			{
				m_used.push_back(item);
			}
			item->set_trace_flag(bTrace);
			//激活分配事件
			item->recycle_alloc_event();
		}
		return item;
	}

	recycle_pool_item * recycle_pool::add_alloc_fast_item(recycle_pool_item *item, bool bTrace)
	{
		if (item)
		{
			item->set_owner_flag(this);	
			item->reset_ref_count();

			if(bTrace)
			{
				boost::mutex::scoped_lock lock(m_mutex);
				m_used.push_back(item);
			}
			item->set_trace_flag(bTrace);
			//激活分配事件
			item->recycle_alloc_event();
		}
		return item;
	}

	void recycle_pool::release_item(recycle_pool_item *item)
	{
		if (item)
		{
			boost::mutex::scoped_lock lock(m_mutex);
			m_unused.push_back(item);
			
			if (item->get_trace_flag()) 
			{
				std::vector<recycle_pool_item *>::iterator itr = m_used.begin();
				for (;itr!=m_used.end();++itr)
				{
					if (item == *itr)
					{
						m_used.erase(itr);
						break;
					}
				}
			}

			//激活回收事件
			item->recycle_release_event();
		}
	}
	uint32_t recycle_pool::clear()
	{
		uint32_t nRet = m_used.size();
		boost::mutex::scoped_lock lock(m_mutex);
		//仅负责清空m_unused中节点，m_used作为内存泄露统计
		std::vector<recycle_pool_item *>::iterator it;
		for (it = m_unused.begin(); it != m_unused.end(); ++it)
		{
			recycle_pool_item *item = *it;
			if (item) delete item;
		}
		m_unused.clear();
		m_used.clear();
		return nRet;
	}

	///////////////////////////////////////////////////
	//recycle_queue
	bool recycle_queue::_push(recycle_pool_item *item)
	{
		bool bRet = false;
		
		if (item)
		{
			do 
			{
				if (0 == m_max_size) { bRet = true; break; }
				if (m_queue.size() == m_max_size) 
				{ 
					if (m_overlapped)
					{
						bRet = true;
						recycle_pool_item *ov_item = m_queue.front();
						m_queue.erase(m_queue.begin());
						//如果无内存管理器，则释放该节点空间
						if(0 > ov_item->release()) delete ov_item;
					}
					else
					{
						bRet = false; 
					}
					break; 
				}
				bRet = true;
			} while (false);

			if (bRet)
			{
				m_queue.push_back(item);
				item->assign();
			}
		}
		return bRet;
	}

	recycle_pool_item * recycle_queue::_pop(bool bRelease)
	{
		recycle_pool_item * item = 0;

		if (!m_queue.empty())
		{
			item = m_queue.front();
			m_queue.erase(m_queue.begin());
			if (bRelease) item->release();
		}

		return item;
	}

	bool recycle_queue::push(recycle_pool_item *item)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		return _push(item);
	}

	int recycle_queue::try_push(recycle_pool_item *item)
	{
		int nRet = -1;
		if (m_mutex.try_lock())
		{
			nRet = _push(item) ? 1 : 0;
			m_mutex.unlock();	
		}
		return nRet;
	}

	recycle_pool_item * recycle_queue::pop(bool bRelease)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		return _pop(bRelease);
	}

	recycle_pool_item * recycle_queue::try_pop(bool &lock_flag, bool bRelease)
	{
		recycle_pool_item * item = 0;
		lock_flag = false;
		if(m_mutex.try_lock())
		{
			lock_flag = true;
			item = _pop(bRelease);
			m_mutex.unlock();
		}
		return item;
	}

	void recycle_queue::clear()
	{
		boost::mutex::scoped_lock lock(m_mutex);
		std::vector<recycle_pool_item *>::iterator it;
		for (it = m_queue.begin(); it != m_queue.end(); ++it)
		{
			recycle_pool_item * item = *it;
			if (item)
			{
				//如果无内存管理器，则释放该节点空间
				if(0 > item->release()) delete item;
			}
		}
		m_queue.clear();
	}

	///////////////////////////////////////////////////
	//recycle_stack
	bool recycle_stack::_push(recycle_pool_item *item)
	{
		bool bRet = false;

		if (item)
		{
			do 
			{
				if (0 == m_max_size) { bRet = true; break; }
				if (m_stack.size() == m_max_size) { bRet = false; break; }
				bRet = true;
			} while (false);

			if (bRet)
			{
				m_stack.insert(m_stack.begin(), item);
				item->assign();
			}
		}
		return bRet;
	}

	recycle_pool_item * recycle_stack::_pop(bool bRelease)
	{
		recycle_pool_item * item = 0;

		if (!m_stack.empty())
		{
			item = m_stack.front();
			m_stack.erase(m_stack.begin());
			if (bRelease) item->release();
		}

		return item;
	}

	bool recycle_stack::push(recycle_pool_item *item)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		return _push(item);
	}

	int recycle_stack::try_push(recycle_pool_item *item)
	{
		int nRet = -1;
		if (m_mutex.try_lock())
		{
			nRet = _push(item) ? 1 : 0;
			m_mutex.unlock();	
		}
		return nRet;
	}

	recycle_pool_item * recycle_stack::pop(bool bRelease)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		return _pop(bRelease);
	}

	recycle_pool_item * recycle_stack::try_pop(bool &lock_flag, bool bRelease)
	{
		recycle_pool_item * item = 0;
		lock_flag = false;
		if(m_mutex.try_lock())
		{
			lock_flag = true;
			item = _pop(bRelease);
			m_mutex.unlock();
		}
		return item;	
	}

	void recycle_stack::clear()
	{
		boost::mutex::scoped_lock lock(m_mutex);
		std::vector<recycle_pool_item *>::iterator it;
		for (it = m_stack.begin(); it != m_stack.end(); ++it)
		{
			recycle_pool_item * item = *it;
			if (item)
			{
				//如果无内存管理器，则释放该节点空间
				if(0 > item->release()) delete item;
			}
		}
		m_stack.clear();
	}

	///////////////////////////////////////////////////
	// 工具函数
	int recycle_pool_mov_items(recycle_pool * src, recycle_pool * dst, int nums)
	{
		int nRet = 0;
		do 
		{
			//入口条件检查
			if ((!src) || (!dst)) break;
			if (0 >= nums) break;
			while (nRet < nums)
			{
				recycle_pool_item * item = src->alloc_item(false);
				if (!item) break;
				dst->mov_item(item);
				nRet++;
			}
		} while (false);
		return nRet;
	}

	int recycle_pool_mov_all_items(recycle_pool * src, recycle_pool * dst)
	{
		int nRet = 0;
		do 
		{
			//入口条件检查
			if ((!src) || (!dst)) break;
			int nums = src->freesize();
			if (0 >= nums) break;
			while (nRet < nums)
			{
				recycle_pool_item * item = src->alloc_item(false);
				if (!item) break;
				dst->mov_item(item);
				nRet++;
			}
		} while (false);
		return nRet;
	}
}