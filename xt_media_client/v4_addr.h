#ifndef _V4_ADDR_H_INCLUDED
#define _V4_ADDR_H_INCLUDED

#include "compat.h"
#include <stdint.h>

namespace xt_media_client
{
    struct v4_addr_t
    {
        uint32_t ip_;
        uint16_t port_;

        bool operator ==(const v4_addr_t&rhs) const
        {
            return ((ip_ == rhs.ip_) && (port_ == rhs.port_));
        }

        v4_addr_t()
            :ip_(0),
            port_(0)
        {}

        v4_addr_t(uint32_t ip, uint16_t port)
            :ip_(ip),
            port_(port)
        {}

        v4_addr_t(const char *ip, uint16_t port)
            :ip_(0),
            port_(port)
        {
            ip_from_string(ip, ip_);
        }

        explicit v4_addr_t(const v4_addr_t& rhs)
            :ip_(rhs.ip_),
            port_(rhs.port_)
        {}

        bool set_addr(const char *ip, uint16_t port)
        {
            if (!ip_from_string(ip, ip_))
            {
                return false;
            }
            port_ = port;
            return true;
        }

        void get_addr(char *ip, uint16_t& port) const
        {
            ip_to_string(ip_, ip);
            port = port_;
        }

        static bool ip_from_string(const char *ip, uint32_t& n)
        {
            uint32_t dots[4] = {0};
            if (4 != sscanf_s(ip, "%d.%d.%d.%d", dots, dots + 1, dots + 2, dots + 3))
            {
                return false;
            }

            uint8_t *ns = reinterpret_cast<uint8_t *>(&n);
            ns[0] = static_cast<uint8_t>(dots[0]);
            ns[1] = static_cast<uint8_t>(dots[1]);
            ns[2] = static_cast<uint8_t>(dots[2]);
            ns[3] = static_cast<uint8_t>(dots[3]);
            return true;
        }

        static bool ip_to_string(uint32_t n, char *ip)
        {
            uint8_t *ns = reinterpret_cast<uint8_t *>(&n);
            return (4 == snprintf_s(ip, 32, 32, "%d.%d.%d.%d", ns[0], ns[1], ns[2], ns[3]));
        }
    };
}

#endif //_V4_ADDR_H_INCLUDED
