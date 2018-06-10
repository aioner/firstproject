#include "udp_server_impl.h"

namespace udp_session_server
{
    udp_server_impl::udp_server_impl(boost::asio::io_service& service)
        :rbuf_(),
        socket_(new boost::asio::ip::udp::socket(service, boost::asio::ip::udp::v4())),
        recvfrom_peer_(),
        events_(NULL)
    {}

    bool udp_server_impl::init(const char *v4_ip, uint16_t port, uint32_t size)
    {
        if (!rbuf_.empty())
        {
            return false;
        }

        boost::system::error_code ec;
        socket_->bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::from_string(v4_ip), port), ec);
        if (ec)
        {
            return false;
        }

        rbuf_.resize(size);

        //added by lichao, 20150212 DT??windows udp bug(10054:远程主机强迫关闭了一个现有的连接)
#ifdef _WIN32
        BOOL opt = FALSE;
        DWORD dw = 0;
        ::WSAIoctl(socket_->native_handle(), SIO_UDP_CONNRESET, &opt, sizeof(opt), NULL, 0, &dw, NULL, NULL);
#endif

        handle_receive_from();
        return true;
    }

    bool udp_server_impl::send_msg(const char *peer_v4_ip, uint16_t peer_port, const uint8_t* buf, uint32_t len)
    {
        boost::asio::ip::udp::endpoint peer(boost::asio::ip::address_v4::from_string(peer_v4_ip), peer_port);
        return handle_send_to(peer, buf, len);
    }

    void udp_server_impl::register_events(events_type *events)
    {
        events_ = events;
    }

    std::size_t udp_server_impl::get_receive_buffer_size() const
    {
        return rbuf_.size();
    }

    bool udp_server_impl::handle_send_to(const boost::asio::ip::udp::endpoint& peer, const uint8_t* buf, uint32_t len)
    {
        boost::system::error_code ec;
        socket_->send_to(boost::asio::buffer(buf, len), peer, 0, ec);
        if (ec)
        {
            ::printf("udp_client_impl::handle_send_to| boost::send_to [%d,%s]\n", ec.value(), ec.message().c_str());
        }
        return !ec;
    }

    void udp_server_impl::handle_receive_from()
    {
        socket_->async_receive_from(boost::asio::buffer(rbuf_), recvfrom_peer_, boost::bind(&udp_server_impl::receive_from_handler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void udp_server_impl::receive_from_handler(const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (!ec && (bytes_transferred > 0))
        {
            events_->on_receive_msg(recvfrom_peer_.address().to_string().c_str(), recvfrom_peer_.port(), &rbuf_[0], bytes_transferred);
        }

        handle_receive_from();
    }
}

