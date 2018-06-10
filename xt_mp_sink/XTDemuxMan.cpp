#include "XTDemuxMan.h"
#include <../rv_adapter/rv_api.h>
#include <string.h>

XTDemuxMan XTDemuxMan::m_self;
XTDemuxMan::XTDemuxMan(void)
:m_numOfDemuxs(1)
,m_numOfSessions(128)
,m_bInit(false)
{
}

XTDemuxMan::~XTDemuxMan(void)
{
}

void XTDemuxMan::init(int numOfDemux, int numOfSessions)
{
	m_numOfDemuxs = numOfDemux;
	m_numOfSessions = numOfSessions;
}

bool XTDemuxMan::create_demux(rv_session_descriptor *descriptor, int numOfSessions,unsigned short port)
{
	if (!descriptor)
	{
		return false;
	}
	_XTDemux *demux = get_demux(port);

	if (NULL == demux)
	{
		demux = new _XTDemux;
	}
	else
	{
		return true;
	}
	if (!demux)
	{
		return false;
	}

	::memset(demux,0,sizeof(_XTDemux));
    bool ret = RV_ADAPTER_TRUE ==::open_session2(descriptor, &demux->handle) ? true : false;
	if (!ret)
	{
		if (demux)
		{
			delete demux;
			demux = NULL;
		}
		
		return false;
	}

	if (numOfSessions > MAX_NUM_SESSIONS)
	{
		numOfSessions = MAX_NUM_SESSIONS;
	}

	demux->demux = ::demux_construct(numOfSessions, &demux->handle);

	::memcpy(&demux->addr, &descriptor->local_address, sizeof(rv_net_address));

	uint32_t multid = 0;
	::open_demux_session(descriptor, demux->demux, &multid, &demux->bindHandle);

	demux->port = port;
	m_vecDemux.push_back(demux);

	return true;
}

void XTDemuxMan::delete_demux(_XTDemux *demux)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);

	std::vector<_XTDemux*>::iterator itr = m_vecDemux.begin();
	for (;itr != m_vecDemux.end();++itr)
	{
		_XTDemux* d = *itr;
		if (demux != d)
		{
			continue;
		}

		if (d->demux)
		{
			::demux_deconstruct(demux->demux);
		}

		m_vecDemux.erase(itr);

		break;
	}
}

void XTDemuxMan::uninit()
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);

	std::vector<_XTDemux*>::iterator itr = m_vecDemux.begin();
	for (;itr != m_vecDemux.end();++itr)
	{
		_XTDemux* demux = *itr;
		if (!demux)
		{
			continue;
		}

		if (demux->demux)
		{
			::demux_deconstruct(demux->demux);
		}
	}

	m_vecDemux.clear();

	m_bInit = false;
}

_XTDemux* XTDemuxMan::get_demux(unsigned short port)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);

	_XTDemux *d = NULL;
	int num = MAX_NUM_SESSIONS;
	std::vector<_XTDemux*>::iterator itr = m_vecDemux.begin();
	for (;itr != m_vecDemux.end();++itr)
	{
		_XTDemux* demux = *itr;
		if (!demux)
		{
			continue;
		}

		if (demux->port != port)
		{
			continue;
		}

		int n = get_session(demux);
		if (num > n)
		{
			d = demux;
			num = n;
		}
	}

	return d;
}

void XTDemuxMan::add_session(_XTDemux *demux, rv_handler_s *hrv)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
	if (!hrv)
	{
		return;
	}

	std::vector<_XTDemux*>::iterator itr = m_vecDemux.begin();
	for (;itr != m_vecDemux.end();++itr)
	{
		_XTDemux* d = *itr;
		if (!d || d!=demux)
		{
			continue;
		}

		for (int nS = 0;nS < MAX_NUM_SESSIONS;++nS)
		{
			rv_handler_s &handle = d->subHandle[nS];
			if (!handle.hrtp)
			{
				::memcpy(&handle, hrv, sizeof(rv_handler_s));
				break;
			}
		}
	}
}

void XTDemuxMan::del_session(rv_handler_s *hrv)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);

	if (!hrv)
	{
		return;
	}

	std::vector<_XTDemux*>::iterator itr = m_vecDemux.begin();
	for (;itr != m_vecDemux.end();++itr)
	{
		_XTDemux* demux = *itr;
		if (!demux)
		{
			continue;
		}

		for (int nS = 0;nS < MAX_NUM_SESSIONS;++nS)
		{
			rv_handler_s &handle = demux->subHandle[nS];
			if (handle.hrtp == hrv->hrtp)
			{
				::memset(&handle, 0, sizeof(rv_handler_s));
				break;
			}
		}
	}
}

int XTDemuxMan::get_session(_XTDemux* demux)
{
	int num = 0;
	if (!demux)
	{
		return 0;
	}

	for (int nS = 0;nS < MAX_NUM_SESSIONS;++nS)
	{
		rv_handler_s &handle = demux->subHandle[nS];
		if (handle.hrtp)
		{
			num++;
		}
	}

	return num;
}

bool XTDemuxMan::open_session(rv_session_descriptor *descriptor, uint32_t *multiplexID, rv_handler_s *hrv,unsigned short port)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);

	bool ret = false;
	if (!descriptor || !multiplexID || !hrv)
	{
		return false;
	}

	if (!m_bInit)
	{
		for (int nI = 0;nI < m_numOfDemuxs;++nI)
		{
			rv_session_descriptor des;
			::memcpy(&des, descriptor, sizeof(rv_session_descriptor));

			rv_net_ipv4 addr;
			convert_rvnet_to_ipv4(&addr, &des.local_address);

			addr.port += 2*nI; 
			convert_ipv4_to_rvnet(&des.local_address, &addr);

			create_demux(&des, m_numOfSessions,port);
		}

		m_bInit = true;
	}

	_XTDemux *demux = get_demux(port);
	if (demux)
	{
		::memcpy(&descriptor->local_address, &demux->addr, sizeof(rv_net_address));
        ret = RV_ADAPTER_TRUE  == ::open_demux_session(descriptor, demux->demux, multiplexID, hrv) ? true : false;
		if (ret)
		{
			add_session(demux, hrv);
		}
	}
	else
	{
		//解决失败后无法使用问题 by songlei 20150415
		m_bInit = false;
	}

	return ret;
}

void XTDemuxMan::close_session(rv_handler_s *hrv)
{
	if (!hrv)
	{
		return;
	}

	del_session(hrv);

	::close_demux_session(hrv);
}
