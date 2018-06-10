#ifndef _MTS_FRAMEWORK_DETAIL_SERVICE_H_INCLUDED
#define _MTS_FRAMEWORK_DETAIL_SERVICE_H_INCLUDED

#include "singleton.h"
#include "thread_creator.h"

namespace framework
{
    namespace detail
    {
        template<
            typename ServiceTag,
            typename ServiceImpl,
            typename ThreadCreatorPolicy = detail::thread_creator<>::dynamic_,
            template<typename> class ExistMode = detail::singleton
        >
        class service :
            public ServiceImpl,
            public ThreadCreatorPolicy,
            public ExistMode<service<ServiceTag, ServiceImpl, ThreadCreatorPolicy, ExistMode> >
        {
        public:
            typedef ServiceTag service_tag;
            typedef ServiceImpl service_impl;
            typedef ExistMode<service<ServiceTag, ServiceImpl, ThreadCreatorPolicy, ExistMode> > exist_mode;

            void on_run_thread()
            {
                ServiceImpl::run();
            }

            void shutdown()
            {
                ServiceImpl::stop();
            }


            service()
            {
                ThreadCreatorPolicy::start_thread();
            }

            ~service()
            {
                shutdown();
            }
	     private:
            friend  class ExistMode<service<ServiceTag, ServiceImpl, ThreadCreatorPolicy, ExistMode> >;
        };
    }
}

#endif //_MTS_FRAMEWORK_DETAIL_SERVICE_H_INCLUDEDs
