#ifndef REGIST_SERVER_H_INCLUDE__
#define REGIST_SERVER_H_INCLUDE__

#include "XTDef.h"
#include <vector>
#include <string>
#include <boost/thread/mutex.hpp>
#include "utility/thread.h"

#ifndef _OS_WINDOWS
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#endif   //#ifndef _OS_WINDOWS 

class regist_server
{
public:
	struct  Client
	{
		SOCKET		sock;			//���Ӿ��
		struct sockaddr_in addr;			//��ַ
		std::string	ids;			//IDS
		unsigned	usr_data;		//�û�����
		bool		regist;			//�Ƿ�ע��
	};

private:
	regist_server(void);
	~regist_server(void);

	static regist_server m_server;

public:
	static regist_server* instance(){return &m_server;}

	bool start(const char *ip, unsigned short port);

	void stop();

	// ����
	void Clear();

	// ����
	bool Listen(const char *ip, unsigned short port);

	// ��������
	bool Accept();

	// �ͻ��˹���
	void AddClient(Client &client);
	void DelClient(SOCKET sock);
    void SetClient(SOCKET sock, const Client&client);
	int GetClient(SOCKET sock, Client &client);
	int GetClient_IDS(const std::string ids, Client &client);
	int GetClient_IP(const std::string ip, Client &client);

	// ��������
	bool RecvData(SOCKET sock);

	// ���ݽ���
	bool AnalyData(XT_MSG &pack, char *payload, Client &client);

	// ���������߳�
	bool CreateListenThread();

	// ������Ϣ�߳�
	bool CreateMsgThread();

	// �˳������߳�
	void ExitListenTread();

	// �˳���Ϣ�߳�
	void ExitMsgTread();

	// �����߳�
#ifdef _OS_WINDOWS
	static void __cdecl listenThreadFunc(void* pParam);

	// ��Ϣ�߳�
	static void __cdecl msgThreadFunc(void* pParam);
#else
	static void* listenThreadFunc(void* pParam);

	// ��Ϣ�߳�
	static void* msgThreadFunc(void* pParam);
#endif


private:
	// �ٽ���
#ifdef _OS_WINDOWS
	CRITICAL_SECTION m_csClient;
	HANDLE m_hThreadListen;
	HANDLE m_hThreadMsg;
#else
	boost::mutex   m_csClient;
	pthread_t  m_hThreadListen;
	pthread_t m_hThreadMsg;
#endif

	// �����߳�

	
	bool m_bExitListen;

	// ��Ϣ�߳�
	
	bool m_bExitMsg;

	// ����sock
	SOCKET m_sockListen;

	// �ͻ���
	std::vector<Client> m_vecClient;
};
#endif//REGIST_SERVER_H_INCLUDE__
