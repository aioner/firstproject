#ifndef XTROUTER_H__
#define XTROUTER_H__

#include <string>
#include <vector>
#include <stdint.h>
#include <boost/noncopyable.hpp>
#include "Router_config.h"
#include "mml/cmd_manager.h"
#define  COMMAND_DISPATTCH_FUNCTION bool
struct  router_config
{
    std::string local_ip;                 //���ص�ַ
    std::string local_snd_ip;             //���ط��͵�ַ
    std::string mul_ip;                  //�鲥��ʼ��ַ 
    std::string log_usre_name;         //��¼Openfire�û���
    std::string log_usre_password;     //����
    std::string log_usre_res_id;       //��ԴID
    std::string log_center_name;       //����������
    std::string default_sdp;           //Ĭ��ȫ����������sdp
    uint32_t   log_center_port;       //��¼���Ķ˿�
    unsigned long chan_num;           //����ͨ����
    unsigned short start_sndport;      //������ʼ�˿�
    bool demux_flg_s;                      //���͸���
    bool reserve1;                      //Ԥ��
    unsigned xtmsg_listenport;         //xtmsg�����˿�
    unsigned rtsp_listenport;          //rtsp�����˿�
    unsigned tcp_listenport;           //tcp����&�����˿�
    unsigned regist_listenport;        //ע������˿�
    unsigned udp_listenport;           //UDP�Ự�����˿� 
    bool regist;                      //�Ƿ�ע��
    unsigned regist_bind;              //ע��󶨶˿�
    std::string regist_ip;            //ע���ַ
    unsigned regist_port;              //ע��˿�
    bool std_rtp;                    //���ͱ�׼RTP
    bool demux_r;           //���ն˸��ÿ���
    bool use_strmtype_param; //ʹ��strmtype
    center_type link_center_type;//������������
};

class CXTRouter : boost::noncopyable
{
private:
    static CXTRouter m_obj;
public:
    static CXTRouter* Instance()
    {
        return &m_obj;
    }
    static CXTRouter* _()
    {
        return &m_obj;
    }

    int StartXTRouter();

    void init_cfg();

    int StopXTRouter();

    void GetConfig(router_config &config);

    bool SndStdRtp() const
    {
        return m_config.std_rtp;
    }

    //������ú���
protected:
    //�㲥����
	COMMAND_DISPATTCH_FUNCTION  Play(const command_argument_t& Args,std::string&result);
	COMMAND_DISPATTCH_FUNCTION  sdp_send(const command_argument_t& Args,std::string&result);

    //ֹͣ
    COMMAND_DISPATTCH_FUNCTION  Stop(const command_argument_t& Args,std::string&result);
    //ֹͣ
    COMMAND_DISPATTCH_FUNCTION  StopAll(const command_argument_t& Args,std::string&result);

    //������Ϣ
    COMMAND_DISPATTCH_FUNCTION  ShowConnectInfo(const command_argument_t& Args,std::string&result);
    COMMAND_DISPATTCH_FUNCTION  HideConnectInfo(const command_argument_t& Args,std::string &result);

    //�鿴�����㲥��Ϣ
    COMMAND_DISPATTCH_FUNCTION  ShowPlayInfo(const command_argument_t& Args,std::string&result);
    COMMAND_DISPATTCH_FUNCTION  HidePlayInfo(const command_argument_t& Args,std::string &result);

    //�鿴ϵͳͷ��Ϣ
    COMMAND_DISPATTCH_FUNCTION  ShowKeyInfo(const command_argument_t& Args, std::string&result);

    //����
    COMMAND_DISPATTCH_FUNCTION  Help(const command_argument_t& Args,std::string&result);

    //��¼�ǳ����� ��Ϣ�鿴
    COMMAND_DISPATTCH_FUNCTION ShowlogInfo(const command_argument_t& Args,std::string &result);

    //���ĵ㲥���� ��Ϣ�鿴
    COMMAND_DISPATTCH_FUNCTION ShowCenterCommand(const command_argument_t& Args,std::string &result);

    //����ִ�н�� ��Ϣ�鿴
    COMMAND_DISPATTCH_FUNCTION ShowSigalingExecRet(const command_argument_t& Args,std::string &result);

    //���������¼���Ϣ
    COMMAND_DISPATTCH_FUNCTION ShowResponseInfo(const command_argument_t& Args,std::string &result);

    //LinkSever�¼���Ϣ�鿴
    COMMAND_DISPATTCH_FUNCTION ShowLinkSeverEventInfo(const command_argument_t& Args,std::string &result);

    //����
    COMMAND_DISPATTCH_FUNCTION Clear(const command_argument_t& Args,std::string &result);

    COMMAND_DISPATTCH_FUNCTION AppExit(const command_argument_t& Args,std::string &result);

    //������Ϣ�鿴
    COMMAND_DISPATTCH_FUNCTION ShowBaseInfo(const command_argument_t& Args,std::string &result);

    //query information of the framework task schedulers
    COMMAND_DISPATTCH_FUNCTION QueryFrameworkTaskScheduler(const command_argument_t& Args,std::string &result);

    COMMAND_DISPATTCH_FUNCTION LogOnOff(const command_argument_t& Args,std::string&result);

    COMMAND_DISPATTCH_FUNCTION XmppPlay(const command_argument_t& Args,std::string &result);
    COMMAND_DISPATTCH_FUNCTION XmppStop(const command_argument_t& Args,std::string &result);

    COMMAND_DISPATTCH_FUNCTION xregister(const command_argument_t& Args,std::string &result);
    COMMAND_DISPATTCH_FUNCTION xstop_register(const command_argument_t& Args,std::string &result);

    COMMAND_DISPATTCH_FUNCTION get_center_name(const command_argument_t& Args,std::string &result);

    COMMAND_DISPATTCH_FUNCTION log_ccs(const command_argument_t& Args,std::string &result);

    //�򿪽���
    COMMAND_DISPATTCH_FUNCTION openr(const command_argument_t& Args,std::string &result);
    COMMAND_DISPATTCH_FUNCTION setsdpr(const command_argument_t& Args,std::string &result);
    COMMAND_DISPATTCH_FUNCTION close_recv(const command_argument_t& Args,std::string &result);

    //��һ������
    COMMAND_DISPATTCH_FUNCTION add_send(const command_argument_t& Args,std::string &result);

    //ɾ�����з���
    COMMAND_DISPATTCH_FUNCTION del_send_all(const command_argument_t& Args,std::string &result);

    //ɾ��ָ������
    COMMAND_DISPATTCH_FUNCTION del_send(const command_argument_t& Args,std::string &result);

    //��ȡ������Ϣ
    COMMAND_DISPATTCH_FUNCTION getsndinf(const command_argument_t& Args,std::string&result);

    //��ȡ���ն���Ϣ
    COMMAND_DISPATTCH_FUNCTION getrecvinf(const command_argument_t& Args,std::string&result);

    //�鿴���ػỰ״̬
    COMMAND_DISPATTCH_FUNCTION gws(const command_argument_t& Args,std::string&result);

    //�鿴������Ϣ
    COMMAND_DISPATTCH_FUNCTION recv(const command_argument_t& Args,std::string&result);

protected:
    void PrintPlayCommandHelp(std::string &result);
	void PrintlogCommandHelp(std::string &result);
    void PrintSkiCommandHelp(std::string &result);
    void PrintSwiCommandHelp(std::string &result);
    void PrintGetrevinfCommandHelp(std::string &result);
    void PrintGersndinfCommandHelp(std::string &result);
    void PrintStopCommandHelp(std::string &result);
    void PrintSpiCommandHelp(std::string &result);
    void PrintslsiCommandHelp(std::string &result);
    void PrintsliCommandHelp(std::string &result);
    void PrintclsComamndHelp(std::string &result);
    void PrintScoiCommandHelp(std::string &result);
private:
    void ResCommand();

    void CtrlMsgInit();
    void CtrlMsgUninit();
public:
    const router_config& get_router_cfg(){return m_config;}
    const std::string& get_local_ip() const
    {
        return m_config.local_ip;
    }
    void update_cfg_local_ip(const std::string& ip)
    {
        m_config.local_ip.assign(ip);
    }
    bool get_send_demux_flag() const
    {
        return m_config.demux_flg_s;
    }

    bool get_demux_mode_r() const
    {
        return m_config.demux_r;
    }

    const bool use_jk() const 
    { 
        return (m_config.link_center_type == config_type::LINK_XTCENTER); 
    }

    const bool use_ccs()const 
    {
        return (m_config.link_center_type == config_type::LINK_CCS); 
    }

    const bool use_sc()const
    {
        return(m_config.link_center_type == config_type::LIPK_SC);
    }
	const bool use_sc2()const
	{
		return true;
	}
    const std::string& get_default_sdp() const 
    {
        return m_config.default_sdp;
    }
public:
    void regist_log_sys();
    // ��������
    router_config m_config;
};
#endif //XTROUTER_H__
