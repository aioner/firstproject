#ifndef _XT_CONFIG_H
#define _XT_CONFIG_H

typedef unsigned int      UINT;
typedef unsigned short    UINT16;
typedef unsigned int      uint32_t;
typedef unsigned short    WORD;
typedef unsigned long     DWORD;
typedef long              LONG;
typedef unsigned char     UCHAR;
typedef unsigned char      uint8_t;
typedef unsigned int       UINT32;
typedef int                BOOL;
typedef unsigned char      BYTE;
typedef void*              PVOID;
typedef unsigned short     uint16_t;
typedef void*              LPVOID;
typedef void*              HANDLE;
typedef struct sockaddr_in  SOCKADDR_IN;
typedef struct sockaddr     SOCKADDR;
typedef unsigned short     USHORT;
typedef struct hostent     hostent;
typedef const char* LPCSTR;
typedef const char* LPCTSTR;
typedef void* HINSTANCE;
typedef unsigned int UINT_PTR;
typedef void* HMODULE;
typedef unsigned int UINT_PTR;

#define BOOL int
#define FALSE 0
#define TRUE 1

#define __int8             char
#define __int16            short
#define __int32            int
#define __int64            long long
#define int64__            long long
#ifndef NULL
#define NULL    0
#endif

#define HWND               void*
#define MAX_PATH           256
typedef BYTE*              PBYTE;
#define _snprintf          snprintf
#define __stdcall           
typedef int            SOCKET;
#define CALLBACK            
#define INVALID_SOCKET      -1
#define SOCKET_ERROR        -1
#define _try               try
#define _except            catch
//#define EXCEPTION_EXECUTE_HANDLER  ...

#define __cdecl         
#include <sys/time.h>
inline UINT32  GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv,0);
	return tv.tv_sec*1000+tv.tv_usec/1000;
}

#define _vsnprintf_s snprintf
#endif
