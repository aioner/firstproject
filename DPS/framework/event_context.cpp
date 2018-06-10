#include "framework/event_context.h"

namespace framework
{
    void event_context_t::request_event(){process_event();}
    void event_context_t::cancel_event(){}
}
