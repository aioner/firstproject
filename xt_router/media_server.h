#ifndef	MEDIA_SERVER_H__
#define MEDIA_SERVER_H__ 

#include <string>
#include <boost/noncopyable.hpp>
#include "h_xtmediaserver.h"
#include <stdint.h> 

class media_server : boost::noncopyable
{
private:
    media_server(void);
    ~media_server(void);

    static media_server self;

public:
    static media_server* instance(){return &self;}
    static media_server* _(){return &self;}

    //初始化
    static int init(MS_CFG &cfg);

    // 反初始化
    static int uninit();

    // 注册
    static int regist(const std::string ids, const std::string local_ip, unsigned short local_port,
        const std::string server_ip, unsigned short server_port);

    static int stop_regist(const char* sz_server_ip,unsigned short server_port);

    static int regist2(const std::string ids, const std::string server_ip, unsigned short server_port, unsigned int millisec);

    static int stop_regist2(const char* sz_server_ip,unsigned short server_port, unsigned int millisec);

    // 创建转发源
    static int create_src(int tracknum,				// 流数量
        int *trackids,
        char *tracknames[],
        int &srcno,					// 源id
        long chanid = -1);			// 指定通道

    // 删除转发源
    static int destroy_src(int srcno);

    // 设置系统头
    static int set_key_data(int srcno, char *keydata, long len, long datatype);

    // 数据发送_外面传入时戳
    static int send_frame_stamp(int srcno,				// 源id
        int trackid,			// trackid
        char *buff,				// 发送数据
        unsigned long len,		// 数据长度
        int frame_type,			// 帧类型
        long device_type,		// 设备类型
        uint32_t in_time_stamp,
		bool use_ssrc = false,
		uint32_t ssrc = 0);// 外部输入时戳							
	static int send_rtp_stamp(int srcno,				// 源id
		int trackid,			// trackid
		char *buff,				// 发送数据
		unsigned long len,		// 数据长度
		int frame_type,			// 帧类型
		long device_type,		// 设备类型
		uint32_t in_time_stamp,
		bool use_ssrc = false,
		uint32_t ssrc = 0);// 外部输入时戳	
    // 查找转发源对应通道号
    static int get_chanid(int srcno, int trackid, long &chanid);

    static int add_send(xmpp_cfg_t& cfg);

    //static int get_sever_info(SERVER_INFO& info,const long chid);

    //增加发一个发送
    static int add_send(int srcno,              // 源id
        int trackid,            //track id
        const char *ip,           // 目标ip
        unsigned short port,      // 目标port
        bool demux = false,       // 复用
        unsigned int demuxid = 0); // 复用id

    // 删除转发
    static int del_send(int srcno,              // 源id
        int trackid,             //track id
        const char *ip,          // 目标ip
        unsigned short port,      // 目标port
        bool demux = false,        // 复用
        unsigned int demuxid = 0);  // 复用id

    // 删除转发(srcno 源id)
    static int del_send_src(int srcno);

    // 删除所有转发
    static int del_send_all();

    static int get_svr_info(svr_info info[],int& tracknum,const int srcno);

    static int create_src_defult(int* srcno,char sdp[],int* sdp_len,const long chanid,const char* local_bind_ip);

    static int set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);
};
#endif//MEDIA_SERVER_H__

