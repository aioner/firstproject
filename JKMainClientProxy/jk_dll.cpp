#include "jk_dll.h"

void jk_dll::init()
{
	::Initialize();	
}

void jk_dll::uninit()
{  
	::UnInitialize();
}

void  jk_dll::SetCallBackOnTellLocalIDS(OnTellLocalIDS CallBack ,void* pUser)
{
	::SetCallBackOnTellLocalIDS(CallBack,pUser);
}

void  jk_dll::SetCallBackOnLinkServer(OnLinkServer CallBack ,void* pUser)
{
	::SetCallBackOnLinkServer(CallBack,pUser);
}

void  jk_dll::SetCallBackOnUserInOut(OnUserInOut CallBack ,void* pUser)
{
	::SetCallBackOnUserInOut(CallBack,pUser);
}

void  jk_dll::SetCallBackOnDBImageCenter(OnDBImageCenter CallBack ,void* pUser)
{
	::SetCallBackOnDBImageCenter(CallBack,pUser);
}

void  jk_dll::SetCallBackOnEventGetMsg(OnEventGetMsg CallBack ,void* pUser)
{
	::SetCallBackOnEventGetMsg(CallBack,pUser);
}

void jk_dll::SetCallBackOnTransparentCommand(OnTransparentCommand CallBack ,void* pUser)
{
	::SetCallBackOnTransparentCommand(CallBack,pUser);
}

// ���õ��ÿؼ��������ھ�������ڿؼ��ڲ��������������Ϣ
void jk_dll::SetMainHwnd( long mhwnd)
{
	::SetMainHwnd(mhwnd);
}

// ֹͣ���ӷ����� 
void jk_dll::SetServerInfo( short sNum,const char* sIPS, long sPort)
{
	::SetServerInfo(sNum,sIPS,sPort);
}

// ��½ʱ����У������
void jk_dll::StartLinkServer( short sNum)
{
	::StartLinkServer(sNum);
}

//ֹͣ���ӷ�����
void jk_dll::StopLinkServer( short sNum)
{
	::StopLinkServer(sNum);
}

//���ñ������ͣ�������IDS 
void jk_dll::NewSetLocalType( long sType)
{
	::NewSetLocalType(sType);	
}

//��½ʱ����У������
long jk_dll::CheckPassword( long nType,const char* Name,const char* Mima,const char* sRes1, long iRes2)
{
	return ::CheckPassword( nType,  Name, Mima, sRes1,iRes2);
}

// ��JK�õ�DVR�ĵ�½�û���������
void jk_dll::GetLoginInfo(const char* sIDS, char* sName, char* sPassword, char* iPort, char* sRes1, char* iRes1)
{
	::GetLoginInfo(sIDS,sName,sPassword,iPort,sRes1,iRes1);
}

void jk_dll::GetLoginInformation(const char* sIDS, char* szName, char* szPassword, long *plPort, char* szRes1, long *plRes1)
{
    ::GetLoginInformation(sIDS,szName,szPassword,plPort,szRes1,plRes1);
}
//����͸���ַ������������¼�
long jk_dll::SendTransparentCommand(const char* sIDS,const char* sIPS,const char* sCommands)
{
	return ::SendTransparentCommand(sIDS,sIPS,sCommands);
}

//֪ͨ����ͼ��㲥��Ϣ����Ƶ������ר�ã�
void jk_dll::SetVideoCenterPlayID(const char* strSID, long lDvrChID, long lVCenterChID, long iRes1,const char* sRes1)
{
	::SetVideoCenterPlayID(strSID,lDvrChID,lVCenterChID,iRes1,sRes1);
}

void jk_dll::SetVideoCenterPlayIDEx(const char* strSID, long lDvrChID, long lVCenterChID, long iRes1,const char* sRes1,const char* sExInfo)
{
    ::SetVideoCenterPlayIDEx(strSID,lDvrChID,lVCenterChID,iRes1,sRes1,sExInfo);
}

//���ñ�����½ʱע�ᵽ���ĵ����ֺ�IP��ַ
void jk_dll::SendInfo(const char* myName,const char* myIPS)
{
	::SendInfo(myName, myIPS); 
}

void jk_dll::SendDARReq(const char* sIDS,long sCH,	long iCodeType,	 long iRes1,const char* sRes1)
{
	::SendDARReq(sIDS,sCH,iCodeType,iRes1,sRes1); 
}

void jk_dll::SetLinkType(long sNum , long sType)
{
	::SetLinkType(sNum,sType);
}