#include "tools.h"
#include <boost/lexical_cast.hpp>

#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#define  XTRouter_PROCESS_NAME "XTRouter.exe"
#else
#include <dirent.h>
#include <stdlib.h>
#define  XTRouter_PROCESS_NAME "XTRouter"
#endif //#ifdef _WIN32
#define BUF_SIZE 1024

tools_mgr tools_mgr::my_obj;

bool tools_mgr::get_pid(const char* process_name,long& pid)
{
    pid = -1;
#ifdef _WIN32
    HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (NULL == snap)
    {
        return false;
    }

    PROCESSENTRY32 pe;
    ZeroMemory(&pe, sizeof(PROCESSENTRY32));
    pe.dwSize = sizeof(PROCESSENTRY32);

    BOOL found = ::Process32First(snap, &pe);
    while (found)
    {
        if (0 == ::strcmp(process_name,pe.szExeFile))
        {
            pid = boost::lexical_cast<DWORD>(pe.th32ProcessID);
            break;
        }

        found = ::Process32Next(snap, &pe);
    }
    if (!found)
    {
        pid = -2;
    }
    ::CloseHandle(snap);
#else
    DIR* dir=NULL;
    dir = ::opendir("/proc");
    if (NULL != dir)
    {
        struct dirent *ptr=NULL;
        while(NULL != (ptr = ::readdir(dir)))
        {
            if (0 == ::strcmp(ptr->d_name,".")
                ||0 == ::strcmp(ptr->d_name,".."))
            {
                continue;
            }

            if (DT_DIR != ptr->d_type)
            {
                continue;
            }

            FILE* fp = NULL;
            char filepath[50]={0};
            ::sprintf(filepath,"/proc/%s/status",ptr->d_name);
            fp = ::fopen(filepath,"r");

            char cur_process_name[128]={0};
            char buf[BUF_SIZE]={0};
            if (NULL != fp)
            {
                if (NULL == ::fgets(buf,BUF_SIZE-1,fp))
                {
                    ::fclose(fp);
                    continue;
                }
                if (1 != ::sscanf(buf,"%*s %s",cur_process_name))
                {
                    pid = -2;
                    ::fclose(fp);
                    continue;
                }
                ::fclose(fp);
            }
            if ( 0 == ::strcmp(process_name,cur_process_name))
            {
                pid = boost::lexical_cast<unsigned long>(ptr->d_name);
                break;
            }
            else
            {
                continue;
            }
        }
        ::closedir(dir);
    }
#endif //#ifdef _WIN32
    if (pid < 0)
    {
        return false;
    }
    return true;
}
bool tools_mgr::kill_process(const char *process_name)
{
    bool result = false;
    do
    {
#ifdef _WIN32
        long pid = 0;
        if (!get_pid(process_name, pid))
        {
            result = true;
            break;
        }
        DWORD pid_win = boost::lexical_cast<DWORD>(pid);
        HANDLE hp = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (NULL == hp)
        {
            break;
        }
        result = (TRUE == ::TerminateProcess(hp, 0));
        ::CloseHandle(hp);
#else
        char cmd[256]={0};
        ::sprintf(cmd,"killall %s",process_name);
        if (::system(cmd) < 0)
        {
            result = false;
            break;
        };
#endif //#ifdef _WIN32
    }
    while (false);
    return result;
}

bool tools_mgr::start_process(const char * process_name)
{
    char cmd[256]={0};
#ifdef _WIN32
    ::sprintf(cmd,".\\\\%s",process_name);
    HANDLE result = ::ShellExecuteA(NULL, "open", cmd, NULL, NULL, SW_SHOWMINIMIZED);
    return ((int)result > 32);
#else
    ::sprintf(cmd,"./%s",process_name);
    if (::system(cmd) < 0)
    {
        std::cout<<"system cmd:"<<cmd<<"fail!"<<std::endl;
        return false;
    }
#endif //#ifdef _WIN32
    return true;
}
bool tools_mgr::is_exist_process(const char* process_name)
{
    long pid = 0;
    if (!get_pid(process_name, pid))
    {
        return false;
    }
    return true;
}
