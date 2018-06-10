#include "config.h"
#include "XTSock.h"
#ifdef _OS_WINDOWS
#include <process.h>
#else
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif

#include "regist_server.h"


regist_server regist_server::m_server;
regist_server::regist_server(void)
{
	m_sockListen	= INVALID_SOCKET;
	m_hThreadListen = NULL;
	m_hThreadMsg	= NULL;
	m_bExitListen	= false;
	m_bExitMsg		= false;
	m_vecClient.clear();

#ifdef _OS_WINDOWS
	::InitializeCriticalSection(&m_csClient);
#endif
}

regist_server::~regist_server(void)
{
	Clear();
}

// 清理
void regist_server::Clear()
{
    //modified by lichao, 20150408 非select阻塞accept 先关闭socket
    // 关闭侦听sock
    if (m_sockListen != INVALID_SOCKET)
    {
#ifdef _OS_WINDOWS
        shutdown(m_sockListen, SD_BOTH);
#else
        shutdown(m_sockListen, SHUT_RDWR);
#endif
        XTSock::CloseSocket(m_sockListen);
    }

    // 退出监听线程
    ExitListenTread();

    // 退出消息线程
    ExitMsgTread();
	// 关闭客户端sock
	for (unsigned int nS = 0;nS < m_vecClient.size();++nS)
	{
		SOCKET &sock = m_vecClient[nS].sock;
		if (sock != INVALID_SOCKET)
		{
			XTSock::CloseSocket(sock);
		}
	}
	m_vecClient.clear();
}

bool regist_server::start(const char *ip, unsigned short port)
{
	bool ret = true;
	ret = Listen(ip, port);
	if (!ret)
	{
		return false;
	}

	ret = CreateListenThread();
	if (!ret)
	{
		return false;
	}

	ret = CreateMsgThread();
	if (!ret)
	{
		return false;
	}

	return true;
}

void regist_server::stop()
{
	Clear();
}

// 创建监听线程
bool regist_server::CreateListenThread()
{
	m_bExitListen = false;
#ifdef _OS_WINDOWS
	uintptr_t hThread = ::_beginthread(listenThreadFunc, 0, this);
	if (hThread == -1)
	{
		return false;
	}

	m_hThreadListen = (HANDLE)hThread;
#else
	if(pthread_create(&m_hThreadListen, NULL,listenThreadFunc, this))
	{
		m_hThreadListen=-1;
	    return false;
	}
#endif

	return true;
}

// 创建消息线程
bool regist_server::CreateMsgThread()
{
	m_bExitMsg = false;
#ifdef _OS_WINDOWS
	uintptr_t hThread = ::_beginthread(msgThreadFunc, 0, this);
	if (hThread == -1)
	{
		return false;
	}

	m_hThreadMsg = (HANDLE)hThread;
#else
	if(pthread_create(&m_hThreadMsg, NULL,msgThreadFunc, this))
	{
		m_hThreadMsg=-1;
	    return false;
	}
#endif
	return true;
}

// 退出监听线程
void regist_server::ExitListenTread()
{
	if (m_hThreadListen)
	{
		m_bExitListen = true;
#ifdef _OS_WINDOWS
		::WaitForSingleObject(m_hThreadListen, INFINITE);
#else
		 pthread_join(m_hThreadListen,NULL);
#endif

		m_hThreadListen = NULL;
	}
}

// 退出消息线程
void regist_server::ExitMsgTread()
{
	if (m_hThreadMsg)
	{
		m_bExitMsg = true;
        //modified by lichao, 20150408 msg线程 应该使用m_hThreadMsg变量
#ifdef _OS_WINDOWS
		::WaitForSingleObject(m_hThreadMsg, INFINITE);
#else
		 pthread_join(m_hThreadMsg,NULL);
#endif
		m_hThreadMsg = NULL;
	}
}

// 侦听
bool regist_server::Listen(const char *ip, unsigned short port)
{
	// 开始侦听
	m_sockListen = XTSock::Listen(ip, port);
	if (m_sockListen == INVALID_SOCKET)
	{
		return false;
	}

	return true;
}

// 接受连接
bool regist_server::Accept()
{
	Client client;

	SOCKET sock = XTSock::Accept(m_sockListen, client.addr);
	if (sock == INVALID_SOCKET)
	{
		return false;
	}

	// 设置超时
// 	if (!XTSock::SetRcvTimeOut(sock, TIMEOUT_RECV))
// 	{
// 		XTSock::CloseSocket(sock);
// 		return false;
// 	}
	if (!XTSock::SetSndTimeOut(sock, TIMEOUT_SEND))
	{
		XTSock::CloseSocket(sock);
		return false;
	}

	// 设置缓冲区
	if (!XTSock::SetRcvBuf(sock, BUF_RECV))
	{
		XTSock::CloseSocket(sock);
		return false;
	}
	if (!XTSock::SetSndBuf(sock, BUF_SEND))
	{
		XTSock::CloseSocket(sock);
		return false;
	}

	XTSock::SetKeepLive(sock);
	
	client.sock = sock;
	client.regist = false;
	AddClient(client);

	return true;
}

// 客户端管理
void regist_server::AddClient(Client &client)
{
#ifdef _OS_WINDOWS
	::EnterCriticalSection(&m_csClient);
#else
	m_csClient.lock();
#endif

	m_vecClient.push_back(client);

#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	m_csClient.unlock();
#endif
}
void regist_server::DelClient(SOCKET sock)
{
#ifdef _OS_WINDOWS
	::EnterCriticalSection(&m_csClient);
#else
	m_csClient.lock();
#endif

	std::vector<Client>::iterator itr = m_vecClient.begin();
	for (;itr != m_vecClient.end();++itr)
	{
		if (sock == itr->sock)
		{
			XTSock::CloseSocket(sock);
			m_vecClient.erase(itr);
			break;
		}
	}

#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	m_csClient.unlock();
#endif
}
int regist_server::GetClient(SOCKET sock, Client &client)
{
	int ret = -1;
#ifdef _OS_WINDOWS
	::EnterCriticalSection(&m_csClient);
#else
	m_csClient.lock();
#endif

	std::vector<Client>::iterator itr = m_vecClient.begin();
	for (;itr != m_vecClient.end();++itr)
	{
		if (sock == itr->sock)
		{
			client = *itr;
			ret = 0;
			break;
		}
	}

#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	m_csClient.unlock();
#endif
	return ret;
}

void regist_server::SetClient(SOCKET sock, const Client&client)
{
#ifdef _OS_WINDOWS
	::EnterCriticalSection(&m_csClient);
#else
	m_csClient.lock();
#endif

    std::vector<Client>::iterator itr = m_vecClient.begin();
    for (;itr != m_vecClient.end();++itr)
    {
        if (sock == itr->sock)
        {
            itr->addr = client.addr;
            itr->ids = client.ids;
            itr->usr_data = client.usr_data;
            break;
        }
    }


#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	m_csClient.unlock();
#endif
}

int regist_server::GetClient_IDS(const std::string ids, Client &client)
{
	int ret = -1;
#ifdef _OS_WINDOWS
	::EnterCriticalSection(&m_csClient);
#else
	m_csClient.lock();
#endif

	Client *pC = NULL;
	std::vector<Client>::iterator itr = m_vecClient.begin();
	for (;itr != m_vecClient.end();++itr)
	{
		if (ids == itr->ids)
		{
			client = *itr;
			ret = 0;
			break;
		}
	}

#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	m_csClient.unlock();
#endif
	return ret;
}
int regist_server::GetClient_IP(const std::string ip, Client &client)
{
	int ret = -1;
#ifdef _OS_WINDOWS
	::EnterCriticalSection(&m_csClient);
#else
	m_csClient.lock();
#endif

	Client *pC = NULL;
	std::vector<Client>::iterator itr = m_vecClient.begin();
	for (;itr != m_vecClient.end();++itr)
	{
		SOCKADDR_IN addr_in;
#ifdef _OS_WINDOWS
		addr_in.sin_addr.S_un.S_addr = ::inet_addr(ip.c_str());
#else
		in_addr_t addr=inet_addr(ip.c_str());
	    addr_in.sin_addr = *((struct in_addr*)&addr);
#endif
#ifdef _OS_WINDOWS
		if (addr_in.sin_addr.S_un.S_addr == itr->addr.sin_addr.S_un.S_addr)
#else
		if (memcmp(&addr_in.sin_addr, &itr->addr.sin_addr,sizeof(addr_in.sin_addr))==0)
#endif


		{
			client = *itr;
			ret = 0;
			break;
		}
	}

#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	m_csClient.unlock();
#endif

	return ret;
}

// 接收数据
bool regist_server::RecvData(SOCKET sock)
{
	bool bR = true;
#ifdef _OS_WINDOWS
	::EnterCriticalSection(&m_csClient);
#else
	m_csClient.lock();
#endif
	
	//////////////////////////////////////////////////////////////////////////
	Client client;
	int ret = GetClient(sock, client);
	if (ret < 0)
	{
		XTSock::CloseSocket(sock);
#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	m_csClient.unlock();
#endif
		return false;
	}

	char data[MAX_MSG_SIZE];
	unsigned len = SIZE_PACKHEAD;
	bool success = XTSock::RecvData(sock, data, len);
	if (!success)
	{
		DelClient(sock);
#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	m_csClient.unlock();
#endif
		return false;
	}

	XT_MSG msg;
	::memcpy(&msg.type, data, 4);
	::memcpy(&msg.size, data+4, 4);
	::memcpy(&msg.lpayload, data+8, 4);

	if (MAX_MSG_SIZE < msg.lpayload)
	{
		DelClient(sock);
#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	m_csClient.unlock();
#endif
		return false;
	}

	len = msg.lpayload;
	::memset(data, 0, MAX_MSG_SIZE);
	success = XTSock::RecvData(sock, data, len);
	if (!success)
	{
		DelClient(sock);
#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	m_csClient.unlock();
#endif
		return false;
	}

	// 数据解析
	bR = AnalyData(msg, data, client);
	//////////////////////////////////////////////////////////////////////////
#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&m_csClient);
#else
	  m_csClient.unlock();
#endif

    SetClient(sock, client);

	return bR;
}

// 数据解析
bool regist_server::AnalyData(XT_MSG &pack, char *payload, Client &client)
{
	bool bRet = true;

	switch (pack.type)
	{
	case PT_IDS:
		{
			client.ids = payload;
			client.regist = true;
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

// 监听线程
#ifdef _OS_WINDOWS
void __cdecl regist_server::listenThreadFunc(void* pParam)
{
	regist_server* pServer = (regist_server*)pParam;
	if (!pServer)
	{
		return;
	}

	while (!pServer->m_bExitListen)
	{
		pServer->Accept();
	}
}
#else
void* regist_server::listenThreadFunc(void* pParam)
{
	regist_server* pServer = (regist_server*)pParam;
	if (!pServer)
	{
		return NULL;
	}

	while (!pServer->m_bExitListen)
	{
		pServer->Accept();
	}
	return NULL;
}
#endif

// 消息线程
#ifdef _OS_WINDOWS
  void __cdecl regist_server::msgThreadFunc(void* pParam)
#else
  void* regist_server::msgThreadFunc(void* pParam)
#endif
{
	regist_server* pServer = (regist_server*)pParam;
	if (!pServer)
	{
#ifdef _OS_WINDOWS
		return;
#else
		return NULL;
#endif
	}

	// socket队列
	fd_set fdRead;

	while (!pServer->m_bExitMsg)
	{
		FD_ZERO(&fdRead);

		bool is_null = true;

	#ifdef _OS_WINDOWS
		::EnterCriticalSection(&pServer->m_csClient);
#else
		pServer->m_csClient.lock();
#endif
		std::vector<Client> &vecClient = pServer->m_vecClient;
		for (unsigned int nC = 0;nC < vecClient.size();++nC)
		{
			if (!vecClient[nC].regist)
			{
				is_null = false;
				FD_SET(vecClient[nC].sock, &fdRead);
			}
		}
#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&pServer->m_csClient);
#else
	pServer->m_csClient.unlock();
#endif

	if (is_null)
	{
		boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
		continue;
	}


	timeval tv = {1, 0}; 
	int nRet = ::select(0, &fdRead, NULL, NULL, &tv);   //3查询满足要求的套接字，不满足要求，出队  
	if (nRet == 0)      
	{   
		continue;     
	} 

	#ifdef _OS_WINDOWS
	 ::EnterCriticalSection(&pServer->m_csClient);
#else
	  pServer->m_csClient.lock();
#endif
		for (unsigned int nC = 0;nC < vecClient.size();++nC)
		{
			SOCKET sock = vecClient[nC].sock;
			if (FD_ISSET(sock, &fdRead))
			{
				if (!pServer->RecvData(sock))
				{
					pServer->DelClient(sock);
					nC -= 1;
				}
                else
                {
#ifdef _OS_WINDOWS
	            ::LeaveCriticalSection(&pServer->m_csClient);
#else
	              pServer->m_csClient.unlock();
#endif
 #ifdef _OS_WINDOWS
		return;
#else
		return NULL;
#endif
                }
			}
		}
		
#ifdef _OS_WINDOWS
	::LeaveCriticalSection(&pServer->m_csClient);
#else
	pServer->m_csClient.unlock();
#endif
	}
	#ifdef _OS_WINDOWS
		return;
#else
		return NULL;
#endif
}