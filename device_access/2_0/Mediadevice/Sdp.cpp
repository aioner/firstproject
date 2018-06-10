/*
 * Sdp.cpp
 *
 *  Created on: 2013Äê12ÔÂ6ÈÕ
 *      Author: zhengchuanjiang
 */

#include "Sdp.h"
#include <map>
#include <vector>
#include "TStringCast.h"


namespace sdp
{

namespace detail
{

class StringMap: public std::map< std::string, std::string >
{
};

}


using namespace detail;

////////////////////////////////////////////////////////////////////////////
class ConstName
{
public:
    static const char* get(SendRecvMode mode);

    static const char* recvonly;
    static const char* sendrecv;
    static const char* sendonly;
    static const char* inactive;

};

const char* ConstName::get(SendRecvMode mode)
{
    if (mode == kSendRecv)
    {
        return "sendrecv";
    }
    else if (mode == kRecvOnly)
    {
        return "recvonly";
    }
    else if (mode == kSendonly)
    {
        return "sendonly";
    }
    else if (mode == kInactive)
    {
        return "inactive";
    }
    return "";
}


const char* ConstName::recvonly = "recvonly";
const char* ConstName::sendrecv = "sendrecv";
const char* ConstName::sendonly = "sendonly";
const char* ConstName::inactive = "inactive";


SendRecvMode findMode(const Attributes& attr)
{
    SendRecvMode mode = kSendRecv;
    if (attr.exists(ConstName::recvonly))
    {
        mode = kRecvOnly;
    }
    else if (attr.exists(ConstName::sendonly))
    {
        mode = kSendonly;
    }
    else if (attr.exists(ConstName::inactive))
    {
        mode = kInactive;
    }
    return mode;
}

const char* getName(SendRecvMode mode)
{
    return ConstName::get(mode);
}


////////////////////////////////////////////////////////////////////////////

Attributes::Attributes():
        m_pMap(new StringMap())
{

}

Attributes::~Attributes()
{
    delete m_pMap;
}

Attributes::Attributes(const Attributes& other):
        m_pMap(new StringMap(*other.m_pMap))
{
}

Attributes& Attributes::operator = (const Attributes& other)
{
    *m_pMap = *other.m_pMap;
    return *this;
}


bool Attributes::operator == (const Attributes& other)
{
    return (*m_pMap == *other.m_pMap);
}

bool Attributes::operator < (const Attributes& other)
{
    return (*m_pMap < *other.m_pMap);
}


void Attributes::swap(Attributes& other)
{
    std::swap(m_pMap, other.m_pMap);
}


bool Attributes::insert(const std::string& key, const std::string& value)
{
    return m_pMap->insert(StringMap::value_type(key, value)).second;
}

bool Attributes::replace(const std::string& key, const std::string& value)
{
    std::pair< StringMap::iterator, bool > pairb =
            m_pMap->insert(StringMap::value_type(key, value));
    if (!pairb.second)
    {
        (*pairb.first).second = value;
    }
    return pairb.second;
}

std::string& Attributes::operator [](const char* key)
{
    return (*m_pMap)[key];
}

std::string& Attributes::operator [](const std::string& key)
{
    return (*m_pMap)[key];
}


size_t Attributes::size() const
{
    return (*m_pMap).size();
}


bool Attributes::empty() const
{
    return (*m_pMap).empty();
}


bool Attributes::getAt(size_t idx, std::string& value) const
{
    if (idx >= size())
    {
        return false;
    }

    StringMap::const_iterator it = (*m_pMap).begin();
    std::advance(it, idx);

    value = it->second;

    return true;
}

bool Attributes::getAt(size_t idx, std::string& key, std::string& value) const
{
    if (idx >= size())
    {
        return false;
    }

    StringMap::const_iterator it = (*m_pMap).begin();
    std::advance(it, idx);

    value = it->second;
    key = it->first;

    return true;
}

bool Attributes::find(const std::string& key, std::string& value) const
{
    bool found = false;
    StringMap::const_iterator it = (*m_pMap).find(key);
    if (it != (*m_pMap).end())
    {
        value = it->second;
    }
    return found;
}

bool Attributes::find(const char* key, std::string& value) const
{
    bool found = false;
    StringMap::const_iterator it = (*m_pMap).find(key);
    if (it != (*m_pMap).end())
    {
        value = it->second;
        found = true;
    }
    return found;
}

bool Attributes::find(const char* key, long long& value) const
{
    bool found = false;
    StringMap::const_iterator it = (*m_pMap).find(key);
    if (it != (*m_pMap).end())
    {
        comn::StringCast::toValue(it->second, value);
        found = true;
    }
    return found;
}

bool Attributes::find(const char* key, int& value) const
{
    bool found = false;
    StringMap::const_iterator it = (*m_pMap).find(key);
    if (it != (*m_pMap).end())
    {
        comn::StringCast::toValue(it->second, value);
        found = true;
    }
    return found;
}

bool Attributes::find(const char* key, double& value) const
{
    bool found = false;
    StringMap::const_iterator it = (*m_pMap).find(key);
    if (it != (*m_pMap).end())
    {
        comn::StringCast::toValue(it->second, value);
        found = true;
    }
    return found;
}

bool Attributes::exists(const std::string& key) const
{
    StringMap::const_iterator it = (*m_pMap).find(key);
    return (it != (*m_pMap).end());
}

bool Attributes::exists(const char* key) const
{
    StringMap::const_iterator it = (*m_pMap).find(key);
    return (it != (*m_pMap).end());
}


void Attributes::clear()
{
    (*m_pMap).clear();
}


bool Attributes::erase(const std::string& key)
{
    return (*m_pMap).erase(key) > 0;
}


bool Attributes::erase(const char* key)
{
    return (*m_pMap).erase(key) > 0;
}

bool Attributes::findWithValue(const std::string& value, std::string& key) const
{
    bool found = false;
    StringMap::const_iterator it = (*m_pMap).begin();
    for (; it != (*m_pMap).end(); it ++)
    {
        if (it->second == value)
        {
            key = it->first;
            found = true;
            break;
        }
    }
    return found;
}

////////////////////////////////////////////////////////////////////////////
int Util::icompare(const std::string& src, const char* target)
{
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
    return _stricmp( src.c_str(), target );
#else
    return strcasecmp(src.c_str(), target);
#endif
}

int Util::split(const std::string& src, char sp, std::string& head, std::string& tail)
{
    if (src.empty())
    {
        return 0;
    }

    size_t pos = src.find(sp);
    if (pos != std::string::npos)
    {
        head = src.substr(0, pos);
        tail = src.substr(pos+1);
        return 2;
    }
    head = src;
    return 1;
}

int Util::split(const std::string& src, char sp, size_t start,
            std::string& head, std::string& tail)
{
    if (src.empty())
    {
        return 0;
    }

    size_t pos = src.find(sp, start);
    if (pos != std::string::npos)
    {
        head = src.substr(start, pos - start);
        tail = src.substr(pos+1);
        return 2;
    }
    head = src.substr(start);
    return 1;
}

bool Util::tail(const std::string& src, char sp, std::string& str)
{
    size_t idx = src.find(sp);
    if (idx == std::string::npos)
    {
        return false;
    }

    str = src.substr(idx + 1);
    return true;
}

bool Util::head(const std::string& src, char sp, std::string& value)
{
    size_t idx = src.find(sp);
    if (idx == std::string::npos)
    {
        value = src;
        return false;
    }

    value = src.substr(0, idx);
    return true;
}

bool Util::tail(const std::string& src, char sp, int& value)
{
    bool done = false;
    std::string str;
    if (tail(src, sp, str))
    {
        comn::StringCast::toValue(str, value);
        done = true;
    }
    return done;
}




StringToken::StringToken(const std::string& src, char sp, size_t start):
        m_src(src),
        m_sp(sp),
        m_pos(start)
{

}

bool StringToken::next(std::string& section)
{
    if (m_pos == std::string::npos)
    {
        return false;
    }

    size_t nextPos = m_src.find(m_sp, m_pos);
    if (nextPos == std::string::npos)
    {
        section.assign(m_src, m_pos, std::string::npos);
        m_pos = std::string::npos;
    }
    else
    {
        section.assign(m_src, m_pos, (nextPos - m_pos));
        m_pos = nextPos + 1;
    }
    return true;
}

bool StringToken::next(int& value)
{
    bool done = false;
    std::string section;
    if (next(section))
    {
        comn::StringCast::toValue(section, value);
        done = true;
    }
    return done;
}


bool AttributeParser::parse(const std::string& str, char eol, char equal, Attributes& attr)
{
    StringToken token(str, eol, 0);
    std::string line;
    while (token.next(line))
    {
        std::string key;
        std::string value;
        Util::split(line, equal, key, value);

        attr[key] = value;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool H264Fmtp::parse(const std::string& line)
{
    Attributes attr;
    if (!AttributeParser::parse(line, ';', '=', attr))
    {
        return false;
    }

    attr.find("packetization-mode", packetization_mode);
    attr.find("profile-level-id", profile_level_id);
    attr.find("sprop-parameter-sets", sprop_parameter_sets);
    return true;
}

bool AacFmtp::parse(const std::string& line)
{
    Attributes attr;
    if (!AttributeParser::parse(line, ';', '=', attr))
    {
        return false;
    }

    attr.find("mode", mode);
    attr.find("streamtype", streamType);
    attr.find("indexdeltalength", indexDeltaLength);
    attr.find("indexlength", indexLength);
    attr.find("sizelength", sizeLength);
    attr.find("config", config);
    return true;
}
////////////////////////////////////////////////////////////////////////////
class MediaDescriptionArray : public std::vector< MediaDescription >
{
};

SessionDescription::SessionDescription():
        m_version(),
        m_origin(),
        m_name(),
        m_info(),
        m_uri(),
        m_email(),
        m_phone(),
        m_connection(),
        m_bandwidth(),
        m_timeDesc(),
        m_repeatTime(),
        m_timeZone(),
        m_key(),
        m_attributes(),
        m_pArrMediaDesc(new MediaDescriptionArray())
{

}

SessionDescription::~SessionDescription()
{
    delete m_pArrMediaDesc;
}

SessionDescription::SessionDescription(const SessionDescription& other):
        m_version(),
        m_origin(),
        m_name(),
        m_info(),
        m_uri(),
        m_email(),
        m_phone(),
        m_connection(),
        m_bandwidth(),
        m_timeDesc(),
        m_repeatTime(),
        m_timeZone(),
        m_key(),
        m_attributes(),
        m_pArrMediaDesc(new MediaDescriptionArray())
{
    assign(other);
}

void SessionDescription::assign(const SessionDescription& other)
{
    m_version = other.m_version;
    m_origin = other.m_origin;
    m_name = other.m_name;
    m_info = other.m_info;
    m_uri = other.m_uri;
    m_email = other.m_email;
    m_phone = other.m_phone;
    m_connection = other.m_connection;
    m_bandwidth = other.m_bandwidth;
    m_timeDesc = other.m_timeDesc;
    m_repeatTime = other.m_repeatTime;
    m_timeZone = other.m_timeZone;
    m_key = other.m_key;
    m_attributes = other.m_attributes;
    *m_pArrMediaDesc = *other.m_pArrMediaDesc;
}

SessionDescription& SessionDescription::operator = (const SessionDescription& other)
{
    assign(other);
    return *this;
}

bool SessionDescription::parse(const char* str, size_t length)
{
    clear();

    size_t start = 0;
    size_t end = 0;
    std::string line;
    for (size_t i = 0; i < length; ++ i)
    {
        if (str[i] == '\n')
        {
            if (i > 0 && str[i-1] == '\r')
            {
                end = i - 1;
            }
            else
            {
                end = i;
            }

            line.assign(str + start, end - start);
            parseLine(line);
            start = i + 1;
        }
    }

    if (start < length)
    {
        line.assign(str + start, length - start);
        parseLine(line);
    }

    return true;
}

bool SessionDescription::parse(const std::string& str)
{
    return parse(str.c_str(), str.length());
}

size_t SessionDescription::getMediaCount() const
{
    return m_pArrMediaDesc->size();
}

MediaDescription& SessionDescription::getMedia(size_t idx)
{
    return (*m_pArrMediaDesc)[idx];
}

bool SessionDescription::hasMedia()
{
    return ((*m_pArrMediaDesc).size() > 0);
}

bool SessionDescription::parseLine(const std::string& line)
{
    if (line.empty())
    {
        return false;
    }

    bool done = true;
    switch (line[0])
    {
    case 'v':
        // version
        Util::tail(line, '=', m_version);
        break;
    case 'o':
        // origin
        done = parseOrigin(line, m_origin);
        break;
    case 's':
        // session
        Util::tail(line, '=', m_name);
        break;
    case 'i':
        // info
        Util::tail(line, '=', m_info);
        break;
    case 'u':
        // uri
        Util::tail(line, '=', m_uri);
        break;
    case 'e':
        // email
        Util::tail(line, '=', m_email);
        break;
    case 'p':
        // phone
        Util::tail(line, '=', m_phone);
        break;
    case 'c':
        // connection
        if (hasMedia())
        {
            MediaDescription& mediaDesc = getCurMedia();
            done = parseAddr(line, mediaDesc.addr);
        }
        else
        {
            done = parseAddr(line, m_connection);
        }
        break;
    case 'b':
        // bandwidth
        if (hasMedia())
        {
            MediaDescription& mediaDesc = getCurMedia();
            done = parseBandwidth(line, mediaDesc.bandwidth);
        }
        else
        {
            done = parseBandwidth(line, m_bandwidth);
        }
        break;
    case 'z':
        // time zone
        Util::tail(line, '=', m_timeZone);
        break;
    case 'k':
        // key
        if (hasMedia())
        {
            MediaDescription& mediaDesc = getCurMedia();
            done = parseKey(line, mediaDesc.key);
        }
        else
        {
            done = parseKey(line, m_key);
        }
        break;
    case 't':
        // time
        done = parseTime(line, m_timeDesc);
        break;
    case 'r':
        // repeat
        Util::tail(line, '=', m_repeatTime);
        break;
    case 'm':
        // media
        done = parseMedia(line);
        break;
    case 'a':
        // attribute
        if (hasMedia())
        {
            MediaDescription& mediaDesc = getCurMedia();
            done = parseMediaAttribute(line, mediaDesc);
        }
        else
        {
            done = parseAttribute(line, m_attributes);
        }
        break;

    default:
        done = false;
        break;
    }

    return done;
}

bool SessionDescription::parseOrigin(const std::string& line, SessionOrigin& origin)
{
    StringToken token(line, ' ', 2);
    token.next(origin.username);
    token.next(origin.sessionID);
    token.next(origin.version);
    token.next(origin.addr.nettype);
    token.next(origin.addr.addrtype);
    token.next(origin.addr.address);

    return true;
}

bool SessionDescription::parseTime(const std::string& line, TimeDescription& timeDesc)
{
    std::string head;
    std::string tail;
    Util::split(line, ' ', 2, head, tail);
    comn::StringCast::toValue(head, timeDesc.start);
    comn::StringCast::toValue(tail, timeDesc.stop);
    return true;
}

bool SessionDescription::parseAddr(const std::string& line, ConnectionData& conn)
{
    StringToken token(line, ' ', 2);
    token.next(conn.nettype);
    token.next(conn.addrtype);

    std::string addr;
    token.next(addr);

    Util::head(addr, '/', conn.address);

    return true;
}

bool SessionDescription::parseBandwidth(const std::string& line, Bandwidth& bandwidth)
{
    std::string tail;
    Util::split(line, ':', 2, bandwidth.bwtype, tail);
    comn::StringCast::toValue(tail, bandwidth.bandwidth);
    return true;
}


bool SessionDescription::parseKey(const std::string& line, EncryptionKey& key)
{
    std::string tail;
    Util::split(line, ':', 2, key.method, key.key);
    return true;
}

bool SessionDescription::parseMedia(const std::string& line)
{
    (*m_pArrMediaDesc).push_back(MediaDescription());
    MediaDescription& mediaDesc = (*m_pArrMediaDesc).back();
    return parseMedia(line, mediaDesc);
}

bool SessionDescription::parseAttribute(const std::string& line, Attributes& attr)
{
    bool done = false;
    std::string name;
    std::string value;
    if (parseAttribute(line, name, value))
    {
        attr[name] = value;
        done = true;
    }
    return done;
}

bool SessionDescription::parseMedia(const std::string& line, MediaDescription& mediaDesc)
{
    StringToken token(line, ' ', 2);
    token.next(mediaDesc.media);
    std::string sec;
    token.next(sec);
    comn::StringCast::toValue(sec, mediaDesc.port);
    token.next(mediaDesc.proto);

    while (token.next(sec))
    {
        int format = 0;
        comn::StringCast::toValue(sec, format);

        mediaDesc.formats[mediaDesc.fmtCount] = format;
        mediaDesc.fmtCount ++;
    }

    return true;
}

bool SessionDescription::parseAttribute(const std::string& line, std::string& name, std::string& value)
{
    std::string tail;
    Util::split(line, ':', 2, name, value);
    return true;
}

bool SessionDescription::parseMediaAttribute(const std::string& line, MediaDescription& mediaDesc)
{
    bool done = false;
    std::string name;
    std::string value;
    if (parseAttribute(line, name, value))
    {
        mediaDesc.attributes[name] = value;

        if (name == "rtpmap")
        {
            for (int i = 0; i < mediaDesc.fmtCount; i ++)
            {
                if (mediaDesc.rtpmaps[i].name.empty())
                {
                    parseRtpmap(value, mediaDesc.rtpmaps[i]);
                    break;
                }
            }

        }
        else if (name == "fmtp")
        {
            for (int i = 0; i < mediaDesc.fmtCount; i ++)
            {
                if (mediaDesc.fmtps[i].empty())
                {
                    mediaDesc.fmtps[i] = value;
                    break;
                }
            }

        }
        else if (name == "control")
        {
            mediaDesc.control = value;
        }

        done = true;
    }
    return done;
}

bool SessionDescription::parseRtpmap(const std::string& str, Rtpmap& rtpmap)
{
    std::string strPayload;
    std::string section;
    Util::split(str, ' ', strPayload, section);

    comn::StringCast::toValue(strPayload, rtpmap.payload);

    StringToken token(section, '/');
    token.next(rtpmap.name);
    token.next(rtpmap.clockRate);
    token.next(rtpmap.channels);
    token.next(rtpmap.param);

    return true;
}

MediaDescription& SessionDescription::getCurMedia()
{
    return (*m_pArrMediaDesc).back();
}

void SessionDescription::clear()
{
    m_version = 0;
    //m_origin;
    m_name.clear();
    m_info.clear();
    m_uri.clear();
    m_email.clear();
    m_phone.clear();

    m_connection.clear();
    m_bandwidth.clear();

    m_timeDesc.clear();
    m_repeatTime.clear();
    m_timeZone.clear();
    m_key.clear();

    m_attributes.clear();

    (*m_pArrMediaDesc).clear();
}

SendRecvMode SessionDescription::getMode() const
{
    return findMode(m_attributes);
}

std::string SessionDescription::getControl() const
{
    std::string url;
    m_attributes.find("control", url);
    return url;
}

bool SessionDescription::getRange(double& start, double& stop)
{
    std::string strRange;
    if (!m_attributes.find("range", strRange))
    {
        return false;
    }

    std::string strTime;
    Util::tail(strRange, '=', strTime);
    std::string strStart;
    std::string strStop;
    Util::split(strTime, '-', strStart, strStop);
    
    comn::StringCast::toValue(strStart, start);
    comn::StringCast::toValue(strStop, stop);
    return true;
}


} /* namespace net */
