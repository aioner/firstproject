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
		//1:��¼�ǳ�����
	case info_mgr::INFO_LOGIN_OR_LOGOUT_CENTER_EVNT_ID:
		{
			//������ʷ��¼
			CHistoryInfo::instance()->SaveLoginOrLogoutCenterEvent(
				info_mgr::B2C<info_mgr::INFO_LOGINORLOGOUTCENTEREVNT>(pInfoData));
			break;		
		}

		//2:���ĵ㲥����
	case info_mgr::INFO_CENTER_DB_COMMAND_EVENT:
		{
			//Ͷ��ʵʱ��ʾ
			CRealInfo::instance()->PostCenterDbCommandEventInfo(
				info_mgr::B2C<info_mgr::INFO_CENTERDBCOMMANDEVENT>(pInfoData));

			//������ʷ��¼
			CHistoryInfo::instance()->SavePlayEventInfo(
				info_mgr::B2C<info_mgr::INFO_CENTERDBCOMMANDEVENT>(pInfoData));
			
			break;
		}

		//3:���������¼�
	case info_mgr::INFO_RESPONSE_TO_CENTER_EVET_ID:
		{
			//Ͷ��ʵʱ��ʾ
			CRealInfo::instance()->PostRealResponseToCenterEventInfo(
				info_mgr::B2C<info_mgr::INFO_RESPONSETOCENTER>(pInfoData));

			//������ʷ��¼
			CHistoryInfo::instance()->SaveResponseToCenterEvent(
				info_mgr::B2C<info_mgr::INFO_RESPONSETOCENTER>(pInfoData));
			
			break;
		}

		//4:����ִ�н��
	case info_mgr::INFO_SIGNALING_EXEC_RESULT_ID:
		{
			//Ͷ��ʵʱ��ʾ
			CRealInfo::instance()->PostRealSigalingExecRet(
				info_mgr::B2C<info_mgr::INFO_SIGNALINGEXECRESULT>(pInfoData));

			//������ʷ��¼
			CHistoryInfo::instance()->SaveSigalingExecRet(
				info_mgr::B2C<info_mgr::INFO_SIGNALINGEXECRESULT>(pInfoData));

			break;
		}

		//5:LinkSever����ע��
	case info_mgr::INFO_LINK_SEVER_EVENT:
		{
			//������ʷ��¼
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
