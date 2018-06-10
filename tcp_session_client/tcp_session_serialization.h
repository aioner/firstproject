#ifndef _TCP_SESSION_SERIALIZATION_H_INCLUDED
#define _TCP_SESSION_SERIALIZATION_H_INCLUDED

#include "tcp_session_msg.h"

#include <string.h>

namespace tcp_session
{
    template<typename RequestT>
    inline bool serialize_request(const RequestT& request, std::pair<const void *, uint32_t> &out, std::pair<void *, uint32_t> *in = NULL)
    {
        out.first = &request;
        out.second = sizeof(RequestT);
        return true;
    }

    template<typename ResponseT>
    bool deserialize_response(const tcp_session::msg_header_t& header, char *body, ResponseT& response);

    inline bool deserialize_response(const tcp_session::msg_header_t& header, char *body, multicast_info_t& multicast_info)
    {
        if (TCP_SESSION_OPID_MULTICAST_INFO != header.op_id)
        {
            return false;
        }

        if (header.length() < sizeof(multicast_info_t))
        {
            return false;
        }

        memcpy(&multicast_info, body, sizeof(multicast_info_t));

        return true;
    }

    inline bool deserialize_response(const tcp_session::msg_header_t& header, char *body, sdp_info_t& sdp)
    {
        if (TCP_SESSION_OPID_KEY_INFO == header.op_id)
        {
            if (header.length() > sizeof(sdp.sdp))
            {
                return false;
            }

            sdp.length = header.length();
            memcpy(&sdp.sdp, body, sdp.length);
            sdp.multiplex = 0;
            sdp.multiplexID = 0;
            sdp.stop_flag = header.flag;
            sdp.rtp_port = header.flag;

            sdp.data_type = header.data_type;
        }
        else if (TCP_SESSION_OPID_KEY_INFO2 == header.op_id)
        {
            if (header.length() > sizeof(sdp.sdp) + sizeof(uint32_t) * 2)
            {
                return false;
            }

            sdp.length = header.length() - 8;
            memcpy(&sdp.sdp, body, sdp.length);
            memcpy(&sdp.multiplex, body + sdp.length, sizeof(uint32_t));
            memcpy(&sdp.multiplexID, body + sdp.length + sizeof(uint32_t), sizeof(uint32_t));
            sdp.stop_flag = header.flag;
            sdp.data_type = header.data_type;

            if (0 != sdp.multiplex)
            {
                sdp.rtp_port = sdp.rtp_port;
            }
            else
            {
                sdp.rtp_port = 0;
            }
        }
        else
        {
            return false;
        }

        return true;
    }

    inline bool deserialize_response(const tcp_session::msg_header_t& header, char *body, stop_response_msg_t& response)
    {
        if (TCP_SESSION_OPID_STOP != header.op_id)
        {
            return false;
        }

        if (header.length() < sizeof(stop_response_msg_t))
        {
            return false;
        }

        memcpy(&response, body, sizeof(stop_response_msg_t));

        return true;
    }
};

#endif //_TCP_SESSION_SERIALIZATION_H_INCLUDED
