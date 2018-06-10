#ifndef HISTORYINFO_H__
#define HISTORYINFO_H__

#include "InfoTypeDef.h"
#include <list>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/noncopyable.hpp>


class CHistoryInfo
{
protected:
    CHistoryInfo(void);
    ~CHistoryInfo(void);
public:
    static CHistoryInfo* instance()
    {		
        return &m_obj;
    }

private:
    static CHistoryInfo m_obj;

public:

    //保存登录中心事件
    void SaveLoginOrLogoutCenterEvent(const info_mgr::INFO_LOGINORLOGOUTCENTEREVNT& infoLoginOrLogoutCenterEvent);
    void GetLoginOrLogoutCenterEvent(std::list<info_mgr::INFO_LOGINORLOGOUTCENTEREVNT>& lstLoginOrLogoutCenterEvent);

    //保存点播事件
    void SavePlayEventInfo(const info_mgr::INFO_CENTERDBCOMMANDEVENT& infoPlayEvent);
    void GetPlayEventInfo(std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>& listPlayEvent);

    //保存LinkSever事件
    void SaveLinkSeverEventInfo(const info_mgr::INFO_LINKSERVEREVENT& infoLinkSeverEvent);
    void GetLinkSeverEventInfo(std::list<info_mgr::INFO_LINKSERVEREVENT>& listLinkSeverEvent);

    //反馈中心事件信息
    void SaveResponseToCenterEvent(const info_mgr::INFO_RESPONSETOCENTER& infoResponseToCenterEvent);
    void GetResponseToCenterEvent(std::list<info_mgr::INFO_RESPONSETOCENTER>& listResponseToCenterEvent);

    //信令执行结果
    void SaveSigalingExecRet(const info_mgr::INFO_SIGNALINGEXECRESULT& info);
    void GetSigalingExecRet(std::list<info_mgr::INFO_SIGNALINGEXECRESULT>& lstSigalingExecRuslut);

private:
    unsigned int m_uiMaxSaveNum; //最大存储条数

private:
    //boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    boost::recursive_mutex		m_mutex;		                   //全局锁
    boost::recursive_mutex        m_LoginOrLogoutCenterMutex;        //登录登出中心事件锁
    boost::recursive_mutex        m_PlayEventMutex;                  //登出中心事件锁
    boost::recursive_mutex        m_LinkSeverMutex;                  //LinkSever事件锁
    boost::recursive_mutex        m_ResponseToCenterMutex;           //反馈中心事件信息事件锁
    boost::recursive_mutex        m_SigalingExecRuslutMutex;           //信令执行结果


private:	
    std::list<info_mgr::INFO_LOGINORLOGOUTCENTEREVNT>       m_lstLoginOrLogoutCenterEvent;        //登录中心事件
    std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>           m_listPlayEvent;                      //点播信息
    std::list<info_mgr::INFO_LINKSERVEREVENT>                m_listLinkSeverEvent;                 //LinkSever事件
    std::list<info_mgr::INFO_RESPONSETOCENTER>               m_listResponseToCenterEvent;          //反馈中心事件信息
    std::list<info_mgr::INFO_SIGNALINGEXECRESULT>            m_lstSigalingExecRuslut;              //信令执行结果

};
#endif//HISTORYINFO_H__
