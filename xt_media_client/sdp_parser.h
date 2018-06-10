#ifndef _SDP_PARSER_H_INCLUDED
#define _SDP_PARSER_H_INCLUDED

#include "xt_media_client_types.h"
#include "utility/singleton.h"

#include <vector>
#include <stdint.h>

namespace xt_media_client
{
    class sdp_parser_library : public xt_utility::singleton<sdp_parser_library>
    {
    public:
        bool init();
        void *parse(const char *sdp, int len);
        bool encode(void *msg, char *sdp, int& len);

        ~sdp_parser_library()
        {
            term();
        }
    private:
        void term();
    };

    class sdp_parser_t
    {
    public:
        static bool parse(const char *uri, const char *sdp, uint32_t length, std::vector<xt_sdp_media_info_t>& media_infos);
        static bool parse_sip(const char *uri, const char *sdp, uint32_t length, std::vector<xt_sdp_media_info_t>& media_infos);
        static bool remove_on_pure_audio(char *sdp, int& len);
    };
}

#endif //_SDP_PARSER_H_INCLUDED
