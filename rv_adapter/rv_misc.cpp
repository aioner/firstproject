///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_misc.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月16日
// 内容描述：radvsion ARTP协议栈适配器 -- 杂项函数
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
	//	获取当前rtp的ssrc
	uint32_t rv_adapter::get_rtp_ssrc(RV_IN rv_handler hrv)
	{
		uint32_t ssrc = 0;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			ssrc = RvRtpGetSSRC(rtpH);
		#endif
		} while (false);
		return ssrc;
	}

	//	重新生成rtp的ssrc，并返回生成的当前值
	uint32_t rv_adapter::regen_rtp_ssrc(RV_IN rv_handler hrv)
	{
		uint32_t ssrc = 0;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			ssrc = RvRtpRegenSSRC(rtpH);
		#endif
		} while (false);
		return ssrc;
	}

	//	获取当前rtp的ssrc
	uint32_t rv_adapter::get_rtcp_ssrc(RV_IN rv_handler hrv)
	{
		uint32_t ssrc = 0;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			ssrc = RvRtcpGetSSRC(rtcpH);
		#endif
		} while (false);
		return ssrc;
	}

	//	重新生成rtp的ssrc，并返回生成的当前值
	bool rv_adapter::set_rtcp_ssrc(RV_IN rv_handler hrv, uint32_t ssrc)
	{
		bool bRet =false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtcpSession rtcpH = (RvRtcpSession)(hrv->hrtcp);
			ssrc = RvRtcpSetSSRC(rtcpH, ssrc);
		#endif
		} while (false);
		return bRet;
	}
	//  提取系统默认rtp固定头字节长度
	uint32_t rv_adapter::get_rtp_header_lenght()
	{
		uint32_t len = 0;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
		#endif
		#if (RV_CORE_ENABLE)
			len = RvRtpGetHeaderLength();
		#endif
		} while (false);
		return len;
	}
	//  Sets the RTP header before sending an RTP packet
	bool rv_adapter::rv_rtp_pack(
		RV_IN rv_handler	hrv,
		RV_IN void*         buf,
		RV_IN uint32_t      len,
		RV_IN rv_rtp_param* p)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !buf || !p) break;
			if (!len) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			RvRtpParam _p;
			rv_rtp_param_to_RvRtpParam(&_p, p);
			bRet = (RV_OK == RvRtpPack(rtpH, buf, len, &_p));
		#endif
		} while (false);
		return bRet;
	}
	bool rv_adapter::rv_rtp_pack_ex(
		RV_IN rv_handler	hrv,
		RV_IN uint32_t      ssrc,		/* 可替换SSRC */
		RV_IN void*         buf,
		RV_IN uint32_t      len,
		RV_IN rv_rtp_param* p)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !buf || !p) break;
			if (!len) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			RvRtpParam _p;
			rv_rtp_param_to_RvRtpParam(&_p, p);
			bRet = (RV_OK == RvRtpPackEx(rtpH, ssrc, buf, len, &_p));
		#endif
		} while (false);
		return bRet;
	}

	//
	// Gets the RTP header from a received RTP message buffer
	bool rv_adapter::rv_rtp_unpack(
		RV_IN rv_handler	hrv,
		RV_IN void*         buf,
		RV_IN uint32_t      len,
		RV_OUT rv_rtp_param *p)
	{
		bool bRet = false;
		do
		{
		#if (RV_ADAPTER_PARAM_CHECK)
			if (!m_bReady) break;
			if (!hrv || !buf || !p) break;
			if (!len) break;
		#endif
		#if (RV_CORE_ENABLE)
			RvRtpSession rtpH = (RvRtpSession)(hrv->hrtp);
			RvRtpParam _p;
			RvRtpParamConstruct(&_p);
			bRet = (RV_OK == RvRtpUnpack(rtpH, buf, len, &_p));
			if (bRet) RvRtpParam_to_rv_rtp_param(p, &_p);
		#endif
		} while (false);
		return bRet;
	}

	//  设置协议栈传输缓冲区字节长度
	bool rv_adapter::set_rtp_transmit_buffer_size(RV_IN rv_handler hrv,RV_IN uint32_t size)
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
			bRet = (RV_OK == RvRtpSetTransmitBufferSize(rtpH, size));
		#endif
		} while (false);
		return bRet;
	}

	//  设置协议栈接收缓冲区字节长度
	bool rv_adapter::set_rtp_receive_buffer_size(RV_IN rv_handler hrv,RV_IN uint32_t size)
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
			bRet = (RV_OK == RvRtpSetReceiveBufferSize(rtpH, size));
		#endif
		} while (false);
		return bRet;
	}

	//全局函数
	//  rv_net_ipv4 translate to rv_net_address
	bool rv_adapter::convert_ipv4_to_rvnet(RV_OUT rv_net_address * dst, RV_IN rv_net_ipv4 * src)
	{
		RvNetAddress  rtpAddress;
		RvNetIpv4     ipv4Address;
		rv_net_ipv4_to_RvNetIpv4(&ipv4Address, src);
		if (RV_OK == RvNetCreateIpv4(&rtpAddress, &ipv4Address))
		{
			RvNetAddress_to_rv_net_address(dst, &rtpAddress);
			return true;
		}
		else
			return false;
	}
	//  rv_net_ipv6 translate to rv_net_address
	bool rv_adapter::convert_ipv6_to_rvnet(RV_OUT rv_net_address * dst, RV_IN rv_net_ipv6 * src)
	{
		RvNetAddress  rtpAddress;
		RvNetIpv6     ipv6Address;
		rv_net_ipv6_to_RvNetIpv6(&ipv6Address, src);
		if (RV_OK == RvNetCreateIpv6(&rtpAddress, &ipv6Address))
		{
			RvNetAddress_to_rv_net_address(dst, &rtpAddress);
			return true;
		}
		else
			return false;
	}
	//  rv_net_address translate to rv_net_ipv4
	bool rv_adapter::convert_rvnet_to_ipv4(RV_OUT rv_net_ipv4 * dst, RV_IN rv_net_address * src)
	{
		RvNetAddress  rtpAddress;
		RvNetIpv4     ipv4Address;
		rv_net_address_to_RvNetAddress(&rtpAddress, src);
		
		if (RV_OK == RvNetGetIpv4(&ipv4Address, &rtpAddress))
		{
			RvNetIpv4_to_rv_net_ipv4(dst, &ipv4Address);
			return true;
		}
		else
			return false;
	}
	//  rv_net_address translate to rv_net_ipv4
	bool rv_adapter::convert_rvnet_to_ipv6(RV_OUT rv_net_ipv6 * dst, RV_IN rv_net_address * src)
	{
		RvNetAddress  rtpAddress;
		RvNetIpv6     ipv6Address;
		rv_net_address_to_RvNetAddress(&rtpAddress, src);
		if (RV_OK == RvNetGetIpv6(&ipv6Address, &rtpAddress))
		{
			RvNetIpv6_to_rv_net_ipv6(dst, &ipv6Address);
			return true;
		}
		else
			return false;
	}
	//  ipv4_str translate to rv_net_ipv4
	bool rv_adapter::rv_inet_pton4(RV_OUT rv_net_ipv4 * dst, RV_IN const char *src)
	{
		static char digits[] = "0123456789";
		int32_t saw_digit, octets, ch;
		uint8_t tmp[4], *tp;

		saw_digit = 0;
		octets = 0;
		*(tp = tmp) = 0;
		while ((ch = *src++) != '\0' && (ch != ':')) {
			RvChar *pch;

			if ((pch = strchr(digits, ch)) != NULL) {
				RvUint newValue = (RvUint)(*tp * 10 + (pch - digits));

				if (newValue > 255)
					return false;
				*tp = (RvUint8)newValue;
				if (! saw_digit) {
					if (++octets > 4)
						return false;
					saw_digit = 1;
				}
			} else if (ch == '.' && saw_digit) {
				if (octets == 4)
					return false;
				*++tp = 0;
				saw_digit = 0;
			} else
				return false;
		}
		if (octets < 4)
			return false;
		memcpy(dst, tmp, 4);
		return true;
	}

	bool rv_adapter::construct_rv_handler(RV_INOUT rv_handler hrv)
	{
		if (hrv)
		{
			memset(hrv, 0, sizeof(rv_handler_s));
			return true;
		}
		else
			return false;
	}
	bool rv_adapter::construct_rv_net_ipv4(RV_INOUT rv_net_ipv4 *address)
	{
		if (address)
		{
			memset(address, 0, sizeof(rv_net_ipv4));
			return true;
		}
		else
			return false;
	}
	bool rv_adapter::construct_rv_net_ipv6(RV_INOUT rv_net_ipv6 *address)
	{
		if (address)
		{
			memset(address, 0, sizeof(rv_net_ipv6));
			return true;
		}
		else
			return false;
	}
	bool rv_adapter::construct_rv_net_address(RV_INOUT rv_net_address *address)
	{
		if (address)
		{
			memset(address, 0, sizeof(rv_net_address));
			return true;
		}
		else
			return false;
	}
	bool rv_adapter::construct_rv_session_descriptor(RV_INOUT rv_session_descriptor *descriptor)
	{
		if (descriptor)
		{
			memset(descriptor, 0, sizeof(rv_session_descriptor));
			return true;
		}
		else
			return false;
	}

	bool rv_adapter::construct_rv_rtp_param(RV_INOUT rv_rtp_param *param)
	{
		if (param)
		{
			memset(param, 0, sizeof(rv_rtp_param));
			return true;
		}
		else
			return false;
	}

	void RvRtcpINFO_to_rv_rtcp_info(rv_rtcp_info *dst, RvRtcpINFO *src)
	{
		dst->selfNode = src->selfNode;
		memcpy(&(dst->sr), &(src->sr), sizeof(RvRtcpSRINFO));
		memcpy(&(dst->rrFrom), &(src->rrFrom), sizeof(RvRtcpRRINFO));
		memcpy(&(dst->rrTo), &(src->rrTo), sizeof(RvRtcpRRINFO));
		memcpy(dst->cname, src->cname, 255);
        dst->rtt = src->rtt;
	}

	void rv_rtcp_info_to_RvRtcpINFO(RvRtcpINFO *src, rv_rtcp_info *dst)
	{
		dst->selfNode = src->selfNode;
		memcpy(&(dst->sr), &(src->sr), sizeof(RvRtcpSRINFO));
		memcpy(&(dst->rrFrom), &(src->rrFrom), sizeof(RvRtcpRRINFO));
		memcpy(&(dst->rrTo), &(src->rrTo), sizeof(RvRtcpRRINFO));
		memcpy(dst->cname, src->cname, 255);
	}

} /* end of namespace rv*/
