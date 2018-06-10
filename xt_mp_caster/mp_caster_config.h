///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：mp_caster_config.h
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：媒体数据广播器 -- 配置文件
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MP_CASTER_CONFIG_
#define MP_CASTER_CONFIG_

#ifndef _WIN32_WINNT            // 指定要求的最低平台是 Windows Vista。
#define _WIN32_WINNT 0x0501     // 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif

//版本定义
#define MP_CASTER_VER1	0
#define MP_CASTER_VER2	0
#define MP_CASTER_VER3	15

/*
出于性能优化目可中release版本不执行函数入口参数检查机制，提高函数执行速度
前提是中debug版本调试没有问题后才启用该机制
*/
#ifndef NDEBUG
#define MP_CASTER_PARAM_CHECK	1
#else
#define MP_CASTER_PARAM_CHECK	1
#endif

//多线程管理和访问库时，此宏必须启用。
//如果能确定库的生存周期，则从性能优化角度可以考虑关闭该宏
#define MP_CASTER_SHARE_LOCK	1

//默认MP数据池配置 -- 块物理尺寸
#define MP_BLOCK_POOL_SIZE		2048
//默认MP数据池配置 -- 自动扩展数量
#define MP_BLOCK_POOL_EXPAND	10
//默认MP数据池配置 -- 预分配数量
#define MP_BLOCK_POOL_DEFAULT	10

//默认帧方式输出的伪装媒体数据类型 96 -- H.263 90000Hz Clock
//1ms = 1 * 90, 30ms = 2700, 40ms = 3600ms
#define MP_PSEUDO_PAYLOAD_TYPE	96
#define MP_PSEUDO_RTP_MAX_SIZE	1400
#define MP_PSEUDO_RTP_HEAD_SIZE	16
#define MP_PSEUDO_TS_CLOCK		90

//Caster Engine内部定时器最小周期值，不同操作系统该值会略有区别
#define CASTER_ENGINE_TIMER_SLICE	10

#define MP_MSSRC_TASK_LOCK_TM		1
#define MP_MSINK_TASK_LOCK_TM		1


//FIFO属性配置
/*
MSSRC_FRAME		按120fps设计缓冲节点数
MSSRC_RTP		按30M带宽计算 最大包长 MP_PSEUDO_RTP_MAX_SIZE
MSSRC_RV_RTP	按30M带宽计算 最大包长 MP_PSEUDO_RTP_MAX_SIZE
*/
//#define MSSRC_FRAME_FIFO_SIZE			120
//#define MSSRC_FRAME_FIFO_OVERLAPPED		true
#define MSSRC_FRAME_FIFO_SIZE			0
#define MSSRC_FRAME_FIFO_OVERLAPPED		false

//#define MSSRC_RTP_FIFO_SIZE				256
//#define MSSRC_RTP_FIFO_OVERLAPPED		true
#define MSSRC_RTP_FIFO_SIZE				0
#define MSSRC_RTP_FIFO_OVERLAPPED		false

//#define MSSRC_RV_RTP_FIFO_SIZE			256
//#define MSSRC_RV_RTP_FIFO_OVERLAPPED	true
#define MSSRC_RV_RTP_FIFO_SIZE			0
#define MSSRC_RV_RTP_FIFO_OVERLAPPED	false

//配置mssrc_rv_rtp实体的rtcp sr报告的最大缓存数量
//配置mssrc_rv_rtp实体的rtcp sr报告的是否采用覆盖历史方式存放
#define MSSRC_RV_RTP_SR_FIFO_SIZE		2
#define MSSRC_RV_RTP_SR_FIFO_OVERLAPPED	true
//配置mssrc_rv_rtp实体的rtcp rr报告的最大缓存数量
//配置mssrc_rv_rtp实体的rtcp rr报告的是否采用覆盖历史方式存放
#define MSSRC_RV_RTP_RR_FIFO_SIZE		2
#define MSSRC_RV_RTP_RR_FIFO_OVERLAPPED	true

#ifdef TI_368
//配置mssrc_rv_rtp实体的socket接收缓冲区尺寸
#define MSSRC_RV_RTP_RECEIVE_BUFFER_SIZE	1 * 1024  *1024
#else
//配置mssrc_rv_rtp实体的socket接收缓冲区尺寸
#define MSSRC_RV_RTP_RECEIVE_BUFFER_SIZE	5 * 1024  *1024
#endif

/*
MSINK_MEMORY	按30M带宽计算 最大包长 MP_PSEUDO_RTP_MAX_SIZE
MSINK_RV_RTP	按30M带宽计算 最大包长 MP_PSEUDO_RTP_MAX_SIZE
*/
//#define MSINK_MEMORY_FIFO_SIZE		256
//#define MSINK_MEMORY_FIFO_OVERLAPPED	true
#define MSINK_MEMORY_FIFO_SIZE			0
#define MSINK_MEMORY_FIFO_OVERLAPPED	false

//#define MSINK_RV_RTP_FIFO_SIZE		256
//#define MSINK_RV_RTP_FIFO_OVERLAPPED	true
#define MSINK_RV_RTP_FIFO_SIZE			0
#define MSINK_RV_RTP_FIFO_OVERLAPPED	false

//配置msink_rv_rtp实体的rtcp sr报告的最大缓存数量
//配置msink_rv_rtp实体的rtcp sr报告的是否采用覆盖历史方式存放
#define MSINK_RV_RTP_SR_FIFO_SIZE		2
#define MSINK_RV_RTP_SR_FIFO_OVERLAPPED	true
//配置msink_rv_rtp实体的rtcp rr报告的最大缓存数量
//配置msink_rv_rtp实体的rtcp rr报告的是否采用覆盖历史方式存放
#define MSINK_RV_RTP_RR_FIFO_SIZE		1024
#define MSINK_RV_RTP_RR_FIFO_OVERLAPPED	true

#ifdef TI_368
//配置msink_rv_rtp实体的socket发送缓冲区尺寸
#define MSINK_RV_RTP_TRANSMIT_BUFFER_SIZE	1 * 1024 * 1024
#else
//配置msink_rv_rtp实体的socket发送缓冲区尺寸
#define MSINK_RV_RTP_TRANSMIT_BUFFER_SIZE	20 * 1024 * 1024
#endif

//msink_rv_rtp工作模式
#define MSINK_RV_RTP_WRITE_BYPASS			0
#define MSINK_RV_RTP_SYNC_WRITE_SAFE		1
#define MSINK_RV_RTP_ASYNC_WRITE_SAFE		2
#define MSINK_RV_RTP_SYNC_WRITE_FAST		3
#define MSINK_RV_RTP_ASYNC_WRITE_FAST		4
#define MSINK_RV_RTP_WRITE_MODE				MSINK_RV_RTP_WRITE_BYPASS

#include "caster_config.h"
#endif
