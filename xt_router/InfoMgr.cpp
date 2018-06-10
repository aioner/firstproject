#include "InfoMgr.h"

CInfoMgr CInfoMgr::m_Obj;

CInfoMgr::CInfoMgr(void)
{

}

CInfoMgr::~CInfoMgr(void)
{
}

void CInfoMgr::SetEventRealShowFlg(const info_mgr::INFO_TYPE ulInfoType,const bool bIsRealShowFlg/* = true*/)
{
	CRealInfo::instance()->SetRealShowFlg(ulInfoType,bIsRealShowFlg);
}

void CInfoMgr::GetConnectInfo(std::list<info_mgr::INFO_TRANS>& lstTransInfo)
{
	CRealInfo::instance()->GetConnectInfo(lstTransInfo);
}

void CInfoMgr::GetPlayInfo(std::list<info_mgr::INFO_PLAY>& lstPlayInfo)
{
	CRealInfo::instance()->GetPlayInfo(lstPlayInfo);
}


void CInfoMgr::PostEventInfo(const info_mgr::INFO_TYPE ulInfoType,info_mgr::PINFOBASE pInfoData)
{
	if (!pInfoData)
	{
		return;
	}
	switch (ulInfoType)
	{	
		//1:登录登出中心
	case info_mgr::INFO_LOGIN_OR_LOGOUT_CENTER_EVNT_ID:
		{
			//保存历史记录
			CHistoryInfo::instance()->SaveLoginOrLogoutCenterEvent(
				info_mgr::B2C<info_mgr::INFO_LOGINORLOGOUTCENTEREVNT>(pInfoData));
			break;		
		}

		//2:中心点播信令
	case info_mgr::INFO_CENTER_DB_COMMAND_EVENT:
		{
			//投递实时显示
			CRealInfo::instance()->PostCenterDbCommandEventInfo(
				info_mgr::B2C<info_mgr::INFO_CENTERDBCOMMANDEVENT>(pInfoData));

			//保存历史记录
			CHistoryInfo::instance()->SavePlayEventInfo(
				info_mgr::B2C<info_mgr::INFO_CENTERDBCOMMANDEVENT>(pInfoData));
			
			break;
		}

		//3:反馈中心事件
	case info_mgr::INFO_RESPONSE_TO_CENTER_EVET_ID:
		{
			//投递实时显示
			CRealInfo::instance()->PostRealResponseToCenterEventInfo(
				info_mgr::B2C<info_mgr::INFO_RESPONSETOCENTER>(pInfoData));

			//保存历史记录
			CHistoryInfo::instance()->SaveResponseToCenterEvent(
				info_mgr::B2C<info_mgr::INFO_RESPONSETOCENTER>(pInfoData));
			
			break;
		}

		//4:信令执行结果
	case info_mgr::INFO_SIGNALING_EXEC_RESULT_ID:
		{
			//投递实时显示
			CRealInfo::instance()->PostRealSigalingExecRet(
				info_mgr::B2C<info_mgr::INFO_SIGNALINGEXECRESULT>(pInfoData));

			//保存历史记录
			CHistoryInfo::instance()->SaveSigalingExecRet(
				info_mgr::B2C<info_mgr::INFO_SIGNALINGEXECRESULT>(pInfoData));

			break;
		}

		//5:LinkSever中心注消
	case info_mgr::INFO_LINK_SEVER_EVENT:
		{
			//保存历史记录
			CHistoryInfo::instance()->SaveLinkSeverEventInfo(
				info_mgr::B2C<info_mgr::INFO_LINKSERVEREVENT>(pInfoData));
			
			break;
		}
	default:
		{			
			break;
		}
	}

}
