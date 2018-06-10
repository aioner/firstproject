#ifndef _RTP_SINK_IMPL_H_INCLUDED
#define _RTP_SINK_IMPL_H_INCLUDED

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtp_sink.h"
#include "xt_mp_sink_api.h"
#include "rtp_unpack.h"
#include "utility/singleton.h"
#include "rtp_buf.h"
#include "spinlock.h"

#include <boost/unordered_map.hpp>
namespace xt_media_client
{
	class rtp_sink_impl;
    class xt_mp_sink_library : public xt_utility::singleton<xt_mp_sink_library>
    {
    public:
		xt_mp_sink_library()
			:rtp_to_sink_map_mutex_(),
			rtp_to_sink_map_()
		{}
        ~xt_mp_sink_library()
        {
            term();
        }

        bool init(uint16_t rtp_rv_thread_num, uint16_t rtp_sink_thread_num, uint16_t rtp_post_thread_num);
		bool add_rtp_sink(rv_rtp rtpH, rtp_sink_impl *sink);
		bool del_rtp_sink(rv_rtp rtpH);
		rtp_sink_impl *get_rtp_sink(rv_rtp rtpH);
    private:
        void term();
		typedef boost::unordered_map<rv_rtp, rtp_sink_impl *> rtp_to_sink_map_t;
		
		rtp_to_sink_map_t rtp_to_sink_map_;

	public:
		spinlock_t rtp_to_sink_map_mutex_;
    };
	class payload_to_unpacker_t
	{
	public:
		bool add_unpacker(int payload, const rtp_unpack_ptr& unpacker);
		rtp_unpack_ptr get_unpacker(int payload);
		bool empty() const;
		void register_frame_dump_callback(frame_data_dump_callback_t *cb);
	private:
		mutable spinlock_t mutex_;
		typedef boost::unordered_map<int, rtp_unpack_ptr> unpacker_map_t;
		unpacker_map_t unpackers_;
	};

    class rtp_sink_impl : public rtp_sink_t
    {
    public:
        rtp_sink_impl(uint32_t rtp_frame_bytes, uint32_t rtp_frame_max_bytes);
        explicit rtp_sink_impl(const rtp_unpack_ptr&packer);
        ~rtp_sink_impl();

        bool open_rtp(const char *ip, uint16_t rtp_port, uint16_t rtcp_port,open_rtp_mode open_mode, bool demux, uint32_t& demuxid, const multicast_param_t *multicast, rv_context demux_handler);
        void active_rtp(frame_data_dump_callback_t *cb);
        void close_rtp();
        bool add_remote_address(const char *ip, uint16_t rtp_port, uint16_t rtcp_port, bool demux, uint32_t demuxid);
        //bool get_rtcp_rr(rtcp_recv_report_t *rr) const;
        bool get_rtp_prof_info(rtp_prof_info_t *rpi) const;
        uint32_t get_ssrc() const;

		void add_unpacker(int payload, const rtp_unpack_ptr &unpacker);

		long rtcp_send_fir();

		bool is_demux();
		void register_rtcp_callback(rtcp_report_callback_t *cb);

		msink_handle* get_handle();
		static int pump_demux_rtp(rtp_sink_impl *sink, void *buf, uint32_t len, const rv_rtp_param &p, const rv_net_address &address);

    private:
        bool is_open() const;

        static void s_rtp_frame_handler(mp_handle hmp, void* ctx);
        static void s_rtcp_report_handler(uint32_t, uint32_t, uint32_t, void *); 
		static void s_rtp_raw_handler(mp_handle hmp, void* ctx);
        static void s_report_receive_handler(const rtcp_send_report *sr, void *context);
        static void s_report_send_handler(const rtcp_receive_report *rr, void *context);
        
		void rtp_frame_handler(mp_handle hmp);
	public:
        void rtp_raw_handler(mp_handle hmp);
        void report_receive_handler(const rtcp_send_report *sr);
        void report_send_handler(const rtcp_receive_report *rr);

        bool add_rtp_remote_address(const char *ip, uint16_t port, bool demux, uint32_t demuxid);
        bool add_rtcp_remote_address(const char *ip, uint16_t port, bool demux, uint32_t demuxid);

        msink_handle handle_;

        frame_data_dump_callback_t *cb_;
		payload_to_unpacker_t payload_to_unpacker_;

        std::auto_ptr<rtp_buf_t> rtp_frame_buf_;
        bool is_open_;
		bool is_multicast_;
		bool is_demux_;

        uint64_t rtp_total_bytes_;
		rtcp_report_callback_t *rtcp_cb_;
    };

}

#endif //_RTP_SINK_IMPL_H_INCLUDED
