// WriteLog.cpp: implementation of the CWriteLog class.
//
//////////////////////////////////////////////////////////////////////
#ifndef WIN32
#include <stdarg.h>
#include <assert.h>
#else
#include <direct.h>
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include "WriteLog.h"
#include "SyncLock.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction

char g_szLogDir[128] =
#ifndef WIN32
"/var/log/xtlog";
#else
"D:/XTlog";
#endif
int g_nLogLevel = 0;

//////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

void __inline TRACE_V(const char* szLogFmt, va_list vArgList)
{
    static char szMsg[512];
    static time_t timer;
    static struct tm *tblock;

    //获取当前时间
    timer = time(NULL);
    tblock = localtime(&timer);

    memset(szMsg, 0, sizeof(char)*256);
    sprintf(szMsg, "%02d:%02d:%02d ", tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
    vsprintf(szMsg+9, szLogFmt, vArgList);

#ifndef WIN32
    fprintf(stderr, szMsg);
    puts("");
#else
    OutputDebugString(szMsg);
    OutputDebugString("\n");
#endif
}

void TRACE(const char* szLogFmt, ...)
{
#ifdef _DEBUG
    va_list v;
    va_start(v, szLogFmt);
    TRACE_V(szLogFmt, v);
    va_end(v);
#endif
}

void MakeDirectory(const char* szDirPath)
{
    size_t i;
    char szTmpDir[256] = {0};
    char szMidDir[256] = {0};
    size_t nDirLen = strlen( szDirPath );
    strcpy(szTmpDir, szDirPath);
    if (szTmpDir[nDirLen] != '\\' && szTmpDir[nDirLen] != '/')
    {
        szTmpDir[nDirLen] = '/';
        szTmpDir[nDirLen+1] = 0;
        nDirLen++;
    }
    for (i=0; i<nDirLen; i++)
    {
        if (szTmpDir[i] == '\\' || szTmpDir[i] == '/')
        {
            if (szTmpDir[i-1] == ':')
                continue;
            strncpy(szMidDir, szTmpDir, i);
		if (szMidDir[0] == 0)
		    continue;
            assert(szMidDir[0]!=0);
#ifndef WIN32
            mkdir(szMidDir, 0777);
#else
            _mkdir(szMidDir);
#endif
        }
    }
}

void SetLogPath(const char* szPath)
{
    strcpy(g_szLogDir, szPath);
}

void EnableLog(int nLogLevel)
{
	g_nLogLevel = nLogLevel;

	MakeDirectory(g_szLogDir);
}

void WriteLogV(int nLogLevel, const char* szFlag, int nFlag, const char* szLogFmt, va_list vArgList)
{
	FILE * pFile = NULL;
	char szDir[256] = {0};
	char szFile[256] = {0};
	time_t timer;
	struct tm *tblock;

    if(!strcmp(szFlag, "TRACE"))
    {
        TRACE_V(szLogFmt, vArgList);
        return;
    }

	if (nLogLevel > g_nLogLevel)
		return;

	//获取当前时间
	timer = time(NULL);
	tblock = localtime(&timer);
    if(nFlag == -1)
    {
        sprintf(szFile,"%s/%s_%02d%02d%02d.log", g_szLogDir, szFlag,
            (tblock->tm_year+1900)%100,tblock->tm_mon+1,tblock->tm_mday);
    }
    else
    {
	    sprintf(szFile,"%s/%s%d_%02d%02d%02d.log", g_szLogDir, szFlag, nFlag,
		    (tblock->tm_year+1900)%100,tblock->tm_mon+1,tblock->tm_mday);
    }

	//开启文件
	pFile = fopen(szFile,  "a");
	if (pFile == NULL)
		return;

	//记录时间戳
	fprintf(pFile, "%02d:%02d:%02d ", tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
	//写入数据
	vfprintf(pFile, szLogFmt, vArgList);
	fprintf(pFile, "\n");

	fclose(pFile);
}

void WriteLog(const char* szFlag, const char* szLogFmt, ...)
{
	//写入数据
	va_list argptr;
	va_start(argptr, szLogFmt);
	WriteLogV(1, szFlag, -1, szLogFmt, argptr);
	va_end(argptr);
}

void WriteLog1(int nLogLevel, const char* szFlag, const char* szLogFmt, ...)
{
	//写入数据
	va_list argptr;
	va_start(argptr, szLogFmt);
	WriteLogV(nLogLevel, szFlag, -1, szLogFmt, argptr);
	va_end(argptr);
}

void WriteLog2(const char* szFlag, int nFlag, const char* szLogFmt, ...)
{
	//写入数据
	va_list argptr;
	va_start(argptr, szLogFmt);
	WriteLogV(1, szFlag, nFlag, szLogFmt, argptr);
	va_end(argptr);
}

void WriteLog3(int nLogLevel, const char* szFlag, int nFlag, const char* szLogFmt, ...)
{
	//写入数据
	va_list argptr;
	va_start(argptr, szLogFmt);
	WriteLogV(nLogLevel, szFlag, nFlag, szLogFmt, argptr);
	va_end(argptr);
}

typedef struct _SLogInfo
{
    LOCK_HANDLE hWLock;
    FILE* pFile;
    time_t timer;
    int nFlag;
    char szFlag[32];
}SLogInfo, *PSLogInfo;

void* LogOpen(const char* szFlag, int nFlag)
{
    char szFile[256] = {0};
    FILE * pFile = NULL;
    time_t timer;
    struct tm *tblock;
    PSLogInfo pLog;
    
    //获取当前时间
    timer = time(NULL);
    tblock = localtime(&timer);
    sprintf(szFile,"%s/%s%d_%02d%02d%02d_%02d.log", g_szLogDir, szFlag, nFlag,
        tblock->tm_year%100,tblock->tm_mon+1,tblock->tm_mday,
        tblock->tm_hour);
    
    //开启文件
    pFile = fopen(szFile,  "a");
    if (pFile == NULL)
        return NULL;

    //建立操作句柄
    pLog = (PSLogInfo)malloc(sizeof(SLogInfo));
    assert(pLog!=NULL);
    SyncLock_Create(&pLog->hWLock);
    pLog->timer = timer;
    pLog->pFile = pFile;
    pLog->nFlag = nFlag;
    strcpy(pLog->szFlag, szFlag);
    
    return pLog;
}

BOOL LogClose(void* hLog)
{
    return LogCloseFree(hLog, TRUE);
}

BOOL LogCloseFree(void* hLog, int bFree)
{
    SLogInfo* pLog = (SLogInfo*)hLog;
    BOOL nRet = FALSE;
    if(!hLog)
        return nRet;
    
    if(SyncLock_Lock(&pLog->hWLock, -1))
    {
        if(pLog->pFile)
        {
            fclose(pLog->pFile);
            pLog->pFile = NULL;
            nRet = TRUE;
        }
        else
            nRet = FALSE;
        SyncLock_Unlock(&pLog->hWLock);
        SyncLock_Destroy(&pLog->hWLock);
        if(bFree)
            free(pLog);
    }
    else
        nRet = FALSE;
    return nRet;
}

void LogWriteV(void* hLog, int nLogLevel, const char* szLogFmt, va_list vArgList)
{
    SLogInfo* pLog = (SLogInfo*)hLog;
    time_t timer;
    struct tm *tblock;
    if (nLogLevel<g_nLogLevel || !pLog) return;
    
    //获取当前时间
    timer = time(NULL);

    if((timer/3600) != (pLog->timer/3600))
    {
        if(LogCloseFree(hLog, FALSE))
            return;
        hLog = LogOpen(pLog->szFlag, pLog->nFlag);
        memcpy(pLog, hLog, sizeof(SLogInfo));
        free(hLog);
    }
    
    //记录时间戳
    tblock = localtime(&timer);
    fprintf(pLog->pFile, "%02d:%02d:%02d ", tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
    
    //写入数据
    vfprintf(pLog->pFile, szLogFmt, vArgList);
    fprintf(pLog->pFile, "\n");
}

void LogWrite(void* hLog, const char* szLogFmt, ...)
{
	va_list argptr;
	va_start(argptr, szLogFmt);
	LogWriteV(hLog, 1, szLogFmt, argptr);
	va_end(argptr);
}

BOOL bUsedConsole = FALSE;
void __enable_console(int bEnable)
{
    bUsedConsole = bEnable;
    if(bUsedConsole)
    {
        //AllocConsole();
    }
    else
    {
        //FreeConsole();
    }
}
/*
void __printf(const char* format,...)
{
    if(!bUsedConsole) return;
    //HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if(INVALID_HANDLE_VALUE != hConsole)
    {
        char buf[1024];
        int nTobeWritten;
        DWORD nIsWritten = 0;

        va_list	argl;
        va_start(argl, format); 
        nTobeWritten = _vsnprintf(buf,sizeof(buf),format, argl);
        va_end(argl);
        buf[nTobeWritten]='\n';
        buf[nTobeWritten+1]=0;

        WriteConsoleA(hConsole,buf,nTobeWritten+1,&nIsWritten,NULL);
    }
}
*/
#ifdef __cplusplus
}
#endif
