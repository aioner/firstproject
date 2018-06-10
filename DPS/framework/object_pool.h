#ifndef _OBJECT_POOL_H_INCLUDED
#define _OBJECT_POOL_H_INCLUDED

#include "boost/pool/singleton_pool.hpp"
#include "framework/allocators.h"

namespace framework
{
    struct user_allocator_sync_shared_t
    {
        typedef std::size_t size_type; 
        typedef std::ptrdiff_t difference_type; 

        static char * (malloc)(const size_type bytes)
        {
            return static_cast<char *>(allocators::sync_shared_allocator_t::allocate(bytes));
        }

        static void (free)(char * const block)
        {
            allocators::sync_shared_allocator_t::deallocate(block);
        }
    };

    template<typename T>
    class object_pool_t
    {
    public:
        typedef boost::singleton_pool<T, sizeof(T)> pool_t;

        static T *create()
        {
            return new ((pool_t::malloc)()) T;
        }

        template<typename Arg0>
        static T *create(Arg0 arg0)
        {
            return new ((pool_t::malloc)()) T(arg0);
        }

        template<typename Arg0, typename Arg1>
        static T *create(Arg0 arg0, Arg1 arg1)
        {
            return new ((pool_t::malloc)()) T(arg0, arg1);
        }

        template<typename Arg0, typename Arg1, typename Arg2>
        static T *create(Arg0 arg0, Arg1 arg1, Arg2 arg2)
        {
            return new ((pool_t::malloc)()) T(arg0, arg1, arg2);
        }

        template<typename Arg0, typename Arg1, typename Arg2, typename Arg3>
        static T *create(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3)
        {
            return new ((pool_t::malloc)()) T(arg0, arg1, arg2, arg3);
        }

        template<typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        static T *create(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
        {
            return new ((pool_t::malloc)()) T(arg0, arg1, arg2, arg3, arg4);
        }

        template<typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        static T *create(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
        {
            return new ((pool_t::malloc)()) T(arg0, arg1, arg2, arg3, arg4, arg5);
        }

        template<typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        static T *create(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
        {
            return new ((pool_t::malloc)()) T(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
        }

        static void destroy(T* const p)
        {
            p->~T();
            (pool_t::free)(p);
        }
    };
}

#endif //_OBJECT_POOL_H_INCLUDED
