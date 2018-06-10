#include "cmd_manager.h"

#include <boost/tokenizer.hpp>
#include "file_stream.h"

//static 
command_manager_t command_manager_t::s_inst_;

command_manager_t::status_type command_manager_t::parse_cmd(const std::string& line)
{
    typedef boost::char_separator<char> parser_rule;
    parser_rule rule(dropped_delims_.c_str(), "=");

    typedef boost::tokenizer<parser_rule> tokenizer;
    tokenizer tok(line, rule);
    std::ostringstream oss;
    tokenizer::iterator result = tok.begin();
    if (tok.end() == result)
    {
        oss<<"cmd_format_error" <<std::endl;
        return cmd_format_error;
    }

	std::vector<std::string> vArgs;
	for (;result!=tok.end();++result)
	{
		vArgs.push_back(*result);
	}

	if (vArgs.size() == 0)
	{ 
		oss<<"cmd_format_error" <<std::endl;
		return cmd_format_error;
	}

	std::string cmd = vArgs[0];
	if (cmd=="stream" && vArgs.size() >= 3)
	{
		std::string src_file = vArgs[1];
		std::string dest_file = vArgs[2];
		std::string dest_ip = "";
		unsigned short dest_port = 0;
        unsigned int demuxid = 0;
		if (vArgs.size() >= 5)
		{
			dest_ip = vArgs[3];
			dest_port = ::atoi(vArgs[4].c_str());
		}

        if (vArgs.size() >= 6)
        {
            demuxid = ::atoi(vArgs[5].c_str());;
        }

		file_stream::inst()->stop_file_stream();
		file_stream::inst()->start_file_stream(src_file, dest_file, dest_ip, dest_port, demuxid);
	}
	else if (cmd=="stop")
	{
		file_stream::inst()->stop_file_stream();
	}

    return ok;
}
