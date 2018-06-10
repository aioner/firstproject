#ifndef RUNINFOMGR_H
#define RUNINFOMGR_H

#include "common.h"
#include "h_xtmediaserver.h"
#include <vector>
class CRunInfoMgr
{
protected:
    CRunInfoMgr(void);
    ~CRunInfoMgr(void);

public:
    static CRunInfoMgr *instance();

public:
    int GetInfoConnectInfo(std::vector<connect_info_t>& vecConnectInfoData);
    int get_connect_info(connect_info_t out_cinfo[], uint32_t& connect_num);
    uint32_t get_cur_connect_num();
};

#endif//RUNINFOMGR_H
