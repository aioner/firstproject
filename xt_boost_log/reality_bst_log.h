#ifndef REALITY_BST_LOG_H
#define REALITY_BST_LOG_H

#include <boost/log/core.hpp>
#include <boost/log/keywords/channel.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/channel_logger.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <string>
#include <iostream>

enum severity_level
{
	ll_off = 0,
	ll_all,
	ll_trace,
	ll_debug,
	ll_info,
	ll_warn,
	ll_error
};

std::ostream& operator << (std::ostream& strm, severity_level level);

class file_log_t : public boost::log::sources::severity_channel_logger_mt< severity_level,std::string >
{
public:
	typedef boost::log::sources::severity_channel_logger_mt< severity_level, std::string > base_t;

	file_log_t(const std::string& file);

};


#endif