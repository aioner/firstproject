#ifndef _SPINLOCK_H_INCLUDED
#define _SPINLOCK_H_INCLUDED

#include <boost/smart_ptr/detail/spinlock.hpp>

namespace xt_rtsp_client
{
    class spinlock_t : public boost::detail::spinlock
    {
    public:
        spinlock_t()
        {
            boost::detail::spinlock dummy = BOOST_DETAIL_SPINLOCK_INIT;
            *(boost::detail::spinlock *)this = dummy;
        }
    };
}

#endif //_SPINLOCK_H_INCLUDED
