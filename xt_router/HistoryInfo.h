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

    //�����¼�����¼�
    void SaveLoginOrLogoutCenterEvent(const info_mgr::INFO_LOGINORLOGOUTCENTEREVNT& infoLoginOrLogoutCenterEvent);
    void GetLoginOrLogoutCenterEvent(std::list<info_mgr::INFO_LOGINORLOGOUTCENTEREVNT>& lstLoginOrLogoutCenterEvent);

    //����㲥�¼�
    void SavePlayEventInfo(const info_mgr::INFO_CENTERDBCOMMANDEVENT& infoPlayEvent);
    void GetPlayEventInfo(std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>& listPlayEvent);

    //����LinkSever�¼�
    void SaveLinkSeverEventInfo(const info_mgr::INFO_LINKSERVEREVENT& infoLinkSeverEvent);
    void GetLinkSeverEventInfo(std::list<info_mgr::INFO_LINKSERVEREVENT>& listLinkSeverEvent);

    //���������¼���Ϣ
    void SaveResponseToCenterEvent(const info_mgr::INFO_RESPONSETOCENTER& infoResponseToCenterEvent);
    void GetResponseToCenterEvent(std::list<info_mgr::INFO_RESPONSETOCENTER>& listResponseToCenterEvent);

    //����ִ�н��
    void SaveSigalingExecRet(const info_mgr::INFO_SIGNALINGEXECRESULT& info);
    void GetSigalingExecRet(std::list<info_mgr::INFO_SIGNALINGEXECRESULT>& lstSigalingExecRuslut);

private:
    unsigned int m_uiMaxSaveNum; //���洢����

private:
    //boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    boost::recursive_mutex		m_mutex;		                   //ȫ����
    boost::recursive_mutex        m_LoginOrLogoutCenterMutex;        //��¼�ǳ������¼���
    boost::recursive_mutex        m_PlayEventMutex;                  //�ǳ������¼���
    boost::recursive_mutex        m_LinkSeverMutex;                  //LinkSever�¼���
    boost::recursive_mutex        m_ResponseToCenterMutex;           //���������¼���Ϣ�¼���
    boost::recursive_mutex        m_SigalingExecRuslutMutex;           //����ִ�н��


private:	
    std::list<info_mgr::INFO_LOGINORLOGOUTCENTEREVNT>       m_lstLoginOrLogoutCenterEvent;        //��¼�����¼�
    std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>           m_listPlayEvent;                      //�㲥��Ϣ
    std::list<info_mgr::INFO_LINKSERVEREVENT>                m_listLinkSeverEvent;                 //LinkSever�¼�
    std::list<info_mgr::INFO_RESPONSETOCENTER>               m_listResponseToCenterEvent;          //���������¼���Ϣ
    std::list<info_mgr::INFO_SIGNALINGEXECRESULT>            m_lstSigalingExecRuslut;              //����ִ�н��

};
#endif//HISTORYINFO_H__
