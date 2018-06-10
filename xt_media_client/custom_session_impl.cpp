#include "custom_session_impl.h"
#include <string.h>

namespace xt_media_client
{
    custom_session_impl::custom_session_impl(const xt_custom_session_callback_t *session_callbacks, void *ctx)
        :callbacks_(*session_callbacks),
        ctx_(ctx)
    {}

    custom_session_impl::~custom_session_impl()
    {
        (void)memset(&callbacks_, 0, sizeof(xt_custom_session_callback_t));
        ctx_ = NULL;
    }

    xt_media_client_status_t custom_session_impl::get_server_info(xt_session_server_info_t& server_info)
    {
        if (NULL == callbacks_.get_server_info_callback)
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        return callbacks_.get_server_info_callback(ctx_, &server_info);
    }

    xt_media_client_status_t custom_session_impl::parse_sdp(const std::string& sdp, std::vector<xt_sdp_media_info_t>&sdp_media_infos)
    {
        if (NULL == callbacks_.parse_sdp_callback)
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        uint16_t num = MEDIA_CLIENT_STREAM_NUM;
        sdp_media_infos.resize(num);
        xt_media_client_status_t stat = callbacks_.parse_sdp_callback(ctx_, sdp.c_str(), (uint32_t)sdp.length(), &(sdp_media_infos[0]), &num);
        if (MEDIA_CLIENT_STATUS_OK == stat)
        {
            sdp_media_infos.resize(num);
        }
        else
        {
            sdp_media_infos.clear();
        }

        return stat;
    }

    xt_media_client_status_t custom_session_impl::describe(std::string& sdp)
    {
        if (NULL == callbacks_.describe_callback)
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        uint32_t length = 2048;
        sdp.resize(length);
        xt_media_client_status_t stat = callbacks_.describe_callback(ctx_, &(sdp[0]), &length);
        if (MEDIA_CLIENT_STATUS_OK == stat)
        {
            sdp.resize(length);
        }
        else
        {
            sdp.clear();
        }

        return stat;
    }

    xt_media_client_status_t custom_session_impl::setup(std::vector<xt_session_param_t>& params)
    {
        if (NULL == callbacks_.setup_callback)
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        return callbacks_.setup_callback(ctx_, &(params[0]), (uint16_t)params.size());
    }

    xt_media_client_status_t custom_session_impl::play(double npt, float scale, uint32_t *seq, uint32_t *timestamp)
    {
        if (NULL == callbacks_.play_callback)
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        return callbacks_.play_callback(ctx_, npt, scale, seq, timestamp);
    }

    xt_media_client_status_t custom_session_impl::pause()
    {
        if (NULL == callbacks_.pause_callback)
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        return callbacks_.pause_callback(ctx_);
    }

    xt_media_client_status_t custom_session_impl::describe_and_setup(std::vector<xt_session_param_t>& params, std::string& sdp)
    {
        if (NULL == callbacks_.describe_and_setup_callback)
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        uint32_t length = 2048;
        sdp.resize(length);
        xt_media_client_status_t stat = callbacks_.describe_and_setup_callback(ctx_, &(params[0]), (uint16_t)params.size(), &(sdp[0]), &length);
        if (MEDIA_CLIENT_STATUS_OK == stat)
        {
            sdp.resize(length);
        }
        else
        {
            sdp.clear();
        }

        return stat;
    }

    xt_media_client_status_t custom_session_impl::teardown()
    {
        if (NULL == callbacks_.teardown_callback)
        {
            return MEDIA_CLIENT_STATUS_NOT_SUPPORTED;
        }

        return callbacks_.teardown_callback(ctx_);
    }
}
