#ifndef INFOMRG_H__
#define INFOMRG_H__

#include "HistoryInfo.h"
#include "RealInfo.h"
#include <list>

#define  INFO_MGR_CALLBACK __stdcall

//#define USE_INFO_MGR_
class CInfoMgr
{

protected:
    CInfoMgr(void);
    ~CInfoMgr(void);	

public:
    static CInfoMgr* instance()
    { 

        return &m_Obj; 
    };

public:
    //设置事件信息实时显示标志
    void SetEventRealShowFlg(const info_mgr::INFO_TYPE ulInfoType,const bool bIsRealShowFlg = true);

public:

    void GetPlayInfo(std::list<info_mgr::INFO_PLAY>& lstPlayInfo);

    void GetConnectInfo(std::list<info_mgr::INFO_TRANS>& lstTransInfo);

    void PostEventInfo(const info_mgr::INFO_TYPE ulInfoType,info_mgr::PINFOBASE pInfoData);
private:
    static CInfoMgr m_Obj;

};
#endif//INFOMRG_H__
