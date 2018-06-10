#pragma once

#define BUF_SEND	128*1024
#define BUF_RECV	128*1024
#define TIMEOUT_SEND 3000
#define TIMEOUT_RECV 3000
#define TINEOUT_CONNECT	5

#define MAX_MSG_SIZE 2048	//最大消息长度
#define SIZE_PACKHEAD 12	//包头大小

// 包类型
enum XT_PACKTYPE
{
	PT_IDS,
};

#pragma pack(push)//字节对齐
#pragma pack(1) 
//////////////////////////////////////////////////////////////////////////

// 包头
struct XT_MSG 
{
	XT_PACKTYPE type;		//包类型
	unsigned	size;		//包头大小
	unsigned	lpayload;	//负载长度
};

//////////////////////////////////////////////////////////////////////////
#pragma pack(pop)//恢复对齐状态
