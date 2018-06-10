///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����stream_modem.h
// �� �� �ߣ�����
// ����ʱ�䣺2011��02��27��
// ����������ͨ����ʽ���ƽ����--�����ڴ������ļ�����tcp���ϴ�����Ϣ�鹤����
//
// 1�����нӿ����Ϊ���̲߳���ȫ�����̻߳����ⲿ��������ش���
// 2���ڲ�����base64����ת����ʽ����
// 3��������ڲ�������Ч�Լ��ǳ�������Ҫ����ߺ�����ִ��Ч��
//
// �޶���־
// [2011-02-27]		���������汾
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
		ESMS_IDLE,		//����״̬��ɨ��PU��ͷ
		ESMS_WAITTING,	//�ȴ������״̬��ɨ��PU��β
		ESMS_READY,		//��¼��Чʵ��
		//����״̬��ϵͳ����״̬����״̬
		ESMS_ERR_FULL,	//������������״̬
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
			//signal unit�����γ�protocol unit
			bool su_encode(const uint8_t *su, const uint32_t su_size, // input
						   uint8_t *pu, uint32_t *pu_size);			  // output	
			//protocol unit�����γ�signal unit
			bool pu_decode(const uint8_t *pu, const uint32_t pu_size, // input
						   uint8_t *su, uint32_t *su_size);			  // output	
			//��������
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
