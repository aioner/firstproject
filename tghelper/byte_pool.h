///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：byte_pool.h
// 创 建 者：汤戈
// 创建时间：2012年03月18日
// 内容描述：循环字节内存池
//
// 1、所有接口类均为线程安全，继承自recycle_pool;
// 2、采用boost库中的mutex对象实现线程同步
// 3、recycle_pool没有做元素类型检查，所以byte_pool关闭了必要的父类接口，
//    使用时采用本地接口完成池操作，以此保证池内容的一致性
//
// 修订日志
// [2012-03-18]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef BYTE_POOL_
#define BYTE_POOL_

#include<stdint.h>
#include "recycle_pool.h"

namespace tghelper
{
	namespace inner
	{
	}

	class byte_block : public recycle_pool_item
	{
	public:
		byte_block(const uint32_t block_size) : m_block_size(block_size)
		{
			m_block = new uint8_t[m_block_size];
			clear();
		}

		virtual ~byte_block()
		{
			if (m_block) delete [] m_block;
		}
		
		virtual uint32_t size() { return m_block_size; }
		virtual uint32_t size_bytes() { return m_block_size; }

		//数据内容操作
		// 外部内容压入 -- 返回实际压入数量 -- write
		inline void set_params(uint32_t payload_size, uint32_t offset)
		{
			m_payload_offset = offset;
			m_payload_size = payload_size;
		}
		uint32_t write(const uint8_t *src, uint32_t src_size, uint32_t offset);
		uint32_t write(const char *src, uint32_t src_size, uint32_t offset)
		{ return write((const uint8_t*)src, src_size, offset); }
		// 内部数据弹出 -- 返回实际弹出数量 -- read
		// offset变量有效则带offset内容弹出，否则仅弹出payload部分
		uint32_t read(uint8_t *dst, uint32_t dst_size, uint32_t *offset = 0);
		uint32_t read(char *dst, uint32_t dst_size, uint32_t *offset = 0)
		{ return read((uint8_t*)dst, dst_size, offset); }
		//对象复制操作--内容拷贝 -- memcpy
		bool copy(byte_block &src);
		bool copy(byte_block *src);
		
		bool available() { return (0 != m_block); }
		inline bool has_payload() { return (m_payload_size > 0); }
		inline uint32_t payload_offset() { return m_payload_offset; }
		inline uint32_t payload_size() { return m_payload_size; }
		inline uint32_t payload_totalsize() { return (m_payload_offset + m_payload_size); }
		inline uint8_t * get_raw() { return m_block;}


		//子类可重载事件
		virtual void recycle_alloc_event() 
		{
			//由子类规划相关行为
		}	
		virtual void recycle_release_event() 
		{
			//节点被回收入池后需要清空内部容器的信息
			clear();
		}

		void clear()
		{
			m_payload_offset = 0;
			m_payload_size = 0;
		}

	public:
		uint8_t *m_block;
		uint32_t m_block_size;
		//有关payload定义 (m_payload_offset + m_payload_size) < m_block_size
		uint32_t m_payload_offset;			//负荷内容偏移量
		uint32_t m_payload_size;		//负荷内容长度
		
	};
	
	class byte_macro_block: public recycly_macro_block
	{
	public:
		byte_macro_block() : m_payload_size(0)
		{

		}
		virtual ~byte_macro_block()
		{
		}

		void push_byte_block(byte_block *item, bool lockable = false)
		{
			if (lockable) m_container_mutex.lock();
			_push_block(item);
			m_payload_size += item->payload_size();
			if (lockable) m_container_mutex.unlock();
		}

		byte_block * pop_byte_block(bool lockable = false)
		{
			if (lockable) m_container_mutex.lock();
			byte_block * pItem = static_cast<byte_block *>(_pop_block(false));
			if (pItem)
			{
				m_payload_size -= pItem->payload_size();
			}
			if (lockable) m_container_mutex.unlock();
			return pItem;
		}

		virtual void clear(bool lockable = false)
		{
			if (lockable) m_container_mutex.lock();
			_clear();
			m_payload_size = 0;
			if (lockable) m_container_mutex.unlock();
		}

		inline uint32_t payload_size() { return m_payload_size; }

		//数据内容操作
		// 外部数据写入, slice_offset指示当前分片偏移量 -- 返回实际写入负荷字节长度 
		// 函数调用前，数据分片已经完成，并且slice_offset必须和分片时的偏移量保持小于等于的关系
		uint32_t write(const uint8_t *src, uint32_t src_size, 
					   uint32_t slice_size, uint32_t slice_offest);
		uint32_t write(const char *src, uint32_t src_size, 
					   uint32_t slice_size, uint32_t slice_offest) 
		{ return write((char *)src, src_size, slice_size, slice_offest); }
		// 内部数据读出, slice_offset输出分片的偏移量值 -- 返回实际读出负荷字节长度
		// 读出的数据不包含任何分片头部数据。与write函数配套理解
		uint32_t read(uint8_t *dst, uint32_t dst_size, uint32_t *slice_offset = 0);
		uint32_t read(char *dst, uint32_t dst_size, uint32_t *slice_offset = 0)	
		{ return read((uint8_t*)dst, dst_size, slice_offset); }

		//对象复制操作--指针拷贝 -- 0拷贝方式交换数据
		//空拷贝或输入参数非法视为失败调用，不提供内部的锁控保护
		bool copy(byte_macro_block &src);
		bool copy(byte_macro_block *src);

	private:
		uint32_t m_payload_size;
	};

	class byte_pool : public recycle_pool
	{
	public:
		byte_pool(uint32_t block_size, 
				  uint32_t expand_size, 
				  uint32_t init_blocks) : m_block_size(block_size), m_expand_size(expand_size)
		{
			build_block(init_blocks);
		}
		virtual ~byte_pool()
		{

		}

		inline uint32_t get_block_size() { return m_block_size; }
		inline uint32_t get_expand_size(){ return m_expand_size; }
		//池填充函数
		void build_block(uint32_t block_nums);
		
		//自然分配，类型转换后的版本
		inline byte_block * alloc(bool bTrace = false)
		{ return static_cast<byte_block *>(alloc_item(bTrace));}

		//强制分配模式，如果内存池没有足够内存，强制new出来
		byte_block * force_alloc_item(bool bTrace = false);

		//尝试分配模式，如果内存池没有足够内存，存在自动扩展定义，强制分配出来
		byte_block * try_alloc_item(bool bTrace = false);

		//组分配函数--普通版本
		bool alloc_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace = false);

		//组分配函数--强制操作版本
		bool force_alloc_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace = false);

		//组分配函数--自动扩展版本
		bool try_alloc_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace = false);

		//分片计算函数--返回0表示错误
		uint32_t calc_slice_nums(uint32_t src_size, uint32_t slice_size, uint32_t slice_offest);
	
	private:
		//加入一个节点，节点存储空间内部管理
		void add_item(recycle_pool_item *item){ recycle_pool::add_item(item); }	
		//分配一个节点
		recycle_pool_item * alloc_item(bool bTrace = false) { return recycle_pool::alloc_item(bTrace); }
		//快速加入并分配节点
		recycle_pool_item * add_alloc_fast_item(recycle_pool_item *item, bool bTrace = false)
		{ return recycle_pool::add_alloc_fast_item(item, bTrace);	}
		
	protected:
		uint32_t m_block_size;
		uint32_t m_expand_size;
	};

	/*
		elemT必须是byte_block或其子类
	*/
	template<typename elemT, 
		     uint32_t block_size,
			 uint32_t expand_size,
			 uint32_t init_blocks>
	class any_byte_pool : public recycle_pool
	{
	public:
		any_byte_pool()
		{
			m_block_size = block_size;
			m_expand_size = expand_size;
			build_blocks(init_blocks);
		}

		virtual ~any_byte_pool()
		{

		}
		
		inline uint32_t get_block_size() { return m_block_size; }
		inline uint32_t get_expand_size(){ return m_expand_size; }

		//池填充函数
		void build_blocks(uint32_t block_nums)
		{
			for (uint32_t i = 0; i < block_nums; i++)
			{
				elemT * item = new elemT(get_block_size());
				add_item(item);
			}
		}	

		//自然分配，类型转换后的版本
		inline elemT * alloc_any(bool bTrace = false)
		{ return static_cast<elemT *>(alloc_item(bTrace)); }
		
		//强制分配模式，如果内存池没有足够内存，强制new出来
		elemT * force_alloc_any(bool bTrace = false)
		{
			elemT * pRet = 0;
			pRet = static_cast<elemT *>(alloc_item(bTrace));
			if (!pRet)
			{
				pRet = new elemT(get_block_size());
				pRet = static_cast<elemT *>(add_alloc_fast_item(pRet, bTrace));
			}
			return pRet;
		}

		//尝试分配模式，如果内存池没有足够内存，存在自动扩展定义，强制分配出来
		elemT * try_alloc_any(bool bTrace = false)
		{
			elemT * pRet = 0;
			pRet = static_cast<elemT *>(alloc_item(bTrace));
			if (!pRet && (0 < m_expand_size))
			{
				if (1 < m_expand_size)
				{
					for (uint32_t i = 0; i < (m_expand_size - 1); i++)
					{	
						elemT * item = new elemT(get_block_size());
						add_item(item);
					}
				}

				pRet = new elemT(get_block_size());
				pRet = static_cast<elemT *>(add_alloc_fast_item(pRet, bTrace));
			}
			return pRet;
		}
		//组分配函数--普通版本
		bool alloc_any_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace)
		{
			bool bRet = false;
			m_mutex.lock();
			do 
			{
				if (!dst) break;
				if (freesize() < block_nums) break;
				for (uint32_t i = 0; i < block_nums; i++)
				{
					elemT * item = static_cast<elemT *>(_alloc_item(bTrace));
					dst->push_byte_block(item);
				}
				bRet = true;
			} while (false);
			m_mutex.unlock();
			return bRet;
		}
		//组分配函数--强制操作版本
		bool force_alloc_any_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace)
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
					elemT * item = static_cast<elemT *>(_alloc_item(bTrace));
					dst->push_byte_block(item);
				}
				if (alloc_nums < block_nums)
				{
					alloc_nums = block_nums - alloc_nums;
					for (uint32_t j = 0; j < alloc_nums; j++)
					{
						elemT * item = new elemT(get_block_size());
						item = static_cast<elemT *>(_add_alloc_fast_item(item, bTrace));
						dst->push_byte_block(item);
					}
				}
				bRet = true;
			} while (false);
			m_mutex.unlock();
			return bRet;
		}

		//组分配函数--自动扩展版本
		bool try_alloc_any_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace = false)
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
					elemT * item = static_cast<elemT *>(_alloc_item(bTrace));
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
						elemT * item = new elemT(get_block_size());
						if (j < alloc_nums)
						{
							item = static_cast<elemT *>(_add_alloc_fast_item(item, bTrace));
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

		//分片计算函数--返回0表示错误
		uint32_t calc_slice_nums(uint32_t src_size, uint32_t slice_size, uint32_t slice_offest)
		{
			if (get_block_size() < (slice_size + slice_offest)) return 0;

			uint32_t payload_size = slice_size;
			uint32_t block_nums = src_size / payload_size;
			if ((src_size % payload_size) > 0) block_nums++;
			return block_nums;
		}

	private:
		//加入一个节点，节点存储空间内部管理
		void add_item(recycle_pool_item *item){ recycle_pool::add_item(item); }	
		//分配一个节点
		recycle_pool_item * alloc_item(bool bTrace = false) { return recycle_pool::alloc_item(bTrace); }
		//快速加入并分配节点
		recycle_pool_item * add_alloc_fast_item(recycle_pool_item *item, bool bTrace = false)
		{ return recycle_pool::add_alloc_fast_item(item, bTrace);	}

	protected:
		uint32_t m_block_size;
		uint32_t m_expand_size;

	};

}


#endif
