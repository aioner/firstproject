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
		SOCKET		sock;			//连接句柄
		struct sockaddr_in addr;			//地址
		std::string	ids;			//IDS
		unsigned	usr_data;		//用户数据
		bool		regist;			//是否注册
	};

private:
	regist_server(void);
	~regist_server(void);

	static regist_server m_server;

public:
	static regist_server* instance(){return &m_server;}

	bool start(const char *ip, unsigned short port);

	void stop();

	// 清理
	void Clear();

	// 侦听
	bool Listen(const char *ip, unsigned short port);

	// 接受连接
	bool Accept();

	// 客户端管理
	void AddClient(Client &client);
	void DelClient(SOCKET sock);
    void SetClient(SOCKET sock, const Client&client);
	int GetClient(SOCKET sock, Client &client);
	int GetClient_IDS(const std::string ids, Client &client);
	int GetClient_IP(const std::string ip, Client &client);

	// 接收数据
	bool RecvData(SOCKET sock);

	// 数据解析
	bool AnalyData(XT_MSG &pack, char *payload, Client &client);

	// 创建监听线程
	bool CreateListenThread();

	// 创建消息线程
	bool CreateMsgThread();

	// 退出监听线程
	void ExitListenTread();

	// 退出消息线程
	void ExitMsgTread();

	// 监听线程
#ifdef _OS_WINDOWS
	static void __cdecl listenThreadFunc(void* pParam);

	// 消息线程
	static void __cdecl msgThreadFunc(void* pParam);
#else
	static void* listenThreadFunc(void* pParam);

	// 消息线程
	static void* msgThreadFunc(void* pParam);
#endif


private:
	// 临界区
#ifdef _OS_WINDOWS
	CRITICAL_SECTION m_csClient;
	HANDLE m_hThreadListen;
	HANDLE m_hThreadMsg;
#else
	boost::mutex   m_csClient;
	pthread_t  m_hThreadListen;
	pthread_t m_hThreadMsg;
#endif

	// 监听线程

	
	bool m_bExitListen;

	// 消息线程
	
	bool m_bExitMsg;

	// 侦听sock
	SOCKET m_sockListen;

	// 客户端
	std::vector<Client> m_vecClient;
};
#endif//REGIST_SERVER_H_INCLUDE__
