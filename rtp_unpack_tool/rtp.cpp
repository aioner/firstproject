#include "rtp.h"

#include "rtp_unpack.h"
#include "rtp_unpack_impl.h"
using namespace xt_media_client;

#include <vector>

namespace helper
{
    template<typename T>
    void reverse(T *begin, T *end)
    {
        while (begin < end)
        {
            T x = *begin;
            *begin = *end;
            *end = x;

            begin++;
            end--;
        }
    }

    template<typename T, size_t N>
    void reverse(T (&a)[N])
    {
        reverse(a, a + N - 1);
    }

    bool is_little_endian()
    {
        union 
        {
            uint16_t u16;
            uint8_t a[sizeof(uint16_t) / sizeof(uint8_t)];
        } convertor;

        convertor.u16 = 0x1;
        return (convertor.a[0] == 0x1);
    }

    template<typename T>
    T network_to_host(T t)
    {
        if (is_little_endian())
        {
            union 
            {
                T t;
                uint8_t a[sizeof(T) / sizeof(uint8_t)];
            } convertor;

            convertor.t = t;

            reverse(convertor.a);

            return convertor.t;
        }
        else
        {
            return t;
        }
    }

    void convert_from_network(void *buf, size_t pos, size_t n)
    {
        uint32_t *p = (uint32_t *)buf;
        for (size_t i = pos; i < pos + n; ++i)
        {
            p[i] = network_to_host(p[i]);
        }
    }

    class parser_buffer_t : public std::vector<char>
    {
    public:
        explicit parser_buffer_t(size_t n)
            :std::vector<char>(n, '\0')
        {}

        char *buf()
        {
            return &(operator [](0));
        }

        bool parse(size_t length, rtp_fixed_header_t & h)
        {
            char *p = buf();

            h.version = ((p[0] >> 6) & 0x3);
            h.padding = ((p[0] >> 5) & 0x1);
            h.extension = ((p[0] >> 4) & 0x1);
            h.CC = (p[0] & 0xf);

            h.marker = ((p[1] >> 7) & 0x1);
            h.payload = (p[1] & 0xef);

            h.seq = *(uint16_t *)(p + 2);
            h.timestamp = *(uint32_t *)(p + 4);
            h.ssrc = *(uint32_t *)(p + 8);

            h.seq = network_to_host(h.seq);
            h.timestamp = network_to_host(h.timestamp);
            h.ssrc = network_to_host(h.ssrc);

            return true;
        }
    };

    class frame_data_dump_impl_t : public frame_data_dump_callback_t
    {
    public:
        explicit frame_data_dump_impl_t(std::ostream& output)
            :output_(output)
        {}

        void on_frame_dump(void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc)
        {
            rtp_priv_header_t *x = static_cast<rtp_priv_header_t *>(data);
            output_.write((const char *)(x + 1), length - sizeof(rtp_priv_header_t));
        }

    private:
        std::ostream& output_;
    };
}

bool rtp_unpack_parser_impl_t::parse(std::istream& input, std::ostream& output, const std::string &format, std::ostream& log)
{
    log << "rtp unpacker with format:" << format << " creating..." << std::endl;

    rtp_unpack_ptr unpacker(rtp_unpack_t::create(format));
    if (!unpacker)
    {
        log << "format:" << format << " has no unpacker." << std::endl;
        return false;
    }

    helper::frame_data_dump_impl_t frame_writer(output);
    unpacker->register_frame_dump_callback(&frame_writer);

    uint32_t rtp_pkt_length = 0;
    helper::parser_buffer_t pb(2048);
    while (input && output)
    {
        input.read((char *)&rtp_pkt_length, 4);
        log << "------------------------------------------------" << std::endl;
        log << "rtp_pkt_length:" << rtp_pkt_length << std::endl;

        if (rtp_pkt_length < sizeof(rtp_fixed_header_t) - sizeof(uint32_t) * 15)
        {
            log << "rtp_pkt_length not enough." << std::endl;
            break;
        }

        if (rtp_pkt_length > pb.size())
        {
            pb.resize(rtp_pkt_length);
        }

        input.read(pb.buf(), rtp_pkt_length);

        if (rtp_pkt_length != input.gcount())
        {
            log << "file size: " << input.tellg() << " but eof." << std::endl;
            return false;
        }

        rtp_fixed_header_t fixed_header = { 0 };
        if (!pb.parse(rtp_pkt_length, fixed_header))
        {
            log << "fixed header parse failed." << std::endl;
            return false;
        }

        log << fixed_header;

        unpacker->pump_rtp_raw_data((uint8_t *)pb.buf() + RTP_FIXED_HEADER_LEN, rtp_pkt_length - RTP_FIXED_HEADER_LEN, fixed_header);
    }

    return true;
}