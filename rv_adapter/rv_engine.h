///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����rv_engine.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��19��
// ����������radvsion ARTPЭ��ջ������ -- ���̵߳�������
//
//
// �޶���־
// [2012-03-22]		 �����첽����������ܣ�����������rv_engine�߳���ִ��
//		1���ṩwrite_session_event��ʽ���������ݿ������Ʒ�ʽ�첽���
//		2���ṩwrite_session_ex_event��ʽ����������ָ���0����ģʽ�첽������Ƽ����ø÷�ʽ
//
// [2012-03-19]		���������汾
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
		RV_WRITE_SESSION_EX_EVENT,		//�����ⲿ��byte_pool�ط�ʽ�����ٿ�������
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

		//�����������λ
		virtual void recycle_alloc_event();

		void setparams(const rv_session_descriptor &init_params);
		void setparams(const rv_session_descriptor *init_params);

		//�����������
	public:
		RV_IN rv_session_descriptor params;

		int nDemux;//0:session 1:demux session 2:demux_subsession 3:demux
		void *demux;
		uint32_t *multiplexID;
		uint32_t nSessions;

		//����������
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

		//�����������λ
		virtual void recycle_alloc_event();

		//�����������
	public:
		RV_IN rv_handler hrv;

		int nDemux;

		//����������
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

		//���������������λ
		virtual void recycle_alloc_event();

		//�����������
	public:
		RV_IN rv_handler hrv;
		RV_IN uint8_t *buf;
		RV_IN uint32_t buf_len;
		RV_INOUT rv_rtp_param p;

		//����������
	public:
		bool bRetState;
	};

	class write_session_ex_event : public tghelper::async_event
	{
		/*
			��ǿ���첽�������ģʽ���÷�ʽ�£�
			�û�����tghelper���е�byte_pool�����ڴ�Ƭ��
			rv_engineĬ���Դ����ù��̣��������ͷ��ڴ�Ƭ���û��ڴ��
		*/
	public:
		write_session_ex_event() : tghelper::async_event(RV_WRITE_SESSION_EX_EVENT)
		{
			buf = 0;
		}

		virtual void do_event();

		//���������������λ
		virtual void recycle_release_event();

		//�����������
	public:
		RV_IN rv_handler hrv;
		RV_IN tghelper::byte_block *buf;
		RV_INOUT rv_rtp_param p;

		//����������
	public:
		bool bRetState;
	};

	typedef enum rv_eng_context_state_
	{
		RVENG_IDLE = 0,	/* ��δ��ɳ�ʼ�� */
		RVENG_READY,	/* ��ɳ�ʼ���ȴ���Ӧ�ⲿ�¼��ͼ���rv selectEngine */
		RVENG_QUIT,	/* �˳�״̬ */
	} rv_eng_context_state;

	class rv_engine_context
	{
	public:
		rv_engine_context(uint32_t key) :
			m_state(RVENG_IDLE),
			m_msgQueue(0, false),		//���ó���������ģʽ��������ִ���������Ӵ˿����ڴ�й¶
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

		//����this->session_nums��context->session_nums�Ĵ�С��ϵ��
		//���С�򷵻�true������Ϊfalse
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
		//��Ӧkey�������������������ü���
		bool add_ref(uint32_t key);
		//��Ӧkey�����������ļ������ü���
		bool dec_ref(uint32_t key);
		//ѡ��һ��������key,���ݸ��ؾ��⻯ԭ��ѡ��
		bool sel_context(uint32_t &key);
		//��ȡ��Ӧkey��������
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

