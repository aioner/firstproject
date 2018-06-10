#include "xt_av_check.h"

xt_av_check::xt_av_check(void)
:func_check_sn_(0)
{
#ifdef _WIN32
    module_ = LoadLibrary("av_check_sn.dll");
    if (module_)
    {
        func_check_sn_ = (func_check_sn)GetProcAddress(module_, "av_check_sn"); 
    }
#endif
}

xt_av_check::~xt_av_check(void)
{
#ifdef _WIN32
    if (module_)
    {
        FreeLibrary(module_);
        module_ = NULL;
    }
#endif
}

int xt_av_check::check_sn(char *lic_file)
{
#ifdef OPEN_CHECK
    int ret = -1;

#ifdef _WIN32
    if (func_check_sn_)
    {
        ret = func_check_sn_(lic_file);
    }
#endif

#ifdef LINUX
    int v1,v2;
    get_version(&v1,&v2);
    ret = av_check_sn(lic_file);
#endif

    return ret;
#else
    return 0;
#endif //OPEN_CHECK
}
