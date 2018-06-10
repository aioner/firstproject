#ifndef _CLOSED_FLAG_H_INCLUDED
#define _CLOSED_FLAG_H_INCLUDED

#include <boost/atomic.hpp>

namespace xt_rtsp_client
{
    class closed_flag_t
    {
    public:
        closed_flag_t()
            :flag_(false)
        {}

        bool closed() const
        {
            return flag_;
        }

        void close()
        {
            flag_ = true;
        }

        void reset()
        {
            flag_ = false;
        }
    private:
        boost::atomic_bool flag_;
    };
}

#endif //_CLOSED_FLAG_H_INCLUDED
