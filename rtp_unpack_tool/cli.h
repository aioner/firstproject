#ifndef _CLI_H_INCLUDED
#define _CLI_H_INCLUDED

#include <string>
#include <iostream>

class rtp_unpack_parser_t
{
public:
    virtual bool parse(std::istream& input, std::ostream& output, const std::string &format, std::ostream& log) = 0;

protected:
    virtual ~rtp_unpack_parser_t() {}
};

class command_line_interface_t
{
public:
    explicit command_line_interface_t(const std::string& caption);

    void register_rtp_unpack_parser(rtp_unpack_parser_t *cb);
    int parse_command_line(int argc, char *argv[], std::ostream& os);

private:
    std::string caption_;
    rtp_unpack_parser_t *cb_;
};

#endif//_CLI_H_INCLUDED

