#ifndef _XT_CONFIG_H
#define _XT_CONFIG_H
#ifndef _WIN32
typedef unsigned int		UINT;
typedef unsigned short		UINT16;
typedef unsigned int		uint32_t;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;
typedef long				LONG;
typedef unsigned char		UCHAR;
typedef unsigned char		uint8_t;
typedef unsigned int		UINT32;
typedef int					BOOL;
typedef unsigned char		BYTE;
typedef void*				PVOID;
typedef unsigned short		uint16_t;
typedef void*				LPVOID;
typedef void*				HANDLE;
typedef struct sockaddr_in  SOCKADDR_IN;
typedef struct sockaddr     SOCKADDR;
typedef unsigned short		USHORT;
typedef struct hostent		hostent;
typedef const char*			LPCSTR;
typedef const char*			LPCTSTR;
typedef void*				HINSTANCE;
typedef unsigned int		UINT_PTR;
typedef void*				HMODULE;
typedef unsigned int		UINT_PTR;
typedef int					SOCKET;
typedef BYTE*				PBYTE;
typedef int					INT_PTR;
typedef char				WIN32_FIND_DATAA;

#define BOOL int
#define FALSE 0
#define TRUE 1

#define __int8				char
#define __int16				short
#define __int32				int
#define __int64				long long
#define int64__				long long
#ifndef NULL
#define NULL				0
#endif
#define CALLBACK            
#define INVALID_SOCKET      -1
#define SOCKET_ERROR		-1
#define HWND				void*
#define MAX_PATH			256
#define INVALID_HANDLE_VALUE NULL

#define _snprintf			snprintf
#define _vsnprintf_s snprintf
#define __stdcall           
#define _try				try
#define _except				catch
#define lstrlen				strlen
#define wsprintf			sprintf
#define lstrcmp				strcmp
#define lstrcpy				strcpy
#define lstrcpyn			strncpy
#define Sleep(a) usleep(a*1000)
//#define EXCEPTION_EXECUTE_HANDLER  ...

#define __cdecl         
typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME;

#include <time.h>
#include <sys/time.h>
inline UINT32  GetTickCount()
{
    struct timeval tv;
    gettimeofday(&tv,0);
    return tv.tv_sec*1000+tv.tv_usec/1000;
}

inline void GetLocalTime(PSYSTEMTIME lpSystemTime)
{
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    lpSystemTime->wYear = 1900+p->tm_year;
    lpSystemTime->wMonth = 1+p->tm_mon;
    lpSystemTime->wDay = p->tm_mday;
    lpSystemTime->wHour = p->tm_hour;
    lpSystemTime->wMinute = p->tm_min;
    lpSystemTime->wSecond = p->tm_sec;
    lpSystemTime->wMilliseconds = 0;
}
#endif //#ifndef _WIN32
#endif
