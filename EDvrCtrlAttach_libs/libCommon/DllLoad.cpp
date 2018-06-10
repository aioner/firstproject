#include "stdafx.h"
#include "DllLoad.h"

#ifdef __cplusplus
extern "C" {
#endif

int DllLoadInit(SDllLoad *pInput)
{
    memset(pInput, 0, sizeof(SDllLoad));
    return 0;
}
int LoadOneFile(SDllLoad *pInput, LPCSTR szPath, LPCSTR szDllName)
{
    unsigned i;
    char szDllFullName[MAX_PATH];
    //string strDllFullName(szPath);
    strcpy(szDllFullName, szPath);
    for(i=0; i<strlen(szDllFullName); i++)
    {
        if(szDllFullName[i] == '/')
            szDllFullName[i] = '\\';
    }
    PathCombine(szDllFullName, szDllFullName, szDllName);
    pInput->m_hApiHandl[pInput->m_nHandleID] = LoadLibrary(szDllFullName);
    if(pInput->m_hApiHandl[pInput->m_nHandleID] == NULL)
    {
        return -1;
    }
    else
    {
        pInput->m_nHandleID++;
        return 0;
    }
}

void ReleaseLoad(SDllLoad *pInput)
{
    for(;pInput->m_nHandleID > 0; pInput->m_nHandleID--)
    {
        FreeLibrary(pInput->m_hApiHandl[pInput->m_nHandleID]);
    }
    DllLoadInit(pInput);
}

const char* GetFileDirectoryName(const char* szModuleFile)
{
    HINSTANCE hAppIns = GetModuleHandle(szModuleFile);
    return GetFileDirectoryModule(hAppIns);
}

const char* GetFileDirectoryModule(HMODULE hModule)
{
    static char strAppFilePath[MAX_PATH]={0};
    char strInstancePath[MAX_PATH]={0};
    unsigned i=0, j=0;
    GetModuleFileName(hModule,strInstancePath,MAX_PATH);
    while(strlen(strInstancePath)>i){
        j=i;
        for(i++; i<(strlen(strInstancePath)); i++)
        {
            if('\\' == strInstancePath[i])
                break;
        }
    }
    strncpy(strAppFilePath, strInstancePath, j);
    return strAppFilePath;
}

HINSTANCE LoadDll(LPCSTR szDirectory, LPCSTR szDllName, LPCSTR szDllNameEx[], unsigned nExSize)
{
    dgSetDllDirectory fpSetDllDirectory = NULL;
    unsigned i;
    HINSTANCE hInstance=NULL;
    HINSTANCE hKernel32=NULL;
    SDllLoad dllLoad;
    char strFilePath[MAX_PATH]={0}, szAppPath[MAX_PATH]={0};

    GetCurrentDirectory(MAX_PATH,szAppPath);

    PathCombine(strFilePath, szDirectory, szDllName);
    SetCurrentDirectory(szDirectory);

#if (_MSC_VER < 1200)
    //SetDllDirectory 为WinxpSp1以后公布的接口，设置动态库搜索路径
    hKernel32 = GetModuleHandle("Kernel32.dll");
    assert(hKernel32!=NULL);
    fpSetDllDirectory = (dgSetDllDirectory)GetProcAddress(hKernel32, "SetDllDirectoryA");
    if(fpSetDllDirectory)
        fpSetDllDirectory(szDirectory);
#else
    SetDllDirectoryA(szDirectory);
#endif

    for(i=0; i<nExSize; i++)
    {
        LoadOneFile(&dllLoad, szDirectory, szDllNameEx[i]);
    }
    hInstance = LoadLibrary(strFilePath);
    SetCurrentDirectory(szAppPath);

    return hInstance;
}

int FreeDll(HINSTANCE hInstance)
{
    return FreeLibrary(hInstance);
}

#ifdef __cplusplus
}
#endif

CDllLoad::CDllLoad()
{
    DllLoadInit(this);
}

CDllLoad::~CDllLoad()
{
    ReleaseLoad(this);
}

int CDllLoad::LoadFile(LPCSTR szPath, LPCSTR szDllName)
{
    return LoadOneFile(this, szPath, szDllName);
}

void CDllLoad::ReleaseAll()
{
    ReleaseLoad(this);
}
