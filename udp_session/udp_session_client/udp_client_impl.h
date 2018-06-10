#ifndef _UDP_CLIENT_IMPL_H_INCLUED
#define _UDP_CLIENT_IMPL_H_INCLUED

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/bind/placeholders.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <vector>
#include <memory>

#include "udp_client.h"

namespace udp_session_client
{
    class udp_client_impl : public udp_client_t, public boost::enable_shared_from_this<udp_client_impl>, private boost::noncopyable
    {
    public:
        udp_client_impl(boost::asio::io_service& service);

        bool init(const char *v4_ip, uint16_t port, uint32_t size);
        bool send_msg(const char *peer_v4_ip, uint16_t peer_port, const uint8_t* buf, uint32_t len);
        void register_events(events_type *events);

        std::size_t get_receive_buffer_size() const;

    private:
        bool handle_send_to(const boost::asio::ip::udp::endpoint& peer, const uint8_t* buf, uint32_t len);
        void handle_receive_from();
        void receive_from_handler(const boost::system::error_code& ec, std::size_t bytes_transferred);

        std::vector<uint8_t> rbuf_;
        boost::shared_ptr<boost::asio::ip::udp::socket> socket_;
        boost::asio::ip::udp::endpoint recvfrom_peer_;
        udp_client_t::events_type *events_;
    };
}

#endif //_UDP_CLIENT_IMPL_H_INCLUED
