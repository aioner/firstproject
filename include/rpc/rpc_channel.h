#ifndef _RPC_CHANNEL_H_INCLUDED
#define _RPC_CHANNEL_H_INCLUDED

#include "rpc_closure.h"
#include <stdint.h>
#include <string>
#include <map>
#include <memory>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/atomic.hpp>

namespace rpc
{
    class channel_t
    {
    public:
        class events_type
        {
        public:
            virtual void on_channel_connect(channel_t *) = 0;
            virtual void on_channel_recv_raw_data(channel_t *, uint32_t sequence, uint8_t *, uint32_t) = 0;
            virtual void on_channel_close(channel_t *) = 0;
            virtual void on_process_exit(channel_t *) = 0;
        };

        virtual void close() = 0;
        virtual bool send_msg(uint32_t sequence, const uint8_t *, uint32_t, closure_t *) = 0;
        virtual bool register_events(events_type *ev) = 0;
    };

    class channel_impl : public channel_t::events_type
    {
    public:
        typedef recv_msg_closure_t operations_closure_type;
        typedef std::map<uint32_t, operations_closure_type *> operations_type;
        typedef boost::shared_ptr<operations_type> operations_ptr;
        typedef std::map<channel_t *, operations_ptr> operations_container_type;
        typedef boost::recursive_mutex mutex_type;

        explicit channel_impl()
            :sequence_(1),
             channel_(NULL)
        {}

        ~channel_impl()
        {
            if (NULL != channel_)
            {
                channel_->close();
            }
        }

        void set_channel(channel_t *channel)
        {
            channel_ = channel;
            channel_->register_events(this);
        }

        channel_t *get_channel()
        {
            return channel_;
        }

        bool send_msg(const uint8_t *data, uint32_t data_bytes, closure_t *request, operations_closure_type *response)
        {
            return x_send_msg(channel_, data, data_bytes, request, response);
        }

        bool x_send_msg(channel_t *channel, const uint8_t *data, uint32_t data_bytes, closure_t *request, operations_closure_type *response, uint32_t sequence = 0)
        {
            return do_channel_send_msg(channel, data, data_bytes, request, response, sequence);
        }

    protected:
        void on_channel_recv_raw_data(channel_t *channel, uint32_t sequence, uint8_t *data, uint32_t data_bytes)
        {
            if (on_channel_recv_msg(channel, sequence, data, data_bytes))
            {
                do_channel_recv_msg(channel, sequence, data, data_bytes);
            }
        }

        void on_channel_close(channel_t *channel)
        {
            //todo:ÐèÒªÊÍ·Åclosure_t
            mutex_type::scoped_lock lock(mutex_);
            operations_container_.erase(channel);
        }

        void on_channel_connect(channel_t *) {}

        virtual bool on_channel_recv_msg(channel_t *, uint32_t, uint8_t *, uint32_t)  { return true; }
    private:
        bool do_channel_send_msg(channel_t *channel, const uint8_t *data, uint32_t data_bytes, closure_t *request, operations_closure_type *response, uint32_t sequence = 0)
        {
            if (NULL != response)
            {
                mutex_type::scoped_lock lock(mutex_);

                operations_container_type::iterator result = operations_container_.find(channel);
                operations_ptr operations;
                if (operations_container_.end() == result)
                {
                    operations.reset(new operations_type);
                    operations_container_.insert(operations_container_type::value_type(channel, operations));
                }
                else
                {
                    operations = result->second;
                }

                if (0 == sequence)
                {
                    sequence = sequence_++;
                }

                operations->operator [](sequence) = response;
            }

            return channel->send_msg(sequence, data, data_bytes, request);
        }

        bool do_channel_recv_msg(channel_t *channel, uint32_t sequence, uint8_t *data, uint32_t data_bytes)
        {
            mutex_type::scoped_lock lock(mutex_);

            operations_container_type::iterator result = operations_container_.find(channel);
            if (operations_container_.end() == result)
            {
                return false;
            }

            operations_type::iterator op = result->second->find(sequence);
            if (result->second->end() == op)
            {
                return false;
            }

            op->second->done(data, data_bytes);
            result->second->erase(op);
            return true;
        }

        boost::atomic_uint32_t sequence_;
        channel_t *channel_;
        operations_container_type operations_container_;

        mutex_type mutex_;
    };
}

#endif//_RPC_CHANNEL_H_INCLUDED
