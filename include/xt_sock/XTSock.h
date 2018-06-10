#pragma once
#ifdef _OS_WINDOWS
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
typedef int SOCKET;
typedef unsigned int UINT;
typedef unsigned short USHORT;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SOCKADDR_IN struct sockaddr_in
#endif

class XTSock
{
public:
	XTSock(void);
	~XTSock(void);

public:
	// 初始化socket
	static bool InitSocket(void);

	// 创建socket
	static SOCKET Socket();

	// 绑定
	static bool Bind(SOCKET sock, const char *ip, unsigned short port);

	// 域名2IP
	static bool Name2IP(char *szName, char *szIP);

	// 设置缓冲区大小
	static bool SetSndBuf(SOCKET sock, UINT nSize);
	static bool SetRcvBuf(SOCKET sock, UINT nSize);

	// 设置超时参数
	static bool SetSndTimeOut(SOCKET sock, int nTimeOut);
	static bool SetRcvTimeOut(SOCKET sock, int nTimeOut);

	//设置阻塞/非阻塞模式(0:阻塞 1:非阻塞)
	static bool SetBlock(SOCKET sock, unsigned long ul);

	// 侦听
	static SOCKET Listen(const char *ip, unsigned short port);

	// 接受连接
	static SOCKET Accept(SOCKET sockListen, SOCKADDR_IN &addr_from);

	// 主动连接
	static SOCKET Connect(const char *szServerIP, USHORT nServerPort, int nTimeOut);
	static bool Connect(SOCKET sockConnect, const char *szServerIP, USHORT nServerPort, int nTimeOut);

	// 发送数据
	static bool SendData(SOCKET sock, char *pData, UINT nLen, int nTimeOut = -1);
	static bool SendData2(SOCKET sock, char *pData, UINT nLen);//非阻塞

	// 接收数据
	static bool RecvData(SOCKET sock, char *pData, UINT nLen, int nTimeOut = -1);
	static int RecvData2(SOCKET sock, char *pData, UINT nLen, int nTimeOut = -1);//非阻塞

	// 关闭socket
	static void CloseSocket(SOCKET &sock);

	// 设置keep alive
	static void SetKeepLive(SOCKET sock);
};
