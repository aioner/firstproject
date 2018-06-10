//
//create by songlei 20160316
//
#ifndef DPS_DATA_SEND_MGR_H__
#define DPS_DATA_SEND_MGR_H__
#include <string>
#include <stdint.h>
#include <boost/noncopyable.hpp>
#include "h_xtmediaserver.h"
#include "dps_ch_mgr.h"

typedef MS_CFG ms_cfg;

class dps_data_send_mgr : boost::noncopyable
{
public:
    typedef std::vector<connect_info_t> connect_info_container_t;
    typedef connect_info_container_t::iterator connect_info_container_itr_t;
private:
    dps_data_send_mgr():cfg_(){}
    static dps_data_send_mgr my_;
    ms_cfg cfg_;

public:
    static dps_data_send_mgr* _(){return &my_;}

    //����
    int start();

    //��ʼ��
    int init(ms_cfg &cfg);

    // ����ʼ��
    int uninit();

    srcno_t create_src_ex(src_info_t& src,bool is_auto_malloc_srno = true);

    int regist(const std::string ids, const std::string server_ip, unsigned short server_port, unsigned int millisec);

    int stop_regist(const char* sz_server_ip,unsigned short server_port, unsigned int millisec);

    // ����ת��Դ
    int create_src(int tracknum,  int *trackids, char *tracknames[], int &srcno, long chanid = -1);

    // ɾ��ת��Դ
    int destroy_src(int srcno);

    // ����ϵͳͷ
    int set_key_data(int srcno, char *keydata, long len, long datatype);

    // ���ݷ���_���洫��ʱ��
    int send_data_stamp(int srcno, int trackid, char *buff, unsigned long len, int frame_type, long device_type, uint32_t in_time_stamp);

    // ����ת��Դ��Ӧͨ����
    int get_chanid(int srcno, int trackid, long &chanid);

    int add_send(xmpp_cfg_t& cfg);

    //���ӷ�һ������
    int add_send(int srcno, int trackid, const char *ip, unsigned short port, bool demux = false, unsigned int demuxid = 0);

    // ɾ��ת��
    int del_send(int srcno, int trackid, const char *ip, unsigned short port, bool demux = false, unsigned int demuxid = 0);

    // ɾ��ת��(srcno Դid)
    int del_send_src(int srcno);

    // ɾ������ת��
    int del_send_all();

    int get_svr_info(svr_info info[],int& tracknum,const int srcno);

    int create_src_defult(int* srcno,char sdp[],int* sdp_len,const long chanid,const char* local_bind_ip);

    int set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);

    void const get_connect_info(connect_info_container_t& connect_infos)const;

    long ms_code_stream_type_cast(ms_code_t code) const;
};

#endif // #ifndef DPS_DATA_SEND_MGR_H__
