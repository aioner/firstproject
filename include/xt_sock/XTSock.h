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
	// ��ʼ��socket
	static bool InitSocket(void);

	// ����socket
	static SOCKET Socket();

	// ��
	static bool Bind(SOCKET sock, const char *ip, unsigned short port);

	// ����2IP
	static bool Name2IP(char *szName, char *szIP);

	// ���û�������С
	static bool SetSndBuf(SOCKET sock, UINT nSize);
	static bool SetRcvBuf(SOCKET sock, UINT nSize);

	// ���ó�ʱ����
	static bool SetSndTimeOut(SOCKET sock, int nTimeOut);
	static bool SetRcvTimeOut(SOCKET sock, int nTimeOut);

	//��������/������ģʽ(0:���� 1:������)
	static bool SetBlock(SOCKET sock, unsigned long ul);

	// ����
	static SOCKET Listen(const char *ip, unsigned short port);

	// ��������
	static SOCKET Accept(SOCKET sockListen, SOCKADDR_IN &addr_from);

	// ��������
	static SOCKET Connect(const char *szServerIP, USHORT nServerPort, int nTimeOut);
	static bool Connect(SOCKET sockConnect, const char *szServerIP, USHORT nServerPort, int nTimeOut);

	// ��������
	static bool SendData(SOCKET sock, char *pData, UINT nLen, int nTimeOut = -1);
	static bool SendData2(SOCKET sock, char *pData, UINT nLen);//������

	// ��������
	static bool RecvData(SOCKET sock, char *pData, UINT nLen, int nTimeOut = -1);
	static int RecvData2(SOCKET sock, char *pData, UINT nLen, int nTimeOut = -1);//������

	// �ر�socket
	static void CloseSocket(SOCKET &sock);

	// ����keep alive
	static void SetKeepLive(SOCKET sock);
};
