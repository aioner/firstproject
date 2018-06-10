///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_adapter_config.h
// 创 建 者：汤戈
// 创建时间：2012年03月19日
// 内容描述：radvsion ARTP协议栈适配器 -- 配置文件
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef RADVISION_ADAPTER_CONFIG_
#define RADVISION_ADAPTER_CONFIG_

//版本定义
#define RV_ADAPTER_VER1		0
#define RV_ADAPTER_VER2		0
#define RV_ADAPTER_VER3		12

//#define DUMP_RTP_FILE
/*
	出于调试目的可以临时关闭库中全部的artp协议栈操作，考察框架的有效性和稳定性，
	如果RV_CORE_ENABLE = 0后，各个桩函数返回值参加代码定义，一般为成功路径
*/
#ifndef NDEBUG
#define RV_CORE_ENABLE	1	
#else
#define RV_CORE_ENABLE	1	
#endif

/*
	出于性能优化目可中release版本不执行函数入口参数检查机制，提高函数执行速度
	前提是中debug版本调试没有问题后才启用该机制
*/
#ifndef NDEBUG
#define RV_ADAPTER_PARAM_CHECK	1
#else
#define RV_ADAPTER_PARAM_CHECK	0
#endif

//多线程管理和访问库时，此宏必须启用。
//如果能确定库的生存周期，则从性能优化角度可以考虑关闭该宏
#define RV_ADAPTER_SHARE_LOCK	1

//异步写出RTP包时，模式1的缓冲区长度规划
#define RV_ADAPTER_ASYNC_WRITE_BUFFER_SIZE	2048

#endif

