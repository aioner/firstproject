#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#define DEBUG_OUTPUT	0						//控制是否输出流
#define USE_MP_MANAGER	0						//控制是否使用mp_manager管理类
#define WRITE_LOG		1						//控制是否输出日志
#define WRITE_LOG_RTP   1
#define BYTE_POOL_BLOCK_SIZE	2048			//bytepool数据块大小
#define BYTE_POOL_EXPAND_SIZE	5				//bytepool自动扩展长度
#define BYTE_POOL_INIT_SIZE		5				//bytepool初始化数据块个数
#define BYTE_BLOCK_FIFO_SIZE	0
#define MP_SOURCE_FIFO_MAX_SIZE	0				//mp source fifo最大长度,循环使用10*1024*1024
#define MP_SINK_FIFO_MAX_SIZE	0				//mp sink fifo最大长度,循环使用
#define MP_SOURCE_SN_END_JUDGE_OFFSET	65000	//rtp数据包SN号结束变化判断参数
												//(例如：一个数据包的SN为65530，下一包为3，
												//那么65530-3 > 65000则判断该rtp数据包的SN号循环一轮)
#define MP_SOURCE_SN_MAX_NUM	65536			//rtp数据包SN号最大值
#define MP_SN_JUDGE_CONDITION	32768			//
#define MP_TS_JUDGE_CONDITION	0X7FFFFFFF
#define MP_SOURCE_MAX_TURNS		100				//rtp数据包SN循环最大数
#define MP_RECEIVE_BUFFER_SIZE	20*1024*1024	//接收缓冲区大小
#define MP_SEND_BUFFER_SIZE		20*1024*1024	//发送缓冲区大小
#define MP_RTCP_INTERVAL		5				//5s发送间隔
#define MP_SND_OUT_BUFFER_SIZE	0				//发送缓冲区大小
#define MP_MSSRC_TASK_LOCK_TM	1				//二级流水串行锁超时时间
#define MP_MSINK_TASK_LOCK_TM	1				//三级流水串行锁超时时间
#define MP_FORWORD_TASK_LOCK_TM	1				//四级流水串行锁超时时间
#define MP_VERSION_MAIN			0
#define MP_VERSION_SUB			0
#define MP_VERSION_TEMP			18
#define MP_RTP_MUTICAST_TTL		128

#define __DEBUG					0

#endif
