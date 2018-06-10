/*
 * TStringCast.h
 *
 *  Created on: 2008-8-4
 *      Author: terry
 */

#ifndef TSTRINGCAST_H_
#define TSTRINGCAST_H_

#include <string>
#include <sstream>
#include <cstring>


////////////////////////////////////////
namespace comn
{


class StringCast
{
public:
	template < class T >
	static std::string toString( const T& t )
	{
		std::ostringstream ss;
		ss << t;
		return ss.str();
	}

	static std::string toString( const char t )
	{
		std::ostringstream ss;
		ss << (short)t;
		return ss.str();
	}

	static std::string toString( const unsigned char t )
	{
		std::ostringstream ss;
		ss << (short)t;
		return ss.str();
	}

	////////////////////////////////////////
	static void charToHex( char ch, char* buf, size_t length )
	{
		static std::string s_hex = "0123456789abcdef";
		buf[ 0 ] = s_hex[ (ch >> 4 ) & 0xF ];
		buf[ 1 ] = s_hex[ ch & 0xF ];
	}

	template < class T >
	static std::string toHexString( const T& t )
	{
		std::ostringstream ss;
		ss << std::hex << t;
		return ss.str();
	}

	static std::string toHexString( const char t )
	{
		std::ostringstream ss;
		char buf[3] = {0};
		charToHex( t, buf, 3 );
		ss << buf;
		//ss << std::hex << (short)t;
		return ss.str();
	}

	static std::string toHexString( const unsigned char t )
	{
		std::ostringstream ss;
		char buf[3] = {0};
		charToHex( t, buf, 3 );
		ss << buf;
		//ss << std::hex << (short)t;
		return ss.str();
	}

	static std::string toHexString( const char* t, size_t len )
	{
		std::ostringstream ss;
		char buf[3] = {0};
		for ( size_t i = 0; i < len; ++ i )
		{
			charToHex( t[i], buf, 3 );
			ss << buf;
			//ss << std::hex << (int)t[i] << " ";
		}
		return ss.str();
	}

	static std::string toHexString( const unsigned char* t, size_t len )
	{
	    return toHexString((const char*)t, len);
	}

	static std::string toHexString( const char* t )
	{
		return toHexString( t, strlen( t ) );
	}

	static std::string toHexString( const std::string& t )
	{
		return toHexString( t.c_str(), t.size() );
	}

	static std::string toHexGroupString( const char* t, size_t len )
	{
		std::ostringstream ss;
		char buf[3] = {0};
		for ( size_t i = 0; i < len; ++ i )
		{
			charToHex( t[i], buf, 3 );
			ss << buf << " ";
			if ( i % 16 == 15 )
				ss << "\n";
			else if ( i % 8 == 7 )
				ss << "  ";
		}
		return ss.str();
	}

	static std::string toHexGroupString( const unsigned char* t, size_t len )
	{
	    return toHexGroupString((const char*)t, len);
	}

	static std::string toHexGroupString( const std::string& t )
	{
		return toHexGroupString( t.c_str(), t.size() );
	}

	static std::string toHexCode(const char* t, size_t len)
	{
        std::ostringstream ss;
        char buffer[5] = "0x00";
        for ( size_t i = 0; i < len; ++ i )
        {
            charToHex( t[i], &buffer[2], 3 );
            ss << buffer << ", ";
            if ( i % 16 == 15 )
                ss << "\n";
            else if ( i % 8 == 7 )
                ss << "  ";
        }
        return ss.str();
	}
	////////////////////////////////////////

	template < class T >
	static void toValue( const std::string& str, T& t )
	{
		std::istringstream iss( str );
		iss >> t;
	}


};

} // end of namespace

#endif /* TSTRINGCAST_H_ */
