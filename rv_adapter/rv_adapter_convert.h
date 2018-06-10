///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_adapter_convert.h
// 创 建 者：汤戈
// 创建时间：2012年03月19日
// 内容描述：radvsion ARTP协议栈适配器 -- 数据格式转换工具
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef RADVISION_ADAPTER_CONVERT_
#define RADVISION_ADAPTER_CONVERT_

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
//RV Adapter Data
#include "rv_def.h"

namespace rv
{
	inline void rv_net_ipv4_to_RvNetIpv4(RvNetIpv4 *dst, rv_net_ipv4 *src)
	{ memcpy(dst, src, sizeof(RvNetIpv4)); }

	inline void rv_net_ipv6_to_RvNetIpv6(RvNetIpv6 *dst, rv_net_ipv6 *src)
	{ memcpy(dst, src, sizeof(RvNetIpv6)); }

	inline void rv_net_address_to_RvNetAddress(RvNetAddress *dst, rv_net_address *src)
	{ memcpy(dst, src, sizeof(RvNetAddress)); }

	inline void RvNetAddress_to_rv_net_address(rv_net_address *dst, RvNetAddress *src)
	{ memcpy(dst, src, sizeof(RvNetAddress)); }

	inline void RvNetIpv6_to_rv_net_ipv6(rv_net_ipv6 *dst, RvNetIpv6 *src)
	{ memcpy(dst, src, sizeof(RvNetIpv6)); }

	inline void RvNetIpv4_to_rv_net_ipv4(rv_net_ipv4 *dst, RvNetIpv4 *src)
	{ memcpy(dst, src, sizeof(RvNetIpv4)); }

	inline void rv_rtp_param_to_RvRtpParam(RvRtpParam *dst, rv_rtp_param *src)
	{ memcpy(dst, src, sizeof(RvRtpParam)); }

	inline void RvRtpParam_to_rv_rtp_param(rv_rtp_param *dst, RvRtpParam *src)
	{ memcpy(dst, src, sizeof(RvRtpParam)); }

	void RvRtcpINFO_to_rv_rtcp_info(rv_rtcp_info *dst, RvRtcpINFO *src);
	void rv_rtcp_info_to_RvRtcpINFO(RvRtcpINFO *src, rv_rtcp_info *dst);

}

#endif
