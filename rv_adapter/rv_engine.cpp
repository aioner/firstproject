///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_engine.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月19日
// 内容描述：radvsion ARTP协议栈适配器 -- 多线程调度引擎
//
//
// 修订日志
// [2012-03-19]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////
//Radvision ARTP toolsuit
#include <rvconfig.h>
#include <rvtypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <rvansi.h>
#include <rvthread.h>
#include <rtp.h>
#include <rtcp.h>
#include <rvrtpseli.h>
#include <payload.h>
#include <rvrtpnatfw.h>
#include <rvrtpstunfw.h>
#include <RtpDemux.h>
#include <RtcpTypes.h>

#include <tghelper/async_event.h>

#include "rv_engine.h"
#include "rv_adapter_convert.h"
#include "rv_adapter.h"
#include "mem_check_on.h"

#ifndef _WIN32
#define sprintf_s snprintf
#endif

extern rtpDemuxEventHandler g_rtpDemuxEventHandler;
extern rtpDemuxEventHandler g_rtpDemuxCasterEventHandler;
namespace rv
{
	namespace inner
	{

	}

	//内部回调函数接口
	void rvcore_rtpEventHandler_cb(RvRtpSession hRTP, void * context)
	{
		//接续回调过程
		rv_handler hrv = (rv_handler)context;

		//add by songlei 20150507 modify coredump
		if (NULL == hrv)
		{
			return;
		}

		if (hrv->onRtpRcvEvent)
			(*((RtpReceiveEventHandler_CB)(hrv->onRtpRcvEvent)))(hrv, hrv->context);
	}

	void rvcore_rtpDemuxEventHandler_CB(
		IN  RvRtpDemux   hDemux,
		IN  void *       context)
	{
		
        if (context == NULL && g_rtpDemuxCasterEventHandler)
        {
            g_rtpDemuxCasterEventHandler((void*)hDemux, context);
        }
        else if (g_rtpDemuxEventHandler)
        {
            g_rtpDemuxEventHandler((void*)hDemux, context);
        }
	}

	void RVCALLCONV rvcore_rtcpSendEventHandler_cb(
		RvRtcpSession hRTCP,
		void * context,
		RvUint32 ssrc,
		RvUint8  *buffer,
		RvUint32 size)
	{
		//接续回调过程
		rv_handler hrv = (rv_handler)context;
		
		//add by songlei 20150507 modify coredump
		if (NULL == hrv)
		{
			return;
		}

		if (hrv->onRtcpSndSREvent)
		{
			(*((RtcpSendHandler_CB)(hrv->onRtcpSndSREvent)))(hrv, hrv->context, ssrc, buffer, size);
		}
			
	}

	void RVCALLCONV rvcore_rtcpReceiveEventHandler_cb(
		IN RvRtcpSession    hRTCP,
		IN void *           context,
		IN RvUint32         ssrc,
		IN RvUint8          *rtcpPack,
		IN RvUint32         packLen,
		IN RvUint8          *ip,
		IN RvUint16         port,
		IN RvBool           mutiplex,
		IN RvUint32         multid)
	{
		//接续回调过程
		rv_handler hrv = (rv_handler)context;

		//add by songlei 20150507 modify coredump
		if (NULL == hrv)
		{
			return;
		}

		if (hrv->onRtcpRcvSRRREvent)
			(*((RtcpReceivedHandler_CB)(hrv->onRtcpRcvSRRREvent)))(hrv, hrv->context, ssrc, rtcpPack, packLen,ip,port,mutiplex,multid);
	}

	RvBool RVCALLCONV rvcore_rtcpAppEventHandler_CB(
		IN  RvRtcpSession  hRTCP,
		IN  void		   *context,
		IN  RvUint8        subtype,
		IN  RvUint32       ssrc,
		IN  RvUint8*       name,
		IN  RvUint8*       userData,
		IN  RvUint32       userDataLen) //userData in bites!, not in 4 octet words
	{
		//接续回调过程
		rv_handler hrv = (rv_handler)context;

		//add by songlei 20150507 modify coredump
		if (NULL == hrv)
		{
			return RV_FALSE;
		}

		if (hrv->onRtcpAppEvent)
			(*((RtcpAppEventHandler_CB)(hrv->onRtcpAppEvent)))(hrv, hrv->context, subtype, ssrc, name, userData, userDataLen);

		return RV_TRUE;
	}

	void RVCALLCONV rvcore_rtcpRawBufferReceived_CB(IN  RvRtcpSession    hRTCP,
													IN  RvUint8*         buffer,
													IN  RvSize_t         buffLen,
													IN  RvNetAddress*    remoteAddress, 
													IN  void*			context,
													OUT RvBool*          pbDiscardBuffer)
	{  
		//接续回调过程
		rv_handler hrv = (rv_handler)context;
		if (NULL == hrv)
		{
			return;
		}

		if (hrv->onRtcpRawEvent)
			(*((RtcpRawBufferReceived_CB)(hrv->onRtcpRawEvent)))(hrv, hrv->context, buffer, buffLen, (rv_net_address*)remoteAddress, (rv_bool*)pbDiscardBuffer);
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// rv_context_event
	void open_session_event::recycle_alloc_event()
	{
		tghelper::async_event::recycle_alloc_event();
		memset(&params, 0, sizeof(params));
		hrv = 0;
		bRetState = false;
		nDemux = 0;
	}

	void open_session_event::setparams(const rv_session_descriptor &init_params)
	{
		memcpy(&params, &init_params, sizeof(rv_session_descriptor));
	}

	void open_session_event::setparams(const rv_session_descriptor *init_params)
	{
		memcpy(&params, init_params, sizeof(rv_session_descriptor));
	}

	void open_session_event::do_event()
	{
		//打开指定的rtp/rtcp端口并设置回调函数
		do
		{
			if (!hrv) break;

		#if (RV_CORE_ENABLE)
			RvRtpSession	rtpH;
			RvRtcpSession	rtcpH;

			if (nDemux == 3)
			{
				demux = (void*)RvRtpDemuxConstruct(nSessions);
				if (demux)
				{
					RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
					RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);

					if (rtpH)
					{
						RtpDemux *d = (RtpDemux*) demux;
						d->rtpTransport = ((RvRtpSessionInfo *)rtpH)->transport;
						d->rtpSessionsCounter++;
						((RvRtpSessionInfo *)rtpH)->demux = (RvRtpDemux)demux;
					}
					if (rtcpH)
					{
						RtpDemux *d = (RtpDemux*) demux;
						d->rtcpTransport = ((rtcpSession *)rtcpH)->transport;
						d->rtcpSessionsCounter++;
						((rtcpSession *)rtcpH)->demux = (RvRtpDemux)demux;
					}

					RvRtpDemuxSetEventHandler((RvRtpDemux)demux, rvcore_rtpDemuxEventHandler_CB, hrv->user_context);
				}

				bRetState = true;
				break;
			}


			//CNAME规划为rtp_[open_session实例指针值]@[ip地址dword值]
			rv_net_ipv4 address;
			rv_adapter::convert_rvnet_to_ipv4(&address, &(params.local_address));
			char sname[128] = "";
			sprintf_s(sname, sizeof(sname), "rtp_%d@%d", this, address.ip);

			RvNetAddress  rtpReceivingAddress;
			rv_net_address_to_RvNetAddress(&rtpReceivingAddress, &(params.local_address));

			if (nDemux == 1)
			{
				rtpH  = RvRtpOpenEx(&rtpReceivingAddress, 0, 0, sname);
			}
			else if (nDemux == 2)
			{
				rtpH = RvRtpDemuxOpenSession((RvRtpDemux)demux, &rtpReceivingAddress, 0, 0, sname, NULL, (RvUint32*)multiplexID);
			}
			else
			{
				rtpH  = RvRtpOpenEx(&rtpReceivingAddress, 0, 0, sname);
			}

			if (rtpH == NULL) break;
			//强制使用外部SN计数，避免多线程环境导致的包顺序紊乱。
			RvRtpUseSequenceNumber(rtpH);

			rtcpH = RvRtpGetRTCPSession(rtpH);

			hrv->hrtcp = rtcpH;
			hrv->hrtp = rtpH;
			hrv->context = params.context;
            hrv->user_context = params.user_context;

			if (nDemux == 2)
			{
				RtpDemux *d = (RtpDemux*)demux;
				RvRtpSessionInfo *r1 = ((RvRtpSessionInfo *)rtpH);
				RvRtpSessionInfo *r2 = ((RvRtpSessionInfo *)rtcpH);
				if (d && r1)
				{
					r1->transport = d->rtpTransport;
				}
			}

			if (params.onRtpRcvEvent)
			{
				hrv->onRtpRcvEvent = (rv_context)(params.onRtpRcvEvent);
				if (nDemux==0 || nDemux==1)
				{
					RvRtpSetEventHandler(rtpH, (RvRtpEventHandler_CB)(rvcore_rtpEventHandler_cb), hrv);
				}
			}

			if (params.onRtcpSndSREvent && rtcpH)
			{
				hrv->onRtcpSndSREvent = (rv_context)(params.onRtcpSndSREvent);
				RvRtcpSetRTCPSendHandlerEx(rtcpH, rvcore_rtcpSendEventHandler_cb);
			}

			if (params.onRtcpRcvSRRREvent && rtcpH)
			{
				hrv->onRtcpRcvSRRREvent = (rv_context)(params.onRtcpRcvSRRREvent);
				RvRtcpSetRTCPRecvEventHandlerEx(rtcpH, rvcore_rtcpReceiveEventHandler_cb, hrv);
			}
			if (params.manual_rtcp && rtcpH)
			{
				RvBool bManual = RV_TRUE;
				RvRtcpSetManual(rtcpH, bManual);
			}

			if (params.multicast_rtp_address_opt)
			{
				RvNetAddress  rtpAddress;
				rv_net_address_to_RvNetAddress(&rtpAddress, &params.multicast_rtp_address);
				RvRtpSetGroupAddress(rtpH, &rtpAddress);
			}

			if (params.multicast_rtcp_address_opt && rtcpH)
			{
				RvNetAddress  rtcpAddress;
				rv_net_address_to_RvNetAddress(&rtcpAddress, &params.multicast_rtcp_address);
				RvRtcpSetGroupAddress(rtcpH, &rtcpAddress);
			}

#ifndef CLOSE_SESSION
  			if (params.multicast_rtp_ttl > 0)
  			{
  				RvUint8 ttl = params.multicast_rtp_ttl;
 				RvRtpSetMulticastTTL(rtpH, ttl);
  			}

 			if (params.multicast_rtcp_ttl > 0 && rtcpH)
 			{
				RvUint8 ttl = params.multicast_rtcp_ttl;
				RvRtcpSetMulticastTTL(rtcpH, ttl); 			
			}
			#endif

			//zzx
			if (rtcpH)
			{
				RvRtcpSetAppEventHandler(rtcpH, rvcore_rtcpAppEventHandler_CB, hrv);
				RvRtcpSetRawBufferReceivedEventHandler(rtcpH, rvcore_rtcpRawBufferReceived_CB, hrv);
			}

		#endif

			bRetState = true;
		} while (false);

		tghelper::async_event::do_event();
	}

	void close_session_event::recycle_alloc_event()
	{
		tghelper::async_event::recycle_alloc_event();
		hrv = 0;
		bRetState = false;
		nDemux = 0;
	}

	void close_session_event::do_event()
	{
		//关闭指定的rtp/rtcp端口
		do
		{
		#if (RV_CORE_ENABLE)
			if (!hrv) break;
			RvRtpSession	rtpH = (RvRtpSession)(hrv->hrtp);
			RvRtcpSession	rtcpH= (RvRtcpSession)(hrv->hrtcp);

            if (nDemux == 1)
            {
                if (RV_OK != RvRtpClose(rtpH)) break;
            }
			else if (nDemux == 2)
			{
				if (RV_OK != RvRtpDemuxCloseSession(rtpH)) break;
			}
			else
			{
				//关联创建的RTCP端口不能显式关闭
				//if (RV_OK != RvRtcpClose(rtcpH)) break;
				if (RV_OK != RvRtpClose(rtpH)) break;
			}
		#endif
			bRetState = true;
		} while (false);

		tghelper::async_event::do_event();
	}

	void write_session_event::recycle_alloc_event()
	{
		tghelper::async_event::recycle_alloc_event();
		hrv = 0;
		buf_len = 0;
		bRetState = false;
	}

	void write_session_event::do_event()
	{
		//异步发送session

		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!hrv) break;
			if (!buf) break;
			if (!buf_len) break;
		#endif

		#if (RV_CORE_ENABLE)
			if (!hrv) break;
			RvRtpSession	rtpH = (RvRtpSession)(hrv->hrtp);
			RvRtcpSession	rtcpH= (RvRtcpSession)(hrv->hrtcp);
			RvRtpParam _p;
			rv_rtp_param_to_RvRtpParam(&_p, &p);
			if (RV_OK != RvRtpWrite(rtpH, buf, buf_len, &_p)) break;
		#endif
			bRetState = true;
		} while (false);

		tghelper::async_event::do_event();
	}

	void write_session_ex_event::recycle_release_event()
	{
		if (buf) buf->release();
		buf = 0;
		tghelper::async_event::recycle_alloc_event();
	}

	void write_session_ex_event::do_event()
	{
		//异步发送session
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!hrv) break;
			if (!buf) break;
		#endif

		#if (RV_CORE_ENABLE)
			if (!hrv) break;
			RvRtpSession	rtpH = (RvRtpSession)(hrv->hrtp);
			RvRtcpSession	rtcpH= (RvRtcpSession)(hrv->hrtcp);
			RvRtpParam _p;
			rv_rtp_param_to_RvRtpParam(&_p, &p);
			if (RV_OK != RvRtpWrite(rtpH, buf->get_raw(), buf->payload_totalsize(), &_p)) break;
		#endif
			bRetState = true;
		} while (false);
		if (buf)
		{
			buf->release();
			buf = 0;
		}
		tghelper::async_event::do_event();
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// rv_engine_contexts

	//对应key描述的上下文增加引用计数
	bool rv_engine_contexts::add_ref(uint32_t key)
	{
		rv_engine_context * context = get_context(key);
		if (context)
		{
			context->m_session_nums++;
			return true;
		}
		return false;
	}
	//对应key描述的上下文减少引用计数
	bool rv_engine_contexts::dec_ref(uint32_t key)
	{
		rv_engine_context * context = get_context(key);
		if (context)
		{
			if (context->m_session_nums > 0) context->m_session_nums--;
			return true;
		}
		return false;
	}
	//选择一个上下文key,根据负载均衡化原则选择
	bool rv_engine_contexts::sel_context(uint32_t &key)
	{
		if (m_contexts.empty()) return false;
		key = 0;
		uint32_t session_nums = 0xFFFFFFFF;
		for (uint32_t i = 0; i < m_contexts.size(); i++)
		{
			if (m_contexts[i]->compare_light(session_nums))
			{
				key = i;
				session_nums = m_contexts[i]->m_session_nums;
			}
		}
		return true;
	}
	tghelper::async_event * rv_engine_contexts::forceAllocEvent(uint32_t event_id)
	{
		tghelper::async_event * event = 0;
		tghelper::recycle_pool * pool = m_msgPools.get_pools(event_id);
		if (!pool) return 0;
		event = static_cast<tghelper::async_event *>(pool->alloc_item());
		if (!event)
		{
			switch(event_id)
			{
			case RV_QUIT_CONTEXT_EVENT:
				event = static_cast<tghelper::async_event *>(
					tghelper::recycle_pool_build_item<quit_context_event>(pool, false));
				break;
			case RV_OPEN_SESSION_EVENT:
				event = static_cast<tghelper::async_event *>(
					tghelper::recycle_pool_build_item<open_session_event>(pool, false));
				break;
			case RV_CLOSE_SESSION_EVENT:
				event = static_cast<tghelper::async_event *>(
					tghelper::recycle_pool_build_item<close_session_event>(pool, false));
				break;
			case RV_WRITE_SESSION_EVENT:
				event = static_cast<tghelper::async_event *>(
					tghelper::recycle_pool_build_item<write_session_event>(pool, false));
				break;
			case RV_WRITE_SESSION_EX_EVENT:
				event = static_cast<tghelper::async_event *>(
					tghelper::recycle_pool_build_item<write_session_ex_event>(pool, false));
				break;
			}
		}
		return event;
	}

	void rv_engine_contexts::post_all_quit_msg()
	{
		tghelper::async_event * event = forceAllocEvent(RV_QUIT_CONTEXT_EVENT);
		if (!event) return;
		event->assign();
		std::vector<rv_engine_context *>::iterator it;
		for (it = m_contexts.begin(); it != m_contexts.end(); it++)
		{
			(*it)->m_msgQueue.push(event);
		}
		event->release();
	}

	bool rv_engine_contexts::post_asyn_msg(uint32_t key, tghelper::async_event *event)
	{
		if (!event) return false;
		rv_engine_context * context = get_context(key);
		if (!context) return false;
		context->m_msgQueue.push(event);
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// rv_engine
	void rv_engine::rv_engine_func(rv_engine_context *context)
	{
		//引擎主循环
		if (!context) return;

	#if (RV_CORE_ENABLE)
		RvThread th;
		RvThreadConstructFromUserThread(0, &th);
		/* Initialization of SELI interface */
		RvRtpSeliInit();
	#endif

	#if defined( WIN32 ) && defined( _MSC_VER )
		//增加线程优先级
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	#endif

		context->transit(RVENG_READY);
		bool bQuit = false;
		while(!bQuit)
		{

			tghelper::async_event *event = static_cast<tghelper::async_event *>(context->m_msgQueue.pop());
			if(event)
			{
				event->do_event();
				if (RV_QUIT_CONTEXT_EVENT == event->get_event_id()) bQuit = true;
				event->release();
			#if (RV_CORE_ENABLE)
				RvRtpSeliSelectUntil(0);
			#else
				boost::this_thread::sleep(boost::posix_time::millisec(1));
			#endif
			}
			else
			{
			#if (RV_CORE_ENABLE)
#ifdef _ANDROID
                RvRtpSeliSelectUntil(10);
#else
                RvRtpSeliSelectUntil(1);
#endif
			#else
#ifdef _ANDROID
                boost::this_thread::sleep(boost::posix_time::millisec(10));
#else
				boost::this_thread::sleep(boost::posix_time::millisec(1));
#endif
			#endif
			}
		}

		context->transit(RVENG_QUIT);


	#if (RV_CORE_ENABLE)
		/* Ends SELI interface*/
		RvRtpSeliEnd();

		RvThreadDestruct(&th);
	#endif

	}

	void rv_engine::build(uint32_t thread_nums)
	{
		m_engine_nums = thread_nums;
		m_contexts.build(thread_nums);
		for (uint32_t i = 0; i < thread_nums; i++)
		{
			m_engines.create_thread(
				boost::bind(&rv_engine::rv_engine_func, m_contexts.get_context(i)));
		}
	}

	void rv_engine::close()
	{
		if (0 == m_engine_nums) return;
		m_contexts.post_all_quit_msg();

		//waitting for all thread stop!!
		m_engines.join_all();

		m_engine_nums = 0;
	}


} /* end of namespace rv */
