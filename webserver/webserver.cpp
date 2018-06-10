// webserver.cpp : 定义控制台应用程序的入口点。
//

#include<iostream>
#include <errno.h>
#include <string>
#include <fstream>
#include <vector>
#ifdef _WIN32
#include <process.h>
#include <winsock2.h>
#include <mswsock.h>
#include <MSTcpIP.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#else
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <sys/ioctl.h>
#define SOCKET unsigned int
#endif

#include "server.h"
#include "request.h"
#include "response.h"

#include "process_dps.h"

#include "webconfig.h"

char *g_buff = NULL;
#ifdef _WIN32
HANDLE g_hEvent = NULL;
#else
sem_t g_sem;
#endif

void output()
{
}

void processfunc(const ClientInfo &info,const char *context)
{
	Request req(context);
	ResponseHeader httpheader;

	if(req.content!="")
	{
		string sendbuff;
		int contextsize = 0;

		memset(g_buff,0x0,BUFFER_SIZE);
		contextsize = req.content.size();
		strcpy(g_buff,req.content.c_str());

		//printf(g_buff);

		int errcode = 0;
		//读取时直接读取xml配置文件，到webpage进行解析
		if (strcmp(g_buff,"restart")==0)
		{
#ifdef _WIN32
			SetEvent(g_hEvent);
#else
			system("killall DPS");
			sem_post(&g_sem);
#endif
			errcode = 1;//process_dps::instance()->restart_process();
			if (errcode < 0)
			{
				sendbuff.assign("-1");
			}
			else
			{
				sendbuff.assign("0");
			}
		}
		else if (strcmp(g_buff,"get_dps_cfg")==0)
		{
			memset(g_buff,0x0,BUFFER_SIZE);
			errcode = webconfig::Instance()->get_dps_cfg(g_buff,BUFFER_SIZE);
			if (errcode < 0)
			{
				sendbuff.assign("-1");
			}
			else
			{
				//printf(g_buff);
				sendbuff.assign(g_buff);
			}
		}
		//写入时根据节点写入，不采用覆盖文件方式
		else if (contextsize > 1500)
		{
			errcode = webconfig::Instance()->set_dps_cfg(g_buff,1);
			if (errcode < 0)
			{
				sendbuff.assign("-1");
			}
			else
			{
				sendbuff.assign("0");
			}
		}

		const char *utf8= sendbuff.c_str();
		httpheader.setsize(strlen(utf8));
		httpheader.settype("text/xml;charset=UTF-8");

		httpheader.prepareheader();
		my_send(info.fd,(void *)httpheader.content.c_str(),httpheader.content.size());
		//printf("req:%s\n",utf8);
		my_send(info.fd,(void *)utf8,strlen(utf8));
	}
	else if(req.method=="GET")
	{
		string reqfile = "./web/"+req.getrequest(1);
		if (reqfile.size() <  strlen("./web/left.html")) //left.html
		{
			reqfile = "./web/index.html";
		}
		/*ifstream file(reqfile.c_str(),ios::in|ios::binary);
		istreambuf_iterator<char> beg(file),end; 
		string strfile = string(beg,end);
		const char *buff = strfile.c_str();
		int len = strfile.size();*/
		FILE *pfile;
		pfile = fopen(reqfile.c_str(),"rb");
		if (!pfile)
		{
			return;
		}
		fseek(pfile,0,SEEK_END);
		int len = ftell(pfile);
		rewind(pfile);
		
		httpheader.setfile(req.getrequest(1).c_str(),0);
		httpheader.setsize(len);
		/*if (req.getrequest(1) == "img/bg.jpg")
		{
			httpheader.settype("image/jpeg");
			httpheader.setdownload();
		}
		else
		{
			httpheader.settype("text/html");
		}*/
		httpheader.prepareheader();
		unsigned long block = 0;
#ifdef _WIN32
		ioctlsocket(info.fd,FIONBIO,&block);
#else
		ioctl(info.fd,FIONBIO,&block);
#endif
		my_send(info.fd, (void *)httpheader.content.c_str(),httpheader.content.size());
		
		int left_bytes = len;
		char buff[1028]={0};
		while (!feof(pfile))
		{
			/*if (!fgets(buff,512, pfile))
			{
				break;
			}*/
			//memset(buff,0,514);
			
			int read_bytes = fread(buff, 1024,1,pfile);
			int num = 0;
			if (left_bytes - 1024 < 0)
			{
				num = left_bytes;
			}
			else
			{
				num = 1024;
			}
			int send_bytes = my_send(info.fd,(void *)buff,num);
			if (send_bytes > 0)
			{
				left_bytes -= send_bytes;
			}
			else
			{
				break;
			}
		}
		fclose(pfile);
	}
}

int main(int argc, char* argv[])
{
	g_buff = (char *)malloc(BUFFER_SIZE);

	Server websvr;
	websvr.setprocessfunc(processfunc);
	websvr.init(8753);
#ifdef _WIN32
	process_dps::instance()->init("DPS.exe");
	g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	while (1)
	{
		(WaitForSingleObject(g_hEvent, INFINITE) == WAIT_FAILED);
		int errcode = process_dps::instance()->restart_process();
	}
#else
	sem_init(&g_sem,0, 0);
	process_dps::instance()->init("DPS");
	while (1)
	{
		sem_wait(&g_sem);
		system("./DPS");
	}
#endif

#ifdef _WIN32
	CloseHandle(g_hEvent);
#else
	sem_destroy(&g_sem);
#endif
	free(g_buff);
	return 0;
}

