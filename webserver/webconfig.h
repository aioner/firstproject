#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <string.h>
#include "xtXml.h"

class webconfig
{
private:
	static webconfig m_obj;
	std::string m_fPath;
public:
	static webconfig* Instance()
	{
		return &m_obj;
	}
private:
	webconfig(void);
	~webconfig(void);
public:
	int get_dps_cfg(char *buff,int len);
	int set_dps_cfg(const char *buff,int len);
};
