/*
 * Sdp.h
 *
 *  Created on: 2013Äê12ÔÂ6ÈÕ
 *      Author: zhengchuanjiang
 */

#ifndef SDP_H_
#define SDP_H_

//#include "BasicType.h"
#include <string>


namespace sdp
{

enum SendRecvMode
{
    kSendRecv = 0,
    kRecvOnly,
    kSendonly,
    kInactive
};


////////////////////////////////////////////////////////////////////////////

namespace detail
{
    class StringMap;
}

class /*DLLEXPORT*/ Attributes
{
public:
    Attributes();
    ~Attributes();

    Attributes(const Attributes& other);
    Attributes& operator = (const Attributes& other);

    bool operator == (const Attributes& other);
    bool operator < (const Attributes& other);

    void swap(Attributes& other);

    /**
     * insert element. insertion will NOT happen if key already exist
     * @param key
     * @param value
     * @return true if key does not exist
     */
    bool insert(const std::string& key, const std::string& value);

    /**
     * replace value with specified key
     * @param key
     * @param value
     * @return true if key does not exist
     */
    bool replace(const std::string& key, const std::string& value);

    std::string& operator [](const char* key);

    std::string& operator [](const std::string& key);

    size_t size() const;

    bool empty() const;

    bool getAt(size_t idx, std::string& value) const;
    bool getAt(size_t idx, std::string& key, std::string& value) const;

    bool find(const std::string& key, std::string& value) const;
    bool find(const char* key, std::string& value) const;
    bool find(const char* key, long long& value) const;
    bool find(const char* key, int& value) const;
    bool find(const char* key, double& value) const;

    bool exists(const std::string& key) const;
    bool exists(const char* key) const;

    void clear();

    bool erase(const std::string& key);

    bool erase(const char* key);

    bool findWithValue(const std::string& value, std::string& key) const;

public:
    detail::StringMap*  m_pMap;

};

////////////////////////////////////////////////////////////////////////////
class /*DLLEXPORT*/ Util
{
public:
    static int icompare(const std::string& src, const char* target);

    static int split(const std::string& src, char sp, std::string& head, std::string& tail);

    static int split(const std::string& src, char sp, size_t start,
            std::string& head, std::string& tail);

    static bool tail(const std::string& src, char sp, std::string& str);
    static bool head(const std::string& src, char sp, std::string& value);

    static bool tail(const std::string& src, char sp, int& value);



};

class /*DLLEXPORT*/ StringToken
{
public:
    StringToken(const std::string& src, char sp, size_t start = 0);

    bool next(std::string& section);

    bool next(int& value);
private:
    std::string m_src;
    char    m_sp;
    size_t  m_pos;

};

class /*DLLEXPORT*/ AttributeParser
{
public:
    static bool parse(const std::string& str, char eol, char equal, Attributes& attr);

};

/*DLLEXPORT*/ SendRecvMode findMode(const Attributes& attr);
/*DLLEXPORT*/ const char* getName(SendRecvMode mode);

////////////////////////////////////////////////////////////////////////////
class  /*DLLEXPORT*/ ConnectionData
{
public:
    std::string  nettype;
    std::string  addrtype;
    std::string  address;

    ConnectionData():
        nettype(),
        addrtype(),
        address()
    {
    }

    void clear()
    {
        nettype.clear();
        addrtype.clear();
        address.clear();
    }
};


class /*DLLEXPORT*/ SessionOrigin
{
public:
    std::string  username;
    std::string  sessionID;
    std::string  version;

    ConnectionData  addr;

    SessionOrigin():
        username(),
        sessionID(),
        version(),
        addr()
    {
    }

};

class /*DLLEXPORT*/ Bandwidth
{
public:
    std::string     bwtype;
    int     bandwidth;      /// kbps

    Bandwidth():
        bwtype(),
        bandwidth()
    {
    }

    void clear()
    {
        bwtype.clear();
        bandwidth = 0;
    }
};


class /*DLLEXPORT*/ TimeDescription
{
public:
    double start;
    double stop;

    TimeDescription():
        start(),
        stop()
    {
    }

    double getDuration() const
    {
        return stop - start;
    }

    void clear()
    {
        start = 0;
        stop = 0;
    }
};


class /*DLLEXPORT*/ EncryptionKey
{
public:
    std::string method;
    std::string key;

    EncryptionKey():
        method(),
        key()
    {
    }

    void clear()
    {
        method.clear();
        key.clear();
    }
};

class /*DLLEXPORT*/ Rtpmap
{
public:
    int payload;
    std::string name;
    int clockRate;
    int channels;
    std::string param;

    Rtpmap():
        payload(),
        name(),
        clockRate(9000),
        channels(1),
        param()
    {
    }

};



class /*DLLEXPORT*/ H264Fmtp
{
public:
    int format;
    int packetization_mode;
    std::string  profile_level_id;
    std::string  sprop_parameter_sets;

    H264Fmtp():
        format(),
        packetization_mode(),
        profile_level_id(),
        sprop_parameter_sets()
    {
    }

    bool parse(const std::string& line);
    
};

class /*DLLEXPORT*/ AacFmtp
{
public:
    std::string  mode;
    int     streamType;
    int     indexDeltaLength;
    int     indexLength;
    int     sizeLength;
    std::string  config;

    AacFmtp():
        mode(),
        streamType(),
        indexDeltaLength(),
        indexLength(),
        sizeLength(),
        config()
    {
    }

    bool parse(const std::string& line);
};


class /*DLLEXPORT*/ MediaDescription
{
public:
    enum Constant
    {
        kMaxFormat = 8
    };

public:
    std::string  media;     /// "audio", "video", "text", "application", and "message",
    int     port;
    int     number;
    std::string  proto;  //transport protocol

    int     fmtCount;
    int     formats[kMaxFormat];

    Rtpmap  rtpmaps[kMaxFormat];

    std::string     title;
    ConnectionData  addr;

    Attributes  attributes;

    Bandwidth   bandwidth;
    EncryptionKey   key;

    std::string     fmtps[kMaxFormat];
    std::string     control;

    MediaDescription():
        media(),
        port(),
        number(),
        proto(),
        fmtCount(),
        title(),
        addr(),
        attributes(),
        bandwidth(),
        key(),
        control()
    {
        for (size_t i = 0; i < kMaxFormat; i ++)
        {
            formats[i] = 0;
        }
    }

    bool isVideo() const
    {
        return (media == "video");
    }

    bool isAudio() const
    {
        return (media == "audio");
    }

    bool isH264() const
    {
        return Util::icompare(getRtpmap().name, "h264") == 0;
    }

    bool isAac() const
    {
        return Util::icompare(getRtpmap().name, "mpeg4-generic") == 0;
    }

    int getFormat() const
    {
        return formats[0];
    }

    const Rtpmap& getRtpmap() const
    {
        return rtpmaps[0];
    }

    std::string getFmtp() const
    {
        return fmtps[0];
    }

    SendRecvMode getMode() const
    {
        return findMode(attributes);
    }

    std::string getControl() const
    {
        std::string url;
        attributes.find("control", url);
        return url;
    }

};


class MediaDescriptionArray;

class /*DLLEXPORT*/ SessionDescription
{
public:
    enum Constant
    {
        kMaxMedia = 8
    };

public:
    SessionDescription();
    ~SessionDescription();

    int m_version;
    SessionOrigin  m_origin;
    std::string  m_name;    /// session name
    std::string  m_info;
    std::string  m_uri;
    std::string  m_email;
    std::string  m_phone;

    ConnectionData  m_connection;
    Bandwidth       m_bandwidth;

    TimeDescription m_timeDesc;
    std::string m_repeatTime;
    std::string m_timeZone;
    EncryptionKey   m_key;

    Attributes  m_attributes;

    bool parse(const char* str, size_t length);

    bool parse(const std::string& str);

    size_t getMediaCount() const;

    MediaDescription& getMedia(size_t idx);

    // tool method

    SessionDescription(const SessionDescription& other);

    void assign(const SessionDescription& other);

    SessionDescription& operator = (const SessionDescription& other);

    bool hasMedia();

    void clear();

    SendRecvMode getMode() const;

    std::string getControl() const;

    bool getRange(double& start, double& stop);

private:
    bool parseLine(const std::string& line);

    bool parseOrigin(const std::string& line, SessionOrigin& origin);
    bool parseTime(const std::string& line, TimeDescription& timeDesc);

    bool parseAddr(const std::string& line, ConnectionData& conn);
    bool parseBandwidth(const std::string& line, Bandwidth& bandwidth);
    bool parseKey(const std::string& line, EncryptionKey& key);

    bool parseMedia(const std::string& line);
    bool parseAttribute(const std::string& line, Attributes& attr);

    bool parseMedia(const std::string& line, MediaDescription& mediaDesc);
    bool parseAttribute(const std::string& line, std::string& name, std::string& value);

    bool parseMediaAttribute(const std::string& line, MediaDescription& mediaDesc);

    bool parseRtpmap(const std::string& str, Rtpmap& rtpmap);

    MediaDescription& getCurMedia();

private:
    MediaDescriptionArray*  m_pArrMediaDesc;

};






} /* namespace net */

#endif /* SDP_H_ */
