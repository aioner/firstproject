#pragma once

#define BUF_SEND	128*1024
#define BUF_RECV	128*1024
#define TIMEOUT_SEND 3000
#define TIMEOUT_RECV 3000
#define TINEOUT_CONNECT	5

#define MAX_MSG_SIZE 2048	//�����Ϣ����
#define SIZE_PACKHEAD 12	//��ͷ��С

// ������
enum XT_PACKTYPE
{
	PT_IDS,
};

#pragma pack(push)//�ֽڶ���
#pragma pack(1) 
//////////////////////////////////////////////////////////////////////////

// ��ͷ
struct XT_MSG 
{
	XT_PACKTYPE type;		//������
	unsigned	size;		//��ͷ��С
	unsigned	lpayload;	//���س���
};

//////////////////////////////////////////////////////////////////////////
#pragma pack(pop)//�ָ�����״̬
