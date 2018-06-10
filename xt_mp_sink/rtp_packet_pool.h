#ifndef RTP_PACKET_POOL_H
#define RTP_PACKET_POOL_H

#include "rtp_packet_block.h"


namespace xt_mp_sink
{
	class rtp_packet_pool : public tghelper::recycle_pool
	{
	public:

		rtp_packet_pool(uint32_t block_size,
						uint32_t expand_size,
						uint32_t init_blocks)
						: m_block_size(block_size)
						, m_expand_size(expand_size)
		{
			build_block(init_blocks);
		}

		virtual ~rtp_packet_pool(){}

	public:

		inline uint32_t get_block_size() { return m_block_size; }
		inline uint32_t get_expand_size(){ return m_expand_size; }

		void build_block(uint32_t block_num);

		//��Ȼ���䣬����ת����İ汾
		inline rtp_packet_block * alloc(bool bTrace = false)
		{ return static_cast<rtp_packet_block *>(alloc_item(bTrace));}

		//��Ƭ���㺯��--����0��ʾ����
		uint32_t calc_slice_nums(uint32_t src_size, uint32_t slice_size, uint32_t slice_offest);

	protected:

		//����һ���ڵ㣬�ڵ�洢�ռ��ڲ�����
		void add_item(tghelper::recycle_pool_item *item){ tghelper::recycle_pool::add_item(item); }
		//����һ���ڵ�
		tghelper::recycle_pool_item * alloc_item(bool bTrace = false) { return tghelper::recycle_pool::alloc_item(bTrace); }
		//���ټ��벢����ڵ�
		tghelper::recycle_pool_item * add_alloc_fast_item(tghelper::recycle_pool_item *item, bool bTrace = false)
		{ return tghelper::recycle_pool::add_alloc_fast_item(item, bTrace);	}

	private:

		uint32_t	m_block_size;
		uint32_t	m_expand_size;
	};
}

#endif
