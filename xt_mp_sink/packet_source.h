#ifndef	PACKET_SOURCE_H
#define PACKET_SOURCE_H

#include "rtp_packet_block.h"
#include "rtp_macro_block.h"
#include "sink_config.h"

namespace xt_mp_sink
{
	typedef std::vector<rtp_packet_block *>::iterator SOURCE_ITER;
	typedef std::vector<rtp_packet_block *>::reverse_iterator SOURCE_RITER;

	typedef struct _pump_param
	{
		trans_mode mode;
		tghelper::recycle_queue * queue;
		rtp_macro_block * mblock;
	}pump_param;

	class rtp_packet_source : private boost::noncopyable
	{
	public:
		rtp_packet_source()
		{
	        memset(&m_last_param,0,sizeof(m_last_param));

			m_max_size = sink_config::inst()->pkt_cache(1024);
			m_frame_cache = sink_config::inst()->frame_cache(50);
			
			if (m_frame_cache<5 || m_frame_cache>500)
			{
				m_frame_cache = 25;
			}
		}
		virtual ~rtp_packet_source(){ clear(); }

	public:
		// xy:丢包乱序处理
		//////////////////////////////////////////////////////////////////////////
		//对接收到的包进行组包
		bool dataRecombine(rtp_packet_block* item,long& lastSeqNum,pump_param& param);

		//判断是否超时
		void checkTimeout(long& lastSeqNum);;
		//////////////////////////////////////////////////////////////////////////

		bool push(rtp_packet_block *item,long& lastSeqNum);
		rtp_packet_block * pop(bool bRelease = false);
		void erase(SOURCE_ITER pos,bool bRelease = false);
		void clear();

		uint32_t size();
//		inline void setParams(ReportEvent_CB cb_fun,void* context)
//		{
			//if(cb_fun != null)
//				m_pRptEventCB = cb_fun;

			//if(context != null)
//				m_pContext = context;
//		}
		inline void reset()
		{
			clear();
		}

	private:

		bool _push(rtp_packet_block *item,long& lastSeqNum);
		rtp_packet_block * _pop(bool bRelease);
		void _erase(SOURCE_ITER pos,bool bRelease);
		bool _insert(rtp_packet_block *item);

		bool _is_sequence_sn(SOURCE_ITER pos);

		// SN比较
		bool IsSmaller(uint16_t sn1, uint16_t sn2);

		// 丢包
		void _dropData(long& lastSeqNum);

		// 
		uint32_t _output_data(int _distance,pump_param& param);

	private:
		long m_lastFrameMarkerSN;
		
		uint32_t m_max_size;

		ReportEvent_CB m_pRptEventCB;
		void * m_pContext;
		rv_rtp_param m_last_param;

		// 缓冲队列长度
		unsigned int m_frame_cache;
	
	public:

		std::vector<rtp_packet_block *> m_queue;

		boost::mutex m_mutex;
	};
}

#endif
