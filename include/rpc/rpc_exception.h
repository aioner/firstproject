#ifndef _RPC_EXCEPTION_H_INCLUDED
#define _RPC_EXCEPTION_H_INCLUDED

#include <stdexcept>
#include <string>
#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#define RPC_CDECL __cdecl
#else
#define RPC_CDECL
#endif

#define RPC_EXCEPTION_DESC_MAX   1024

namespace rpc
{
    class exception : public std::exception
    {
    public:
        explicit exception(const char *desc)
            :desc_(desc)
        {}

        ~exception() throw() {}

        const char *what() const throw()
        {
            return desc_.c_str();
        }

    private:
        std::string desc_;
    };

    inline void RPC_CDECL throw_exception(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);

        char desc[RPC_EXCEPTION_DESC_MAX];
#ifdef _WIN32
        _vsnprintf_s(desc, RPC_EXCEPTION_DESC_MAX, fmt, args);
#else
        vsnprintf(desc, RPC_EXCEPTION_DESC_MAX, fmt, args);
#endif
        desc[RPC_EXCEPTION_DESC_MAX - 1] = 0;
        va_end(args);

        throw exception(desc);
    }
}

#define THROW_EXCEPTION(flag)   if (flag) rpc::throw_exception(#flag)

#endif //_RPC_EXCEPTION_H_INCLUDED
