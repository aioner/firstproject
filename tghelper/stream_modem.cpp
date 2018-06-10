///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：stream_modem.cpp
// 创 建 者：汤戈
// 创建时间：2011年02月27日
// 内容描述：通用流化器--用于内存流、文件流或tcp流上传输信息块工具类
//
// 修订日志
// [2011-02-27]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////
#include "stream_modem.h"
#include "base64.h"

namespace tghelper 
{
	namespace inner 
	{

		///////////////////////////////////////////////////
		//stream_modem_kernel::
		stream_modem_kernel::stream_modem_kernel()
		{

		}
		stream_modem_kernel::~stream_modem_kernel()
		{

		}

		//signal unit编码形成protocol unit
		bool stream_modem_kernel::su_encode(const uint8_t *su, const uint32_t su_size,	// input
												  uint8_t *pu, uint32_t *pu_size)		// output	
		{
			uint32_t encLen = base64EncodeLen(su_size);
			if (encLen > *pu_size) return false;

			pu[0] = (uint8_t)(ESMF_HEAD);
			*pu_size = base64Encode((char *)(pu + 1), su, su_size) + 2;
			pu[encLen + 1] = (uint8_t)(ESMF_TAIL);
			return true;
		}
		//protocol unit解码形成signal unit
		bool stream_modem_kernel::pu_decode(const uint8_t *pu, const uint32_t pu_size,	// input
												  uint8_t *su, uint32_t *su_size)		// output	
		{
			if (((uint8_t)(ESMF_HEAD) != pu[0]) || ((uint8_t)(ESMF_TAIL) != pu[pu_size - 1])) return false;
			if (base64DecodeLen((char *)(pu + 1)) > *su_size) return false;
			*su_size = base64Decode(su, (char *)(pu + 1));
			return true;
		}

		//流解析器
		EStreamModemState stream_modem_kernel::pu_demux(const uint8_t pu_byte, StreamModemState* state)
		{
			EStreamModemState sRet = state->m_state;
			switch(state->m_state)
			{
			case ESMS_READY:
				state->reset();
			case ESMS_IDLE:
				if ((uint8_t)(ESMF_HEAD) == pu_byte)
				{
					if (state->push(pu_byte))
						sRet = state->transit(ESMS_WAITTING);
					else
						sRet = state->transit(ESMS_ERR_FULL);
				}
				break;
			case ESMS_WAITTING:
				if ((uint8_t)(ESMF_TAIL) == pu_byte)
				{
					if (state->push(pu_byte))
						sRet = state->transit(ESMS_READY);
					else
						sRet = state->transit(ESMS_ERR_FULL);
				}
				else
				{
					if (!state->push(pu_byte))
						sRet = state->transit(ESMS_ERR_FULL);	
				}
				break;
			}
			return sRet;
		}
	}

	///////////////////////////////////////////////////
	//stream_modem
	stream_modem::stream_modem()
	{

	}

	stream_modem::~stream_modem()
	{

	}

	EStreamModemResult stream_modem::decode_demux(const uint8_t pu_byte, StreamModemState* state,
												  uint8_t *pu, uint32_t *pu_size,
												  uint8_t *su, uint32_t *su_size)
	{
		EStreamModemResult eRet = STREAM_MODEM_OK;
		
		EStreamModemState eState =  pu_demux(pu_byte, state);
		switch(eState)
		{
		case ESMS_IDLE:
		case ESMS_WAITTING:
			eRet = STREAM_MODEM_OK;
			break;
		case ESMS_READY:
			eRet = STREAM_MODEM_ERR;
			if (state->pop(pu, pu_size)) 
			{
				if (pu_decode(pu, *pu_size, su, su_size))
					eRet = STREAM_MODEM_RDY;
			}
			break;
		case ESMS_ERR_FULL:
			eRet = STREAM_MODEM_STATE_ERR;
			break;
		}

		return eRet;
	}

}