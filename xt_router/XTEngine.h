#ifndef XT_ENGINE_H__
#define XT_ENGINE_H__

#include <stdint.h>
#include <string>
#include <list>
#include <algorithm>
#include <boost/noncopyable.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/threadpool.hpp>
#include "media_server.h"
#include "media_device.h"
#include "InfoTypeDef.h"
#include "gw_join_sip_session_mgr.h"

#include "sdp.h"
#include "parse_buffer.h"

#ifdef _WIN32
#define XT_CALLBACK __stdcall
#else
#define XT_CALLBACK
#endif

#define MAX_FRAME_SIZE   1024*1024	//帧最大长度
#define MAX_KEY_SIZE     4096	//系统头长度
// 设备信息
#define MAX_TRACKNAME_LEN 64

//sdp 中的track 信息 
struct track_ctx_t
{
    int trackId;        //数据ID
    int trackType;      //数据类型。0：视频；1：音频；－1：其他
    char trackname[MAX_TRACKNAME_LEN];

    track_ctx_t():trackId(-1),trackType(-1)
    {
        ::memset(trackname,0,MAX_TRACKNAME_LEN);
    }
    track_ctx_t& operator=(const track_ctx_t& other)
    {
        this->trackId = other.trackId;
        this->trackType = other.trackType;
        ::strncpy(this->trackname,other.trackname,MAX_TRACKNAME_LEN);
        return *this;
    }
};

struct device_info  
{
    std::string      dev_ids;             // 设备IDS
    std::string      db_url;            // 点播url
    long            db_type;            // 点播类型
    long            dev_chanid;          // 设备通道号
    long            dev_strmtype;        // 设备码流类型

    long            db_chanid;          // 点播通道
    long            link_type;          // 连接类型
    long            db_port;              // 点播端口

    dev_handle_t    dev_handle;          // 设备句柄
    long            strmid;            // 流标识
    long           transmit_ch;        //外部指定的转发服务通道号（私有协议）

    int             tracknum;          // 流数量
    char            key[MAX_KEY_SIZE];  // 系统头
    long            key_len;           // 系统头长度

    typedef std::vector<track_ctx_t> track_info_container_t;
    typedef track_info_container_t::iterator track_info_container_handle_t;
    std::vector<track_ctx_t> track_infos;//接入流信息

    device_info()
        :dev_handle(-1),
        dev_ids(),
        dev_chanid(-1),
        dev_strmtype(-1),
        db_type(-1),
        db_url(),
        db_chanid(-1),
        tracknum(-1),
        db_port(-1),
        link_type(-1),
        key_len(-1),
        strmid(-1),
        transmit_ch(-1),
        track_infos(MAX_TRACK)
    {
        std::fill_n(key, MAX_KEY_SIZE, 0);
    }

    void reset()
    {
        dev_handle = -1;
        dev_ids.clear();
        dev_chanid = -1;
        dev_strmtype = -1;
        db_type = -1;
        db_url.clear();
        db_chanid = -1;
        tracknum = -1;
        db_port = -1;
        link_type = -1;
        key_len = -1;
        strmid = -1;
        transmit_ch=-1;
        std::fill_n(key, MAX_KEY_SIZE, 0);
        track_infos.clear();
    }

    typedef device_info my_t;
    my_t &operator=(const my_t& other)
    {
        this->dev_handle = other.dev_handle;
        this->dev_ids = other.dev_ids;
        this->dev_chanid = other.dev_chanid;
        this->dev_strmtype = other.dev_strmtype;
        this->db_type = other.db_type;
        this->db_url = other.db_url;
        this->db_chanid = other.db_chanid;
        this->strmid = other.strmid;
        this->tracknum = other.tracknum;
        this->db_port = other.db_port;
        this->transmit_ch = other.transmit_ch;
        this->link_type = other.link_type;
        this->key_len = other.key_len;
        ::strncpy(this->key,other.key,MAX_KEY_SIZE);
        this->track_infos = other.track_infos;
        return *this;
    }
};

// 转发源
struct src_info
{
    int							srcno;			//转发源标识
    bool						active;			//激活
    device_info					device;			//设备信息
    bool						regist;			//中心注册
    unsigned long				frames;			//接收帧数
    boost::posix_time::ptime    create_time;	//创建时间

    src_info()
        :srcno(-1),
        active(false),
        device(),
        regist(false),
        frames(0),
        create_time()
    {}

    void reset()
    {
        srcno=-1;
        active = false;
        device.reset();
        regist = false;
        frames = 0;
    }

    typedef src_info my_t;
    my_t &operator=(const my_t& other)
    {
        this->srcno = other.srcno;
        this->active = other.active;
        this->device = other.device;
        this->regist = other.regist;
        this->frames = other.frames;
        this->create_time = other.create_time;
        return *this;
    }
};

// 流id
struct strmid_info
{
    long strmid;
    bool use;
};

#include "../tghelper/recycle_pool.h"
#include "../tghelper/recycle_pools.h"
#include "../tghelper/byte_pool.h"
#include "../rv_adapter/rv_def.h"

typedef struct _XTFrameInfo
{
	unsigned int verify;
	unsigned int frametype;
	unsigned int datatype;
}XTFrameInfo;
//rtp数据块，
class rtp_block : public tghelper::byte_block
{
public:
	rtp_block(const uint32_t block_size) : tghelper::byte_block(block_size), m_bind_block(0), m_resend(false)
	{		}
	rtp_block() : tghelper::byte_block(2048), m_bind_block(0), m_resend(false)
	{		}
protected:
	virtual void recycle_alloc_event()
	{
		m_bFrameInfo = false;
		memset(&m_infoFrame, 0, sizeof(XTFrameInfo));
		memset(&m_rtp_param, 0, sizeof(rv_rtp_param));
		m_priority = 0;
		m_resend = false;
		m_use_ssrc = false;
	}
	virtual void recycle_release_event()
	{
		if (m_bind_block)
		{
			m_bind_block->release();
			m_bind_block = 0;
		}
	}

public:
	void set_rtp_param(rv_rtp_param *rtp_param)
	{ memcpy(&m_rtp_param, rtp_param, sizeof(rv_rtp_param)); }

	void ser_rtp_prority(uint8_t prority)
	{
		m_priority = prority;
	}
	void set_bind_block(byte_block *bind_block)
	{
		if (m_bind_block)
		{
			m_bind_block->release();
		}
		m_bind_block = bind_block;
		if (m_bind_block)
		{
			m_bind_block->assign();
		}
	}
	inline byte_block *get_bind_block() { return m_bind_block; }
public:
	rv_rtp_param m_rtp_param;

	bool m_use_ssrc;

	uint32_t m_ssrc;

	bool m_bFrameInfo;
	XTFrameInfo m_infoFrame;

	uint8_t m_priority;

	bool m_resend;

	uint32_t m_exHead[16];
private:
	byte_block * m_bind_block;		//外挂用户block
};

namespace inner
{
	typedef tghelper::any_byte_pool<
		rtp_block,
		2048,
		10,
		10> rtp_pool;
}

class XTEngine: boost::noncopyable
{
private:
    XTEngine(void);
    ~XTEngine(void);

    static XTEngine self;

public:
    static XTEngine* instance(){return &self;}
    static XTEngine* _(){return &self;}

    //回调函数
    ///////////////////////////////////////////////////
    //RTSP流化控制
    static int MEDIASERVER_STDCALL ctrl_rtsp_play_cb(int srcno, int trackid,long chid, double npt,float scale, uint32_t *rtp_pkt_seq, uint32_t *rtp_pkt_timestamp);

    static int MEDIASERVER_STDCALL ctrl_rtsp_pause_cb(int srcno, int trackid,long chid);

    //TCP流化控制
    static int MEDIASERVER_STDCALL ctrl_tcp_play_cb(int srcno,long chid,double npt,float scale,uint32_t *rtp_pkt_timestamp);

    static int MEDIASERVER_STDCALL ctrl_tcp_pause_cb(int srcno,long chid);

    //数据抛出回调
    static long XT_CALLBACK data_out_cb(long handle, unsigned char* data, long len, long frame_type,long data_type,void* user_data, long time_stamp, unsigned long nSSRC);
	static boost::threadpool::pool *	m_tp;
	static inner::rtp_pool m_rtp_pool; //内存池，管理由拷贝复制产生的rtp_block
	static void post_one_frame(int strmid,int frame_type,int media_type,int data_type, void *data,int len);
    //media_sever.dll日起输出回调
    static void MEDIASERVER_STDCALL media_server_log_cb(char* logname,log_level_type level,char* log_ctx,uint32_t log_ctx_len);

    //rtcp强制I帧回调
    static void MEDIASERVER_STDCALL rtcp_force_iframe_cb (const int srcno);
    ////////////////////////////////////////////////////

public:
    // 启动
    int start(unsigned long num_chan,			// 通道数
        const std::string& ip,			// 发送ip
        unsigned short start_port,		// 起始端口
        bool multiplex,					// 复用
        const std::string& mul_start_ip,// 组播起始地址
        unsigned short msg_liten_port,	// 私有监听端口
        unsigned short rtsp_listen_port,// rtsp监听端口
        unsigned short tcp_listen_port,	// tcp传输监听端口
        unsigned short udp_listen_port, // udp侦听端口
        unsigned short regist_listen_port,// tcp反向注册监听端口
        bool snd_std_rtp = false);		//发送标准流

    // 停止
    int stop();

	// 检测重复点播
	int is_exist_src(const std::string &ids,const int dev_chid,const int dev_strmtype);

	//获得登陆信息
	int  get_login_info(const std::string &dev_ids, long db_type, std::string &name, std::string &pwd, long &port);

    // 开启点播
    int start_play(const std::string& on_db_sn,
        const std::string& dev_ids,           // 设备IDS	
        long dev_chanid,                               // 设备通道号
        long dev_strmtype,                           // 设备码流类型
		const std::string &localip,
        const std::string& db_url,                    // 点播IP 
        long db_chanid,                             // 点播通道	
        long db_type,                              // 设备类型
        long &transmit_ch,                          // 指定转发服务通道号
        long link_type,                             // 连接类型
        const std::string login_name = "admin",        // 登录设备用户名
        const std::string login_password ="12345",     // 登录设备密码
        const long login_port = -1);                // 登录设备端口

    // 开启点播
    int start_play_v1(const std::string& on_db_sn,const std::string& dev_ids,const long dev_chanid,const long dev_strmtype,const std::string& db_url,
        const long db_chanid,const long db_type,const long transmit_ch,long link_type,const std::string& login_name = "admin",const std::string& login_password ="12345",const long login_port = -1);

    // 停止点播
    int stop_play(const std::string& dev_ids,  // 设备IDS	
        long dev_chanid,   // 设备通道号
        long dev_strmtype);    // 设备码流类型
    
    //由转发源标志停点
    int stop_play_by_srcno(const int srcno);
    
    //由IDS停点
    int stop_play_ids(const std::string& dev_ids);

    //处理设备下线
    int pro_dev_logout(const std::string& dev_ids);

    //由ip停点
    int stop_play_ip(const std::string& strip);

    //由通道号停点
    int stop_play_trans_chid(int srcno);

    //由流ID停点对应转发流
    int stop_play_stramid(const long lstramid);

    // 停止所有点播
    int stop_allplay();

    // IDS更新
    int update_ids(const std::string& dev_ids,			// 设备IDS	
        long dev_chanid,		// 设备通道号
        long dev_strmtype,		// 设备码流类型
        const std::string& dev_ids_new,		// 设备IDS	
        long dev_chid_new);		// 设备通道号

    // userid管理
    //////////////////////////////////////////////////////////////
    long init_strmid(long num);
    long uninit_strmid();
    long get_free_strmid();
    void free_strmid(long userid);
    //////////////////////////////////////////////////////////////

    // src管理
    ////////////////////////////////////////////////////////////////
    int init_src(int num);
    int uninit_src();
    int active_src(int srcno, const src_info &info,const bool active_state=true);
    bool get_src_status(const int srcno);
    int update_src(const std::string& dev_ids,long dev_chanid,long dev_strmtype,const std::string& dev_ids_new,long dev_chid_new);
    int upate_src(const src_info &new_src_info);
    int upate_src(const int srcno,const src_info& new_src);
    int update_strmid_of_src(const int srcno,const long stramid);
    int update_dev_handle_of_src(const int srcno,dev_handle_t handle);
    int update_access_info_of_src(const int srcno,const long strmid,const dev_handle_t h);
    int get_src_ids(const std::string& dev_ids,long dev_chanid,long dev_strmtype,src_info &info);
    int get_src_no(int srcno,src_info &info);
	int mod_src_ids(int srcno,std::string ids);
    int get_src_ids2(const std::string& dev_ids,std::vector<src_info> &srcs);
    int get_src_strmid(long strmid,std::vector<src_info> &srcs);
    int get_src_url(const std::string &db_url,std::vector<src_info> &srcs);
    int get_src_all(std::vector<src_info> &srcs);
    int free_src(int srcno);
    int inactive_src(int srcno);
    dev_handle_t get_dev_link_handle_src(const int srcno);
    bool is_active_src(const int srcno);

    // 增加/删除转发源
    int create_src(device_info &device,int &srcno,long chanid = -1);// long chanid = -1指定通道
    int create_src_v1(device_info &device,int &srcno);
    int destroy_src(int srcno);
    int destroy_src(const std::string& dev_ids,const long dev_chanid,const long dev_strmtype);

    // 中心登记
    int regist_src(int srcno,const std::string& ondb_sn);
    ////////////////////////////////////////////////////////////////

    void get_media_type(int& media_type,const unsigned int frame_type);

    void upate_frame_state(const int srcno);
    // 保存系统头
    long get_sdp_size(const int srcno);
    int update_sdp(const int srcno,char *key, long len,long data_type);
    long save_sdp_to_srv(const int srcno,std::string& recv_sdp);
    long save_sdp_to_srv(const int srcno,const char* sdp,const long sdp_len,const long data_type);
    int get_key(long strmid, char *key, long &len);
    long save_sdp_srcno_to_srv_and_access(const int srcno,const long recv_dev_handle,const std::string& sdp);
    long save_sdp_to_access(const dev_handle_t recv_dev_handle,const char* recv_sdp,const long sdp_len);

    //流管理 
    int get_trackid(const int srcno,const int media_type);

    // 查找转发源对应通道号
    int get_chanid(int srcno, int trackid, long &chanid);

    int get_main_chanid(int srcno, long &chanid) { return get_chanid(srcno, -1, chanid); }

    //获取转发服务通道
    int get_chanid_by_srcno(std::list<long>& lst_chanid,const src_info& src);

    //获取最大支持转发通道
    unsigned long get_max_trans_ch();

    void get_all_src(std::list<src_info>& lst_src);
    long get_dev_stremtype_by_srcno(const int srcno);

	void get_all_src_1(std::list<src_info>& lst_src); //added by wluo
    //sip 对接部分
public:
    //创建转发通道生成双向转发通道
    long sip_2_sip_create_free_transmit_channel(const std::string& recvids,const std::string& sendids,
        const long transmit_ch,const long dev_chanid, const long dev_strmtype,const bool dumx_flags,std::string& sdp_two_way,int& out_srcno,dev_handle_t& recv_dev_handle);

    //创建接收 直接操作RTP接收库
    dev_handle_t rtp_create_recv(const std::string& dev_ids, long dev_chanid, long dev_strmtype,int track_num,bool demux);
    long rtp_get_rcv_inf(dev_handle_t link_handle, _RCVINFO *infos, int &num);
    long rtp_close_recv(const dev_handle_t dev_handle);
    long rtp_close_recv(const std::string& dev_ids, const long dev_chanid, const long dev_strmtype);

    //关闭接收
    long clear_recv_link_mgr_buf(const std::string& dev_ids, long dev_chanid, long dev_strmtype);
    void clear_recv_link_mgr_buf(const long dev_handle);

    //数据捕获
    long start_link_capture(const dev_handle_t link_handle,long strmid);

    //保存sdp
    int save_sdp_to_access(const std::string& dev_ids, long dev_chanid, long dev_strmtype, const char *sdp, long len_sdp);

    //sip对接操作接口
    long sip_create_r(const std::string& dev_ids, const long dev_chanid,const long dev_strmtype,const bool demux,std::string& sdp,dev_handle_t& out_dev_handle);

    //生成发交换sdp
    long sip_create_sdp_r(long link_handle,const long dev_strmtype,int& track_num,std::string&sdp);
    long create_sdp_s(const int srcno,std::string &sdp);
    long sip_create_sdp_s(const std::string& dev_ids, const long dev_chanid,const long dev_strmtype,std::string& sdp,int&srcno,dev_handle_t& dev_handle);
    long sip_create_sdp_s(const int srcno,const long dev_strmtype,const char* in_sdp, const int in_sdp_len,std::string &out_sdp);

    //处理sdp
    int sip_transform_call_sdp_ex(std::string& recv_sdp,std::string& send_sdp,const std::string& call_sdp);

    long gw_join_sip_2_sip_call_switch_set_data(const session_inf_t& ssession_1,std::string& send_sdp);
    long gw_join_sip_close_r(const long sessionid);

    //增加交换发送
    long gw_join_sip_create_s(const std::string& dev_ids, const long dev_chanid,const long dev_strmtype,std::string& sdp,int&srcno,dev_handle_t& dev_handle);
    long gw_join_sip_close_s(const long sessionid);
    long gw_join_sip_clear_trans_ch(const long sessionid);

    //打开接收与发送
    long gw_join_sip_create_rs(const std::string& dev_ids_r,const long dev_chid_r,const std::string& dev_ids_s,
        const long dev_chid_s,const long dev_strmtype,std::string& sdp,const bool demux_r=false);

    //推送交换流
    long gw_join_sip_add_send(const long sessionid);

    //保存SDP
    int gw_join_sip_save_sdp_rs(const long sessionid,const char *sdp, long len_sdp);
    int gw_join_sip_transform_call_sdp(std::string& out_sdp,const std::string& call_sdp,const std::string& condition = "recvonly");
    int gw_join_sip_save_sdp_to_access(const std::string& dev_ids, long dev_chanid, long dev_strmtype, const char *sdp, long len_sdp);

    //sip 2 sip
    //创建转发机制
    int gw_join_sip_create_transmit_machine_made(const std::string& dev_ids,const long dev_ch,const long dev_strmtype,
         const long transmit_ch,const dev_handle_t dev_handle,const long strmid);
public:
    void del_m_of_sdp(const char* in_sdp,const int in_sdp_len,const std::string& m_name,std::string& out_sdp);
    long create_sdp_s_impl(const int srcno,const long dev_strmtype,const char* in_sdp, const int in_sdp_len,std::string &out_sdp);
    //转换media_type
    void transform_media_type_to_name(const int media_type,std::string& trackname)const;

    // 开启设备点播
    int start_capture(device_info &device,	            // 返回设备信息
        const std::string& dev_ids,         // 设备IDS	
        long dev_chid,                      // 设备通道号
        long dev_strmtype,                  // 设备码流类型
        int	db_type,                        // 点播类型
		const std::string &localip,
        const std::string& db_url,          // 点播url
        long db_chanid,                     // 点播通道
        int port = 8000,                    // 点播端口
        const std::string& szUser = "",     // 点播登陆名
        const std::string& szPassword = "", // 点播登陆密码
        int link_type = 0,
		void *bc_mp = NULL);// 点播类型

    int start_capture_v1(dev_handle_t& out_handle,long& out_strmid,const std::string& dev_ids,const long dev_chid,const long dev_strmtype,const int db_type,
        const std::string& db_url,const long db_chanid,const int link_type,long port/* = 8000*/,const std::string& login_name /*= ""*/, const std::string& login_pwd/* = ""*/);

    // 停止设备点播
    int	stop_capture(const device_info &device);

    //停止转发与点播
    int stop_play_trans(const src_info& src);

    // xt内部点播类型
    bool is_xttype(long type);
    void set_xttype(const std::string& types);

    // 复制转发
    static long copy_send(long strmid, unsigned char* data, unsigned int len,unsigned int frame_type,int data_type, unsigned int time_stamp);

    //根据码流类型获取流数量
    int get_tracknum(const long stream_type) const;

    int parse_tracks_ex(const char* sdp,const int sdp_len,std::vector<track_ctx_t>& track_infos);
    int parse_tracks(const char* sdp,const int sdp_len,track_ctx_t* track_info);

    void construct_video_for_sdp_default(xt_sdp::sdp_session_t::medium_t& m);
    void construct_audio_for_sdp_default(xt_sdp::sdp_session_t::medium_t& m);

    inline int find_trackid(const char *str, size_t len)
    {
        const char *t = str + len - 1;
        while (t >= str)
        {
            if (!isdigit(*t))
            {
                break;
            }
            t--;
        }
        return ((len - 1) == (t - str)) ? -1 : atoi(t + 1);
    }

    inline int find_trackid(const std::string& str)
    {
        return find_trackid(str.c_str(), str.length());
    }

    const MS_CFG& cfg()const
    {
        return m_msCfg;
    }
    const int& copy_send_num() const{return m_nCopy;}
private:
    long sip_create_sdp_r_impl_v1(long link_handle,const long dev_strmtype,int& track_num,std::string&sdp);
    long sip_create_sdp_r_impl_v2(long link_handle,const long dev_strmtype,int& track_num,std::string&sdp);
private:
    boost::shared_mutex            m_global_mutex;      // mutex(全局)
    MS_CFG                         m_msCfg;       // 转发服务配置
    int                            m_nCopy;        // 模拟DRV模式
    std::map<long, long>            m_xtdbtypes;    // xt内部点播类型(使用配置linktype)

private:
    boost::shared_mutex           m_mSrc;       // mutex(src)-m_srcs 	
    src_info                       *m_srcs;      // 转发源

    boost::shared_mutex           strmid_mutex_;     // mutex(strmid)
    strmid_info                    *m_strmids;     // 流ids(数据回调使用)
};
#endif//XT_ENGINE_H__
