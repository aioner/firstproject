///////////////////////////////////////////////////////////////////////////////////////////
// ÎÄ ¼þ Ãû£ºxt_log_def.h
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef XT_LOG_DEFINE_
#define XT_LOG_DEFINE_

#ifdef _WIN32
#define XT_STDCALL __stdcall
#else
#define XT_STDCALL
#endif

enum xt_log_level
{
	level_off = 0,
	level_all,
	level_trace,
	level_debug,
	level_info,
	level_warn,
	level_error,
};

typedef void (XT_STDCALL *xt_print_cb)(const char* logger_name,
							 const xt_log_level ll,
							 const char* context);

#endif//XT_LOG_DEFINE_
