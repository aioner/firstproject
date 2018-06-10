///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_adapter.h
// 创 建 者：汤戈
// 创建时间：2012年03月16日
// 内容描述：radvsion ARTP协议栈适配器
//
// 1、所有接口类均为线程安全
// 2、采用boost库中的mutex对象实现线程同步
//
// 修订日志
// [2012-03-16]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef RADVISION_ADAPTER_
#define RADVISION_ADAPTER_

#include <rvconfig.h>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <rvrtpnatfw.h>
#include "rv_def.h"
#include "rv_adapter_config.h"
#include "rv_engine.h"

namespace rv
{
	class rv_adapter : private boost::noncopyable
	{
		//单件接口
	private:
		static rv_adapter * _instance;
		rv_adapter();
		~rv_adapter();

		bool init_adapter(RV_IN rv_adapter_descriptor *descriptor);
		void end_adapter();

        //added by lichao, 20150429 解决engine线程与此处写锁构成死锁的bug
        static void engine_end();

	public:
		//全局管理函数
		// 1、本模块第一调用函数，成功返回rv_adapter句柄
		static rv_adapter *init(RV_IN rv_adapter_descriptor *descriptor);
		// 2、本模块最后一个调用函数
		static void end();
		// 3、取rv_adapter句柄
		static rv_adapter *self();
		// 4、共享加锁，确保rv_adapter句柄有效性
		static void share_lock();
		// 5、共享解锁，确保rv_adapter句柄有效性
		static void share_unlock();

	public:

		//RTP/RTCP Session操作函数
		bool open_session(RV_IN rv_session_descriptor *descriptor, RV_OUT rv_handler hrv);
		bool open_session2(RV_IN rv_session_descriptor *descriptor, RV_OUT rv_handler hrv);
		bool close_session(RV_IN rv_handler hrv);
		bool close_session2(RV_IN rv_handler hrv);

		//--------------rtp控制
		//rtp数据发送函数，
		//	在用户线程调用的直接写出方式，函数内部直接调用ARTP协议栈函数
		//  radvision官方不推荐方式
		bool write_rtp(
				RV_IN rv_handler hrv,
				RV_IN void * buf,
				RV_IN uint32_t buf_len,
				RV_INOUT rv_rtp_param * p);

		//rtp数据发送函数，
		//	在用户线程调用的间接写出方式，函数内部以异步事件方式间接调用ARTP协议栈函数
		//  radvision官方推荐方式
		bool write_rtp_s(RV_IN rv_handler hrv,
			RV_IN void * buf,
			RV_IN uint32_t buf_len,
			RV_INOUT rv_rtp_param * p,
			RV_IN bool async_mode);

		//rtp数据发送函数，
		//	在用户线程调用的间接写出方式，函数内部以异步事件方式间接调用ARTP协议栈函数
		//  radvision官方推荐方式
		//  此版本比write_rtp_s优势中采用了智能指针方式交换数据，但与客户系统的内存管理模块
		//	存在深度耦合，需要谨慎采用这一高效模式
		bool write_rtp_s2(RV_IN rv_handler hrv,
			RV_IN void * buf,			//tghelper::byte_block
			RV_INOUT rv_rtp_param * p,
			RV_IN bool async_mode);

		//rtp数据接收函数，
		//	仅在adapter内部线程产生的RtpReceiveEventHandler_CB中调用
		//  读取数据长度保存中rv_rtp_param中
		bool read_rtp(
				RV_IN rv_handler hrv,
				RV_IN void * buf,
				RV_IN uint32_t buf_len,
				RV_INOUT rv_rtp_param * p,
				RV_INOUT rv_net_address *addr);

		bool add_rtp_remote_address(
			/*
				Adds the new RTP address of the remote peer,
				or the address of a multicast group,
				or of the multi-unicast address list to which the RTP stream will be sent.
			*/
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtpAddress);

		bool del_rtp_remote_address(
			/*
				Removes the specified RTP address of the remote peer,
				or the address of a multicast group,
				or of the multi-unicast address list to which the RTP stream was sent.
			*/
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtpAddress);

		bool add_rtp_mult_remote_address(
			/*
				Adds the new RTP address of the remote peer,
				or the address of a multicast group,
				or of the multi-unicast address list to which the RTP stream will be sent.
			*/
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtpAddress,
			RV_IN uint32_t multiplexID);

		bool del_rtp_mult_remote_address(
			/*
				Removes the specified RTP address of the remote peer,
				or the address of a multicast group,
				or of the multi-unicast address list to which the RTP stream was sent.
			*/
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtpAddress,
			RV_IN uint32_t multiplexID);

		bool clear_rtp_remote_address(
			/*
				Removes all RTP addresses of the remote peer,
				or the address of a multicast group,
				or of the multi-unicast address list to which the RTP stream was sent.
			*/
			RV_IN rv_handler hrv);

		//rtp输出组播地址设置函数，
		bool set_rtp_group_address(
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtpAddress);

		//rtp输出组播TTL设置函数，
		bool set_rtp_multicast_ttl(
			RV_IN rv_handler hrv,
			RV_IN uint8_t    ttl);


		//--------------rtcp控制
		bool set_rtcp_bandwidth(
			RV_IN rv_handler hrv,				/* The handle of the RTCP session */
			RV_IN uint32_t bandwidth);			/* The bandwidth for RTCP packets */
		bool set_manual_rtcp(
			RV_IN rv_handler hrv,
			RV_IN bool manual_rtcp);
		bool manual_send_rtcp_sr(
			RV_IN rv_handler hrv,				/* The handle of the RTCP session */
			RV_IN uint32_t pack_size,			/* The number of bytes in the packet that was sent */
			RV_IN uint32_t pack_ts);			/* The RTP timestamp from the packet that was sent */
		bool manual_send_rtcp_rr(
			RV_IN rv_handler hrv,				/* The handle of the RTCP session */
			RV_IN uint32_t ssrc,				/* The synchronization source value of the participant that sent the packet */
			RV_IN uint32_t localtimestamp,		/* The local RTP timestamp when the received packet arrived */
			RV_IN uint32_t mytimestamp,			/* The RTP timestamp from the packet that was received */
			RV_IN uint16_t sequence);			/* The sequence number of the packet */
		bool force_send_rtcp_sr(
			RV_IN rv_handler hrv,				/* The handle of the RTCP session */
			RV_IN bool bCompound,				/* If set to true, sends a compound report (RR + ALL SDES + APP) */
												/* If set to false, sends a receiver report only */
			RV_IN rv_rtcp_appmessage * appMessageTable, /* A pointer to an array holding all APP messages to be sent */
			RV_IN int32_t	appMessageTableEntriesNum);	/* The number of messages in appMessageTable */
		bool force_send_rtcp_rr(
			RV_IN rv_handler hrv,				/* The handle of the RTCP session */
			RV_IN bool bCompound,				/* If set to true, sends a compound report (RR + ALL SDES + APP) */
												/* If set to false, sends a receiver report only */
			RV_IN rv_rtcp_appmessage * appMessageTable, /* A pointer to an array holding all APP messages to be sent */
			RV_IN int32_t	appMessageTableEntriesNum);	/* The number of messages in appMessageTable */
		bool set_rtcp_param(
			/*
				For RTP session with dynamic payload (with param = RVRTCP_PARAMS_RTPCLOCKRATE)
				this function should be called after the RTP session
				opening for accurate RTCP timestamp calculation.
				This call can be omitted for standard payload types.
			*/
			RV_IN rv_handler hrv,				/* The handle of the RTCP session */
			RV_IN rv_rtcp_parameters   param,	/* The type of parameter */
			RV_IN void*      data);				/* A pointer to the parameter which is defined by param */
		bool add_rtcp_remote_address(
			/*
				Adds the new RTCP address of the remote peer,
				or of the multicast group,
				or of the multi-unicast list with elimination of address duplication
			*/
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtcpAddress);
		bool del_rtcp_remote_address(
			/*
				Removes the specified RTCP address of the remote peer,
				or of the multicast group,
				or of the multi-unicast list with elimination of address duplication
			*/
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtcpAddress);
		rv_bool add_rtcp_mult_remote_address(
			/*
				Adds the new RTCP address of the remote peer,
				or of the multicast group,
				or of the multi-unicast list with elimination of address duplication
			*/
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtcpAddress,
			RV_IN uint32_t multiplexID);
		rv_bool del_rtcp_mult_remote_address(
			/*
				Removes the specified RTCP address of the remote peer,
				or of the multicast group,
				or of the multi-unicast list with elimination of address duplication
			*/
			RV_IN rv_handler hrv,
			RV_IN rv_net_address*  pRtcpAddress,
			RV_IN uint32_t multiplexID);
		bool clear_rtcp_remote_address(
			/*
				Removes all RTCP addresses of the remote peer,
				or of the multicast group,
				or of the multi-unicast list with elimination of address duplication
			*/
			RV_IN rv_handler hrv);
		bool get_rtcp_sourceinfo(
			/*
				Provides information about a particular synchronization source
			*/
			RV_IN rv_handler hrv,
			RV_IN uint32_t ssrc,
			RV_OUT rv_rtcp_info * info
			);

		bool get_rtcp_ssrc_remote(
			/*
				Provides information about a particular synchronization source
			*/
			RV_IN rv_handler hrv,  
			RV_IN uint32_t ssrc,
			RV_OUT rv_net_address *addr
			);

		bool rtcp_set_appevent(
			RV_IN rv_handler         hrv,
			RV_IN RtcpAppEventHandler_CB pAppEventHandler);

		int rtcp_send_apps(
			RV_IN rv_handler         hrv,
			RV_IN RtcpAppMessage* appMessageTable,
			RV_IN uint32_t appMessageTableEntriesNum,
			RV_IN bool bCompound);

		int rtcp_send_raw(
			RV_IN rv_handler	hrv,
			RV_IN void *		buf,
			RV_IN uint32_t	buf_len);

		int rtcp_set_rawevent(
			RV_IN rv_handler         hrv,
			RV_IN RtcpRawBufferReceived_CB cb);



		//辅助函数组
		//	获取当前rtp的ssrc
		uint32_t get_rtp_ssrc(RV_IN rv_handler hrv);
		//	重新生成rtp的ssrc，并返回生成的当前值
		uint32_t regen_rtp_ssrc(RV_IN rv_handler hrv);
		//	获取当前rtp的ssrc
		uint32_t get_rtcp_ssrc(RV_IN rv_handler hrv);
		//	重新生成rtp的ssrc，并返回生成的当前值
		bool set_rtcp_ssrc(RV_IN rv_handler hrv, uint32_t ssrc);
		//  提取系统默认rtp固定头字节长度
		uint32_t get_rtp_header_lenght();
		//  Sets the RTP header before sending an RTP packet
		bool rv_rtp_pack(
			RV_IN rv_handler	hrv,
			RV_IN void*         buf,
			RV_IN uint32_t      len,
			RV_IN rv_rtp_param* p);
		bool rv_rtp_pack_ex(
			RV_IN rv_handler	hrv,
			RV_IN uint32_t      ssrc,		/* 可替换SSRC */
			RV_IN void*         buf,
			RV_IN uint32_t      len,
			RV_IN rv_rtp_param* p);
		// Gets the RTP header from a received RTP message buffer
		bool rv_rtp_unpack(
			RV_IN rv_handler	hrv,
			RV_IN void*         buf,
			RV_IN uint32_t      len,
			RV_OUT rv_rtp_param *p);

		//  设置协议栈传输缓冲区字节长度
		bool set_rtp_transmit_buffer_size(RV_IN rv_handler hrv,RV_IN uint32_t size);
		//  设置协议栈接收缓冲区字节长度
		bool set_rtp_receive_buffer_size(RV_IN rv_handler hrv,RV_IN uint32_t size);

		void* demux_construct(RV_IN uint32_t numberOfSessions, RV_IN rv_handler hrv);
		void demux_deconstruct(RV_IN void* demux);

		bool open_demux_session(RV_IN rv_session_descriptor *descriptor, RV_IN void* rtpDemux, RV_IN uint32_t *multiplexID, RV_OUT rv_handler hrv);
		bool close_demux_session(RV_IN rv_handler hrv);

		//全局函数
		//  rv_net_ipv4 translate to rv_net_address
		static bool convert_ipv4_to_rvnet(RV_OUT rv_net_address * dst, RV_IN rv_net_ipv4 * src);
		//  rv_net_ipv6 translate to rv_net_address
		static bool convert_ipv6_to_rvnet(RV_OUT rv_net_address * dst, RV_IN rv_net_ipv6 * src);
		//  rv_net_address translate to rv_net_ipv4
		static bool convert_rvnet_to_ipv4(RV_OUT rv_net_ipv4 * dst, RV_IN rv_net_address * src);
		//  rv_net_address translate to rv_net_ipv4
		static bool convert_rvnet_to_ipv6(RV_OUT rv_net_ipv6 * dst, RV_IN rv_net_address * src);
		//  ipv4_str translate to rv_net_ipv4
		static bool rv_inet_pton4(RV_OUT rv_net_ipv4 * dst, RV_IN const char *src);
		//  初始化 rv_handler/rv_net_ipv4/rv_net_ipv6/rv_net_address/rv_session_descriptor/rv_rtp_param
		static bool construct_rv_handler(RV_INOUT rv_handler hrv);
		static bool construct_rv_net_ipv4(RV_INOUT rv_net_ipv4 *address);
		static bool construct_rv_net_ipv6(RV_INOUT rv_net_ipv6 *address);
		static bool construct_rv_net_address(RV_INOUT rv_net_address *address);
		static bool construct_rv_session_descriptor(RV_INOUT rv_session_descriptor *descriptor);
		static bool construct_rv_rtp_param(RV_INOUT rv_rtp_param *param);
	private:
		bool m_bReady;
		rv_engine m_core;
	};
}


#endif
