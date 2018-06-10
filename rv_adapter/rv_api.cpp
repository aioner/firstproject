///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_api.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月16日
// 内容描述：radvsion ARTP协议栈适配器 -- 外部引用接口实现
///////////////////////////////////////////////////////////////////////////////////////////
#include <rvconfig.h>

#include "rv_def.h"
#include "rv_api.h"
#include "rv_adapter.h"
#include "mem_check_on.h"
/*
#ifdef NDEBUG
#pragma comment(lib,"libmcbl_tghelper.lib")
#pragma comment(lib,"rv32rtp.lib")
#pragma comment(lib,"rvcommon.lib")
#else
#pragma comment(lib,"libmcbl_tghelper_d.lib")
#pragma comment(lib,"rv32rtp_d.lib")
#pragma comment(lib,"rvcommon_d.lib")
#endif
*/

rv_bool init_rv_adapter(RV_IN rv_adapter_descriptor *descriptor)
{
	if (rv::rv_adapter::init(descriptor))
		return RV_ADAPTER_TRUE;
	else
		return RV_ADAPTER_FALSE;
}

void end_rv_adapter(void)
{
	rv::rv_adapter::end();
}

//获取当前版本号
void get_rv_adapter_version(uint32_t *v1, uint32_t *v2, uint32_t *v3)
{
	if(v1) *v1 = RV_ADAPTER_VER1;
	if(v2) *v2 = RV_ADAPTER_VER2;
	if(v3) *v3 = RV_ADAPTER_VER3;
}

rv_bool open_session(RV_IN rv_session_descriptor *descriptor, RV_OUT rv_handler hrv)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->open_session(descriptor, hrv)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}
rv_bool open_session2(RV_IN rv_session_descriptor *descriptor, RV_OUT rv_handler hrv)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->open_session2(descriptor, hrv)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool close_session(RV_IN rv_handler hrv)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->close_session(hrv)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool close_session2(RV_IN rv_handler hrv)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->close_session2(hrv)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rtpDemuxEventHandler g_rtpDemuxEventHandler = NULL;
rtpDemuxEventHandler g_rtpDemuxCasterEventHandler = NULL;
void setdemux_handler(rv_context func)
{
	g_rtpDemuxEventHandler = (rtpDemuxEventHandler)func;
}
void setdemux_caster_handler(rv_context func)
{
    g_rtpDemuxCasterEventHandler = (rtpDemuxEventHandler)func;
}

rv_bool read_demux_rtp(
					  RV_IN void* demux,
					  RV_IN void * buf,
					  RV_IN uint32_t buf_len,
					  RV_INOUT rv_rtp *rtpH,
					  RV_INOUT void **context,
					  RV_INOUT rv_rtp_param * p,
					  RV_INOUT rv_net_address *address)
{
	if (!demux || !buf || !rtpH || !p || !address)
	{
		return false;
	}

	RvInt32 status = RvRtpDemuxReadWithRemoteAddress((RvRtpDemux)demux, buf, buf_len, (RvRtpSession*)rtpH, context, (RvRtpParam*)p, (RvNetAddress*)address);
	if (status < 0)
	{
		return false;
	}

	return true;
}

rv_bool open_demux_session(RV_IN rv_session_descriptor *descriptor, RV_IN void* rtpDemux, RV_OUT uint32_t *multiplexID, RV_OUT rv_handler hrv)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->open_demux_session(descriptor, rtpDemux, multiplexID, hrv)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}
rv_bool close_demux_session(RV_IN rv_handler hrv)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->close_demux_session(hrv)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

void* demux_construct(RV_IN uint32_t numberOfSessions, RV_IN rv_handler hrv)
{
	void *ret = NULL;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;

		ret = adapter->demux_construct(numberOfSessions, hrv);

	} while (false);
	rv::rv_adapter::share_unlock();

	return ret;
}

void demux_deconstruct(RV_IN void* demux)
{
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;

		adapter->demux_deconstruct(demux);

	} while (false);
	rv::rv_adapter::share_unlock();
	return;
}
