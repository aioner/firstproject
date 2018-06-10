#ifndef _EVENT_CONTEXT_H_INCLUDED
#define _EVENT_CONTEXT_H_INCLUDED

namespace framework
{
    class event_context_t
    {
    public:
        void request_event();
        void cancel_event();
    protected:
        virtual void process_event() = 0;
        virtual ~event_context_t() {}
    };
}

#endif //_EVENT_CONTEXT_H_INCLUDED
