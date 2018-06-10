#ifndef _DL_LIBRYRY_LOAD_H__INCLUDE_
#define _DL_LIBRYRY_LOAD_H__INCLUDE_
#pragma once

#ifdef WIN32
#include <shlwapi.h>
#if (_MSC_VER > 1000)
#pragma comment (lib, "shlwapi")
#endif
#else
#include <string.h>
#include <dlfcn.h>
#include "xt_config.h"

#define WINAPI

#endif//////

#define MAX_LOAD_NUM 16

typedef struct _SDllLoad
{
    HINSTANCE m_hApiHandl[MAX_LOAD_NUM];
    int m_nHandleID;
}SDllLoad;

typedef BOOL (WINAPI *dgSetDllDirectory)(LPCTSTR lpPathName);

#ifdef __cplusplus
extern "C" {
#endif

int DllLoadInit(SDllLoad *pInput);
int LoadOneFile(SDllLoad *pInput, LPCSTR szPath, LPCSTR szDllName);
void ReleaseLoad(SDllLoad *pInput);

const char* GetFileDirectoryName(const char* szModuleFile);
const char* GetFileDirectoryModule(HMODULE hModule);

HINSTANCE LoadDll(LPCSTR szDirectory, LPCSTR szDllName, LPCSTR szDllNameEx[], unsigned nExSize);
int FreeDll(HINSTANCE hInstance);

#ifdef __cplusplus
};
#endif

#ifdef __cplusplus
class CDllLoad : public _SDllLoad
{
public:
    CDllLoad();
    ~CDllLoad();

    int LoadFile(LPCSTR szPath, LPCSTR szDllName);
    void ReleaseAll();
};
#endif

#endif//_DL_LIBRYRY_LOAD_H__INCLUDE_
