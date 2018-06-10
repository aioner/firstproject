#ifndef _MEDIA_LINK_IMPL_H_INCLUDED
#define _MEDIA_LINK_IMPL_H_INCLUDED

#include "media_session.h"
#include "rtp_sink.h"
#include "ports_mgr.h"
#include "ports_mgr_helper.h"
#include "media_link.h"
#include "spinlock.h"
#include "thread_timer.h"
#include "msec_timer.h"

#include <string>
#include <vector>

#define RTP_BPS_CALA_PRIOD              500

namespace xt_media_client{
    class rtcp_report_callback_impl : public rtcp_report_callback_t
    {
    public:
        rtcp_report_callback_impl();
        ~rtcp_report_callback_impl();

        void register_rtcp_callback(xt_media_client_rtcp_report_callback_t cb, void *ctx);

    private:
        void on_rtcp_receive(uint32_t ssrc, const xt_media_client_rtcp_sr_t *sr);
        void on_rtcp_send(uint32_t ssrc, const xt_media_client_rtcp_rr_t *rr);

        xt_media_client_rtcp_report_callback_t cb_;
        void *ctx_;
    };

	//class deadline_timer_callback_t
	//{
	//public:
	//	virtual uint32_t on_expires() = 0;

	//protected:
	//	virtual ~deadline_timer_callback_t() {}
	//};

    class no_frame_arrived_callback_impl : public deadline_timer_callback_t
	{
        public:
            no_frame_arrived_callback_impl();
            ~no_frame_arrived_callback_impl();

            void register_no_frame_arrived_callback(uint32_t priod, xt_media_client_no_frame_arrived_callback_t cb, void *ctx);
        protected:
            void update_frame_arrived_ts();
        private:
            uint32_t on_expires();

            xt_media_client_no_frame_arrived_callback_t cb_;
            void *ctx_;
            volatile uint32_t priod_;
            volatile uint32_t ts_frame_arrived_;
        };

    class media_link_impl_base
    {
    public:
        media_link_impl_base(ports_mgr_t *ports_mgr);

        ~media_link_impl_base();

        void register_session(const media_session_ptr& session)
        {
            session_ = session;
        }

        const media_session_ptr&get_session() const
        {
            return session_;
        }

        xt_media_client_status_t create_link(xt_media_client_link_mode mode, uint32_t media_type, bool demux, const char *multicast_ip = NULL, uint16_t multicast_port = 0,const char* localip="0.0.0.0");
        xt_media_client_status_t create_link(xt_media_client_link_mode mode, int track_num, bool demux);
        xt_media_client_status_t create_link(xt_media_client_link_mode mode,int track_num, bool demux, bool multicast, const char* multicastip, int* multiports);
        xt_media_client_status_t create_link(const char *multicast_ip, uint16_t multicast_port);

        void close_link();
        xt_media_client_status_t start_capture(frame_data_dump_callback_t *cb);
        const std::string& get_sdp() const;

        xt_media_client_status_t set_sdp(const std::string& sdp);

        void get_port(std::vector<xt_sink_info_t> &infos);

        //// rtcp-based feedback message I Ö¡²¶»ñ
        xt_media_client_status_t rtcp_send_fir();

        static void s_rtp_demux_handler(void *hdemux,void *ctx);
		xt_media_client_status_t register_rtcp_callback(rtcp_report_callback_t *cb);
        static void rtp_demux_handler(void *hdemux);

        static void init();
        static void uninit();

	private:
		bool is_demux();

    public:
        virtual rtp_sink_ptr new_rtp_sink(xt_av_media_t media_type, xt_av_codec_id_t cid);
        virtual bool filter_media_streams(uint32_t media_type, std::vector<xt_sdp_media_info_t>&media_infos) { return true; }

        xt_media_client_status_t std_create_link(uint32_t media_type, bool demux,const char* localip);
        xt_media_client_status_t std_multi_create_link(uint32_t media_type, bool demux,const char *multicast_ip = NULL, uint16_t multicast_port = 0);
        xt_media_client_status_t priv_create_link(uint32_t media_type, bool demux);
        xt_media_client_status_t priv_create_link2(uint32_t media_type, bool demux, const char *multicast_ip = NULL, uint16_t multicast_port = 0);
        xt_media_client_status_t create_recv(int track_num, bool demux);// push model
        xt_media_client_status_t create_recv(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports);
        xt_media_client_status_t set_packer(const std::string& sdp);// push model
        xt_media_client_status_t set_packer(std::size_t index, int payload, const rtp_unpack_ptr &packer);// push model
        bool add_remote_address(std::size_t index,const char *ip, uint16_t rtp_port, uint16_t rtcp_port, bool demux, uint32_t demuxid);

        bool stream_setup(const char* localip,xt_av_media_t media_type, xt_av_codec_id_t cid, bool demux, bool with_describe = false, 
			const char *multicast_ip = NULL, uint16_t multicast_port = 0,uint32_t code_type = 0);
        bool streams_setup(const char* localip,const std::vector<xt_sdp_media_info_t>& sdp_media_infos, bool demux, bool with_describe = false);
		bool stream_multi_setup(xt_av_media_t media_type, xt_av_codec_id_t cid, bool demux, bool with_describe = false, const char *multicast_ip = NULL, uint16_t multicast_port = 0);
        bool streams_multi_setup(const std::vector<xt_sdp_media_info_t>& sdp_media_infos, bool demux, bool with_describe = false,const char *multicast_ip = NULL, uint16_t multicast_port = 0);
        bool session_setup(std::vector<xt_session_param_t>&params, bool with_describe = false);

        void add_rtp_sink(const rtp_sink_ptr&rtp_sink);
        void active_rtp_sinks(frame_data_dump_callback_t *cb);
        void close_rtp_sinks();
        bool rtp_sinks_empty() const;

        media_session_ptr session_;

        std::size_t get_rtp_sink_size()const;
        ports_mgr_helper<ports_mgr_t> ports_mgr_;
        std::string sdp_;
        std::vector<rtp_sink_ptr> rtp_sinks_;

        mutable spinlock_t sdp_mutex_;
        mutable spinlock_t rtp_sinks_mutex_;
    };//class media_link_impl_base

    class media_link_impl : public media_link_t, public frame_data_dump_callback_t, public media_link_impl_base, protected rtcp_report_callback_impl, private no_frame_arrived_callback_impl
    {
    public:
        explicit media_link_impl(ports_mgr_t *ports_mgr);

        xt_media_client_status_t get_header(uint8_t *data, uint32_t *length);
        xt_media_client_status_t play(xt_media_client_frame_callback_t cb, void *ctx);
        xt_media_client_status_t close();
        xt_media_client_status_t pause();
        xt_media_client_status_t seek(double npt, float scale, uint32_t *seq, uint32_t *timestamp);
        xt_media_client_status_t get_ports_info(std::vector<xt_sink_info_t>& ports_info);
        xt_media_client_status_t rtcp_send_fir();
        xt_media_client_status_t set_sdp(const std::string& sdp);
		xt_media_client_status_t register_rtcp_callback(xt_media_client_rtcp_report_callback_t cb, void *ctx);
        xt_media_client_status_t register_no_frame_arrived_callback(uint32_t priod, xt_media_client_no_frame_arrived_callback_t cb, void *ctx);
        void on_frame_dump(void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc);
    protected:
        xt_media_client_frame_callback_t cb_;
        void *ctx_;
        spinlock_t cb_mtx_;

    };//class media_link_impl

    class media_link_impl2 : public media_link_impl, public thread_timer
    {
    public:
        explicit media_link_impl2(ports_mgr_t *ports_mgr);
        ~media_link_impl2();

        xt_media_client_status_t query_prof_info(xt_rtp_prof_info_t *prof);
        xt_media_client_status_t play(xt_media_client_frame_callback_t cb, void *ctx);
    protected:
        void on_timer();

        uint64_t prev_rtp_total_bytes_;
        uint64_t last_rtp_total_bytes_;
        //rtcp_recv_report_t last_rtcp_rr_;
    };

    class media_link_ref_impl_t : public frame_data_dump_callback_t, public media_link_impl_base
    {
    public:
        explicit media_link_ref_impl_t(ports_mgr_t *ports_mgr);
        void on_frame_dump(void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc);
        void register_frame_dump_observer(frame_data_dump_callback_t *cb);
        void unregister_frame_dump_observer(frame_data_dump_callback_t *cb);
        bool empty() const;

        xt_media_client_status_t get_header(uint8_t *data, uint32_t *length);
        xt_media_client_status_t play();
        xt_media_client_status_t close();
        xt_media_client_status_t pause();
        xt_media_client_status_t seek(double npt, float scale, uint32_t *seq, uint32_t *timestamp);
    private:
        typedef std::vector<frame_data_dump_callback_t *> frame_cb_container_t;
        frame_cb_container_t obs_;
        mutable spinlock_t mutex_;
        typedef spinlock_t::scoped_lock scoped_lock;
    };

    class  media_link_ref_t : public media_link_t, public frame_data_dump_callback_t, protected rtcp_report_callback_impl, private no_frame_arrived_callback_impl
    {
    public:
        media_link_ref_t();
        ~media_link_ref_t();

        void init(media_link_ref_impl_t *impl, uint32_t stream_type);

        xt_media_client_status_t get_header(uint8_t *data, uint32_t *length);
        xt_media_client_status_t play(xt_media_client_frame_callback_t cb, void *ctx);
        xt_media_client_status_t close();
        xt_media_client_status_t pause();
        xt_media_client_status_t seek(double npt, float scale, uint32_t *seq, uint32_t *timestamp);
        xt_media_client_status_t get_ports_info(std::vector<xt_sink_info_t>& ports_info);
        xt_media_client_status_t rtcp_send_fir();
        xt_media_client_status_t set_sdp(const std::string& sdp);
		xt_media_client_status_t register_rtcp_callback(xt_media_client_rtcp_report_callback_t cb, void *ctx);
        xt_media_client_status_t register_no_frame_arrived_callback(uint32_t priod, xt_media_client_no_frame_arrived_callback_t cb, void *ctx);

        void on_frame_dump(void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc);
    protected:
        media_link_ref_impl_t *impl_;
        uint32_t stream_type_;
        xt_media_client_frame_callback_t cb_;
        void *ctx_;
        spinlock_t cb_mtx_;
    };
}

#endif //_MEDIA_LINK_IMPL_H_INCLUDED

