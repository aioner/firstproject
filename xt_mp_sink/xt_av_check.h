#ifndef XT_AV_CHECK_CASTER_
#define XT_AV_CHECK_CASTER_

#ifdef OPEN_CHECK
#include "av_check_sn.h"
#endif //OPEN_CHECK

#ifdef _WIN32
#include <windows.h>
#endif

typedef int (*func_get_id)(char *res, int *res_len);
typedef int (*func_check_sn)(char *lic_file);
class xt_av_check
{
private:
    xt_av_check(void);
    ~xt_av_check(void);

public:
    static xt_av_check* inst()
    {
        static  xt_av_check self;
        return &self;
    }

    int check_sn(char *lic_file);

private:
    func_check_sn func_check_sn_;

#ifdef _WIN32
    HMODULE  module_;
#endif
};


#endif//XT_AV_CHECK_CASTER_