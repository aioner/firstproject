#ifndef _COMMON_MSG_SERIALIZATION_IMPL_H_INCLUED
#define _COMMON_MSG_SERIALIZATION_IMPL_H_INCLUED

#include "msg_serialization.h"
#include <cstdlib>

namespace udp_session
{
    //不考虑大小端的序列化
    class msg_serialization_impl : public msg_serialization_t
    {
    public:
        bool serialize_request(const msg_request_t *request, uint8_t *buf, uint32_t& len);
        bool serialize_response(const msg_response_t *response, uint8_t *buf, uint32_t& len);
        bool deserialize_request(const uint8_t *buf, uint32_t len, msg_request_t *request, uint32_t& request_len);
        bool deserialize_response(const uint8_t *buf, uint32_t len, msg_response_t *response, uint32_t& response_len);

        static std::size_t get_msg_base_size(const msg_header_t *msg, bool is_request = true);
    };
}

#endif //_COMMON_MSG_SERIALIZATION_IMPL_H_INCLUED
