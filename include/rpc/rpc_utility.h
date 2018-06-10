#ifndef _RPC_UTILITY_H_INCLUDED
#define _RPC_UTILITY_H_INCLUDED

#include "rpc_memory_pool.h"
#include "rpc_serialize.h"

namespace rpc
{
    namespace utility
    {
        template<typename DrivedT>
        class drived_base
        {
        public:
            typedef DrivedT drived_type;

            drived_type *get_drived()
            {
                return (drived_type *)this;
            }

            drived_type * const get_drived() const
            {
                return (drived_type * const)this;
            }
        };

        template<typename D, typename S>
        D bitwise_cast(S s)
        {
            union
            {
                D d_;
                S s_;
            } convertor;
            convertor.s_ = s;
            return convertor.d_;
        }

        template<typename DrivedT>
        class allocate_wrapper_t : 
            public drived_base<DrivedT>, 
            public memorypool::use_singleton_impl<DrivedT>, 
            public memorypool::attach_impl<allocate_wrapper_t<DrivedT> >
        {
        public:
            typedef DrivedT drived_type;
            typedef memorypool::attach_impl<allocate_wrapper_t<DrivedT> > base_t;
            static void *allocate()
            {
                return base_t::template get_object_singleton<drived_type>();
            }

            static void deallocate(void *p)
            {
                base_t::template free_object_singleton<drived_type>((drived_type *)p);
            }

            static bool out_object(void *p, serialization::type which, std::ostream& os)
            {
                drived_type *px = static_cast<drived_type *>(p);
                if (NULL != px)
                {
                    serialization::serialize_object(which, *px, os);
                }
                return true;
            }

            static bool in_object(void *p, serialization::type which, std::istream& is)
            {
                drived_type *px = static_cast<drived_type *>(p);
                if (NULL != px)
                {
                    serialization::serialize_object(which, *px, is);
                }
                return true;
            }
        };
    }
}

#endif //_RPC_UTILITY_H_INCLUDED
