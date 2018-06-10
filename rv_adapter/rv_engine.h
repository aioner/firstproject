///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_engine.h
// 创 建 者：汤戈
// 创建时间：2012年03月19日
// 内容描述：radvsion ARTP协议栈适配器 -- 多线程调度引擎
//
//
// 修订日志
// [2012-03-22]		 增加异步数据输出功能，数据最终中rv_engine线程中执行
//		1、提供write_session_event方式，采用内容拷贝复制方式异步输出
//		2、提供write_session_ex_event方式，采用智能指针的0拷贝模式异步输出，推荐采用该方式
//
// [2012-03-19]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef RADVISION_ENGINE_
#define RADVISION_ENGINE_

#include<stdint.h>
#include <rvconfig.h>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <vector>
#include <tghelper/recycle_pool.h>
#include <tghelper/async_event.h>
#include <tghelper/recycle_pools.h>
#include <tghelper/byte_pool.h>
#include "rv_def.h"
#include "rv_adapter_config.h"
#include <rvrtpnatfw.h>

namespace rv
{
	void rvcore_rtpEventHandler_cb(RvRtpSession hRTP, void * context);

	void rvcore_rtpDemuxEventHandler_CB(
								IN  RvRtpDemux   hDemux,
								IN  void *       context);

	void RVCALLCONV rvcore_rtcpSendEventHandler_cb(
		RvRtcpSession hRTCP,
		void * context,
		RvUint32 ssrc,
		RvUint8  *buffer,
		RvUint32 size);

	void RVCALLCONV rvcore_rtcpReceiveEventHandler_cb(
		IN RvRtcpSession    hRTCP,
		IN void *           context,
		IN RvUint32         ssrc,
		IN RvUint8          *rtcpPack,
		IN RvUint32         packLen,
		IN RvUint8          *ip,
		IN RvUint16         port,
		IN RvBool           mutiplex,
		IN RvUint32         multid);

	RvBool RVCALLCONV rvcore_rtcpAppEventHandler_CB(
		IN  RvRtcpSession  hRTCP,
		IN  void		   *context,
		IN  RvUint8        subtype,
		IN  RvUint32       ssrc,
		IN  RvUint8*       name,
		IN  RvUint8*       userData,
		IN  RvUint32       userDataLen);

	void RVCALLCONV rvcore_rtcpRawBufferReceived_CB(
		IN  RvRtcpSession    hRTCP,
		IN  RvUint8*         buffer,
		IN  RvSize_t         buffLen,
		IN  RvNetAddress*    remoteAddress, 
		IN  void*			context,
		OUT RvBool*          pbDiscardBuffer);

	typedef enum ERV_CONTEXT_EVENT_
	{
		RV_QUIT_CONTEXT_EVENT,
		RV_OPEN_SESSION_EVENT,
		RV_CLOSE_SESSION_EVENT,
		RV_WRITE_SESSION_EVENT,
		RV_WRITE_SESSION_EX_EVENT,		//利用外部的byte_pool池方式，减少拷贝次数
	} ERV_CONTEXT_EVENT;
	class quit_context_event : public tghelper::async_event
	{
	public:
		quit_context_event() : tghelper::async_event(RV_QUIT_CONTEXT_EVENT)
		{		}
	};

	class open_session_event : public tghelper::async_event
	{
	public:
		open_session_event() : tghelper::async_event(RV_OPEN_SESSION_EVENT)
		{	nDemux = 0;	}
		virtual void do_event();

		//输入输出区域复位
		virtual void recycle_alloc_event();

		void setparams(const rv_session_descriptor &init_params);
		void setparams(const rv_session_descriptor *init_params);

		//输入参数区域
	public:
		RV_IN rv_session_descriptor params;

		int nDemux;//0:session 1:demux session 2:demux_subsession 3:demux
		void *demux;
		uint32_t *multiplexID;
		uint32_t nSessions;

		//输出结果区域
	public:
		RV_OUT rv_handler hrv;
		bool bRetState;
	};

	class close_session_event : public tghelper::async_event
	{
	public:
		close_session_event() : tghelper::async_event(RV_CLOSE_SESSION_EVENT)
		{	nDemux = 0;	}
		virtual void do_event();

		//输入输出区域复位
		virtual void recycle_alloc_event();

		//输入参数区域
	public:
		RV_IN rv_handler hrv;

		int nDemux;

		//输出结果区域
	public:
		bool bRetState;
	};

	class write_session_event : public tghelper::async_event
	{
	public:
		write_session_event() : tghelper::async_event(RV_WRITE_SESSION_EVENT)
		{
			buf = new uint8_t[RV_ADAPTER_ASYNC_WRITE_BUFFER_SIZE];
		}
		virtual ~write_session_event()
		{
			if (buf) delete [] buf;
			buf = 0;
		}
		virtual void do_event();

		//输入输出参数区域复位
		virtual void recycle_alloc_event();

		//输入参数区域
	public:
		RV_IN rv_handler hrv;
		RV_IN uint8_t *buf;
		RV_IN uint32_t buf_len;
		RV_INOUT rv_rtp_param p;

		//输出结果区域
	public:
		bool bRetState;
	};

	class write_session_ex_event : public tghelper::async_event
	{
		/*
			增强型异步数据输出模式，该方式下，
			用户采用tghelper库中的byte_pool管理内存片，
			rv_engine默认以此理解该过程，并主动释放内存片至用户内存池
		*/
	public:
		write_session_ex_event() : tghelper::async_event(RV_WRITE_SESSION_EX_EVENT)
		{
			buf = 0;
		}

		virtual void do_event();

		//输入输出参数区域复位
		virtual void recycle_release_event();

		//输入参数区域
	public:
		RV_IN rv_handler hrv;
		RV_IN tghelper::byte_block *buf;
		RV_INOUT rv_rtp_param p;

		//输出结果区域
	public:
		bool bRetState;
	};

	typedef enum rv_eng_context_state_
	{
		RVENG_IDLE = 0,	/* 尚未完成初始化 */
		RVENG_READY,	/* 完成初始化等待响应外部事件和激励rv selectEngine */
		RVENG_QUIT,	/* 退出状态 */
	} rv_eng_context_state;

	class rv_engine_context
	{
	public:
		rv_engine_context(uint32_t key) :
			m_state(RVENG_IDLE),
			m_msgQueue(0, false),		//配置成无限增长模式，如果出现处理阻塞会从此看到内存泄露
			m_session_nums(0),
			m_key(key)
		{
		}
		~rv_engine_context()
		{

		}

		void init_fifo()
		{
		}

		void end_fifo()
		{
		}

		rv_eng_context_state transit(rv_eng_context_state newState)
		{
			m_state = newState;
			return m_state;
		}

		//返回this->session_nums和context->session_nums的大小关系，
		//如果小则返回true，否则为false
		inline bool compare_light(rv_engine_context &context)
		{
			return (context.m_session_nums > m_session_nums);
		}

		inline bool compare_light(rv_engine_context *context)
		{
			return (context->m_session_nums > m_session_nums);
		}
		inline bool compare_light(uint32_t session_nums)
		{
			return (session_nums > m_session_nums);
		}

	public:
		rv_eng_context_state m_state;
		tghelper::recycle_queue m_msgQueue;
		uint32_t m_session_nums;
		uint32_t m_key;
	};

	class rv_engine_contexts
	{
	public:
		rv_engine_contexts()
		{
			build_msgPools();
		}
		rv_engine_contexts(uint32_t context_nums)
		{
			build_msgPools();
			build(context_nums);
		}

		~rv_engine_contexts()
		{
			clear();
		}

		void clear()
		{
			if (m_contexts.empty()) return;

			for (uint32_t i = 0; i < m_contexts.size(); i++)
			{
				if (m_contexts[i]) delete m_contexts[i];
			}
			m_contexts.clear();
		}

		void build(uint32_t context_nums)
		{
			clear();
			m_contexts.resize(context_nums);
			for (uint32_t i = 0; i < context_nums; i++)
			{
				m_contexts[i] = new rv_engine_context(i);
			}
		}

		void build_msgPools()
		{
			m_msgPools.add_pool(RV_QUIT_CONTEXT_EVENT);
			m_msgPools.add_pool(RV_OPEN_SESSION_EVENT);
			m_msgPools.add_pool(RV_CLOSE_SESSION_EVENT);
			m_msgPools.add_pool(RV_WRITE_SESSION_EVENT);
			m_msgPools.add_pool(RV_WRITE_SESSION_EX_EVENT);
		}
	public:
		//对应key描述的上下文增加引用计数
		bool add_ref(uint32_t key);
		//对应key描述的上下文减少引用计数
		bool dec_ref(uint32_t key);
		//选择一个上下文key,根据负载均衡化原则选择
		bool sel_context(uint32_t &key);
		//提取对应key的上下文
		inline rv_engine_context * get_context(uint32_t key)
		{
			if (m_contexts.empty() || (m_contexts.size() <= key)) return 0;
			return m_contexts[key];
		}

		void post_all_quit_msg();
		tghelper::async_event *forceAllocEvent(uint32_t event_id);
		bool post_asyn_msg(uint32_t key, tghelper::async_event *event);

	private:
		tghelper::recycle_pools m_msgPools;
		std::vector<rv_engine_context *> m_contexts;
	};

	class rv_engine : private boost::noncopyable
	{
	public:
		rv_engine() :
		  m_engine_nums(0),
		  m_contexts()
		{

		}
		rv_engine(uint32_t thread_nums)
		{
			build(thread_nums);
		}
		~rv_engine()
		{
			close();
		}

		void build(uint32_t thread_nums);
		void close();

		static void rv_engine_func(rv_engine_context *context);

	private:
		uint32_t m_engine_nums;
		boost::thread_group m_engines;

	public:
		rv_engine_contexts m_contexts;
	};

}


#endif

