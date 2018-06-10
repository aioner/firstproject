//
//create by songlei 20160316
//
#ifndef DPS_JK_DISPATCH_MGR_H__
#define DPS_JK_DISPATCH_MGR_H__
#include <boost/noncopyable.hpp>
#include <string>
#include "JKMainClient.h"

#ifdef WIN32
#else
	#define __stdcall
#endif

class dps_jk_dispatch_mgr : boost::noncopyable
{
public:
    // ֪ͨ������IDS 
    static void __stdcall on_tell_local_ids(void* pUser, char* LocalIDS, char* sRes1, long iRes1);

    // ֪ͨ������IDS  
    static void __stdcall on_link_server(void* pUser, long sNum, long bz);

    // ��·���ӳɹ� 
    static void __stdcall on_user_in_out(void* pUser, char* sIDS, char* sName, long sType, char* sIPS, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2);

    //ר�߱���
    static void __stdcall on_event_get_msg(void* pUser, char* sSrcIDS, char* sData,long nDataLen, long nOrderbz);

    //֪ͨ��������������ĳ��ͼ�񱻵㲥 
    static void __stdcall on_dbimage_center(void* pUser, char* sIDS, long sCH, long successbz, char* fIDS, char* fIPS, long DBMode, long localVPort,long localAPort, long destVPort, long destAPort, long iRes1, char* sRes1);

    //͸��ָ��ص�
    static void __stdcall on_transparent_command_cb(void* pUser, char* fIDS, char* sCommands);

    //���Ʒ�����¼��
    static void __stdcall on_client_record(void* pUser, char* sIDS, long sCH, long bz);

    //���Ʒ����������㲼��
    static void __stdcall on_client_alarm(void* pUser, char* sIDS, long sCH, long bz);

    //���Ʒ����������ƶ����
    static void __stdcall on_client_motion(void* pUser, char* sIDS, long sCH, long bz);

    //������������
    static void __stdcall on_client_image(void* pUser, char* sIDS, long sCH, long bz, long value);

    //Ҫ��dIDS�����DVS���ͷ�λ�ǡ����ǡ���̨��Ȧ����������
    static void __stdcall on_ask_angelcamerazt(void* pUser, long OP, char* dIDS, long dCH, char* sRes1, long iRes1);

    //Ҫ��ǿ�Ƴ�I֡
    static void __stdcall on_client_capture_iframe(void* pUser, char* sIDS, long sCH);

    //OSD������Ӧ�¼�
    static void __stdcall on_client_osd( void* pUser,char* sIDS, long sCH, char* osdName, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2);

    //������̨
    static void __stdcall on_client_yt(void* puser, char* sIDS, long sCH, long Op);

    //��̨������Ӧ�¼�
    static void __stdcall on_client_ytex(void* pUser, char* dIDS, long dCH, long op, long val, long iRes1, char* sRes1);

    //ѡ��Ԥ�õ�
    static void __stdcall on_client_select_point(void* pUser, char* sIDS, long sCH, long cNum);

    //����Ԥ�õ�
    static void __stdcall on_client_set_point(void* pUser, char* sIDS, long sCH, long cNum, char* pName);

    //����͸������
    static void __stdcall on_transparent_data(void* pUser, char* fIDS, long iCmd, char* Buf, long len);

    //�����豸״̬�ı䴥��
    static void __stdcall on_group_device_state_change( void* pUser,char* sGroupIDS, char* sDeviceIDS, char* sDeviceName, char* sDeviceIPS, long iType, long iIsOnline, long iRes1, char* sRes1);

    //Уʱ
    static void __stdcall on_check_time(void* pUser, long iYear, long iMonth, long iDate, long iWeek, long iHour, long iMin, long iSec, long iRes1, long iRes2);

    //���ܽӿ�
public:
    static dps_jk_dispatch_mgr* _() {return &my_;}
    void start();
    void stop();
    void init();
    void uninit();

    //����ص���
    ////////////////////////////////////////////////////////////////////
    void  SetCallBackOnTellLocalIDS(OnTellLocalIDS CallBack ,void* pUser);

    void  SetCallBackOnLinkServer(OnLinkServer CallBack ,void* pUser);

    void  SetCallBackOnUserInOut(OnUserInOut CallBack ,void* pUser);

    void  SetCallBackOnDBImageCenter(OnDBImageCenter CallBack ,void* pUser);

    void  SetCallBackOnEventGetMsg(OnEventGetMsg CallBack ,void* pUser);

    void  SetCallBackOnTransparentCommand(OnTransparentCommand CallBack ,void* pUser);

    void  SetCallBackOnClientRecord(OnClientRecord CallBack,void* pUser);

    void  SetCallBackOnClientAlarm(OnClientAlarm CallBack,void* pUser);

    void  SetCallBackOnClientMotion(OnClientMotion CallBack,void* pUser);

    void  SetCallBackOnClientImage(OnClientImage CallBack,void* pUser);

    void  SetCallBackOnAskAngelCameraZt(OnAskAngelCameraZt CallBack,void* pUser);

    void  SetCallBackOnClientCaptureIFrame(OnClientCaptureIFrame CallBack,void* pUser);

    void  SetCallBackOnOnClientOSD(OnClientOSD CallBack,void* pUser);

    void  SetCallBackOnClientYT(OnClientYT CallBack,void* pUser);

    void  SetCallBackOnClientYTEx(OnClientYTEx CallBack,void* pUser);

    void  SetCallBackOnClientSelectPoint(OnClientSelectPoint CallBack,void* pUser);

    void  SetCallBackOnClientSetPoint(OnClientSetPoint CallBack,void* pUser);

    void  SetCallBackOnTransparentData(OnTransparentData CallBack,void* pUser);

    void  SetCallBackOnGroupDeviceStateChange(OnGroupDeviceStateChange CallBack,void* pUser);

    void  SetCallBackOnCheckTime(OnCheckTime CallBack,void* pUser);
    ////////////////////////////////////////////////////////////////////

    //ֹͣ���ӷ����� 
    void SetServerInfo( short sNum,const char* sIPS, long sPort);

    //���õ��ÿؼ��������ھ�������ڿؼ��ڲ��������������Ϣ
    void SetMainHwnd( long mhwnd);

    //��½ʱ����У������
    void StartLinkServer( short sNum);

    //ֹͣ���ӷ�����
    void StopLinkServer( short sNum);

    //���ñ������ͣ�������IDS
    void NewSetLocalType( long sType);

    //��½ʱ����У������
    long CheckPassword( long nType,const char* Name,const char* Mima,const char* sRes1, long iRes2);

    //��JK�õ�DVR�ĵ�½�û���������
    void GetLoginInfo(const char* sIDS, char* sName, char* sPassword, char* iPort, char* sRes1, char* iRes1);

    void GetLoginInformation(const char* sIDS, char* szName, char* szPassword, long *plPort, char* szRes1, long *plRes1);

    //��Ƶ������ͨ���˷���֪ͨ���Ķ�ĳ���豸�ĵ㲥�����������Ҫ����
    void SendDARReq(const char* sIDS,long sCH,	long iCodeType,	 long iRes1,const char* sRes1);

    //����͸���ַ������������¼�
    long SendTransparentCommand(const char* sIDS,const char* sIPS,const char* sCommands);

    //֪ͨ����ͼ��㲥��Ϣ����Ƶ������ר�ã�
    void SetVideoCenterPlayID(const char* strSID, long lDvrChID, long lVCenterChID, long iRes1,const char* sRes1);

    //֪ͨ����ͼ��㲥��Ϣ����Ƶ������ר�ã�
    void  SetVideoCenterPlayIDEx(const char* strSID, long lDvrChID, long lVCenterChID, long iRes1,const char* sRes1,const char* sExInfo);

    //���ñ�����½ʱע�ᵽ���ĵ����ֺ�IP��ַ
    void SendInfo(const char* myName,const char* myIPS);

    void SetLinkType(long sNum , long sType);

    bool link_center();
    void exit_center();
    void loading_cfg();
    void set_local_ids(const std::string& ids)
    {
        local_ids_ = ids;

    }
    const std::string& get_local_ids()const 
    {
        return local_ids_;
    }

private:
    long center_link_type_;
    long centerid_;
    long center_port_;
    long server_num_;
    std::string center_ip_;
    std::string local_name_;
    std::string local_ip_;
    std::string local_ids_;
    static dps_jk_dispatch_mgr my_;
};

#endif // #ifndef DPS_JK_DISPATCH_MGR_H__
