#ifndef _MSEC_TIMER_MGR_H_INCLUDED
#define _MSEC_TIMER_MGR_H_INCLUDED

#include "msec_timer.h"
#include "utility/singleton.h"

namespace xt_media_client
{
	typedef xt_utility::singleton_impl<deadline_timer_mgr_t> msec_timer_mgr_t;
}

#endif //_MSEC_TIMER_MGR_H_INCLUDED
