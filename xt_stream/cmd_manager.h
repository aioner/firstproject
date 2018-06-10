#ifndef _CMD_MANAGER_H_INCLUDED
#define _CMD_MANAGER_H_INCLUDED
#include "utility/mutex.h"

#include <vector>
#include <string>
#include <map>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

class command_manager_t : private boost::noncopyable
{
public:
    enum status_type
    {
        ok,
        exec_will_exit,
        cmd_not_exists,
        cmd_arguments_error,
        cmd_format_error
    };

    typedef utility::read_write_mutex mutex_type;

    status_type parse_cmd(const std::string& line);

    static command_manager_t *instance() { return &s_inst_;}
private:
    command_manager_t() 
        : dropped_delims_(" ")
    {}

    ~command_manager_t()
    {}


    mutex_type mutex_;

    std::string dropped_delims_;

    static command_manager_t s_inst_;
};
#endif //_CMD_MANAGER_H_INCLUDED
