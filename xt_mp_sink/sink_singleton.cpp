//#include "stdafx.h"
#include "sink_singleton.h"
#include "sink_config.h"

namespace xt_mp_sink
{
	int32_t sink_inst::mp_sink_init(sink_init_descriptor * sink_des)
	{
		
		//标准流线程池数量
		//sink_des->sink_thread_num = 16;
		//性能最好的情况是sink实体数量等于线程数量
		_tp = new boost::threadpool::pool(sink_des->sink_thread_num);
		printf("\nxt_mp_sink threadnums:%d\n",sink_des->sink_thread_num);
		//私有流线程池数量
		_post_tp = new boost::threadpool::pool(sink_des->post_thread_num);

		//rv_adapter只有一个线程，只做可读事件通知
		rv_adapter_descriptor des;
		des.thread_nums = 1;

		return init_rv_adapter(&des) ? 0 : -1;
	}
	void sink_inst::add_ent(mp_entity* ent)
	{
		boost::unique_lock<boost::shared_mutex> lock(m_ment);

		m_map_ents[ent] = ent;
	}

	void sink_inst::del_ent(mp_entity* ent)
	{
		boost::unique_lock<boost::shared_mutex> lock(m_ment);
		if (m_map_ents.find(ent) == m_map_ents.end())
		{
			return;
		}

		m_map_ents.erase(ent);
	}

	void sink_inst::mp_task_heart()
	{
		while (sink_inst::sink_singleton()->m_run)
		{
			int heart = sink_config::inst()->rtp_heart(0);
			if (heart > 0)
			{
				do
				{
					boost::shared_lock<boost::shared_mutex> lock(sink_inst::sink_singleton()->m_ment);
					std::map<mp_entity*,mp_entity*> &ents = sink_inst::sink_singleton()->m_map_ents;
					std::map<mp_entity*,mp_entity*>::iterator itr = ents.begin();
					for (;itr != ents.end();++itr)
					{
						mp_entity* ent = itr->second;
						if (!ent || ent->m_state != 0x0002)
						{
							continue;
						}

						char natbuf[96];
						rv_rtp_param rtp_param;
						::construct_rv_rtp_param(&rtp_param);

						rtp_param.sByte = 16;
						rtp_param.sequenceNumber = 0;
						rtp_param.payload = 96;
						rtp_param.timestamp = 0;		
						write_rtp(&ent->m_handle, natbuf, 32, &rtp_param); 
					}
				}while(false);
			}

			boost::this_thread::sleep(boost::posix_time::seconds(3));
		}	
	}
}
