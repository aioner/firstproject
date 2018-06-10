#include "cli.h"

#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"
#include <fstream>

namespace po = boost::program_options;

command_line_interface_t::command_line_interface_t(const std::string& caption)
:caption_(caption),
cb_(NULL)
{}

void command_line_interface_t::register_rtp_unpack_parser(rtp_unpack_parser_t *cb)
{
    cb_ = cb;
}

int command_line_interface_t::parse_command_line(int argc, char *argv[], std::ostream& os)
{
    po::options_description description(caption_);

    description.add_options()
        ("input,I", po::value<std::string>()->required(), "input rtp file")
        ("format,F", po::value<std::string>(), "media format,h264/g711")
        ("output,O", po::value<std::string>()->required(), "output frame file")
        ("log,L", po::value<std::string>(), "log file")
        ("help,H", "help information");

    po::variables_map vm;

    try
    {
        po::store(po::parse_command_line(argc, argv, description), vm);
    }
    catch (const std::exception& e)
    {
        os << e.what() << std::endl;
        return -1;
    }

    if (0 != vm.count("help"))
    {
        os << description;
        return 0;
    }

    if (NULL == cb_)
    {
        os << "rtp unpack callback unregistered." << std::endl;
        return -1;
    }

    if (0 == vm.count("input"))
    {
         os << "param --input needed." << std::endl;
        return -1;
    }

    std::ifstream input(vm["input"].as<std::string>().c_str(), std::ios_base::binary);
    if (!input)
    {
        os << "input file:" << vm["input"].as<std::string>() << " not exists." << std::endl;
        return -1;
    }

    if (0 == vm.count("output"))
    {
        os << "param --output needed." << std::endl;
        return -1;
    }

    std::ofstream output(vm["output"].as<std::string>().c_str(), std::ios_base::binary);
    if (!output)
    {
        os << "output file:" << vm["output"].as<std::string>() << " open failed." << std::endl;
        return -1;
    }

    std::string format;
    if (0 == vm.count("format"))
    {
        std::string ext = boost::filesystem::extension(vm["output"].as<std::string>());
        if ((".h264" == ext) || (".264" == ext))
        {
            format = "h264";
        }
        else if ((".711a" == ext) || (".711u" == ext))
        {
            format = "g711";
        }
        else
        {
            os << "output file:" << vm["output"].as<std::string>() << " unknown extension." << std::endl;
            return -1;
        }
    }
    else
    {
        format = vm["format"].as<std::string>();
    }

    std::ofstream log;
    if (0 != vm.count("log"))
    {
        log.open(vm["log"].as<std::string>().c_str());
    }

    if (!cb_->parse(input, output, format, log ? log : os))
    {
        os << "parse failed." << std::endl;
        return -1;
    }

    return 0;
}
