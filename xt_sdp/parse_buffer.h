#ifndef _PARSE_BUFFER_H_INCLUDED
#define _PARSE_BUFFER_H_INCLUDED

#include <string>
#include <assert.h>
#include <bitset>
#include <stdint.h>
#include <string.h>

namespace xt_sdp
{
    class parse_buffer_t
    {
    public:
        parse_buffer_t(const char* buff, size_t len, const std::string& errorContext = "");

        explicit parse_buffer_t(const char* buff, const std::string& errorContext = "");

        explicit parse_buffer_t(const std::string& data, const std::string& errorContext = "");

        parse_buffer_t(const parse_buffer_t& other);

    private:
        class current_position_t
        {
        public:
            inline explicit current_position_t(const parse_buffer_t& pb) :
                pb_(pb)
            {}

            operator const char*() const
            {
                return pb_.position_;
            }

            const char& operator*() const
            {
                pb_.assert_not_eof();
                return *pb_.position_;
            }

            const parse_buffer_t& pb_;
        };

        class pointer_t
        {
        public:
            pointer_t(const parse_buffer_t& pb, const char* position, bool atEof);
            pointer_t(const current_position_t& pos);

            operator const char*() const
            {
                return position_;
            }

            const char& operator*() const;
        private:
            const parse_buffer_t& pb_;
            const char* position_;
            const bool is_valid_;
            static const std::string msg_;
        };

    public:
        const std::string& get_context() const { return error_context_; }

        parse_buffer_t& operator=(const parse_buffer_t& other);
        void reset(const char* pos)
        {
            assert(buf_ <= end_);
            assert((pos >= buf_) && (pos <= end_));
            position_ = pos;
        }

        bool eof() const { return position_ >= end_; }
        bool bof() const { return position_ <= buf_; }
        bool valid() const { return (!eof()) && (!bof()); }
        pointer_t start() const { return pointer_t(*this, buf_, eof()); }
        current_position_t position() const { return current_position_t(*this); }
        pointer_t end() const { return pointer_t(*this, end_, true); }
        size_t length_remaining() { return end_ - position_; }

        current_position_t skip_char()
        {
            if (eof())
            {
                fail(__FILE__, __LINE__, "skipped over eof");
            }
            ++position_;
            return current_position_t(*this);
        }

        current_position_t skip_char(char c);
        current_position_t skip_chars(const char* cs);
        current_position_t skip_chars(const std::string& cs);
        current_position_t skip_non_whitespace();
        current_position_t skip_whitespace();
        current_position_t skip_LWS();
        current_position_t skip_to_termCRLF();
        current_position_t skip_to_char(char c)
        {
            position_ = (const char*)memchr(position_, c, end_ - position_);
            if (!position_)
            {
                position_ = end_;
            }
            return current_position_t(*this);
        }
        current_position_t skip_to_chars(const char* cs);
        current_position_t skip_to_chars(const std::string& cs);
        current_position_t skip_to_one_of(const char* cs);
        current_position_t skip_to_one_of(const char* cs1, const char* cs2);
        current_position_t skip_to_one_of(const std::string& cs);
        current_position_t skip_to_one_of(const std::string& cs1, const std::string& cs2);

        // std::bitset based parse function
        current_position_t skip_chars(const std::bitset<256>& cs)
        {
            while (position_ < end_)
            {
                if (cs.test((unsigned char)(*position_)))
                {
                    position_++;
                }
                else
                {
                    return current_position_t(*this);
                }
            }
            return current_position_t(*this);
        }

        current_position_t skip_to_one_of(const std::bitset<256>& cs)
        {
            while (position_ < end_)
            {
                if (cs.test((unsigned char)(*position_)))
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

        const char* skip_to_end_quote(char quote = '"');
        current_position_t skipN(int count)
        {
            position_ += count;
            if (position_ > end_)
            {
                fail(__FILE__, __LINE__, "skipped eof");
            }
            return current_position_t(*this);
        }

        current_position_t skip_to_end()
        {
            position_ = end_;
            return current_position_t(*this);
        }

        // inverse of skip_char() -- end up at char not before it
        const char* skip_back_char();
        const char* skip_back_whitespace();
        const char* skip_backN(int count)
        {
            position_ -= count;
            if (bof())
            {
                fail(__FILE__, __LINE__, "backed over beginning of buffer");
            }
            return position_;
        }

        const char* skip_back_char(char c);
        const char* skip_back_to_char(char c);
        const char* skip_back_to_one_of(const char* cs);

        void assert_eof() const
        {
            if (!eof())
            {
                fail(__FILE__, __LINE__, "expected eof");
            }
        }

        void assert_not_eof() const
        {
            if (eof())
            {
                fail(__FILE__, __LINE__, "unexpected eof");
            }
        }

        void fail(const char* file, unsigned int line, const std::string& errmsg = "") const;

        /// make the passed in data share memory with the buffer (uses std::string::Share)
        void data(std::string& data, const char* start) const;

        std::string data(const char* start) const;

        void data_unescaped(std::string& data, const char* start) const;

        int integer();

        uint8_t to_uint8();
        uint32_t to_uint32();
        uint64_t to_uint64();

#ifndef XT_SDP_FIXED_POINT
        float float_val();
#endif
        int q_val();

        static bool one_of(char c, const char* cs);
        static bool one_of(char c, const std::string& cs);
        static const char* whitespace_;
        static const char* param_term_;
    private:
        friend class parse_buffer_t::current_position_t;
        const char* buf_;
        const char* position_;
        const char* end_;
        const std::string& error_context_;
    };
}
#endif  //_PARSE_BUFFER_H_INCLUDED
