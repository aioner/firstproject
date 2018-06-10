#ifndef RTP_MACRO_BLOCK_H
#define RTP_MACRO_BLOCK_H

#include "rtp_packet_block.h"

namespace xt_mp_sink
{

	class rtp_macro_block : public tghelper::byte_macro_block
	{
	public:

		rtp_macro_block(void){}
		virtual ~rtp_macro_block(void){}

	public:

		uint32_t get_timestamp() const { return _timestamp; }
		void set_timestamp(uint32_t val) { _timestamp = val; }

		uint32_t get_payload_type() const { return _payload_type; }
		void set_payload_type(uint32_t val) { _payload_type = val; }

		uint32_t get_ssrc() const { return _ssrc; }
		void set_ssrc(uint32_t val) { _ssrc = val; }

		uint32_t get_data_size() const { return _data_size; }
		void set_data_size(uint32_t val) { _data_size = val; }

		uint32_t get_real_size() const { return _real_size; }
		void set_real_size(uint32_t val) { _real_size = val; }

		rtp_packet_block * get_first_block()
		{
			return static_cast<rtp_packet_block *>(get_container().front());
		}

		bool get_last_param(rv_rtp_param * param)
		{
			if(get_container().empty()) return false;
			rtp_packet_block * last_item = static_cast<rtp_packet_block *>(get_container().back());
			rv_rtp_param * _param = last_item->get_head();
			memcpy(param,_param,sizeof(rv_rtp_param));
			return true;
		}

		uint16_t get_last_sn() const { return _last_sn; }
		void set_last_sn(uint32_t val) { _last_sn = val; }

		uint32_t get_last_ts() const { return _last_ts; }
		void set_last_ts(uint32_t val) { _last_ts = val; }

		uint8_t get_last_marker() const { return _last_marker; }
		void set_last_marker(uint8_t val) { _last_marker = val; } 

	private:

		uint32_t _timestamp;
		uint32_t _marker;
		uint32_t _payload_type;
		uint32_t _ssrc;
		uint32_t _offset;
		uint32_t _data_size;
		uint32_t _real_size;
		uint16_t _last_sn;
		uint32_t _last_ts;
		uint8_t	 _last_marker;
	};
}

#endif