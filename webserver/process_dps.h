
#pragma once

#include <string>
class process_dps
{
public:
	process_dps(void);
	~process_dps(void);
	static process_dps self;
public:
	static process_dps* instance(){return &self;}
	int restart_process();
	int init(std::string fpath);
public:
#ifdef _WIN32
	int terminate_process(std::string exe_name);
	int startup_process(const char * exe_path);
#else
	long fork_child();
	void to_background();
#endif
private:
	std::string exec_fpath;
	std::string exec_fname;
};
