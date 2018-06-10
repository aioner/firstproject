#ifndef _RPC_INVOKE_H_INCLUDED
#define _RPC_INVOKE_H_INCLUDED

#include "rpc_closure.h"
#include "rpc_control.h"
#include "rpc_channel.h"
#include "rpc_serialize.h"
#include "rpc_memory_pool.h"
#include "rpc_exception.h"

#include <sstream>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace rpc
{
    namespace invoke_detail
    {
        class request_closure_t;
        class response_closure_t;
        class sync_wait_response_closure_t;

        class use_pool_impl :
            public memorypool::use_impl<request_closure_t>,
            public memorypool::use_impl<response_closure_t>,
            public memorypool::use_impl<sync_wait_response_closure_t>,
            public memorypool::attach_impl<use_pool_impl>
        {};

        class request_closure_t : public closure_t
        {
        public:
           explicit request_closure_t(use_pool_impl *pl)
                :pl_(pl)
           {}

           void done()
           {
                pl_->free_object(this);
           }

           closure_t *clone()
           {
               return pl_->get_object<request_closure_t>(pl_);
           }

        private:
            use_pool_impl *pl_;
        };

        class response_closure_t : public recv_msg_closure_t
        {
        public:
            response_closure_t(serialization::type type, void *response, bool (*fn)(void *, rpc::serialization::type, std::istream&), closure_t *done, use_pool_impl *pl)
                :serialize_type_(type),
                response_(response),
                in_object_(fn),
                done_(done),
                pl_(pl)
            {}

            void done(uint8_t *data, uint32_t data_bytes)
            {
                serialize(data, data_bytes);

                done_->done();
                pl_->free_object(this);
            }

        protected:
            void serialize(uint8_t *data, uint32_t data_bytes)
            {
                std::istringstream is(std::string((const char *)data, data_bytes));

                try
                {
                    control_t control;
                    control.load(is);

                    in_object_(response_, serialize_type_, is);
                }
                catch (...)
                {}
            }

            serialization::type serialize_type_;
            void *response_;
            bool (*in_object_)(void *, rpc::serialization::type, std::istream&);
            closure_t *done_;
            use_pool_impl *pl_;
        };

        class sync_wait_response_closure_t : public response_closure_t
        {
        public:
            enum
            {
                init_state = 0x1,
                done_state,
                timeout_state
            };

            sync_wait_response_closure_t(serialization::type type, void *response, bool (*fn)(void *, rpc::serialization::type, std::istream&), use_pool_impl *pl)
                :response_closure_t(type, response, fn, NULL, pl),
                result_state_(init_state)
            {}

            void done(uint8_t *data, uint32_t data_bytes)
            {
                bool free_flag = false;
                {
                    boost::unique_lock<boost::mutex> lock(result_mutex_);
                    if (init_state == result_state_)
                    {
                        serialize(data, data_bytes);
                        result_state_ = done_state;
                        result_cv_.notify_all();
                    }

                    free_flag = (timeout_state == result_state_);
                }

                //超时由done释放
                if (free_flag)
                {
                    pl_->free_object(this);
                }
            }

            bool wait_result(uint32_t millsec)
            {
                bool result = false;
                {
                    boost::unique_lock<boost::mutex> lock(result_mutex_);
                    bool ret = false;
                    if (init_state == result_state_)
                    {
                        ret = result_cv_.timed_wait(lock, boost::posix_time::millisec(millsec));
                        if (!ret)
                        {
                            result_state_ = timeout_state;
                        }
                    }
                    result = (done_state == result_state_);
                }

                if (result)
                {
                    pl_->free_object(this);
                }
                return result;
            }

        private:
            boost::mutex result_mutex_;
            boost::condition_variable result_cv_;
            uint8_t result_state_;
        };
    }

    template<typename DriveT>
    class invoke_serization_t : 
        public utility::drived_base<DriveT>
    {
    public:
	typedef DriveT drived_type;

        void do_request(control_t& control, void *request, std::ostream& os)
        {
            const typename drived_type::func_info_t *info = drived_type::get_func_info(control.get_dispid());

            THROW_EXCEPTION(NULL == info);
            THROW_EXCEPTION(NULL == info->out_request);

            //序列化的异常原封不动抛出
            try
            {
                control.save(os);
                info->out_request(request, control.get_serialize_type(), os);
            }
            catch (...)
            {
                throw;
            }
        }

        //
        bool do_response(control_t& control, std::istream& is, std::ostream& os)
        {
            const typename drived_type::func_info_t*info = drived_type::get_func_info(control.get_dispid());

            THROW_EXCEPTION(NULL == info);

            switch (control.get_msg_type())
            {
            case control_t::func_request_msg:   //需要返回结果
                do_func_request_msg(info, control, is, os);
                break;
            case control_t::func_response_msg:  //不需要返回结果，执行回调即可
                return false;
                break;
            case control_t::event_request_msg:
                do_event_request_msg(info, control, is);
                break;
            default:
                return false;
                break;
            }

            return true;
        }

    private:
        void do_func_request_msg(const void *p, control_t& control, std::istream& is, std::ostream& os)
        {
            const typename drived_type::func_info_t*info = (const typename drived_type::func_info_t*)p;

            THROW_EXCEPTION(NULL == info->create_request);
            THROW_EXCEPTION(NULL == info->in_request);
            THROW_EXCEPTION(NULL == info->create_response);
            THROW_EXCEPTION(NULL == info->func);
            THROW_EXCEPTION(NULL == info->destroy_request);
            THROW_EXCEPTION(NULL == info->out_response);
            THROW_EXCEPTION(NULL == info->destroy_response);

            void *request = info->create_request();

            try
            {
                info->in_request(request, control.get_serialize_type(), is);
            }
            catch (...)
            {
                throw;
            }

            void *response = info->create_response();

            (this->get_drived()->*(info->func))(control, request, response, NULL);

            info->destroy_request(request);
            control.set_msg_type(control_t::func_response_msg);
            control.save(os);
            info->out_response(response, control.get_serialize_type(), os);
            info->destroy_response(response);
        }

        void do_event_request_msg(const void *p, control_t& control, std::istream& is)
        {
            const typename drived_type::func_info_t*info = (const typename drived_type::func_info_t*)p;

            THROW_EXCEPTION(NULL == info->create_request);
            THROW_EXCEPTION(NULL == info->in_request);
            THROW_EXCEPTION(NULL == info->event);
            THROW_EXCEPTION(NULL == info->destroy_request);

            void *request = info->create_request();

            try
            {
                info->in_request(request, control.get_serialize_type(), is);
            }
            catch (...)
            {
                throw;
            }

            (this->get_drived()->*(info->event))(control, request);

            info->destroy_request(request);
        }
    };

    class msg_dispatch_t
    {
    public:
        virtual void on_send_msg(const uint8_t *msg, uint32_t msg_len, closure_t *request_closure, recv_msg_closure_t *response_closure) = 0;

    protected:
        virtual ~msg_dispatch_t() {}
    };

    template<typename DriveT>
    class invoke_t : 
        public invoke_serization_t<DriveT>,
        public invoke_detail::use_pool_impl
    {
    public:
	typedef DriveT drived_type;

        template<typename SerializationT, typename RequestT, typename ResponseT>
        bool do_invoke(msg_dispatch_t *msg, dispid_t dispid, control_t& control, RequestT *request, ResponseT *response, closure_t *done)
        {
            const typename drived_type::func_info_t *info = drived_type::get_func_info(dispid);
            THROW_EXCEPTION(NULL == info);

            control.set_iid(this->get_drived()->get_register_iid());
            control.set_dispid(dispid);
            control.set_serialize_type(SerializationT::type_value);
            control.set_msg_type(control_t::func_request_msg);

            std::ostringstream os;
            this->do_request(control, request, os);


            invoke_detail::request_closure_t *request_closure = get_object<invoke_detail::request_closure_t>(this);
            invoke_detail::response_closure_t *response_closure = NULL;

            //处理同步调用
            invoke_detail::sync_wait_response_closure_t *sync_call_result_closure = NULL;
            if (NULL == done)
            {
                sync_call_result_closure = get_object<invoke_detail::sync_wait_response_closure_t>(control.get_serialize_type(), response, info->in_response, this);
                response_closure = sync_call_result_closure;
            }
            else
            {
                sync_call_result_closure = NULL;
                response_closure = get_object<invoke_detail::response_closure_t>(control.get_serialize_type(), response, info->in_response, done, this);
            }

            msg->on_send_msg((const uint8_t *)os.str().c_str(), (uint32_t)os.str().length(), request_closure, response_closure);

            return sync_call_result_closure ? sync_call_result_closure->wait_result(control.get_timeout()) : true;
        }

        template<typename SerializationT, typename RequestT>
        void do_event(msg_dispatch_t *msg, dispid_t dispid, control_t& control, RequestT *request)
        {
            const typename drived_type::func_info_t *info = drived_type::get_func_info(dispid);
            THROW_EXCEPTION(NULL == info);

            control.set_iid(this->get_drived()->get_register_iid());
            control.set_dispid(dispid);
            control.set_serialize_type(SerializationT::type_value);
            control.set_msg_type(control_t::event_request_msg);

            std::ostringstream os;
            this->do_request(control, request, os);

            invoke_detail::request_closure_t *request_closure = get_object<invoke_detail::request_closure_t>(this);
            msg->on_send_msg((const uint8_t *)os.str().c_str(), os.str().length(), request_closure, NULL);
        }

        bool do_result(channel_impl *impl, channel_t *channel, control_t&control, std::istream& is, uint32_t sequence)
        {
            if (pre_result(channel, control))
            {
                return true;
            }

            std::ostringstream os;
            bool result = this->do_response(control, is, os);
            if (result && !os.str().empty())
            {
                impl->x_send_msg(channel, (const uint8_t *)os.str().c_str(), (uint32_t)os.str().length(), get_object<invoke_detail::request_closure_t>(this), NULL, sequence);
            }

            return !result;
        }

        bool do_result(channel_impl *impl, channel_t *channel, uint8_t *data, uint32_t data_bytes, uint32_t sequence)
        {
            std::istringstream is(std::string((const char *)data, data_bytes));
            control_t control;

            try
            {
                control.load(is);
            }
            catch (const std::exception &e)
            {
                const char *what =  e.what();
                throw;
            }

            return do_result(impl, channel, control, is, sequence);
        }

        template<typename SerializationT>
        void do_connect_event(msg_dispatch_t *msg, bool connect)
        {
            control_t control;
            control.set_iid(this->get_drived()->get_register_iid());
            control.set_dispid(0);
            control.set_serialize_type(SerializationT::type_value);
            control.set_msg_type(connect ? control_t::event_connect_msg : control_t::event_disconnect_msg);

            std::ostringstream os;
            control.save(os);

            invoke_detail::request_closure_t *request_closure = get_object<invoke_detail::request_closure_t>(this);
            msg->on_send_msg((const uint8_t *)os.str().c_str(), (uint32_t)os.str().length(), request_closure, NULL);
        }

        void on_connnect_event(channel_t *, bool)
        {
            //do-nothing
        }

    private:
        bool pre_result(channel_t *channel, control_t& control)
        {
            if (control_t::event_connect_msg == control.get_msg_type())
            {
                this->get_drived()->on_connnect_event(channel, true);
                return true;
            }

            if (control_t::event_disconnect_msg == control.get_msg_type())
            {
                this->get_drived()->on_connnect_event(channel, false);
                return true;
            }

            return false;
        }
    };
}

#endif //_RPC_INVOKE_H_INCLUDED

