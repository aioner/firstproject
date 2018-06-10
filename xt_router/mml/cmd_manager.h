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

namespace xt_cmd_type_cast
{
    template<typename T>
    struct cast_from_string
    {
        typedef T cast_type;

        static cast_type cast(const std::string& s)
        {
            return  boost::lexical_cast<cast_type>(s);
        }
    };

    template<>
    struct cast_from_string<std::string>
    {
        typedef const char *cast_type;

        static const char *cast(const std::string& s)
        {
            return s.c_str();
        }
    };
}

template<typename T1, typename T2>
class bytes_union_t
{
public:
    typedef T1 first_type;
    typedef T2 second_type;

    template<typename T3>
    void construct()
    {
        new (stored_bytes_) T3;
    }

    template<typename T3>
    void destruct()
    {
        get<T3>()->~T3();
    }

    template<typename T3>
    T3 *get()
    {
        return (T3 *)(stored_bytes_);
    }

    template<typename T3>
    const T3 *get() const
    {
        return (const T3 *)(stored_bytes_);
    }
private:
    char stored_bytes_[sizeof(T1) > sizeof(T2) ? sizeof(T1) : sizeof(T2)];
};

class command_argument_t
{
public:
    enum stored_type 
    {
        invalid_t = 0,
        position_t,
        mapped_t 
    };

    typedef std::vector<std::string> position_arguments_type;
    typedef std::map<std::string, std::string> mapped_arguments_type;

    template<typename T>
    typename xt_cmd_type_cast::cast_from_string<T>::cast_type get(size_t index) const
    {
        if (position_t != type())
        {
            throw std::runtime_error("not position command");
        }

        const position_arguments_type *args = get_position_arguments();
        if (args->size() <= index)
        {
            throw std::runtime_error("index too large");
        }

        return xt_cmd_type_cast::cast_from_string<T>::cast(args->at(index));
    }

    template<typename T>
    typename xt_cmd_type_cast::cast_from_string<T>::cast_type get(const std::string&index) const
    {
        if (mapped_t != type())
        {
            throw std::runtime_error("not mapped command");
        }

        const mapped_arguments_type *args = get_mapped_arguments();
        mapped_arguments_type::const_iterator result = args->find(index);
        if (args->end() == result)
        {
            throw std::runtime_error("index not exist");
        }

        return xt_cmd_type_cast::cast_from_string<T>::cast(result->second);
    }

    template<typename T, typename T2>
    typename xt_cmd_type_cast::cast_from_string<T>::cast_type get(const std::string&index, T2 default_val) const
    {
        if (mapped_t != type())
        {
            throw std::runtime_error("not mapped command");
        }

        const mapped_arguments_type *args = get_mapped_arguments();
        mapped_arguments_type::const_iterator result = args->find(index);
        if (args->end() == result)
        {
            //不存在返回默认类型
            return default_val;
        }

        return xt_cmd_type_cast::cast_from_string<T>::cast(result->second);
    }

    bool exist(const std::string&index) const
    {
        if (mapped_t != type())
        {
            throw std::runtime_error("not mapped command");
        }

        const mapped_arguments_type *args = get_mapped_arguments();
        return (args->end() != args->find(index));
    }

    size_t count() const
    {
        if (position_t == type())
        {
            return arguments_.get<position_arguments_type>()->size();
        }

        if (mapped_t == type())
        {
            return arguments_.get<mapped_arguments_type>()->size();
        }

        throw std::runtime_error("bad stored type");
    }

    stored_type type() const
    {
        return stored_type_;
    }

private:
    friend class command_manager_t;

    command_argument_t(const command_argument_t&);
    const command_argument_t& operator = (const command_argument_t&);

    command_argument_t() : stored_type_(invalid_t) {}

    ~command_argument_t()
    {
        if (position_t == stored_type_)
        {
            arguments_.destruct<position_arguments_type>();
        }

        if (mapped_t == stored_type_)
        {
            arguments_.destruct<mapped_arguments_type>();
        }
    }

    void construct(stored_type which)
    {
        stored_type_ = which;
        if (position_t == stored_type_)
        {
            arguments_.construct<position_arguments_type>();
        }

        if (mapped_t == stored_type_)
        {
            arguments_.construct<mapped_arguments_type>();
        }
    }

    position_arguments_type *get_position_arguments()
    {
        return arguments_.get<position_arguments_type>();
    }

    mapped_arguments_type *get_mapped_arguments()
    {
        return arguments_.get<mapped_arguments_type>();
    }

    const position_arguments_type *get_position_arguments() const
    {
        return arguments_.get<position_arguments_type>();
    }

    const mapped_arguments_type *get_mapped_arguments() const
    {
        return arguments_.get<mapped_arguments_type>();
    }

    stored_type from_cmd(const std::vector<std::string>& cmds)
    {
        stored_type type = invalid_t;
        if (0 == (cmds.size() % 3))
        {
            type = mapped_t;
            for (size_t pos = 1; pos < cmds.size(); pos += 3)
            {
                if ("=" != cmds[pos])
                {
                    type = position_t;
                    break;
                }
            }
        }
        else
        {
            type = position_t;
        }

        construct(type);

        if (mapped_t == type)
        {
            for (size_t pos = 0; pos < cmds.size(); pos += 3)
            {
                get_mapped_arguments()->insert(mapped_arguments_type::value_type(cmds[pos], cmds[pos + 2]));
            }
        }

        if (position_t == type)
        {
            get_position_arguments()->assign(cmds.begin(), cmds.end());
        }

        return type;
    }

    bytes_union_t<position_arguments_type, mapped_arguments_type> arguments_;
    stored_type stored_type_;
};

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

    typedef boost::function<bool (const command_argument_t&, std::string& result)> dispatch_function_type;
    typedef std::map<std::string, dispatch_function_type> register_info_type;
    typedef utility::read_write_mutex mutex_type;

    void register_dropped_delims(const char *dropped_delims);
    bool register_cmd(const std::string& cmd, const dispatch_function_type& func);
    bool register_exit_cmd(const std::string& cmd, const dispatch_function_type& func);
    bool unregister_cmd(const std::string& cmd);
    status_type parse_cmd(const std::string& line, std::string &result);
    void get_cmd_register_info(std::vector<std::string>&);

    static command_manager_t *instance() { return &s_inst_;}
private:
    command_manager_t() 
        : dropped_delims_(" :-,|")
    {}

    ~command_manager_t()
    {}

    std::pair<std::string, dispatch_function_type> exit_command_;

    register_info_type commands_;
    mutex_type mutex_;

    std::string dropped_delims_;

    static command_manager_t s_inst_;
};
#endif //_CMD_MANAGER_H_INCLUDED
