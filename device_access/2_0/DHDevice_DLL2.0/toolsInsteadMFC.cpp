#include "toolsInsteadMFC.h"


#ifdef _WIN32

#else
//**************** �� �� ʵ �� *******************************//

//��ȡ�����ļ�������ֵ
int GetPrivateProfileInt(const char* section, const char* key, int defaultValue, const char* filePath)
{
	if (section==NULL || key==NULL || filePath==NULL)
	{
		return defaultValue;
	}

	//ȥkey����Ŀո�
	char *pTempSz = new char[strlen(key)+1];
	strncpy(pTempSz, key, strlen(key));
	pTempSz[strlen(key)] = '\0';
	
	for(int i=strlen(pTempSz)-1; i>=0; i--)
	{
		if(pTempSz[i] == ' ')
		{
			pTempSz[i] = '\0';
		}
		else
		{
			break;
		}
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
		delete pTempSz;
		return defaultValue;
	}
	
	while (getline((char**)&buffer, (size_t*)&nbuf, fl) != -1)
	{
		if (strstr(buffer,sect) != NULL)
		{
			while (getline((char**)&buffer, (size_t*)&nbuf, fl) != -1 && buffer[0] !='[')
			{
				char* pos=NULL;
				if ((pos = strstr(buffer, pTempSz)) != NULL && (*(pos+strlen(pTempSz))==' '||*(pos+strlen(pTempSz))=='='))
				{
					//��ȡ�Ӵ�
					char *p = pos+strlen(pTempSz);
					pos = strstr(p,"=");
					if (pos != NULL)
					{
						p = pos +1;
						
						//ȥǰ��Ŀո�
						while(*p ==' ')
						{
							p = p+1;
						}
						
						//ȥ���з�
						int len = strlen(p);
						while((*(p+len-1)=='\r' || *(p+len-1)=='\n')&&len>0)
						{
							*(p+len-1)='\0';
							len = len -1;
						}		
						trueValue = atoi(p);
						fclose(fl);	
						delete[] buffer;
						delete pTempSz;
						
						return trueValue;		
					}
					else
					{
						fclose(fl);	
						delete[] buffer;
						delete pTempSz;
						
						return defaultValue;		
					}			
				}
			}
		}
	}
	
	
	fclose(fl);
	delete[] buffer;
	delete pTempSz;
	
	return defaultValue;
}



//��ȡ�����ļ����ַ�ֵ
int GetPrivateProfileString(const char* section, const char* key, const char* defaultValue, char*returnBuf, int bufLen, const char* filePath)
{
	if (section==NULL || key==NULL || filePath==NULL || returnBuf==NULL || bufLen<=0)
	{
		return 0;
	}
	
	//ȥkey����Ŀո�
	char *pTempSz = new char[strlen(key)+1];
	strncpy(pTempSz, key, strlen(key));
	pTempSz[strlen(key)] = '\0';
	
	for(int i=strlen(pTempSz)-1; i>=0; i--)
	{
		if(pTempSz[i] == ' ')
		{
			pTempSz[i] = '\0';
		}
		else
		{
			break;
		}	
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
	char *buffer=new char[260];
	size_t nbuf=0;
	
	fl = fopen(filePath,"r");
	if (fl == NULL)
	{
		delete[] buffer;
		delete pTempSz;
		
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
				if ((pos = strstr(buffer, pTempSz)) != NULL&& (*(pos+strlen(pTempSz))==' '||*(pos+strlen(pTempSz))=='='))
				{
					//��ȡ�Ӵ�
					char *p = pos+strlen(pTempSz);
					pos = strstr(p,"=");
					if (pos != NULL)
					{
						p = pos +1;
						
						//ȥǰ��Ŀո�
						while(*p ==' ')
						{
							p = p+1;
						}
						
						//ȥ���з�
						int len = strlen(p);
						while((*(p+len-1)=='\r' || *(p+len-1)=='\n' || *(p+len-1)==' ')&&len>0)
						{
							*(p+len-1)='\0';
							len = len -1;
						}
						
						int cpyLen = strlen(p)<(unsigned int)bufLen?strlen(p):(unsigned int)bufLen;	
						strncpy(returnBuf, p, cpyLen);
						fclose(fl);	
						
						delete pTempSz;
						delete[] buffer;
						
						return strlen(p);		
					}
					else
					{
						fclose(fl);	
						delete[] buffer;
						delete pTempSz;
						
						strncpy(returnBuf, defaultValue, bufLen);					
						return strlen(defaultValue);		
					}			
				}
			}
		}
	}
	
	
	fclose(fl);
	delete[] buffer;
	delete pTempSz;
	
	strncpy(returnBuf, defaultValue, bufLen);
	return strlen(defaultValue);	
}

//д�������ļ����ַ���ֵ
bool WritePrivateProfileString(const char* section, const char* key, const char*value, const char* filePath)
{
	
	if (section==NULL || key==NULL || value==NULL || filePath==NULL)
	{
		return false;
	}
	
	//ȥkey����Ŀո�
	char *pTempSz = new char[strlen(key)+1];
	strncpy(pTempSz, key, strlen(key));
	pTempSz[strlen(key)] = '\0';
	
	for(int i=strlen(pTempSz)-1; i>=0; i--)
	{
		if(pTempSz[i] == ' ')
		{
			pTempSz[i] = '\0';
		}
		else
		{
			break;
		}
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

	vector<char*> arrFileLines;
	fl = fopen(filePath,"a+");
	if (fl != NULL)
	{			
		//�������ļ����������������ڴ��У�����������
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
	

		//��ӻ��д������
		bool isExistSect = false;
		for (unsigned int i=0; i<arrFileLines.size();i++)
		{
			if (strstr(arrFileLines[i], sect) != NULL)
			{
				isExistSect= true;
				for (unsigned int j=i+1; j<arrFileLines.size();j++)
				{
					if (strstr(arrFileLines[j],"[") != NULL)
					{
						buffer = new char[260];
						sprintf(buffer, "%s=%s\n", pTempSz, value);
						arrFileLines.insert(arrFileLines.begin()+i, buffer);
						break;
					}
				
					char* pos=NULL;
					if ((pos=strstr(arrFileLines[j],pTempSz)) != NULL && (*(pos+strlen(pTempSz))==' '||*(pos+strlen(pTempSz))=='='))
					{
						sprintf(arrFileLines[j], "%s=%s\n", pTempSz, value);
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
			sprintf(buffer, "%s=%s\n", pTempSz, value);
			arrFileLines.push_back(buffer);
		}	
	}
	else
	{
		buffer = new char[260];
		sprintf(buffer, "%s\n", sect);
		arrFileLines.push_back(buffer);
		
		buffer = new char[260];
		sprintf(buffer, "%s=%s\n", pTempSz, value);
		arrFileLines.push_back(buffer);
	}
	
	
	
	//��д�ļ�����ɾ���ڴ�����
	fl = fopen(filePath,"w");
	for (unsigned int i=0; i<arrFileLines.size();i++)
	{
		fwrite(arrFileLines[i], strlen(arrFileLines[i]), sizeof(char), fl);
		delete[] arrFileLines[i];
	}
	arrFileLines.clear();
	fclose(fl);
	
	delete pTempSz;
	
	return true;
}

//��ȡ��ǰ�ļ��е�·��
int GetCurrentDirectory(int bufLen, char* buf)
{
	getcwd(buf, bufLen);
	
	return strlen(buf);
}

//��ȡ�ļ����е������ļ����������ļ����е��ļ�
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
		
		/*//��ʾ�ض������ļ�
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
#endif


