
#include "process_dps.h"

#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

process_dps process_dps::self;

process_dps::process_dps(void)
{
#ifdef _WIN32
	exec_fpath = "dps_server.exe";
#else
	exec_fpath = "./dps_server";
#endif
}

process_dps::~process_dps(void)
{
}

int process_dps::init(std::string fpath)
{
	if (fpath.empty())
	{
		return -1;
	}
	exec_fpath = fpath;
#ifdef _WIN32
	int pos  = fpath.rfind('\\');
#else
	int pos  = fpath.rfind('/');
#endif
	if (pos < 0)
	{
		exec_fname = fpath;
	}
	else
	{
		exec_fname = fpath.substr(pos+1);
	}
	
	printf("\n>>fname: %s\n>>fpath: %s\n",exec_fname.c_str(),exec_fpath.c_str());

	return 0;
}

int process_dps::restart_process()
{
	int errcode =0;
#ifdef _WIN32
	errcode = terminate_process(exec_fname);
	errcode = startup_process(exec_fpath.c_str());
#else
	char buff[512]={0};
	sprintf(buff,"killall %s",exec_fname.c_str());
	system(buff);

	sprintf(buff,"./%s",exec_fname.c_str());
	system(buff);
#endif
	return errcode;
}


#ifdef _WIN32
int process_dps::startup_process(const char * exe_path)
{
	STARTUPINFO si={sizeof(si)};

	SECURITY_ATTRIBUTES saProcess,saThread;
	PROCESS_INFORMATION piProcess;
	TCHAR szPath[MAX_PATH];

	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = TRUE;

	saProcess.nLength = sizeof(saProcess);
	saProcess.lpSecurityDescriptor = NULL;
	saProcess.bInheritHandle = TRUE;

	saThread.nLength = sizeof(saThread);
	saThread.lpSecurityDescriptor = NULL;
	saThread.bInheritHandle = FALSE;

	lstrcpy(szPath,exe_path);

	BOOL bret = CreateProcess(NULL,szPath,&saProcess,&saThread,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&piProcess);
	if (!bret)
	{
		printf("CreateProcess failed:%d\n",GetLastError());
		return -1;
	}

	return 0;
}



int process_dps::terminate_process(std::string exe_name)
{
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hProcessShot;
	hProcessShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if (hProcessShot == INVALID_HANDLE_VALUE)
	{
		puts("get process list failed");
		return -1;
	}
	if (Process32First(hProcessShot,&pe32))
	{
		for (int i=0;Process32Next(hProcessShot,&pe32);i++)
		{
			if (!strcmp(exe_name.c_str(),pe32.szExeFile))
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,pe32.th32ProcessID);
				TCHAR pBuffer[MAX_PATH+1];
				::GetModuleFileNameEx(hProcess,NULL,pBuffer,MAX_PATH+1);
				printf("file name is:%s\n\n",pBuffer);

				HANDLE hprocess = ::OpenProcess(PROCESS_ALL_ACCESS,FALSE,pe32.th32ProcessID);
				if (hprocess != NULL)
				{
					::TerminateProcess(hprocess,0);
					::CloseHandle(hprocess);
				}

				TCHAR buff[1024];
				wsprintf(buff,"%d:%-20s ",i,pe32.szExeFile);
				printf(buff);
				char szStrTemp[50];
				sprintf(szStrTemp,"PID:%d",pe32.th32ProcessID);
				printf("%-20s ",szStrTemp);
				sprintf(szStrTemp,"cntThreads:%d",pe32.cntThreads);
				printf("%-20s\n",szStrTemp);
			}
		}
	}
	CloseHandle(hProcessShot);

	return 0;
}
#else
long process_dps::fork_child()
{
	long pid = fork();
	if (pid == 0)
	{
		char buff[512]={0};
		sprintf(buff,"killall %s",exec_fname.c_str());
		system(buff);
		if (exec_fpath == exec_fname)
		{
			sprintf(buff,"./%s",exec_fname.c_str());
			system(buff);
		}
		else
		{
			execlp(exec_fpath.c_str(),exec_fname.c_str());
		}
	}
	else if (pid>0)
	{
		return pid;
	}
	else
	{
		return -1;
	}
}

void process_dps::to_background()
{
	bool rc = true;

	setsid();

	int fd = open("/dev/null",O_RDWR);
	if (fd < 0)
	{
		rc = false;
	}
	if (dup2(fd,STDIN_FILENO)!=STDIN_FILENO)
	{
		rc = false;
	}
	if (dup2(fd,STDOUT_FILENO)!=STDOUT_FILENO)
	{
		rc = false;
	}
	if (dup2(fd,STDERR_FILENO)!=STDERR_FILENO)
	{
		rc = false;
	}
	if (fd > STDERR_FILENO)
	{
		close(fd);
	}
}

#endif //ifdef _WIN32