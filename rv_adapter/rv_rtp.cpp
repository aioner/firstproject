///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_rtp.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月16日
// 内容描述：radvsion ARTP协议栈适配器 -- rtp函数
///////////////////////////////////////////////////////////////////////////////////////////

//Radvision ARTP toolsuit
#include <rvconfig.h>

#include <rvtypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <rvansi.h>
#include <rtp.h>
#include <rtcp.h>
#include <rvrtpseli.h>
#include <payload.h>

#include "rv_adapter.h"
#include "rv_adapter_convert.h"

#include "mem_check_on.h"

namespace rv
{
	bool rv_adapter::write_rtp(
		RV_IN rv_handler hrv,
		RV_IN void * buf,
		RV_IN uint32_t buf_len,
		RV_INOUT rv_rtp_param * p)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !buf || !p) break;
			if (!buf_len) break;
		#endif

		#if (RV_CORE_ENABLE)
			//输出
			RvRtpParam _p;
			rv_rtp_param_to_RvRtpParam(&_p, p);
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			bRet = (RV_OK == RvRtpWrite(rtpH, buf, buf_len, &_p));
		#else
			bRet = true;
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::write_rtp_s(RV_IN rv_handler hrv,
		RV_IN void * buf,
		RV_IN uint32_t buf_len,
		RV_INOUT rv_rtp_param * p,
		RV_IN bool async_mode)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !buf || !p) break;
			if (!buf_len) break;
		#endif
			if (RV_ADAPTER_ASYNC_WRITE_BUFFER_SIZE < buf_len) break;
			rv::write_session_event *event_write =
				static_cast<rv::write_session_event *>(m_core.m_contexts.forceAllocEvent(rv::RV_WRITE_SESSION_EVENT));
			if(!event_write) break;
			event_write->assign();
			event_write->hrv = hrv;
			memcpy(event_write->buf, buf, buf_len);
			event_write->buf_len = buf_len;
			memcpy(&event_write->p, p, sizeof(rv_rtp_param));
			if (!async_mode) event_write->set_wait_state();
			if(!(m_core.m_contexts.post_asyn_msg(hrv->hthread, event_write)))
			{
				event_write->release();
				break;
			}
			if (async_mode)		//异步方式不等返回状态
				bRet = true;
			else
			{
				event_write->wait_event();
				if (event_write->bRetState)
				{
					bRet = true;
				}
			}
			event_write->release();
		} while (false);
		return bRet;
	}

	bool rv_adapter::write_rtp_s2(RV_IN rv_handler hrv,
		RV_IN void * buf,			//tghelper::byte_block
		RV_INOUT rv_rtp_param * p,
		RV_IN bool async_mode)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !buf || !p) break;
		#endif

		rv::write_session_ex_event *event_write =
			static_cast<rv::write_session_ex_event *>(m_core.m_contexts.forceAllocEvent(rv::RV_WRITE_SESSION_EX_EVENT));
		if(!event_write) break;
		event_write->assign();
		event_write->hrv = hrv;
		event_write->buf = (tghelper::byte_block *)buf;
		event_write->buf->assign();
		memcpy(&event_write->p, p, sizeof(rv_rtp_param));
		if (!async_mode) event_write->set_wait_state();
		if(!(m_core.m_contexts.post_asyn_msg(hrv->hthread, event_write)))
		{
			event_write->release();
			break;
		}
		if (async_mode)		//异步方式不等返回状态
			bRet = true;
		else
		{
			event_write->wait_event();
			if (event_write->bRetState)
			{
				bRet = true;
			}
		}
		event_write->release();

		} while (false);
		return bRet;
	}

	bool rv_adapter::read_rtp(
		RV_IN rv_handler hrv,
		RV_IN void * buf,
		RV_IN uint32_t buf_len,
		RV_INOUT rv_rtp_param * p,
		RV_INOUT rv_net_address *addr)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !buf || !p) break;
			if (!buf_len) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtpParam _p;
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
            //bRet = (RV_OK == RvRtpRead(rtpH, buf, buf_len, &_p));
            RvNetAddress internel_addr;
            bRet = (RV_OK==RvRtpReadWithRemoteAddress(rtpH, buf, buf_len, &_p, &internel_addr));//zhouzx 20141016 get packet address
            if (bRet)
            {
                RvNetAddress_to_rv_net_address(addr, &internel_addr);
                RvRtpParam_to_rv_rtp_param(p, &_p);
            }
#ifdef DUMP_RTP_FILE
			if((bRet)&&(p->len>0))
			{

				//dump_rtp2file(p->sSrc,(unsigned char *)buf+p->sByte,p->len-p->sByte);
				RtpH264DataProc((unsigned char *)buf+p->sByte,p);
			}

#endif
		#else
			bRet = true;
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::add_rtp_remote_address(
		RV_IN rv_handler hrv,
		RV_IN rv_net_address*  pRtpAddress)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !pRtpAddress) break;
		#endif
			bRet = true;
		#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			RvNetAddress  rtpAddress;
			rv_net_address_to_RvNetAddress(&rtpAddress, pRtpAddress);
			RvRtpAddRemoteAddress(rtpH, &rtpAddress);
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::del_rtp_remote_address(
		RV_IN rv_handler hrv,
		RV_IN rv_net_address*  pRtpAddress)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !pRtpAddress) break;
		#endif
			bRet = true;
		#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			RvNetAddress  rtpAddress;
			rv_net_address_to_RvNetAddress(&rtpAddress, pRtpAddress);
			RvRtpRemoveRemoteAddress(rtpH, &rtpAddress);
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::add_rtp_mult_remote_address(
		RV_IN rv_handler hrv,
		RV_IN rv_net_address*  pRtpAddress,
		RV_IN uint32_t multiplexID)
	{
		bool bRet = false;
		do
		{
#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !pRtpAddress) break;
#endif
			bRet = true;
#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			RvNetAddress  rtpAddress;
			rv_net_address_to_RvNetAddress(&rtpAddress, pRtpAddress);

			RvRtpMultiplexingAddRemoteAddress(rtpH, &rtpAddress, multiplexID);
#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::del_rtp_mult_remote_address(
		RV_IN rv_handler hrv,
		RV_IN rv_net_address*  pRtpAddress,
		RV_IN uint32_t multiplexID)
	{
		bool bRet = false;
		do
		{
#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !pRtpAddress) break;
#endif
			bRet = true;
#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			RvNetAddress  rtpAddress;
			rv_net_address_to_RvNetAddress(&rtpAddress, pRtpAddress);

			RvRtpMultiplexingRemoveRemoteAddress(rtpH, &rtpAddress, multiplexID);
#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::clear_rtp_remote_address(
		RV_IN rv_handler hrv)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv) break;
		#endif
			bRet = true;
		#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			RvRtpRemoveAllRemoteAddresses(rtpH);
		#endif
		} while (false);
		return bRet;
	}

	//rtp输出组播地址设置函数，
	bool rv_adapter::set_rtp_group_address(
		RV_IN rv_handler hrv,
		RV_IN rv_net_address*  pRtpAddress)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !pRtpAddress) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			RvNetAddress  rtpAddress;
			rv_net_address_to_RvNetAddress(&rtpAddress, pRtpAddress);
			bRet = (RV_OK == RvRtpSetGroupAddress(rtpH,&rtpAddress));
		#endif
		} while (false);
		return bRet;
	}

	//rtp输出组播TTL设置函数，
	bool rv_adapter::set_rtp_multicast_ttl(
		RV_IN rv_handler hrv,
		RV_IN uint8_t    ttl)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			bRet = (RV_OK == RvRtpSetMulticastTTL(rtpH,ttl));
		#endif
		} while (false);
		return bRet;
	}
} /* end of namespace rv*/
