///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����recycle_pools.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��17��
// ����������ѭ���ڴ����
//
// 1�����нӿ����Ϊ���̲߳���ȫ��������������ڱ������ⲿ��֤�̰߳�ȫ
//
// �޶���־
// [2012-03-17]		���������汾
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

		//�ӳز�������
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