#ifndef _MTS_UTILITY_MEMORY_POOL_H_INCLUDED
#define _MTS_UTILITY_MEMORY_POOL_H_INCLUDED

#include "config.h"

#ifdef _USE_BOOST
#include <boost/pool/pool.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <boost/pool/object_pool.hpp>
#endif

/*
    pool,
    object_pool,
    singleton_pool
*/

namespace utility
{
#ifdef _USE_BOOST
    using namespace boost;
#endif

    template<typename T>
    class singleton_object_pool_default_tag {};

    template<typename T, typename Tag = singleton_object_pool_default_tag<T> >
    class singleton_object_pool : public singleton_pool<Tag, sizeof(T)>
    {
    public:
        typedef T element_type;
        typedef singleton_pool<Tag, sizeof(T)> base_type;

        static element_type *(malloc)()
        {
            return (element_type *)(base_type::malloc)();
        }

        static element_type *construct()
        {
            element_type *obj = (malloc)();
            if (NULL == obj)
            {
                return NULL;
            }

            try
            {
                new (obj) T;
            }
            catch (...)
            {
                (free)(obj);
                throw;
            }
            return obj;
        }

        template<typename U>
        static element_type *construct(U& arg0)
        {
            element_type *obj = (malloc)();
            if (NULL == obj)
            {
                return NULL;
            }

            try
            {
                new (obj) T(arg0);
            }
            catch (...)
            {
                (free)(obj);
                throw;
            }
            return obj;
        }

        template<typename U>
        static element_type *construct(const U& arg0)
        {
            element_type *obj = (malloc)();
            if (NULL == obj)
            {
                return NULL;
            }

            try
            {
                new (obj) T(arg0);
            }
            catch (...)
            {
                (free)(obj);
                throw;
            }
            return obj;
        }

        template<typename U>
        static element_type *construct(volatile U& arg0)
        {
            element_type *obj = (malloc)();
            if (NULL == obj)
            {
                return NULL;
            }

            try
            {
                new (obj) T(arg0);
            }
            catch (...)
            {
                (free)(obj);
                throw;
            }
            return obj;
        }

        template<typename U>
        static element_type *construct(const volatile U& arg0)
        {
            element_type *obj = (malloc)();
            if (NULL == obj)
            {
                return NULL;
            }

            try
            {
                new (obj) T(arg0);
            }
            catch (...)
            {
                (free)(obj);
                throw;
            }
            return obj;
        }

        static void destroy(element_type * const obj)
        {
            obj->~T();
            (free)(obj);
        }
    };
}
#endif //_MTS_UTILITY_MEMORY_POOL_H_INCLUDED
