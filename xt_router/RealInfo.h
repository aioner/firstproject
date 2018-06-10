#ifndef REALINFO_H
#define REALINFO_H

#include <boost/thread/recursive_mutex.hpp>
#include <list>
#include "InfoTypeDef.h"

class CRealInfo
{
protected:
	CRealInfo(void);
	~CRealInfo(void);
public:
	static CRealInfo* instance()
	{
		return &m_obj;
	}

public:
	void GetConnectInfo(std::list<info_mgr::INFO_TRANS>& lstTransInfo);

	void GetPlayInfo(std::list<info_mgr::INFO_PLAY>& lstPlayInfo);

	//投递中心信令事件信息
	void PostCenterDbCommandEventInfo(const info_mgr::INFO_CENTERDBCOMMANDEVENT& infoRealPlayEvent);
	void GetCenterDbCommandEventInfo(std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>& listRealPlayEvent);
	
	//设置实时显示交换运行状态开关 -true：开
	void SetRealShowFlg(const info_mgr::INFO_TYPE ulInfoType,const bool bIsRealShowFlg = true);

	//投递反馈中心事件信息到显示
	void PostRealResponseToCenterEventInfo(const info_mgr::INFO_RESPONSETOCENTER& infoResponseToCenterEvent);
	void GetRealResponseToCenterEventInfo(std::list<info_mgr::INFO_RESPONSETOCENTER>& listRealResponseToCenterEvent);

	//信令执行结果
	void PostRealSigalingExecRet(const info_mgr::INFO_SIGNALINGEXECRESULT& info);
	void GetRealSigalingExecRet(std::list<info_mgr::INFO_SIGNALINGEXECRESULT>& lstSigalingExecRuslut);
	
private:
	boost::recursive_mutex m_CenterDbCommandFlgMutex;
	bool m_bCenterDbCommandShowFlg;                     //中心信令实时显示开关

	boost::recursive_mutex m_ResponseToCenterFlgMutex;
	bool m_bResponseToCenterShowFlg;                    //反馈中心事件显示开关

	boost::recursive_mutex m_SigalingExecRetFlgMutex;
	bool m_bSigalingExecRetShowFlg;                    //信令执行结果显示开关

private:
	boost::recursive_mutex        m_PlayRealEventMutex;     //点播事件信息
	boost::recursive_mutex		m_mutex;		      //全局锁
	boost::recursive_mutex        m_ResponseToCenterMutex;  //反馈中心事件信息事件锁
	boost::recursive_mutex        m_SigalingExecRuslutMutex;           //信令执行结果
	
	static CRealInfo m_obj;

private:	
	std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>   m_listRealPlayEvent;   //点播事件信息
	std::list<info_mgr::INFO_RESPONSETOCENTER>       m_listRealResponseToCenterEvent;  //反馈中心事件信息
	std::list<info_mgr::INFO_SIGNALINGEXECRESULT>    m_lstRealSigalingExecRuslut;              //信令执行结果

};

#endif//REALINFO_H
