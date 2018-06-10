///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：byte_pool.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月18日
// 内容描述：循环字节内存池
//
// 修订日志
// [2012-03-18]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////
#include "byte_pool.h"

namespace tghelper
{
	namespace inner
	{

	}
	///////////////////////////////////////////////////////////////////////////////////////////
	// byte_block
	uint32_t byte_block::write(const uint8_t *src, uint32_t src_size, uint32_t offset)
	{
		uint32_t nRet = 0;
		do 
		{
			if (!m_block) break;
			if (!src) break;
			if (src_size + offset > m_block_size) break;
			//内存数据复制
			memcpy(m_block + offset, src, src_size);
			m_payload_offset = offset;
			m_payload_size = src_size;
			nRet = src_size + offset;
		} while (false);
		return nRet;
	}
	
	uint32_t byte_block::read(uint8_t *dst, uint32_t dst_size, uint32_t* offset)
	{
		uint32_t nRet = 0;

		do 
		{
			if (!m_block) break;
			if (!dst) break;
			if (offset)
			{
				if (dst_size < (m_payload_size + m_payload_offset)) break;
				*offset = m_payload_offset;
				nRet = m_payload_size + m_payload_offset;
			}
			else
			{
				if (dst_size < m_payload_size) break;
				nRet = m_payload_size;
			}
			//内存数据复制
			if (offset)
				memcpy(dst, m_block, m_payload_size + m_payload_offset);
			else
				memcpy(dst, m_block + m_payload_offset, m_payload_size);
		} while (false);

		return nRet;
	}
	
	bool byte_block::copy(byte_block &src)
	{
		bool bRet = false;
		do 
		{
			if (!src.available()) break;
			if (!src.has_payload()) break;
			if (src.payload_totalsize() > m_block_size) break;
			m_payload_offset = src.payload_offset();
			m_payload_size = src.payload_size();
			memcpy(m_block, src.get_raw(), src.payload_totalsize());
			bRet = true;
		} while (false);
		return bRet;
	}
	bool byte_block::copy(byte_block *src)
	{
		bool bRet = false;
		do 
		{
			if (!src) break;
			if (!src->available()) break;
			if (!src->has_payload()) break;
			if (src->payload_totalsize() > m_block_size) break;
			m_payload_offset = src->payload_offset();
			m_payload_size = src->payload_size();
			memcpy(m_block, src->get_raw(), src->payload_totalsize());
			bRet = true;
		} while (false);
		return bRet;
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// byte_macro_block
	uint32_t byte_macro_block::write(const uint8_t *src, uint32_t src_size, 
									 uint32_t slice_size, uint32_t slice_offest)
	{
		uint32_t nRet = 0;
		do 
		{
			if (!src) break;
			if (!slice_size) break;	//未指定分片长度
			if (src_size > (slice_size * size())) break;
			const uint8_t * addr_pos = src;
			uint32_t  size_pos = src_size;
			m_payload_size = 0;
			std::vector<recycle_pool_item *>::iterator it;
			for (it = m_container.begin(); it != m_container.end(); it++)
			{
				uint32_t nWrite = (size_pos > slice_size) ? slice_size : size_pos;
				uint32_t nPush = static_cast<byte_block *>(*it)->write(addr_pos, nWrite, slice_offest);
				nPush -= slice_offest; 
				addr_pos += nPush;
				size_pos -= nPush;
				//修正当前payload_size总长度
				m_payload_size += nPush;
			}
			nRet = src_size;
		} while (false);
		return nRet;
	}

	uint32_t byte_macro_block::read(uint8_t *dst, uint32_t dst_size, uint32_t *slice_offset)
	{
		uint32_t nRet = 0;
		do 
		{
			if (!dst) break;
			if (dst_size < payload_size()) break;
			uint8_t * addr_pos = dst;
			uint32_t  size_pos = dst_size;
			if (m_container.empty()) break;
			if (slice_offset) *slice_offset = static_cast<byte_block *>(m_container.front())->payload_offset();
			std::vector<recycle_pool_item *>::iterator it;
			for (it = m_container.begin(); it != m_container.end(); it++)
			{
				uint32_t nPop = static_cast<byte_block *>(*it)->read(addr_pos, size_pos);
				addr_pos += nPop;
				size_pos -= nPop;
			}
			nRet = payload_size();
		} while (false);
		return nRet;
	}

	bool byte_macro_block::copy(byte_macro_block &src)
	{
		bool bRet = false;
		do 
		{
			//空复制
			if (0 == src.payload_size()) break;	
			clear(false);
			std::vector<recycle_pool_item *>::iterator it;
			for (it = src.get_container().begin(); it != src.get_container().end(); it++)
			{
				push_byte_block(static_cast<byte_block *>(*it), false);
			}
			bRet = true;
		} while (false);
		return bRet;
	}

	bool byte_macro_block::copy(byte_macro_block *src)
	{
		bool bRet = false;
		do 
		{
			//无效参数
			if (!src) break;
			//空复制
			if (0 == src->payload_size()) break;
			clear(false);
			std::vector<recycle_pool_item *>::iterator it;
			for (it = src->get_container().begin(); it != src->get_container().end(); it++)
			{
				push_byte_block(static_cast<byte_block *>(*it), false);
			}
			bRet = true;
		} while (false);
		return bRet;
	}


	///////////////////////////////////////////////////////////////////////////////////////////
	// byte_pool
	void byte_pool::build_block(uint32_t block_nums)
	{
		for (uint32_t i = 0; i < block_nums; i++)
		{
			byte_block * item = new byte_block(get_block_size());
			add_item(item);
		}
	}

	byte_block * byte_pool::force_alloc_item(bool bTrace)
	{
		byte_block * pRet = 0;
		pRet = static_cast<byte_block *>(alloc_item(bTrace));
		if (!pRet)
		{
			pRet = new byte_block(get_block_size());
			pRet = static_cast<byte_block *>(add_alloc_fast_item(pRet, bTrace));
		}
		return pRet;
	}

	byte_block * byte_pool::try_alloc_item(bool bTrace)
	{
		byte_block * pRet = 0;
		pRet = static_cast<byte_block *>(alloc_item(bTrace));
		if (!pRet && (0 < m_expand_size))
		{
			if (1 < m_expand_size)
			{
				for (uint32_t i = 0; i < (m_expand_size - 1); i++)
				{	
					byte_block * item = new byte_block(get_block_size());
					add_item(item);
				}
			}

			pRet = new byte_block(get_block_size());
			pRet = static_cast<byte_block *>(add_alloc_fast_item(pRet, bTrace));
		}
		return pRet;
	}

	bool byte_pool::alloc_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace)
	{
		bool bRet = false;
		m_mutex.lock();
		do 
		{
			if (!dst) break;
			if (freesize() < block_nums) break;
			for (uint32_t i = 0; i < block_nums; i++)
			{
				byte_block * item = static_cast<byte_block *>(_alloc_item(bTrace));
				dst->push_byte_block(item);
			}
			bRet = true;
		} while (false);
		m_mutex.unlock();
		return bRet;
	}

	bool byte_pool::force_alloc_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace)
	{
		bool bRet = false;
		m_mutex.lock();
		do 
		{
			if (!dst) break;
			uint32_t alloc_nums = block_nums;
			if (freesize() < block_nums)
			{
				alloc_nums = freesize();
			}
			for (uint32_t i = 0; i < alloc_nums; i++)
			{
				byte_block * item = static_cast<byte_block *>(_alloc_item(bTrace));
				dst->push_byte_block(item);
			}
			if (alloc_nums < block_nums)
			{
				alloc_nums = block_nums - alloc_nums;
				for (uint32_t j = 0; j < alloc_nums; j++)
				{
					byte_block * item = new byte_block(get_block_size());
					item = static_cast<byte_block *>(_add_alloc_fast_item(item, bTrace));
					dst->push_byte_block(item);
				}
			}
			bRet = true;
		} while (false);
		m_mutex.unlock();
		return bRet;
	}

	bool byte_pool::try_alloc_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace)
	{
		bool bRet = false;
		m_mutex.lock();
		do 
		{
			if (!dst) break;
			uint32_t alloc_nums = block_nums;
			if ((freesize() < block_nums) && (0 == m_expand_size)) break;
			if (freesize() < block_nums)
			{
				alloc_nums = freesize();
			}
			for (uint32_t i = 0; i < alloc_nums; i++)
			{
				byte_block * item = static_cast<byte_block *>(_alloc_item(bTrace));
				dst->push_byte_block(item);
			}
			if (alloc_nums < block_nums)
			{
				alloc_nums = block_nums - alloc_nums;
				uint32_t add_nums = 0;
				if (alloc_nums < m_expand_size)
				{
					add_nums = m_expand_size - alloc_nums;
				}
				else
				{
					add_nums = (alloc_nums / m_expand_size) * m_expand_size + m_expand_size - alloc_nums;
				}
				for (uint32_t j = 0; j < (alloc_nums + add_nums); j++)
				{
					byte_block * item = new byte_block(get_block_size());
					if (j < alloc_nums)
					{
						item = static_cast<byte_block *>(_add_alloc_fast_item(item, bTrace));
						dst->push_byte_block(item);
					}
					else
						_add_item(item);
				}
			}
			bRet = true;
		} while (false);
		m_mutex.unlock();
		return bRet;
	}

	//分片计算函数
	uint32_t byte_pool::calc_slice_nums(uint32_t src_size, uint32_t slice_size, uint32_t slice_offest)
	{
		if (get_block_size() < (slice_size + slice_offest)) return 0;

		uint32_t payload_size = slice_size;
		uint32_t block_nums = src_size / payload_size;
		if ((src_size % payload_size) > 0) block_nums++;
		return block_nums;
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// any_byte_pool
	

}