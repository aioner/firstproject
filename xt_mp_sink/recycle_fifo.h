#ifndef RECYCLE_FIFO_H
#define RECYCLE_FIFO_H
#include "sink_common.h"

namespace xt_mp_sink
{
	template<typename elemT>
	class recycle_fifo : private boost::noncopyable
	{
	public:
		recycle_fifo(uint32_t max_size,bool overlapped)
			: m_max_size(max_size)
			, m_bOverlapped(overlapped){}
		virtual ~recycle_fifo(){}

	public:
		//压入队列自动调用item->assign()
		bool push(const elemT& item)
		{
			boost::mutex::scoped_lock lock(m_mutex);
			return _push(item);
		}
		int try_push(const elemT& item)
		{
			int nRet = -1;
			if (m_mutex.try_lock())
			{
				nRet = _push(item) ? 1 : 0;
				m_mutex.unlock();	
			}
			return nRet;
		}
		bool pop()
		{
			boost::mutex::scoped_lock lock(m_mutex);
			return _pop();
		}
		int try_pop()
		{
			int ret = -1;
			if(m_mutex.try_lock())
			{
				ret = _pop() ? 1 : 0;
				m_mutex.unlock();
				return ret;
			}
			return ret;	
		}
		void clear()
		{
			boost::mutex::scoped_lock lock(m_mutex);
			m_fifo.clear();
		}
		elemT& front()
		{
			boost::mutex::scoped_lock lock(m_mutex);
			return m_fifo.front();
		}

		inline int size() { return m_fifo.size(); }		//函数线程不安全

	private:
		bool _push(const elemT& item)
		{
			bool bRet = false;

			do 
			{
				if (0 == m_max_size) { bRet = true; break; }
				if (m_fifo.size() == m_max_size) 
				{ 
					if (m_bOverlapped)
					{
						bRet = true;
						m_fifo.erase(m_fifo.begin());
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
				m_fifo.push_back(item);
#if DEBUG_OUTPUT
				std::cout << "rtcp fifo push_back,fifo size " << m_fifo.size() << std::endl;
#endif
			}

			return bRet;
		}

		bool _pop()
		{
			if (!m_fifo.empty())
			{
				m_fifo.erase(m_fifo.begin());
				return true;
			}

			return false;
		}

	private:
		bool		m_bOverlapped;
		uint32_t	m_max_size;
		std::vector<elemT>	m_fifo;
		boost::mutex	m_mutex;
	};

}

#endif 
