#ifndef _SDP_H_INCLUDED
#define _SDP_H_INCLUDED

#include <vector>
#include <list>
#include <iosfwd>
#include <memory>
#include <string>
#include <map>
#include <stdint.h>

#define _USE_SDP_ACCESS_PUBLIC

namespace xt_sdp
{
    class parse_buffer_t;

    class uri_t
    {
    public:
        std::ostream& encode(std::ostream&) const;
        void parse(parse_buffer_t& pb);

        const std::string& scheme() const { return scheme_; }
        const std::string& host() const { return host_; }
        const std::string& user() const { return user_; }
        const std::string& user_parameters() const { return user_parameters_; }
        int port() const { return port_; }
        const std::string& pwd() const { return pwd_; }

#ifdef _USE_SDP_ACCESS_PUBLIC
    public:
#else
    private:
#endif
        std::string scheme_;
        std::string host_;
        std::string user_;
        std::string user_parameters_;
        int port_;
        std::string pwd_;
        std::string net_ns_;
    };

    class mime_t
    {};

    class sdp_ctx_t;

    class attribute_helper_t
    {
    public:
        attribute_helper_t();
        attribute_helper_t(const attribute_helper_t& rhs);
        attribute_helper_t& operator=(const attribute_helper_t& rhs);

        bool exists(const std::string& key) const;
        const std::list<std::string>& get_values(const std::string& key) const;
        std::ostream& encode(std::ostream& s) const;
        void parse(parse_buffer_t& pb);
        void add_attribute(const std::string& key, const std::string& value = "");
        void clear_attribute(const std::string& key);
        void clear() { attributes_.clear(); attribute_list_.clear(); }

#ifdef _USE_SDP_ACCESS_PUBLIC
    public:
#else
    private:
#endif
        std::list<std::pair<std::string, std::string> > attribute_list_;  // used to ensure attribute ordering on encode
        std::map<std::string, std::list<std::string> > attributes_;
    };

    typedef enum { ipv4 = 1, ipv6 } addr_type;

    class sdp_session_t
    {
    public:
        class medium_t;

        class codec_t
        {
        public:
            codec_t() : name_(), rate_(0), payload_type_(-1) {}

            codec_t(const std::string& name, int payload, unsigned long rate, const std::string& parameters = "", const std::string& encoding_parameters = "");
            codec_t(const codec_t& rhs);
            codec_t& operator=(const codec_t& codec);

            void parse(parse_buffer_t& pb, const sdp_session_t::medium_t& medium, int payload);

            void assign_format_parameters(const sdp_session_t::medium_t& medium);

            const std::string& name() const { return name_; }

            int rate() const { return rate_; }

            int payload() const { return payload_type_; }
            int& payload() { return payload_type_; }

            const std::string& parameters() const { return parameters_; }
            std::string& parameters() { return parameters_; }

            const std::string& encoding_parameters() const { return encoding_parameters_; }
            std::string& encoding_parameters() { return encoding_parameters_; }

            static const codec_t ULaw_8000;
            static const codec_t ALaw_8000;
            static const codec_t G729_8000;
            static const codec_t G723_8000;
            static const codec_t GSM_8000;
            static const codec_t TelephoneEvent;
            static const codec_t FrfDialedDigit;
            static const codec_t CN;

            typedef std::map<int, codec_t> codec_map_t;
            static codec_map_t& get_static_codecs();

            friend bool operator==(const codec_t&, const codec_t&);

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
            std::string name_;
            unsigned long rate_;
            int payload_type_;
            std::string parameters_;  // Format parameters
            std::string encoding_parameters_;

            static std::auto_ptr<codec_map_t> s_static_codecs_;
            static bool s_static_codecs_created_;
            friend std::ostream& operator<<(std::ostream&, const codec_t&);
        };

        class origin_t
        {
        public:
            origin_t(const std::string& user, const uint64_t& sessionId, const uint64_t& version, addr_type addr, const std::string& address);
            origin_t(const origin_t& rhs);
            origin_t& operator=(const origin_t& rhs);

            void parse(parse_buffer_t& pb);
            std::ostream& encode(std::ostream&) const;

            const uint64_t& session_id() const { return session_id_; }
            uint64_t& session_id() { return session_id_; }
            const uint64_t& version() const { return version_; }
            uint64_t& version() { return version_; }
            const std::string& user() const { return user_; }
            std::string& user() { return user_; }
            addr_type address_type() const { return addr_type_; }
            const std::string& address() const { return address_; }
            void set_address(const std::string& host, addr_type type = ipv4);

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
            origin_t();

            std::string user_;
            uint64_t session_id_;
            uint64_t version_;
            addr_type addr_type_;
            std::string address_;

            friend class sdp_session_t;
        };

        class email_t
        {
        public:
            email_t(const std::string& address, const std::string& freeText);

            email_t(const email_t& rhs);
            email_t& operator=(const email_t& rhs);

            void parse(parse_buffer_t& pb);
            std::ostream& encode(std::ostream&) const;

            const std::string& address() const { return address_; }
            const std::string& free_text() const { return free_text_; }

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
            email_t() {}

            std::string address_;
            std::string free_text_;

            friend class sdp_session_t;
        };

        class phone_t
        {
        public:
            phone_t(const std::string& number, const std::string& freeText);
            phone_t(const phone_t& rhs);
            phone_t& operator=(const phone_t& rhs);

            void parse(parse_buffer_t& pb);
            std::ostream& encode(std::ostream&) const;

            const std::string& number() const { return number_; }
            const std::string& free_text() const { return free_text_; }

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
            phone_t() {}

            std::string number_;
            std::string free_text_;

            friend class sdp_session_t;
        };

        class connection_t
        {
        public:
            connection_t(addr_type addType, const std::string& address, unsigned long ttl = 0);
            connection_t(const connection_t& rhs);
            connection_t& operator=(const connection_t& rhs);

            void parse(parse_buffer_t& pb);
            std::ostream& encode(std::ostream&) const;

            addr_type address_type() const { return addr_type_; }
            const std::string& address() const { return address_; }
            void set_address(const std::string& host, addr_type type = ipv4);
            unsigned long ttl() const { return ttl_; }
            unsigned long& ttl() { return ttl_; }

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
            connection_t();

            addr_type addr_type_;
            std::string address_;
            unsigned long ttl_;

            friend class sdp_session_t;
            friend class medium_t;
        };

        class bandwidth_t
        {
        public:
            bandwidth_t(const std::string& modifier, unsigned long kbs);
            bandwidth_t(const bandwidth_t& rhs);
            bandwidth_t& operator=(const bandwidth_t& rhs);

            void parse(parse_buffer_t& pb);
            std::ostream& encode(std::ostream&) const;

            const std::string& modifier() const { return modifier_; }
            std::string& modifier() { return modifier_; }

            unsigned long kbs() const { return kbs_; }
            unsigned long& kbs() { return kbs_; }

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
            bandwidth_t() {}
            std::string modifier_;
            unsigned long kbs_;

            friend class sdp_session_t;
            friend class medium_t;
        };

        class time_t
        {
        public:
            time_t(unsigned long start, unsigned long stop);
            time_t(const time_t& rhs);
            time_t& operator=(const time_t& rhs);

            void parse(parse_buffer_t& pb);
            std::ostream& encode(std::ostream&) const;

            class repeat_t
            {
            public:
                repeat_t(unsigned long interval, unsigned long duration, std::list<int> offsets);

                void parse(parse_buffer_t& pb);
                std::ostream& encode(std::ostream&) const;

                unsigned long interval() const { return interval_; }
                unsigned long duration() const { return duration_; }
                const std::list<int>& offsets() const { return offsets_; }

#ifdef _USE_SDP_ACCESS_PUBLIC
            public:
#else
            private:
#endif
                repeat_t() {}
                unsigned long interval_;
                unsigned long duration_;
                std::list<int> offsets_;

                friend class time_t;
            };

            void add_repeat(const repeat_t& repeat);
            unsigned long start() const { return start_; }
            unsigned long stop() const { return stop_; }
            const std::list<repeat_t>& repeats() const { return repeats_; }

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
            time_t() {}
            unsigned long start_;
            unsigned long stop_;
            std::list<repeat_t> repeats_;

            friend class sdp_session_t;
        };

        class timezones_t
        {
        public:
            class adjustment_t
            {
            public:
                adjustment_t(unsigned long time, int offset);
                adjustment_t(const adjustment_t& rhs);
                adjustment_t& operator=(const adjustment_t& rhs);

                unsigned long time;
                int offset;
            };

            timezones_t();
            timezones_t(const timezones_t& rhs);
            timezones_t& operator=(const timezones_t& rhs);

            void parse(parse_buffer_t& pb);
            std::ostream& encode(std::ostream&) const;

            void add_adjustment(const adjustment_t& adjusment);
            const std::list<adjustment_t>& adjustments() const { return adjustments_; }

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
            std::list<adjustment_t> adjustments_;
        };

        class encryption_t
        {
        public:
            typedef enum { no_encryption = 0, prompt, clear, base64, uri_key } key_type;

            encryption_t(const key_type& method, const std::string& key);
            encryption_t(const encryption_t& rhs);
            encryption_t& operator=(const encryption_t& rhs);

            void parse(parse_buffer_t& pb);
            std::ostream& encode(std::ostream&) const;

            const key_type& method() const { return method_; }
            key_type& method() { return method_; }
            const std::string& key() const { return key_; }
            std::string& key() { return key_; }

            encryption_t();

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
            key_type method_;
            std::string key_;
        };

        class medium_t
        {
        public:
            medium_t();
            medium_t(const medium_t& rhs);

            medium_t(const std::string& name, unsigned long port, unsigned long multicast, const std::string& protocol);
            medium_t& operator=(const medium_t& rhs);

            void parse(parse_buffer_t& pb);
            std::ostream& encode(std::ostream&) const;

            void add_format(const std::string& format);
            void set_connection(const connection_t& connection);
            void add_connection(const connection_t& connection);
            void set_bandwidth(const bandwidth_t& bandwidth);
            void add_bandwidth(const bandwidth_t& bandwidth);
            void add_attribute(const std::string& key, const std::string& value = "");

            const std::string& name() const { return name_; }
            std::string& name() { return name_; }

            int port() const { return port_; }
            unsigned long& port() { return port_; }

            int multicast() const { return multicast_; }
            unsigned long& multicast() { return multicast_; }

            const std::string& protocol() const { return protocol_; }
            std::string& protocol() { return protocol_; }

            typedef std::list<codec_t> codec_container_t;
            const codec_container_t& codecs() const;
            codec_container_t& codecs();
            void clear_codecs();
            void add_codec(const codec_t& codec);

            const std::list<std::string>& formats() const { return formats_; }

            const std::string& information() const { return information_; }
            std::string& information() { return information_; }

            const std::list<bandwidth_t>& bandwidths() const { return bandwidths_; }
            std::list<bandwidth_t>& bandwidths() { return bandwidths_; }

            const std::list<connection_t> connections() const;

            const std::list<connection_t>& get_medium_connections() const { return connections_; }
            std::list<connection_t>& get_medium_connections() { return connections_; }
            const encryption_t& encryption() const { return encryption_; }
            encryption_t& encryption() { return encryption_; }
            bool exists(const std::string& key) const;
            const std::list<std::string>& get_values(const std::string& key) const;

            void clear_attribute(const std::string& key);

            const codec_t& find_first_matching_codecs(const codec_container_t& codecs, codec_t* pMatchingCodec = 0) const;
            const codec_t& find_first_matching_codecs(const medium_t& medium, codec_t* pMatchingCodec = 0) const;

            int find_telephone_event_payload_type() const;

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
            void set_session(sdp_session_t* session);
            sdp_session_t* session_;

            std::string name_;
            unsigned long port_;
            unsigned long multicast_;
            std::string protocol_;
            std::list<std::string> formats_;
            codec_container_t codecs_;
            std::string transport_;
            std::string information_;
            std::list<connection_t> connections_;
            std::list<bandwidth_t> bandwidths_;
            encryption_t encryption_;
            attribute_helper_t attribute_helper_;

            bool rtp_map_done_;
            typedef std::map<int, codec_t> rtp_map_t;
            rtp_map_t rtp_map_;

            friend class sdp_session_t;
        };

        sdp_session_t(int version, const origin_t& origin, const std::string& name);

        sdp_session_t() : version_(0) {}
        sdp_session_t(const sdp_session_t& rhs);
        sdp_session_t& operator=(const sdp_session_t& rhs);

        void parse(parse_buffer_t& pb);
        std::ostream& encode(std::ostream&) const;

        int version() const { return version_; }
        int& version() { return version_; }

        const origin_t& origin() const { return origin_; }
        origin_t& origin() { return origin_; }

        const std::string& name() const { return name_; }
        std::string& name() { return name_; }

        const std::string& information() const { return information_; }
        std::string& information() { return information_; }

        const uri_t& uri() const { return uri_; }
        uri_t& uri() { return uri_; }

        const std::list<email_t>& emails() const { return emails_; }

        const std::list<phone_t>& phones() const { return phones_; }

        const connection_t& connection() const { return connection_; }
        connection_t& connection() { return connection_; } // !dlb! optional?
        bool is_connection() const { return connection_.address_ != ""; }

        const std::list<bandwidth_t>& bandwidths() const { return bandwidths_; }
        std::list<bandwidth_t>& bandwidths() { return bandwidths_; }

        const std::list<time_t>& times() const { return times_; }
        std::list<time_t>& times() { return times_; }

        const timezones_t& timezones() const { return timezones_; }
        const encryption_t& encryption() const { return encryption_; }
        encryption_t& encryption() { return encryption_; }

        typedef std::list<medium_t> medium_container_t;
        const medium_container_t& media() const { return media_; }
        medium_container_t& media() { return media_; }

        void add_email(const email_t& email);

        void add_phone(const phone_t& phone);

        void add_bandwidth(const bandwidth_t& bandwidth);

        void add_time(const time_t& t);

        void add_medium(const medium_t& medium);
        void clear_medium() { media_.clear(); }

        void clear_attribute(const std::string& key);
        void add_attribute(const std::string& key, const std::string& value = "");
        bool exists(const std::string& key) const;
        const std::list<std::string>& get_values(const std::string& key) const;

#ifdef _USE_SDP_ACCESS_PUBLIC
        public:
#else
        private:
#endif
        int version_;
        origin_t origin_;
        std::string name_;
        medium_container_t media_;

        std::string information_;
        uri_t uri_;
        std::list<email_t> emails_;
        std::list<phone_t> phones_;
        connection_t connection_;
        std::list<bandwidth_t> bandwidths_;
        std::list<time_t> times_;
        timezones_t timezones_;
        encryption_t encryption_;
        attribute_helper_t attribute_helper_;

        friend class sdp_ctx_t;
    };

    typedef sdp_session_t::codec_t codec_t;

    bool operator==(const sdp_session_t::codec_t& lhs, const sdp_session_t::codec_t& rhs);

    std::ostream& operator<<(std::ostream& s, const sdp_session_t::codec_t& codec);

    void skip_eol(parse_buffer_t& pb);
}

#endif
