// WriteLog.h: interface for the CWriteLog class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _WRITE_LOG_H__INCLUDED_
#define _WRITE_LOG_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#ifndef WIN32
#include "xt_config.h"
//#define BOOL int
//typedef char* va_list;
#endif
//////////////////////////////////////////////////////////////////////////
void MakeDirectory(const char* szDirPath);
void SetLogPath(const char* szDirectory);
void EnableLog(int nLogLevel);
//写日志
//szFlag in 日至文件标志
//szLogFmt in 格式化字符串
void WriteLogV(int nLogLevel, const char* szFlag, int nFlag, const char* szLogFmt, va_list vArgList);
void WriteLog(const char* szFlag, const char* szLogFmt, ...);
void WriteLog1(int nLogLevel, const char* szFlag, const char* szLogFmt, ...);
void WriteLog2(const char* szFlag, int nFlag, const char* szLogFmt, ...);
void WriteLog3(int nLogLevel, const char* szFlag, int nFlag, const char* szLogFmt, ...);

void* LogOpen(const char* szFlag, int nFlag);
BOOL LogClose(void* hLog);
BOOL LogCloseFree(void* hLog, int bFree);
void LogWriteV(void* hLog, int nLogLevel, const char* szLogFmt, va_list vArgList);
void LogWrite(void* hLog, const char* szLogFmt, ...);

//void __enable_console(int bEnable);
//void __printf(const char* format,...);

#ifndef TRACE
void TRACE(const char* szLogFmt, ...);
#endif//TRACE

#ifdef __cplusplus
}
#endif

#endif//_WRITE_LOG_H__INCLUDED_
