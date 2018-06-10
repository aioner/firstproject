#ifndef LOGGER_H
#define LOGGER_H

#include <map>
#include "reality_bst_log.h"

#ifdef _WIN32
#ifndef   XT_BOOST_LOG_EXPORTS
#define   BOOST_LOGAPI  __declspec(dllimport)
#else
#define   BOOST_LOGAPI  __declspec(dllexport)
#endif
#else
#define __stdcall
#define BOOST_LOGAPI __attribute__((visibility("default")))
#endif // END #ifdef _WIN32

class xt_log_manager
{
public:
	static xt_log_manager* Instance(){
		if(_instance == 0){
			_instance = new xt_log_manager();
		}
		return _instance;
	}

	void level_log(const char* local_file, severity_level level, const char *fmt, va_list args);
	void new_mkdir_log(const char*);

public:
	typedef std::map<std::string, file_log_t*> LOGINSTANCE;
	typedef LOGINSTANCE::iterator PTRLOGINSTANCE;
	LOGINSTANCE collect_map;

private:
	static xt_log_manager* _instance;
};

//给外部导出的接口
#ifdef __cplusplus
extern "C" {
#endif

	void BOOST_LOGAPI init_log_target(const char* file_name);
	void BOOST_LOGAPI xt_log_write(const char* local_file, severity_level level, const char *fmt, ...);
	void BOOST_LOGAPI delete_log_target(const char* file_name);
	void BOOST_LOGAPI set_log_level( severity_level level);
	void BOOST_LOGAPI set_log_on_off( bool temp);

	// tools unit
// 	std::string BOOST_LOGAPI file_function_line(const char* file_info, const char* func_info, int line_info);

#ifdef __cplusplus
};
#endif

#ifdef _WIN32
#ifndef  XT_BOOST_LOG_EXPORTS
#ifdef _DEBUG
#pragma comment(lib, "xt_boost_log_d.lib")
#pragma message("Auto Link xt_boost_log_d.lib")
#else
#pragma comment(lib, "xt_boost_log.lib")
#pragma message("Auto Link xt_boost_log.lib")
#endif //#ifdef DEBUG

#endif//#ifndef  XT_BOOST_LOG_EXPORTS

#endif//#ifdef _WIN32

#endif