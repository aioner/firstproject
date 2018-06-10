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

    //��ʼ��
    static int init(MS_CFG &cfg);

    // ����ʼ��
    static int uninit();

    // ע��
    static int regist(const std::string ids, const std::string local_ip, unsigned short local_port,
        const std::string server_ip, unsigned short server_port);

    static int stop_regist(const char* sz_server_ip,unsigned short server_port);

    static int regist2(const std::string ids, const std::string server_ip, unsigned short server_port, unsigned int millisec);

    static int stop_regist2(const char* sz_server_ip,unsigned short server_port, unsigned int millisec);

    // ����ת��Դ
    static int create_src(int tracknum,				// ������
        int *trackids,
        char *tracknames[],
        int &srcno,					// Դid
        long chanid = -1);			// ָ��ͨ��

    // ɾ��ת��Դ
    static int destroy_src(int srcno);

    // ����ϵͳͷ
    static int set_key_data(int srcno, char *keydata, long len, long datatype);

    // ���ݷ���_���洫��ʱ��
    static int send_frame_stamp(int srcno,				// Դid
        int trackid,			// trackid
        char *buff,				// ��������
        unsigned long len,		// ���ݳ���
        int frame_type,			// ֡����
        long device_type,		// �豸����
        uint32_t in_time_stamp,
		bool use_ssrc = false,
		uint32_t ssrc = 0);// �ⲿ����ʱ��							
	static int send_rtp_stamp(int srcno,				// Դid
		int trackid,			// trackid
		char *buff,				// ��������
		unsigned long len,		// ���ݳ���
		int frame_type,			// ֡����
		long device_type,		// �豸����
		uint32_t in_time_stamp,
		bool use_ssrc = false,
		uint32_t ssrc = 0);// �ⲿ����ʱ��	
    // ����ת��Դ��Ӧͨ����
    static int get_chanid(int srcno, int trackid, long &chanid);

    static int add_send(xmpp_cfg_t& cfg);

    //static int get_sever_info(SERVER_INFO& info,const long chid);

    //���ӷ�һ������
    static int add_send(int srcno,              // Դid
        int trackid,            //track id
        const char *ip,           // Ŀ��ip
        unsigned short port,      // Ŀ��port
        bool demux = false,       // ����
        unsigned int demuxid = 0); // ����id

    // ɾ��ת��
    static int del_send(int srcno,              // Դid
        int trackid,             //track id
        const char *ip,          // Ŀ��ip
        unsigned short port,      // Ŀ��port
        bool demux = false,        // ����
        unsigned int demuxid = 0);  // ����id

    // ɾ��ת��(srcno Դid)
    static int del_send_src(int srcno);

    // ɾ������ת��
    static int del_send_all();

    static int get_svr_info(svr_info info[],int& tracknum,const int srcno);

    static int create_src_defult(int* srcno,char sdp[],int* sdp_len,const long chanid,const char* local_bind_ip);

    static int set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);
};
#endif//MEDIA_SERVER_H__

