
#ifndef _REQUEST_
#define _REQUEST_
using namespace std;
class Request
{
public:
	string str;
	string header;
	string content;
	string method;
	Request(){str=header=content=method="";}
public:
	Request(const char *buff)
	{
		str=buff;
		if(str.find("HTTP/1.1")!=-1)
		{
			int a,b;
			a=str.find("\r\n\r\n");
			b=str.find("\n\r\n\r");
			//printf("\n__a:%d b:%d___\n",a,b);
			int part=(a==-1)?b:a;
			if(part==-1)return;
			//printf("\n__part:%d___\n",part);
			header=str.substr(0,part);

			//printf("\n__repart:%d___\n",str.size()-part);
			method=header.substr(0,header.find(" "));
			if("POST"==method)
				content=str.substr(part+4,str.size()-part-4);
		}
		else
		{
			content=str;
		}

	}
	char *getitem(char *itemname)
	{
		return NULL;
	}
	string getrequest(int flag=0)
	{
		string con="";
		if(method=="GET"||method=="POST")
		{
			int l=header.find(" ");
			int r=header.find(" ",l+1);
			if(l==-1||r==-1)return 0;
			if(flag==0)
				con=header.substr(l+1,r-l-1);
			else
				con=header.substr(l+2,r-l-2);

		}
		return con;
	}
	string getpost()
	{
		if(method=="POST")
			return content; 
		else return NULL;    
	}    
};

#endif


