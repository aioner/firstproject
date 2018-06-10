#ifndef _RPC_SERVER_H_INCLUDED
#define _RPC_SERVER_H_INCLUDED

#include "rpc_proxy.h"
#include "rpc_memory_pool.h"
#include <map>
#include <boost/thread/recursive_mutex.hpp>

namespace rpc
{
    class server_t;

    namespace server_detail
    {
        class request_closure : public closure_t
        {
        public:
            request_closure(server_t *server, std::string *buf)
                :server_(server),
                buf_(buf)
            {}

            inline void done();

        protected:
            server_t *server_;
            std::string *buf_;
        };
    }

    class server_t :
        public memorypool::use_impl<std::string>,
        public memorypool::use_impl<server_detail::request_closure>,
        public memorypool::attach_impl<server_t>,
        public channel_impl
    {
    public:
        typedef std::map<iid_t, proxy_base *> proxy_container_type;
        typedef boost::recursive_mutex mutex_type;

        explicit server_t()
            :proxys_()
        {}

        bool register_proxy(proxy_base * x)
        {
            if (NULL == x)
            {
                return false;
            }

            mutex_type::scoped_lock lock(mutex_);

            //代理已经存在
            if (NULL != find_proxy(x->get_iid()))
            {
                return false;
            }

            x->set_channel_impl(this);

            proxys_.insert(proxy_container_type::value_type(x->get_iid(), x));
            return true;
        }

        bool on_channel_recv_msg(channel_t *channel, uint32_t sequence, uint8_t *data, uint32_t data_bytes)
        {
            std::istringstream is(std::string((const char *)data, data_bytes));

            control_t control;
            try
            {
                control.load(is);
            }
            catch (...)
            {
                throw;
            }

            proxy_base *proxy = find_proxy(control.get_iid());
            if (NULL == proxy)
            {
                return false;
            }

            if (!proxy->do_response(channel, control, is, sequence))
            {
                return false;
            }

            return false;
        }

        void on_channel_close(channel_t *channel)
        {
            {
                mutex_type::scoped_lock lock(mutex_);

                for (proxy_container_type::iterator iter = proxys_.begin(); proxys_.end() != iter; ++iter)
                {
                    iter->second->on_channel_disconnect(channel);
                }
            }

            channel_impl::on_channel_close(channel);
        }

        void on_process_exit(channel_t *channel)
        {
            mutex_type::scoped_lock lock(mutex_);

            for (proxy_container_type::iterator iter = proxys_.begin(); proxys_.end() != iter; ++iter)
            {
                iter->second->on_process_exit(channel);
            }
        }
    private:
        proxy_base *find_proxy(iid_t iid)
        {
            mutex_type::scoped_lock lock(mutex_);

            proxy_container_type::iterator result = proxys_.find(iid);
            if (proxys_.end() == result)
            {
                return NULL;
            }
            return result->second;
        }

        friend class server_detail::request_closure;

        //all proxys
        proxy_container_type proxys_;
        mutex_type mutex_;
    };

    namespace server_detail
    {
        inline void request_closure::done()
        {
            server_->free_object(buf_);
            server_->free_object(this);
        }
    }
}

#endif //_RPC_SERVER_H_INCLUDED
