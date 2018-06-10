#ifndef _MTS_FRAMEWORK_EVENT_CONTEXT_H_INCLUDED
#define _MTS_FRAMEWORK_EVENT_CONTEXT_H_INCLUDED

#include "utility/noncopyable.h"
#include "utility/function.h"

namespace framework
{
    class event_context_impl : private utility::noncopyable
    {
    public:
        typedef utility::function<void (void)> event_handler;
        static void post(const event_handler &handler);
    };

    class event_context : public event_context_impl
    {
    public:
        void request_event();
        void cancel_event();
    protected:
        virtual void process_event() = 0;
        virtual ~event_context() {}
    };
}

#endif//_MTS_FRAMEWORK_EVENT_CONTEXT_H_INCLUDED
