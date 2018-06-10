///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����byte_pool.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��18��
// ����������ѭ���ֽ��ڴ��
//
// 1�����нӿ����Ϊ�̰߳�ȫ���̳���recycle_pool;
// 2������boost���е�mutex����ʵ���߳�ͬ��
// 3��recycle_poolû����Ԫ�����ͼ�飬����byte_pool�ر��˱�Ҫ�ĸ���ӿڣ�
//    ʹ��ʱ���ñ��ؽӿ���ɳز������Դ˱�֤�����ݵ�һ����
//
// �޶���־
// [2012-03-18]		���������汾
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

		//�������ݲ���
		// �ⲿ����ѹ�� -- ����ʵ��ѹ������ -- write
		inline void set_params(uint32_t payload_size, uint32_t offset)
		{
			m_payload_offset = offset;
			m_payload_size = payload_size;
		}
		uint32_t write(const uint8_t *src, uint32_t src_size, uint32_t offset);
		uint32_t write(const char *src, uint32_t src_size, uint32_t offset)
		{ return write((const uint8_t*)src, src_size, offset); }
		// �ڲ����ݵ��� -- ����ʵ�ʵ������� -- read
		// offset������Ч���offset���ݵ��������������payload����
		uint32_t read(uint8_t *dst, uint32_t dst_size, uint32_t *offset = 0);
		uint32_t read(char *dst, uint32_t dst_size, uint32_t *offset = 0)
		{ return read((uint8_t*)dst, dst_size, offset); }
		//�����Ʋ���--���ݿ��� -- memcpy
		bool copy(byte_block &src);
		bool copy(byte_block *src);
		
		bool available() { return (0 != m_block); }
		inline bool has_payload() { return (m_payload_size > 0); }
		inline uint32_t payload_offset() { return m_payload_offset; }
		inline uint32_t payload_size() { return m_payload_size; }
		inline uint32_t payload_totalsize() { return (m_payload_offset + m_payload_size); }
		inline uint8_t * get_raw() { return m_block;}


		//����������¼�
		virtual void recycle_alloc_event() 
		{
			//������滮�����Ϊ
		}	
		virtual void recycle_release_event() 
		{
			//�ڵ㱻������غ���Ҫ����ڲ���������Ϣ
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
		//�й�payload���� (m_payload_offset + m_payload_size) < m_block_size
		uint32_t m_payload_offset;			//��������ƫ����
		uint32_t m_payload_size;		//�������ݳ���
		
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

		//�������ݲ���
		// �ⲿ����д��, slice_offsetָʾ��ǰ��Ƭƫ���� -- ����ʵ��д�븺���ֽڳ��� 
		// ��������ǰ�����ݷ�Ƭ�Ѿ���ɣ�����slice_offset����ͷ�Ƭʱ��ƫ��������С�ڵ��ڵĹ�ϵ
		uint32_t write(const uint8_t *src, uint32_t src_size, 
					   uint32_t slice_size, uint32_t slice_offest);
		uint32_t write(const char *src, uint32_t src_size, 
					   uint32_t slice_size, uint32_t slice_offest) 
		{ return write((char *)src, src_size, slice_size, slice_offest); }
		// �ڲ����ݶ���, slice_offset�����Ƭ��ƫ����ֵ -- ����ʵ�ʶ��������ֽڳ���
		// ���������ݲ������κη�Ƭͷ�����ݡ���write�����������
		uint32_t read(uint8_t *dst, uint32_t dst_size, uint32_t *slice_offset = 0);
		uint32_t read(char *dst, uint32_t dst_size, uint32_t *slice_offset = 0)	
		{ return read((uint8_t*)dst, dst_size, slice_offset); }

		//�����Ʋ���--ָ�뿽�� -- 0������ʽ��������
		//�տ�������������Ƿ���Ϊʧ�ܵ��ã����ṩ�ڲ������ر���
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
		//����亯��
		void build_block(uint32_t block_nums);
		
		//��Ȼ���䣬����ת����İ汾
		inline byte_block * alloc(bool bTrace = false)
		{ return static_cast<byte_block *>(alloc_item(bTrace));}

		//ǿ�Ʒ���ģʽ������ڴ��û���㹻�ڴ棬ǿ��new����
		byte_block * force_alloc_item(bool bTrace = false);

		//���Է���ģʽ������ڴ��û���㹻�ڴ棬�����Զ���չ���壬ǿ�Ʒ������
		byte_block * try_alloc_item(bool bTrace = false);

		//����亯��--��ͨ�汾
		bool alloc_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace = false);

		//����亯��--ǿ�Ʋ����汾
		bool force_alloc_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace = false);

		//����亯��--�Զ���չ�汾
		bool try_alloc_items(byte_macro_block *dst, uint32_t block_nums, bool bTrace = false);

		//��Ƭ���㺯��--����0��ʾ����
		uint32_t calc_slice_nums(uint32_t src_size, uint32_t slice_size, uint32_t slice_offest);
	
	private:
		//����һ���ڵ㣬�ڵ�洢�ռ��ڲ�����
		void add_item(recycle_pool_item *item){ recycle_pool::add_item(item); }	
		//����һ���ڵ�
		recycle_pool_item * alloc_item(bool bTrace = false) { return recycle_pool::alloc_item(bTrace); }
		//���ټ��벢����ڵ�
		recycle_pool_item * add_alloc_fast_item(recycle_pool_item *item, bool bTrace = false)
		{ return recycle_pool::add_alloc_fast_item(item, bTrace);	}
		
	protected:
		uint32_t m_block_size;
		uint32_t m_expand_size;
	};

	/*
		elemT������byte_block��������
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

		//����亯��
		void build_blocks(uint32_t block_nums)
		{
			for (uint32_t i = 0; i < block_nums; i++)
			{
				elemT * item = new elemT(get_block_size());
				add_item(item);
			}
		}	

		//��Ȼ���䣬����ת����İ汾
		inline elemT * alloc_any(bool bTrace = false)
		{ return static_cast<elemT *>(alloc_item(bTrace)); }
		
		//ǿ�Ʒ���ģʽ������ڴ��û���㹻�ڴ棬ǿ��new����
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

		//���Է���ģʽ������ڴ��û���㹻�ڴ棬�����Զ���չ���壬ǿ�Ʒ������
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
		//����亯��--��ͨ�汾
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
		//����亯��--ǿ�Ʋ����汾
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

		//����亯��--�Զ���չ�汾
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

		//��Ƭ���㺯��--����0��ʾ����
		uint32_t calc_slice_nums(uint32_t src_size, uint32_t slice_size, uint32_t slice_offest)
		{
			if (get_block_size() < (slice_size + slice_offest)) return 0;

			uint32_t payload_size = slice_size;
			uint32_t block_nums = src_size / payload_size;
			if ((src_size % payload_size) > 0) block_nums++;
			return block_nums;
		}

	private:
		//����һ���ڵ㣬�ڵ�洢�ռ��ڲ�����
		void add_item(recycle_pool_item *item){ recycle_pool::add_item(item); }	
		//����һ���ڵ�
		recycle_pool_item * alloc_item(bool bTrace = false) { return recycle_pool::alloc_item(bTrace); }
		//���ټ��벢����ڵ�
		recycle_pool_item * add_alloc_fast_item(recycle_pool_item *item, bool bTrace = false)
		{ return recycle_pool::add_alloc_fast_item(item, bTrace);	}

	protected:
		uint32_t m_block_size;
		uint32_t m_expand_size;

	};

}


#endif
