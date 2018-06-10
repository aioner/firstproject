#ifndef STD_SIP_TASK_H2__
#define STD_SIP_TASK_H2__
#include "framework/task.h"
#include "framework/event_context.h"
#include "common_type.h"
#include "sip_svr_engine.h"

#define recv_sc_sip_sesseion_pro_thread 3

typedef enum
{
	mode_offer=0,
	mode_answer
}sip_mode_t;

class recv_invite_task : public framework::task_base, public framework::event_context
{
public:
	recv_invite_task(const std_sip_session_t& session):
	  session_(session)
	  {
	  }

	  ~recv_invite_task()
	  {
	  }

	  uint32_t run();
	  void process_event() { signal(deferred, recv_sc_sip_sesseion_pro_thread); }
private:
	std_sip_session_t session_;
};

class recv_reinvite_task : public framework::task_base, public framework::event_context
{
public:
	recv_reinvite_task(const std_sip_session_t& session,const xt_sip_server_invite_handle_t handle,const sip_mode_t mode):
	  session_(session),mode_(mode),invite_handle_(handle)
	  {
	  }

	  ~recv_reinvite_task()
	  {
	  }


	  uint32_t run();
	  void process_event() { signal(deferred, recv_sc_sip_sesseion_pro_thread); }
private:
	std_sip_session_t session_;
	xt_sip_server_invite_handle_t invite_handle_;
	sip_mode_t mode_;
};

class send_invite_task : public framework::task_base, public framework::event_context
{
public:
	send_invite_task(const xt_sip_handle_t sip_handle,long id,std_sip_session_t dst_session):
	  dst_session_(dst_session),sip_hanle_(sip_handle),id_(id)
	  {
	  }

	  ~send_invite_task()
	  {
	  }

	  uint32_t run();
	  void process_event() { signal(deferred, recv_sc_sip_sesseion_pro_thread); }
private:
	std_sip_session_t dst_session_;
	//sip ¾ä±ú
	xt_sip_handle_t sip_hanle_;
	long id_;
};

class ack_with_sdp_task : public framework::task_base, public framework::event_context
{
public:
	ack_with_sdp_task(const std::string& call_id, const std::string& sdp,const xt_sip_server_invite_handle_t handle):
	  call_id_(call_id),sdp_(sdp)
	  {
		  invite_handle_ = ::xt_sip_server_invite_handle_clone(handle);
	  }
	  ~ack_with_sdp_task()
	  {
		  ::xt_sip_server_invite_handle_delete(invite_handle_);
	  }

	  uint32_t run();
	  void process_event() { signal(deferred, recv_sc_sip_sesseion_pro_thread); }

private:
	std::string call_id_;
	std::string sdp_;
	xt_sip_server_invite_handle_t invite_handle_;
};

class switchset_task : public framework::task_base, public framework::event_context
{
public:
	switchset_task(xt_sip_client_invite_handle_t h,std::string callid,std::string sdp,long len):
	  client_handle_(h),call_id_(callid),sdp_(sdp),len_(len)
	  {
	  }
	  ~switchset_task()
	  {
	  }
	  uint32_t run();
	  void process_event() { signal(deferred, recv_sc_sip_sesseion_pro_thread); }

private:
	xt_sip_client_invite_handle_t client_handle_;
	std::string call_id_;
	std::string sdp_;
	long len_;
};

class invite_bye_task : public framework::task_base, public framework::event_context
{
public:
	invite_bye_task(const std::string& call_id,const xt_sip_invite_terminated_reason_t reason):
	  call_id_(call_id),reason_(reason)
	  {

	  }
	  ~invite_bye_task()
	  {

	  }
	  uint32_t run();
	  void process_event() { signal(deferred, recv_sc_sip_sesseion_pro_thread); }

private:
	std::string call_id_;
	xt_sip_invite_terminated_reason_t reason_;
};
#endif