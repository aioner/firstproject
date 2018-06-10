
#ifndef _SERVER_
#define _SERVER_
using namespace std;
typedef struct ClientInfo
{
	SOCKET fd;
	sockaddr_in addr;
	char ip[20];
}ClientInfo;    

typedef void (* ReceiveProcess)(const ClientInfo& userinfo,const char* receivetext);

void servefunc(SOCKET *param);

#define BUFFER_SIZE (8*1024)

class Server
{
private:
	char *m_buffer;
	SOCKET fdListen;
	fd_set fdSocket;
	vector<ClientInfo> vtinfo;
	void add_client(ClientInfo info);
	void del_client(SOCKET fd);
	void get_client(SOCKET fd,ClientInfo &info);
	SOCKET get_maxsock();
	int get_numsock();
	ReceiveProcess receivefunc;
public:
	Server()
	{
		m_buffer=(char *)malloc(BUFFER_SIZE);
	}
	~Server()
	{
		free(m_buffer);
	}
public:
	void setprocessfunc(ReceiveProcess func)
	{
		receivefunc=func;
	}
	void init(int PORT=80)
	{
#ifdef _WIN32
		WSADATA     wsaData;
		WSAStartup(0x0202, &wsaData);
#endif
		struct sockaddr_in svr_addr;
		memset(&svr_addr,0, sizeof(svr_addr));
		svr_addr.sin_family = AF_INET;
		svr_addr.sin_port = htons(PORT); 
		svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		fdListen=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (bind(fdListen, (struct sockaddr *)&svr_addr, sizeof(svr_addr))==-1)
		{
			printf("bind error,please check the port\n");
			exit(1);
		}
		listen(fdListen, SOMAXCONN);
		printf("\n>>websvr ip:%s port:%d\n",inet_ntoa(svr_addr.sin_addr),PORT);
		FD_ZERO(&fdSocket);
		FD_SET(fdListen,&fdSocket);

		vtinfo.clear();
		ClientInfo info;
		info.fd = fdListen;
		info.addr = svr_addr;

		add_client(info);
#ifdef _WIN32
		_beginthread(serverfunc,0,this);
#else
		pthread_t  pth;
		pthread_create(&pth, NULL, &serverfunc, this);
#endif
	}
protected:
#ifdef _WIN32
	static void serverfunc(void *param);
#else
	static void* serverfunc(void *param);
#endif

};

int my_recv(int fd,void *buffer,int length)
{
	int bytes_left = 0;
	int bytes_read = 0;
	char *ptr = NULL;

	ptr = (char *)buffer;
	bytes_left = length;
	while (bytes_left > 0)
	{
		int num = 0;
		if (bytes_left > 1024)
		{
			num = 1024;
		}
		else
		{
			num = bytes_left;
		}
		bytes_read = recv(fd,ptr,num,0);
		if (bytes_read<0)
		{
			if (errno==EINTR)
			{
				bytes_read = 0;
			}
			else
			{
				return (-1);
			}
		}
		else if (bytes_read == 0)
		{
			break;
		}
		bytes_left -= bytes_read;
		ptr += bytes_read;
	}

	return (length-bytes_left);
}

int my_send(int fd, void *buffer, int length)
{
	int bytes_left = 0;
	int written_bytes = 0;
	char *ptr = NULL;

	ptr = (char *)buffer;
	bytes_left = length;

	while (bytes_left > 0)
	{
		int num = 0;
		if (bytes_left > 1024)
		{
			num = 1024;
		}
		else
		{
			num = bytes_left;
		}
		written_bytes = send(fd,ptr,num,0);
		if (written_bytes <= 0)
		{
			if (errno == EINTR)
			{
				written_bytes = 0;
			}
			else
			{
				return -1;
			}
		}
		bytes_left -= written_bytes;
		ptr += written_bytes;
	}
	return length-bytes_left;
}

#ifdef _WIN32
void Server::serverfunc(void *param)
#else
void* Server::serverfunc(void *param)
#endif
{
	Server *pThis = (Server *)param;
	while(1)
	{
		//linux select will change it after called
		struct timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;

		fd_set fdRead = pThis->fdSocket;
		fd_set fdWrite = pThis->fdSocket;
		fd_set fdError = pThis->fdSocket;

		int fd_num = pThis->vtinfo.size();
		int maxsock = pThis->get_maxsock();
		int nRet = select(maxsock+1, &fdRead, /*&fdWrite*/ NULL, &fdError,/*&tv*/ NULL);
		//printf("select ret:%d %d %d\n",nRet,tv.tv_sec,tv.tv_usec);
		if (nRet > 0)
		{
			for (int i=0;i<fd_num;i++)
			{
				if (FD_ISSET(pThis->vtinfo[i].fd,&fdRead))
				{
					SOCKET fdtemp = pThis->vtinfo[i].fd;
					if (fdtemp == pThis->fdListen)
					{
						sockaddr_in addrRemote={0};
#ifdef _WIN32
						int nAddrLen = sizeof (addrRemote);
#else
						unsigned int nAddrLen = sizeof (addrRemote);
#endif

						SOCKET fdclient = ::accept(pThis->fdListen,(sockaddr *)&addrRemote,&nAddrLen);
						if (fd_num < FD_SETSIZE)//FD_SETSIZE=64
						{
							ClientInfo info;
							info.fd = fdclient;
							info.addr = addrRemote;
							pThis->add_client(info);

							int timeout=1000;//send timeout
							setsockopt(fdclient,SOL_SOCKET,SO_SNDTIMEO,(char *)&timeout,sizeof(int));
							FD_SET(fdclient,&(pThis->fdSocket));

							printf("cnt:%d fd:%u ip:%s port:%u\n",pThis->get_numsock(),fdclient,
								inet_ntoa(addrRemote.sin_addr),ntohs(addrRemote.sin_port));
						}
						else
						{
							printf("max connections arrive,bye it\n");
							send(fdtemp,"bye",4,0);
							shutdown(fdtemp,0x01);
#ifdef _WIN32
							closesocket(fdtemp);
#else
							close(fdtemp);
#endif
						}
					}
					else
					{
						unsigned long block = 1;
						unsigned long size=0;
#ifdef _WIN32
						ioctlsocket(fdtemp,FIONBIO,&block);
						ioctlsocket(fdtemp,FIONREAD,&size);
#else
						ioctl(fdtemp,FIONBIO,&block);
						ioctl(fdtemp,FIONREAD,&size);
#endif
						if(size > 0)
						{
							//char *buffer=(char *)malloc(size+3);
							memset(pThis->m_buffer,0x0,BUFFER_SIZE);
							int ret = my_recv(fdtemp,pThis->m_buffer,size);
							if( ret > 0)
							{
								ClientInfo info;
								pThis->get_client(fdtemp,info);

								strcpy(info.ip,inet_ntoa(info.addr.sin_addr));
								pThis->receivefunc(info,pThis->m_buffer);
							}
							//printf("\n===========buffer size:%d===========\n%s",ret,pThis->m_buffer);
							//free(buffer);
						}
						else
						{
							FD_CLR(fdtemp,&(pThis->fdSocket));
							shutdown(fdtemp,0x01);
#ifdef _WIN32
							closesocket(fdtemp);
#else
							close(fdtemp);
#endif
							pThis->del_client(fdtemp);
							--fd_num;//@warn bugs
						}
					}
				}
			}
			for (int i=0;i<fd_num;i++)
			{
			}
			for (int i=0;i<fd_num;i++)
			{
			}
		}
		else if (nRet < 0)
		{
			printf("select ret error:%d\n",nRet);
		}
	}
}

void Server::add_client(ClientInfo info)
{
	vtinfo.push_back(info);
}

void Server::del_client(SOCKET fd)
{
	vector<ClientInfo>::iterator itr;
	itr = vtinfo.begin();
	for (;itr!=vtinfo.end();++itr)
	{
		if (itr->fd == fd)
		{
			vtinfo.erase(itr);
			break;
		}
	}
}

void Server::get_client(SOCKET fd,ClientInfo &info)
{
	vector<ClientInfo>::iterator itr;
	itr = vtinfo.begin();
	for (;itr!=vtinfo.end();++itr)
	{
		if (itr->fd == fd)
		{
			info = *itr;
			break;
		}
	}
}

SOCKET Server::get_maxsock()
{
	SOCKET maxsock = fdListen;
	vector<ClientInfo>::iterator itr;
	itr = vtinfo.begin();
	for (;itr!=vtinfo.end();++itr)
	{
		if (itr->fd > maxsock)
		{
			maxsock = itr->fd;
		}
	}

	return maxsock;
}

int Server::get_numsock()
{
	return vtinfo.size();
}
#endif



