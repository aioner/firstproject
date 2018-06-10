#ifndef XTSRC_H__INCLUDE_
#define XTSRC_H__INCLUDE_

#include <list>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "h_xtmediaserver.h"
#include "sdp.h"
#include "parse_buffer.h"
#include <boost/noncopyable.hpp>
class multi_code_mgr : boost::noncopyable
{
protected:
    multi_code_mgr():cb_(NULL){}
public:
    static multi_code_mgr* _() {return &self_;}
    void register_callback(multi_code_query_callback_t cb){cb_ = cb;}
    int multi_code_query(const int prime_srcno,const ms_code_t code,int* srcno)
    {
        if ( NULL == cb_) return -1;
        return cb_(prime_srcno,code,srcno);
    }
private:
    multi_code_query_callback_t cb_;
    static multi_code_mgr self_;
};

// ת����Ϣ
typedef struct __struct_xt_sink_type__ 
{
    void                *session;    // �Ự���
    int                 srcno_prime; // ����ͨ��
    unsigned long       chanid;      // ͨ��id
    std::string         addr;        // IP
    unsigned short      port;        // port
    bool                demux;       // �Ƿ�˿ڸ���
    unsigned int        demuxid;     // ����ID
} xt_sink_t,*ptr_xt_sink_t;

// Դ��Ϣ
struct xt_src
{
    int srcno; // Դid
    std::list<xt_track_t> tracks; // track��Ϣ
    std::list<xt_sink_t>  sinks; // ת����Ϣ
};

class XTSrc : boost::noncopyable
{
public:
    typedef boost::unique_lock<boost::shared_mutex> write_lock_t;
    typedef boost::shared_lock<boost::shared_mutex> read_lock_t;
public:
    static XTSrc* instance(){return &self_;}
    static XTSrc* _(){return &self_;}

    // ����ת��Դ(tracknum ������ chanid ָ��ת������ͨ��)
    int create_src(int tracknum, int *trackids, char *tracknames[], int &srcno, long chanid = -1);

    int create_src_sdp(int* srcno,xt_track_t* trackinfos,int* tracknum,const char* sdp,const long sdp_len,const long chanid);

    int create_src_defult(int* srcno,char sdp[],int* sdp_len,const long chanid,const std::string& local_bind_ip);

    // ����ת��Դ
    int destroy_src(int srcno);

    // ���ת��
    int clear_send(int srcno);

    // �������ת��
    int clear_allsend();

    // ���������
    int get_tracknum(int srcno);

    // �����id
    int get_trackids(int srcno, int *ids);

    // ���Դtrack
    int get_track(int srcno, int trackid, xt_track_t &track);

    // ���Դtrack(�����)
    int get_main_track(int srcno, xt_track_t &track);

    // ���tracks
    int get_tracks(int srcno, std::list<xt_track_t> &tracks);

    //��ȡת��ͨ��chanid
    int get_chanid(unsigned long& out_chanid,const int srcno,const char* trackname);

    // ����ת���ڵ�
    int add_sink(int srcno, xt_sink_t &sink);

    // ɾ��ת���ڵ�
    int del_sink(int srcno, xt_sink_t &sink);
    int del_sink(void *session);

    // ����ת���ڵ�
    int get_srcno_by_sink(int &srcno, const xt_sink_t &single_sink);

    // ����srcno
    int find_src(void *session, std::vector<int> &srcs);

    int find_src(unsigned long chanid);

    int set_std_track_frametype(const int srcno,const char* sdp,const long sdp_len);

private:
    int pare_tracks(const char* sdp,const int sdp_len,xt_track_t* track_info);
    int create_sdp(xt_track_t trackinfs[],const int tracknum,char sdp[],int& sdp_len,const std::string& local_bind_ip);
    int get_trackid(const std::string& tsr);
    boost::shared_mutex     m_mutex; // mutex
    std:: map<int, xt_src*>  m_srcs;  // ͨ����Ϣ
    static XTSrc self_;
};
#endif//end XTSRC_H__INCLUDE_
