#ifndef XTRTP_H__INCLUDE___
#define XTRTP_H__INCLUDE___

#ifdef _OS_WINDOWS
#include <Windows.h>
#endif
#include <string>
#include <map>
#include "xt_mp_caster_def.h"
#include "xt_mp_caster_api.h"
#include "common.h"
#include <list>

#include <boost/thread/thread.hpp>
#include "h_xtmediaserver.h"

enum Rtp_Mode 
{
    M_RTP = 0,
    M_MUL = 1,
};

// rtp发送单元信息
struct  Rtp_Handle
{
    mp_h_s			hmp;		// 发送单元句柄
    mssrc_h_s		hmssrc;		// 数据传入句柄
    msink_h_s		hmsink;		// 数据发送句柄
    int				payload_old;// 负载类型old
    int				payload;	// 负载类型new
    unsigned short  port;		// 发送端口
    bool			multiplex;	// 服用
    unsigned int	multid;		// 复用ID

#ifdef _USE_RTP_SEND_CONTROLLER
    xt_network_changed_callback_t cb;
    void *ctx;
#endif
};

// 转发节点
struct Rtp_Sink 
{
    common::Time_Type  createtime;//创建时间
    unsigned int	chanid;		// 通道号
    msink_h_s		hmsink;		// 节点句柄
    std::string			ip;			// ip		
    unsigned short	port;		// port
    void*			linkid;		// 连接句柄
    short			mode;		// 传输模式(0,1: 单播，2：组播)
    unsigned int	ssrc;		// ssrc
    bool			multiplex;	// 复用
    unsigned int	multid;		// 复用id

	bool local_multiplex;//发送复用
	unsigned int local_multid;//发送复用id

    rv_rtcp_info send;
    rv_rtcp_info recieve;
	uint32_t rr_rtt;

    Rtp_Sink& operator=(const Rtp_Sink& rf)
    {
        this->createtime = rf.createtime;
        this->chanid = rf.chanid;
        this->hmsink = rf.hmsink;
        this->ip = rf.ip;
        this->port = rf.port;
        this->linkid = rf.linkid;
        this->mode = rf.mode;
        this->ssrc = rf.ssrc;
        this->multiplex = rf.multiplex;
        this->multid = rf.multid;
		this->local_multiplex = rf.local_multiplex;
		this->local_multid = rf.local_multid;
        ::memcpy(&(this->send),&(rf.send),sizeof(rv_rtcp_info));
        ::memcpy(&(this->recieve),&(rf.recieve),sizeof(rv_rtcp_info));

        return *this;
    }
};

#define MAX_SINK_IP_LEN 256
typedef struct _struct_get_rtp_sink_condition_type_
{
    char ip[MAX_SINK_IP_LEN];  //ip
    unsigned long chanid;      //通道
    unsigned short port;      //端口
    bool multiplex_flag;      //复用标识
    unsigned int muxid;       //复用id
}rtp_sink_comdition_t,*prtp_sink_comdition_t;

class XTRtp
{
private:
    XTRtp(void);
    ~XTRtp(void);

    static XTRtp self;

public:
    static XTRtp* instance(){return &self;}
    static XTRtp* _(){return &self;}

    // rtp远端地址更新
    static void _raddr_cb(void *hmp, rv_net_address *addr);

    // rtcp回调
    static void sink_rtcp_cb(unsigned int ssrc,		// ssrc
        rv_rtcp_info &send,			// sr
        rv_rtcp_info &recieve,
		unsigned char *ip,
		unsigned short port,
		int multiplex,
		unsigned int multid);		// rr

    // rtcp app回调
    static rv_bool rtcpapp_msg_cb(unsigned int		chan_id,	// 通道号
        unsigned char		subtype,	// 子类型	
        unsigned int		ssrc,		// ssrc
        unsigned char*		name,		// msg name
        unsigned char*		userData,	// user data
        unsigned int		userDataLen);// user data length

    // rtcp raw cb
    static rv_bool rtcp_rawdata_cb(
        mp_handle sink,
        uint8_t *buffer,
        uint32_t buffLen,
        rv_net_address *remoteAddress,
        rv_bool *pbDiscardBuffer);

    // 是否相同sink
    static bool is_same_sink(Rtp_Sink &s1, Rtp_Sink &s2);

    // 初始化
    int init(unsigned int num_chan,			// 通道数
        std::string ip,						// ip
        unsigned short start_port,		// 起始端口
        bool multiplex,					// 复用
        bool sink_single,				// 通道单转发
        bool use_traffic_shapping = false);     //是否开启流量整形

    // 反初始化
    int uninit();

    // 设置负载类型
    void set_payload(unsigned long chanid, int payload, bool update);
    void reset_payload(unsigned long chanid);

	int get_payload(unsigned long chanid);

    //更新丢包重传标志 add by songlei 20150708
    int update_resend_flag(const int flag);

	// 设置文件保存
	void set_file_patha(const char * file);

    // 增加转发
    int add_send(long chanid,			// 通道号
        std::string ip,						// ip
        unsigned short port,			// port
        void* linkid,					// 连接句柄
        long mode,						// 传输模式(0,1: 单播，2：组播)
        unsigned int ssrc = 0,			// ssrc
        bool multiplex = false,			// 复用
        unsigned int multid = 0);		// 复用id

    // 删除转发
    int del_send(long chanid,			// 通道号
        std::string ip,						// ip
        unsigned short port,			// port
        long mode,						// 传输模式(0,1: 单播，2：组播)
        bool multiplex = false,			// 复用
        unsigned int multid = 0);		// 复用id

    // 删除转发(linkid 会话句柄)
    int del_send(void *linkid);

    // 删除所有转发
    int del_send_all();

    // 删除通道转发(chanid 通道号)
    int del_send_chan(unsigned long chanid);

    int send_data(unsigned long chanid, char *buff, unsigned long len, int frame_type, long device_type, bool is_std);
    int send_data(const xt_track_t& track, char *buff, unsigned long len, long device_type, bool is_std);

    // 数据发送
    int send_data_in_stamp(unsigned long chanid,			// 通道号
        char *buff,						      // 发送数据
        unsigned long len,				// 数据长度
        int frame_type,					// 帧类型
        long device_type,               // 设备类型
        bool frame_ts_flg,    // 是否外部传入时戳
        uint32_t in_time_stamp,// 外部输入时戳
        uint8_t priority,
        bool is_std = false);

    // 帧数据发送
    int send_data_in_stamp(const xt_track_t& track,  // 转发track
        char *buff,                                  // 发送数据
        unsigned long len,                           // 数据长度
        long device_type,                            // 设备类型
        bool frame_ts_flg,                           // 是否外部传入时戳
        uint32_t in_time_stamp,                      // 外部输入时戳
        uint8_t priority,                            // 优先级
        bool is_std = false,
		bool use_ssrc =false,
		uint32_t ssrc = 0);                        // 标准流开关

	// 包数据发送
	int send_rtp_in_stamp(const xt_track_t& track,  // 转发track
		char *buff,                                  // 发送数据
		unsigned long len,                           // 数据长度
		long device_type,                            // 设备类型
		bool frame_ts_flg,                           // 是否外部传入时戳
		uint32_t in_time_stamp,                      // 外部输入时戳
		uint8_t priority,                            // 优先级
		bool is_std = false,
		bool use_ssrc =false,
		uint32_t ssrc = 0);                        // 标准流开关

        //20160405 增加国标PS流RTP发送接口
    int send_data_in_stamp_ps(const xt_track_t& track,    // 转发track
        char *buff,                               // 发送数据
        unsigned long len,                        // 数据长度
        long device_type,                         // 设备类型
        bool frame_ts_flg,                        // 是否外部传入时戳
        uint32_t in_time_stamp,                   // 外部输入时戳
        uint8_t priority,                         // 优先级
        bool is_std,                              // 标准流开关
        bool usr_ssrc,
        uint32_t ssrc);


    void get_rtp_sink(std::list<Rtp_Sink>& lst_rtp_Sink);

    int get_cur_sink_num();

	int get_sink_sn(long chanid, unsigned short *sn);

    int get_rtp(unsigned long chanid, Rtp_Handle &rtp);

    //保存RTCP报告
    void set_rtcp_rport(unsigned int ssrc, const rv_rtcp_info &send,const rv_rtcp_info &recieve,
		char *ip, unsigned short port, bool multiplex, unsigned int multid);

    //获取sink
    int get_sink_by_comdition(Rtp_Sink& out_sink,const rtp_sink_comdition_t& comdition);

#ifdef _USE_RTP_SEND_CONTROLLER
    int register_network_changed_callback(int chan, xt_network_changed_callback_t cb, void *ctx);
#endif

private:
    // 获得转发节点(sink in/out)
    int get_sink(Rtp_Sink &sink);
    int get_sink(void *linkid, Rtp_Sink &sink);
    int get_sink(unsigned long chanid, Rtp_Sink &sink);

    int get_rtp(void *hmp, unsigned long &chanid, Rtp_Handle &rtp);

    // 增加转发节点
    int add_sink(Rtp_Sink &sink);

    // 删除转发节点
    int delete_sink(Rtp_Sink &sink);

    // 删除转发(未加锁)
    int _del_send(unsigned long chanid,			
        std::string ip,						
        unsigned short port,			
        short mode,					
        bool multiplex,		
        unsigned int multid); 
private:
    boost::shared_mutex			m_mutex;		//mutex
    std::map<unsigned int, Rtp_Handle>	m_rtpHandles;	// rtp转发单元
    std::list<Rtp_Sink>					m_rtpSinks;		// rtp转发节点

    bool							m_sink_single;	//通道单转发
};
#endif//#ifndef XTRTP_H__INCLUDE___
