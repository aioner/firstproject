#ifndef SPECIALLINE_H_
#define SPECIALLINE_H_

#include <list>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <boost/atomic.hpp>

#include "xtXml.h"

#define SpLineLog "ר�߱���"

#define USE_SplecialLine_func

typedef struct _struct_SpecialLine //��������
{
	std::string m_strSIDS; //  ר���е�Դ����IDS
	std::string	m_strSName; //ר���е�Դ��������
	std::string m_strSIP;  //���Ͷ�ԭʼIP
	std::string	 m_strSZIP;  // ���Ͷ�ר��IP
	std::string m_strDIDS;   //ר���е�Ŀ������IDS;
	std::string m_strDName; //ר���е�Ŀ����������
	std::string m_strDIP; //���ն�ԭʼIP
	std::string m_strDZIP;   //ר���е�Ŀ��IP
	std::string m_strTime;//���һ��״̬�仯ʱ�� (��-��-�գ�ʱ:��:��) ��Ƶ�������ϵ�ʱ�䣨�Ա��ѯ��
	int m_iStat;//����״̬ (0:���ߣ�1������)

	_struct_SpecialLine()
	{
		m_iStat = -1;
	}

} SPLINEINFO, *PSPLINEINFO;

class SpecialLine
{

//�������
protected:
	SpecialLine(void);
	~SpecialLine(void);

//�����ӿ�
public:
	static SpecialLine* instance()
	{
		return &m_ins;
	}

//�ӿ�
public:
	void ChangeRouterSet(const char *pstrRouterSet, const long nSize);


	//ר�߼��
	void Check(std::string& strRetSPLine);

	//��ȡ����ר������
	void GetLocalLineCfg(std::string & strLocalCfg);

	//��ȡ����ר��
	void GetBreakSPLine(std::list<SPLINEINFO>& lstBreakLine);

protected:

	//���ר��״̬
	int CheckState(const char *lpdest);

	//��ȡ���ʱ��
	std::string GetCheckTime();

	//��ȡ����IP�б�
	void GetLocalIpList(std::list<std::string>& lstIP);

	//����XML��ר������
	void Analyze();

//�������
private:

	int m_iPingPacklen;//Ping������
	int m_iPingtimeout;//Ping��ʱ

//����Data���
private:

	xtXml m_xmlSPLineInfo;//����ר����Ϣ	

	std::list<SPLINEINFO> m_lstLineInfo;

	boost::atomic<bool> m_bUpate; //�����Ƿ��͸��µ�ר����Ϣ

	std::list<SPLINEINFO> m_lstBreakLine;//�Ѿ����ߵ�ר��

//�̰߳�ȫ���
private:
	
	//�̰߳�ȫ
	boost::detail::spinlock  m_xmlSPLineInfoMutex;     //ר����Ϣ��

    static SpecialLine m_ins;
};

#endif //SPECIALLINE_H_