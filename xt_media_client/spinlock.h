#ifndef _SPINLOCK_H_INCLUDED
#define _SPINLOCK_H_INCLUDED

#include <boost/smart_ptr/detail/spinlock.hpp>

namespace xt_media_client
{
    class spinlock_t : public boost::detail::spinlock
    {
    public:
        spinlock_t()
        {
            boost::detail::spinlock fake = BOOST_DETAIL_SPINLOCK_INIT;
            *(boost::detail::spinlock *)this = fake;
        }
    };
}

#endif //_SPINLOCK_H_INCLUDED
