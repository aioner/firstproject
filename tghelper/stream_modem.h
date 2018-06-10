///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：stream_modem.h
// 创 建 者：汤戈
// 创建时间：2011年02月27日
// 内容描述：通用流式调制解调器--用于内存流、文件流或tcp流上传输信息块工具类
//
// 1、所有接口类均为多线程不安全，多线程环境外部需进行锁控处理
// 2、内部采用base64编码转换方式工作
// 3、函数入口参数的有效性检查非常弱，主要是提高函数的执行效率
//
// 修订日志
// [2011-02-27]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef STREAM_MODEM_
#define STREAM_MODEM_

#include<stdint.h>
#include <boost/circular_buffer.hpp>

#if (_TGHELP_DEBUG_ENABLE)
	#include <iostream>
#endif

namespace tghelper
{
	typedef enum _EStreamModemResult
	{
		STREAM_MODEM_OK	= 0,
		STREAM_MODEM_RDY = 1,
		STREAM_MODEM_ERR = -1,
		STREAM_MODEM_STATE_ERR	= -2,
	}EStreamModemResult;

	typedef enum _EStreamModemState
	{
		ESMS_IDLE,		//空闲状态，扫描PU块头
		ESMS_WAITTING,	//等待块结束状态，扫描PU块尾
		ESMS_READY,		//登录有效实体
		//错误状态，系统进入状态锁定状态
		ESMS_ERR_FULL,	//缓冲区满错误状态
	} EStreamModemState;

	typedef enum _EStreamModemFlag
	{
		ESMF_HEAD = 0xFF,
		ESMF_TAIL = 0xFE,
	}EStreamModemFlag;

	struct StreamModemState
	{
		EStreamModemState m_state;
		
		StreamModemState() : m_state(ESMS_IDLE)
		{ 	
		}
		virtual ~StreamModemState() 
		{
		}
		
		inline EStreamModemState transit(EStreamModemState newState)
		{
			if (m_state != newState) m_state = newState;
			return m_state;
		}

		virtual void reset()
		{
			m_state = ESMS_IDLE;
		}

		virtual bool push(uint8_t pu_byte) { return false;}
		virtual bool pop(uint8_t *pu, uint32_t *pu_size) { return false; }
	};

	namespace inner
	{
		class stream_modem_kernel
		{
		public:
			stream_modem_kernel();
			~stream_modem_kernel();
		protected:
			//signal unit编码形成protocol unit
			bool su_encode(const uint8_t *su, const uint32_t su_size, // input
						   uint8_t *pu, uint32_t *pu_size);			  // output	
			//protocol unit解码形成signal unit
			bool pu_decode(const uint8_t *pu, const uint32_t pu_size, // input
						   uint8_t *su, uint32_t *su_size);			  // output	
			//流解析器
			EStreamModemState pu_demux(const uint8_t pu_byte, StreamModemState* state); 
		};
	}
	
	class stream_modem : public inner::stream_modem_kernel
	{
	public:
		stream_modem();
		~stream_modem();
	
	public:
		bool encode(const uint8_t *su, const uint32_t su_size, uint8_t *pu, uint32_t *pu_size)
		{ return su_encode(su, su_size, pu, pu_size); }

		bool decode(const uint8_t *pu, const uint32_t pu_size, uint8_t *su, uint32_t *su_size)
		{ return pu_decode(pu, pu_size, su, su_size); }

		EStreamModemState demux(const uint8_t pu_byte, StreamModemState* state)
		{ return pu_demux(pu_byte, state); }

		EStreamModemResult decode_demux(const uint8_t pu_byte, StreamModemState* state, 
										uint8_t *pu, uint32_t *pu_size,
										uint8_t *su, uint32_t *su_size);
	};

	template<uint32_t bufSize>
	struct DefaultStreamModemState : public StreamModemState
	{
		typedef boost::circular_buffer<uint8_t> smsbuffers;
		smsbuffers m_buffer;
		DefaultStreamModemState() : m_buffer(bufSize)
		{
			
		}
		virtual ~DefaultStreamModemState()
		{

		}

		virtual void reset()
		{
			StreamModemState::reset();
			m_buffer.clear();
		}

		virtual bool push(uint8_t pu_byte) 
		{ 
			if (m_buffer.full()) return false;
			m_buffer.push_back(pu_byte);
			return true;
		}
		virtual bool pop(uint8_t *pu, uint32_t *pu_size) 
		{ 
			if (m_buffer.empty()) return false;
			uint32_t buf_size = m_buffer.size();
			if (buf_size > *pu_size) return false;
			*pu_size = buf_size;
			uint32_t i = 0;
			smsbuffers::iterator it;
			for (it = m_buffer.begin(); it != m_buffer.end(); it++)
			{
				pu[i] = *it;
				i++;
			}
			return true; 
		}
	};
 
}
#endif
