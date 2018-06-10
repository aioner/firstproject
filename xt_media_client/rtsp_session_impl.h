#ifndef _RTSP_SESSION_IMPL_H_INCLUDED
#define _RTSP_SESSION_IMPL_H_INCLUDED

#include "media_session.h"
#include "xt_rtsp_client/xt_rtsp_client.h"
#include "xt_media_client_types.h"
#include "utility/singleton.h"

#include <string>
#include <vector>

#define RTSP_CONNECTION_MAX                 512
#define RTSP_SESSION_RESPONSE_TIMEOUT       5000
#define RTSP_SESSION_PING_TIMEOUT           3000
#define RTSP_CONNECTION_DNS_MAX_RESULTS     512
#define RTSP_CONNECTION_MAX_HEADER_INMSG    512
#define RTSP_CONNECTION_MAX_SESSION         1024
#define RTSP_CONNECTION_MAX_URLS_INMSG      64
#define RTSP_CONNECTION_TRANSMIT_QUEUE_SIZE 512
#define RTSP_CONNECTION_WAIT_DESCRIBE_REQ   512

namespace xt_media_client
{
    class rtsp_session_library : public xt_utility::singleton<rtsp_session_library>
    {
    public:
        rtsp_session_library()
            :rtsp_session_handle_(NULL),
            connect_timeout_(0),
            describe_timeout_(0),
            setup_timeout_(0),
            play_timeout_(0),
            pause_timeout_(0),
            teardown_timeout_(0)
        {}

        ~rtsp_session_library()
        {
            term();
        }

        bool init(uint16_t connect_timeout, uint16_t describe_timeout, uint16_t setup_timeout, uint16_t play_timeout, uint16_t pause_timeout, uint16_t teardown_timeout,xt_rtsp_client_log_cb_t cb);

        rtsp_client_handle_t get_handle() { return rtsp_session_handle_; }
        uint16_t get_connect_timeout() const { return connect_timeout_; }
        uint16_t get_describe_timeout() const { return describe_timeout_; }
        uint16_t get_setup_timeout() const { return setup_timeout_; }
        uint16_t get_play_timeout() const { return play_timeout_; }
        uint16_t get_pause_timeout() const { return pause_timeout_; }
        uint16_t get_teardown_timeout() const {  return teardown_timeout_; }
    private:
        void term();

        rtsp_client_handle_t rtsp_session_handle_;

        uint16_t connect_timeout_;
        uint16_t describe_timeout_;
        uint16_t setup_timeout_;
        uint16_t play_timeout_;
        uint16_t pause_timeout_;
        uint16_t teardown_timeout_;
    };

    class rtsp_session_impl : public media_session_t
    {
    public:
        rtsp_session_impl();
        ~rtsp_session_impl();

        bool connect(const char *uri,const char* localip="0.0.0.0");

        xt_media_client_status_t get_server_info(xt_session_server_info_t& server_info);
        xt_media_client_status_t parse_sdp(const std::string& sdp, std::vector<xt_sdp_media_info_t>&sdp_media_infos);
        xt_media_client_status_t describe(std::string& sdp);
        xt_media_client_status_t setup(std::vector<xt_session_param_t>& params);
        xt_media_client_status_t play(double npt, float scale, uint32_t* seq, uint32_t* timestamp);
        xt_media_client_status_t pause();
        xt_media_client_status_t teardown();

    private:
        void close_connection();
        void close_session();

        rtsp_connection_handle_t connection_;
        std::vector<xt_sdp_media_info_t> media_infos_;;
        rtsp_session_handle_t session_;
        std::string uri_;
    };
}

#endif //_RTSP_SESSION_IMPL_H_INCLUDED
