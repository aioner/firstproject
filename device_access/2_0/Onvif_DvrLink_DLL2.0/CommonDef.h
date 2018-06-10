#ifndef COMMONDEF_H__
#define COMMONDEF_H__
#include "soapH.h"
#include "wsseapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib,"libeay32.lib")
#pragma comment(lib,"ssleay32.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include "xt_config.h"
#endif//#ifdef _WIN32

#define DEFAULT_USER    ("admin")
#define DEFAULT_PWD     ("12345")
#define DEFAULT_CTRL_PORT (8557)

#define MAX_LOAD_NUM 16

// typedef struct _SOV_LinkAPI
// {
// 	HINSTANCE m_hInstance;
// }SOV_LinkAPI;

typedef struct _SDllLoad
{
	HINSTANCE m_hApiHandl[MAX_LOAD_NUM];
	int m_nHandleID;
}SDllLoad;

const char c_szFlag[] = "Onvif_DvrLink_DLL2.0";

///基本操作的全局函数
//int LoadPlugAPI();
int SoapInit();
void SoapUninit();
char* Onvif_MakeDeviceUrl(soap *pSoap, char* szAddress, char* szUrlBuffer);
char* Onvif_MakeChannelUrl(soap *pSoap, char* szDeviceUrl, char* szAddress, int nChannelID, int nMediaType, char* szUrlBuffer);

void EnableLog(int nLogLevel);
//void MakeDirectory(const char* szDirPath);

void WriteLogV(int nLogLevel, const char* szFlag, int nFlag, const char* szLogFmt, va_list vArgList);
void WriteLog(const char* szFlag, const char* szLogFmt, ...);
#endif//#ifndef COMMONDEF_H__
