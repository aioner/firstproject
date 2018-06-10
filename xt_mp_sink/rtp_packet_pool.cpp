//#include "stdafx.h"
#include "rtp_packet_pool.h"

namespace xt_mp_sink
{
	void rtp_packet_pool::build_block(uint32_t block_num)
	{
		for(uint32_t i=0;i<block_num;i++)
			add_item(new rtp_packet_block(m_block_size));
	}

	uint32_t rtp_packet_pool::calc_slice_nums( uint32_t src_size, uint32_t slice_size, uint32_t slice_offest )
	{
		if (get_block_size() < (slice_size + slice_offest)) return 0;

		uint32_t payload_size = slice_size;
		uint32_t block_nums = src_size / payload_size;
		if ((src_size % payload_size) > 0) block_nums++;
		return block_nums;
	}

}