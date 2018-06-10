#include "event_context.h"
#include "detail/event_context_service.h"
#include "utility/function.h"


namespace framework
{
    //static
    void event_context_impl::post(const event_handler &handler)
    {
        detail::event_context_service::instance()->post(handler);
    }

    void event_context::request_event()
    {
        post(utility::bind(&event_context::process_event, this));
    }

    void event_context::cancel_event()
    {}
}