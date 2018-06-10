///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：recycle_pools.h
// 创 建 者：汤戈
// 创建时间：2012年03月17日
// 内容描述：循环内存池组
//
// 1、所有接口类均为多线程不安全，池组的生命周期必须有外部保证线程安全
//
// 修订日志
// [2012-03-17]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef RECYCLE_POOLS_
#define RECYCLE_POOLS_

#include<stdint.h>
#include "recycle_pool.h"
#include <map>

#if (_TGHELP_DEBUG_ENABLE)
#include <iostream>
#endif

namespace tghelper
{
	class recycle_pools
	{
	public:
		recycle_pools()
		{

		}

		virtual ~recycle_pools()
		{
			clear_pools();
		}

		//子池操作函数
		void add_pool(uint32_t key)
		{
			m_pools[key] = new recycle_pool();
		}

		void del_pool(uint32_t key)
		{
			if (0 == m_pools.count(key)) return;
			recycle_pool *pool = m_pools[key];
			if(pool) delete pool;
			m_pools.erase(key);
		}

		void clear_pools()
		{
			std::map<uint32_t, recycle_pool *>::iterator it;
			for (it = m_pools.begin(); it != m_pools.end(); it++)
			{
				recycle_pool *pool = it->second;
				if (pool) delete pool;
			}
			m_pools.clear();
		}

		recycle_pool * get_pools(uint32_t key)
		{
			recycle_pool * pool = 0;
			if (0 != m_pools.count(key))
			{
				pool = m_pools[key];
			}
			return pool;
		}

	protected:
		std::map<uint32_t, recycle_pool *> m_pools;

	};
}

#endif