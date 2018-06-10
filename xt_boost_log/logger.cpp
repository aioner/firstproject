#include <boost/filesystem.hpp>

#include "logger.h"

#define LogBufferSize 1024

xt_log_manager* xt_log_manager::_instance = NULL;

void xt_log_manager::level_log( const char* local_file, severity_level level, const char *fmt, va_list args )
{
	char log_buf[LogBufferSize] = { 0 };
#ifdef _WIN32
	vsnprintf_s(log_buf, LogBufferSize, LogBufferSize-1, fmt, args);
#else
	vsnprintf(log_buf, LogBufferSize, fmt, args);
#endif
	PTRLOGINSTANCE itor = collect_map.find(local_file);
	if (itor == collect_map.end())
		return;

	file_log_t* temp =  itor->second;
	try
	{
		BOOST_LOG_SEV(*temp, level) << log_buf;
	}
	catch(...)
	{
	}
}

void xt_log_manager::new_mkdir_log(const char* local_file)
{
    boost::filesystem::path p(local_file);
    std::string str = p.parent_path().string();

    boost::filesystem::create_directory(str);
}

//导出的接口

void BOOST_LOGAPI init_log_target( const char* file_name )
{
	xt_log_manager::Instance()->new_mkdir_log(file_name);
	file_log_t* file_log_ptr =  new file_log_t(file_name);
	xt_log_manager::Instance()->collect_map.insert(
		std::pair<std::string, file_log_t*>(file_name, file_log_ptr));
}

void BOOST_LOGAPI xt_log_write( const char* local_file, severity_level level, const char *fmt, ... )
{
	va_list args;
	va_start(args, fmt);
	xt_log_manager::Instance()->level_log(local_file, level, fmt, args);
	va_end(args);
}

void BOOST_LOGAPI delete_log_target( const char* file_name )
{	
	std::map<std::string, file_log_t*>::iterator itor = 
		xt_log_manager::Instance()->collect_map.find(file_name);
	if (itor == xt_log_manager::Instance()->collect_map.end())
		return;

	delete itor->second;
}


// std::string BOOST_LOGAPI file_function_line( const char* file_info, const char* func_info, int line_info )
// {
// 	std::string str_file_func_line = lexical_cast<std::string>(file_info) 
// 		+ ":" + lexical_cast<std::string>(func_info) 
// 		+ ":" + lexical_cast<std::string>(line_info);
// 
// 	std::string str = str_file_func_line.substr(str_file_func_line.rfind("\\")+1, 
// 		str_file_func_line.length()-str_file_func_line.rfind("\\"));
// 	return str;
// }

void BOOST_LOGAPI set_log_on_off( bool temp )
{
	boost::log::core::get()->set_logging_enabled(temp);
}

void BOOST_LOGAPI set_log_level( severity_level level )
{
	boost::log::core::get()->set_filter(
		boost::log::expressions::attr< severity_level >("Severity") >= level);
}
