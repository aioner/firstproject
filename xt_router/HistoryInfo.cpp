#include "HistoryInfo.h"

CHistoryInfo CHistoryInfo::m_obj;

CHistoryInfo::CHistoryInfo(void)
{
	m_uiMaxSaveNum = 2000;
	m_listPlayEvent.clear();
	m_listLinkSeverEvent.clear();
	m_listResponseToCenterEvent.clear();
	m_lstLoginOrLogoutCenterEvent.clear();
	m_lstSigalingExecRuslut.clear();
}

CHistoryInfo::~CHistoryInfo(void)
{
}

void CHistoryInfo::SaveLoginOrLogoutCenterEvent(const info_mgr::INFO_LOGINORLOGOUTCENTEREVNT& infoLoginOrLogoutCenterEvent)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_LoginOrLogoutCenterMutex);
	if (m_lstLoginOrLogoutCenterEvent.size() > m_uiMaxSaveNum)
	{
		m_lstLoginOrLogoutCenterEvent.clear();
		m_lstLoginOrLogoutCenterEvent.push_back(infoLoginOrLogoutCenterEvent);
	}
	else
	{
		m_lstLoginOrLogoutCenterEvent.push_back(infoLoginOrLogoutCenterEvent);
	}

}
void CHistoryInfo::GetLoginOrLogoutCenterEvent(std::list<info_mgr::INFO_LOGINORLOGOUTCENTEREVNT>& lstLoginOrLogoutCenterEvent)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_LoginOrLogoutCenterMutex);
	lstLoginOrLogoutCenterEvent = m_lstLoginOrLogoutCenterEvent;
}


void CHistoryInfo::SavePlayEventInfo(const info_mgr::INFO_CENTERDBCOMMANDEVENT& infoPlayEvent)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_PlayEventMutex);
	if (m_listPlayEvent.size() > m_uiMaxSaveNum)
	{
		m_listPlayEvent.clear();
		m_listPlayEvent.push_back(infoPlayEvent);
	}
	else
	{
		m_listPlayEvent.push_back(infoPlayEvent);
	}

}

void CHistoryInfo::GetPlayEventInfo(std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>& listPlayEvent)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_PlayEventMutex);
	listPlayEvent = m_listPlayEvent;
}

void CHistoryInfo::SaveLinkSeverEventInfo(const info_mgr::INFO_LINKSERVEREVENT& infoLinkSeverEvent)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_LinkSeverMutex);
	if (m_listLinkSeverEvent.size() > m_uiMaxSaveNum)
	{
		m_listLinkSeverEvent.clear();
		m_listLinkSeverEvent.push_back(infoLinkSeverEvent);
	}
	else
	{
		m_listLinkSeverEvent.push_back(infoLinkSeverEvent);
	}

}

void CHistoryInfo::GetLinkSeverEventInfo(std::list<info_mgr::INFO_LINKSERVEREVENT>& listLinkSeverEvent)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_LinkSeverMutex);
	listLinkSeverEvent = m_listLinkSeverEvent;
}


void CHistoryInfo::SaveResponseToCenterEvent(const info_mgr::INFO_RESPONSETOCENTER& infoResponseToCenterEvent)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_ResponseToCenterMutex);
	if (m_listResponseToCenterEvent.size() > m_uiMaxSaveNum)
	{
		m_listResponseToCenterEvent.clear();
		m_listResponseToCenterEvent.push_back(infoResponseToCenterEvent);
	}
	else
	{
		m_listResponseToCenterEvent.push_back(infoResponseToCenterEvent);
	}


}
void CHistoryInfo::GetResponseToCenterEvent(std::list<info_mgr::INFO_RESPONSETOCENTER>& listResponseToCenterEvent)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_ResponseToCenterMutex);
	listResponseToCenterEvent = m_listResponseToCenterEvent;
}

void CHistoryInfo::SaveSigalingExecRet(const info_mgr::INFO_SIGNALINGEXECRESULT& info)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_SigalingExecRuslutMutex);
	if (m_lstSigalingExecRuslut.size() > m_uiMaxSaveNum)
	{
		m_lstSigalingExecRuslut.clear();
		m_lstSigalingExecRuslut.push_back(info);
	}
	else
	{
		m_lstSigalingExecRuslut.push_back(info);
	}

}
void CHistoryInfo::GetSigalingExecRet(std::list<info_mgr::INFO_SIGNALINGEXECRESULT>& lstSigalingExecRuslut)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_SigalingExecRuslutMutex);
	lstSigalingExecRuslut = m_lstSigalingExecRuslut;
}

