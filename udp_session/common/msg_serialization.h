#ifndef _COMMON_MSG_SERIALIZATION_H_INCLUED
#define _COMMON_MSG_SERIALIZATION_H_INCLUED

#include "msg.h"

namespace udp_session
{
    class msg_serialization_t
    {
    public:
        virtual bool serialize_request(const msg_request_t *request, uint8_t *buf, uint32_t& len) = 0;
        virtual bool serialize_response(const msg_response_t *request, uint8_t *buf, uint32_t& len) = 0;
        virtual bool deserialize_request(const uint8_t *buf, uint32_t len, msg_request_t *request, uint32_t& request_len) = 0;
        virtual bool deserialize_response(const uint8_t *buf, uint32_t len, msg_response_t *response, uint32_t& response_len) = 0;

    protected:
        virtual ~msg_serialization_t() {}
    };
}


#endif //_COMMON_MSG_SERIALIZATION_H_INCLUED
