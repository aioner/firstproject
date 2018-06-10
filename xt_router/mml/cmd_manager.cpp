#include "cmd_manager.h"

#include <boost/tokenizer.hpp>

//static 
command_manager_t command_manager_t::s_inst_;

bool command_manager_t::register_cmd(const std::string& cmd, const dispatch_function_type& func)
{
    utility::write_lock lock(mutex_);

    register_info_type::iterator result = commands_.find(cmd);
    if (commands_.end() != result)
    {
        //命令已经注册
        return false;
    }

    return commands_.insert(register_info_type::value_type(cmd, func)).second;
}

bool command_manager_t::unregister_cmd(const std::string& cmd)
{
    utility::write_lock lock(mutex_);

    register_info_type::iterator result = commands_.find(cmd);
    if (commands_.end() == result)
    {
        //命令不存在
        return false;
    }
    commands_.erase(result);
    return true;
}

command_manager_t::status_type command_manager_t::parse_cmd(const std::string& line,std::string& res)
{
    typedef boost::char_separator<char> parser_rule;
    parser_rule rule(dropped_delims_.c_str(), "=");

    typedef boost::tokenizer<parser_rule> tokenizer;
    tokenizer tok(line, rule);
    std::ostringstream oss;
    res.clear();
    tokenizer::iterator result = tok.begin();
    if (tok.end() == result)
    {
        oss<<"cmd_format_error" <<std::endl;
        res.append(oss.str());
        return cmd_format_error;
    }

    utility::read_lock lock(mutex_);

    dispatch_function_type *pfunc = NULL;
    bool is_exit_cmd = false;
    //先检查是不是退出命令
    if (exit_command_.first == *result)
    {
        pfunc = &exit_command_.second;
        is_exit_cmd = true;
    }
    else
    {
        register_info_type::iterator register_info_result = commands_.find(*result);
        if (commands_.end() == register_info_result)
        {
            //命令不存在
            oss << "cmd_not_exists"<<std::endl;
            res.append(oss.str());
            return cmd_not_exists;
        }

        pfunc = &register_info_result->second;
        is_exit_cmd = false;
    }

    if ((NULL == pfunc) || !(*pfunc))
    {
        //命令函数无效
        oss << cmd_not_exists<<std::endl;
        res.append(oss.str());
        return cmd_not_exists;
    }

    std::vector<std::string> cmds;
    for (result++; tok.end() != result; ++result)
    {
        cmds.push_back(*result);
    }

    command_argument_t argument;
    if (command_argument_t::invalid_t == argument.from_cmd(cmds))
    {
        //无用参数类型
        oss <<cmd_arguments_error<<std::endl;
        res.append(oss.str());
        return cmd_format_error;
    }

    bool argument_func_result = false;

    try
    {
        argument_func_result = (*pfunc)(argument, res);
    }
    catch (...)
    {
         oss << cmd_arguments_error<<std::endl;
         res.append(oss.str());
        return cmd_arguments_error;
    }

    if (is_exit_cmd)
    {
        return exec_will_exit;
    }

    return argument_func_result ? ok : cmd_arguments_error;
}

void command_manager_t::get_cmd_register_info(std::vector<std::string>& cmds_info)
{
    utility::read_lock lock(mutex_);

    cmds_info.clear();
    for (register_info_type::iterator iter = commands_.begin(); commands_.end() != iter; ++iter)
    {
        cmds_info.push_back(iter->first);
    }
}

void command_manager_t::register_dropped_delims(const char *dropped_delims)
{
    if (NULL != dropped_delims)
    {
        dropped_delims_.assign(dropped_delims);
    }
}

bool command_manager_t::register_exit_cmd(const std::string& cmd, const dispatch_function_type& func)
{
    utility::write_lock lock(mutex_);
    exit_command_.first = cmd;
    exit_command_.second = func;
    return true;
}
