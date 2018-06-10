#ifndef _H_XTMEDIASERVER_H
#define _H_XTMEDIASERVER_H

#if (defined(WIN32) || defined(_WIN32) ||defined(WIN64) || defined(_WIN64)) && !defined(_OS_WINDOWS)
#define _OS_WINDOWS
#endif

#ifdef _OS_WINDOWS
#ifdef XTMEDIASERVER_EXPORTS
#define MEDIASERVER_API __declspec(dllexport)
#else
#define MEDIASERVER_API __declspec(dllimport)
#endif

#define MEDIASERVER_STDCALL __stdcall
#else
#define MEDIASERVER_API __attribute__((visibility("default")))
#define MEDIASERVER_STDCALL 
#endif

#include<stdint.h>
#include "xt_log_def.h"
#include "stddef.h"

#ifndef MEDIA_SERVER_IP_LEN
#define MEDIA_SERVER_IP_LEN 32
#endif//

#define MAX_TRACK 9

#ifndef MAX_STR_SIZE
#define  MAX_STR_SIZE 512
#endif//MAX_STR_SIZE

//连接信息输出
typedef struct _struct_connect_info_
{
    char m_pszCreateTime[MAX_STR_SIZE];//创建时间
    char m_pszSrcIDS[MAX_STR_SIZE];    //源IDS
    char m_pszSrcIP[MAX_STR_SIZE];     //源IP
    char m_pszDestIP[MAX_STR_SIZE];    //目标IP 
    long m_lChID;                     //服务通道
    long m_lDestPort;                 //目标接收端口
    long m_lSendPort;                  //发送端口
    unsigned short m_usProtocol;    //协议  
    unsigned int   m_uiSsrc;            //SSRC
    int srcno;
    //RR_Info
    /////////////////////////////////////
    unsigned int    m_uiFractionLost;     //丢包率
    unsigned int    m_uiCumulativeLost;   //累计丢包
    unsigned int    m_uiSequenceNumber;   //Sequence number that was received from the source specified by SSRC.
    unsigned int    m_uiJitter;           //网络抖动 
    unsigned int    m_uiLSR;       //The middle 32 bits of the NTP timestamp received. 
    unsigned int    m_uiDlSR;      //Delay since the last SR.
	unsigned int    m_urtt;        //round trip time
    /////////////////////////////////// 
    //SR_Info
    ///////////////////////////////////
    unsigned int    m_uiMNTPtimestamp;  //Most significant 32bit of NTP timestamp 
    unsigned int    m_uiLNTPtimestamp;  //Least significant 32bit of NTP timestamp
    unsigned int    m_uiTimestamp;  //RTP timestamp
    unsigned int    m_uiPackets;         //累计发包量 
    unsigned int    m_uiOctets;          //累计发送字节数
    ////////////////////////////////
    bool   m_bSendMultiplex;       // 发送复用标志
    bool           m_bDestMultiplex;       //远端接收复用标志
    unsigned int m_uiSendMultid;         // 发送端复用id
    unsigned int    m_uiDestMultid;         //远端接收复用id 
}connect_info_t,*pconnect_info_t;

/**
*@ function: RTCP媒体层强制I帧请求
*@ param[int] const int srcno 转发源标识
**/
typedef void (MEDIASERVER_STDCALL * xt_rtcp_force_iframe_cb_t)(const int srcno);


//流化控制回调函数类型
////////////////////////////////////////////////////////////////////////////////////
/**
*@function:rtsp流化控制播放回调函数
*@param[in]:int srcno  源标识
*@param[in]:int trackid  流ID
*@param[in]:long chid  通道
*@param[in]:double npt  客户端seek之后的npt时间
*@param[in]:float scale  客户端seek之后的播放倍率
*@param[out]:uint32_t *rtp_pkt_seq
*@param[out]:uint32_t *rtp_pkt_timestamp  seek之后的rtp包的时戳
**/
typedef int (MEDIASERVER_STDCALL *xt_rtsp_play_cb)(int srcno, int trackid, long chid, double npt, float scale, uint32_t *rtp_pkt_seq, uint32_t *rtp_pkt_timestamp);

/**
*@function:rtsp流化控制暂停回调函数
*@param[in]:int srcno,  源标识
*@param[in]:int trackid,  流ID
*@param[in]:long chid  通道
**/
typedef int (MEDIASERVER_STDCALL *xt_rtsp_pause_cb)(int srcno, int trackid, long chid);

/**
*@function:rtcp协商流化控制播放
*@param[in]:int srcno  源标识
*@param[in]:long chid  通道
*@param[in]:double npt  客户端seek之后的npt时间
*@param[in]:float scale  客户端seek之后的播放倍率
*@param[out]:uint32_t *rtp_pkt_timestamp  seek之后的rtp包的时戳
**/
typedef int (MEDIASERVER_STDCALL *xt_tcp_play_cb_type)(int srcno, long chid, double npt, float scale, uint32_t *rtp_pkt_timestamp);

/**
*@function:tcp协商流化控制暂停回调函数
*@param[in]:int srcno  源标识
*@param[in]:long chid  通道
**/
typedef int( MEDIASERVER_STDCALL *xt_tcp_pause_cb_type)(int srcno, long chid);

//链路开启与停止事件
enum LINK_STATE_EVENT
{
    EVENT_TCP_PLAY=0, 
    EVENT_TCP_STOP,
    EVENT_TCP_CLOSE,
    EVENT_UDP_PLAY,
    EVENT_UDP_STOP,
    EVENT_RTSP_PLAY,
	EVENT_RTSP_CTRLPLAY,
	EVENT_RTSP_CTRLPAUSE,
    EVENT_RTSP_STOP,
}; 
typedef int(MEDIASERVER_STDCALL *xt_link_state_event_type)(const LINK_STATE_EVENT evnt,const int srno);

//日志输出回调用
enum log_level_type
{
    log_info=0,
    log_err,
    log_warn,
};
typedef void(MEDIASERVER_STDCALL *xt_media_server_log_cb_type)(char* logname,log_level_type level,char* log_ctx,uint32_t log_ctx_len);

//注册时间结果回调
typedef void(MEDIASERVER_STDCALL *regist_response_callback_t)(const char *ip, uint16_t port, uint32_t code);

// mediaserver config
struct MS_CFG 
{
    int num_chan;                        // 通道总数
    char ip[MEDIA_SERVER_IP_LEN];                  // ip
    unsigned short snd_start_port;                 // 起始端口
    bool         demux;                           // 复用
    char mul_start_ip[MEDIA_SERVER_IP_LEN];          // 组播起始地址
    unsigned short msg_liten_port;                  // 私有监听端口   0表示不启用
    unsigned short rtsp_listen_port;                // rtsp监听端口     0表示不启用
    unsigned short tcp_listen_port;                 // tcp传输监听发送端口  0表示不启用
    unsigned short udp_listen_port;                 // udp侦听端口         0表示不启用

    bool          snd_std_rtp;                     // 是否发送标准流
    bool         sink_single;                      // 是否单一转发
    bool        use_traffic_shaping;                //是否开启流量整形
    xt_rtsp_play_cb      rtsp_play_cb;               // 基于RTSP的流化控制
    xt_rtsp_pause_cb     rtsp_pause_cb;
    xt_tcp_play_cb_type   tcp_play_cb;                // 基于TCP会话的流化控制
    xt_tcp_pause_cb_type  tcp_pause_cb;
    xt_link_state_event_type xt_link_state_event;       // 链路状态事件
    xt_media_server_log_cb_type xt_media_server_log_cb;  // 日志输出回调用
    xt_rtcp_force_iframe_cb_t  rtcp_force_iframe_cb;     // RTCP媒体层强制I帧回调
    MS_CFG():
        num_chan(0),
        snd_start_port(0),
        demux(false),
        msg_liten_port(0),
        rtsp_listen_port(0),
        tcp_listen_port(0),
        udp_listen_port(0),
        snd_std_rtp(false),
        sink_single(false),
        use_traffic_shaping(false),
        rtsp_play_cb(NULL),
        rtsp_pause_cb(NULL),
        tcp_play_cb(NULL),
        tcp_pause_cb(NULL),
        xt_link_state_event(NULL),
        xt_media_server_log_cb(NULL),
        rtcp_force_iframe_cb(NULL){}
};
struct src_track_info_t 
{
    int tracknum;
    int trackids[MAX_TRACK];
    char tracknames[MAX_TRACK][128];
};

//SIP回调
///////////////////////////////////////////////////////////////////////////////////////////////////
#define SIPMSG_NOEDE_LEN 128

//镜控制参数结构
struct sipmsg_ptz 
{
    char commandname[SIPMSG_NOEDE_LEN];
    char operation[SIPMSG_NOEDE_LEN];
    char seq[SIPMSG_NOEDE_LEN];
    char devname[SIPMSG_NOEDE_LEN];
    char devid[SIPMSG_NOEDE_LEN];
    char zoomctrl[SIPMSG_NOEDE_LEN];
    char focusctrl[SIPMSG_NOEDE_LEN];
    char aperturectrl[SIPMSG_NOEDE_LEN];
    char direction[SIPMSG_NOEDE_LEN];
    char speed[SIPMSG_NOEDE_LEN];
    char pointindexvalue[SIPMSG_NOEDE_LEN];
};
/**
*@ function:云镜控制
*@ param[in]:const sipmsg_ptz *ptz:输入参数 镜控制参数
**/
typedef void(MEDIASERVER_STDCALL *sipmsg_ptz_cb)(const sipmsg_ptz *ptz);


/**
*@ function: 强制I帧
*@ param[in]:const sipmsg_ifrmconfreq *ifrmconfreq: 输入参数 设备I帧间隔相关参数
*@ param[out]:int& out_oprcode: 输出参数 0:为操作成功 小于0：为操作失败
*@ param[out]:char* out_fail_case: 输出参数 此参数与out_oprcode相关 当out_oprcode为小于0时要写明失败原因长度不能超过2048否则出现截断
**/
typedef void(MEDIASERVER_STDCALL *sipmsg_picupdate_cb)(int streamid,int& out_oprcode,char* out_fail_case);

/**
*@ function: 会话外的 message消息
*@ param[in]:const char *xml: 输入参数 消息体
*@ param[in]:unsigned int len: 输入参数 消息体长度
**/
typedef void(MEDIASERVER_STDCALL *sipmsg_cb)(const char *xml, unsigned int len);

/**
*@ function:会话内info 回调用
*@ param[in]:const char *xml: 输入参数 消息体
*@ param[in]:unsigned int len: 输入参数 消息体长度
**/
typedef void(MEDIASERVER_STDCALL *sipinfo_cb)(int streamid, const char *xml, unsigned int len);

//设备I帧间隔
struct sipmsg_ifrmconfreq 
{
    int seq;
    char devname[SIPMSG_NOEDE_LEN];
    char devid[SIPMSG_NOEDE_LEN];
    char groupid[SIPMSG_NOEDE_LEN];
    int iframedelta;
};
/**
*@ function:设备I帧间隔
*@ param[in]:const sipmsg_ifrmconfreq *ifrmconfreq: 输入参数 设备I帧间隔相关参数
*@ param[out]:int& out_oprcode: 输出参数 0:为操作成功 小于0：为操作失败
*@ param[out]:char* out_fail_case: 输出参数 此参数与out_oprcode相关 当out_oprcode为小于0时要写明失败原因长度不能超过2048否则出现截断
**/
typedef void(MEDIASERVER_STDCALL *sipmsg_ifrmconfreq_cb)(const sipmsg_ifrmconfreq *ifrmconfreq,int& out_oprcode,char* out_fail_case);


//视频调整回调
typedef struct _struct_video_adjust_param_t 
{
    char framerate[64];  //帧率
    char max_br[64];    //码率
    char resolution[64]; //分辨率
}video_adjust_param_t,*pvideo_adjust_param_t;
typedef void(MEDIASERVER_STDCALL *sip_video_adjust_cb)(const video_adjust_param_t&adjust_params,const int streamid);

//获取设备信息
typedef struct _struct_device_info_type
{
    char devname[SIPMSG_NOEDE_LEN];         // 设备名称
    char vendername[SIPMSG_NOEDE_LEN];      // 设备厂商
    char deviceversion[SIPMSG_NOEDE_LEN];   // 设备型号
    char bandwidth[SIPMSG_NOEDE_LEN];       // 视频编码速率单位kbps
}dev_info_t,*pdev_info_t;

/*
*@param[in]:dev_info_t& out_devinfo: 为输出设备信息
*@return int :0<=：获取成功 0>:获取失败
*/
typedef int (MEDIASERVER_STDCALL *sip_get_dev_info_cb)(dev_info_t& out_devinfo);

//预置点控制
typedef struct _struct_point_index_operation_type_
{
    char operation[SIPMSG_NOEDE_LEN];//add|remove|set
    char devname[SIPMSG_NOEDE_LEN];
    char devid[SIPMSG_NOEDE_LEN];
    char  seq[SIPMSG_NOEDE_LEN];
    char pointindexvalue[SIPMSG_NOEDE_LEN];
}point_index_operation_t,*ppoint_index_operation_t;

typedef int (MEDIASERVER_STDCALL *sip_point_index_operation_cb)(point_index_operation_t& pio);

/*
*@ function:反馈注册sip服务器的注册结果
*@param[in]:const char* target 注册目标
*@param[in]:const uint8_t success success=0表示失败else成功
*/
typedef void (MEDIASERVER_STDCALL* sip_register_srv_ret_info_cb)(const char* target,const uint8_t success);

/*
*@ function:解析出SDP里的b字段，并将b字段带宽传给编码器
*@param[in]:uint32_t bandwidth 带宽数值
*@return[]:成功返回0，失败返回负数
*/
typedef int (MEDIASERVER_STDCALL* sipmsg_bandwidth_cb)(uint32_t bandwidth);


/*
*@ function:解析出SDP里的a字段的fmtp，并将fmtp字符串传给编码器
*@param[in]:const char* profile  fmtp字符串
*@return[]:成功返回0，失败返回负数
*/
typedef int (MEDIASERVER_STDCALL* sipmsg_profile_cb)(const char* profile);
MEDIASERVER_API void xt_sipmsg_ptz_cb(sipmsg_ptz_cb cb);
MEDIASERVER_API void xt_sipmsg_picupdate_cb(sipmsg_picupdate_cb cb);
MEDIASERVER_API void xt_sipmsg_cb(sipmsg_cb cb);
MEDIASERVER_API void xt_sipinfo_cb(sipinfo_cb cb);
MEDIASERVER_API void xt_sipmsg_ifrmconfreq_cb(sipmsg_ifrmconfreq_cb cb);
MEDIASERVER_API void xt_regist_sip_video_adjust_cb(sip_video_adjust_cb cb);
MEDIASERVER_API void xt_get_dev_info_cb(sip_get_dev_info_cb cb);
MEDIASERVER_API void xt_point_index_operation_cb(sip_point_index_operation_cb cb);
MEDIASERVER_API void xt_sip_register_srv_ret_info_cb(sip_register_srv_ret_info_cb cb);
MEDIASERVER_API void xt_sipmsg_bandwidth_cb(sipmsg_bandwidth_cb cb);
MEDIASERVER_API void xt_sipmsg_profile_cb(sipmsg_profile_cb cb);
///////////////////////////////////////////////////////////////////////////////////////////////////

//对外接口
//////////////////////////////////////////////////////////////////////////////////////////  
/**
*@name:xt_init_server
*@funcion:初始化 xt_media_server库
*@param[int] MS_CFG &cfg 配置信息
**/
MEDIASERVER_API int xt_init_server(MS_CFG &cfg);

/**
*@name:xt_uninit_server
*@funcion:反初始化 xt_media_server库
**/
MEDIASERVER_API int xt_uninit_server();

/**
*@name:xt_create_src
*@function:创建转发源
*@param[in]:int tracknum  流数量
*@param[in]:int *trackids  流ids
*@param[in]:char *tracknames[] 流的名称
*@param[out]:int &srcno 源标识
*@param[in]:long chanid = -1  指定通道
**/
MEDIASERVER_API int xt_create_src(int tracknum, int *trackids, char *tracknames[], int &srcno, long chanid = -1);

// track信息
#define PRI_TRACK_ID -1
#define MAX_TRACKNAME_LEN 64
typedef struct _struct_xt_track_type__
{
    char trackname[MAX_TRACKNAME_LEN];    // 流name
    int trackid;                          // 流id
    int tracktype;                        // 数据类型。0：video；1：audio；－1：其他
    int frametype;                        // 数据帧类型 参考share_type_def.h ov_frame_type
    unsigned long chanid;                 // 通道id
}xt_track_t,*pxt_track_t;
/**
*@name:xt_create_src_sdp
*@function:创建转发源
*@param[out]:int* srcno  源标识
*@param[out]:xt_track_t* trackinfos
*@param[out]:int* tracknum
*@param[in]:int *sdp  sdp
*@param[in]:char sdp_len 
*@param[in]:long chanid 指定通道 -1表示内部自动分配
**/
MEDIASERVER_API int xt_create_src_sdp(int* srcno,xt_track_t* trackinfos,int* tracknum,const char* sdp,const long sdp_len,const long chanid);

/**
*@name:xt_create_src_defult
*@function:创建转发源
*@param[out]:int* srcno  源标识
*@param[out]:char sdp[]
*@param[out]:int* sdp_len
*@param[in]:long chanid 指定通道 -1表示内部自动分配
*@param[in]:const char* local_bind_ip 本地绑定IP
**/
MEDIASERVER_API int xt_create_src_defult(int* srcno,char sdp[],int* sdp_len,const long chanid,const char* local_bind_ip);

/**
*@name:xt_create_src
*@function:创建转发源
*@param[in]:src_track_info_t &tracks  流信息
*@param[out]:int &srcno  源标识
*@param[in]:long chanid = -1  指定通道 -1表示内部自动分配
**/
MEDIASERVER_API int xt_create_src(src_track_info_t &tracks, int &srcno, const long chanid = -1);

/**
*@name:xt_destroy_src
*@function:删除转发源
*@param[in]:int srcno 源标识
**/
MEDIASERVER_API int xt_destroy_src(int srcno);

/**
*@name:xt_set_key
*@function:设置系统头
*@param[in]:int srcno
*@param[in]:char *keydata
*@param[in]:long len
*@param[in]:long datatype
**/
MEDIASERVER_API int xt_set_key(int srcno, char *keydata, long len, long datatype);

/**
*@name:xt_set_sipsdp_full
*@function:设置sip全能力集系统头
*@param[in]:int srcno
*@param[in]:char *keydata
*@param[in]:long len
**/
int xt_set_sipsdp_full(int srcno, char *keydata, long len);

/**
*@name:xt_send_data
*@function:数据发送
*@param[in]:int srcno  源id
*@param[in]:int trackid  trackid
*@param[in]:char *buff  发送数据
*@param[in]:unsigned long len  数据长度
*@param[in]:int frame_type  帧类型
*@param[in]:long device_type  设备类型
**/
MEDIASERVER_API int xt_send_data(int srcno, int trackid, char *buff, unsigned long len, int frame_type, long device_type);

/**
*@name:xt_send_data_in_stamp
*@function:数据发送由外部生成时戳并传入
*@param[in]:int srcno  源id
*@param[in]:int trackid  trackid
*@param[in]:char *buff  发送数据
*@param[in]:unsigned long len  数据长度
*@param[in]:int frame_type  帧类型
*@param[in]:long device_type  设备类型
*@param[in]:bool frame_ts_flg  是否外部传入时戳
*@param[in]:uint32_t in_time_stamp  外部输入时戳
**/
MEDIASERVER_API int xt_send_data_in_stamp(int srcno, int trackid, char *buff, unsigned long len, 
										  int frame_type, long device_type, bool frame_ts_flg, uint32_t in_time_stamp,
										  bool use_ssrc,uint32_t ssrc);

MEDIASERVER_API int xt_send_rtp_in_stamp(int srcno, int trackid, char *buff, unsigned long len, 
										  int frame_type, long device_type, bool frame_ts_flg, uint32_t in_time_stamp,
										  bool use_ssrc,uint32_t ssrc);

/**
*@name:xt_send_data_in_stamp_p
*@function:数据发送由外部丢包重传优先级
*@param[in]:int srcno  源id
*@param[in]:int trackid  trackid
*@param[in]:char *buff  发送数据
*@param[in]:unsigned long len  数据长度
*@param[in]:int frame_type  帧类型
*@param[in]:long device_type  设备类型
*@param[in]:bool frame_ts_flg  是否外部传入时戳
*@param[in]:uint32_t in_time_stamp  外部输入时戳
*@param[in]:uint8_t priority  发送数据优先级
**/
MEDIASERVER_API int xt_send_data_in_stamp_p(int srcno,int trackid,char *buff,unsigned long len,int frame_type,long device_type,bool frame_ts_flg,uint32_t in_time_stamp,uint8_t priority);

/**
*@name:xt_send_data_in_stamp_ps
*@function:ps流的数据发送接口
*@param[in]:int srcno  源id
*@param[in]:int trackid  trackid
*@param[in]:char *buff  发送数据
*@param[in]:unsigned long len  数据长度
*@param[in]:int frame_type  帧类型
*@param[in]:long device_type  设备类型
*@param[in]:bool frame_ts_flg  是否外部传入时戳
*@param[in]:uint32_t in_time_stamp  外部输入时戳
*@param[in]:uint8_t priority  发送数据优先级
**/
MEDIASERVER_API int xt_send_data_in_stamp_ps(int srcno,int trackid,char *buff,unsigned long len,int frame_type,long device_type,bool frame_ts_flg,uint32_t in_time_stamp,uint8_t priority,bool use_ssrc, uint32_t ssrc);

/**
*@name:xt_send_data_single
*@function:单兵系统支持
*@param[in]:int src_prime  源id
*@param[in]:int trackid  tracki
*@param[in]:char *buff  发送数据
*@param[in]:unsigned long len  数据长度
*@param[in]:int frame_type  帧类型
*@param[in]:long device_type  设备类型
**/
MEDIASERVER_API int xt_send_data_single(int src_prime, int trackid, char *buff, unsigned long len, int frame_type, long device_type);


/**
*@name:xt_get_chanid
*@function:查找转发源对应通道号
*@param[in]:int srcno
*@param[in]:int trackid
*@param[out]:long &chanid
**/
MEDIASERVER_API int xt_get_chanid(int srcno, int trackid, long &chanid);

/**
*@name:xt_get_sinksn
*@function:获取一路rtp流sn
*@param[in]:const int srcno
*@param[in]:const int trackid
*@param[out]:unsigned short *sn
**/
MEDIASERVER_API int xt_get_rtp_sn(const int srcno, const int trackid, unsigned short *sn);

/**
*@name:xt_regist
*@function: 向服务器反向注册
*@param[in]:const char* sz_ids
*@param[in]:const char* sz_local_ip
*@param[in]:unsigned short local_port
*@param[in]:const char* sz_server_ip
*@param[in]:unsigned short server_port
**/
MEDIASERVER_API int xt_regist(const char* sz_ids, const char* sz_server_ip, unsigned short server_port, uint32_t millisec);

/**
*@name:xt_stop_regist
*@function:关闭服务器反向注册
*@param[in]:const char* sz_server_ip
*@param[in]:unsigned short server_port)
**/
MEDIASERVER_API int xt_stop_regist(const char* sz_server_ip,unsigned short server_port,uint32_t millisec);

/**
*@name:xt_isregist
*@function:查看反向注册状态
**/
MEDIASERVER_API bool xt_isregist();

//设置注册结果回调
MEDIASERVER_API void xt_regist_response_callback(regist_response_callback_t func);


//xmpp JigaleRTP支持
///////////////////////////////////////////////////////////////////////////////
enum xmpp_msg_t
{
    XMPP_STOP_PLAY = 0,
    XMPP_START_PLAY,
};
typedef struct _struct_xmpp_cfg
{
    xmpp_msg_t   ctrl;               //控制编号 
    char  LAddr[MEDIA_SERVER_IP_LEN];  //本地IP
    char  Addr[MEDIA_SERVER_IP_LEN];  //远端IP
    long  Channle;
    long  link_type;
    unsigned short nRptPort_r;      //远端端口 
    unsigned int multiplexID;       //复用ID
    bool  multiplex;                //是否端口复用

}xmpp_cfg_t,*pxmpp_cfg_t;
/**
*@name:add_trans_sever
*@function:xmpp JigaleRTP 增加一个转发
*@param[in]:xmpp_cfg_t& cfg
**/
MEDIASERVER_API int add_trans_sever(xmpp_cfg_t& cfg);
///////////////////////////////////////////////////////////////////////////////

/**
*@name:xt_get_cur_connect_num
*@ function:获取当前链接数
*@ return: int 返回当前链接数
*@ note: 配合xt_get_connect_info 接口使用
**/
MEDIASERVER_API uint32_t xt_get_cur_connect_num();

/**
*@name:xt_get_connect_info
*@ function:获取当前链接信息
*@ param[out]:connect_info_t out_cinfo[]
*@ param[in out]:connect_num  in:最大接收输出条数 out:实际获取到的链接数
*@ return: int 小于0为失败
**/
MEDIASERVER_API int xt_get_connect_info(connect_info_t out_cinfo[], uint32_t& connect_num);


//开启SIP协议栈的定时器配置类型
typedef struct _struct_start_sip_timer_type_ 
{
    uint32_t default_session_time_interval; //会话保活时间，单位：秒 [90, 180]秒 发送UPDATE
    uint32_t default_registration_retry_time_interval; //注册重试时间
}start_timer_t,*pstart_timer_t;

typedef enum _enum_sip_trans_protocol_type_
{
    TRANS_PROTOCOL_NONE = 0,
    TRANS_PROTOCOL_TCP = 1<<0,
    TRANS_PROTOCOL_UDP = 1<<1
}sip_trans_protocol_t,*psip_trans_protocol_t;
typedef struct _struct_start_sip_port_type_
{
    uint32_t protocol;   //传输协议 由sip_trans_protocol_t 进行"|"或运算得来有可能只使用tcp或udp与可能同时使用
    uint16_t transport;  //sip 传输端口
    uint16_t tls_port;  //传输层tls端口，填0表示不开启
    uint16_t dtls_port;  //传输层dtls端口，填0表示不开启
}start_sip_port_t,*pstart_sip_port_t;
/**
*@name:xt_start_sip
*@ function: 启动SIP协议栈
*@ param[in]:const start_sip_port_t& sip_port:端口及协议信息
*@ param[in]:const char *domain: 用户认证域名
*@ param[in]:const char *username: 
*@ param[in]:const char *password: 
*@ param[in]:const start_timer_t& timer:定时器
**/
MEDIASERVER_API int xt_start_sip(const start_sip_port_t& sip_port, const start_timer_t& timer, const char *domain, const char *username, const char *password);

/**
*@name:xt_stop_sip
*@function:停止SIP协议栈
**/
MEDIASERVER_API void xt_stop_sip();

//注册时信道特征描述
typedef enum _enum_channel_type
{
    XT_CHAN_NONE            = 0,
    XT_CHAN_SATELLITE       = 1<<0, //卫星 satellite
    XT_CHAN_MOBILE          = 1<<1, //移动 mobile
    XT_CHAN_MICROWARE       = 1<<2, //微波 microwave
    XT_CHAN_SHORTWARE       = 1<<3, //超短波 shortwave
    XT_CHAN_FIXED           = 1<<4 //固定网络通道  fixed
}channel_t;
//信道描述，参考总部规范
typedef struct _struct_sip_channel_type_
{
    uint32_t type;                  //信道类型 此值由channel_t 或运算得到有可能是一种类型也可能是多种类型
    uint32_t delay;                 //延时
    uint32_t packetloss;            //丢包率
    uint32_t mtu;                   //最大传输单元 默认：1500
    uint32_t bandwidth;             //带宽
}sip_channel_t,*psip_channel_t;

typedef struct _struct_sip_regist_timer_type_
{
    uint32_t  expires;    //有两个意思 1、代理服务器过期时间 2、REGIST 刷新时间 [60, 3600]秒
    uint32_t  link_time_interval; //链路保活时间 单位毫秒  发OPTION的时间 [25000, 90000]ms
    uint32_t  regist_retry_time_interval; //REGIST 失败重试时间 [60,3600]s

}regist_timer_t,*pregist_timer_t;
/**
*@name:xt_regist_sip
*@ function:向代理服务器注册
*@ param[in]:const char *target 注册目标服务器 例：sip:router@11.55.30.11:5060
*@ param[in]:const regist_timer_t& timer 注册定时器时间设置参数
*@ param[in]:const char* sdp
*@ param[in]:uint32_t sdp_len
*@ param[in]:const sip_channel_t& chtype 信道信息
*@ return: 0:成功 -1:target 为空失败
**/ 
MEDIASERVER_API int xt_regist_sip(const regist_timer_t& timer, const sip_channel_t& chtype,const char *target,const char* sdp, uint32_t sdp_len);

/**
*@name:xt_unregist_sip
*@function: sip unregister
**/
MEDIASERVER_API void xt_unregist_sip();


/**
*@name:xt_add_send
*@function: 增加转发
*@param[in]:int srcno  源id
*@param[in]:int trackid  track id
*@param[in]:const char *ip,  目标ip
*@param[in]:unsigned short port  目标port
*@param[in]:bool   demux = false  复用
*@param[in]:unsigned int demuxid = 0  复用id
**/
MEDIASERVER_API int xt_add_send(int srcno, int trackid, const char *ip, unsigned short port, bool   demux = false, unsigned int demuxid = 0);

/**
*@name:
*@function:增加转发
*@param[in]:int srcno  源id
*@param[in]:int trackid  track name
*@param[in]:const char *ip,  目标ip
*@param[in]:unsigned short port  目标port
*@param[in]:bool   demux = false  复用
*@param[in]:unsigned int demuxid = 0  复用id
**/
MEDIASERVER_API int xt_add_send(int srcno, const char  *track, const char  *ip, unsigned short port, bool   demux = false, unsigned int demuxid = 0);

/**
*@name:
*@function:设置payload
*@param[in]:int srcno  源id
*@param[in]:int trackid  track id
*@param[in]:int payload  payload
*@param[in]:bool update  update
**/
MEDIASERVER_API int xt_set_payload(int srcno, int trackid, int payload, bool update);

/**
*@name:
*@function:设置payload
*@param[in]:int srcno  源id
*@param[in]:const char  *track  track name
*@param[in]:int payload  payload
*@param[in]:bool update  update
**/
MEDIASERVER_API int xt_set_payload(int srcno, const char  *track, int payload, bool update);

/**
*@name:
*@function:删除转发
*@param[in]:int srcno,  源id
*@param[in]:int trackid  track id
*@param[in]:const char  *ip  目标ip
*@param[in]:unsigned short port / 目标port
*@param[in]:bool demux = false  复用
*@param[in]:unsigned int demuxid = 0  复用id
**/
MEDIASERVER_API int xt_del_send(int srcno, int trackid, const char  *ip, unsigned short port, bool demux = false, unsigned int demuxid = 0);

/**
*@name:xt_del_send
*@function:xt_del_send
*@param[in]:int srcno  源id
*@param[in]:const char *tracki  track name
*@param[in]:unsigned short port  目标port
*@param[in]:bool demux = false  复用
*@param[in]:unsigned int demuxid = 0  复用id
**/
MEDIASERVER_API int xt_del_send(int srcno, const char *tracki, const char *ip, unsigned short port, bool demux = false, unsigned int demuxid = 0);

/**
*@name:xt_del_send_src
*@function:  删除转发(srcno 源id)
*@param[in]:int srcno
**/
MEDIASERVER_API int xt_del_send_src(int srcno);

/**
*@name:xt_del_send_all
*@function:删除所有转发
**/
MEDIASERVER_API int xt_del_send_all();

//发送信息结构类型
typedef struct _struct_server_info
{
    int trackid;
    bool multiplex_s;     // 发送端复用标识 
    uint32_t rtp_send_port;  // 发端口
    uint32_t rtcp_send_port;  // 发端口    
    uint32_t  multid_s;      // 发送端复用ID
    char trackname[64];      //流name

}svr_info,*psvr_info;
/**
*@name:xt_get_svr_info
*@function:xt_get_svr_info
*@param[out]:svr_info info[]  转发信息
*@param[in out]:int& tracknum  流数量
*@param[in]:const int srcno  源标识
**/
MEDIASERVER_API int xt_get_svr_info(svr_info info[],int& tracknum,const int srcno);

/**
*@ function:修改丢包重传开关
*@ param[in] const int resend_flag 重传开关标志 大于0为开
*@ return int 操作结果 小于0操作失败
**/
MEDIASERVER_API int xt_update_resend_flag(const int resend_flag);

MEDIASERVER_API void xt_set_file_path(const char * file);

/**
*@ function: 设置RTSP会话的心跳时间
*@ param[in] const unsigned int check_timer_interval 检测定时器的触发时间间隔ms
*@ param[in] const unsigned int time_out_interval 判定超时的时间间隔差ms
*@ return int 操作结果 小于0失败 其它为成功
**/
MEDIASERVER_API int xt_ms_set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);

#ifdef _USE_RTP_SEND_CONTROLLER
typedef void (MEDIASERVER_STDCALL*xt_network_changed_callback_t)(void *ctx, uint32_t bitrate, uint32_t fraction_lost, uint32_t rtt);
MEDIASERVER_API int xt_register_network_changed_callback(int srcno, int trackid, xt_network_changed_callback_t cb, void *ctx);
#endif

//公司码流类型
typedef enum
{
    ms_code_na   = -1,
    ms_code_main  = 0,
    ms_code_sub   = 1,
    ms_code_audio = 2,
}ms_code_t;

/**
*@ function: 多码流转发查询
*@ param[in] const int main_srcno 主通道
*@ param[in] const ms_code_t code 码流类型
*@ param[out] int* srcno 实际转发的转发源标识号
*@ return int 操作结果 小于0失败 其它为成功
**/
typedef int (MEDIASERVER_STDCALL *multi_code_query_callback_t)(const int prime_srcno,const ms_code_t code,int* srcno);
MEDIASERVER_API void xt_register_multi_code_query_callback(multi_code_query_callback_t cb);

#endif  //_H_XTMEDIASERVER_H
