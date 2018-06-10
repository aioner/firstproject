#ifndef _CUSTOM_SESSION_IMPL_H_INCLUDED
#define _CUSTOM_SESSION_IMPL_H_INCLUDED

#include "media_session.h"
#include "xt_media_client_types.h"

namespace xt_media_client
{
    class custom_session_impl : public media_session_t
    {
    public:
        custom_session_impl(const xt_custom_session_callback_t *session_callbacks, void *ctx);
        ~custom_session_impl();

        xt_media_client_status_t get_server_info(xt_session_server_info_t& server_info);
        xt_media_client_status_t parse_sdp(const std::string& sdp, std::vector<xt_sdp_media_info_t>&sdp_media_infos);
        xt_media_client_status_t describe(std::string& sdp);
        xt_media_client_status_t setup(std::vector<xt_session_param_t>& params);
        xt_media_client_status_t play(double npt, float scale, uint32_t *seq, uint32_t *timestamp);
        xt_media_client_status_t pause();
        xt_media_client_status_t describe_and_setup(std::vector<xt_session_param_t>& params, std::string& sdp);
        xt_media_client_status_t teardown();

    private:
        xt_custom_session_callback_t callbacks_;
        void *ctx_;
    };
}

#endif //_CUSTOM_SESSION_IMPL_H_INCLUDED
