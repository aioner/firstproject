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

	//Ͷ�����������¼���Ϣ
	void PostCenterDbCommandEventInfo(const info_mgr::INFO_CENTERDBCOMMANDEVENT& infoRealPlayEvent);
	void GetCenterDbCommandEventInfo(std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>& listRealPlayEvent);
	
	//����ʵʱ��ʾ��������״̬���� -true����
	void SetRealShowFlg(const info_mgr::INFO_TYPE ulInfoType,const bool bIsRealShowFlg = true);

	//Ͷ�ݷ��������¼���Ϣ����ʾ
	void PostRealResponseToCenterEventInfo(const info_mgr::INFO_RESPONSETOCENTER& infoResponseToCenterEvent);
	void GetRealResponseToCenterEventInfo(std::list<info_mgr::INFO_RESPONSETOCENTER>& listRealResponseToCenterEvent);

	//����ִ�н��
	void PostRealSigalingExecRet(const info_mgr::INFO_SIGNALINGEXECRESULT& info);
	void GetRealSigalingExecRet(std::list<info_mgr::INFO_SIGNALINGEXECRESULT>& lstSigalingExecRuslut);
	
private:
	boost::recursive_mutex m_CenterDbCommandFlgMutex;
	bool m_bCenterDbCommandShowFlg;                     //��������ʵʱ��ʾ����

	boost::recursive_mutex m_ResponseToCenterFlgMutex;
	bool m_bResponseToCenterShowFlg;                    //���������¼���ʾ����

	boost::recursive_mutex m_SigalingExecRetFlgMutex;
	bool m_bSigalingExecRetShowFlg;                    //����ִ�н����ʾ����

private:
	boost::recursive_mutex        m_PlayRealEventMutex;     //�㲥�¼���Ϣ
	boost::recursive_mutex		m_mutex;		      //ȫ����
	boost::recursive_mutex        m_ResponseToCenterMutex;  //���������¼���Ϣ�¼���
	boost::recursive_mutex        m_SigalingExecRuslutMutex;           //����ִ�н��
	
	static CRealInfo m_obj;

private:	
	std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>   m_listRealPlayEvent;   //�㲥�¼���Ϣ
	std::list<info_mgr::INFO_RESPONSETOCENTER>       m_listRealResponseToCenterEvent;  //���������¼���Ϣ
	std::list<info_mgr::INFO_SIGNALINGEXECRESULT>    m_lstRealSigalingExecRuslut;              //����ִ�н��

};

#endif//REALINFO_H
