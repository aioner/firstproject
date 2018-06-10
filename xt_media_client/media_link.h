#ifndef _MEDIA_LINK_H_INCLUDED
#define _MEDIA_LINK_H_INCLUDED

#include "xt_media_client.h"
#include "novtable.h"
#include "utility/singleton.h"
#include "thread.h"

#include <boost/thread/sync_queue.hpp>
#include <vector>
#include <string>

namespace xt_media_client
{
    class MEDIA_CLIENT_NO_VTABLE media_link_t
    {
    public:
        virtual xt_media_client_status_t get_header(uint8_t *data, uint32_t *length) = 0;
        virtual xt_media_client_status_t play(xt_media_client_frame_callback_t cb, void *ctx) = 0;
        virtual xt_media_client_status_t close() = 0;
        virtual xt_media_client_status_t pause() { return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }
        virtual xt_media_client_status_t seek(double npt, float scale, uint32_t *seq, uint32_t *timestamp) { return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }
        virtual xt_media_client_status_t query_prof_info(xt_rtp_prof_info_t *prof) { return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }
        virtual xt_media_client_status_t get_ports_info(std::vector<xt_sink_info_t>& ports_info) = 0; 
        virtual xt_media_client_status_t rtcp_send_fir() = 0;
        virtual xt_media_client_status_t set_sdp(const std::string& sdp) = 0;
		virtual xt_media_client_status_t register_rtcp_callback(xt_media_client_rtcp_report_callback_t cb, void *ctx) = 0;
        virtual xt_media_client_status_t register_no_frame_arrived_callback(uint32_t priod, xt_media_client_no_frame_arrived_callback_t cb, void *ctx) = 0;
        virtual ~media_link_t() {}
    };

    class media_link_factory : public xt_utility::singleton<media_link_factory>, public repeat_thread
    {
    public:
        static xt_media_client_status_t create_link(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports,xt_media_link_handle_t *phandle);
        static xt_media_client_status_t create_link(const char *uri, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle,const char *localip);
        static xt_media_client_status_t create_link(const char *ip, uint16_t port, uint32_t channel, uint32_t link_type, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle);
        static xt_media_client_status_t create_link(void *sock, uint32_t channel, uint32_t link_type, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle);
        static xt_media_client_status_t create_link(const xt_custom_session_callback_t *session, void *ctx, xt_media_client_link_mode mode, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle);
        static xt_media_client_status_t create_link(int track_num, bool demux, xt_media_link_handle_t *phandle);
        static xt_media_client_status_t create_link(const char *ip, uint16_t port, uint32_t channel, uint32_t link_type, uint32_t media_type, const char *multicast_ip, uint16_t multicast_port, xt_media_link_handle_t *phandle);
		static xt_media_client_status_t create_link(const char *uri, uint32_t media_type, bool demux,const char *multicast_ip, uint16_t multicast_port, xt_media_link_handle_t *phandle);
        static media_link_t *query_link(xt_media_link_handle_t handle);
        static bool destory_link(xt_media_link_handle_t handle);

        void init();
        ~media_link_factory();

    private:
        void push_close_link(media_link_t *link);
        void term();

        void on_repeat();
        boost::sync_queue<media_link_t *> link_close_queue_;
    };
}

#endif //_MEDIA_LINK_IMPL_H_INCLUDED

