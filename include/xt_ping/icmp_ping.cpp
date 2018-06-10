
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <istream>
#include <iostream>
#include <ostream>
#include <boost/atomic.hpp>

#include "icmp_header.hpp"
#include "ipv4_header.hpp"
#include "icmp_ping.h"
#include <list>

#ifdef _WIN32
#else
#include <sys/types.h>
#include <unistd.h>
#endif

using boost::asio::ip::icmp;
using boost::asio::deadline_timer;
namespace posix_time = boost::posix_time;

//发送包记录
typedef struct _struct_pack
{
	posix_time::ptime upate_time_;
	unsigned short sequence_number_;

}pack_type,*p_pack_type;

class pinger
{

public:
	pinger(boost::asio::io_service& io_service, const char* destination,const long out_time,const long pack_num)
		: resolver_(io_service), socket_(io_service, icmp::v4()),out_time_(out_time),pack_num_(pack_num),
		timer_(io_service), sequence_number_(0), /*num_replies_(0),*/send_num_(0),recv_num_(0),lost_counts_(0),time_count_(0),
		lost_(0),quit_(false)
	{
		if (destination != NULL)
		{
			icmp::resolver::query query(icmp::v4(), destination, "");
			destination_ = *resolver_.resolve(query);

			start_send();
			start_receive();
		}

	}

	void count_ret(net_state_type& ping_ret)
	{

		//SrcIP
		std::string dest_ip = destination_.address().to_string();
		memcpy(ping_ret.src_ip, dest_ip.c_str(), dest_ip.size());

		//丢包率
		if (0.00001f < send_num_)
		{
			lost_counts_ = send_num_ - recv_num_;

			ping_ret.lost = ((double)lost_counts_ / (double)send_num_);
		}

		std::list<pack_type>::iterator itr_recv = lst_recv_packs.begin();
		std::list<pack_type>::iterator itr_send;
		for (; lst_recv_packs.end() != itr_recv; ++ itr_recv)
		{
			for (itr_send = lst_send_packs.begin(); lst_send_packs.end() != itr_send; ++itr_send)
			{
				if (itr_recv->sequence_number_ == itr_send->sequence_number_)
				{
					time_count_ += (itr_recv->upate_time_ - itr_send->upate_time_).total_microseconds();//mics
					break;
				}
			}
		}		

		//平均往返时间
		if (0.00001f < recv_num_)
		{
			ping_ret.rtt =  time_count_ / recv_num_;
			ping_ret.net_state = NET_STATE_ON_LINE;
		}
		else
		{
			ping_ret.net_state = NET_STATE_OFF_LINE;
			//丢失所有的包
			ping_ret.rtt = 50000000;
		}

		ping_ret.send_packs = send_num_;
		ping_ret.recv_packs = recv_num_;

		std::cout<<"ip:["<<dest_ip<<"] send_num_["<<send_num_<<"] recv_num_["<<recv_num_<<"] loss["<<ping_ret.lost<<"] rtt["<<ping_ret.rtt<<" mics]"<<std::endl;

		resetting();
	}

	void resetting()
	{
		lost_counts_=0;
		sequence_number_ = 0;
		send_num_ = 0;
		recv_num_ = 0;
		time_count_=0;
		rtt_=0;
		lst_sequense_numbers_.clear();
		lst_send_packs.clear();
		lst_recv_packs.clear();
	}

private:
	void start_send()
	{

		for (int i = 0; i<pack_num_; ++i)
		{
			pack_type send_pack;
			++send_num_;

			std::string body("\"Hello!\" from Asio ping.");

			// Create an ICMP header for an echo request.
			icmp_header echo_request;
			echo_request.type(icmp_header::echo_request);
			echo_request.code(0);
			echo_request.identifier(get_identifier());
			echo_request.sequence_number(++sequence_number_);
			compute_checksum(echo_request, body.begin(), body.end());

			lst_sequense_numbers_.push_back(sequence_number_);

			// Encode the request packet.
			boost::asio::streambuf request_buffer;
			std::ostream os(&request_buffer);
			os << echo_request << body;

			// Send the request.
			time_sent_ = posix_time::microsec_clock::universal_time();

			send_pack.sequence_number_ = sequence_number_;
			send_pack.upate_time_ = time_sent_;
			lst_send_packs.push_back(send_pack);

			socket_.send_to(request_buffer.data(), destination_);

		}

		//接收超时
		timer_.expires_at(time_sent_  + posix_time::seconds(out_time_));
		timer_.async_wait(boost::bind(&pinger::handle_timeout, this));
	}

	void handle_timeout()
	{
		//超时就统计
		quit_ = true;
	}

	void start_receive()
	{
		// Discard any data already in the buffer.
		reply_buffer_.consume(reply_buffer_.size());

		// Wait for a reply. We prepare the buffer to receive up to 64KB.
		socket_.async_receive(reply_buffer_.prepare(65536),
			boost::bind(&pinger::handle_receive, this, _2));
	}

	void handle_receive(std::size_t length)
	{
		// The actual number of bytes received is committed to the buffer so that we
		// can extract it using a std::istream object.
		reply_buffer_.commit(length);

		// Decode the reply packet.
		std::istream is(&reply_buffer_);
		ipv4_header ipv4_hdr;
		icmp_header icmp_hdr;
		is >> ipv4_hdr >> icmp_hdr;

		// We can receive all ICMP packets received by the host, so we need to
		// filter out only the echo replies that match the our identifier and
		// expected sequence number.
		if (is && icmp_hdr.type() == icmp_header::echo_reply
			&& icmp_hdr.identifier() == get_identifier()
			/*&& icmp_hdr.sequence_number() == sequence_number_*/
			&& destination_.address().to_v4() == ipv4_hdr.source_address()
			&& ( lst_sequense_numbers_.end() != 
			std::find(lst_sequense_numbers_.begin(),
			lst_sequense_numbers_.end(),icmp_hdr.sequence_number())))
		{
			pack_type recv_pack;
			recv_pack.sequence_number_ = icmp_hdr.sequence_number();
			recv_pack.upate_time_ = posix_time::microsec_clock::universal_time();
			lst_recv_packs.push_back(recv_pack);

			++recv_num_;
     		// 			std::cout << length - ipv4_hdr.header_length()
			// 				<< " bytes from " << ipv4_hdr.source_address()
			// 				<< ": icmp_seq=" << icmp_hdr.sequence_number()
			// 				<< ", ttl=" << ipv4_hdr.time_to_live()
			// 				<< ", time=" << (now - time_sent_).total_milliseconds() << " ms"
			// 				<< std::endl;		
		}

		if (recv_num_ == send_num_)
		{
			timer_.cancel();
		}

		start_receive();
	}

	static unsigned short get_identifier()
	{
#ifdef _WIN32
		getpid();
		return static_cast<unsigned short>(::GetCurrentProcessId());
#else
		pid_t id = getpid();
		return	id;
#endif
	}	

	icmp::resolver resolver_;
	icmp::endpoint destination_;
	icmp::socket socket_;
	deadline_timer timer_;
	unsigned short sequence_number_;
	posix_time::ptime time_sent_;
	boost::asio::streambuf reply_buffer_;
	/*std::size_t num_replies_;*/

	//LARGE_INTEGER time_send_;    //发送时间
	int send_num_;               //发送计数
	int recv_num_;               //接收计数
	unsigned short lost_counts_; //丢包计数
	double lost_;                //丢包率
	uint64_t time_count_;        //发包时间和
	uint64_t rtt_;               //平均RTT
	long out_time_;              //超时
	long pack_num_;               //发包数量

	std::list<unsigned short> lst_sequense_numbers_; //包序号缓存
	std::list<pack_type> lst_send_packs;             
	std::list<pack_type> lst_recv_packs;

public:
	boost::atomic<bool> quit_;
};
	
int ping(net_state_type& ping_ret,const char* destination,const long out_time_seconds,const long pack_num)
{
	try
	{
		boost::asio::io_service io_service;
		pinger ping(io_service,destination,out_time_seconds,pack_num);
		while (!ping.quit_)
		{
			io_service.run_one();
		}
		ping.count_ret(ping_ret);
	}
	catch (...)
	{
		return ICMP_PING_ERR_INIT_FLIED;

	}

	return 0;
}
