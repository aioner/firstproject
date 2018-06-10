#include "cli.h"
#include "rtp.h"

#include <iostream>

int main(int argc, char *argv[])
{
    command_line_interface_t cli("rtp unpack tool");

    rtp_unpack_parser_impl_t parser;
    cli.register_rtp_unpack_parser(&parser);

    return cli.parse_command_line(argc, argv, std::cout);
}