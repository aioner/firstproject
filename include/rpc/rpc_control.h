#ifndef _RPC_CONTROL_H_INCLUDED
#define _RPC_CONTROL_H_INCLUDED

#include "rpc_types.h"
#include "rpc_serialize.h"

namespace rpc
{
    class control_t
    {
    public:
        typedef serialization::binary serialization_type;

        enum msg_type
        {
            invalid_msg = -1,
            func_request_msg = 0,
            func_response_msg,
            event_request_msg,
            event_connect_msg,
            event_disconnect_msg
        };

        explicit control_t()
            :iid_(0),
            dispid_(0),
            timeout_(0),
            serialize_type_(serialization::invalid_type),
            msg_type_(invalid_msg)
        {}

        iid_t get_iid() const
        {
            return iid_;
        }
        void set_iid(iid_t iid)
        {
            iid_ = iid;
        }

        dispid_t get_dispid() const
        {
            return dispid_;
        }
        void set_dispid(dispid_t dispid)
        {
            dispid_ = dispid;
        }

        uint32_t get_timeout() const
        {
            return timeout_;
        }
        void set_timeout(uint32_t timeout)
        {
            timeout_ = timeout;
        }

        serialization::type get_serialize_type() const
        {
            return serialize_type_;
        }
        void set_serialize_type(uint32_t serialize_type)
        {
            serialize_type_ = (serialization::type)serialize_type;
        }

        msg_type get_msg_type() const
        {
            return msg_type_;
        }
        void set_msg_type(msg_type type)
        {
            msg_type_ = type;
        }

        void save(std::ostream& os)
        {
            serialization_type::oarchive ar(os);
            ar << *this;
        }

        void load(std::istream& is)
        {
            serialization_type::iarchive ar(is);
            ar >> *this;
        }

        template<typename Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & iid_ & dispid_  & timeout_ & serialize_type_ & msg_type_;
        }

    private:
        iid_t iid_;
        dispid_t dispid_;
        uint32_t timeout_;
        serialization::type serialize_type_;
        msg_type msg_type_; 
    };
}
#endif //_RPC_CONTROL_H_INCLUDED
