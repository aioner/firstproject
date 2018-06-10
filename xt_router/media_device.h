#ifndef MEDIA_DEVICE_H_INCLUDE__
#define MEDIA_DEVICE_H_INCLUDE__
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include "common_type.h"
#include "MediaDevInterface.h"
typedef POUTPUTREALDATA access_data_output_cb_t;

typedef struct _device_link
{
    dev_handle_t link;
    std::string ids;
    long dev_chanid;
    long dev_strmtype;
    bool active;
    long strmid;
}device_link_t;

class media_device: boost::noncopyable
{
private:
    media_device(void);
    ~media_device(void);

    static media_device self;

public:
    static media_device* instance(){return &self;}
    static media_device* _(){return &self;}

    //added by lichao, 20150408 增加初始化 在main中调用
    static long init();
    static void term();

    // 开启点播
    static long  start_capture(int device_type, const char* localip,char* url, long channel, int media_type, void* user_data,access_data_output_cb_t pfnDataCB, int port = 8000, char* szUser = "", char* szPassword = "",int link_type = 0,void *bc_mp=NULL);

    // 停止点播
    static int stop_capture(dev_handle_t handle);

    //设置注册消息回调
    static long set_regist_callback(regist_call_back_t func);
    //获取SDP
    //static long get_sdp_by_handle(dev_handle_t oper_handle,const char* sdp,long& length);
    static long get_sdp_by_handle(dev_handle_t oper_handle,const char* sdp,long& length,long& data_type);
    static long get_data_type_by_handle(dev_handle_t oper_handle);

    static int tcp_play_ctrl(const long oper_handle,double npt,float scale,uint32_t *rtp_pkt_timestamp);
    static int tcp_pause_ctrl(const long oper_handle);
    static int rtsp_play_ctrl(const long oper_handle,double npt,float scale,uint32_t *rtp_pkt_timestamp);
    static int rtsp_pause_ctrl(const long oper_handle);

    //上层会话协商
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static long rtp_create_recv2(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports);
    static long rtp_create_recv(int track_num, bool demux);
    static long rtp_get_rcvinfo(long link, _RCVINFO *infos, int &num);
    static long rtp_save_sdp(long link, const char *sdp, unsigned int len_sdp); 
    static long start_link_captuer(const long link_handel,access_data_output_cb_t data_out_cb,long strmid);
    static long request_iframe(long link);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//接入句柄管理 上层会话协商过程
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    long add_link(dev_handle_t link, std::string ids, long dev_chanid, long dev_strmtype);
    dev_handle_t find_link(const std::string &ids,long dev_chanid,long dev_strmtype);
    long find_link(const dev_handle_t dev_handle,device_link_t& dev_link_inf);
	long mod_ids_by_handle(const dev_handle_t dev_handle,const std::string ids);
    long find_link(const std::string &ids,long dev_chanid,long dev_strmtype,device_link_t& dev_link_inf);
    long del_link(const std::string &ids,long dev_chanid, long dev_strmtype);
    void del_link_by_ids(const std::string &ids);
    void del_link_by_handle(const dev_handle_t link);
    dev_handle_t active_link(const dev_handle_t link,const long strmid=-1,const bool flg=false);
    dev_handle_t link_active_state(const dev_handle_t link,bool& link_state);
    long get_strmid_by_dev_handle(const dev_handle_t link);
    bool is_md_handle(const dev_handle_t link);
    void get_handle_all(std::vector<_device_link>& handle);
private:
    boost::detail::spinlock m_lock;
    std::vector<_device_link> m_vlink;
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
};
#endif//MEDIA_DEVICE_H__

