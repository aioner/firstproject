//#include "stdafx.h"
#include "rtp_packet_block.h"
#include "packet_sink.h"

namespace xt_mp_sink
{
	bool rtp_packet_sink::_push( rtp_macro_block *item )
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
						rtp_macro_block *ov_item = m_queue.front();
						m_queue.erase(m_queue.begin());
						//如果无内存管理器，则释放该节点空间
						if(0 > ov_item->release()) utility::destruct_ptr(ov_item);
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

	rtp_macro_block * rtp_packet_sink::_pop( bool bRelease )
	{
		rtp_macro_block * item = 0;

		if (!m_queue.empty())
		{
			item = m_queue.front();
			m_queue.erase(m_queue.begin());
			if (bRelease) item->release();
		}

		return item;
	}

	bool rtp_packet_sink::push( rtp_macro_block *item )
	{
		boost::mutex::scoped_lock lock(m_mutex);
		return _push(item);
	}

	int rtp_packet_sink::try_push( rtp_macro_block *item )
	{
		int nRet = -1;
		if (m_mutex.try_lock())
		{
			nRet = _push(item) ? 1 : 0;
			m_mutex.unlock();	
		}
		return nRet;
	}

	rtp_macro_block * rtp_packet_sink::pop( bool bRelease /*= false*/ )
	{
		boost::mutex::scoped_lock lock(m_mutex);
		return _pop(bRelease);
	}

	rtp_macro_block * rtp_packet_sink::front()
	{
		boost::mutex::scoped_lock lock(m_mutex);
		if (m_queue.size() > 0)
		{
			return m_queue.front();
		}
		else
		{
			return NULL;
		}
	}

	rtp_macro_block * rtp_packet_sink::try_pop( bool &lock_flag, bool bRelease /*= false*/ )
	{
		rtp_macro_block * item = 0;
		lock_flag = false;
		if(m_mutex.try_lock())
		{
			lock_flag = true;
			item = _pop(bRelease);
			m_mutex.unlock();
		}
		return item;
	}

	void rtp_packet_sink::clear()
	{
		boost::mutex::scoped_lock lock(m_mutex);
		std::vector<rtp_macro_block *>::iterator it;
		for (it = m_queue.begin(); it != m_queue.end(); ++it)
		{
			rtp_macro_block * item = *it;
			if (item)
			{
				//如果无内存管理器，则释放该节点空间
				if(0 > item->release()) 
					utility::destruct_ptr(item);
			}
		}
		m_queue.clear();
	}

	uint32_t rtp_packet_sink::front_data_size()
	{
		int total = 0;
		do 
		{
			boost::mutex::scoped_lock lock(m_mutex);
			if(m_queue.empty()) break;
			rtp_macro_block * macro = static_cast<rtp_macro_block*>(m_queue.front());
			if(macro == null) break;
			total = macro->get_data_size();
		} while (false);
		return total;
	}

	uint32_t rtp_packet_sink::front_real_size()
	{
		int total = 0;
		do 
		{
			boost::mutex::scoped_lock lock(m_mutex);
			if(m_queue.empty()) break;
			rtp_macro_block * macro = static_cast<rtp_macro_block*>(m_queue.front());
			if(macro == null) break;
			total = macro->get_real_size();
		} while (false);
		return total;
	}

	uint32_t rtp_packet_sink::size()
	{
		boost::mutex::scoped_lock lock(m_mutex);
		return m_queue.size();
	}

	uint32_t rtp_packet_sink::empty()
	{
		boost::mutex::scoped_lock lock(m_mutex);
		return m_queue.empty() ? 1 : 0;
	}

}