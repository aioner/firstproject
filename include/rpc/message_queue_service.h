#ifndef _MESSAGE_QUEUE_SERVICE_H_INCLUDED
#define _MESSAGE_QUEUE_SERVICE_H_INCLUDED

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <list>

namespace rpc
{
    typedef boost::function<bool ()> operation_t;

    class operations_t
    {
    public:
        typedef std::list<operation_t> operation_container_t;

        template<typename HandlerT>
        void add(HandlerT h)
        {
            boost::unique_lock<boost::recursive_mutex> _lock(mutex_);
            operations_.push_back(h);
        }

        std::size_t run()
        {
            std::size_t done_count = 0;
            boost::unique_lock<boost::recursive_mutex> _lock(mutex_);
            for (operation_container_t::iterator it = operations_.begin(); operations_.end() != it; )
            {
                if ((*it)())
                {
                    done_count++;
                    it = operations_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            return done_count;
        }
    private:
        boost::recursive_mutex mutex_;
        operation_container_t operations_;
    };

    class message_queue_service_t;

    class message_queue_t
    {
    public:
        enum status_t
        {
            invalid,
            create_only,
            open_only
        };

        message_queue_t(operations_t& operations)
            :operations_(operations),
            impl_(),
            name_(),
            stat_(invalid)
        {}

        ~message_queue_t()
        {
            remove_self();
        }

        bool create(const std::string &name, uint32_t max_num_msg, uint32_t max_msg_size)
        {
            remove(name);

            try
            {
                impl_.reset(new boost::interprocess::message_queue(boost::interprocess::create_only, name.c_str(), max_num_msg, max_msg_size));
            }
            catch (const std::exception& e)
            {
                std::cerr << "message_queue do_create fail." << e.what() << "." << std::endl;
                return false;
            }

            name_ = name;
            stat_ = create_only;

            return true;
        }

        bool open(const std::string &name)
        {
            try
            {
                impl_.reset(new boost::interprocess::message_queue(boost::interprocess::open_only, name.c_str()));
            }
            catch (const std::exception& e)
            {
                std::cerr << "message_queue do_open fail." << e.what() << "." << std::endl;
                return false;
            }

            name_ = name;
            stat_ = open_only;

            return true;
        }

        void async_send(const void *buffer, size_t buffer_size, const boost::function<void ()>&handler)
        {
            boost::function<bool ()> condition = boost::bind(&message_queue_t::try_send, this, buffer, buffer_size);
            operations_.add(boost::bind(&message_queue_t::condition_op<boost::function<bool ()>, boost::function<void ()> >, condition, handler));
        }

        void async_receive(void *buffer, size_t buffer_size, size_t& recvd_size, const boost::function<void ()>&handler)
        {
             boost::function<bool ()> condition = boost::bind(&message_queue_t::try_receive, this, buffer, buffer_size, boost::ref(recvd_size));
             operations_.add(boost::bind(&message_queue_t::condition_op<boost::function<bool ()>, boost::function<void ()> >, condition, handler));
        }

        template<typename ConditionT>
        void async_condition(ConditionT c, const boost::function<void ()>&handler)
        {
            boost::function<bool ()> condition = boost::bind(c);
            operations_.add(boost::bind(&message_queue_t::condition_op<boost::function<bool ()>, boost::function<void ()> >, condition, handler));
        }

        void send(const void *buffer, size_t buffer_size)
        {
            impl_->send(buffer, buffer_size, 0);
        }

        void receive(void *buffer, size_t buffer_size, size_t& recvd_size)
        {
            unsigned int priority = 0;
            impl_->receive(buffer, buffer_size, recvd_size, priority);
        }

        bool timed_send(const void *buffer, size_t buffer_size, uint32_t millsec)
        {
            using namespace boost::posix_time;
            return impl_->timed_send(buffer, buffer_size, 0, microsec_clock::universal_time() + milliseconds(millsec));
        }

        bool timed_receive(void *buffer, size_t buffer_size, size_t& recvd_size, uint32_t millsec)
        {
            using namespace boost::posix_time;
            unsigned int priority = 0;
            return impl_->timed_receive(buffer, buffer_size, recvd_size, priority, microsec_clock::universal_time() + milliseconds(millsec));
        }

        uint32_t get_num_msg() const
        {
            return impl_->get_num_msg();
        }

        uint32_t get_max_msg_size() const
        {
            return impl_->get_max_msg_size();
        }

        const std::string& get_name() const
        {
            return name_;
        }

        inline message_queue_service_t& get_service();

    private:
        template<typename ConditionT, typename HandlerT>
        static bool condition_op(ConditionT condition, HandlerT handler)
        {
            if (!condition())
            {
                return false;
            }

            handler();
            return true;
        }

        bool try_send(const void *buffer, size_t buffer_size)
        {
            return impl_->try_send(buffer, buffer_size, 0);
        }

        bool try_receive(void *buffer, size_t buffer_size, size_t& recvd_size)
        {
            unsigned int priority = 0;
            return impl_->try_receive(buffer, buffer_size, recvd_size, priority);
        }

        bool remove_self()
        {
            if (create_only == stat_)
            {
                return remove(name_);
            }
            else
            {
                return true;
            }
        }

        static bool remove(const std::string&name)
        {
             return boost::interprocess::message_queue::remove(name.c_str());
        }

        operations_t& operations_;
        std::auto_ptr<boost::interprocess::message_queue> impl_;
        std::string name_;
        status_t stat_;
    };

    class message_queue_service_t : public operations_t
    {
    public:
        void run()
        {
            while (run_one() > 0)
            {
                boost::this_thread::yield();
            }
        }

        std::size_t run_one()
        {
            return operations_t::run();
        }
    };

    inline message_queue_service_t& message_queue_t::get_service()
    {
        return static_cast<message_queue_service_t&>(operations_);
    }
}
#endif //_MESSAGE_QUEUE_SERVICE_H_INCLUDED
