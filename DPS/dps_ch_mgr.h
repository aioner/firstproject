//
// create by songlei 20160316
// 设备接入的设备流与转发源是一对一对关系，即一个接入设备只能对应一个srcno，同理一个srcno也只能
// 对就一个设备流，dps_dev_stream_t 建立这种关系。dps_trans_src_mgr 分配释放转发源，dps_ch_mgr
// 使用转发通道的概念对所有资源进行管理。通道是客户端点播DPS时的代理设备标识。
//
#ifndef DPS_CH_MGR_H__
#define DPS_CH_MGR_H__
#include <string>
#include <stdint.h>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include "dps_device_access_mgr.h"
#include "dps_common_type_def.h"

typedef int (__stdcall *send_data_cb_t)(int srcno, int trackid, unsigned char *buff, unsigned long len, int frame_type, long device_type, uint32_t in_time_stamp);

//sdp 中的track 信息 
typedef struct __struct_track_ctx_type
{
    __struct_track_ctx_type():track_id(-1),track_type(MEDIA_TYPE_NA)
    {
        std::memset(track_name,0,DPS_MAX_TRACK_NAME_LEN);
    }

    __struct_track_ctx_type(const __struct_track_ctx_type& rf)
    {
        *this = rf;
    }
    __struct_track_ctx_type& operator=(const __struct_track_ctx_type& rf)
    {
        this->track_id = rf.track_id;
        this->track_type = rf.track_type;
        std::strncpy(this->track_name,rf.track_name,DPS_MAX_TRACK_NAME_LEN);
        return *this;
    }

    bool operator==(const __struct_track_ctx_type &rf) const
    {
        return (this->track_id == rf.track_id 
            && this->track_type == this->track_type
            && 0 == std::strcmp(this->track_name,this->track_name));
    }

    track_id_t track_id;
    media_type_t track_type;
    char track_name[DPS_MAX_TRACK_NAME_LEN];
}track_ctx_t,*ptr_track_ctx_t;

class src_info_t 
{
public:
    typedef std::vector<track_ctx_t> track_infos_container_t;
    typedef track_infos_container_t::iterator track_infos_container_itr_t;
    typedef track_infos_container_t::const_iterator track_infos_container_const_itr_t;
    typedef boost::shared_lock<boost::shared_mutex> read_lock_t;
    typedef boost::unique_lock<boost::shared_mutex> write_lock_t;
public:
    src_info_t(const char* sdp,const long sdp_len,const long data_type,const dps_ch_t transmit_ch)
        :usr_(0),srcno_(SRCNO_NA),transmit_ch_(transmit_ch)
    {
        set_sdp(sdp,sdp_len,data_type);
    }

    src_info_t():usr_(0),srcno_(SRCNO_NA),track_num_(-1),sdp_len_(-1),data_type_(-1)
    {
        std::memset(sdp_,0,DPS_MAX_SDP_LEN);
    }

    src_info_t(const src_info_t& rf)
    {
        *this = rf;
    }
    src_info_t& operator=(const src_info_t& rf)
    {
        uint16_t tmp = rf.usr_;
        this->usr_ = tmp;
        this->transmit_ch_ = rf.transmit_ch_;
        this->srcno_ = rf.srcno_;
        std::memcpy(this->sdp_,rf.sdp_,DPS_MAX_SDP_LEN);
        this->sdp_len_ = rf.sdp_len_;
        this->track_num_ = rf.track_num_;
        this->track_infos_ = rf.track_infos_;
        return *this;
    }
    bool operator==(const src_info_t& rf) const
    {
        return (this->srcno_ == rf.srcno_ && 0== std::strcmp(this->sdp_,rf.sdp_) 
            && this->sdp_len_ == rf.sdp_len_ && this->track_num_ == rf.track_num_);
    }

    void assign();
    void realse();

    track_id_t parse_track_id(const media_type_t& meida_type)
    {
        read_lock_t lock(mutex_track_infos_);
        track_id_t trackid = -1;
        for(track_infos_container_itr_t itr_track = track_infos_.begin(); track_infos_.end() != itr_track; ++itr_track)
        {
            if (meida_type == itr_track->track_type)
            {
                trackid = itr_track->track_id;
                break;
            }
        }
        return trackid;
    }
    void set_sdp(const char* sdp,const long sdp_len,const long data_type);

private:
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
public:
    boost::atomic<uint16_t> usr_;
    dps_ch_t transmit_ch_;
    srcno_t srcno_;
    char sdp_[DPS_MAX_SDP_LEN];
    long sdp_len_;
    int track_num_;
    long data_type_;

    boost::shared_mutex mutex_track_infos_;
    track_infos_container_t track_infos_;
};

typedef src_info_t* t_src_info_handle_t;//转发源信息句柄
#define T_SRC_INFO_HANDLE_NA NULL

class dps_trans_src_mgr : boost::noncopyable
{
public:
    typedef std::vector<t_src_info_handle_t> t_src_info_handle_container_t;
    typedef t_src_info_handle_container_t::iterator t_src_info_handle_container_t_itr_t;
    typedef t_src_info_handle_container_t::const_iterator t_src_info_handle_container_t_container_const_itr_t;
public:
    static dps_trans_src_mgr* _(){return &my_;}
    t_src_info_handle_t malloc(const src_info_t& src);
    void free(const t_src_info_handle_t ts_handle);

private:
    static dps_trans_src_mgr my_;
    boost::detail::spinlock mutex_trans_src_handles_;
    t_src_info_handle_container_t trans_src_handles_;
};

typedef enum
{
    STATE_IDLE = 0,
    STATE_OPEN,
    STATE_CLOSE
}ch_state_t;

class dps_dev_stream_t
{
public:
    typedef boost::shared_lock<boost::shared_mutex> read_lock_t;
    typedef boost::unique_lock<boost::shared_mutex> write_lock_t;
public:
    dps_dev_stream_t()
        :device_(),dev_handle_(DEV_HANDLE_NA),state_(STATE_IDLE),
        send_data_cb_(NULL),create_time_(),recv_frame_nums_(0),
        is_recv_data_(false),trans_src_handle_(T_SRC_INFO_HANDLE_NA)
    {std::memset(sdp_,0,DPS_MAX_SDP_LEN); }
    ~dps_dev_stream_t(){}
public:
    void* context() { return static_cast<void*>(this); }

    bool is_open() { return STATE_OPEN == state_; }
    bool is_close() { return STATE_CLOSE == state_; }
    bool is_free() { return(STATE_CLOSE == state_ || STATE_IDLE == state_); }
    void set_state(const ch_state_t& state) { state_ = state; }
    void set_device(const device_t& device)
    {
        create_time_ = boost::posix_time::microsec_clock::local_time();
        device_ = device;
    }

    const device_t& get_device() const { return device_; }
    const dev_handle_t& dev_handle() const { return dev_handle_; }
    bool src_is_create();
    void set_dev_handle(const dev_handle_t& hanle);
    void set_sdp(const char* sdp) { std::strncpy(sdp_,sdp,DPS_MAX_SDP_LEN); }
    void set_send_data_cb(const send_data_cb_t cb) { send_data_cb_ = cb; }
    int add_src_info(const src_info_t& src_info);
    void clear_src_info();
    void stop_capture();
    void update_recv_frame_nums()
    {
        if (recv_frame_nums_ < (0xffffffffffffffff)/*_UI64_MAX*/) ++recv_frame_nums_;
        else recv_frame_nums_ = 0;
    }
    void recv_data_refurbish(const bool state=true) { is_recv_data_ = state; }
    bool get_recv_data_refurbish_state() const { return is_recv_data_; }
    std::string get_create_time() const;
    t_src_info_handle_t& get_src();
    uint64_t get_recv_frame_nums(){return recv_frame_nums_;}
    media_type_t parse_media_type(const unsigned int frame_type);
    void send_media_data(unsigned char* data,const long len, const long frame_type,const long data_type,const long time_stamp,const unsigned long ssrc);
    const char* get_sdp()const { return sdp_; }
private:
    boost::posix_time::ptime create_time_;
    boost::atomic<ch_state_t>state_;
    boost::atomic<uint64_t> recv_frame_nums_;
    boost::atomic_bool is_recv_data_;
    dev_handle_t dev_handle_;
    device_t device_;
    char sdp_[DPS_MAX_SDP_LEN];
    boost::atomic<send_data_cb_t> send_data_cb_;
    boost::shared_mutex mutex_trans_src_handle_;
    t_src_info_handle_t trans_src_handle_;
};

typedef dps_dev_stream_t* dps_dev_s_handle_t;
#define DPS_DEV_S_HANDLE_NA NULL

typedef void (__stdcall * play_cb_t)(const dps_dev_s_handle_t s_handle);
typedef void (__stdcall * stop_cb_t)(const dps_dev_s_handle_t s_handle);
typedef struct __struct_dps_channle_info_type
{
    __struct_dps_channle_info_type():ch(DPS_CH_NA),s_handle(DPS_DEV_S_HANDLE_NA){}
    dps_ch_t ch;
    dps_dev_s_handle_t s_handle;
}dps_ch_info_t,*ptr_dps_ch_info_t;

class dps_ch_mgr : boost::noncopyable
{
public:
    typedef std::vector<dps_dev_s_handle_t> dps_dev_s_handle_container_t;
    typedef dps_dev_s_handle_container_t::iterator dps_dev_s_handle_container_itr_t;
    typedef std::vector<dps_ch_info_t> dps_ch_infos_container_t;
    typedef dps_ch_infos_container_t::iterator dps_ch_infos_container_itr_t;
public:
    static dps_ch_mgr* _(){return &my_;}
public:
    int init();
    void uninit();
    void get_s_handle_by_ch(const dps_ch_t ch,dps_dev_s_handle_container_t& s_handles);
    srcno_t get_srcno_by_ch_and_streamtype(const dps_ch_t ch,const long stream_type);
    void get_all_s_handle(dps_dev_s_handle_container_t& s_handles);
    void play_all_ch(play_cb_t cb);
    void stop_all_ch(stop_cb_t cb);
    void stop_ch(const dps_ch_t ch,stop_cb_t cb);

private:
    static dps_ch_mgr my_;
    boost::detail::spinlock mutex_dps_ch_infos_;
    dps_ch_infos_container_t dps_ch_infos_;
};

#endif // #ifndef DPS_CH_MGR_H__
