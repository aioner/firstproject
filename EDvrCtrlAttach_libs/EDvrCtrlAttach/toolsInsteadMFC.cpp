
#include "toolsInsteadMFC.h"





//**************** 函 数 实 现 *******************************//

//获取配置文件的整型值
int GetPrivateProfileInt(const char* section, const char* key, int defaultValue, const char* filePath)
{
	if (section==NULL || key==NULL || filePath==NULL)
	{
		return defaultValue;
	}
	
	char sect[260]={0};
	if (section[0]!='[')
	{
		sprintf(sect, "[%s]", section);
	}
	else
	{
		sprintf(sect, "%s", section);
	}
	
	FILE *fl = NULL;
	int trueValue;
	char *buffer=new char[260];
	size_t nbuf=0;
	
	fl = fopen(filePath,"r");
	if (fl == NULL)
	{
		delete[] buffer;
		return defaultValue;
	}
	
	while (getline((char**)&buffer, (size_t*)&nbuf, fl) != -1)
	{
		if (strstr(buffer,sect) != NULL)
		{
			while (getline((char**)&buffer, (size_t*)&nbuf, fl) != -1 && buffer[0] !='[')
			{
				char* pos=NULL;
				if ((pos = strstr(buffer, key)) != NULL && (*(pos+strlen(key))==' '||*(pos+strlen(key))=='='))
				{
					//截取子串
					char *p = pos+strlen(key);
					pos = strstr(p,"=");
					if (pos != NULL)
					{
						p = pos +1;
						
						//去前面的空格
						while(*p ==' ')
						{
							p = p+1;
						}
						
						//去换行符
						int len = strlen(p);
						while((*(p+len-1)=='\r' || *(p+len-1)=='\n')&&len>0)
						{
							*(p+len-1)='\0';
							len = len -1;
						}		
						trueValue = atoi(p);
						fclose(fl);	
						delete[] buffer;
						
						return trueValue;		
					}
					else
					{
						fclose(fl);	
						delete[] buffer;
						
						return defaultValue;		
					}			
				}
			}
		}
	}
	
	
	fclose(fl);
	delete[] buffer;
	
	return defaultValue;
}



//获取配置文件的字符值
int GetPrivateProfileString(const char* section, const char* key, const char* defaultValue, char*returnBuf, int bufLen, const char* filePath)
{
	if (section==NULL || key==NULL || filePath==NULL || returnBuf==NULL || bufLen<=0)
	{
		return 0;
	}
	
	char sect[260]={0};
	if (section[0]!='[')
	{
		sprintf(sect, "[%s]", section);
	}
	else
	{
		sprintf(sect, "%s", section);
	}
	
	FILE *fl = NULL;
	int trueValue;
	char *buffer=new char[260];
	size_t nbuf=0;
	
	fl = fopen(filePath,"r");
	if (fl == NULL)
	{
		delete[] buffer;
		strncpy(returnBuf, defaultValue, bufLen);					
		return strlen(defaultValue);	
	}
	
	while (getline((char**)&buffer, (size_t*)&nbuf, fl) != -1)
	{
		if (strstr(buffer,sect) != NULL)
		{
			while (getline((char**)&buffer, (size_t*)&nbuf, fl) != -1 && buffer[0] !='[')
			{
				char* pos=NULL;
				if ((pos = strstr(buffer, key)) != NULL&& (*(pos+strlen(key))==' '||*(pos+strlen(key))=='='))
				{
					//截取子串
					char *p = pos+strlen(key);
					pos = strstr(p,"=");
					if (pos != NULL)
					{
						p = pos +1;
						
						//去前面的空格
						while(*p ==' ')
						{
							p = p+1;
						}
						
						//去换行符
						int len = strlen(p);
						while((*(p+len-1)=='\r' || *(p+len-1)=='\n' || *(p+len-1)==' ')&&len>0)
						{
							*(p+len-1)='\0';
							len = len -1;
						}
						
						int cpyLen = strlen(p)<bufLen?strlen(p):bufLen;	
						strncpy(returnBuf, p, cpyLen);
						fclose(fl);	
						delete[] buffer;
						
						return strlen(p);		
					}
					else
					{
						fclose(fl);	
						delete[] buffer;
						
						strncpy(returnBuf, defaultValue, bufLen);					
						return strlen(defaultValue);		
					}			
				}
			}
		}
	}
	
	
	fclose(fl);
	delete[] buffer;
	
	strncpy(returnBuf, defaultValue, bufLen);
	return strlen(defaultValue);	
}

//写入配置文件的字符型值
bool WritePrivateProfileString(const char* section, const char* key, const char*value, const char* filePath)
{
	
	if (section==NULL || key==NULL || value==NULL || filePath==NULL)
	{
		return false;
	}
	
	char sect[260]={0};
	if (section[0]!='[')
	{
		sprintf(sect, "[%s]", section);
	}
	else
	{
		sprintf(sect, "%s", section);
	}

	FILE *fl = NULL;
	char *buffer = new char[260];
	int nbuf = 0;
	int fileLen = 0;

	vector<char*> arrFileLines;
	fl = fopen(filePath,"a+");
	if (fl != NULL)
	{			
		//把配置文件都读出来，放在内存中，方便后面查找
		while (getline((char**)&buffer, (size_t*)&nbuf, fl) != -1)
		{
			char *pos = buffer;
			while(pos != NULL && (*pos != '\r' || *pos != '\n') && *pos == ' ')
			{
				pos = pos + 1;
			}
			arrFileLines.push_back(pos);
			buffer = new char[260];
		}
		fclose(fl);
		delete[] buffer;
	

		//添加或改写配置项
		bool isExistSect = false;
		for (int i=0; i<arrFileLines.size();i++)
		{
			if (strstr(arrFileLines[i], sect) != NULL)
			{
				isExistSect= true;
				for (int j=i+1; j<arrFileLines.size();j++)
				{
					if (strstr(arrFileLines[j],"[") != NULL)
					{
						buffer = new char[260];
						sprintf(buffer, "%s=%s\n", key, value);
						arrFileLines.insert(arrFileLines.begin()+i, buffer);
						break;
					}
				
					char* pos=NULL;
					if ((pos=strstr(arrFileLines[j],key)) != NULL && (*(pos+strlen(key))==' '||*(pos+strlen(key))=='='))
					{
						sprintf(arrFileLines[j], "%s=%s\n", key, value);
						break;
					}
				}
			}
		
			if (isExistSect)  {break;}
		}
	
		if (isExistSect == false)
		{
			buffer = new char[260];
			sprintf(buffer, "%s\n", sect);
			arrFileLines.push_back(buffer);
		
			buffer = new char[260];
			sprintf(buffer, "%s=%s\n", key, value);
			arrFileLines.push_back(buffer);
		}	
	}
	else
	{
		buffer = new char[260];
		sprintf(buffer, "%s\n", sect);
		arrFileLines.push_back(buffer);
		
		buffer = new char[260];
		sprintf(buffer, "%s=%s\n", key, value);
		arrFileLines.push_back(buffer);
	}
	
	
	
	//重写文件，并删除内存数据
	fl = fopen(filePath,"w");
	for (int i=0; i<arrFileLines.size();i++)
	{
		fwrite(arrFileLines[i], strlen(arrFileLines[i]), sizeof(char), fl);
		delete[] arrFileLines[i];
	}
	arrFileLines.clear();
	fclose(fl);
	
	return true;
}

//获取当前文件夹的路径
int GetCurrentDirectory(int bufLen, char* buf)
{
	getcwd(buf, bufLen);
	
	return strlen(buf);
}

//获取文件夹中的所有文件，包括子文件夹中的文件
int GetFilesInFolder(const char* folderPath, vector<const char*> &filepathList)
{
	struct dirent *struDir;
	DIR *dir = opendir(folderPath);
	char *buffer=NULL;
	if(dir == NULL)
	{
		return -1;
	}
	
	while((struDir = readdir(dir)) != NULL)
	{
		if (strcmp(struDir->d_name, ".")==0 || strcmp(struDir->d_name, "..")==0)
		{
			continue;
		}
			
		//printf("file: %s/%s\r\n", folderPath, struDir->d_name);
		else if (struDir->d_type == 8) //file
		{
			buffer = new char[300];
			sprintf(buffer, "%s/%s", folderPath, struDir->d_name);
			filepathList.push_back(buffer);
		}
		else if (struDir->d_type == 10) //link file
		{
		}
		else if (struDir->d_type == 4)  //folder
		{
			char pathName[260]={0};
			sprintf(pathName, "%s/%s", folderPath, struDir->d_name);
			GetFilesInFolder(pathName, filepathList);
		}
		
		/*//显示特定类型文件
		char *pos = NULL;
		pos = strstr(struDir->d_name, ".ini");
		if (pos != NULL)
		{
			if ((pos - struDir->d_name) == strlen(struDir->d_name)-strlen(".ini"))
			{
				printf("ini file: %s\r\n", struDir->d_name);
			}
		}*/
	}
	
	closedir(dir);
	
	return 0;
}



