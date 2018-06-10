#include "XTSock.h"
#include "regist_client.h"
#ifdef _OS_WINDOWS
#include <process.h>
#else
#include <string.h>
#include <unistd.h>
#endif
#include "h_xtsessionserver.h"

regist_client regist_client::m_client;
regist_client::regist_client(void)
{
	m_sockConnect = INVALID_SOCKET;
}

regist_client::~regist_client(void)
{
	Clear();
}

// ����
void regist_client::Clear()
{
	XTSock::CloseSocket(m_sockConnect);

	ExitWorkTread();
}

int regist_client::start(const std::string ids, const std::string local_ip, unsigned short local_port,
						 const std::string server_ip, unsigned short server_port)
{
	int ret = 0;

	// ����������Ϣ
	m_ids = ids;
	m_localip = local_ip;
	m_localport = local_port;
	m_serverip = server_ip;
	m_serverport = server_port;

	bool r = CreateWorkThread();
	if (!r)
	{
		return -1;
	}	

	return ret;
}

void regist_client::stop()
{
	Clear();
}

int regist_client::get_sock(SOCKET &sock)
{
	if (m_sockConnect == INVALID_SOCKET)
	{
		sock = INVALID_SOCKET;
		return -1;
	}

	sock = m_sockConnect;
	return 0;
}

// �˳����ݽ����߳�
void regist_client::ExitWorkTread()
{
	m_bExit = true;

	if (m_hRegist)
	{		
#ifdef _OS_WINDOWS
		::WaitForSingleObject(m_hRegist, INFINITE);
#else
		pthread_join(m_hRegist,NULL);
#endif
		m_hRegist = NULL;
	}

	if (m_hRecv)
	{		
#ifdef _OS_WINDOWS
		::WaitForSingleObject(m_hRecv, INFINITE);
#else
		pthread_join(m_hRecv,NULL);
		m_hRecv = NULL;
#endif
	}
}

// ���ӷ�����
int regist_client::Connect(const std::string local_ip, unsigned short local_port,
							const std::string server_ip, unsigned short server_port, 
							unsigned short nTimeOut/* = 10*/)
{
	// ����
	SOCKET sockConnect = XTSock::Socket();
	if (INVALID_SOCKET == sockConnect)
	{
		return -1;
	}
	
	bool ret = XTSock::Bind(sockConnect, local_ip.c_str(), local_port);
	if (!ret)
	{
		XTSock::CloseSocket(sockConnect);
		return 1;
	}

	ret = XTSock::Connect(sockConnect, server_ip.c_str(), server_port, nTimeOut);
	if (!ret)
	{
		XTSock::CloseSocket(sockConnect);
		return -1;
	}

	// ���ó�ʱ
// 	if (!XTSock::SetRcvTimeOut(sockConnect, TIMEOUT_RECV))
// 	{
// 		XTSock::CloseSocket(sockConnect);
// 		return false;
// 	}
	if (!XTSock::SetSndTimeOut(sockConnect, TIMEOUT_SEND))
	{
		XTSock::CloseSocket(sockConnect);
		return -1;
	}

	// ���û�����
	if (!XTSock::SetRcvBuf(sockConnect, BUF_RECV))
	{
		XTSock::CloseSocket(sockConnect);
		return -1;
	}
	if (!XTSock::SetSndBuf(sockConnect, BUF_SEND))
	{
		XTSock::CloseSocket(sockConnect);
		return -1;
	}

	XTSock::SetKeepLive(sockConnect);

	XTSock::CloseSocket(m_sockConnect);
	m_sockConnect = sockConnect;

	return 0;
}

// �������ݽ����߳�
bool regist_client::CreateWorkThread()
{
	m_bExit = false;

// 	uintptr_t hThread = ::_beginthread(recvThreadFunc, 0, this);
// 	if (hThread == -1)
// 	{
// 		return false;
// 	}
// 	m_hRecv = (HANDLE)hThread;

#ifdef _OS_WINDOWS
	uintptr_t hThread = ::_beginthread(registThreadFunc, 0, this);
	if (hThread == -1)
	{
		return false;
	}
	m_hRegist = (HANDLE)hThread;
#else
	if(pthread_create(&m_hRegist,NULL,registThreadFunc,this))
	{
		return false;
	}
#endif
	return true;
}

// ��������
bool regist_client::RecvData()
{
	bool bR = true;
	if (INVALID_SOCKET == m_sockConnect)
	{
		return false;
	}

	char data[MAX_MSG_SIZE];
	unsigned len = SIZE_PACKHEAD;
	int recv = XTSock::RecvData(m_sockConnect, data, len);
	if (recv < len)
	{
		return false;
	}

	XT_MSG msg;
	::memcpy(&msg.type, data, 4);
	::memcpy(&msg.size, data+4, 4);
	::memcpy(&msg.lpayload, data+8, 4);

	if (MAX_MSG_SIZE < msg.lpayload)
	{
		return false;
	}

	len = msg.lpayload;
	recv = XTSock::RecvData(m_sockConnect, data, len);
	if (recv < len)
	{
		return false;
	}

	// ���ݽ���
	bR = AnalyData(msg, data);

	return bR;
}

// ���ݽ���
bool regist_client::AnalyData(XT_MSG &pack, char *payload)
{
	bool bRet = true;

	switch (pack.type)
	{
	case PT_IDS:
		{
			break;
		}
	default:
		{
			bRet = false;
			break;
		}
	}

	return bRet;
}

// ע��
int regist_client::regist()
{
	int ret = Connect(m_localip, m_localport, m_serverip, m_serverport);
	if (ret < 0)
	{
		return -1;
	}
	else if (ret > 0)
	{
		return 0;
	}

	XT_MSG msg;
	msg.type = PT_IDS;
	msg.size = SIZE_PACKHEAD;
	msg.lpayload = m_ids.size();

	char data[MAX_MSG_SIZE];
	::memset(data, 0, sizeof(data));
	unsigned len = 0;

	::memcpy(data+len, &msg.type, 4);	len+=4;
	::memcpy(data+len, &msg.size, 4);	len+=4;
	::memcpy(data+len, &msg.lpayload, 4);len+=4;
	::memcpy(data+len, m_ids.c_str(), m_ids.size());len+=m_ids.size();

	bool r = XTSock::SendData(m_sockConnect, data, len, 10);
	if (!r)
	{
		return -1;
	}

	xt_msg_add_socket_client((void *)m_sockConnect);

	return 0;
}

// ���ݽ����߳�
#ifdef _OS_WINDOWS
void __cdecl regist_client::recvThreadFunc(LPVOID pParam)
#else
void* regist_client::recvThreadFunc(LPVOID pParam)
#endif
{
	regist_client* pClient = (regist_client*)pParam;
	if (!pClient)
	{
#ifdef _OS_WINDOWS
	return ;
#else
	return NULL;
#endif

	}

	while (!pClient->m_bExit)
	{
		pClient->RecvData();
	}
#ifndef _OS_WINDOWS
	return NULL;
#endif
}

#ifdef _OS_WINDOWS
void __cdecl regist_client::registThreadFunc(LPVOID pParam)
#else
void* regist_client::registThreadFunc(LPVOID pParam)
#endif
{
	regist_client* pClient = (regist_client*)pParam;
	if (!pClient)
	{
#ifdef _OS_WINDOWS
	return ;
#else
	return NULL;
#endif
	}

	while (!pClient->m_bExit)
	{
		pClient->regist();
#ifdef _OS_WINDOWS
		Sleep(1000);
#else
		sleep(1);
#endif
	}
#ifndef _OS_WINDOWS
	return NULL;
#endif
}
