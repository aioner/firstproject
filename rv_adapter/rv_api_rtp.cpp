///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_api_rtp.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月16日
// 内容描述：radvsion ARTP协议栈适配器 -- 外部引用接口实现 -- rtp
///////////////////////////////////////////////////////////////////////////////////////////

#include <rvconfig.h>

#include "rv_def.h"
#include "rv_api.h"
#include "rv_adapter.h"
#include "mem_check_on.h"


//rtp数据发送函数，
rv_bool write_rtp(
				  RV_IN rv_handler hrv,
				  RV_IN void * buf,
				  RV_IN uint32_t buf_len,
				  RV_INOUT rv_rtp_param * p)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->write_rtp(hrv, buf, buf_len, p)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool write_rtp_s(RV_IN rv_handler hrv,
					RV_IN void * buf,
					RV_IN uint32_t buf_len,
					RV_INOUT rv_rtp_param * p,
					RV_IN rv_bool async_mode)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->write_rtp_s(hrv, buf, buf_len, p, (RV_ADAPTER_TRUE == async_mode))) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

rv_bool write_rtp_s2(RV_IN rv_handler hrv,
					 RV_IN void * buf,			//tghelper::byte_block
					 RV_INOUT rv_rtp_param * p,
					 RV_IN rv_bool async_mode)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->write_rtp_s2(hrv, buf, p, (RV_ADAPTER_TRUE == async_mode))) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//rtp数据接收函数，
rv_bool read_rtp(
				 RV_IN rv_handler hrv,
				 RV_IN void * buf,
				 RV_IN uint32_t buf_len,
				 RV_INOUT rv_rtp_param * p,
				 RV_INOUT rv_net_address *addr)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->read_rtp(hrv, buf, buf_len, p, addr)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//rtp输出地址增加函数，可以增加组播地址
rv_bool add_rtp_remote_address(
							   RV_IN rv_handler hrv,
							   RV_IN rv_net_address*  pRtpAddress)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->add_rtp_remote_address(hrv, pRtpAddress)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//rtp输出地址删除函数，可以删除组播地址
rv_bool del_rtp_remote_address(
							   RV_IN rv_handler hrv,
							   RV_IN rv_net_address*  pRtpAddress)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->del_rtp_remote_address(hrv, pRtpAddress)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//rtp输出地址增加函数，可以增加组播地址
rv_bool add_rtp_mult_remote_address(
							   RV_IN rv_handler hrv,
							   RV_IN rv_net_address*  pRtpAddress,
							   RV_IN uint32_t multiplexID)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->add_rtp_mult_remote_address(hrv, pRtpAddress, multiplexID)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//rtp输出地址删除函数，可以删除组播地址
rv_bool del_rtp_mult_remote_address(
							   RV_IN rv_handler hrv,
							   RV_IN rv_net_address*  pRtpAddress,
							   RV_IN uint32_t multiplexID)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->del_rtp_mult_remote_address(hrv, pRtpAddress, multiplexID)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//rtp输出地址清空函数
rv_bool clear_rtp_remote_address(
								 RV_IN rv_handler hrv)
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->clear_rtp_remote_address(hrv)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//rtp输出地址清空函数
rv_bool set_rtp_group_address(
							  RV_IN rv_handler hrv,
							  RV_IN rv_net_address*  pRtpAddress
							  )
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->set_rtp_group_address(hrv,pRtpAddress)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}

//rtp输出地址清空函数
rv_bool set_rtp_multicast_ttl(
							  RV_IN rv_handler hrv,
							  RV_IN uint8_t    ttl
							  )
{
	rv_bool bRet = RV_ADAPTER_FALSE;
	rv::rv_adapter::share_lock();
	do
	{
		rv::rv_adapter * adapter = rv::rv_adapter::self();
		if (!adapter) break;
		if (!adapter->set_rtp_multicast_ttl(hrv,ttl)) break;
		bRet = RV_ADAPTER_TRUE;
	} while (false);
	rv::rv_adapter::share_unlock();
	return bRet;
}
