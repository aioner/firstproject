#ifndef _TOOLS_H__
#define  _TOOLS_H__
#include <boost/noncopyable.hpp>

#ifdef _WIN32
#define  XTRouter_PROCESS_NAME "XTRouter.exe"
#else
#define  XTRouter_PROCESS_NAME "XTRouter"
#endif //#ifdef _WIN32

class tools_mgr : boost::noncopyable
{
public:
    static tools_mgr* _(){return &my_obj;};
public:
    bool get_pid(const char* process_name,long& pid);
    bool  kill_process(const char *process_name);
    bool start_process(const char * process_name);
    bool is_exist_process(const char* process_name);
private:
   static tools_mgr my_obj;
};
#endif //#ifndef _TOOLS_H__