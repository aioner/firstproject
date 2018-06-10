#ifndef PACKET_SINK_H
#define PACKET_SINK_H

#include "rtp_macro_block.h"

namespace xt_mp_sink
{
	class rtp_packet_sink : private boost::noncopyable
	{
	public:
		rtp_packet_sink(uint32_t max_size, bool overlapped)
			: m_max_size(max_size)
			, m_overlapped(overlapped){}
		virtual ~rtp_packet_sink(){ clear(); }
	public:
		bool push(rtp_macro_block *item);
		int try_push(rtp_macro_block *item);
		rtp_macro_block * front();
		rtp_macro_block * pop(bool bRelease = false);
		rtp_macro_block * try_pop(bool &lock_flag, bool bRelease = false);
		void clear();
		uint32_t size();
		uint32_t empty();
		uint32_t front_data_size();
		uint32_t front_real_size();

	private:

		bool _push(rtp_macro_block *item);
		rtp_macro_block * _pop(bool bRelease);

	public:
		std::vector<rtp_macro_block *> m_queue;
		boost::mutex m_mutex;
		uint32_t m_max_size;
		bool m_overlapped;
	};
}

#endif