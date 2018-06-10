#pragma once
#include "XTDef.h"
#include <string>
#ifdef _OS_WINDOWS
#include <WinSock2.h>
#else
#define __cdecl
typedef void * LPVOID;
#define SOCKET		int
#endif
class regist_client
{
private:
	regist_client(void);
	~regist_client(void);

	static regist_client m_client;

public:
	static regist_client* instance(){return &m_client;}

	int start(const std::string ids, const std::string local_ip, unsigned short local_port,
				const std::string server_ip, unsigned short server_port);

	void stop();

	int get_sock(SOCKET &sock);

	// ����
	void Clear();

	// ���ӷ�����
	int Connect(const std::string local_ip, unsigned short local_port,
				const std::string server_ip, unsigned short server_port, 
				unsigned short nTimeOut = 10);

	// ���������߳�
	bool CreateWorkThread();

	// �˳������߳�
	void ExitWorkTread();

	// ��������
	bool RecvData();

	// ���ݽ���
	bool AnalyData(XT_MSG &pack, char *payload);

	// ע��
	int regist();

	// ���ݽ����߳�
#ifdef _OS_WINDOWS
	static void __cdecl recvThreadFunc(LPVOID pParam);

	// ע���߳�
	static void __cdecl registThreadFunc(LPVOID pParam);
#else
	static void* recvThreadFunc(LPVOID pParam);

	// ע���߳�
	static void* registThreadFunc(LPVOID pParam);
#endif


private:
	// ����sock
	SOCKET m_sockConnect;

	std::string m_ids;
	std::string m_localip;
	unsigned short m_localport;

	// ��������Ϣ
	std::string m_serverip;
	unsigned short m_serverport;

#ifdef _OS_WINDOWS
	// ���ݽ����߳�
	HANDLE m_hRecv;

	// ע���߳�
	HANDLE m_hRegist;
#else
	pthread_t m_hRecv;
	pthread_t m_hRegist;
#endif
	bool m_bExit;
};
