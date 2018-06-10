#ifndef _MTS_FRAMEWORK_DETAIL_THREAD_CREATOR_H_INCLUDED
#define _MTS_FRAMEWORK_DETAIL_THREAD_CREATOR_H_INCLUDED

#include "utility/thread.h"
#include "utility/function.h"
#include <boost/shared_ptr.hpp>
#include <memory>

namespace framework
{
    namespace detail
    {
        class thread_create_policy
        {
        public:
            template<typename F>
            void create_and_start_thread(const F& f)
            {
                sp_thread_.reset(new utility::thread(f));
            }

            ~thread_create_policy()
            {
                if (sp_thread_)
                {
                    sp_thread_->join();
                }
            }

            bool is_ok() const
            {
                return true;
            }
        protected:
#ifdef _WIN32
            std::tr1::shared_ptr<utility::thread> sp_thread_;
#else         
			boost::shared_ptr<utility::thread> sp_thread_;
#endif
        };

        class threadgroup_create_policy
        {
        public:
            typedef utility::thread_group thread_group_type;

            threadgroup_create_policy()
                :threadgroup_(NULL)
            {}

            void set_threadgroup(thread_group_type *tg)
            {
                threadgroup_ = tg;
            }

            thread_group_type *get_threadgroup()
            {
                return threadgroup_;
            }

            template<typename F>
            void create_and_start_thread(const F& f)
            {
                get_threadgroup()->create_thread(f);
            }

            bool is_ok() const
            {
                return (NULL != threadgroup_);
            }
        protected:
             thread_group_type*threadgroup_;
        };

        template<typename ThreadCreatorPolicy = thread_create_policy>
        struct thread_creator
        {
            template<typename DrivedT>
            class static_ : public ThreadCreatorPolicy
            {
            public:
                typedef DrivedT drived_type;

                bool start_thread()
                {
                    return ThreadCreatorPolicy::is_ok() ? ThreadCreatorPolicy::create_and_start_thread(utility::bind(&drived_type::on_run_thread, get_drived())), true : false;
                }

                void on_run_thread();
            protected:
                drived_type *get_drived()
                {
                    return (drived_type *)this;
                }
            };

            class dynamic_ : public ThreadCreatorPolicy
            {
            public:
                bool start_thread()
                {
                    if (!ThreadCreatorPolicy::is_ok())
                    {
                        return false;
                    }
                    ThreadCreatorPolicy::create_and_start_thread(utility::bind(&dynamic_::on_run_thread, this));
                    return true;
                }
            protected:
                virtual void on_run_thread() = 0;
            };
        };

        class null_thread_creator
        {
        public:
            void start_thread() {}
        };
    }
}

#endif // _MTS_FRAMEWORK_THREAD_CREATOR_H_INCLUDED
