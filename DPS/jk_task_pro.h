//
//create by songlei 20160316
//
#ifndef JK_TASK_PRO_H__
#define JK_TASK_PRO_H__
#include <string>
#include "framework/task.h"
#include "framework/event_context.h"
#include "dps_common_type_def.h"


class on_tell_local_ids_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_tell_local_ids_task(const char* LocalIDS, const char* sRes1, const long iRes1):
      local_ids_(LocalIDS),s_res1_(sRes1),i_res1(iRes1){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string local_ids_;
    std::string s_res1_;
    long i_res1;
};

class on_link_server_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_link_server_task(const long sNum, const long bz):s_num_(sNum),bz_(bz){}
    uint32_t run();
    void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    long s_num_;
    long bz_;
};

class on_user_in_out_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_user_in_out_task( const char* sIDS, const char* sName, const long sType, const char* sIPS, const long bz, const long iRes1, const long iRes2, const char* sRes1, const char* sRes2)
        :s_ids_(sIDS),s_name_(sName),s_type_(sType),s_ips_(sIPS),bz_(bz),i_res1_(iRes1),i_res2_(iRes2),s_res1_(sRes1),s_res2_(sRes2){}
    uint32_t run();
    void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string s_ids_;
    std::string s_name_;
    long s_type_;
    std::string s_ips_;
    long bz_;
    long i_res1_;
    long i_res2_;
    std::string s_res1_;
    std::string s_res2_;
};

class on_event_get_msg_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_event_get_msg_task( const char* sSrcIDS, const char* sData, const long nDataLen, const long nOrderbz):
      s_src_ids_(sSrcIDS),s_data_(sData),n_data_len(nDataLen),n_orderbz_(nOrderbz){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string s_src_ids_;
    std::string s_data_;
    long n_data_len;
    long n_orderbz_;
};

class on_dbimage_center_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_dbimage_center_task( const char* sIDS, const long sCH, const long successbz, const char* fIDS, const char* fIPS, const long DBMode, const long localVPort, const long localAPort,
        const long destVPort, const long destAPort, const long iRes1, const char* sRes1)
        :s_ids_(sIDS),s_ch_(sCH),successbz_(successbz),fids_(fIDS),fips_(fIPS),db_mode_(DBMode),local_vport_(localVPort),local_aport_(localAPort),
        dest_vport_(destVPort),dest_aport_(destAPort),i_res1(iRes1),s_res1_(sRes1){}
    uint32_t run();
    void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string s_ids_;
    long s_ch_;
    long successbz_;
    std::string fids_;
    std::string fips_;
    long db_mode_;
    long local_vport_;
    long local_aport_;
    long dest_vport_;
    long dest_aport_;
    long i_res1;
    std::string s_res1_;
};

class on_transparent_command_cb_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_transparent_command_cb_task( const char* fIDS, const char* sCommands):
      fids_(fIDS),s_commnads_(sCommands){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string fids_;
    std::string s_commnads_;
};

class on_client_record_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_client_record_task( const char* sIDS, const long sCH, const long bz):
      sids_(sIDS),sch_(sCH),bz_(bz){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string sids_;
    long sch_;
    long bz_;
};

class on_client_alarm_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_client_alarm_task( const char* sIDS, const long sCH, const long bz):
      sids_(sIDS),sch_(sCH),bz_(bz){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string sids_;
    long sch_;
    long bz_;
};

class on_client_motion_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_client_motion_task(const char* sIDS, const long sCH, const long bz):
      sids_(sIDS),sch_(sCH),bz_(bz){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string sids_;
    long sch_;
    long bz_;
};

class on_client_image_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_client_image_task(const char* sIDS, const long sCH, const long bz, const long value):
      sids_(sIDS),sch_(sCH),bz_(bz),value_(value){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string sids_;
    long sch_;
    long bz_;
    long value_;
};

class on_ask_angelcamerazt_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_ask_angelcamerazt_task( const long OP, const char* dIDS, const long dCH, const char* sRes1, const long iRes1):
      dids_(dIDS),dch_(dCH),sres1_(sRes1),ires1(iRes1){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string dids_;
    long dch_;
    std::string sres1_;
    long ires1;
};

class on_client_capture_iframe_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_client_capture_iframe_task( const char* sIDS, const long sCH):
      sids_(sIDS),sch_(sCH){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string sids_;
    long sch_;
};

class on_client_osd_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_client_osd_task( const char* sIDS, const long sCH, const char* osdName, const long bz, const long iRes1, const long iRes2, const char* sRes1, const char* sRes2):
      sids_(sIDS),osd_name_(osdName),bz_(bz),ires1_(iRes1),ires2_(iRes2),sres1_(sRes1),sres2_(sRes2){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string sids_;
    long sch_;
    std::string osd_name_;
    long bz_;
    long ires1_;
    long ires2_;
    std::string sres1_;
    std::string sres2_;
};

class on_client_yt_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_client_yt_task( const char* sIDS, const long sCH, const long Op):
      sids_(sIDS),sch_(sCH),op_(Op){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string sids_;
    long sch_;
    long op_;
};

class on_client_ytex_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_client_ytex_task( const char* dIDS, const long dCH, const long op, const long val, const long iRes1, const char* sRes1):
      dids_(dIDS),dch_(dCH),op_(op),val_(val),ires1_(iRes1),sres1_(sRes1){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string dids_;
    long dch_;
    long op_;
    long val_;
    long ires1_;
    std::string sres1_;
};

class on_client_select_point_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_client_select_point_task( const char* sIDS, const long sCH, const long cNum):
      sids_(sIDS),sch_(sCH),cnum_(cNum){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string sids_;
    long sch_;
    long cnum_;
};

class on_client_set_point_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_client_set_point_task( char* sIDS, long sCH, long cNum, char* pName):
      sids_(sIDS),sch_(sCH),cnum_(cNum),pname_(pName){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string sids_;
    long sch_;
    long cnum_;
    std::string pname_;
};

class on_transparent_data_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_transparent_data_task( const char* fIDS, const long iCmd, const char* Buf, const long lens):
      fids_(fIDS),icmd_(iCmd),buf_(Buf),lens_(lens){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string fids_;
    long icmd_;
    std::string buf_;
    long lens_;
};

class on_group_device_state_change_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_group_device_state_change_task( const char* sGroupIDS, const char* sDeviceIDS, const char* sDeviceName, const char* sDeviceIPS, const long iType, const long iIsOnline, const long iRes1, const char* sRes1):
      s_group_ids_(sGroupIDS),s_device_ids(sDeviceIDS),s_device_name(sDeviceName),s_device_ips(sDeviceIPS),itype_(iType),iisonline_(iIsOnline),ires1_(iRes1),sres1_(sRes1){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    std::string s_group_ids_;
    std::string s_device_ids;
    std::string s_device_name;
    std::string s_device_ips;
    long itype_;
    long iisonline_;
    long ires1_;
    std::string sres1_;
};

class on_check_time_task : public framework::task_base_t, public framework::event_context_t
{
public:
    on_check_time_task( const long iYear, const long iMonth, const long iDate, const long iWeek, const long iHour, const long iMin, const long iSec, const long iRes1, const long iRes2 ):
      iyear_(iYear),imonth_(iMonth),idate_(iDate),iweek_(iWeek),ihour_(iHour),imin_(iMin),isec_(iSec),ires1_(iRes1),ires2_(iRes2){}
      uint32_t run();
      void process_event() {signal(deferred, recv_center_cmd_pro_thread);}
private:
    long iyear_;
    long imonth_;
    long idate_;
    long iweek_;
    long ihour_;
    long imin_;
    long isec_;
    long ires1_;
    long ires2_;
};
#endif //end #ifndef JK_TASK_PRO_H__
