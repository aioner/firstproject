#ifndef _CLI_H_INCLUDED
#define _CLI_H_INCLUDED

#include "boost/program_options.hpp"
namespace bpo = boost::program_options;

#include "boost/function.hpp"
#include "boost/thread/lock_guard.hpp"
#include "boost/typeof/typeof.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/bind.hpp"
#include <string>
#include <iostream>
#include <map>

namespace framework { namespace cli {

    typedef bpo::positional_options_description positional_command_description_t;

    typedef bpo::variables_map variables_map_t;

    using bpo::value;

    class command_description_t;
    typedef boost::function<void (const command_description_t&, const variables_map_t&, std::ostream&)> command_callback_t;

    class command_description_t : public bpo::options_description
    {
    public:
        command_description_t(const std::string& caption, const command_callback_t& cb, const std::string& description)
            :bpo::options_description(caption),
            cb_(cb),
            description_(description)
        {}

        void do_callback(const variables_map_t&vm, std::ostream& os)
        {
            cb_(*this, vm, os);
        }

        void print(std::ostream& os) const
        {
            bpo::options_description::print(os);
            os << "----[" << description_ << ']' << std::endl;
        }

    private:
        command_callback_t cb_;
        std::string description_;
    };

    typedef boost::shared_ptr<command_description_t> command_description_ptr;

    template<typename Mutex = boost::mutex>
    class parser_t
    {
    public:
        typedef std::map<std::string, command_description_ptr> command_descriptions_map_t;

        command_description_ptr register_cmd(const std::string& cmd, const command_callback_t& cb, const std::string& description = "na")
        {
            scoped_lock _lock(mutex_);

            BOOST_AUTO(it, command_descriptions_.find(cmd));
            if (command_descriptions_.end() == it)
            {
                BOOST_AUTO(desc_of_cmd, boost::make_shared<command_description_t>(cmd, cb, description));
                it = command_descriptions_.insert(command_descriptions_map_t::value_type(cmd, desc_of_cmd)).first;
            }
            return it->second;
        }

        bool parse_cmd(const std::string& cmd, bool allow_unregistered = false, std::ostream& os = std::cout) throw()
        {
            try
            {
                BOOST_AUTO(args, bpo::split_unix(cmd));
                if (args.empty())
                {
                    os << "cmd \'" << cmd << "\' split fail" << std::endl;
                    return false;
                }

                command_description_ptr desc_of_cmd = find_cmd(args[0]);
                if (!desc_of_cmd)
                {
                    os << "cmd \'" << cmd << "\' unregistered" << std::endl;
                    return false;
                }

                args.erase(args.begin());

                bpo::command_line_parser parser(args);
                parser.options(*desc_of_cmd);

                if (allow_unregistered)
                {
                    parser.allow_unregistered();
                }

                variables_map_t vm;
                bpo::store(parser.run(), vm);

                desc_of_cmd->do_callback(vm, os);
            }
            catch (const std::exception& e)
            {
                os << e.what() << std::endl;
                return false;
            }

            return true;
        }

        void print(std::ostream& os = std::cout)
        {
            scoped_lock _lock(mutex_);
            for (BOOST_AUTO(it, command_descriptions_.begin()); command_descriptions_.end() != it; ++it)
            {
                it->second->print(os);
            }
        }

        void print(const std::string& cmd, std::ostream& os = std::cout)
        {
            command_description_ptr desc_of_cmd = find_cmd(cmd);
            if (desc_of_cmd)
            {
                desc_of_cmd->print(os);
            }
            else
            {
                os << "cmd \'" << cmd << "\' unregistered" << std::endl;
            }
        }

        template<typename E>
        void run(std::istream& is, std::ostream& os, E& exec, const std::string& exit_cmd = "exit", const std::string& prompt = ">>")
        {
            std::string cmd;
            std::string last_cmd;
            while (true)
            {
                os << prompt;
                if (!std::getline(is, cmd) || exit_cmd == cmd)
                {
                    break;
                }

                if (cmd.empty())
                {
                    if (!last_cmd.empty() && 0 != last_cmd.compare("cls"))
                    {
                        exec.submit(boost::bind(&parser_t<Mutex>::parse_cmd, this, last_cmd, false, boost::ref(std::cout)));
                    }
                }
                else
                {
                    exec.submit(boost::bind(&parser_t<Mutex>::parse_cmd, this, cmd, false, boost::ref(std::cout)));
                    last_cmd = cmd;
                }
            }
        }

    private:
        command_description_ptr find_cmd(const std::string& cmd)
        {
            scoped_lock _lock(mutex_);
            BOOST_AUTO(it, command_descriptions_.find(cmd));
            return (command_descriptions_.end() == it) ? command_description_ptr() : it->second;
        }

        command_descriptions_map_t command_descriptions_;
        Mutex mutex_;

        typedef boost::lock_guard<Mutex> scoped_lock;
    };
} }

#endif //_CLI_H_INCLUDED
