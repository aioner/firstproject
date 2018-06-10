
#ifndef TOOLS_INSTEAD_MFC_H
#define TOOLS_INSTEAD_MFC_H


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <dirent.h>

using namespace std;

//**************** 函 数 定 义 ******************************//


//获取配置文件的整型值
int GetPrivateProfileInt(const char* section, const char* key, int defaultValue, const char* filePath);

//获取配置文件的字符型值
int GetPrivateProfileString(const char* section, const char* key, const char* defaultValue, char*returnBuf, int bufLen, const char* filePath);

//写入配置文件的字符型值
bool WritePrivateProfileString(const char* section, const char* key, const char*value, const char* filePath);

//获取当前文件夹的路径
int GetCurrentDirectory(int bufLen, char* buf);

//获取文件夹中的所有文件的绝对路径，包括子文件夹中的文件
//filepathList：返回所有文件的绝对路径。其中的元数据在用完后需要手动释放。	
int GetFilesInFolder(const char* folderPath, vector<const char*> &filepathList);
/*	用法举例：
	vector<const char*> arrList;
	GetFilesInFolder("/home/liujin/Workspace/Test3", arrList);
	
	for (int i=0; i<arrList.size();i++)
	{
		printf("%s\n", arrList[i]);
		delete[] arrList[i];   //一定要手动释放，避免内存泄漏
	}
*/



#endif

