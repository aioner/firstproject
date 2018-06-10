#ifndef _XT_MEDIA_CLIENT_RTP_BUF_H_INCLUDED
#define _XT_MEDIA_CLIENT_RTP_BUF_H_INCLUDED

#define RTP_PACKAGE_MAX_SIZE            2048
#include<stdint.h>

#include <string.h>

namespace xt_media_client
{
    class rtp_buf_t
    {
    public:
        rtp_buf_t(uint32_t capacity, uint32_t max_bound)
            :length_(0),
            capacity_(capacity),
            max_bound_(max_bound),
            data_(NULL)
        {
            data_ = static_cast<uint8_t *>(::malloc(capacity));
        }

        ~rtp_buf_t()
        {
            if (NULL != data_)
            {
                ::free(data_);
                data_ = 0;
                length_ = 0;
                capacity_ = 0;
            }
        }

        uint32_t write(const void *data, uint32_t len)
        {
            if ((len > unused_length()) && !resize(len))
            {
                return 0;
            }

            memcpy(unused_data(), data, len);
            seek(len);

            return len;
        }

        bool resize(uint32_t len)
        {
            uint32_t new_capacity = capacity_;

            do
            {
                new_capacity <<= 1;

                if (new_capacity > max_bound_)
                {
                    return false;
                }
            }
            while (new_capacity - length_ < len);

            data_ = static_cast<uint8_t *>(::realloc(data_, new_capacity));
            capacity_ = new_capacity;

            return true;
        }

        void rewind()
        {
            length_ = 0;
        }

        void seek(int32_t offset)
        {
            length_ += offset;
        }

        uint8_t *data()
        {
            return data_;
        }

        uint8_t *unused_data()
        {
            return data_ + length_;
        }

        uint32_t length() const
        {
            return length_;
        }

        uint32_t unused_length() const
        {
            return capacity_ - length_;
        }

        uint32_t capacity() const
        {
            return capacity_;
        }

    private:
        uint32_t length_;
        uint32_t capacity_;
        const uint32_t max_bound_;
        uint8_t *data_;
    };
}

#endif //_XT_MEDIA_CLIENT_RTP_BUF_H_INCLUDED
