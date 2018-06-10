#ifndef _CENTER_COMMON_TYPES_H_INCLUDED
#define _CENTER_COMMON_TYPES_H_INCLUDED

//与中心相关的一些定义 原来放在MDef.h中

//中心指令
enum CenterServerCtrlType
{
    CT_STOPDB		= 0,        // 停止点播
    CT_STARTDB		= 1,        // 开启点播
    CT_LOGIN		= 2,        // 设备登陆
    CT_STOPALLDB	= 10,       // 停止所有点播
    CT_UPDATEIDS	= 20,       // IDS更新
};

enum CenterServerOperationType
{
    OP_PLAY = 0x1,              //开始点播操作
    OP_STOP = 0x2,              //停止点播操作
    OP_INIT_SERVER = 0x3,       //初始化服务器操作
    OP_LOGIN = 0x4              //登录到EDVR上
};

enum CenterServerErrorCode
{
    ERR_ALREADY_PLAY = 0xe01,           //已经点播
    ERR_SERVER_NO_ANSWER = 0xe02,       //服务器未响应
    ERR_NO_SERVER_ID = 0xe03,           //没有可以分配的服务器通道号
    ERR_NO_PLAY_INFO = 0xe04,           //当前点播信息不存在
    ERR_PLAY_FAIL = 0xe05,              //点播失败
    ERR_NOT_LOGIN = 0xe06,              //未登录到EDVR上
    ERR_E_DVR = 0xe07,                  //EDVR设备出错
    ERR_SERVER_IP = 0xe08,              //服务器的IP出错
    ERR_SERVER_NUM = 0xe09,             //服务器的通道数出错
    ERR_MUL_SERVER_IP = 0xe0a,          //服务器组播IP出错
    ERR_CHECK_LINK_HANDLE = 0xe0b,      //检查链接句柄
    ERR_LOGIN_ERROR = 0xe0c,            //登录失败
    ERR_ALREADY_CAPTURE = 0xe0d,        //已经开启了数据捕获
    ERR_WAIT_RDATA = 0xe0e,             //连接建立后未获取任何数据
    ERR_OVERTIME = 0xe0f,               //等待数据超时
    ERR_DEVICE_NOT_FOUND = 0xe10,       //未找到此设备
    ERR_WAIT_RDATA_CLOSE = 0xe11,       //连接建立后未获取任何数据,然后进行清除连接操作

    //成功信息
    SUCCESS_PLAY_OK = 0xa01,                //成功信息数量
    SUCCESS_LOGIN_OK = 0xa02,               //登录成功
    SUCCESS_ALREADY_LINK = 0xa03,           //已经开启了链接
    SUCCESS_STOP_OK = 0xa04,                 //停点成功
};

#endif //_CENTER_COMMON_TYPES_H_INCLUDED
