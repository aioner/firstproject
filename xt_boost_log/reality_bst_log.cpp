#include <boost/filesystem.hpp>
#include "reality_bst_log.h"

std::ostream& operator << (std::ostream& strm, severity_level level)
{
	static const char* strings[] =
	{
		"ll_off",
		"ll_all",
		"ll_trace",
		"ll_debug",
		"ll_info",
		"ll_warn",
		"ll_error"
	};

	if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
		strm << strings[level];
	else
		strm << static_cast< int >(level);

	return strm;
}

file_log_t::file_log_t(const std::string& file) : base_t(boost::log::keywords::channel = file)
{
	boost::filesystem::path p(file);
#ifdef _WIN32
	std::string str = p.parent_path().string() + "\\" + p.filename().stem().string();
#else
	std::string str = p.parent_path().string() + "/" + p.filename().stem().string();
#endif
	boost::shared_ptr< boost::log::sinks::text_file_backend > backend = boost::make_shared< boost::log::sinks::text_file_backend >(
		boost::log::keywords::file_name = str+"_%Y.%m.%d_%2N.txt",
		boost::log::keywords::open_mode = std::ios::out,
		boost::log::keywords::auto_flush = true,
		boost::log::keywords::rotation_size = 300 * 1024 * 1024,                             
		boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(12, 0, 0)  
		);

	typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_file_backend > sink_t;
	boost::shared_ptr< sink_t > sink(new sink_t(backend));

	sink->set_formatter(		boost::log::expressions::format("[%1%][%2%][%3%]%4%")		% boost::log::expressions::attr< boost::posix_time::ptime >("TimeStamp")		% boost::log::expressions::attr< boost::log::attributes::current_thread_id::value_type >("ThreadID")		% boost::log::expressions::attr< severity_level >("Severity")		% boost::log::expressions::smessage		);

	sink->set_filter(boost::log::expressions::attr< std::string >("Channel") == file);

	boost::log::core::get()->add_sink(sink);
	boost::log::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
	boost::log::core::get()->add_global_attribute("RecordID", boost::log::attributes::counter< unsigned int >());
	boost::log::core::get()->add_global_attribute("ThreadID", boost::log::attributes::current_thread_id());

}