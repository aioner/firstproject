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

	// 清理
	void Clear();

	// 连接服务器
	int Connect(const std::string local_ip, unsigned short local_port,
				const std::string server_ip, unsigned short server_port, 
				unsigned short nTimeOut = 10);

	// 创建工作线程
	bool CreateWorkThread();

	// 退出接收线程
	void ExitWorkTread();

	// 接收数据
	bool RecvData();

	// 数据解析
	bool AnalyData(XT_MSG &pack, char *payload);

	// 注册
	int regist();

	// 数据接收线程
#ifdef _OS_WINDOWS
	static void __cdecl recvThreadFunc(LPVOID pParam);

	// 注册线程
	static void __cdecl registThreadFunc(LPVOID pParam);
#else
	static void* recvThreadFunc(LPVOID pParam);

	// 注册线程
	static void* registThreadFunc(LPVOID pParam);
#endif


private:
	// 连接sock
	SOCKET m_sockConnect;

	std::string m_ids;
	std::string m_localip;
	unsigned short m_localport;

	// 服务器信息
	std::string m_serverip;
	unsigned short m_serverport;

#ifdef _OS_WINDOWS
	// 数据接收线程
	HANDLE m_hRecv;

	// 注册线程
	HANDLE m_hRegist;
#else
	pthread_t m_hRecv;
	pthread_t m_hRegist;
#endif
	bool m_bExit;
};
