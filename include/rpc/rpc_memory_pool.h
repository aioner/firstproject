//1.modified by lichao,20151216 修改 boost::object_pool<>没有锁保护的，导致多线程环境中崩溃的bug
//
//
#ifndef _RPC_MEMORY_POOL_H_INCLUDED
#define _RPC_MEMORY_POOL_H_INCLUDED
#include "rpc_config.h"

#ifdef _USE_MEMORY_POOL
#include <boost/pool/singleton_pool.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/thread/mutex.hpp>
#endif

namespace rpc
{
    namespace memorypool
    {
        template<typename T>
        class use_singleton_impl
        {
        public:
#ifdef _USE_MEMORY_POOL
            typedef boost::singleton_pool<T, sizeof(T)> singleton_pool;
            static T *base_get_object()
            {
                T *px = static_cast<T *>((singleton_pool::malloc)());
                if (NULL != px)
                {
                    new (px) T;
                }
                return px;
            }

            static void base_free_object(T *px)
            {
                if (NULL != px)
                {
                    px->~T();
                    (singleton_pool::free)(px);
                }
            }
#else
            static T *base_get_object()
            {
                return new T;
            }

            static void base_free_object(T *px)
            {
                delete px;
            }
#endif
        };

        template<typename T>
        class use_impl
        {
        public:
#ifdef _USE_MEMORY_POOL
            T *base_get_object()
            {
                boost::mutex::scoped_lock _lock(mtx_);
                return pl_.construct();
            }

            template<typename T1>
            T *base_get_object(T1 arg1)
            {
                boost::mutex::scoped_lock _lock(mtx_);
                return pl_.construct(arg1);
            }

            template<typename T1, typename T2>
            T *base_get_object(T1 arg1, T2 arg2)
            {
                boost::mutex::scoped_lock _lock(mtx_);
                return pl_.construct(arg1, arg2);
            }

            template<typename T1, typename T2, typename T3>
            T *base_get_object(T1 arg1, T2 arg2, T3 arg3)
            {
                boost::mutex::scoped_lock _lock(mtx_);
                return pl_.construct(arg1, arg2, arg3);
            }

            template<typename T1, typename T2, typename T3, typename T4>
            T *base_get_object(T1 arg1, T2 arg2, T3 arg3, T4 arg4)
            {
                boost::mutex::scoped_lock _lock(mtx_);
                T *p = static_cast<T *>((pl_.malloc)());
                new (p) T(arg1, arg2, arg3, arg4);
                return p;
            }

            template<typename T1, typename T2, typename T3, typename T4, typename T5>
            T *base_get_object(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
            {
                boost::mutex::scoped_lock _lock(mtx_);
                T *p = static_cast<T *>((pl_.malloc)());
                new (p) T(arg1, arg2, arg3, arg4, arg5);
                return p;
            }

            void base_free_object(T *px)
            {
                boost::mutex::scoped_lock _lock(mtx_);
                pl_.destroy(px);
            }
        private:
            boost::object_pool<T> pl_;
            boost::mutex mtx_;
#else
            T *base_get_object()
            {
                return new T;
            }

            template<typename T1>
            T *base_get_object(T1 arg1)
            {
                return new T(arg1);
            }

            template<typename T1, typename T2>
            T *base_get_object(T1 arg1, T2 arg2)
            {
                return new T(arg1, arg2);
            }

            template<typename T1, typename T2, typename T3>
            T *base_get_object(T1 arg1, T2 arg2, T3 arg3)
            {
                return new T(arg1, arg2, arg3);
            }

            template<typename T1, typename T2, typename T3, typename T4>
            T *base_get_object(T1 arg1, T2 arg2, T3 arg3, T4 arg4)
            {
                return new T(arg1, arg2, arg3, arg4);
            }

            template<typename T1, typename T2, typename T3, typename T4, typename T5>
            T *base_get_object(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
            {
                return new T(arg1, arg2, arg3, arg4, arg5);
            }

            void base_free_object(T *px)
            {
                delete px;
            }
#endif//#ifdef _USE_MEMORY_POOL
        };

        template<typename DrivedT>
        class attach_impl
        {
        public:
            template<typename X>
            X *get_object()
            {
                return _get_drived()->use_impl<X>::base_get_object();
            }

            template<typename X, typename T1>
            X *get_object(T1 arg1)
            {
                return _get_drived()->use_impl<X>::base_get_object(arg1);
            }

            template<typename X, typename T1, typename T2>
            X *get_object(T1 arg1, T2 arg2)
            {
                return _get_drived()->use_impl<X>::base_get_object(arg1, arg2);
            }

            template<typename X, typename T1, typename T2, typename T3>
            X *get_object(T1 arg1, T2 arg2, T3 arg3)
            {
                return _get_drived()->use_impl<X>::base_get_object(arg1, arg2, arg3);
            }

            template<typename X, typename T1, typename T2, typename T3, typename T4>
            X *get_object(T1 arg1, T2 arg2, T3 arg3, T4 arg4)
            {
                return _get_drived()->use_impl<X>::base_get_object(arg1, arg2, arg3, arg4);
            }

            template<typename X, typename T1, typename T2, typename T3, typename T4, typename T5>
            X *get_object(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
            {
                return _get_drived()->use_impl<X>::base_get_object(arg1, arg2, arg3, arg4, arg5);
            }

            template<typename X>
            void free_object(X *px)
            {
                _get_drived()->use_impl<X>::base_free_object(px);
            }

            template<typename X>
            static X *get_object_singleton()
            {
#ifdef _WIN32
                return DrivedT::use_singleton_impl<X>::base_get_object();
#else
		return new X;
#endif//#ifdef _WIN32
            }

            template<typename X>
            static void free_object_singleton(X *px)
            {
#ifdef _WIN32
                DrivedT::use_singleton_impl<X>::base_free_object(px);
#else
		delete px;
#endif//#ifdef _WIN32

            }

        private:
            DrivedT *_get_drived()
            {
                return (DrivedT *)this;
            }
        };
    }
}

#endif //_RPC_MEMORY_POOL_H_INCLUDED
