#ifndef _RPC_MESSAGE_QUEUE_H_INCLUDED
#define _RPC_MESSAGE_QUEUE_H_INCLUDED

#include "rpc_channel.h"
#include "message_queue_service.h"
#include "rpc_memory_pool.h"
#include <memory>
#include <map>
#include <boost/shared_ptr.hpp>
#include <stddef.h>
#include <boost/bind.hpp>


#if (defined(WIN32) || defined(WIN64))   && !defined(_OS_WINDOWS)
#define _OS_WINDOWS
#endif//#if (defined(WIN32) || defined(WIN64))   && !defined(_OS_WINDOWS)

#ifdef _OS_WINDOWS
#include <wtypes.h>
#else
#include <unistd.h>
typedef pid_t HANDLE;
#endif

#define MAX_BEF_SIZE 4096
#define MSG_QUEUE_SIZE 4096
#define THREAD_CHECK_FREQUENCY 1
#define JK_MSG_QUEUE_SERVER "JKMainClientServer"
#define JK_MSG_QUEUE_CLIENT "JKMainClientClient"
namespace rpc 
{ 
    namespace message_queue
    {
        typedef message_queue_service_t service_t;

        enum msg_type
        {
            invalid_msg = -1,
            connect_msg,
            connect_response_msg,
            disconnect_msg,
            normal_msg
        };

        struct msg_header_type
        {
            uint32_t sequence_;
            uint16_t msg_len_;
            uint8_t msg_type_;
            uint8_t name_len_;

            msg_header_type()
                :sequence_(0),
                msg_len_(0),
                msg_type_(invalid_msg),
                name_len_(0)
            {}

            char *get_name()
            {
                return (char *)(this + 1);
            }

            uint8_t *get_msg()
            {
                return (uint8_t *)(this + 1) + name_len_;
            }
        };

        class channel_base_t;
        class base_t;

        class recv_msg_closure
        {
        public:
            recv_msg_closure()
                :channel_(NULL),
                recvd_size_(0)
            {}

            inline void done();
            channel_base_t *channel_;
            uint8_t buf_[MAX_BEF_SIZE];
            std::size_t recvd_size_;
        };

        class send_msg_closure
        {
        public:
            send_msg_closure()
                :done_(NULL)
            {}

            inline void done();

            uint8_t buf_[MAX_BEF_SIZE];
            closure_t *done_;
            base_t *base_;
        };

        class base_t : 
            public memorypool::use_impl<send_msg_closure>,
            public memorypool::use_impl<recv_msg_closure>,
            public memorypool::attach_impl<base_t>
        {
        public:
            explicit base_t(service_t& service)
                :point_(service)
            {}

            bool x_send_msg(msg_type type, const std::string& name, uint32_t sequence = 0, const uint8_t *data = NULL, uint32_t data_bytes = 0, closure_t *done = NULL)
            {
                send_msg_closure *closure = NULL;
                uint8_t *buf = NULL;
                uint8_t buf2[MAX_BEF_SIZE];
                if (NULL == done)
                {
                    buf = buf2;
                }
                else
                {
                    closure = get_object<send_msg_closure>();
                    buf = closure->buf_;
                    closure->done_ = done;
                    closure->base_ = this;
                    std::cout << "x_send_msg |open, closure=" << closure <<  ",done" << closure->done_ << std::endl;
                }
                msg_header_type *msg = (msg_header_type *)(buf);
                msg->sequence_ = sequence;
                msg->msg_len_ = (uint16_t)(sizeof(msg_header_type) + name.length() + data_bytes);
                msg->msg_type_ = type;
                msg->name_len_ = (uint8_t)name.length();

                uint32_t buf_size = sizeof(msg_header_type);
                (void)memcpy(buf + buf_size, name.c_str(), name.length());
                buf_size += (uint32_t)name.length();
                (void)memcpy(buf + buf_size, data, data_bytes);
                buf_size += data_bytes;

                if (NULL == done)
                {
                    //不允许不限时间阻塞，这里使用10秒超时
                    return point_.timed_send(buf, buf_size, 10000);
                }
                else
                {
                    point_.async_send(buf, buf_size, boost::bind(&send_msg_closure::done, closure));
                }
                return true;
            }

            const std::string& get_name() const
            {
                return point_.get_name();
            }

            bool do_create(const char *name, uint32_t max_num_msg, uint32_t max_msg_size)
            {
                return point_.create(name, max_num_msg, max_msg_size);
            }

            bool do_open(const char *name)
            {
                return point_.open(name);
            }

            message_queue_t point_;
        };

        class channel_base_t : public rpc::channel_t, public base_t
        {
        public:
            typedef boost::shared_ptr<channel_base_t> channel_base_ptr;

            channel_base_t(service_t& service)
                :base_t(service),
                events_(NULL)
            {}

            ~channel_base_t()
            {
                events_ = NULL;
            }

            void close()
            {
                send_msg(disconnect_msg);
            }

            bool send_msg(uint32_t sequence, const uint8_t *data, uint32_t data_bytes, closure_t *done)
            {
                return send_msg(normal_msg, sequence, data, data_bytes, done);
            }

            bool register_events(events_type *ev)
            {
                events_ = ev;
                return true;
            }

            bool bind(const char *name, uint32_t max_num_msg, uint32_t max_msg_size)
            {
                if (!do_create(name, max_num_msg, max_msg_size))
                {
                    return false;
                }
                start_recv();
                return true;
            }

        protected:
            void start_recv()
            {
                recv_msg_closure *closure = get_object<recv_msg_closure>();
                closure->channel_ = this;
                point_.async_receive(closure->buf_, MAX_BEF_SIZE, closure->recvd_size_, boost::bind(&recv_msg_closure::done, closure));
            }

            bool send_msg(msg_type type, uint32_t sequence = 0, const uint8_t *data = NULL, uint32_t data_bytes = 0, closure_t *done = NULL)
            {
                return get_send_channel()->x_send_msg(type, get_local_point_name(), sequence, data, data_bytes, done);
            }

            void _process_exit_notify(HANDLE hp)
            {
                point_.async_condition(boost::bind(&channel_base_t::process_exit_true_condition, hp), boost::bind(&channel_base_t::on_process_exit, this));
            }

            static bool process_exit_true_condition(HANDLE hp)
            {
#ifdef _OS_WINDOWS
                DWORD exit_code = 0;
                if (::GetExitCodeProcess(hp, &exit_code))
                {
                    return (STILL_ACTIVE != exit_code);
                }
                return true;
#else
                return (ESRCH != getpgid(hp));
#endif
            }

            virtual channel_base_t *get_send_channel() { return this; };
            virtual const std::string& get_local_point_name() const { return get_name(); };
            virtual channel_t *find_client(const std::string&) {  return this; };
            virtual void on_process_exit() {}
        protected:
            friend class recv_msg_closure;
            friend class server_channel_t;
            events_type *events_;
        };

        class client_channel_t : public channel_base_t
        {
        public:
            client_channel_t(service_t& service)
                :channel_base_t(service),
                h_process_(NULL)
            {}

            bool connect(const char *name)
            {
                channel_base_ptr point(new channel_base_t(point_.get_service()));
                if (!point->do_open(name))
                {
                    return false;
                }
                remote_point_ = point;

#ifdef _OS_WINDOWS
                return send_msg(connect_msg, ::GetCurrentProcessId());
#else
                return send_msg(connect_msg, ::getpid());
#endif
            }

            channel_base_t *get_send_channel()
            {
                return remote_point_.get();
            }

            void process_exit_notify(HANDLE hp)
            {
                h_process_ = hp;
                _process_exit_notify(hp);
            }

            void on_process_exit()
            {
                std::cout << "rpc server process exit" << std::endl;
                if (NULL != events_)
                {
                    events_->on_process_exit(this);
                }
#ifdef _OS_WINDOWS
                ::CloseHandle(h_process_);
#endif
            }

            void on_connect_response(HANDLE hp)
            {
#ifdef _OS_WINDOWS
                process_exit_notify(hp);
#endif//#ifdef _OS_WINDOWS
            }
        private:
            channel_base_ptr remote_point_;
            HANDLE h_process_;
        };

        class server_channel_t : public channel_base_t
        {
        public:
            typedef boost::recursive_mutex mutex_type;

            class client_t : public channel_base_t
            {
            public:
                client_t(service_t& service, server_channel_t *server, HANDLE hp)
                    :channel_base_t(service),
                    server_(server),
                    h_process_(hp)
                {
#ifdef _OS_WINDOWS
                    _process_exit_notify(h_process_);
#endif//#ifdef _OS_WINDOWS
                }

                channel_base_t *get_send_channel()
                {
                    return this;
                }

                const std::string& get_local_point_name() const
                {
                    return server_->get_name();
                }

                void on_process_exit()
                {
                      std::cout << "on_process_exit | rpc client[" << get_name() << "]process exit" << std::endl;
                      //modified by lichao, 20150309 提前释放资源
#ifdef _OS_WINDOWS
                      ::CloseHandle(h_process_);
#endif
                    if (server_)
                    {
                        server_channel_t *svr = server_;
                        server_ = NULL;
                        svr->on_clients_process_exit(this);
                    }
                }
            private:
                server_channel_t *server_;
                HANDLE h_process_;
            };

            typedef std::map<std::string, channel_base_ptr> clients_container_type;

            server_channel_t(service_t& service)
                :channel_base_t(service)
            {}

            void on_clients_process_exit(client_t *client)
            {
                if (events_)
                {
                    events_->on_process_exit(client);
                }
                del_client(client->get_name());
            }

            void on_connect(const std::string& name, HANDLE hp)
            {
                channel_t * channel = add_client(name, hp);
                if (channel)
                {
                    if (NULL != events_)
                    {
                        events_->on_channel_connect(channel);
                    }

#ifdef _OS_WINDOWS
                    ((client_t *)channel)->send_msg(connect_response_msg, ::GetCurrentProcessId());
#else
                    ((client_t *)channel)->send_msg(connect_response_msg, getpid());
#endif
                }
            }

            void on_disconnect(const std::string& name)
            {
                std::cout << "on_disconnect | rpc client[" << name << "] normal exit" << std::endl;
                del_client(name);
            }

            channel_t *find_client(const std::string& name)
            {
                mutex_type::scoped_lock lock(mutex_);

                clients_container_type::iterator iter = clients_.find(name);
                if (clients_.end() == iter)
                {
                    return NULL;
                }
                return iter->second.get();
            }
        protected:
            channel_t *add_client(const std::string& name, HANDLE hp)
            {
                mutex_type::scoped_lock lock(mutex_);

                if (channel_t *result = find_client(name))
                {
                    return result;
                }

                channel_base_ptr channel(new client_t(point_.get_service(), this, hp));
                if (!channel->do_open(name.c_str()))
                {
                    return NULL;
                }

                clients_.insert(clients_container_type::value_type(name, channel));
                return channel.get();
            }

            void del_client(const std::string& name)
            {
                mutex_type::scoped_lock lock(mutex_);
                channel_t *result = find_client(name);
                if (NULL != result)
                {
                    events_->on_channel_close(result);
                }
                clients_.erase(name);
            }
        private:
            //服务端的channel不能发送
            using channel_base_t::send_msg;

            clients_container_type clients_;
            mutex_type mutex_;
        };

        inline void recv_msg_closure::done()
        {
            msg_header_type *msg = (msg_header_type *)(buf_);
            switch (msg->msg_type_)
            {
            case connect_msg:
                {
                    server_channel_t *impl = dynamic_cast<server_channel_t *>(channel_);
                    if (NULL != impl)
                    {

                        std::string name((const char *)(buf_ + sizeof(msg_header_type)), msg->name_len_);
#ifdef _OS_WINDOWS
                        DWORD pid = (DWORD)msg->sequence_;
                        HANDLE hp = ::OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
#else
                        HANDLE hp  = msg->sequence_;
                        HANDLE pid = hp;
#endif
                        if (0 != hp)
                        {
                            impl->on_connect(name, hp);
                        }
                        else
                        {
                            std::cout << "recv_msg_closure::done() |connect msg pid=" << pid << " not found" << std::endl;
                        }
                    }
                }
                break;
            case connect_response_msg:
                {
                    client_channel_t *impl = dynamic_cast<client_channel_t *>(channel_);
                    if (NULL != impl)
                    {
#ifdef _OS_WINDOWS
                        DWORD pid = (DWORD)msg->sequence_;
                        HANDLE hp = ::OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
#else
                        HANDLE hp = msg->sequence_;
                        HANDLE pid = hp;
#endif
                        if (0 != hp)
                        {
                            impl->on_connect_response(hp);
                        }
                        else
                        {
                            std::cout << "recv_msg_closure::done() | connect response msg pid=" << pid << " not found" << std::endl;
                        }
                    }
                }
                break;
            case disconnect_msg:
                {
                    server_channel_t *impl = dynamic_cast<server_channel_t *>(channel_);
                    if (NULL != impl)
                    {
                        std::string name((const char *)(buf_ + sizeof(msg_header_type)), msg->name_len_);
                        impl->on_disconnect(name);
                    }
                }
                break;
            case normal_msg:
                {
                    std::string name((const char *)(buf_ + sizeof(msg_header_type)), msg->name_len_);
                    uint32_t bytes = (uint32_t)recvd_size_ - (uint32_t)(sizeof(msg_header_type) + msg->name_len_);
                    uint8_t *data = buf_ + sizeof(msg_header_type) + msg->name_len_;
                    if (bytes > 0)
                    {
                        channel_t *client = channel_->find_client(name);
                        if (NULL != client) //消息到达之前，可能client已经退出了
                        {
                            channel_->events_->on_channel_recv_raw_data(client, msg->sequence_, data, bytes);
                        }
                    }
                }
                break;
            }
            channel_->start_recv();
            channel_->free_object(this);
        }

        inline void send_msg_closure::done()
        {
            done_->done();
            std::cout << "send_msg_closure::done() |close closure=" << this <<  ",done" << done_ << std::endl;
            base_->free_object(this);
        }

    }
}
#endif //_RPC_MESSAGE_QUEUE_H_INCLUDED
