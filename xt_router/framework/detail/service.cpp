#include "deadline_timer_service.h"
#include "event_context_service.h"
#include "task_scheduler_service.h"

void framework_services_construct()
{
    framework::detail::task_scheduler_service::new_instance();
    framework::detail::event_context_service::new_instance();
    framework::detail::deadline_timer_service::new_instance();
}

void framework_services_destruct()
{
    framework::detail::deadline_timer_service::instance()->shutdown();
    framework::detail::event_context_service::instance()->shutdown();
    framework::detail::task_scheduler_service::instance()->shutdown();

    framework::detail::deadline_timer_service::delete_instance();
    framework::detail::event_context_service::delete_instance();
    framework::detail::task_scheduler_service::delete_instance();
}

