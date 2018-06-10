#include "XTSock.h"
#ifdef _OS_WINDOWS
#include <MSTcpIP.h>
#else
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <fcntl.h>
#endif
XTSock::XTSock(void)
{
}
XTSock::~XTSock(void)
{
}

// ³õÊ¼»¯socket
bool XTSock::InitSocket(void)
{
#ifdef _OS_WINDOWS
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 2, 2 );

	err = ::WSAStartup(wVersionRequested, &wsaData);
	if ( err != 0 ) 
	{
		return false;
	}

	if ( LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2 ) 
	{
		return false; 
	}
#endif
	return true;
}

// ÓòÃû2IP
bool XTSock::Name2IP(char *szName, char *szIP)
{
	hostent *host = ::gethostbyname(szName);
	if (!host)
	{
		return false;
	}

	char *szAddr = ::inet_ntoa(*(in_addr*)(host->h_addr));

	::strcpy(szIP, szAddr);

	return true;
}

// ´´½¨socket
SOCKET XTSock::Socket()
{
	SOCKET sockConnect = ::socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sockConnect)
	{
		return INVALID_SOCKET;
	}

	return sockConnect;
}

// °ó¶¨
bool XTSock::Bind(SOCKET sock, const char *ip, unsigned short port)
{
	SOCKADDR_IN addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = ::htons(port);
#ifdef _OS_WINDOWS
	addr_in.sin_addr.S_un.S_addr = ::inet_addr(ip);
#else
	in_addr_t addr=inet_addr(ip);
	addr_in.sin_addr = *((struct in_addr*)&addr);
#endif
	int nRet = ::bind(sock, (sockaddr*)&addr_in, sizeof(addr_in));	
	if (SOCKET_ERROR == nRet)
	{
		return false;
	}

	return true;
}


// ¹Ø±Õsocket
void XTSock::CloseSocket(SOCKET &sock)
{
	if (sock != INVALID_SOCKET)
	{
#ifdef _OS_WINDOWS
		::closesocket(sock);
#else
		close(sock);
#endif
		sock = INVALID_SOCKET;
	}
}

// ÉèÖÃ»º³åÇø´óÐ¡
bool XTSock::SetSndBuf(SOCKET sock, UINT nSize)
{
	int nBuf = nSize;
#ifdef _OS_WINDOWS
	int nBufLen = sizeof(nBuf);
#else
	socklen_t nBufLen = sizeof(nBuf);
#endif
	int nRe = ::setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&nBuf, nBufLen);
	if(SOCKET_ERROR == nRe)
	{
		return false;
	}

	//¼ì²é»º³åÇøÊÇ·ñÉèÖÃ³É¹¦
	nRe = ::getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&nBuf, &nBufLen);
	if(nSize != nBuf)
	{
		return false;
	}

	return true;
}
bool XTSock::SetRcvBuf(SOCKET sock, UINT nSize)
{
	int nBuf = nSize;
#ifdef _OS_WINDOWS
	int nBufLen = sizeof(nBuf);
#else
	socklen_t nBufLen = sizeof(nBuf);
#endif
	int nRe = ::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&nBuf, nBufLen);
	if(SOCKET_ERROR == nRe)
	{
		return false;
	}

	//¼ì²é»º³åÇøÊÇ·ñÉèÖÃ³É¹¦
	nRe = ::getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&nBuf, &nBufLen);
	if(nSize != nBuf)
	{
		return false;
	}

	return true;
}

// ÉèÖÃ³¬Ê±²ÎÊý
bool XTSock::SetSndTimeOut(SOCKET sock, int nTimeOut)
{
	//ÉèÖÃ³¬Ê±
	int nNetTimeout = nTimeOut;
	int error = ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&nNetTimeout, sizeof(int));
	if (0 != error)
	{
		return false;
	}

	return true;
}
bool XTSock::SetRcvTimeOut(SOCKET sock, int nTimeOut)
{
	//ÉèÖÃ³¬Ê±
	int nNetTimeout = nTimeOut;
	int error = ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int));
	if (0 != error)
	{
		return false;
	}

	return true;
}

//ÉèÖÃ×èÈû/·Ç×èÈûÄ£Ê½
bool XTSock::SetBlock(SOCKET sock, unsigned long ul)
{
	int nRet;
#ifdef _OS_WINDOWS
	nRet = ::ioctlsocket(sock, FIONBIO, (unsigned long*)&ul);
#else
	if(ul)
	{
		nRet=fcntl(sock,F_GETFL, 0);
		nRet=fcntl(sock,F_SETFL, nRet&~O_NONBLOCK);
	}
	else
	   nRet=fcntl(sock,F_SETFL, O_NONBLOCK);
#endif
	if(nRet == SOCKET_ERROR)
	{
		return false;
	}

	return true;
}

// ÕìÌý
SOCKET XTSock::Listen(const char *ip, unsigned short port)
{
	SOCKET sockListen = ::socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sockListen)
	{
		return INVALID_SOCKET;
	}

	SOCKADDR_IN addr_in;
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = ::htons(port);
#ifdef _OS_WINDOWS
	addr_in.sin_addr.S_un.S_addr = ::inet_addr(ip);
#else
	in_addr_t addr=inet_addr(ip);
	addr_in.sin_addr = *((struct in_addr*)&addr);
#endif
	int nRet = ::bind(sockListen, (sockaddr*)&addr_in, sizeof(addr_in));	
	if (SOCKET_ERROR == nRet)
	{
		CloseSocket(sockListen);
		return INVALID_SOCKET;
	}

	nRet = ::listen(sockListen, SOMAXCONN);
	if (SOCKET_ERROR == nRet)
	{
		CloseSocket(sockListen);
		return INVALID_SOCKET;
	}

	return sockListen;
}

// ½ÓÊÜÁ¬½Ó
SOCKET XTSock::Accept(SOCKET sockListen, SOCKADDR_IN &addr_from)
{
#ifdef _OS_WINDOWS
	int nLen = sizeof(SOCKADDR_IN);
#else
	socklen_t nLen = sizeof(SOCKADDR_IN);
#endif
	SOCKET sockAccept = INVALID_SOCKET;

	if (INVALID_SOCKET == sockListen)
	{
		return INVALID_SOCKET;
	}

#ifdef _OS_WINDOWS
	_try
	{
		sockAccept = ::WSAAccept(sockListen, (sockaddr*)&addr_from, &nLen, NULL, 0);
#else
		sockAccept = ::accept(sockListen, (sockaddr*)&addr_from, &nLen);
#endif
#ifdef _OS_WINDOWS
	}
	_except(EXCEPTION_EXECUTE_HANDLER)
	{
		CloseSocket(sockAccept);
#else
		close(sockAccept);
#endif
		return INVALID_SOCKET;
#ifdef _OS_WINDOWS
	}	
#endif
	return sockAccept;
}

// Ö÷¶¯Á¬½Ó
SOCKET XTSock::Connect(const char *szServerIP, USHORT nServerPort, int nTimeOut)
{
	SOCKET sockConnect = ::socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sockConnect)
	{
		return INVALID_SOCKET;
	}

	//ÉèÖÃ·Ç×èÈûÄ£Ê½
	if (!SetBlock(sockConnect,1))
	{
		CloseSocket(sockConnect);
		return INVALID_SOCKET;
	}

	//Á¬½Ó
	SOCKADDR_IN addr_to;
	addr_to.sin_family = AF_INET;
	addr_to.sin_port = ::htons(nServerPort);

	
#ifdef _OS_WINDOWS
	addr_to.sin_addr.S_un.S_addr = ::inet_addr(szServerIP);
#else
	in_addr_t addr=inet_addr(szServerIP);
	addr_to.sin_addr = *((struct in_addr*)&addr);
#endif
	

	int nRet = ::connect(sockConnect, (sockaddr*)&addr_to, sizeof(addr_to));
	if ( nRet != 0 )
	{
#ifdef _OS_WINDOWS
		if (::GetLastError() == WSAEWOULDBLOCK)
#else
		if (errno == EWOULDBLOCK)
#endif
		{
			CloseSocket(sockConnect);
			return INVALID_SOCKET;
		}

		//selectÄ£ÐÍÉèÖÃ³¬Ê±
		timeval timeout;
		fd_set r;
		FD_ZERO(&r);
		FD_SET(sockConnect, &r);
		timeout.tv_sec = nTimeOut;
		timeout.tv_usec = 0;
		nRet = select(0, 0, &r, 0, &timeout);
		if ( nRet <= 0 )
		{
			CloseSocket(sockConnect);
			return INVALID_SOCKET;
		}
	}

	//ÉèÖÃ×èÈûÄ£Ê½
	if (!SetBlock(sockConnect,0))
	{
		CloseSocket(sockConnect);
		return INVALID_SOCKET;
	}

	return sockConnect;
}
bool XTSock::Connect(SOCKET sockConnect, const char *szServerIP, USHORT nServerPort, int nTimeOut)
{
	//ÉèÖÃ·Ç×èÈûÄ£Ê½
	if (!SetBlock(sockConnect,1))
	{
		return false;
	}

	//Á¬½Ó
	SOCKADDR_IN addr_to;
	addr_to.sin_family = AF_INET;
	addr_to.sin_port = ::htons(nServerPort);
#ifdef _OS_WINDOWS
	addr_to.sin_addr.S_un.S_addr = ::inet_addr(szServerIP);
#else
	in_addr_t addr=inet_addr(szServerIP);
	addr_to.sin_addr = *((struct in_addr*)&addr);
#endif
	int nRet = ::connect(sockConnect, (sockaddr*)&addr_to, sizeof(addr_to));
	if ( nRet != 0 )
	{
	#ifdef _OS_WINDOWS
		if (::GetLastError() == WSAEWOULDBLOCK)
#else
		if (errno == EWOULDBLOCK)
#endif
		{
			return false;
		}

		//selectÄ£ÐÍÉèÖÃ³¬Ê±
		timeval timeout;
		fd_set r;
		FD_ZERO(&r);
		FD_SET(sockConnect, &r);
		timeout.tv_sec = nTimeOut;
		timeout.tv_usec = 0;
		nRet = select(0, 0, &r, 0, &timeout);
		if ( nRet <= 0 )
		{
			return false;
		}
	}

	//ÉèÖÃ×èÈûÄ£Ê½
	if (!SetBlock(sockConnect,0))
	{
		return false;
	}

	return true;
}

// ·¢ËÍÊý¾Ý
bool XTSock::SendData(SOCKET sock, char *pData, UINT nLen, int nTimeOut/* = -1*/)
{
	bool bRet = true;
	if (!pData)
	{
		return false;
	}

	//ÉèÖÃ³¬Ê±
	int nOltNetTimeOut = 0;
	if (nTimeOut >= 0)
	{
	#ifdef _OS_WINDOWS
	 int nSize = sizeof(int);
#else
	 socklen_t nSize = sizeof(int);
#endif
	
		int error = ::getsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&nOltNetTimeOut, &nSize);
		if (0 != error)
		{
			return false;
		}

		int nNetTimeout = nTimeOut;
		error = ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&nNetTimeout, sizeof(int));
		if (0 != error)
		{
			return false;
		}
	}	

	UINT nByteSent = 0;
	while (nLen > nByteSent)
	{
		int nSend = ::send(sock, pData+nByteSent, nLen-nByteSent, 0);
		if(nSend > 0)
		{
			nByteSent += nSend;
		}
		else
		{
			bRet = false;
			break;
		}
	}

	//ÉèÖÃ³¬Ê±
	if (nTimeOut >= 0)
	{
		::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&nOltNetTimeOut, sizeof(int));
	}

	return bRet;
}
bool XTSock::SendData2(SOCKET sock, char *pData, UINT nLen)
{
	bool bRet = true;
	if (!pData)
	{
		return false;
	}

	UINT nByteSent = 0;
	while (nLen > nByteSent)
	{
		int nSend = ::send(sock, pData+nByteSent, nLen-nByteSent, 0);
		if(nSend > 0)
		{
			nByteSent += nSend;
		}
#ifdef _OS_WINDOWS
		else if (::GetLastError() == WSAEWOULDBLOCK)
#else
		else if (errno == EWOULDBLOCK)
#endif
		{
#ifdef _OS_WINDOWS
		    ::Sleep(50);
#else
		    usleep(50*1000);
#endif
			continue;
		}
		else
		{
			bRet = false;
			break;
		}
	}

	return bRet;
}

// ½ÓÊÕÊý¾Ý
bool XTSock::RecvData(SOCKET sock, char *pData, UINT nLen, int nTimeOut/* = -1*/)
{
	bool bRet = true;
	if (!pData)
	{
		return false;
	}

	//ÉèÖÃ³¬Ê±
	int nOltNetTimeOut = 0;
	if (nTimeOut >= 0)
	{
		
#ifdef _OS_WINDOWS
		int nSize = sizeof(int);
#else
		socklen_t nSize = sizeof(int);
#endif
		int error = ::getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nOltNetTimeOut, &nSize);
		if (0 != error)
		{
			return false;
		}

		int nNetTimeout = nTimeOut;
		error = ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int));
		if (0 != error)
		{
			return false;
		}
	}	

	UINT nByteRecved = 0;
	while (nLen > nByteRecved)
	{
		int nRecv = ::recv(sock, pData+nByteRecved, nLen-nByteRecved, 0);
		if(nRecv > 0)
		{
			nByteRecved += nRecv;
		}
		else
		{
			bRet = false;
			break;
		}
	}

	if (nTimeOut >= 0)
	{
		::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nOltNetTimeOut, sizeof(int));
	}

	return bRet;
}
int XTSock::RecvData2(SOCKET sock, char *pData, UINT nLen, int nTimeOut/* = -1*/)
{
	bool bRet = true;
	if (!pData)
	{
		return false;
	}

	//ÉèÖÃ³¬Ê±
	int nOltNetTimeOut = 0;
	if (nTimeOut >= 0)
	{
		
#ifdef _OS_WINDOWS
		int nSize = sizeof(int);
#else
		socklen_t nSize = sizeof(int);
#endif
		int error = ::getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nOltNetTimeOut, &nSize);
		if (0 != error)
		{
			return false;
		}

		int nNetTimeout = nTimeOut;
		error = ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int));
		if (0 != error)
		{
			return false;
		}
	}	

	int nByteRecved = ::recv(sock, pData, nLen, 0);

	if (nTimeOut >= 0)
	{
		::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nOltNetTimeOut, sizeof(int));
	}

	return nByteRecved;
}

void XTSock::SetKeepLive(SOCKET sock)
{
#ifdef _OS_WINDOWS
	tcp_keepalive live, liveout;
	memset(&live, 0, sizeof(live));

	live.onoff = 1;
	live.keepalivetime = 5000;
	live.keepaliveinterval = 6000;

	int nlive = sizeof(tcp_keepalive);
	::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&live, nlive);
	
	DWORD dw;
	::WSAIoctl(sock, SIO_KEEPALIVE_VALS, &live, sizeof(live), &liveout, sizeof(liveout), &dw, NULL, NULL);
#else
	int keepalive = 1; // 开启keepalive属性
	int keepidle = 5; // 如该连接在60秒内没有任何数据往来,则进行探测
	int keepinterval = 6; // 探测时发包的时间间隔为5 秒
	int keepcount = 1; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
	setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
	setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
	setsockopt(sock, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));
#endif
}
