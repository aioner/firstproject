#ifndef JK_DLL_H__
#include "jk/JKMainClient.h"

class jk_dll
{
	//���ܽӿ�
public:
	void init();
	void uninit();

	//����ص���
	////////////////////////////////////////////////////////////////////
	void  SetCallBackOnTellLocalIDS(OnTellLocalIDS CallBack ,void* pUser);

	void  SetCallBackOnLinkServer(OnLinkServer CallBack ,void* pUser);

	void  SetCallBackOnUserInOut(OnUserInOut CallBack ,void* pUser);

	void  SetCallBackOnDBImageCenter(OnDBImageCenter CallBack ,void* pUser);

	void  SetCallBackOnEventGetMsg(OnEventGetMsg CallBack ,void* pUser);

	void SetCallBackOnTransparentCommand(OnTransparentCommand CallBack ,void* pUser);
	////////////////////////////////////////////////////////////////////

	//ֹͣ���ӷ����� 
	void SetServerInfo( short sNum,const char* sIPS, long sPort);

	//���õ��ÿؼ��������ھ�������ڿؼ��ڲ��������������Ϣ
	void SetMainHwnd( long mhwnd);

	//��½ʱ����У������
	void StartLinkServer( short sNum);

	//ֹͣ���ӷ�����
	void StopLinkServer( short sNum);

	//���ñ������ͣ�������IDS
	void NewSetLocalType( long sType);

	//��½ʱ����У������
	long CheckPassword( long nType,const char* Name,const char* Mima,const char* sRes1, long iRes2);

	//��JK�õ�DVR�ĵ�½�û���������
	void GetLoginInfo(const char* sIDS, char* sName, char* sPassword, char* iPort, char* sRes1, char* iRes1);

    void GetLoginInformation(const char* sIDS, char* szName, char* szPassword, long *plPort, char* szRes1, long *plRes1);

	//��Ƶ������ͨ���˷���֪ͨ���Ķ�ĳ���豸�ĵ㲥�����������Ҫ����
	void SendDARReq(const char* sIDS,long sCH,	long iCodeType,	 long iRes1,const char* sRes1);

	//����͸���ַ������������¼�
	long SendTransparentCommand(const char* sIDS,const char* sIPS,const char* sCommands);

	//֪ͨ����ͼ��㲥��Ϣ����Ƶ������ר�ã�
	void SetVideoCenterPlayID(const char* strSID, long lDvrChID, long lVCenterChID, long iRes1,const char* sRes1);

    //֪ͨ����ͼ��㲥��Ϣ����Ƶ������ר�ã�
    void  SetVideoCenterPlayIDEx(const char* strSID, long lDvrChID, long lVCenterChID, long iRes1,const char* sRes1,const char* sExInfo);

	//���ñ�����½ʱע�ᵽ���ĵ����ֺ�IP��ַ
	void SendInfo(const char* myName,const char* myIPS);

	void SetLinkType(long sNum , long sType);

private:

};
#endif//#ifndef JK_DLL_H__