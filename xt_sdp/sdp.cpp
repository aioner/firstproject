#include "sdp.h"
#include "parse_buffer.h"

#include <stdexcept>
#include <cctype>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>

namespace Helper
{
    template<typename T>
    std::string to_string(const T& x)
    {
        std::ostringstream oss;
        oss << x;
        return oss.str();
    }

    void tolower(std::string& s)
    {
        for (std::string::size_type ii = 0; ii < s.size(); ++ii)
        {
            s[ii] = std::tolower(s[ii]);
        }
    }

    bool nocasecmp(const std::string& s1, const std::string& s2)
    {
        if (s1.size() != s2.size())
        {
            return false;
        }

        for (std::string::size_type ii = 0; ii < s1.size(); ++ii)
        {
            if (std::tolower(s1[ii]) != std::tolower(s2[ii]))
            {
                return false;
            }
        }

        return true;
    }

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
    T t2n(T t)
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

    void integer2hex(char* _d, unsigned int _s, bool _l)
    {
        int i;
        unsigned char j;
        int k = 0;
        char* s;

        _s = t2n(_s);
        s = (char*)&_s;

        for (i = 0; i < 4; i++)
        {
            j = (s[i] >> 4) & 0xf;
            if (j <= 9)
            {
                if (_l || j != 0 || k != 0)
                {
                    _d[k++] = (j + '0');
                }
            }
            else
            {
                _d[k++] = (j + 'a' - 10);
            }

            j = s[i] & 0xf;
            if (j <= 9)
            {
                if (_l || j != 0 || k != 0)
                {
                    _d[k++] = (j + '0');
                }
            }
            else
            {
                _d[k++] = (j + 'a' - 10);
            }
        }
    }

    unsigned int hex2integer(const char* _s)
    {
        unsigned int i, res = 0;

        for (i = 0; i < 8; i++)
        {
            if ((_s[i] >= '0') && (_s[i] <= '9'))
            {
                res *= 16;
                res += _s[i] - '0';
            }
            else if ((_s[i] >= 'a') && (_s[i] <= 'f'))
            {
                res *= 16;
                res += _s[i] - 'a' + 10;
            }
            else if ((_s[i] >= 'A') && (_s[i] <= 'F'))
            {
                res *= 16;
                res += _s[i] - 'A' + 10;
            }
            else
            {
                return res;
            }
        }

        return res;
    }

    std::bitset<256> to_bitset(const std::string& chars)
    {
        std::bitset<256> result;
        result.reset();
        for (unsigned int i=0; i!=chars.size();++i)
        {
            result.set(chars[i]);
        }
        return result;
    }
}

using namespace xt_sdp;

std::ostream& uri_t::encode(std::ostream& str) const
{
    str << scheme_ << ":";
    if (!user_.empty())
    {
        str << user_;
        if (!user_parameters_.empty())
        {
            str << ';' << user_parameters_;
        }
        if (!pwd_.empty())
        {
            str << ":";
            str << pwd_;
        }
    }
    if (!host_.empty())
    {
        if (!user_.empty())
        {
            str << "@";
        }
        if (0)
        {
            str << '[' << host_ << ']';
        }
        else
        {
            str << host_;
        }
    }
    if (port_ != 0)
    {
        str << ":" << port_;
    }

    return str;
}

void uri_t::parse(parse_buffer_t& pb)
{
    pb.skip_whitespace();
    const char* start = pb.position();
    pb.skip_to_one_of(":@");

    pb.assert_not_eof();

    pb.data(scheme_, start);
    pb.skip_char(':');
    Helper::tolower(scheme_);

    if ("tel" == scheme_)
    {
        const char* anchor = pb.position();
        static std::bitset<256> delimiter = Helper::to_bitset("\r\n\t ;>");
        pb.skip_to_one_of(delimiter);
        pb.data(user_, anchor);
        if (!pb.eof() && *pb.position() == ';')
        {
            anchor = pb.skip_char();
            pb.skip_to_one_of(parse_buffer_t::whitespace_, ">");
            pb.data(user_parameters_, anchor);
        }
        return;
    }

    start = pb.position();
    static std::bitset<256> userPortOrPasswordDelim = Helper::to_bitset("@:\"");
    // stop at double-quote to prevent matching an '@' in a quoted string param. 
    pb.skip_to_one_of(userPortOrPasswordDelim);
    if (!pb.eof())
    {
        const char* atSign = 0;
        if (*pb.position() == ':')
        {
            // Either a password, or a port
            const char* afterColon = pb.skip_char();
            pb.skip_to_one_of("@\"");
            if (!pb.eof() && *pb.position() == '@')
            {
                atSign = pb.position();
                // password
                pb.data(pwd_, afterColon);

                pb.reset(afterColon - 1);
            }
            else
            {
                // port. No user part.
                pb.reset(start);
            }
        }
        else if (*pb.position() == '@')
        {
            atSign = pb.position();
        }
        else
        {
            // Only a hostpart
            pb.reset(start);
        }

        if (atSign)
        {
            pb.data(user_, start);
            pb.reset(atSign);
            start = pb.skip_char();
        }
    }
    else
    {
        pb.reset(start);
    }

    //bool mHostCanonicalized = false;
    static std::bitset<256> hostDelimiter = Helper::to_bitset("\r\n\t :;?>");
    if (*start == '[')
    {
        assert(false);
    }
    else
    {
        pb.skip_to_one_of(hostDelimiter);
        pb.data(host_, start);
    }

    if (!pb.eof() && *pb.position() == ':')
    {
        start = pb.skip_char();
        port_ = pb.to_uint32();
    }
    else
    {
        port_ = 0;
    }
}

static const char* NetworkType[] = { "???", "IP4", "IP6" };

static const std::string rtpmap("rtpmap");
static const std::string fmtp("fmtp");

// RFC2327 6. page 9
// "parsers should be tolerant and accept records terminated with a single
// newline character"
void xt_sdp::skip_eol(parse_buffer_t& pb)
{
    while (!pb.eof() && (*pb.position() == ' ' ||
        *pb.position() == '\t'))
    {
        pb.skip_char();
    }

    if (*pb.position() == '\n')
    {
        pb.skip_char();
    }
    else
    {
        // allow extra 0x0d bytes.
        while (*pb.position() == '\r')
        {
            pb.skip_char();
        }
        pb.skip_char('\n');
    }

}

attribute_helper_t::attribute_helper_t(const attribute_helper_t& rhs)
    : attribute_list_(rhs.attribute_list_),
    attributes_(rhs.attributes_)
{}

attribute_helper_t::attribute_helper_t()
{}

attribute_helper_t& attribute_helper_t::operator=(const attribute_helper_t& rhs)
{
    if (this != &rhs)
    {
        attribute_list_ = rhs.attribute_list_;
        attributes_ = rhs.attributes_;
    }
    return *this;
}

bool attribute_helper_t::exists(const std::string& key) const
{
    return attributes_.find(key) != attributes_.end();
}

const std::list<std::string>& attribute_helper_t::get_values(const std::string& key) const
{
    if (!exists(key))
    {
        static const std::list<std::string> emptyList;
        return emptyList;
    }
    return attributes_.find(key)->second;
}

std::ostream& attribute_helper_t::encode(std::ostream& s) const
{
    for (std::list<std::pair<std::string, std::string> >::const_iterator i = attribute_list_.begin();
        i != attribute_list_.end(); ++i)
    {
        s << "a=" << i->first;
        if (!i->second.empty())
        {
            s << ':' << i->second;
        }
        s << "\r\n";
    }
    return s;
}

void attribute_helper_t::parse(parse_buffer_t& pb)
{
    std::string key;
    std::string value;

    pb.skip_char('a');
    const char* anchor = pb.skip_char('=');
    pb.skip_to_one_of(":", "\r\n");
    pb.data(key, anchor);
    if (!pb.eof() && *pb.position() == ':')
    {
        anchor = pb.skip_char(':');
        pb.skip_to_one_of("\r\n");
        pb.data(value, anchor);
    }

    if (!pb.eof()) skip_eol(pb);

    attribute_list_.push_back(std::make_pair(key, value));
    attributes_[key].push_back(value);
}

void attribute_helper_t::add_attribute(const std::string& key, const std::string& value)
{
    attribute_list_.push_back(std::make_pair(key, value));
    attributes_[key].push_back(value);
}

void attribute_helper_t::clear_attribute(const std::string& key)
{
    for (std::list<std::pair<std::string, std::string> >::iterator i = attribute_list_.begin(); i != attribute_list_.end();)
    {
        std::list<std::pair<std::string, std::string> >::iterator j = i++;
        if (j->first == key)
        {
            attribute_list_.erase(j);
        }
    }
    attributes_.erase(key);
}

static std::string nullOrigin("0.0.0.0");

sdp_session_t::origin_t::origin_t()
    : user_(),
    session_id_(0),
    version_(0),
    addr_type_(ipv4),
    address_(nullOrigin)
{}

sdp_session_t::origin_t::origin_t(const origin_t& rhs)
    : user_(rhs.user_),
    session_id_(rhs.session_id_),
    version_(rhs.version_),
    addr_type_(rhs.addr_type_),
    address_(rhs.address_)
{}

sdp_session_t::origin_t& sdp_session_t::origin_t::operator=(const origin_t& rhs)
{
    if (this != &rhs)
    {
        user_ = rhs.user_;
        session_id_ = rhs.session_id_;
        version_ = rhs.version_;
        addr_type_ = rhs.addr_type_;
        address_ = rhs.address_;
    }
    return *this;
}

sdp_session_t::origin_t::origin_t(const std::string& user,
    const uint64_t& sessionId,
    const uint64_t& version,
    addr_type addr,
    const std::string& address)
    : user_(user),
    session_id_(sessionId),
    version_(version),
    addr_type_(addr),
    address_(address)
{}

std::ostream& sdp_session_t::origin_t::encode(std::ostream& s) const
{
    s << "o="
        << user_ << ' '
        << session_id_ << ' '
        << version_ << ' '
        << "IN "
        << NetworkType[addr_type_] << ' '
        << address_ << "\r\n";
    return s;
}

void sdp_session_t::origin_t::set_address(const std::string& host, addr_type addr)
{
    address_ = host;
    addr_type_ = addr;
}

void sdp_session_t::origin_t::parse(parse_buffer_t& pb)
{
    pb.skip_char('o');
    const char* anchor = pb.skip_char('=');

    pb.skip_to_char(' ');
    pb.data(user_, anchor);

    anchor = pb.skip_char(' ');

    session_id_ = pb.to_uint64();

    pb.skip_to_char(' ');

    anchor = pb.skip_char(' ');

    version_ = pb.to_uint64();

    pb.skip_to_char(' ');

    pb.skip_char(' ');
    pb.skip_char('I');
    pb.skip_char('N');

    anchor = pb.skip_char(' ');
    pb.skip_to_char(' ');
    std::string addrType;
    pb.data(addrType, anchor);
    if (addrType == NetworkType[ipv4])
    {
        addr_type_ = ipv4;
    }
    else if (addrType == NetworkType[ipv6])
    {
        addr_type_ = ipv6;
    }
    else
    {
        addr_type_ = static_cast<addr_type>(0);
    }

    anchor = pb.skip_char(' ');
    pb.skip_to_one_of("\r\n");
    pb.data(address_, anchor);

    skip_eol(pb);
}

sdp_session_t::email_t::email_t(const std::string& address,
    const std::string& freeText)
    : address_(address),
    free_text_(freeText)
{}

sdp_session_t::email_t::email_t(const email_t& rhs)
    : address_(rhs.address_),
    free_text_(rhs.free_text_)
{}

sdp_session_t::email_t& sdp_session_t::email_t::operator=(const email_t& rhs)
{
    if (this != &rhs)
    {
        address_ = rhs.address_;
        free_text_ = rhs.free_text_;
    }
    return *this;
}

std::ostream& sdp_session_t::email_t::encode(std::ostream& s) const
{
    s << "e=" << address_;
    if (!free_text_.empty())
    {
        s << ' ';
        s << '(' << free_text_ << ')';
    }
    s << "\r\n";

    return s;
}

// helper to parse email and phone numbers with display name
static void parseEorP(parse_buffer_t& pb, std::string& eOrp, std::string& freeText)
{
    // =mjh@isi.edu (Mark Handley)
    // =mjh@isi.edu
    // =Mark Handley <mjh@isi.edu>
    // =<mjh@isi.edu>

    const char* anchor = pb.skip_char('=');

    pb.skip_to_one_of("<(\n\r");  // find a left angle bracket "<", a left paren "(", or a CR 
    switch (*pb.position())
    {
    case '\n':                  // '\r'
    case '\r':                  // '\n'
        // mjh@isi.edu
        //            ^
        pb.data(eOrp, anchor);
        break;

    case '<':                       // Symbols::LA_QUOTE[0]
        // Mark Handley <mjh@isi.edu>
        //              ^
        // <mjh@isi.edu>
        // ^

        pb.data(freeText, anchor);
        anchor = pb.skip_char();
        pb.skip_to_end_quote('>');
        pb.data(eOrp, anchor);
        pb.skip_char('>');
        break;

    case '(':                       // '('
        // mjh@isi.edu (Mark Handley)
        //             ^

        pb.data(eOrp, anchor);
        anchor = pb.skip_char();
        pb.skip_to_end_quote(')');
        pb.data(freeText, anchor);
        pb.skip_char(')');
        break;
    default:
        assert(0);
    }
}

void sdp_session_t::email_t::parse(parse_buffer_t& pb)
{
    pb.skip_char('e');
    parseEorP(pb, address_, free_text_);
    skip_eol(pb);
}

sdp_session_t::phone_t::phone_t(const std::string& number,
    const std::string& freeText)
    : number_(number),
    free_text_(freeText)
{}

sdp_session_t::phone_t::phone_t(const phone_t& rhs)
    : number_(rhs.number_),
    free_text_(rhs.free_text_)
{}

sdp_session_t::phone_t& sdp_session_t::phone_t::operator=(const phone_t& rhs)
{
    if (this != &rhs)
    {
        number_ = rhs.number_;
        free_text_ = rhs.free_text_;
    }
    return *this;
}

std::ostream& sdp_session_t::phone_t::encode(std::ostream& s) const
{
    s << "p=" << number_;
    if (!free_text_.empty())
    {
        s << ' ';
        s << '(' << free_text_ << ')';
    }
    s << "\r\n";

    return s;
}

void sdp_session_t::phone_t::parse(parse_buffer_t& pb)
{
    pb.skip_char('p');
    parseEorP(pb, number_, free_text_);
    skip_eol(pb);
}

sdp_session_t::connection_t::connection_t(addr_type addType,
    const std::string& address,
    unsigned long ttl)
    : addr_type_(addType),
    address_(address),
    ttl_(ttl)
{}

sdp_session_t::connection_t::connection_t()
    : addr_type_(ipv4),
    address_(),
    ttl_(0)
{}

sdp_session_t::connection_t::connection_t(const connection_t& rhs)
    : addr_type_(rhs.addr_type_),
    address_(rhs.address_),
    ttl_(rhs.ttl_)
{
}

sdp_session_t::connection_t& sdp_session_t::connection_t::operator=(const connection_t& rhs)
{
    if (this != &rhs)
    {
        addr_type_ = rhs.addr_type_;
        address_ = rhs.address_;
        ttl_ = rhs.ttl_;
    }
    return *this;
}

std::ostream& sdp_session_t::connection_t::encode(std::ostream& s) const
{
    s << "c=IN "
        << NetworkType[addr_type_] << ' ' << address_;

    if (ttl_)
    {
        s << '/' << ttl_;
    }
    s << "\r\n";
    return s;
}

void sdp_session_t::connection_t::set_address(const std::string& host, addr_type addr)
{
    address_ = host;
    addr_type_ = addr;
}

void sdp_session_t::connection_t::parse(parse_buffer_t& pb)
{
    pb.skip_char('c');
    pb.skip_char('=');
    pb.skip_char('I');
    pb.skip_char('N');

    const char* anchor = pb.skip_char(' ');
    pb.skip_to_char(' ');
    std::string addrType;
    pb.data(addrType, anchor);
    if (addrType == NetworkType[ipv4])
    {
        addr_type_ = ipv4;
    }
    else if (addrType == NetworkType[ipv6])
    {
        addr_type_ = ipv6;
    }
    else
    {
        addr_type_ = static_cast<addr_type>(0);
    }

    anchor = pb.skip_char();
    pb.skip_to_one_of("/", "\r\n");
    pb.data(address_, anchor);

    ttl_ = 0;
    if (addr_type_ == ipv4 && !pb.eof() && *pb.position() == '/')
    {
        pb.skip_char();
        ttl_ = pb.integer();
    }

    // multicast dealt with above this parser
    if (!pb.eof() && *pb.position() != '/')
    {
        skip_eol(pb);
    }
}

sdp_session_t::bandwidth_t::bandwidth_t(const std::string& modifier,
    unsigned long kbs)
    : modifier_(modifier),
    kbs_(kbs)
{}

sdp_session_t::bandwidth_t::bandwidth_t(const bandwidth_t& rhs)
    : modifier_(rhs.modifier_),
    kbs_(rhs.kbs_)
{}

sdp_session_t::bandwidth_t& sdp_session_t::bandwidth_t::operator=(const bandwidth_t& rhs)
{
    if (this != &rhs)
    {
        modifier_ = rhs.modifier_;
        kbs_ = rhs.kbs_;
    }
    return *this;
}

std::ostream& sdp_session_t::bandwidth_t::encode(std::ostream& s) const
{
    s << "b="
        << modifier_
        << ':' << kbs_
        << "\r\n";
    return s;
}

void sdp_session_t::bandwidth_t::parse(parse_buffer_t& pb)
{
    pb.skip_char('b');
    const char* anchor = pb.skip_char('=');

    pb.skip_to_one_of(":", "\r\n");
    if (*pb.position() == ':')
    {
        pb.data(modifier_, anchor);

        anchor = pb.skip_char(':');
        kbs_ = pb.integer();

        skip_eol(pb);
    }
    else
    {
        pb.fail(__FILE__, __LINE__);
    }
}

sdp_session_t::time_t::time_t(unsigned long start,
    unsigned long stop)
    : start_(start),
    stop_(stop)
{}

sdp_session_t::time_t::time_t(const time_t& rhs)
    : start_(rhs.start_),
    stop_(rhs.stop_)
{}

sdp_session_t::time_t& sdp_session_t::time_t::operator=(const time_t& rhs)
{
    if (this != &rhs)
    {
        start_ = rhs.start_;
        stop_ = rhs.stop_;
        repeats_ = rhs.repeats_;
    }
    return *this;
}

std::ostream& sdp_session_t::time_t::encode(std::ostream& s) const
{
    s << "t=" << start_ << ' '
        << stop_
        << "\r\n";

    for (std::list<repeat_t>::const_iterator i = repeats_.begin();
        i != repeats_.end(); ++i)
    {
        i->encode(s);
    }
    return s;
}

void sdp_session_t::time_t::parse(parse_buffer_t& pb)
{
    pb.skip_char('t');
    pb.skip_char('=');

    start_ = pb.to_uint32();
    pb.skip_char(' ');
    stop_ = pb.to_uint32();

    skip_eol(pb);

    while (!pb.eof() && *pb.position() == 'r')
    {
        add_repeat(repeat_t());
        repeats_.back().parse(pb);
    }
}

void sdp_session_t::time_t::add_repeat(const repeat_t& repeat)
{
    repeats_.push_back(repeat);
}

sdp_session_t::time_t::repeat_t::repeat_t(unsigned long interval,
    unsigned long duration,
    std::list<int> offsets)
    : interval_(interval),
    duration_(duration),
    offsets_(offsets)
{}

std::ostream& sdp_session_t::time_t::repeat_t::encode(std::ostream& s) const
{
    s << "r="
        << interval_ << ' '
        << duration_ << 's';
    for (std::list<int>::const_iterator i = offsets_.begin();
        i != offsets_.end(); ++i)
    {
        s << ' ' << *i << 's';
    }

    s << "\r\n";
    return s;
}

static int parseTypedTime(parse_buffer_t& pb)
{
    int v = pb.integer();
    if (!pb.eof())
    {
        switch (*pb.position())
        {
        case 's':
            pb.skip_char();
            break;
        case 'm':
            v *= 60;
            pb.skip_char();
            break;
        case 'h':
            v *= 3600;
            pb.skip_char();
            break;
        case 'd':
            v *= 3600 * 24;
            pb.skip_char();
        }
    }
    return v;
}

void sdp_session_t::time_t::repeat_t::parse(parse_buffer_t& pb)
{
    pb.skip_char('r');
    pb.skip_char('=');

    interval_ = parseTypedTime(pb);
    pb.skip_char(' ');

    duration_ = parseTypedTime(pb);

    while (!pb.eof() && *pb.position() != '\r')
    {
        pb.skip_char(' ');

        offsets_.push_back(parseTypedTime(pb));
    }

    skip_eol(pb);
}

sdp_session_t::timezones_t::adjustment_t::adjustment_t(unsigned long _time,
    int _offset)
    : time(_time),
    offset(_offset)
{}

sdp_session_t::timezones_t::adjustment_t::adjustment_t(const adjustment_t& rhs)
    : time(rhs.time),
    offset(rhs.offset)
{}

sdp_session_t::timezones_t::adjustment_t&
sdp_session_t::timezones_t::adjustment_t::operator=(const adjustment_t& rhs)
{
    if (this != &rhs)
    {
        time = rhs.time;
        offset = rhs.offset;
    }
    return *this;
}

sdp_session_t::timezones_t::timezones_t()
    : adjustments_()
{}

sdp_session_t::timezones_t::timezones_t(const timezones_t& rhs)
    : adjustments_(rhs.adjustments_)
{}

sdp_session_t::timezones_t& sdp_session_t::timezones_t::operator=(const timezones_t& rhs)
{
    if (this != &rhs)
    {
        adjustments_ = rhs.adjustments_;
    }
    return *this;
}

std::ostream& sdp_session_t::timezones_t::encode(std::ostream& s) const
{
    if (!adjustments_.empty())
    {
        s << "z=";
        bool first = true;
        for (std::list<adjustment_t>::const_iterator i = adjustments_.begin();
            i != adjustments_.end(); ++i)
        {
            if (!first)
            {
                s << ' ';
            }
            first = false;
            s << i->time << ' '
                << i->offset << 's';
        }

        s << "\r\n";
    }
    return s;
}

void sdp_session_t::timezones_t::parse(parse_buffer_t& pb)
{
    pb.skip_char('z');
    pb.skip_char('=');

    while (!pb.eof() && *pb.position() != '\r')
    {
        adjustment_t adj(0, 0);
        adj.time = pb.integer();
        pb.skip_char(' ');
        adj.offset = parseTypedTime(pb);
        add_adjustment(adj);

        if (!pb.eof() && *pb.position() == ' ')
        {
            pb.skip_char();
        }
    }

    skip_eol(pb);
}

void sdp_session_t::timezones_t::add_adjustment(const adjustment_t& adjust)
{
    adjustments_.push_back(adjust);
}

sdp_session_t::encryption_t::encryption_t()
    : method_(no_encryption),
    key_()
{}

sdp_session_t::encryption_t::encryption_t(const key_type& method,
    const std::string& key)
    : method_(method),
    key_(key)
{}

sdp_session_t::encryption_t::encryption_t(const encryption_t& rhs)
    : method_(rhs.method_),
    key_(rhs.key_)
{}

sdp_session_t::encryption_t& sdp_session_t::encryption_t::operator=(const encryption_t& rhs)
{
    if (this != &rhs)
    {
        method_ = rhs.method_;
        key_ = rhs.key_;
    }
    return *this;
}

static const char* KeyTypes[] = { "????", "prompt", "clear", "base64", "uri" };

std::ostream& sdp_session_t::encryption_t::encode(std::ostream& s) const
{
    s << "k="
        << KeyTypes[method_];
    if (method_ != prompt)
    {
        s << ':' << key_;
    }
    s << "\r\n";

    return s;
}

void sdp_session_t::encryption_t::parse(parse_buffer_t& pb)
{
    pb.skip_char('k');
    const char* anchor = pb.skip_char('=');

    pb.skip_to_char(':');
    if (!pb.eof())
    {
        std::string p;
        pb.data(p, anchor);
        if (p == KeyTypes[clear])
        {
            method_ = clear;
        }
        else if (p == KeyTypes[base64])
        {
            method_ = base64;
        }
        else if (p == KeyTypes[uri_key])
        {
            method_ = uri_key;
        }

        anchor = pb.skip_char(':');
        pb.skip_to_one_of("\r\n");
        pb.data(key_, anchor);
    }
    else
    {
        pb.reset(anchor);
        pb.skip_to_one_of("\r\n");

        std::string p;
        pb.data(p, anchor);
        if (p == KeyTypes[prompt])
        {
            method_ = prompt;
        }
    }

    skip_eol(pb);
}

sdp_session_t::sdp_session_t(int version,
    const origin_t& origin,
    const std::string& name)
    : version_(version),
    origin_(origin),
    name_(name)
{}

sdp_session_t::sdp_session_t(const sdp_session_t& rhs)
{
    *this = rhs;
}

sdp_session_t& sdp_session_t::operator=(const sdp_session_t& rhs)
{
    if (this != &rhs)
    {
        version_ = rhs.version_;
        origin_ = rhs.origin_;
        name_ = rhs.name_;
        media_ = rhs.media_;
        information_ = rhs.information_;
        uri_ = rhs.uri_;
        emails_ = rhs.emails_;
        phones_ = rhs.phones_;
        connection_ = rhs.connection_;
        bandwidths_ = rhs.bandwidths_;
        times_ = rhs.times_;
        timezones_ = rhs.timezones_;
        encryption_ = rhs.encryption_;
        attribute_helper_ = rhs.attribute_helper_;

        for (medium_container_t::iterator i = media_.begin(); i != media_.end(); ++i)
        {
            i->set_session(this);
        }
    }
    return *this;
}

void sdp_session_t::parse(parse_buffer_t& pb)
{
    while (!pb.eof())
    {
        switch (*pb.position())
        {
        case 'v':
            pb.skip_char('v');
            pb.skip_char('=');
            version_ = pb.integer();
            skip_eol(pb);
            break;
        case 'o':
            origin_.parse(pb);
            break;
        case 's':
            {
                pb.skip_char('s');
                const char* anchor = pb.skip_char('=');
                pb.skip_to_one_of("\r\n");
                pb.data(name_, anchor);
                skip_eol(pb);
            }
            break;
        case 'i':
            {
                pb.skip_char('i');
                const char* anchor = pb.skip_char('=');
                pb.skip_to_one_of("\r\n");
                pb.data(information_, anchor);
                skip_eol(pb);
            }
            break;
        case 'u':
            pb.skip_char('u');
            pb.skip_char('=');
            uri_.parse(pb);
            skip_eol(pb);
            break;
        case 'e':
            add_email(email_t());
            emails_.back().parse(pb);
            break;
        case 'p':
            add_phone(phone_t());
            phones_.back().parse(pb);
            break;
        case 'c':
            connection_.parse(pb);
            break;
        case 'b':
            add_bandwidth(bandwidth_t());
            bandwidths_.back().parse(pb);
            break;
        case 't':
            add_time(time_t());
            times_.back().parse(pb);
            break;
        case 'z':
            timezones_.parse(pb);
            break;
        case 'k':
            encryption_.parse(pb);
            break;
        case 'a':
            attribute_helper_.parse(pb);
            break;
        case 'm':
            add_medium(medium_t());
            media_.back().parse(pb);
            break;
        case 0:
            return ;
            break;
        default:
            {
                skip_eol(pb);
            }
            break;
        }
    }
}

std::ostream& sdp_session_t::encode(std::ostream& s) const
{
    s << "v=" << version_ << "\r\n";
    origin_.encode(s);
    s << "s=" << name_ << "\r\n";

    if (!information_.empty())
    {
        s << "i=" << information_ << "\r\n";
    }

    if (!uri_.host().empty())
    {
        s << "u=";
        uri_.encode(s);
        s << "\r\n";
    }

    for (std::list<email_t>::const_iterator i = emails_.begin(); i != emails_.end(); ++i)
    {
        i->encode(s);
    }

    for (std::list<phone_t>::const_iterator i = phones_.begin(); i != phones_.end(); ++i)
    {
        i->encode(s);
    }

    if (!connection_.address().empty())
    {
        connection_.encode(s);
    }

    for (std::list<bandwidth_t>::const_iterator i = bandwidths_.begin(); i != bandwidths_.end(); ++i)
    {
        i->encode(s);
    }

    if (times_.empty())
    {
        s << "t=0 0" << "\r\n";
    }
    else
    {
        for (std::list<time_t>::const_iterator i = times_.begin(); i != times_.end(); ++i)
        {
            i->encode(s);
        }
    }

    timezones_.encode(s);

    if (encryption_.method() != encryption_t::no_encryption)
    {
        encryption_.encode(s);
    }

    attribute_helper_.encode(s);

    for (medium_container_t::const_iterator i = media_.begin(); i != media_.end(); ++i)
    {
        i->encode(s);
    }

    return s;
}

void sdp_session_t::add_email(const email_t& email)
{
    emails_.push_back(email);
}

void sdp_session_t::add_time(const time_t& t)
{
    times_.push_back(t);
}

void sdp_session_t::add_phone(const phone_t& phone)
{
    phones_.push_back(phone);
}

void sdp_session_t::add_bandwidth(const bandwidth_t& bandwidth)
{
    bandwidths_.push_back(bandwidth);
}

void sdp_session_t::add_medium(const medium_t& medium)
{
    media_.push_back(medium);
    media_.back().set_session(this);
}

void sdp_session_t::add_attribute(const std::string& key, const std::string& value)
{
    attribute_helper_.add_attribute(key, value);

    if (key == rtpmap)
    {
        for (medium_container_t::iterator i = media_.begin();
            i != media_.end(); ++i)
        {
            i->rtp_map_done_ = false;
        }
    }
}

void sdp_session_t::clear_attribute(const std::string& key)
{
    attribute_helper_.clear_attribute(key);

    if (key == rtpmap)
    {
        for (medium_container_t::iterator i = media_.begin();
            i != media_.end(); ++i)
        {
            i->rtp_map_done_ = false;
        }
    }
}

bool sdp_session_t::exists(const std::string& key) const
{
    return attribute_helper_.exists(key);
}

const std::list<std::string>& sdp_session_t::get_values(const std::string& key) const
{
    return attribute_helper_.get_values(key);
}

sdp_session_t::medium_t::medium_t(const std::string& name,
    unsigned long port,
    unsigned long multicast,
    const std::string& protocol)
    : session_(0),
    name_(name),
    port_(port),
    multicast_(multicast),
    protocol_(protocol),
    rtp_map_done_(false)
{}

sdp_session_t::medium_t::medium_t()
    : session_(0),
    port_(0),
    multicast_(1),
    rtp_map_done_(false)
{}

sdp_session_t::medium_t::medium_t(const medium_t& rhs)
    : session_(0),
    name_(rhs.name_),
    port_(rhs.port_),
    multicast_(rhs.multicast_),
    protocol_(rhs.protocol_),
    formats_(rhs.formats_),
    codecs_(rhs.codecs_),
    transport_(rhs.transport_),
    information_(rhs.information_),
    connections_(rhs.connections_),
    bandwidths_(rhs.bandwidths_),
    encryption_(rhs.encryption_),
    attribute_helper_(rhs.attribute_helper_),
    rtp_map_done_(rhs.rtp_map_done_),
    rtp_map_(rhs.rtp_map_)
{}

sdp_session_t::medium_t& sdp_session_t::medium_t::operator=(const medium_t& rhs)
{
    if (this != &rhs)
    {
        session_ = 0;
        name_ = rhs.name_;
        port_ = rhs.port_;
        multicast_ = rhs.multicast_;
        protocol_ = rhs.protocol_;
        formats_ = rhs.formats_;
        codecs_ = rhs.codecs_;
        transport_ = rhs.transport_;
        information_ = rhs.information_;
        connections_ = rhs.connections_;
        bandwidths_ = rhs.bandwidths_;
        encryption_ = rhs.encryption_;
        attribute_helper_ = rhs.attribute_helper_;
        rtp_map_done_ = rhs.rtp_map_done_;
        rtp_map_ = rhs.rtp_map_;
    }
    return *this;
}

void sdp_session_t::medium_t::set_session(sdp_session_t* session)
{
    session_ = session;
}

void sdp_session_t::medium_t::parse(parse_buffer_t& pb)
{
    pb.skip_char('m');
    const char* anchor = pb.skip_char('=');

    pb.skip_to_char(' ');
    pb.data(name_, anchor);
    pb.skip_char(' ');

    port_ = pb.integer();

    if (*pb.position() == '/')
    {
        pb.skip_char();
        multicast_ = pb.integer();
    }

    anchor = pb.skip_char(' ');
    pb.skip_to_one_of(" ", "\r\n");
    pb.data(protocol_, anchor);

    while (*pb.position() != '\r' &&
        *pb.position() != '\n')
    {
        anchor = pb.skip_char(' ');
        pb.skip_to_one_of(" ", "\r\n");
        if (pb.position() != anchor)
        {
            std::string format;
            pb.data(format, anchor);
            add_format(format);
        }
    }

    skip_eol(pb);

    while (!pb.eof())
    {
        bool parsed_finish = false;

        switch (*pb.position())
        {
        case 'm':
            parsed_finish = true;
            break;
        case 'i':
            pb.skip_char('i');
            anchor = pb.skip_char('=');
            pb.skip_to_one_of("\r\n");
            pb.data(information_, anchor);
            skip_eol(pb);
            break;
        case 'c':
            add_connection(connection_t());
            connections_.back().parse(pb);
            if (!pb.eof() && *pb.position() == '/')
            {
                // Note:  we only get here if there was a /<number of addresses> 
                //        parameter following the connection address. 
                pb.skip_char();
                int num = pb.integer();

                connection_t& con = connections_.back();
                const std::string& addr = con.address();
                size_t i = addr.size() - 1;
                for (; i; i--)
                {
                    if (addr[i] == '.' || addr[i] == ':') // ipv4 or ipv6
                    {
                        break;
                    }
                }

                if (addr[i] == '.')  // add a number of ipv4 connections
                {
                    std::string before(addr.data(), i + 1);
                    parse_buffer_t subpb(addr.data() + i + 1, addr.size() - i - 1);
                    int after = subpb.integer();

                    for (int i = 1; i < num; i++)
                    {
                        add_connection(con);
                        connections_.back().address_ = before + Helper::to_string(after + i);
                    }
                }
                if (addr[i] == ':') // add a number of ipv6 connections
                {
                    std::string before(addr.data(), i + 1);
                    int after = Helper::hex2integer(addr.data() + i + 1);
                    char hexstring[9];

                    for (int i = 1; i < num; i++)
                    {
                        add_connection(con);
                        memset(hexstring, 0, sizeof(hexstring));
                        Helper::integer2hex(hexstring, after + i, false /* supress leading zeros */);
                        connections_.back().address_ = before + std::string(hexstring);
                    }
                }

                skip_eol(pb);
            }
            break;
        case 'b':
            add_bandwidth(bandwidth_t());
            bandwidths_.back().parse(pb);
            break;
        case 'k':
            encryption_.parse(pb);
            break;
        case 'a':
            attribute_helper_.parse(pb);
            break;
        case 0:
            parsed_finish = true;
            break;
        default:
            {
                skip_eol(pb);
//                 std::ostringstream oss;
//                 oss << "field " << *pb.position() << " not parsed.";
//                 pb.fail(__FILE__, __LINE__, oss.str());
            }
            break;
        }

        if (parsed_finish)
        {
            break;
        }
    }
}

std::ostream& sdp_session_t::medium_t::encode(std::ostream& s) const
{
    s << "m=" << name_ << ' ' << port_;
    if (multicast_ > 1)
    {
        s << '/' << multicast_;
    }
    s << ' '<< protocol_;

    for (std::list<std::string>::const_iterator i = formats_.begin(); i != formats_.end(); ++i)
    {
        s << ' ' << *i;
    }

    if (!codecs_.empty())
    {
        for (codec_container_t::const_iterator i = codecs_.begin(); i != codecs_.end(); ++i)
        {
            s << ' ' << i->payload();
        }
    }

    s << "\r\n";

    if (!information_.empty())
    {
        s << "i=" << information_ << "\r\n";
    }

    for (std::list<connection_t>::const_iterator i = connections_.begin(); i != connections_.end(); ++i)
    {
        i->encode(s);
    }

    for (std::list<bandwidth_t>::const_iterator i = bandwidths_.begin(); i != bandwidths_.end(); ++i)
    {
        i->encode(s);
    }

    if (encryption_.method() != encryption_t::no_encryption)
    {
        encryption_.encode(s);
    }

    if (!codecs_.empty())
    {
        // add codecs to information and attributes
        for (codec_container_t::const_iterator i = codecs_.begin(); i != codecs_.end(); ++i)
        {
            // If codec is static (defined in RFC 3551) we probably shouldn't
            // add attributes for it. But some UAs do include them.
            //codec_t::codec_map_t& staticCodecs = codec_t::get_static_codecs();
            //if (staticCodecs.find(i->payload()) != staticCodecs.end())
            //{
            //    continue;
            //}

            s << "a=rtpmap:" << i->payload() << ' ' << *i << "\r\n";
            if (!i->parameters().empty())
            {
                s << "a=fmtp:" << i->payload() << ' ' << i->parameters() << "\r\n";
            }
        }
    }

    attribute_helper_.encode(s);

    return s;
}

void sdp_session_t::medium_t::add_format(const std::string& format)
{
    formats_.push_back(format);
}

void sdp_session_t::medium_t::set_connection(const connection_t& connection)
{
    connections_.clear();
    add_connection(connection);
}

void sdp_session_t::medium_t::add_connection(const connection_t& connection)
{
    connections_.push_back(connection);
}

void sdp_session_t::medium_t::set_bandwidth(const bandwidth_t& bandwidth)
{
    bandwidths_.clear();
    add_bandwidth(bandwidth);
}

void sdp_session_t::medium_t::add_bandwidth(const bandwidth_t& bandwidth)
{
    bandwidths_.push_back(bandwidth);
}

void sdp_session_t::medium_t::add_attribute(const std::string& key, const std::string& value)
{
    attribute_helper_.add_attribute(key, value);
    if (key == rtpmap)
    {
        rtp_map_done_ = false;
    }
}

const std::list<sdp_session_t::connection_t> sdp_session_t::medium_t::connections() const
{
    std::list<connection_t> connections = const_cast<medium_t*>(this)->get_medium_connections();
    // If there are connections specified at the medium level, then check if a session level
    // connection is present - if so then return it
    if (connections.empty() && session_ && !session_->connection().address().empty())
    {
        connections.push_back(session_->connection());
    }

    return connections;
}

bool sdp_session_t::medium_t::exists(const std::string& key) const
{
    if (attribute_helper_.exists(key))
    {
        return true;
    }
    return session_ && session_->exists(key);
}

const std::list<std::string>& sdp_session_t::medium_t::get_values(const std::string& key) const
{
    if (attribute_helper_.exists(key))
    {
        return attribute_helper_.get_values(key);
    }
    if (!session_)
    {
        assert(false);
        static std::list<std::string> error;
        return error;
    }
    return session_->get_values(key);
}

void sdp_session_t::medium_t::clear_attribute(const std::string& key)
{
    attribute_helper_.clear_attribute(key);
    if (key == rtpmap)
    {
        rtp_map_done_ = false;
    }
}

void sdp_session_t::medium_t::clear_codecs()
{
    formats_.clear();
    clear_attribute(rtpmap);
    clear_attribute(fmtp);
    codecs_.clear();
}

void sdp_session_t::medium_t::add_codec(const codec_t& codec)
{
    codecs();
    codecs_.push_back(codec);
}

const sdp_session_t::medium_t::codec_container_t& sdp_session_t::medium_t::codecs() const
{
    return const_cast<medium_t*>(this)->codecs();
}

sdp_session_t::medium_t::codec_container_t& sdp_session_t::medium_t::codecs()
{
#if defined(WIN32) && defined(_MSC_VER) && (_MSC_VER < 1310)  // CJ TODO fix 
    assert(0);
#else 
    if (!rtp_map_done_)
    {
        // prevent recursion
        rtp_map_done_ = true;

        if (exists(rtpmap))
        {
            for (std::list<std::string>::const_iterator i = get_values(rtpmap).begin(); i != get_values(rtpmap).end(); ++i)
            {
                //DebugLog(<< "sdp_session_t::medium_t::getCodec(" << *i << ")");
                parse_buffer_t pb(i->data(), i->size());
                int format = pb.integer();
                // pass to codec constructor for parsing
                // pass this for other codec attributes
                try
                {
                    rtp_map_[format].parse(pb, *this, format);
                }
                catch (const std::exception&)
                {
                    //ErrLog(<< "Caught exception: " << e);
                    rtp_map_.erase(format);
                }
            }
        }

        for (std::list<std::string>::const_iterator i = formats_.begin(); i != formats_.end(); ++i)
        {
            int mapKey = atoi(i->c_str());
            rtp_map_t::const_iterator ri = rtp_map_.find(mapKey);
            if (ri != rtp_map_.end())
            {
                //DebugLog(<< "sdp_session_t::medium_t::getCodec[](" << ri->second << ")");
                codecs_.push_back(ri->second);
            }
            else
            {
                // !kk! Is it a static format?
                codec_t::codec_map_t& staticCodecs = codec_t::get_static_codecs();
                codec_t::codec_map_t::const_iterator ri = staticCodecs.find(mapKey);
                if (ri != staticCodecs.end())
                {
                    //DebugLog(<< "Found static codec for format: " << mapKey);
                    codec_t codec(ri->second);

                    // Look for format parameters, and assign
                    codec.assign_format_parameters(*this);

                    codecs_.push_back(codec);
                }
            }
        }

        // don't store twice
        formats_.clear();
        attribute_helper_.clear_attribute(rtpmap);
        attribute_helper_.clear_attribute(fmtp);  // parsed out in codec.parse
    }
#endif

    return codecs_;
}

static codec_t emptyCodec;

const codec_t& sdp_session_t::medium_t::find_first_matching_codecs(const codec_container_t& codecList, codec_t* pMatchingCodec) const
{
    const codec_container_t& internalCodecList = codecs();
    codec_container_t::const_iterator sIter;
    codec_container_t::const_iterator sEnd = internalCodecList.end();
    codec_container_t::const_iterator eIter;
    codec_container_t::const_iterator eEnd = codecList.end();
    for (eIter = codecList.begin(); eIter != eEnd; ++eIter)
    {
        for (sIter = internalCodecList.begin(); sIter != sEnd; ++sIter)
        {
            if (*sIter == *eIter)
            {
                if (pMatchingCodec)
                {
                    *pMatchingCodec = *eIter;
                }
                return *sIter;
            }
        }
    }
    return emptyCodec;
}

const codec_t& sdp_session_t::medium_t::find_first_matching_codecs(const medium_t& medium, codec_t* pMatchingCodec) const
{
    if (&medium == this)
    {
        return codecs().front();
    }
    else
    {
        return find_first_matching_codecs(medium.codecs(), pMatchingCodec);
    }
}

int sdp_session_t::medium_t::find_telephone_event_payload_type() const
{
    const codec_container_t& codecList = codecs();
    for (codec_container_t::const_iterator i = codecList.begin(); i != codecList.end(); i++)
    {
        if (i->name() == sdp_session_t::codec_t::TelephoneEvent.name())
        {
            return i->payload();
        }
    }
    return -1;
}

codec_t::codec_t(const std::string& name,
    int payload,
    unsigned long rate,
    const std::string& parameters,
    const std::string& encodingParameters)
    : name_(name),
    rate_(rate),
    payload_type_(payload),
    parameters_(parameters),
    encoding_parameters_(encodingParameters)
{}

codec_t::codec_t(const codec_t& rhs)
    : name_(rhs.name_),
    rate_(rhs.rate_),
    payload_type_(rhs.payload_type_),
    parameters_(rhs.parameters_),
    encoding_parameters_(rhs.encoding_parameters_)
{}

codec_t& codec_t::operator=(const codec_t& rhs)
{
    if (this != &rhs)
    {
        name_ = rhs.name_;
        rate_ = rhs.rate_;
        payload_type_ = rhs.payload_type_;
        parameters_ = rhs.parameters_;
        encoding_parameters_ = rhs.encoding_parameters_;
    }
    return *this;
}

void codec_t::parse(parse_buffer_t& pb, const sdp_session_t::medium_t& medium, int payload)
{
    const char* anchor = pb.skip_whitespace();
    pb.skip_to_char('/');
    name_ = pb.data(anchor);
    if (!pb.eof())
    {
        pb.skip_char('/');
        rate_ = pb.integer();
        pb.skip_to_char('/');
    }
    if (!pb.eof() && *pb.position() == '/')
    {
        anchor = pb.skip_char('/');
        pb.skip_to_end();
        encoding_parameters_ = pb.data(anchor);
    }
    payload_type_ = payload;

    assign_format_parameters(medium);
}

void codec_t::assign_format_parameters(const sdp_session_t::medium_t& medium)
{
    // get parameters if they exist
    if (medium.exists(fmtp))
    {
        for (std::list<std::string>::const_iterator i = medium.get_values(fmtp).begin(); i != medium.get_values(fmtp).end(); ++i)
        {
            try
            {
                parse_buffer_t pb(i->data(), i->size());
                int payload = pb.integer();
                if (payload == payload_type_)
                {
                    const char* anchor = pb.skip_whitespace();
                    pb.skip_to_end();
                    parameters_ = pb.data(anchor);
                    break;
                }
            }
            catch (const std::exception &)
            {
                throw;
                //InfoLog(<< "Caught exception when parsing a=fmtp: " << e);
            }
        }
    }
}

codec_t::codec_map_t& codec_t::get_static_codecs()
{
    if (!s_static_codecs_created_)
    {
        //
        // Build map of static codecs as defined in RFC 3551
        //
        s_static_codecs_ = std::auto_ptr<codec_map_t>(new codec_map_t);

        // Audio codecs
        s_static_codecs_->insert(std::make_pair(0, codec_t("PCMU", 0, 8000)));
        s_static_codecs_->insert(std::make_pair(3, codec_t("GSM", 3, 8000)));
        s_static_codecs_->insert(std::make_pair(4, codec_t("G723", 4, 8000)));
        s_static_codecs_->insert(std::make_pair(5, codec_t("DVI4", 5, 8000)));
        s_static_codecs_->insert(std::make_pair(6, codec_t("DVI4", 6, 16000)));
        s_static_codecs_->insert(std::make_pair(7, codec_t("LPC", 7, 8000)));
        s_static_codecs_->insert(std::make_pair(8, codec_t("PCMA", 8, 8000)));
        s_static_codecs_->insert(std::make_pair(9, codec_t("G722", 9, 8000)));
        s_static_codecs_->insert(std::make_pair(10, codec_t("L16-2", 10, 44100)));
        s_static_codecs_->insert(std::make_pair(11, codec_t("L16-1", 11, 44100)));
        s_static_codecs_->insert(std::make_pair(12, codec_t("QCELP", 12, 8000)));
        s_static_codecs_->insert(std::make_pair(13, codec_t("CN", 13, 8000)));
        s_static_codecs_->insert(std::make_pair(14, codec_t("MPA", 14, 90000)));
        s_static_codecs_->insert(std::make_pair(15, codec_t("G728", 15, 8000)));
        s_static_codecs_->insert(std::make_pair(16, codec_t("DVI4", 16, 11025)));
        s_static_codecs_->insert(std::make_pair(17, codec_t("DVI4", 17, 22050)));
        s_static_codecs_->insert(std::make_pair(18, codec_t("G729", 18, 8000)));

        // Video or audio/video codecs
        s_static_codecs_->insert(std::make_pair(25, codec_t("CelB", 25, 90000)));
        s_static_codecs_->insert(std::make_pair(26, codec_t("JPEG", 26, 90000)));
        s_static_codecs_->insert(std::make_pair(28, codec_t("nv", 28, 90000)));
        s_static_codecs_->insert(std::make_pair(31, codec_t("H261", 31, 90000)));
        s_static_codecs_->insert(std::make_pair(32, codec_t("MPV", 32, 90000)));
        s_static_codecs_->insert(std::make_pair(33, codec_t("MP2T", 33, 90000)));
        s_static_codecs_->insert(std::make_pair(34, codec_t("H263", 34, 90000)));

        s_static_codecs_created_ = true;
    }
    return *(s_static_codecs_.get());
}

bool xt_sdp::operator==(const codec_t& lhs, const codec_t& rhs)
{
    static std::string defaultEncodingParameters(std::string("1"));  // Default for audio streams (1-Channel)
    return (Helper::nocasecmp(lhs.name(), rhs.name()) && (lhs.rate() == rhs.rate()) &&
        (lhs.encoding_parameters() == rhs.encoding_parameters() ||
        (lhs.encoding_parameters().empty() && rhs.encoding_parameters() == defaultEncodingParameters) ||
        (lhs.encoding_parameters() == defaultEncodingParameters && rhs.encoding_parameters().empty())));
}

std::ostream& xt_sdp::operator<<(std::ostream& str, const codec_t& codec)
{
    str << codec.name();
    str << '/';
    str << codec.rate();
    if (!codec.encoding_parameters().empty())
    {
        str << '/';
        str << codec.encoding_parameters();
    }
    return str;
}

const codec_t codec_t::ULaw_8000("PCMU", 0, 8000);
const codec_t codec_t::ALaw_8000("PCMA", 8, 8000);
const codec_t codec_t::G729_8000("G729", 18, 8000);
const codec_t codec_t::G723_8000("G723", 4, 8000);
const codec_t codec_t::GSM_8000("GSM", 3, 8000);

const codec_t codec_t::TelephoneEvent("telephone-event", 101, 8000);
const codec_t codec_t::FrfDialedDigit("frf-dialed-event", 102, 8000);
const codec_t codec_t::CN("CN", 13, 8000);

bool codec_t::s_static_codecs_created_ = false;
std::auto_ptr<codec_t::codec_map_t> codec_t::s_static_codecs_;

