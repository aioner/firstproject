//
//create by songlei 20160316
//
#ifndef DPS_CMD_CTRL_H__
#define DPS_CMD_CTRL_H__
#include "framework/core.h"
#include "framework/cli.h"
class dps_cmd_ctrl : boost::noncopyable
{
    dps_cmd_ctrl():cli_(){}
public:
    static void help(const framework::cli::command_description_t& cmd, const framework::cli::variables_map_t& vm, std::ostream& os);
    static void cls(const framework::cli::command_description_t& cmd, const framework::cli::variables_map_t& vm, std::ostream& os);
    static void spi(const framework::cli::command_description_t& cmd, const framework::cli::variables_map_t& vm, std::ostream& os);
    static void scoi(const framework::cli::command_description_t& cmd, const framework::cli::variables_map_t& vm, std::ostream& os);
    static void sdp(const framework::cli::command_description_t& cmd, const framework::cli::variables_map_t& vm, std::ostream& os);

public:
    void run();
    static dps_cmd_ctrl* _(){return &my_;}

private:
    void register_cmd();
    static dps_cmd_ctrl my_;
    framework::cli::parser_t<> cli_;
};

#endif // #ifndef DPS_CMD_CTRL_H__