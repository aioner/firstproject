#include "file_stream.h"
#include <windows.h>

//数据块头
#define CHUNK_HEADER_FOURCC 'HUHC'

struct SHeader{
	unsigned uFourCC;       //识别标志 CHUNK_HEADER_FOURCC
	unsigned uHeaderSize;   //帧头长度 sizeof(SChunkHeader)
	unsigned uMediaType;    //帧类型
	unsigned uChunkCount;   //流水序号
	unsigned uDataSize;     //数据长度(不含头结构)
	unsigned uTimeStamp;    //帧时间戳
};

char sdp_h264[] = "v=0\r\n" \
"o=- 1443261584081480 1 IN IP4 0.0.0.0\r\n" \
"s=-\r\n" \
"t=0 0\r\n" \
"a=tool:LIVE555 Streaming Media v2010.07.29\r\n" \
"a=type:broadcast\r\n" \
"a=control:*\r\n" \
"a=range:npt=0-\r\n" \
"m=video 0 RTP/AVP 96\r\n" \
"c=IN IP4 0.0.0.0\r\n" \
"b=AS:12000\r\n" \
"a=rtpmap:96 H264/90000\r\n" \
"a=control:track1\r\n";
//"a=fmtp:96 packetization-mode=1;profile-level-id=64001F;sprop-parameter-sets=Z2QA" \
//"KK2EBUViuKxUdCAqKxXFYqOhAVFYrisVHQgKisVxWKjoQFRWK4rFR0ICorFcVio6ECSFITk8nyfk/k/J" \
//"8nm5s00IEkKQnJ5Pk/J/J+T5PNzZprQCgC3SpAAAAwHgAABwgYEAAfQAAAMCMoC974XhEI1AAAAAAQ==" \
//",aO48sA==\r\n" \


char sdp_g711a[] = "v=0\r\n" \
"o=- 1443261584081480 1 IN IP4 0.0.0.0\r\n" \
"s=-\r\n" \
"t=0 0\r\n" \
"a=tool:LIVE555 Streaming Media v2010.07.29\r\n" \
"a=type:broadcast\r\n" \
"a=control:*\r\n" \
"a=range:npt=0-\r\n" \
"m=audio 0 RTP/AVP 8\r\n" \
"c=IN IP4 0.0.0.0\r\n" \
"a=rtpmap:8 PCMA/8000\r\n" \
"a=control:track1\r\n";

char sdp_g711u[] = "v=0\r\n" \
"o=- 1443261584081480 1 IN IP4 0.0.0.0\r\n" \
"s=-\r\n" \
"t=0 0\r\n" \
"a=tool:LIVE555 Streaming Media v2010.07.29\r\n" \
"a=type:broadcast\r\n" \
"a=control:*\r\n" \
"a=range:npt=0-\r\n" \
"m=audio 0 RTP/AVP 0\r\n" \
"c=IN IP4 0.0.0.0\r\n" \
"a=rtpmap:0 PCMU/8000\r\n" \
"a=control:track1\r\n";

int g_track_id = 1;

file_stream *file_stream::self_ = NULL;
file_stream::file_stream(void)
:thread_work(NULL)
,run_(false)
,src_file_("")
,dest_file_("")
{
}

file_stream::~file_stream(void)
{
}

file_stream* file_stream::inst()
{
	if (!self_)
	{
		self_ = new file_stream;
	}

	return self_;
}

void file_stream::thread_work_h264()
{
    int tm_s = 16;
	long pos_buf = 0; 
	const long len_buff = 1024*1024;
	char *buff = new char[len_buff];
	if (!buff)
	{
		return;
	}

	FILE *pF = ::fopen(src_file_.c_str(),"rb");
	if (!pF)
	{
		return;
	}	

	char nal1[] = {0,0,0,1};
	char nal2[] = {0,0,1};
	SHeader head;
	head.uFourCC = CHUNK_HEADER_FOURCC;
	unsigned long tm_start = ::GetTickCount();
	while (run_)
	{
		::fread(buff+pos_buf, 1, 1, pF);
		pos_buf += 1;

		if(::feof(pF) != 0)
		{
			unsigned long tm_now = ::GetTickCount();
			head.uTimeStamp = (tm_now-tm_start) * 90;
			::memmove(buff+sizeof(SHeader),buff, pos_buf);
			::memcpy(buff, &head, sizeof(SHeader));
			xt_send_data(srcno_, 1, buff, pos_buf+sizeof(SHeader), this->frame_type_, 172);
			pos_buf = 0;
			::fseek(pF, 0, SEEK_SET);
			/*continue;*/
			break;
		}

		if ( (pos_buf==4 && ::memcmp(nal1, buff+pos_buf-4, 4)==0) ||
			 (pos_buf==3 && ::memcmp(nal2, buff+pos_buf-3, 3)==0) ) 
		{
			continue;
		}
		else if(pos_buf>4 && ::memcmp(nal1, buff+pos_buf-4, 4)==0)
		{
			unsigned long tm_now = ::GetTickCount();
			head.uTimeStamp = (tm_now-tm_start) * 90;
			::memmove(buff+sizeof(SHeader),buff, pos_buf);
			::memcpy(buff, &head, sizeof(SHeader));

			long len = pos_buf-4;
			xt_send_data(srcno_, 1, buff, len+sizeof(SHeader), this->frame_type_, 172);
			::memcpy(buff, nal1, 4);
			pos_buf = 4;
			Sleep(tm_s);
		}
		else if(pos_buf>3 && ::memcmp(nal2, buff+pos_buf-3, 3)==0)
		{
			unsigned long tm_now = ::GetTickCount();
			head.uTimeStamp = (tm_now-tm_start) * 90;
			::memmove(buff+sizeof(SHeader),buff, pos_buf);
			::memcpy(buff, &head, sizeof(SHeader));

			long len = pos_buf-3;
			xt_send_data(srcno_, 1, buff, len+sizeof(SHeader), this->frame_type_, 172);
			::memcpy(buff, nal2, 3);
			pos_buf = 3;
			Sleep(tm_s);
		}
		else if (pos_buf >= len_buff)
		{
			long len = len_buff;

			unsigned long tm_now = ::GetTickCount();
			head.uTimeStamp = (tm_now-tm_start) * 90;
			::memmove(buff+sizeof(SHeader),buff, len-sizeof(SHeader));
			::memcpy(buff, &head, sizeof(SHeader));
			
			xt_send_data(srcno_, 1, buff, len, this->frame_type_, 172);
			pos_buf = 0;
			Sleep(tm_s);
		}
	}

	::fclose(pF);

	delete buff;
}

void file_stream::thread_work_g711()
{
	long pos_buf = 0; 
	const long len_buff = 1024*1024;
	const long len_frame = 160;
	long frame_rate = (len_frame*1000) / 8000;
	unsigned int seq_start = 0;
	frame_rate *= 1;
	char *buff = new char[len_buff];
	if (!buff)
	{
		return;
	}

	FILE *pF = ::fopen(src_file_.c_str(),"rb");
	if (!pF)
	{
		return;
	}	

	SHeader head;
	head.uFourCC = CHUNK_HEADER_FOURCC;
	while (run_)
	{
		size_t size = ::fread(buff+pos_buf, 1, len_frame, pF);
		pos_buf += size;

		if(size!=len_frame || ::feof(pF) != 0)
		{
			head.uTimeStamp = (seq_start++) * frame_rate * 8;
			::memmove(buff+sizeof(SHeader),buff, pos_buf);
			::memcpy(buff, &head, sizeof(SHeader));
			xt_send_data(srcno_, 1, buff, pos_buf+sizeof(SHeader), this->frame_type_, 172);
			pos_buf = 0;
			::fseek(pF, 0, SEEK_SET);
			/*continue;*/
			break;
		}
		else
		{
			head.uTimeStamp = (seq_start++) * frame_rate * 8;
			::memmove(buff+sizeof(SHeader),buff, pos_buf);
			::memcpy(buff, &head, sizeof(SHeader));

			long len = pos_buf;
			xt_send_data(srcno_, 1, buff, len+sizeof(SHeader), this->frame_type_, 172);
			pos_buf = 0;
			Sleep(frame_rate/8);
		}
	}

	::fclose(pF);

	delete buff;
}

int file_stream::start_file_stream(std::string& src_file, std::string& dest_file, std::string dest_ip, unsigned short dest_port, unsigned int demuxid)
{
	run_ = true;
	src_file_ = src_file;
	dest_file_ = dest_file_;

	long chanid = 0;
	src_track_info_t tracks;
	tracks.tracknum = 1;
	tracks.trackids[0] = 1;
	::strcpy(tracks.tracknames[0], "");
	xt_create_src(tracks, srcno_, chanid);

	xt_set_file_path(dest_file.c_str());

	std::string::size_type s = src_file.find(".264");
	if (src_file.find(".264") != std::string::npos ||
		src_file.find(".h264") != std::string::npos)
	{
		frame_type_ = OV_H264;
		xt_set_key(srcno_, sdp_h264, sizeof(sdp_h264)-1, 172);
		thread_work = new boost::thread(boost::bind(&file_stream::thread_work_h264,this));
	}
	else if (src_file.find(".711u") != std::string::npos)
	{
		frame_type_ = OV_G711;
		xt_set_key(srcno_, sdp_g711u, sizeof(sdp_g711u)-1, 172);
		thread_work = new boost::thread(boost::bind(&file_stream::thread_work_g711,this));
	}
	else if (src_file.find(".711a") != std::string::npos)
	{
		frame_type_ = OV_G711;
		xt_set_key(srcno_, sdp_g711a, sizeof(sdp_g711a)-1, 172);
		thread_work = new boost::thread(boost::bind(&file_stream::thread_work_g711,this));
	}

    if (demuxid > 0)
    {
        xt_add_send(srcno_, g_track_id, dest_ip.c_str(), dest_port, true, demuxid);
    }
	else if (dest_ip.length()>0 && dest_port>0)
	{
		xt_add_send(srcno_, g_track_id, dest_ip.c_str(), dest_port, false, 0);
	}

	return 0;
}

int file_stream::stop_file_stream()
{
	run_ = false;
	if (thread_work)
	{
		thread_work->join();
		delete thread_work;
		thread_work = NULL;
	}

	xt_destroy_src(srcno_);
	srcno_ = -1;

	return 0;
}
