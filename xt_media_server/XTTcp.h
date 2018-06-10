#ifndef XTTCP_H__INCLUDE_
#define XTTCP_H__INCLUDE_
#include "common.h"

#ifdef _OS_WINDOWS
//#define USE_TCP_SERVER_
#endif

#ifdef USE_TCP_SERVER_
#include <string>
#include "H_VideoServer.h"
#include "RunInfoMgr.h"
#include "utility\mutex.h"

using namespace std;

class XTTcp
{
private:
    XTTcp(void);
    ~XTTcp(void);

    static XTTcp self;

public:
    static XTTcp* instance(){return &self;}

    // ��ʼ��
    int init(unsigned long num_chan,		//����ͨ���� 
        std::string ip,					// ip	
        unsigned short server_port);	//����˿�

    // ����ʼ��
    int uninit();

    // ���ݷ���
    int send_data(unsigned long chanid,			// ͨ����
        char *buff,					// ��������
        unsigned long len,				// ���ݳ���
        int frame_type,				// ֡����
        long device_type);				// �豸����

    // ת����·��
    int get_link_num();

    //��ȡ
    int get_tcp_trans_info(std::vector<connect_info_t>& vecInfo);

private:
    XT_Server		*m_pServer;		// TCP���䵥Ԫ
    unsigned short	m_serverPort;	// ���Ͷ˿�
    unsigned long	m_numChan;		// ͨ����

    utility::shared_mutex			m_mutex;		//mutex
};
#endif //#ifdef USE_TCP_SERVER_
#endif//XTTCP_H__INCLUDE_
