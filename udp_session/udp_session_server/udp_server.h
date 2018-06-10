#ifndef _UDP_SERVER_H_INCLUDED
#define _UDP_SERVER_H_INCLUDED

#include<stdint.h>

namespace udp_session_server
{
    class udp_server_t
    {
    public:
        enum { default_receive_buffer_size = 2048 };

        virtual bool init(const char *v4_ip, uint16_t port, uint32_t size = default_receive_buffer_size) = 0;
        virtual bool send_msg(const char *peer_v4_ip, uint16_t peer_port, const uint8_t *buf, uint32_t len) = 0;

        class events_type
        {
        public:
            virtual bool on_receive_msg(const char *v4_ip, uint16_t port, const uint8_t *buf, uint32_t size) = 0;
        };

        virtual void register_events(events_type *events) = 0;

    protected:
        virtual ~udp_server_t()  {}
    };
}

#endif //_UDP_SERVER_H_INCLUDED
