#pragma once

#include <string>
#include<boost/thread.hpp>
#include "share_type_def.h"
#include "h_xtmediaserver.h"

class file_stream
{
private:
	file_stream(void);
	~file_stream(void);

public:
	static file_stream *self_;

	static file_stream* inst();

	int start_file_stream(std::string& src_file, std::string& dest_file, std::string dest_ip, unsigned short dest_port, unsigned int demuxid);
	int stop_file_stream();

private:
	boost::thread* thread_work;
	bool run_;

	void thread_work_g711();
	void thread_work_h264();

	int srcno_;
	ov_frame_type frame_type_;
	std::string src_file_;
	std::string dest_file_;
};
