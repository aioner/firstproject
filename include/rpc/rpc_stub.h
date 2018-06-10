#ifndef _RPC_STUB_INCLUDED
#define _RPC_STUB_INCLUDED

#include "rpc_invoke.h"

namespace rpc
{
    template<typename DrivedT>
    class stub_t : 
        public invoke_t<DrivedT>,
        public channel_impl,
        public msg_dispatch_t
    {
    public:
	typedef DrivedT drived_type;
        template<typename SerializationT, typename RequestT, typename ResponseT>
        bool invoke(dispid_t dispid, control_t& control, RequestT *request, ResponseT *response, closure_t *done)
        {
            return this->template do_invoke<SerializationT>(this, dispid, control, request, response, done);
        }

        template<typename SerializationT, typename RequestT, typename ResponseT>
        bool invoke(const char *name, control_t& control, RequestT *request, ResponseT *response, closure_t *done)
        {
            dispid_t dispid = drived_type::get_func_dispid(name);
            return this->invoke<SerializationT, RequestT, ResponseT>(dispid, control, request, response, done);
        }

        void connect_event(bool connect)
        {
            this->template do_connect_event<serialization::binary>(this, connect);
        }

        void on_send_msg(const uint8_t *msg, uint32_t msg_len, closure_t *request_closure, recv_msg_closure_t *response_closure)
        {
            this->send_msg(msg, msg_len, request_closure, response_closure);
        }

        bool on_channel_recv_msg(channel_t*channel, uint32_t sequence, uint8_t *data, uint32_t data_bytes)
        {
            return this->do_result(this, channel, data, data_bytes, sequence);
        }

        void on_process_exit(channel_t *channel)
        {
            this->get_drived()->on_process_exit();
        }

        void on_process_exit() {}
    };
}   //namespace rpc

#endif //_RPC_STUB_INCLUDED
