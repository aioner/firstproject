#ifndef _ALLOCATORS_H_INCLUED
#define _ALLOCATORS_H_INCLUED

#include <stdint.h>
#include <string>

#include "boost/thread/recursive_mutex.hpp"

namespace framework { namespace allocators {
    class freelist_t
    {
    public:
        freelist_t()
            :first_(NULL)
        {}

        void *pop()
        {
            void *ptr = NULL;
            if (NULL != first_)
            {
                ptr = first_;
                first_ = *reinterpret_cast<void **>(ptr);
            }
            return ptr;
        }

        bool push(void *ptr)
        {
            *reinterpret_cast<void **>(ptr) = first_;
            first_ = ptr;
            return true;
        }

    protected:
        void *first_;
    };

    struct default_buf_policy_t
    {
        size_t length;

        template<typename Allocator>
        static void *create(Allocator *alloc, size_t length)
        {
            default_buf_policy_t *buf = reinterpret_cast<default_buf_policy_t *>(alloc->allocate(sizeof(default_buf_policy_t) + length));
            if (NULL == buf)
            {
                return NULL;
            }

            buf->length = length;
            return ++buf;
        }

        template<typename Allocator>
        static bool destroy(Allocator *alloc, void *ptr)
        {
            default_buf_policy_t *buf = reinterpret_cast<default_buf_policy_t *>(ptr);
            if (NULL == buf)
            {
                return true;
            }

            buf--;
            return alloc->deallocate(buf, buf->length + sizeof(default_buf_policy_t));
        }

        static size_t ptr_length(void *ptr)
        {
            default_buf_policy_t *buf = reinterpret_cast<default_buf_policy_t *>(ptr);
            buf--;
            return buf->length;
        }
    };

    class malloc_allocator_t
    {
    public:
        static void *allocate(size_t size)
        {
            return malloc(size);
        }

        static bool deallocate(void *ptr, size_t)
        {
            free(ptr);
            return true;
        }

        static void free_all()
        {}
    };

    class new_allocator_t
    {
    public:
        static void *allocate(size_t size)
        {
            return new char[size];
        }

        static bool deallocate(void *ptr, size_t)
        {
            delete [] static_cast<char *>(ptr);
            return true;
        }

        static void free_all()
        {}
    };

    template<size_t Max, size_t N>
    class fixed_freelist_t
    {
    public:
        fixed_freelist_t()
        {
            for (size_t index = 0; index < N; ++index)
            {
                cache_.push(fixed_buf_ + index * Max);
            }
        }

        void *allocate(size_t size)
        {
            if (size > Max)
            {
                return NULL;
            }

            void *ptr = cache_.pop();
            if (NULL == ptr)
            {
                ptr = malloc_allocator_t::allocate(size);
            }

            return ptr;
        }

        bool deallocate(void *ptr, size_t size)
        {
            if (size > Max)
            {
                return false;
            }

            return cache_.push(ptr);
        }

        void free_all()
        {
            void *ptr = NULL;
            while (NULL != (ptr = cache_.pop()))
            {
                if (!is_from_fixed(ptr))
                {
                    malloc_allocator_t::deallocate(ptr, 0);
                }
            }
        }

    private:
        bool is_from_fixed(void *ptr)
        {
            return ((ptr >= fixed_buf_) && (ptr <= fixed_buf_ + Max * N));
        }

        char fixed_buf_[Max * N];
        freelist_t cache_;
    };


    template<typename Allocator, typename BufPolicy = default_buf_policy_t>
    class buf_allocator_t
    {
    public:
        typedef BufPolicy buf_policy_t;

        void *allocate(size_t size)
        {
            return buf_policy_t::create(&allocator_, size);
        }

        bool deallocate(void *ptr)
        {
            return buf_policy_t::destroy(&allocator_, ptr);
        }

        void free_all()
        {
            allocator_.free_all();
        }

    private:
        Allocator allocator_;
    };

    template<size_t Max, typename Base>
    class allocator_t : public Base
    {
    public:
        typedef Base base_t;

        ~allocator_t()
        {
            free_all();
        }

        void *allocate(size_t size)
        {
            void *ptr = allocator_.allocate(size);
            if (NULL != ptr)
            {
                return ptr;
            }
            return get_base()->allocate(size);
        }

        bool deallocate(void *ptr)
        {
            if (allocator_.deallocate(ptr))
            {
                return true;
            }
            return get_base()->deallocate(ptr);
        }

        void free_all()
        {
            allocator_.free_all();
            get_base()->free_all();
        }

    private:
        base_t *get_base()
        {
            return static_cast<base_t *>(this);
        }

        buf_allocator_t<fixed_freelist_t<Max, Max >= 2048 ? 1 : (2048 / Max)>, typename base_t::buf_policy_t> allocator_;
    };

    template<typename Cache>
    class sync_none
    {
    public:
        typedef typename Cache::buf_policy_t buf_policy_t;

        static void *allocate(size_t size)
        {
            return cache_.allocate(size);
        }

        static void deallocate(void *ptr)
        {
            cache_.deallocate(ptr);
        }
    private:
        static Cache cache_;
    };

    template<typename Cache>
    Cache sync_none<Cache>::cache_;

    template<typename Cache>
    class sync_shared
    {
    public:
        typedef typename Cache::buf_policy_t buf_policy_t;

        static void *allocate(size_t size)
        {
            boost::recursive_mutex::scoped_lock _lock(mtx_);
            return cache_.allocate(size);
        }

        static void deallocate(void *ptr)
        {
            boost::recursive_mutex::scoped_lock _lock(mtx_);
            cache_.deallocate(ptr);
        }

    private:
        static boost::recursive_mutex mtx_;
        static Cache cache_;
    };

    template<typename Cache>
    Cache sync_shared<Cache>::cache_;

    template<typename Cache>
    boost::recursive_mutex sync_shared<Cache>::mtx_;

    typedef allocator_t<16, allocator_t<64, allocator_t<128, allocator_t<512, allocator_t<1024, allocator_t<2048, buf_allocator_t<malloc_allocator_t> > > > > > > segment_allocator_t;
    typedef sync_shared<segment_allocator_t> sync_shared_allocator_t;
    typedef sync_none<segment_allocator_t> sync_none_allocator_t;
} }

#endif //_ALLOCATORS_H_INCLUED
