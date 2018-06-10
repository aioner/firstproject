#include "parse_buffer.h"
#include <sstream>
#include <stdexcept>

using namespace xt_sdp;

const char* parse_buffer_t::param_term_ = ";?"; // maybe include "@>,"?
const char* parse_buffer_t::whitespace_ = " \t\r\n";
const std::string parse_buffer_t::pointer_t::msg_("dereferenced parse_buffer_t eof");

parse_buffer_t::parse_buffer_t(const char* buff, size_t len, const std::string& errorContext)
    : buf_(buff),
    position_(buff),
    end_(buff + len),
    error_context_(errorContext)
{}

parse_buffer_t::parse_buffer_t(const char* buff, const std::string& errorContext)
    : buf_(buff),
    position_(buff),
    end_(buff + strlen(buff)),
    error_context_(errorContext)
{}

parse_buffer_t::parse_buffer_t(const std::string& data, const std::string& errorContext)
    : buf_(data.data()),
    position_(buf_),
    end_(buf_ + data.size()),
    error_context_(errorContext)
{}

parse_buffer_t::parse_buffer_t(const parse_buffer_t& rhs)
    : buf_(rhs.buf_),
    position_(rhs.position_),
    end_(rhs.end_),
    error_context_(rhs.error_context_)
{}

parse_buffer_t& parse_buffer_t::operator=(const parse_buffer_t& rhs)
{
    buf_ = rhs.buf_;
    position_ = rhs.position_;
    end_ = rhs.end_;

    return *this;
}

parse_buffer_t::current_position_t parse_buffer_t::skip_char(char c)
{
    if (eof())
    {
        fail(__FILE__, __LINE__, "skipped over eof");
        return skip_to_end();
    }
    if (*position_ != c)
    {
        std::string msg("expected '");
        msg += c;
        msg += "'";
        fail(__FILE__, __LINE__, msg);
        return skip_to_end();
    }
    ++position_;
    return current_position_t(*this);
}

parse_buffer_t::current_position_t parse_buffer_t::skip_chars(const char* cs)
{
    const char* match = cs;
    while (*match != 0)
    {
        if (eof() || (*match != *position_))
        {
            std::string msg("Expected \"");
            msg += cs;
            msg += "\"";
            fail(__FILE__, __LINE__, msg);
            return skip_to_end();
        }
        match++;
        position_++;
    }
    return current_position_t(*this);
}

parse_buffer_t::current_position_t parse_buffer_t::skip_chars(const std::string& cs)
{
    const char* match = cs.data();
    for (std::string::size_type i = 0; i < cs.size(); i++)
    {
        if (eof() || (*match != *position_))
        {
            std::string msg("Expected \"");
            msg += cs;
            msg += "\"";
            fail(__FILE__, __LINE__, msg);
            return skip_to_end();
        }
        match++;
        position_++;
    }
    return current_position_t(*this);
}

parse_buffer_t::current_position_t parse_buffer_t::skip_non_whitespace()
{
    assert_not_eof();
    while (position_ < end_)
    {
        switch (*position_)
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            return current_position_t(*this);
        default:
            position_++;
        }
    }
    return current_position_t(*this);
}

parse_buffer_t::current_position_t parse_buffer_t::skip_whitespace()
{
    while (position_ < end_)
    {
        switch (*position_)
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        {
            position_++;
            break;
        }
        default:
            return current_position_t(*this);
        }
    }
    return current_position_t(*this);
}

// "SIP header field values can be folded onto multiple lines if the
//  continuation line begins with a space or horizontal tab"

// CR can be quote with \ within "" and comments -- treat \CR as whitespace
parse_buffer_t::current_position_t parse_buffer_t::skip_LWS()
{
    enum State { WS, CR, LF };
    State state = WS;
    while (position_ < end_)
    {
        char c = *position_++;
        if (c == '\\')
        {
            // treat escaped CR and LF as space
            c = *position_++;
            if (c == '\r' || c == '\n')
            {
                c = ' ';
            }
        }
        switch (*position_++)
        {
        case ' ':
        case '\t':
        {
            state = WS;
            break;
        }
        case '\r':
        {
            state = CR;
            break;
        }
        case '\n':
        {
            if (state == CR)
            {
                state = LF;
            }
            else
            {
                state = WS;
            }
            break;
        }
        default:
        {
            // terminating CRLF not skipped
            if (state == LF)
            {
                position_ -= 3;
            }
            else
            {
                position_--;
            }
            return current_position_t(*this);
        }
        }
    }
    return current_position_t(*this);
}

static std::string CRLF("\r\n");

parse_buffer_t::current_position_t parse_buffer_t::skip_to_termCRLF()
{
    while (position_ < end_)
    {
        skip_to_chars(CRLF);
        position_ += 2;
        if ((*position_ != ' ' &&
            *position_ != '\t' &&
            // check for \CRLF -- not terminating
            //           \\CRLF -- terminating
            ((position_ - 3 < buf_ || *(position_ - 3) != '\\') ||
            (position_ - 4 > buf_ && *(position_ - 4) == '\\'))))
        {
            position_ -= 2;
            return current_position_t(*this);
        }
    }
    return current_position_t(*this);
}

parse_buffer_t::current_position_t parse_buffer_t::skip_to_chars(const char* cs)
{
    assert(cs);
    unsigned int l = (unsigned int)strlen(cs);

    const char* rpos;
    const char* cpos;
    while (position_ < end_)
    {
        rpos = position_;
        cpos = cs;
        for (unsigned int i = 0; i < l; i++)
        {
            if (*cpos++ != *rpos++)
            {
                position_++;
                goto skip;
            }
        }
        return current_position_t(*this);
    skip:;
    }
    return current_position_t(*this);
}

parse_buffer_t::current_position_t parse_buffer_t::skip_to_chars(const std::string& sub)
{
    const char* begSub = sub.c_str();
    const char* endSub = sub.c_str() + sub.size();
    if (begSub == endSub)
    {
        fail(__FILE__, __LINE__, "parse_buffer_t::skip_to_chars() called with an "
            "empty string. Don't do this!");

        return skip_to_end();
    }

    while (true)
    {
    next:
        const char* searchPos = position_;
        const char* subPos = sub.c_str();

        while (subPos != endSub)
        {
            if (searchPos == end_)
            {
                // nope
                position_ = end_;
                return current_position_t(*this);
            }
            if (*subPos++ != *searchPos++)
            {
                // nope, but try the next position
                ++position_;
                goto next;
            }
        }
        // found a match
        return current_position_t(*this);
    }
}

bool parse_buffer_t::one_of(char c, const char* cs)
{
    while (*cs)
    {
        if (c == *(cs++))
        {
            return true;
        }
    }
    return false;
}

bool parse_buffer_t::one_of(char c, const std::string& cs)
{
    for (std::string::size_type i = 0; i < cs.size(); i++)
    {
        if (c == cs[i])
        {
            return true;
        }
    }
    return false;
}

parse_buffer_t::current_position_t parse_buffer_t::skip_to_one_of(const char* cs)
{
    while (position_ < end_)
    {
        if (one_of(*position_, cs))
        {
            return current_position_t(*this);
        }
        else
        {
            position_++;
        }
    }
    return current_position_t(*this);
}

parse_buffer_t::current_position_t parse_buffer_t::skip_to_one_of(const char* cs1, const char* cs2)
{
    while (position_ < end_)
    {
        if (one_of(*position_, cs1) ||
            one_of(*position_, cs2))
        {
            return current_position_t(*this);
        }
        else
        {
            position_++;
        }
    }
    return current_position_t(*this);
}

parse_buffer_t::current_position_t parse_buffer_t::skip_to_one_of(const std::string& cs)
{
    while (position_ < end_)
    {
        if (one_of(*position_, cs))
        {
            return current_position_t(*this);
        }
        else
        {
            position_++;
        }
    }
    return current_position_t(*this);
}

parse_buffer_t::current_position_t parse_buffer_t::skip_to_one_of(const std::string& cs1, const std::string& cs2)
{
    while (position_ < end_)
    {
        if (one_of(*position_, cs1) ||
            one_of(*position_, cs2))
        {
            return current_position_t(*this);
        }
        else
        {
            position_++;
        }
    }
    return current_position_t(*this);
}

const char* parse_buffer_t::skip_to_end_quote(char quote)
{
    while (position_ < end_)
    {
        // !dlb! mark character encoding
        if (*position_ == '\\')
        {
            position_ += 2;
        }
        else if (*position_ == quote)
        {
            return position_;
        }
        else
        {
            position_++;
        }
    }

   {
       std::string msg("Missing '");
       msg += quote;
       msg += "'";
       fail(__FILE__, __LINE__, msg);
   }
   return 0;
}

const char* parse_buffer_t::skip_back_char()
{
    if (bof())
    {
        fail(__FILE__, __LINE__, "backed over beginning of buffer");
    }
    position_--;
    return position_;
}

const char* parse_buffer_t::skip_back_whitespace()
{
    while (!bof())
    {
        switch (*(--position_))
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        {
            break;
        }
        default:
            return ++position_;
        }
    }
    return buf_;
}

// abcde
//     ^
// skip_back_char('d');
// abcde
//    ^
// skip_char('d');
// abcde
//     ^
const char* parse_buffer_t::skip_back_char(char c)
{
    if (bof())
    {
        fail(__FILE__, __LINE__, "backed over beginning of buffer");
    }
    if (*(--position_) != c)
    {
        std::string msg("Expected '");
        msg += c;
        msg += "'";
        fail(__FILE__, __LINE__, msg);
    }
    return position_;
}

// abcde
//      ^
// skip_back_to_char('c');
// abcde
//    ^
const char* parse_buffer_t::skip_back_to_char(char c)
{
    while (!bof())
    {
        if (*(--position_) == c)
        {
            return ++position_;
        }
    }
    return buf_;
}

const char* parse_buffer_t::skip_back_to_one_of(const char* cs)
{
    while (!bof())
    {
        if (one_of(*(--position_), cs))
        {
            return ++position_;
        }
    }
    return buf_;
}

void parse_buffer_t::data(std::string& data, const char* start) const
{
    if (!(buf_ <= start && start <= position_))
    {
        fail(__FILE__, __LINE__, "Bad anchor position");
    }

    data.assign(const_cast<char*>(start), (unsigned int)(position_ - start));
}

static const unsigned char hexToByte[256] =
{
    // 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//0
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//1
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//2
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 'k', 'k', 'k', 'k', 'k', 'k', //3
    'k', 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', //4
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//5
    'k', 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', //6
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//8
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//9
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//a
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//b
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//c
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//d
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k',//e
    'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k', 'k'   //f
};

void parse_buffer_t::data_unescaped(std::string& dataToUse, const char* start) const
{
    if (!(buf_ <= start && start <= position_))
    {
        fail(__FILE__, __LINE__, "Bad anchor position");
    }

   {
       const char* current = start;
       while (current < position_)
       {
           if (*current == '%')
           {
               // needs to be unencoded
               goto copy;
           }
           current++;
       }
       // can use an overlay
       data(dataToUse, start);
       return;
   }

copy:
   if ((size_t)(position_ - start) > dataToUse.capacity())
   {
       dataToUse.resize(position_ - start, false);
   }

   char* target = &(dataToUse[0]);
   const char* current = start;
   while (current < position_)
   {
       if (*current == '%')
       {
           current++;
           if (position_ - current < 2)
           {
               fail(__FILE__, __LINE__, "Illegal escaping");
           }
           const char high = hexToByte[(unsigned char)*current];
           const char low = hexToByte[(unsigned char)*(current + 1)];
           if (high != 'k' && low != 'k')
           {
               unsigned char escaped = 0;
               escaped = high << 4 | low;
               // !bwc! I think this check is bogus, especially the ':' (58) check
               // You could maybe argue that the point of %-escaping is to allow
               // the use of UTF-8 data (including ASCII that is not allowed in an 
               // on-the-wire representation of whatever it is we're unescaping),
               // and not unprintable characters (the unprintable codes are not 
               // used by UTF-8). 
               if (escaped > 31 && escaped != 127 && escaped != 58)
               {
                   *target++ = escaped;
                   current += 2;
               }
               else
               {
                   *target++ = '%';
                   *target++ = *current++;
                   *target++ = *current++;
               }
           }
           else
           {
               fail(__FILE__, __LINE__, "Illegal escaping, not hex");
           }
       }
       else
       {
           *target++ = *current++;
       }
   }
   *target = 0;
   dataToUse.resize(target - dataToUse.c_str());
}

std::string parse_buffer_t::data(const char* start) const
{
    if (!(buf_ <= start && start <= position_))
    {

        fail(__FILE__, __LINE__, "Bad anchor position");
    }

    std::string data(start, position_ - start);
    return data;
}

int parse_buffer_t::integer()
{
    if (this->eof())
    {
        fail(__FILE__, __LINE__, "Expected a digit, got eof ");
    }

    int signum = 1;
    if (*position_ == '-')
    {
        signum = -1;
        ++position_;
        assert_not_eof();
    }
    else if (*position_ == '+')
    {
        ++position_;
        assert_not_eof();
    }

    if (!isdigit(*position_))
    {
        std::string msg("Expected a digit, got: ");
        msg += std::string(position_, (end_ - position_));
        fail(__FILE__, __LINE__, msg);
    }

    int num = 0;
    int last = 0;
    while (!eof() && isdigit(*position_))
    {
        last = num;
        num = num * 10 + (*position_ - '0');
        if (last > num)
        {
            fail(__FILE__, __LINE__, "Overflow detected.");
        }
        ++position_;
    }

    return signum*num;
}

uint8_t parse_buffer_t::to_uint8()
{
    const char* begin = position_;
    uint8_t num = 0;
    uint8_t last = 0;
    while (!eof() && isdigit(*position_))
    {
        last = num;
        num = num * 10 + (*position_ - '0');
        if (last > num)
        {
            fail(__FILE__, __LINE__, "Overflow detected.");
        }
        ++position_;
    }

    if (position_ == begin)
    {
        fail(__FILE__, __LINE__, "Expected a digit");
    }
    return num;
}


//!dcm! -- merge these, ask about length checks
uint32_t parse_buffer_t::to_uint32()
{
    const char* begin = position_;
    uint32_t num = 0;
    while (!eof() && isdigit(*position_))
    {
        num = num * 10 + (*position_ - '0');
        ++position_;
    }

    switch (position_ - begin)
    {
    case 0:
        fail(__FILE__, __LINE__, "Expected a digit");
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        break;
    case 10:
        if (*begin < '4')
        {
            break;
        }
        else if (*begin == '4' && num >= 4000000000UL)
        {
            break;
        }
    default:
        fail(__FILE__, __LINE__, "Overflow detected");
    }

    return num;
}

uint64_t parse_buffer_t::to_uint64()
{
    const char* begin = position_;
    uint64_t num = 0;
    while (!eof() && isdigit(*position_))
    {
        num = num * 10 + (*position_ - '0');
        ++position_;
    }

    switch (position_ - begin)
    {
    case 0:
        fail(__FILE__, __LINE__, "Expected a digit");
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        break;
    case 20:
        if (*begin == '1' && num >= 10000000000000000000ULL)
        {
            break;
        }
    default:
        fail(__FILE__, __LINE__, "Overflow detected");
    }

    return num;
}

#ifndef XT_SDP_FIXED_POINT
float parse_buffer_t::float_val()
{
    const char* s = position_;
    try
    {
        float mant = 0.0;
        int num = integer();

        if (*position_ == '.')
        {
            skip_char();
            const char* pos = position_;
            mant = float(integer());
            int s = int(position_ - pos);
            while (s--)
            {
                mant /= 10.0;
            }
        }
        return num + mant;
    }
    catch (...)
    {
        std::string msg("Expected a floating point value, got: ");
        msg += std::string(s, position_ - s);
        fail(__FILE__, __LINE__, msg);
        return 0.0;
    }
}
#endif

int parse_buffer_t::q_val()
{
    // parse a qvalue into an integer between 0 and 1000  (ex: 1.0 -> 1000,  0.8 -> 800, 0.05 -> 50)
    const char* s = position_;
    try
    {
        int num = integer();
        if (num == 1)
        {
            num = 1000;
        }
        else if (num != 0)
        {
            // error: qvalue must start with 1 or 0
            return 0;
        }

        if (*position_ == '.')
        {
            skip_char();

            int i = 100;
            while (!eof() && isdigit(*position_) && i)
            {
                num += (*position_ - '0') * i;
                i /= 10;
                skip_char();
            }
        }
        return num;
    }
    catch (...)
    {
        std::string msg("Expected a floating point value, got: ");
        msg += std::string(s, position_ - s);
        fail(__FILE__, __LINE__, msg);
        return 0;
    }
}

std::string spaces(unsigned int numSpaces)
{
    return std::string(numSpaces, ' ');
}

std::string escape_and_annotate(const char* buffer,
    std::string::size_type size,
    const char* position)
{
    std::string ret(2 * size + 16, '\0');

    const char* lastReturn = buffer;
    int lineCount = 0;
    bool doneAt = false;

    const char* p = buffer;
    for (unsigned int i = 0; i < size; i++)
    {
        unsigned char c = *p++;

        switch (c)
        {
        case 0x0D: // CR
        {
            continue;
        }
        case 0x0A: // LF
        {
            if (!doneAt && p >= position)
            {
                ret += "[CRLF]\n";
                ret += spaces((unsigned int)(position - lastReturn));
                ret += "^[CRLF]\n";
                doneAt = true;
            }
            else
            {
                lastReturn = p;
                ret += c;
            }
            lineCount++;
            continue;
        }
        }

        if (iscntrl(c) || (c >= 0x7F))
        {
            ret += '*'; // indicates unprintable character
            continue;
        }

        ret += c;
    }
    if (!doneAt && p >= position)
    {
        ret += "\n";
        ret += spaces((unsigned int)(position - lastReturn));
        ret += "^\n";
    }

    return ret;
}

void parse_buffer_t::fail(const char* file, unsigned int line, const std::string& detail) const
{
    return;
    std::ostringstream ds;
    ds << file << ":" << line
        << ", Parse failed ";

    if (!detail.empty()) ds << detail << ' ';

    ds << "in context: " << error_context_
        << std::endl
        << escape_and_annotate(buf_, end_ - buf_, position_);

    ds.flush();

    throw std::runtime_error(ds.str());
}

parse_buffer_t::pointer_t::pointer_t(const parse_buffer_t& pb,
    const char* position,
    bool atEof)
    : pb_(pb),
    position_(position),
    is_valid_(!atEof)
{}

parse_buffer_t::pointer_t::pointer_t(const current_position_t& pos) :
pb_(pos.pb_),
position_(pos),
is_valid_(pos.pb_.valid())
{}

const char& parse_buffer_t::pointer_t::operator*() const
{
    if (is_valid_)
    {
        return *position_;
    }
    else
    {
        throw std::runtime_error(msg_ + pb_.get_context() + __FILE__);
    }
}
