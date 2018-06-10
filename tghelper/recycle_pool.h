///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：recycle_pool.h
// 创 建 者：汤戈
// 创建时间：2011年02月27日
// 内容描述：循环内存池
//
// 1、所有接口类均为线程安全;
//		a, 循环内存池基本配置单元
//		recycle_pool_item-->元基类
//		recycle_block-->内存块模板基类
//		recycle_pool-->容器类
//		b, 循环内存池配套数据操纵单元
//		recycle_queue-->循环FIFO队列
//		recycle_stack-->循环LIFO堆栈
//
// 2、采用boost库中的mutex对象实现线程同步
// 3、采用stl库中的list构建实际存储容器
// 4、节点引用计数的操作应当配对操作
//		recycle_pool_item->assign()
//		recycle_pool_item->release()
// 5、节点一般操作规则如下：
//		a, 通过调用recycle_pool.add_item()或mov_item()函数增加内存池空闲节点，充值过程
//		b, 通过调用recycle_pool.alloc_item()函数获得可接受内存池管理的节点，该节点引用计数为0，取款过程
//		c, 通过调用recycle_pool_item->assign()函数标记该内存的使用情况，开销过程
//		d, 通过调用recycle_pool_item->release()函数标记该内存的回收情况，如果为0，则自动回归内存池，收获过程
//		e, 通过调用recycle_pool.del_item()或mov_item()函数可让节点改变池管理关系。
//		f, 无池管理器的节点在release引用计数为0后会返回-1，提示用户该节点需要考虑额外释放资源。
// 6、节点提供两个事件点用于子类的特殊处理。
//		recycle_pool_item->recycle_alloc_event()		节点从池中被分配完成
//		recycle_pool_item->recycle_release_event()		节点被成功释放回池
//
// 修订日志
// [2011-02-27]		创建基础版本
// [2012-03-18]		增加内存片库单元定义recycly_marco_block
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
		//此处的枚举定义没有实际意义，仅提供一个recycle_pool_item::release()返回值的说明
		typedef enum _ERecycle_Pool_Item_Release
		{
			ISOLATE = -1,	//悬空状态，有用户自己释放对象
			IDLE = 0,		//空闲状态，对象返回池队列
			USED,			//使用状态，准确定义是大于0的整数表示当前引用计数值
		} EState;
	}

	class recycle_pool;
	class recycle_pool_item : private boost::noncopyable
	{
		friend class recycle_pool;
		friend class recycle_queue;
		friend class recycle_stack;
	protected:
		//阻止用户直接分配该基类
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

		//子类可重载事件
		virtual void recycle_alloc_event() {}
		virtual void recycle_release_event() {}

		virtual uint32_t size() { return 0; }			//内置存储单元数量
		virtual uint32_t size_bytes() { return 0; }		//内置存储单元字节长度

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
		//回收一个节点
		void release_item(recycle_pool_item *item);

	protected:
		//内部使用的非加锁版本
		recycle_pool_item * _alloc_item(bool bTrace = false);
		recycle_pool_item * _add_alloc_fast_item(recycle_pool_item *item, bool bTrace = false);
		void _add_item(recycle_pool_item *item);
		void _del_item(recycle_pool_item *item);

	public:
		//节点基本状态管理
		//加入一个节点，节点存储空间内部管理
		void add_item(recycle_pool_item *item);
		//移出一个节点，节点存储空间有外部管理
		void del_item(recycle_pool_item *item);
		//移入一个节点
		void mov_item(recycle_pool_item *item);
		//分配一个节点
		recycle_pool_item * alloc_item(bool bTrace = false);
		//快速加入并分配节点
		recycle_pool_item * add_alloc_fast_item(recycle_pool_item *item, bool bTrace = false);

		//清空内存池未分配节点，返回未回收节点数量
		uint32_t clear();

		//状态信息
		inline uint32_t freesize() { return m_unused.size(); }
		inline uint32_t tracesize(){ return m_used.size(); }

	protected:
		std::vector<recycle_pool_item *> m_unused;		//未使用列表
		std::vector<recycle_pool_item *> m_used;			//已使用列表
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
				//等长复制功能
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
		//FIFO规则
	public:
		recycle_queue(uint32_t max_size, bool overlapped) :
			m_max_size(max_size),		//0 == m_max_size长度不受限制
			m_overlapped(overlapped)	//覆盖写能力, 自动释放头部数据
		{

		}

		~recycle_queue()
		{
			clear();
		}

	public:
		//压入队列自动调用item->assign()
		bool push(recycle_pool_item *item);				//忙等锁方式
		int try_push(recycle_pool_item *item);			//尝试锁方式
		//弹出队列选择性调用item->release()
		recycle_pool_item * pop(bool bRelease = false);		//忙等锁方式
		recycle_pool_item * try_pop(/*OUT*/bool &lock_flag, bool bRelease = false);	//尝试锁方式
		void clear();

		inline int size() 
		{ 
			//boost::detail::spinlock::scoped_lock lock(dummy);
			boost::mutex::scoped_lock lock(m_mutex);
			return m_queue.size(); 
		}

	private:
		bool _push(recycle_pool_item *item);			//无锁控版本
		recycle_pool_item * _pop(bool bRelease = false);//无锁控版本

	private:
		std::vector<recycle_pool_item *> m_queue;
		boost::mutex m_mutex;
		uint32_t m_max_size;
		bool m_overlapped;
		//boost::detail::spinlock dummy;
	};

	class recycle_stack : private boost::noncopyable
	{
		//LIFO规则
	public:
		recycle_stack(uint32_t max_size) :
		  m_max_size(max_size)		//0 == m_max_size长度不受限制
		  {

		  }

		~recycle_stack()
		  {
			  clear();
		  }

	public:
		//压入堆栈自动调用item->assign()
		bool push(recycle_pool_item *item);				//忙等锁方式
		int try_push(recycle_pool_item *item);			//尝试锁方式
		//弹出堆栈选择性调用item->release()
		recycle_pool_item * pop(bool bRelease = false);
		recycle_pool_item * try_pop(/*OUT*/bool &lock_flag, bool bRelease = false);	//尝试锁方式
		void clear();

		inline int size() { return m_stack.size(); }	//函数线程不安全

	private:
		bool _push(recycle_pool_item *item);			//无锁控版本
		recycle_pool_item * _pop(bool bRelease = false);//无锁控版本

	private:
		std::vector<recycle_pool_item *> m_stack;
		boost::mutex m_mutex;
		uint32_t m_max_size;
	};

	class recycly_macro_block: public recycle_pool_item
	{
		/*
			 此类的内容接口可选择是否具备线程同步安全，
			 不使用内置线程同步对象时，外部模块操作该类数据内容时确保共享安全
			 1、利用push_block压入数据块后默认自动对该数据块引用计数加1处理
			 2、利用pop_block弹出数据块后默认自动对该数据块引用计数减1处理
			 上述两种方式需要配对使用，否则会导致内存泄露
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

		virtual uint32_t size() { return m_container.size(); }			//内置存储单元数量
		virtual uint32_t size_bytes()									//内置存储单元字节总长度
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

		std::vector<recycle_pool_item *> &get_container() { return m_container; }
	protected:
		std::vector<recycle_pool_item *> m_container;
		boost::mutex m_container_mutex;
		uint32_t m_container_size_bytes;		//容器字节长度信息
	};


	///////////////////////////////////////////////////
	// 工具函数
	// 转移内存块 返回实际转移内存单元数量
	int recycle_pool_mov_items(recycle_pool * src, recycle_pool * dst, int nums);
	int recycle_pool_mov_all_items(recycle_pool * src, recycle_pool * dst);
	// 构建内存块
	template<typename elemT>
	recycle_pool_item * recycle_pool_build_item(recycle_pool * pool, bool bTrace = false)
	{
		if (!pool) return 0;
		elemT *pItem = new elemT();
		//类型检查
		recycle_pool_item *item = dynamic_cast<recycle_pool_item *>(pItem);
		pool->add_alloc_fast_item(item, bTrace);

		return item;
	}

}

#endif

