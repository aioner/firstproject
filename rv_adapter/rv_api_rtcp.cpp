///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_api_rtcp.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月16日
// 内容描述：radvsion ARTP协议栈适配器 -- 外部引用接口实现 -- rtcp
///////////////////////////////////////////////////////////////////////////////////////////
#include <rvconfig.h>

#include "rv_def.h"
#include "rv_api.h"
#include "rv_adapter.h"
#include "mem_check_on.h"


rv_bool  set_rtcp_bandwidth(
	RV_IN rv_handler hrv,				/* The handle of the RTCP session */
	RV_IN uint32_t bandwidth)			/* The bandwidth for RTCP packets */
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->set_rtcp_bandwidth(hrv, bandwidth)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool set_manual_rtcp(
	 RV_IN rv_handler hrv,
	 RV_IN rv_bool manual_rtcp)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->set_manual_rtcp(hrv, (RV_ADAPTER_TRUE == manual_rtcp))) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool manual_send_rtcp_sr(
	RV_IN rv_handler hrv,				/* The handle of the RTCP session */
	RV_IN uint32_t pack_size,			/* The number of bytes in the packet that was sent */
	RV_IN uint32_t pack_ts)				/* The RTP timestamp from the packet that was sent */
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->manual_send_rtcp_sr(hrv, pack_size, pack_ts)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool manual_send_rtcp_rr(
							RV_IN rv_handler hrv,				/* The handle of the RTCP session */
							RV_IN uint32_t ssrc,				/* The synchronization source value of the participant that sent the packet */
							RV_IN uint32_t localtimestamp,		/* The local RTP timestamp when the received packet arrived */
							RV_IN uint32_t mytimestamp,			/* The RTP timestamp from the packet that was received */
							RV_IN uint16_t sequence)			/* The sequence number of the packet */
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->manual_send_rtcp_rr(hrv, ssrc, localtimestamp, mytimestamp, sequence)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool force_send_rtcp_sr(
	RV_IN rv_handler hrv,				/* The handle of the RTCP session */
	RV_IN rv_bool bCompound,				/* If set to true, sends a compound report (RR + ALL SDES + APP) */
	/* If set to false, sends a receiver report only */
	RV_IN rv_rtcp_appmessage * appMessageTable, /* A pointer to an array holding all APP messages to be sent */
	RV_IN int32_t	appMessageTableEntriesNum)	/* The number of messages in appMessageTable */
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->force_send_rtcp_sr(hrv, (RV_ADAPTER_TRUE == bCompound), appMessageTable, appMessageTableEntriesNum)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool force_send_rtcp_rr(
	RV_IN rv_handler hrv,				/* The handle of the RTCP session */
	RV_IN rv_bool bCompound,				/* If set to true, sends a compound report (RR + ALL SDES + APP) */
	/* If set to false, sends a receiver report only */
	RV_IN rv_rtcp_appmessage * appMessageTable, /* A pointer to an array holding all APP messages to be sent */
	RV_IN int32_t	appMessageTableEntriesNum)	/* The number of messages in appMessageTable */
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->force_send_rtcp_rr(hrv, (RV_ADAPTER_TRUE == bCompound), appMessageTable, appMessageTableEntriesNum)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool set_rtcp_param(
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
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->set_rtcp_param(hrv, param, data)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool add_rtcp_remote_address(
	/*
		Adds the new RTCP address of the remote peer,
		or of the multicast group,
		or of the multi-unicast list with elimination of address duplication
	*/
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtcpAddress)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->add_rtcp_remote_address(hrv, pRtcpAddress)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool del_rtcp_remote_address(
	/*
		Removes the specified RTCP address of the remote peer,
		or of the multicast group,
		or of the multi-unicast list with elimination of address duplication
	*/
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtcpAddress)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->del_rtcp_remote_address(hrv, pRtcpAddress)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool add_rtcp_mult_remote_address(
	/*
		Adds the new RTCP address of the remote peer,
		or of the multicast group,
		or of the multi-unicast list with elimination of address duplication
	*/
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtcpAddress,
	RV_IN uint32_t multiplexID)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->add_rtcp_mult_remote_address(hrv, pRtcpAddress, multiplexID)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool del_rtcp_mult_remote_address(
	/*
		Removes the specified RTCP address of the remote peer,
		or of the multicast group,
		or of the multi-unicast list with elimination of address duplication
	*/
	RV_IN rv_handler hrv,
	RV_IN rv_net_address*  pRtcpAddress,
	RV_IN uint32_t multiplexID)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->del_rtcp_mult_remote_address(hrv, pRtcpAddress, multiplexID)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool clear_rtcp_remote_address(
	/*
		Removes all RTCP addresses of the remote peer,
		or of the multicast group,
		or of the multi-unicast list with elimination of address duplication
	*/
	RV_IN rv_handler hrv)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->clear_rtcp_remote_address(hrv)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool get_rtcp_sourceinfo(
		/*
			Provides information about a particular synchronization source
		*/
		RV_IN rv_handler hrv,
		RV_IN uint32_t ssrc,
		RV_OUT rv_rtcp_info * info
		)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->get_rtcp_sourceinfo(hrv, ssrc, info)) break;
			bRet = RV_ADAPTER_TRUE;
	} while (false);
		rv::rv_adapter::share_unlock();
	return bRet;
	}
rv_bool get_rtcp_ssrc_remote(
			/*
				Provides information about a particular synchronization source
			*/
			RV_IN rv_handler hrv,  
			RV_IN uint32_t ssrc,
			RV_OUT rv_net_address *addr
			)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do 
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->get_rtcp_ssrc_remote(hrv, ssrc, addr)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}


int RtcpSetAppEventHandler(
						   RV_IN rv_handler         hrv,
						   RV_IN RtcpAppEventHandler_CB pAppEventHandler)
{
	rv::rv_adapter::share_lock();

	rv::rv_adapter * adapter = rv::rv_adapter::self();
	if (adapter)
	{
		adapter->rtcp_set_appevent(hrv, pAppEventHandler);
	}

	rv::rv_adapter::share_unlock();

	return 1;
}

int RtcpSendApps(
						RV_IN rv_handler         hrv,
						RV_IN RtcpAppMessage* appMessageTable,
						RV_IN uint32_t appMessageTableEntriesNum,
						RV_IN bool bCompound)
{
	int ret = 0;

	rv::rv_adapter::share_lock();

	rv::rv_adapter * adapter = rv::rv_adapter::self();
	if (adapter)
	{
		adapter->rtcp_send_apps(hrv, appMessageTable, appMessageTableEntriesNum, bCompound);
	}

	rv::rv_adapter::share_unlock();

	return ret;
}

rv_bool rtcp_send_rawdata(RV_IN rv_handler	hrv,
						  RV_IN void *		buf,
						  RV_IN uint32_t	buf_len)
{
	rv::rv_adapter::share_lock();

	rv::rv_adapter * adapter = rv::rv_adapter::self();
	if (adapter)
	{
		adapter->rtcp_send_raw(hrv, buf, buf_len);
	}

	rv::rv_adapter::share_unlock();

	return true;
}

rv_bool  rtcp_set_raw_eventhandler(RV_IN rv_handler	hrv,
								RV_IN RtcpRawBufferReceived_CB cb)
{
	rv::rv_adapter::share_lock();

	rv::rv_adapter * adapter = rv::rv_adapter::self();
	if (adapter)
	{
		adapter->rtcp_set_rawevent(hrv, cb);
	}

	rv::rv_adapter::share_unlock();

	return RV_TRUE;
}
