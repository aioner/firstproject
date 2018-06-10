#ifndef TOOLS_INSTEAD_MFC_H
#define TOOLS_INSTEAD_MFC_H


#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <vector>

using namespace std;

#ifdef _WIN32

#else
#include <unistd.h>
#include <dirent.h>
//**************** �� �� �� �� ******************************//


//��ȡ�����ļ�������ֵ
int GetPrivateProfileInt(const char* section, const char* key, int defaultValue, const char* filePath);

//��ȡ�����ļ����ַ���ֵ
int GetPrivateProfileString(const char* section, const char* key, const char* defaultValue, char*returnBuf, int bufLen, const char* filePath);

//д�������ļ����ַ���ֵ
bool WritePrivateProfileString(const char* section, const char* key, const char*value, const char* filePath);

//��ȡ��ǰ�ļ��е�·��
int GetCurrentDirectory(int bufLen, char* buf);

//��ȡ�ļ����е������ļ��ľ���·�����������ļ����е��ļ�
//filepathList�����������ļ��ľ���·�������е�Ԫ�������������Ҫ�ֶ��ͷš�	
int GetFilesInFolder(const char* folderPath, vector<const char*> &filepathList);
/*	�÷�������
	vector<const char*> arrList;
	GetFilesInFolder("/home/liujin/Workspace/Test3", arrList);
	
	for (int i=0; i<arrList.size();i++)
	{
		printf("%s\n", arrList[i]);
		delete[] arrList[i];   //һ��Ҫ�ֶ��ͷţ������ڴ�й©
	}
*/
#endif

#endif//#ifndef TOOLS_INSTEAD_MFC_H