#ifndef _RPC_PROXY_H_INCLUDED
#define _RPC_PROXY_H_INCLUDED

#include "rpc_control.h"
#include "rpc_invoke.h"
#include <string>
#include <set>
#include <boost/thread/recursive_mutex.hpp>

namespace rpc
{
    class proxy_base
    {
    public:
        virtual void set_channel_impl(channel_impl *impl) = 0;
        virtual bool do_response(channel_t *, control_t&, std::istream&, uint32_t) = 0;
        virtual iid_t get_iid() const = 0;
        virtual void on_channel_disconnect(channel_t *) = 0;
        virtual void on_process_exit(channel_t *) = 0;
    };

    template<typename DrivedT>
    class proxy_t : 
        public invoke_t<DrivedT>, 
        public proxy_base,
        public msg_dispatch_t
    {
    public:
	typedef DrivedT drived_type;
        typedef boost::recursive_mutex mutex_type;
        typedef std::set<channel_t *> connect_points_container_type;

        proxy_t()
            :impl_(NULL)
        {}

        void set_channel_impl(channel_impl *impl)
        {
            impl_ = impl;
        }

        bool do_response(channel_t *channel, control_t&control, std::istream& is, uint32_t sequence)
        {
            return this->do_result(impl_, channel, control, is, sequence);
        }

        iid_t get_iid() const
        {
            return this->get_drived()->get_register_iid();
        }

        template<typename SerializationT, typename RequestT>
        void fire_event(dispid_t dispid, control_t& control, RequestT *request)
        {
            this->template do_event<SerializationT>(this, dispid, control, request);
        }

        template<typename SerializationT, typename RequestT>
        void fire_event(const char *name, control_t& control, RequestT *request)
        {
            dispid_t dispid = drived_type::get_func_dispid(name);
            fire_event<SerializationT>(dispid, control, request);
        }

        void on_send_msg(const uint8_t *msg, uint32_t msg_len, closure_t *request_closure, recv_msg_closure_t *)
        {
            mutex_type::scoped_lock lock(mutex_);

            for (connect_points_container_type::iterator iter = connect_points_.begin(); connect_points_.end() != iter; ++iter)
            {
                impl_->x_send_msg(*iter, msg, msg_len, request_closure->clone(), NULL);
            }
            request_closure->done();
        }

        bool on_channel_recv_msg(channel_t *channel, uint32_t sequence, uint8_t *data, uint32_t data_bytes)
        {
            return this->do_result(impl_, channel, data, data_bytes, sequence);
        }

        void on_connnect_event(channel_t *channel, bool connect)
        {
            if (connect)
            {
                add_connect_point(channel);
                this->get_drived()->on_client_connect();
            }
            else
            {
                del_connect_point(channel);
                this->get_drived()->on_client_disconnect(false);
            }
        }

        void on_channel_disconnect(channel_t *channel)
        {
            del_connect_point(channel);
        }

        void on_process_exit(channel_t *channel)
        {
            del_connect_point(channel);
            this->get_drived()->on_client_disconnect(true);
        }

        void on_client_connect() {}
        void on_client_disconnect(bool is_exception) {}
    private:
        bool add_connect_point(channel_t *pt)
        {
            mutex_type::scoped_lock lock(mutex_);
            connect_points_.insert(pt);
            return true;
        }

        bool del_connect_point(channel_t *pt)
        {
            mutex_type::scoped_lock lock(mutex_);
            connect_points_.erase(pt);
            return true;
        }

        connect_points_container_type connect_points_;
        mutex_type mutex_;
        channel_impl *impl_;
    };
}

#endif //_RPC_PROXY_H_INCLUDED
