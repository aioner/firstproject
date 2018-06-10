
#ifndef _RESPONSE_
#define _RESPONSE_
using namespace std;
class ResponseHeader
{
public:
	string content; 
	int con_len;
	int isdownload;
	char filename[200];
	char type[100]; 

	void setcontentsize(int a)
	{
		con_len=a;
	}
	ResponseHeader(int asize)
	{
		strcpy(type,"text/html");  
		con_len= asize;
	}
	ResponseHeader(){isdownload=0;}
	void setsize(int asize)
	{
		content="";
		con_len= asize;   
		strcpy(type,"text/html");
	}
	void settype(char *atype)
	{
		strcpy(type,atype);  
	}
	void setdownload()
	{
		isdownload=1;  
	}
	void setfile(const char *fname,int size)
	{
		strcpy(filename,fname);
	}
	void prepareheader()
	{
		char header[900];
		char buffer[200];
		strcpy(header,"HTTP/1.1 200 OK\r\n");
		strcat(header,"Server: JIN\r\n");
		strcat(header,"Date: Tue, 19 Nov 2015 09:33:20 GMT\r\n");

		sprintf(buffer,"Content-Length: %d\r\n",con_len);
		strcat(header,buffer);

		sprintf(buffer,"Location: http://%s\r\n","127.0.0.1");
		strcat(header,buffer);
		if(isdownload==1)
		{
			sprintf(buffer,"Content-Disposition: form-data, name=\"file\" filename=\"%s\"\r\n",filename);
			strcat(header,buffer);
		}
		strcat(header,"Connection: Keep-Alive\r\n");
		sprintf(buffer,"Content-Type: %s\r\n\r\n",type);
		strcat(header,buffer);
		content+=header;
	} 
};
#endif

