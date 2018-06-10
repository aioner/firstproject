///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����recycle_pool.h
// �� �� �ߣ�����
// ����ʱ�䣺2011��02��27��
// ����������ѭ���ڴ��
//
// 1�����нӿ����Ϊ�̰߳�ȫ;
//		a, ѭ���ڴ�ػ������õ�Ԫ
//		recycle_pool_item-->Ԫ����
//		recycle_block-->�ڴ��ģ�����
//		recycle_pool-->������
//		b, ѭ���ڴ���������ݲ��ݵ�Ԫ
//		recycle_queue-->ѭ��FIFO����
//		recycle_stack-->ѭ��LIFO��ջ
//
// 2������boost���е�mutex����ʵ���߳�ͬ��
// 3������stl���е�list����ʵ�ʴ洢����
// 4���ڵ����ü����Ĳ���Ӧ����Բ���
//		recycle_pool_item->assign()
//		recycle_pool_item->release()
// 5���ڵ�һ������������£�
//		a, ͨ������recycle_pool.add_item()��mov_item()���������ڴ�ؿ��нڵ㣬��ֵ����
//		b, ͨ������recycle_pool.alloc_item()������ÿɽ����ڴ�ع���Ľڵ㣬�ýڵ����ü���Ϊ0��ȡ�����
//		c, ͨ������recycle_pool_item->assign()������Ǹ��ڴ��ʹ���������������
//		d, ͨ������recycle_pool_item->release()������Ǹ��ڴ�Ļ�����������Ϊ0�����Զ��ع��ڴ�أ��ջ����
//		e, ͨ������recycle_pool.del_item()��mov_item()�������ýڵ�ı�ع����ϵ��
//		f, �޳ع������Ľڵ���release���ü���Ϊ0��᷵��-1����ʾ�û��ýڵ���Ҫ���Ƕ����ͷ���Դ��
// 6���ڵ��ṩ�����¼���������������⴦��
//		recycle_pool_item->recycle_alloc_event()		�ڵ�ӳ��б��������
//		recycle_pool_item->recycle_release_event()		�ڵ㱻�ɹ��ͷŻس�
//
// �޶���־
// [2011-02-27]		���������汾
// [2012-03-18]		�����ڴ�Ƭ�ⵥԪ����recycly_marco_block
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef RECYCLE_POOL_
#define RECYCLE_POOL_

#include<stdint.h>
#include <string.h>
#include <vector>
#include <boost/thread/mutex.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>

#if (_TGHELP_DEBUG_ENABLE)
	#include <iostream>
#endif

namespace tghelper
{
	namespace inner
	{
		//�˴���ö�ٶ���û��ʵ�����壬���ṩһ��recycle_pool_item::release()����ֵ��˵��
		typedef enum _ERecycle_Pool_Item_Release
		{
			ISOLATE = -1,	//����״̬�����û��Լ��ͷŶ���
			IDLE = 0,		//����״̬�����󷵻سض���
			USED,			//ʹ��״̬��׼ȷ�����Ǵ���0��������ʾ��ǰ���ü���ֵ
		} EState;
	}

	class recycle_pool;
	class recycle_pool_item : private boost::noncopyable
	{
		friend class recycle_pool;
		friend class recycle_queue;
		friend class recycle_stack;
	protected:
		//��ֹ�û�ֱ�ӷ���û���
		recycle_pool_item() :
		   m_ref_count(0),
		   m_owner(0),
		   m_pool_trace(false)
		{	}
		virtual ~recycle_pool_item()
		{
		#if (_TG_DEBUG_ENABLE)
			std::cout << "~recycle_pool_item, ref = " << m_ref_count << std::endl;
		#endif
		}

		inline void set_trace_flag(bool bTrace) { m_pool_trace = bTrace; }
		inline bool get_trace_flag() { return m_pool_trace; }
		inline void set_owner_flag(recycle_pool * owner)
		{ boost::mutex::scoped_lock lock(m_mutex); m_owner = owner; }
		inline recycle_pool * get_owner_flag() { return m_owner; }
		inline void reset_ref_count() { m_ref_count = 0; }

	public:
		int assign();
		int release();
		inline int use_count() { return m_ref_count; }

		//����������¼�
		virtual void recycle_alloc_event() {}
		virtual void recycle_release_event() {}

		virtual uint32_t size() { return 0; }			//���ô洢��Ԫ����
		virtual uint32_t size_bytes() { return 0; }		//���ô洢��Ԫ�ֽڳ���

	private:
		int m_ref_count;
		recycle_pool * m_owner;
		boost::mutex m_mutex;
		bool m_pool_trace;
	};

	class recycle_pool
	{
	public:
		recycle_pool(void);
		virtual ~recycle_pool(void);

	private:
		friend class recycle_pool_item;
		//����һ���ڵ�
		void release_item(recycle_pool_item *item);

	protected:
		//�ڲ�ʹ�õķǼ����汾
		recycle_pool_item * _alloc_item(bool bTrace = false);
		recycle_pool_item * _add_alloc_fast_item(recycle_pool_item *item, bool bTrace = false);
		void _add_item(recycle_pool_item *item);
		void _del_item(recycle_pool_item *item);

	public:
		//�ڵ����״̬����
		//����һ���ڵ㣬�ڵ�洢�ռ��ڲ�����
		void add_item(recycle_pool_item *item);
		//�Ƴ�һ���ڵ㣬�ڵ�洢�ռ����ⲿ����
		void del_item(recycle_pool_item *item);
		//����һ���ڵ�
		void mov_item(recycle_pool_item *item);
		//����һ���ڵ�
		recycle_pool_item * alloc_item(bool bTrace = false);
		//���ټ��벢����ڵ�
		recycle_pool_item * add_alloc_fast_item(recycle_pool_item *item, bool bTrace = false);

		//����ڴ��δ����ڵ㣬����δ���սڵ�����
		uint32_t clear();

		//״̬��Ϣ
		inline uint32_t freesize() { return m_unused.size(); }
		inline uint32_t tracesize(){ return m_used.size(); }

	protected:
		std::vector<recycle_pool_item *> m_unused;		//δʹ���б�
		std::vector<recycle_pool_item *> m_used;			//��ʹ���б�
		boost::mutex m_mutex;
	};

	template<typename elemT, int elemNums>
	class recycle_block : public recycle_pool_item
	{
	public:
		recycle_block()
		{
			m_elements = new elemT[elemNums];
		}
		virtual ~recycle_block()
		{
			if(m_elements)	delete [] m_elements;
		}

		virtual uint32_t size() { return elemNums; }
		virtual uint32_t size_bytes() { return sizeof(elemT) * elemNums; }

		inline void empty(uint8_t v = 0x00){ ::memset(m_elements, v, size_bytes()); }

		inline bool copy(recycle_block * pBlock)
		{
			bool bRet = false;
			if (pBlock && (pBlock->size_bytes() == size_bytes()))
			{
				//�ȳ����ƹ���
				::memcpy(m_elements, pBlock->m_elements, size_bytes());
				bRet = true;
			}
			return bRet;
		}

		inline elemT * get_data() { return m_elements; }

	private:
		elemT *m_elements;
	};

	class recycle_queue : private boost::noncopyable
	{
		//FIFO����
	public:
		recycle_queue(uint32_t max_size, bool overlapped) :
			m_max_size(max_size),		//0 == m_max_size���Ȳ�������
			m_overlapped(overlapped)	//����д����, �Զ��ͷ�ͷ������
		{

		}

		~recycle_queue()
		{
			clear();
		}

	public:
		//ѹ������Զ�����item->assign()
		bool push(recycle_pool_item *item);				//æ������ʽ
		int try_push(recycle_pool_item *item);			//��������ʽ
		//��������ѡ���Ե���item->release()
		recycle_pool_item * pop(bool bRelease = false);		//æ������ʽ
		recycle_pool_item * try_pop(/*OUT*/bool &lock_flag, bool bRelease = false);	//��������ʽ
		void clear();

		inline int size() 
		{ 
			//boost::detail::spinlock::scoped_lock lock(dummy);
			boost::mutex::scoped_lock lock(m_mutex);
			return m_queue.size(); 
		}

	private:
		bool _push(recycle_pool_item *item);			//�����ذ汾
		recycle_pool_item * _pop(bool bRelease = false);//�����ذ汾

	private:
		std::vector<recycle_pool_item *> m_queue;
		boost::mutex m_mutex;
		uint32_t m_max_size;
		bool m_overlapped;
		//boost::detail::spinlock dummy;
	};

	class recycle_stack : private boost::noncopyable
	{
		//LIFO����
	public:
		recycle_stack(uint32_t max_size) :
		  m_max_size(max_size)		//0 == m_max_size���Ȳ�������
		  {

		  }

		~recycle_stack()
		  {
			  clear();
		  }

	public:
		//ѹ���ջ�Զ�����item->assign()
		bool push(recycle_pool_item *item);				//æ������ʽ
		int try_push(recycle_pool_item *item);			//��������ʽ
		//������ջѡ���Ե���item->release()
		recycle_pool_item * pop(bool bRelease = false);
		recycle_pool_item * try_pop(/*OUT*/bool &lock_flag, bool bRelease = false);	//��������ʽ
		void clear();

		inline int size() { return m_stack.size(); }	//�����̲߳���ȫ

	private:
		bool _push(recycle_pool_item *item);			//�����ذ汾
		recycle_pool_item * _pop(bool bRelease = false);//�����ذ汾

	private:
		std::vector<recycle_pool_item *> m_stack;
		boost::mutex m_mutex;
		uint32_t m_max_size;
	};

	class recycly_macro_block: public recycle_pool_item
	{
		/*
			 ��������ݽӿڿ�ѡ���Ƿ�߱��߳�ͬ����ȫ��
			 ��ʹ�������߳�ͬ������ʱ���ⲿģ�����������������ʱȷ������ȫ
			 1������push_blockѹ�����ݿ��Ĭ���Զ��Ը����ݿ����ü�����1����
			 2������pop_block�������ݿ��Ĭ���Զ��Ը����ݿ����ü�����1����
			 �������ַ�ʽ��Ҫ���ʹ�ã�����ᵼ���ڴ�й¶
		*/
	protected:
		void _push_block(recycle_pool_item *item)
		{
			m_container.push_back(item);
			m_container_size_bytes += item->size_bytes();
			item->assign();
		}
		recycle_pool_item * _pop_block(bool bRelease = false)
		{
			recycle_pool_item * pItem = 0;
			if (!m_container.empty())
			{
				pItem = m_container.front();
				m_container.erase(m_container.begin());
				m_container_size_bytes -= pItem->size_bytes();
				if (bRelease) pItem->release();
			}
			return pItem;
		}
		void _clear()
		{
			std::vector<recycle_pool_item *>::iterator it;
			for (it = m_container.begin(); it != m_container.end(); it++)
			{
				(*it)->release();
			}
			m_container.clear();
			m_container_size_bytes = 0;
		}
	public:
		recycly_macro_block() : m_container_size_bytes(0)
		{
			_clear();
		}
		virtual ~recycly_macro_block()
		{
			_clear();
		}

		virtual uint32_t size() { return m_container.size(); }			//���ô洢��Ԫ����
		virtual uint32_t size_bytes()									//���ô洢��Ԫ�ֽ��ܳ���
		{ return m_container_size_bytes; }

		void push_block(recycle_pool_item *item, bool lockable = false)
		{
			if (lockable) m_container_mutex.lock();
			_push_block(item);
			if (lockable) m_container_mutex.unlock();
		}

		recycle_pool_item * pop_block(bool lockable = false)
		{
			if (lockable) m_container_mutex.lock();
			recycle_pool_item * pItem = _pop_block(false);
			if (lockable) m_container_mutex.unlock();
			return pItem;
		}

		virtual void clear(bool lockable = false)
		{
			if (lockable) m_container_mutex.lock();
			_clear();
			if (lockable) m_container_mutex.unlock();
		}

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

		std::vector<recycle_pool_item *> &get_container() { return m_container; }
	protected:
		std::vector<recycle_pool_item *> m_container;
		boost::mutex m_container_mutex;
		uint32_t m_container_size_bytes;		//�����ֽڳ�����Ϣ
	};


	///////////////////////////////////////////////////
	// ���ߺ���
	// ת���ڴ�� ����ʵ��ת���ڴ浥Ԫ����
	int recycle_pool_mov_items(recycle_pool * src, recycle_pool * dst, int nums);
	int recycle_pool_mov_all_items(recycle_pool * src, recycle_pool * dst);
	// �����ڴ��
	template<typename elemT>
	recycle_pool_item * recycle_pool_build_item(recycle_pool * pool, bool bTrace = false)
	{
		if (!pool) return 0;
		elemT *pItem = new elemT();
		//���ͼ��
		recycle_pool_item *item = dynamic_cast<recycle_pool_item *>(pItem);
		pool->add_alloc_fast_item(item, bTrace);

		return item;
	}

}

#endif

