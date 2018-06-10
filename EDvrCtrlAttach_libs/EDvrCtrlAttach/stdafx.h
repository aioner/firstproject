// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once
#ifdef  _WIN32

#if (_MSC_VER >= 1400)
#pragma warning(disable : 4267)
#endif

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件:
#include <windows.h>

// C 运行时库头文件:
#include <time.h>

// TODO: 在此处引用程序需要的其他头文件
#endif //_WIN32
#include <stdio.h>
#include <stdlib.h>