#include "msg_serialization_impl.h"
#include "error.h"
#include <string.h>

namespace udp_session
{
    bool msg_serialization_impl::serialize_request(const msg_request_t *request, uint8_t *buf, uint32_t& len)
    {
        if (NULL == request)
        {
            return false;
        }

        std::size_t msg_need_size = get_msg_base_size(request);

        if (send_data == request->code)
        {
            msg_need_size += ((send_data_request_msg_t *)request)->length;
        }
        else if(send_regist == request->code || send_stop_regist == request->code)
        {
            msg_need_size += ((udp_session::send_regist_request_msg_t *)request)->length;
        }
    
        if (len < msg_need_size)
        {
            return false;
        }

        (void)memcpy(buf, request, msg_need_size);
        len = (uint32_t)msg_need_size;

        return true;
    }

    bool msg_serialization_impl::serialize_response(const msg_response_t *response, uint8_t *buf, uint32_t& len)
    {
        if (NULL == response)
        {
            return false;
        }

        std::size_t msg_need_size = 0;
        if (error::ok == response->error_code)
        {
            msg_need_size = get_msg_base_size(response, false);
            if (get_sdp == response->code)
            {
                msg_need_size += ((get_sdp_response_msg_t *)response)->length;
            }
            else if (get_sdp_and_play == response->code)
            {
                msg_need_size += ((get_sdp_and_play_response_msg_t *)response)->length;
            }
            else if (send_data == response->code)
            {
                msg_need_size += ((send_data_request_msg_t *)response)->length;
            }
			else if (send_regist == response->code || send_stop_regist == response->code)
			{
				msg_need_size += ((udp_session::send_regist_request_msg_t *)response)->length;
			}
        }
        else
        {
            msg_need_size = sizeof(msg_response_t);
        }

        if (len < msg_need_size)
        {
            return false;
        }

        (void)memcpy(buf, response, msg_need_size);
        len = (uint32_t)msg_need_size;

        return true;
    }

    bool msg_serialization_impl::deserialize_request(const uint8_t *buf, uint32_t len, msg_request_t *request, uint32_t& request_len)
    {
        if (NULL == request)
        {
            return false;
        }

        if (len < sizeof(msg_request_t))
        {
            return false;
        }

        const msg_request_t *header = (const msg_request_t *)buf;
        std::size_t msg_need_size = get_msg_base_size(header);

        if (send_data == header->code)
        {
            msg_need_size += ((send_data_request_msg_t *)header)->length;
        }

		if(send_regist == header->code|| send_stop_regist == header->code)
		{
			msg_need_size += ((udp_session::send_regist_request_msg_t *)header)->length;
		}

        if ((msg_need_size > len) || (msg_need_size > request_len))
        {
            return false;
        }

        (void)memcpy(request, buf, msg_need_size);
        request_len = msg_need_size;

        return true;
    }

    bool msg_serialization_impl::deserialize_response(const uint8_t *buf, uint32_t len, msg_response_t *response, uint32_t& response_len)
    {
        if (NULL == response)
        {
            return false;
        }

        if (len < sizeof(msg_response_t))
        {
            return false;
        }

        const msg_response_t *header = (const msg_response_t *)buf;

        std::size_t msg_need_size = 0;
        if (error::ok == header->error_code)
        {
            msg_need_size = get_msg_base_size(header, false);
            if (len < msg_need_size)
            {
                return false;
            }

            if (get_sdp == header->code)
            {
                msg_need_size += ((get_sdp_response_msg_t *)header)->length;
            }
            else if (get_sdp_and_play == header->code)
            {
                msg_need_size += ((get_sdp_and_play_response_msg_t *)header)->length;
            }
            else if (send_data == header->code)
            {
                msg_need_size += ((send_data_request_msg_t *)header)->length;
            }
            else if(send_regist == header->code || send_stop_regist == header->code)
            {
                msg_need_size += ((udp_session::send_regist_request_msg_t*)header)->length;
            }
        }
        else
        {
            msg_need_size = sizeof(msg_response_t);
        }
        
        if ((msg_need_size > len) || (msg_need_size > response_len))
        {
            return false;
        }

        (void)memcpy(response, buf, msg_need_size);
        response_len = msg_need_size;

        return true;
    }

    std::size_t msg_serialization_impl::get_msg_base_size(const msg_header_t *msg, bool is_request)
    {
        switch (msg->code)
        {
        case get_sdp:
            return is_request ? sizeof(get_sdp_request_msg_t) : sizeof(get_sdp_response_msg_t);
        case play:
            return is_request ? sizeof(play_request_msg_t) : sizeof(play_response_msg_t);
        case stop:
            return is_request ? sizeof(stop_request_msg_t) : sizeof(stop_response_msg_t);
        case heartbit:
            return is_request ? sizeof(heartbit_request_msg_t) : 0;
        case get_sdp_and_play:
            return is_request ? sizeof(get_sdp_and_play_request_msg_t) : sizeof(get_sdp_and_play_response_msg_t);
        case send_data:
            return is_request ? sizeof(send_data_request_msg_t) : sizeof(send_data_response_msg_t);
        case send_regist:
            return is_request ? sizeof(udp_session::send_regist_request_msg_t) : sizeof(send_regist_reponse_msg_t);
        case send_stop_regist:
            return is_request ? sizeof(udp_session::send_regist_request_msg_t) : sizeof(send_regist_reponse_msg_t);
        case heartbit2:
            return is_request ? sizeof(heartbit2_request_msg_t) : 0;
        default:
            return 0;
        }
    }

}
