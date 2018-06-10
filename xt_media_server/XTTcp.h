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

    // 初始化
    int init(unsigned long num_chan,		//服务通道数 
        std::string ip,					// ip	
        unsigned short server_port);	//服务端口

    // 反初始化
    int uninit();

    // 数据发送
    int send_data(unsigned long chanid,			// 通道号
        char *buff,					// 发送数据
        unsigned long len,				// 数据长度
        int frame_type,				// 帧类型
        long device_type);				// 设备类型

    // 转发链路数
    int get_link_num();

    //获取
    int get_tcp_trans_info(std::vector<connect_info_t>& vecInfo);

private:
    XT_Server		*m_pServer;		// TCP传输单元
    unsigned short	m_serverPort;	// 发送端口
    unsigned long	m_numChan;		// 通道数

    utility::shared_mutex			m_mutex;		//mutex
};
#endif //#ifdef USE_TCP_SERVER_
#endif//XTTCP_H__INCLUDE_
