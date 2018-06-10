#ifndef _SYNC_DEQUE_H_INCLUDED
#define _SYNC_DEQUE_H_INCLUDED

#include "boost/thread/mutex.hpp"
#include "boost/thread/condition_variable.hpp"
#include "boost/container/deque.hpp"

namespace framework
{
    template<typename T>
    class sync_deque_t
    {
    public:
        typedef T value_type;
        typedef std::size_t size_type;

        sync_deque_t()
            :mtx_(),
            not_empty_(),
            waiting_empty_(0),
            data_(),
            closed_(false)
        {}

        ~sync_deque_t()
        {}

        bool empty() const
        {
            boost::unique_lock<boost::mutex> lk(mtx_);
            return empty(lk);
        }

        size_type size() const
        {
            boost::unique_lock<boost::mutex> lk(mtx_);
            return data_.size();
        }

        bool closed() const
        {
            boost::unique_lock<boost::mutex> lk(mtx_);
            return closed_;
        }

        void close()
        {
            {
                boost::unique_lock<boost::mutex> lk(mtx_);
                closed_ = true;
            }
            not_empty_.notify_all();
        }

        void push_back(const value_type& v)
        {
            try
            {
                boost::unique_lock<boost::mutex> lk(mtx_);
                throw_if_close(lk);
                data_.push_back(v);
                notify_not_empty_if_needed(lk);
            }
            catch (...)
            {
                throw;
                close();
            }
        }

        void push_front(const value_type& v)
        {
            try
            {
                boost::unique_lock<boost::mutex> lk(mtx_);
                throw_if_close(lk);
                data_.push_front(v);
                notify_not_empty_if_needed(lk);
            }
            catch (...)
            {
                throw;
                close();
            }
        }

        void wait_pull_back(value_type& v, bool& closed)
        {
            try
            {
                boost::unique_lock<boost::mutex> lk(mtx_);
                wait_until_not_empty(lk, closed);
                if (!closed)
                {
                    v = data_.back();
                    data_.pop_back();
                }
            }
            catch (...)
            {
                throw;
                close();
            }
        }

        void wait_pull_front(value_type& v, bool& closed)
        {
            try
            {
                boost::unique_lock<boost::mutex> lk(mtx_);
                wait_until_not_empty(lk, closed);
                if (!closed)
                {
                    v = data_.front();
                    data_.pop_front();
                }
            }
            catch (...)
            {
                throw;
                close();
            }
        }
    private:
        void throw_if_close(boost::unique_lock<boost::mutex>&)
        {
            if (closed_)
            {
                throw std::runtime_error("sync_deque closed.");
            }
        }

        bool empty(boost::unique_lock<boost::mutex>&) const
        {
            return data_.empty();
        }

        void wait_until_not_empty(boost::unique_lock<boost::mutex>& lk, bool& closed)
        {
            while (empty(lk))
            {
                if (closed_)
                {
                    closed = true;
                    return;
                }

                ++waiting_empty_;
                not_empty_.wait(lk);
            }

            closed = false;
        }

        void notify_not_empty_if_needed(boost::unique_lock<boost::mutex>& lk)
        {
            if (waiting_empty_ > 0)
            {
                --waiting_empty_;
                lk.unlock();
                not_empty_.notify_one();
            }
        }
    private:
        mutable boost::mutex mtx_;
        boost::condition_variable not_empty_;
        size_type waiting_empty_;
        boost::container::deque<T> data_;
        bool closed_;
    };
}

#endif //_SYNC_DEQUE_H_INCLUDED
