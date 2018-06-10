#ifndef JK_MAIN_CLIENT_H__
#define  JK_MAIN_CLIENT_H__

#include "jk_dll.h"
#include <iostream>

#define Center_TYPE     0x10            //核心交换机(视频交换机)
#define EM_Server_TYPE  0x02            //嵌入式硬盘录像机

#ifndef MAX_PATH
#define MAX_PATH          260
#endif//#ifndef MAX_PATH


class JkMainClient
{
    // 构造
public:
    JkMainClient();	// 标准构造函数 

    bool LinkCenter();
    void ExitCenter();
    void init_jk();
    void uninit_jk();

    //是否启动交换
    bool const is_start_router() const
    {
        return auto_start_router_;
    }

public:	  
    virtual void OnLinkServerJkEvent(long sNum, long bz)=0;
    virtual void OnUserInOutJkEvent(char* sIDS, char* sName, long sType, char* sIPS, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2)=0;
    virtual void OnDBImageCenterJkEvent(char* sIDS, long sCH, long successbz, char* fIDS, char* fIPS, long DBMode, long localVPort, long localAPort, long destVPort, long destAPort, long iRes1, char* sRes1)=0;
    virtual void OnGetMsgJkEvent( char* sSrcIDS, char* sData, long nDataLen, long nOrderbz)=0;
    virtual void OnTellLocalIDSJkEvent(char* LocalIDS, char* sRes1, long iRes1)=0;
    virtual void OnTransparentCommandJkEvent(char* fIDS, char* sCommands)=0;

public:
    // 通知本机的IDS 
    static void __stdcall OnTellLocalIDS(void* pUser, char* LocalIDS, char* sRes1, long iRes1);

    // 通知本机的IDS  
    static void __stdcall OnLinkServer(void* pUser, long sNum, long bz);

    // 链路连接成功 
    static void __stdcall OnUserInOut(void* pUser, char* sIDS, char* sName, long sType, char* sIPS, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2);

    //专线保障
    static void __stdcall OnEventGetMsg(void* pUser, char* sSrcIDS, char* sData,long nDataLen, long nOrderbz);

    //通知服务器，本机的某个图像被点播 
    static void __stdcall OnDBImageCenter(void* pUser, char* sIDS, long sCH, long successbz, char* fIDS, char* fIPS, long DBMode, long localVPort,long localAPort, long destVPort, long destAPort, long iRes1, char* sRes1);

    //透传指令回调
    static void __stdcall OnTransparentCommandCB(void* pUser, char* fIDS, char* sCommands);


private:
    void load_cfg();
public:
    jk_dll m_jk; 
    long center_link_type_;
    long centerid_;
    long center_port_;
    long server_num_;
    std::string center_ip_;
    std::string local_name_;
    std::string local_ip_;
    bool auto_start_router_;
};	   		   	   		  
#endif //#ifndef JK_MAIN_CLIENT_H__
