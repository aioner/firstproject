///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_api_misc.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月16日
// 内容描述：radvsion ARTP协议栈适配器 -- 外部引用接口实现 -- misc
///////////////////////////////////////////////////////////////////////////////////////////
#include <rvconfig.h>

#include "rv_def.h"
#include "rv_api.h"
#include "rv_adapter.h"
#include "mem_check_on.h"


//	获取当前rtp的ssrc
uint32_t get_rtp_ssrc(RV_IN rv_handler hrv)
{
	uint32_t nRet = 0;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		nRet = adapter->get_rtp_ssrc(hrv);
	} while (false);
	rv::rv_adapter::share_unlock();
	return nRet;
}

//	重新生成rtp的ssrc，并返回生成的当前值
uint32_t regen_rtp_ssrc(RV_IN rv_handler hrv)
{
	uint32_t nRet = 0;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		nRet = adapter->regen_rtp_ssrc(hrv);
	} while (false);
	rv::rv_adapter::share_unlock();
	return nRet;
}

//	获取当前rtp的ssrc
uint32_t get_rtcp_ssrc(RV_IN rv_handler hrv)
{
	uint32_t nRet = 0;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		nRet = adapter->get_rtcp_ssrc(hrv);
	} while (false);
	rv::rv_adapter::share_unlock();
	return nRet;
}

//	重新生成rtp的ssrc，并返回生成的当前值
rv_bool set_rtcp_ssrc(RV_IN rv_handler hrv, uint32_t ssrc)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->set_rtcp_ssrc(hrv, ssrc)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//  提取系统默认rtp固定头字节长度
uint32_t get_rtp_header_lenght()
{
	uint32_t nRet = 0;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		nRet = adapter->get_rtp_header_lenght();
	} while (false);
	rv::rv_adapter::share_unlock();
	return nRet;
}

//  Sets the RTP header before sending an RTP packet
rv_bool rv_rtp_pack(
		RV_IN rv_handler	hrv,
		RV_IN void*         buf,
		RV_IN uint32_t      len,
		RV_IN rv_rtp_param* p)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->rv_rtp_pack(hrv, buf, len, p)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool rv_rtp_pack_ex(
	   RV_IN rv_handler	hrv,
	   RV_IN uint32_t      ssrc,		/* 可替换SSRC */
	   RV_IN void*         buf,
	   RV_IN uint32_t      len,
	   RV_IN rv_rtp_param* p)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->rv_rtp_pack_ex(hrv, ssrc, buf, len, p)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

// Gets the RTP header from a received RTP message buffer
rv_bool rv_rtp_unpack(
	  RV_IN rv_handler	hrv,
	  RV_IN void*         buf,
	  RV_IN uint32_t      len,
	  RV_OUT rv_rtp_param *p)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->rv_rtp_unpack(hrv, buf, len, p)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//  设置协议栈传输缓冲区字节长度
rv_bool set_rtp_transmit_buffer_size(RV_IN rv_handler hrv,RV_IN uint32_t size)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->set_rtp_transmit_buffer_size(hrv, size)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//  设置协议栈接收缓冲区字节长度
rv_bool set_rtp_receive_buffer_size(RV_IN rv_handler hrv,RV_IN uint32_t size)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->set_rtp_receive_buffer_size(hrv, size)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//全局函数
//  rv_net_ipv4 translate to rv_net_address
rv_bool convert_ipv4_to_rvnet(RV_OUT rv_net_address * dst, RV_IN rv_net_ipv4 * src)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::convert_ipv4_to_rvnet(dst, src)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//  rv_net_ipv6 translate to rv_net_address
rv_bool convert_ipv6_to_rvnet(RV_OUT rv_net_address * dst, RV_IN rv_net_ipv6 * src)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::convert_ipv6_to_rvnet(dst, src)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//  rv_net_address translate to rv_net_ipv4
rv_bool convert_rvnet_to_ipv4(RV_OUT rv_net_ipv4 * dst, RV_IN rv_net_address * src)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::convert_rvnet_to_ipv4(dst, src)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//  rv_net_address translate to rv_net_ipv4
rv_bool convert_rvnet_to_ipv6(RV_OUT rv_net_ipv6 * dst, RV_IN rv_net_address * src)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::convert_rvnet_to_ipv6(dst, src)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//  ipv4_str translate to rv_net_ipv4
rv_bool rv_inet_pton4(RV_OUT rv_net_ipv4 * dst, RV_IN const char *src)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::rv_inet_pton4(dst, src)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//  初始化 rv_handler/rv_net_ipv4/rv_net_ipv6/rv_net_address/rv_session_descriptor/rv_rtp_param
rv_bool construct_rv_handler(RV_INOUT rv_handler hrv)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::construct_rv_handler(hrv)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool construct_rv_net_ipv4(RV_INOUT rv_net_ipv4 *address)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::construct_rv_net_ipv4(address)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool construct_rv_net_ipv6(RV_INOUT rv_net_ipv6 *address)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::construct_rv_net_ipv6(address)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool construct_rv_net_address(RV_INOUT rv_net_address *address)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::construct_rv_net_address(address)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool construct_rv_session_descriptor(RV_INOUT rv_session_descriptor *descriptor)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::construct_rv_session_descriptor(descriptor)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool construct_rv_rtp_param(RV_INOUT rv_rtp_param *param)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!rv::rv_adapter::construct_rv_rtp_param(param)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}
