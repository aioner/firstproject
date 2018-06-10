#ifndef _MEDIA_SESSION_H_INCLUDED
#define _MEDIA_SESSION_H_INCLUDED

#include "xt_media_client_types.h"
#include "novtable.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace xt_media_client
{
    class MEDIA_CLIENT_NO_VTABLE media_session_t
    {
    public:
        virtual xt_media_client_status_t get_server_info(xt_session_server_info_t& server_info) { return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }
        virtual xt_media_client_status_t parse_sdp(const std::string& sdp, std::vector<xt_sdp_media_info_t>&sdp_media_infos)  { return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }
        virtual xt_media_client_status_t describe(std::string& sdp){ return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }
        virtual xt_media_client_status_t setup(std::vector<xt_session_param_t>& params) { return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }
        virtual xt_media_client_status_t play(double npt = .0, float scale = 1.0f, uint32_t *seq = NULL, uint32_t *timestamp = NULL){ return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }
        virtual xt_media_client_status_t pause() { return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }
        virtual xt_media_client_status_t describe_and_setup(std::vector<xt_session_param_t>& params, std::string& sdp) { return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }
        virtual xt_media_client_status_t teardown() { return MEDIA_CLIENT_STATUS_NOT_SUPPORTED; }

    protected:
        virtual ~media_session_t() {}
    };

    typedef boost::shared_ptr<media_session_t> media_session_ptr;
}

#endif //_MEDIA_SESSION_H_INCLUDED
