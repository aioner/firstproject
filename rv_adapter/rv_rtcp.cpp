///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_rtcp.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月16日
// 内容描述：radvsion ARTP协议栈适配器 -- rtcp函数
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
#include <rvrtpstunfw.h>

#include "rv_adapter.h"
#include "rv_adapter_convert.h"

#include "mem_check_on.h"

namespace rv
{
	bool rv_adapter::set_rtcp_bandwidth(
		RV_IN rv_handler hrv,				/* The handle of the RTCP session */
		RV_IN uint32_t bandwidth)			/* The bandwidth for RTCP packets */
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
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			RvRtcpSetBandwidth(rtcpH, bandwidth);
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::set_manual_rtcp(
		RV_IN rv_handler hrv,
		RV_IN bool manual_rtcp)
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
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			RvBool bManual = (manual_rtcp) ? RV_TRUE : RV_FALSE;
			bRet = (RV_OK == RvRtcpSetManual(rtcpH, bManual));
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::manual_send_rtcp_sr(
		RV_IN rv_handler hrv,				/* The handle of the RTCP session */
		RV_IN uint32_t pack_size,			/* The number of bytes in the packet that was sent */
		RV_IN uint32_t pack_ts)			/* The RTP timestamp from the packet that was sent */
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
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			bRet = (RV_OK == RvRtcpRTPPacketSent(rtcpH, pack_size, pack_ts));
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::manual_send_rtcp_rr(
		RV_IN rv_handler hrv,				/* The handle of the RTCP session */
		RV_IN uint32_t ssrc,				/* The synchronization source value of the participant that sent the packet */
		RV_IN uint32_t localtimestamp,		/* The local RTP timestamp when the received packet arrived */
		RV_IN uint32_t mytimestamp,			/* The RTP timestamp from the packet that was received */
		RV_IN uint16_t sequence)			/* The sequence number of the packet */
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
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			bRet = (RV_OK == RvRtcpRTPPacketRecv(rtcpH, ssrc, localtimestamp, mytimestamp, sequence));
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::force_send_rtcp_sr(
		RV_IN rv_handler hrv,				/* The handle of the RTCP session */
		RV_IN bool bCompound,				/* If set to true, sends a compound report (RR + ALL SDES + APP) */
		/* If set to false, sends a receiver report only */
		RV_IN rv_rtcp_appmessage * appMessageTable, /* A pointer to an array holding all APP messages to be sent */
		RV_IN int32_t	appMessageTableEntriesNum)	/* The number of messages in appMessageTable */
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !appMessageTable) break;
		#endif
			bRet = true;
		#if (RV_CORE_ENABLE)
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			RvBool Compound = (bCompound) ? RV_TRUE : RV_FALSE;
			bRet = (0 < RvRtcpSessionSendSR(rtcpH, Compound,
								(RvRtcpAppMessage *)appMessageTable, appMessageTableEntriesNum));
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::force_send_rtcp_rr(
		RV_IN rv_handler hrv,				/* The handle of the RTCP session */
		RV_IN bool bCompound,				/* If set to true, sends a compound report (RR + ALL SDES + APP) */
		/* If set to false, sends a receiver report only */
		RV_IN rv_rtcp_appmessage * appMessageTable, /* A pointer to an array holding all APP messages to be sent */
		RV_IN int32_t	appMessageTableEntriesNum)	/* The number of messages in appMessageTable */
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
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			RvBool Compound = (bCompound) ? RV_TRUE : RV_FALSE;
			bRet = (0 < RvRtcpSessionSendRR(rtcpH, Compound,
				(RvRtcpAppMessage *)appMessageTable, appMessageTableEntriesNum));
		#endif
		} while (false);
		return bRet;
	}


	bool rv_adapter::set_rtcp_param(
		/*
			For RTP session with dynamic payload (with param = RVRTCP_PARAMS_RTPCLOCKRATE)
			this function should be called after the RTP session
			opening for accurate RTCP timestamp calculation.
			This call can be omitted for standard payload types.
		*/
		RV_IN rv_handler hrv,				/* The handle of the RTCP session */
		RV_IN rv_rtcp_parameters   param,	/* The type of parameter */
		RV_IN void*      data)				/* A pointer to the parameter which is defined by param */
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !data) break;
		#endif
			bRet = true;
		#if (RV_CORE_ENABLE)
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			bRet = (RV_OK == RvRtcpSessionSetParam(rtcpH, (RvRtcpParameters)param, data));
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::add_rtcp_remote_address(
		/*
			Adds the new RTCP address of the remote peer,
			or of the multicast group,
			or of the multi-unicast list with elimination of address duplication
		*/
		RV_IN rv_handler hrv,
		RV_IN rv_net_address*  pRtcpAddress)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !pRtcpAddress) break;
		#endif
			bRet = true;
		#if (RV_CORE_ENABLE)
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			RvNetAddress  rtcpAddress;
			rv_net_address_to_RvNetAddress(&rtcpAddress, pRtcpAddress);
			RvRtcpAddRemoteAddress(rtcpH, &rtcpAddress);
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::del_rtcp_remote_address(
		/*
			Removes the specified RTCP address of the remote peer,
			or of the multicast group,
			or of the multi-unicast list with elimination of address duplication
		*/
		RV_IN rv_handler hrv,
		RV_IN rv_net_address*  pRtcpAddress)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !pRtcpAddress) break;
		#endif
			bRet = true;
		#if (RV_CORE_ENABLE)
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			RvNetAddress  rtcpAddress;
			rv_net_address_to_RvNetAddress(&rtcpAddress, pRtcpAddress);
			RvRtcpRemoveRemoteAddress(rtcpH, &rtcpAddress);
		#endif
		} while (false);
		return bRet;
	}

	rv_bool rv_adapter::add_rtcp_mult_remote_address(
		/*
			Adds the new RTCP address of the remote peer,
			or of the multicast group,
			or of the multi-unicast list with elimination of address duplication
		*/
		RV_IN rv_handler hrv,
		RV_IN rv_net_address*  pRtcpAddress,
		RV_IN uint32_t multiplexID)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !pRtcpAddress) break;
		#endif
			bRet = true;
		#if (RV_CORE_ENABLE)
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			RvNetAddress  rtcpAddress;
			rv_net_address_to_RvNetAddress(&rtcpAddress, pRtcpAddress);

			RvRtcpMultiplexingAddRemoteAddress(rtcpH, &rtcpAddress, multiplexID);
		#endif
		} while (false);
		return bRet;
	}

	rv_bool rv_adapter::del_rtcp_mult_remote_address(
		/*
			Removes the specified RTCP address of the remote peer,
			or of the multicast group,
			or of the multi-unicast list with elimination of address duplication
		*/
		RV_IN rv_handler hrv,
		RV_IN rv_net_address*  pRtcpAddress,
		RV_IN uint32_t multiplexID)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !pRtcpAddress) break;
		#endif
			bRet = true;
		#if (RV_CORE_ENABLE)
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			RvNetAddress  rtcpAddress;
			rv_net_address_to_RvNetAddress(&rtcpAddress, pRtcpAddress);

			RvRtcpMultiplexingRemoveRemoteAddress(rtcpH, &rtcpAddress, multiplexID);
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::clear_rtcp_remote_address(
		/*
			Removes all RTCP addresses of the remote peer,
			or of the multicast group,
			or of the multi-unicast list with elimination of address duplication
		*/
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
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			RvRtcpRemoveAllRemoteAddresses(rtcpH);
		#endif
		} while (false);
		return bRet;
	}

	bool rv_adapter::get_rtcp_sourceinfo(
			/*
				Provides information about a particular synchronization source
			*/
			RV_IN rv_handler hrv,
			RV_IN uint32_t ssrc,
			RV_OUT rv_rtcp_info * info
			)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv) break;
			if (!info) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
            RvRtcpINFO rv_info = { 0 };
			if (0 != RvRtcpGetSourceInfo(rtcpH, ssrc, &rv_info))
            {
                break;
            }
			RvRtcpINFO_to_rv_rtcp_info(info, &rv_info);
		#endif
            bRet = true;
		} while (false);
		return bRet;
	}

	bool rv_adapter::get_rtcp_ssrc_remote(
			/*
				Provides information about a particular synchronization source
			*/
			RV_IN rv_handler hrv,  
			RV_IN uint32_t ssrc,
			RV_OUT rv_net_address *addr
			)
	{
		bool bRet = false;
		do 
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv) break;
			//if (!info) break;
		#endif
			bRet = true;
		#if (RV_CORE_ENABLE)
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			RvNetAddress a;
			int ret = RvRtcpGetRemoteAddress(rtcpH, ssrc, &a);
			if (ret < 0)
			{
				bRet = false;
			}

			RvNetAddress_to_rv_net_address(addr, &a);

		#endif
		} while (false);
		return bRet;
	}
	bool rv_adapter::rtcp_set_appevent(
			RV_IN rv_handler         hrv,
			RV_IN RtcpAppEventHandler_CB pAppEventHandler)
	{
		if (hrv)
		{
			hrv->onRtcpAppEvent = (rv_context)pAppEventHandler;
		}
		return true;
	}

	int rv_adapter::rtcp_send_apps(
			RV_IN rv_handler         hrv,
			RV_IN RtcpAppMessage* appMessageTable,
			RV_IN uint32_t appMessageTableEntriesNum,
			RV_IN bool bCompound)
	{
		RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
		return RvRtcpSessionSendApps(rtcpH, (RvRtcpAppMessage*)appMessageTable, appMessageTableEntriesNum, bCompound);
	}

	int rv_adapter::rtcp_send_raw(
			RV_IN rv_handler	hrv,
			RV_IN void *		buf,
			RV_IN uint32_t	buf_len)
	{
		RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
		return RvRtcpSendRawData(rtcpH, (RvUint8*)buf, (RvSize_t)buf_len);
	}
	int rv_adapter::rtcp_set_rawevent(
		RV_IN rv_handler         hrv,
		RV_IN RtcpRawBufferReceived_CB cb)
	{
		if (hrv)
		{
			hrv->onRtcpRawEvent = (rv_context)cb;
		}

		return 0;
	}

} /* end of namespace rv*/
