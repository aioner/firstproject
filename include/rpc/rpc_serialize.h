#ifndef _RPC_SERIALIZE_H_INCLUDED
#define _RPC_SERIALIZE_H_INCLUDED

#include "rpc_config.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif

#ifdef _USE_TEXT_ARCHIVE
    #include <boost/archive/text_iarchive.hpp>
    #include <boost/archive/text_oarchive.hpp>
#endif

#ifdef _USE_XML_ARCHIVE
    #include <boost/archive/xml_iarchive.hpp>
    #include <boost/archive/xml_oarchive.hpp>
#endif

#ifdef _USE_BINARY_ARCHIVE
    #include <boost/archive/binary_iarchive.hpp>
    #include <boost/archive/binary_oarchive.hpp>
#endif

#ifdef _USE_SERIALIZATION_STRING
    #include <boost/serialization/string.hpp>
#endif

#ifdef _USE_SERIALIZATION_VECTOR
    #include <boost/serialization/vector.hpp>
#endif

#ifdef _USE_SERIALIZATION_LIST
    #include <boost/serialization/list.hpp>
#endif

#ifdef _USE_SERIALIZATION_MAP
#include <boost/serialization/map.hpp>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <iostream>

#define RPC_SERIALIZARTION_NVP  BOOST_SERIALIZATION_NVP

namespace rpc
{
    namespace serialization
    {
        enum type
        {
            invalid_type,
            text_type,
            xml_type,
            binary_type
        };

        template<serialization::type Type, typename StreamT>
        struct archive_traits
        {};

#ifdef _USE_TEXT_ARCHIVE
        struct text
        {
            typedef boost::archive::text_iarchive iarchive;
            typedef boost::archive::text_oarchive oarchive;

            enum { type_value = text_type };
        };

        template<>
        struct archive_traits<text_type, std::ostream>
        {
            typedef text::oarchive type;
        };

        template<>
        struct archive_traits<text_type, std::istream>
        {
            typedef text::iarchive type;
        };
#endif

#ifdef _USE_XML_ARCHIVE
        struct xml
        {
            typedef boost::archive::xml_iarchive iarchive;
            typedef boost::archive::xml_oarchive oarchive;

            enum { type_value = xml_type };
        };

        template<>
        struct archive_traits<xml_type, std::ostream>
        {
            typedef xml::oarchive type;
        };

        template<>
        struct archive_traits<xml_type, std::istream>
        {
            typedef xml::iarchive type;
        };
#endif
        using boost::serialization::make_nvp;

#ifdef _USE_BINARY_ARCHIVE
        struct binary
        {
            typedef boost::archive::binary_iarchive iarchive;
            typedef boost::archive::binary_oarchive oarchive;

            enum { type_value = binary_type };
        };

        template<>
        struct archive_traits<binary_type, std::ostream>
        {
            typedef binary::oarchive type;
        };

        template<>
        struct archive_traits<binary_type, std::istream>
        {
            typedef binary::iarchive type;
        };
#endif

        template<typename ArchiveT, typename ObjT, typename StreamT>
        void serialize_object(ObjT& obj, StreamT& stream)
        {
            typename archive_traits<(serialization::type)ArchiveT::type_value, StreamT>::type ar(stream);
            ar & obj;
        }

        template<typename ObjT, typename StreamT>
        void serialize_object(serialization::type which, ObjT& obj, StreamT& stream)
        {
            switch (which)
            {
#ifdef _USE_TEXT_ARCHIVE
            case text_type:
                serialize_object<text>(obj, stream);
                break;
#endif
#ifdef _USE_XML_ARCHIVE
            case xml_type:
                serialize_object<xml>(obj, stream);
                break;
#endif
#ifdef _USE_BINARY_ARCHIVE
            case binary_type:
                serialize_object<binary>(obj, stream);
                break;
#endif
            default:
                assert(false);
                break;
            }
        }
    };
}
#endif //_RPC_SERIALIZE_H_INCLUDED
