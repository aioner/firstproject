#ifndef RTP_PACKET_BLOCK_H
#define RTP_PACKET_BLOCK_H

//#include "stdafx.h"
#include "sink_common.h"

namespace xt_mp_sink
{
	//rtp数据块，
	class rtp_block : public tghelper::byte_block
	{
	public:
		rtp_block(const uint32_t block_size) : tghelper::byte_block(block_size), m_bind_block(0), m_resend(false)
		{		}
		rtp_block() : tghelper::byte_block(2048), m_bind_block(0), m_resend(false)
		{		}
	protected:
		virtual void recycle_alloc_event()
		{
			m_bFrameInfo = false;
			memset(&m_infoFrame, 0, sizeof(XTFrameInfo));
			memset(&m_rtp_param, 0, sizeof(rv_rtp_param));
			m_priority = 0;
			m_resend = false;
			m_use_ssrc = false;
		}
		virtual void recycle_release_event()
		{
			if (m_bind_block)
			{
				m_bind_block->release();
				m_bind_block = 0;
			}
		}

	public:
		void set_rtp_param(rv_rtp_param *rtp_param)
		{ memcpy(&m_rtp_param, rtp_param, sizeof(rv_rtp_param)); }

		void ser_rtp_prority(uint8_t prority)
		{
			m_priority = prority;
		}
		void set_bind_block(byte_block *bind_block)
		{
			if (m_bind_block)
			{
				m_bind_block->release();
			}
			m_bind_block = bind_block;
			if (m_bind_block)
			{
				m_bind_block->assign();
			}
		}
		inline byte_block *get_bind_block() { return m_bind_block; }
	public:
		rv_rtp_param m_rtp_param;

		bool m_use_ssrc;

		uint32_t m_ssrc;

		bool m_bFrameInfo;
		XTFrameInfo m_infoFrame;

		uint8_t m_priority;

		bool m_resend;

		uint32_t m_exHead[16];
	private:
		byte_block * m_bind_block;		//外挂用户block
	};
    class rtp_packet_block : public tghelper::byte_block
    {
    public:
        enum{ MAX_SEQUENCE_NUM = 65536 };
        rtp_packet_block(const uint32_t block_size)
            : tghelper::byte_block(block_size)
        {
            m_bFrame = false;
            ::memset(&m_frame, 0, sizeof(m_frame));
            memset(&m_head,0,sizeof(rv_rtp_param));
        }
		rtp_packet_block()
			: tghelper::byte_block(2048)//实际打印出来len为1036
		{
			m_bFrame = false;
			::memset(&m_frame, 0, sizeof(m_frame));
			memset(&m_head,0,sizeof(rv_rtp_param));
		}
        virtual ~rtp_packet_block(void){}

    public:	/*properties*/

        inline uint16_t get_sn(){ return m_head.sequenceNumber; }
        inline void set_sn(uint16_t sn) { m_head.sequenceNumber = sn; }

        inline int32_t get_sbyte() { return m_head.sByte; }
        inline void set_sbyte(int32_t sbyte) { m_head.sByte = sbyte; }

        inline int32_t get_size() { return m_head.len; }
        inline void set_size(int32_t hsize) { m_head.len = hsize; }

        inline uint32_t get_ssrc() { return m_head.sSrc; }
        inline void set_ssrc(uint32_t ssrc) { m_head.sSrc = ssrc; }

        inline uint32_t get_ts() { return m_head.timestamp; }
        inline void set_ts(uint32_t ts) { m_head.timestamp = ts; }

        inline uint8_t get_marker() { return m_head.marker; }
        inline void set_marker(uint16_t marker) { m_head.marker = marker; }

        inline rv_rtp_param * get_head() { return &m_head; }

        inline uint8_t get_payload_type() { return m_head.payload; }
        inline void set_payload_type(uint8_t type) { m_head.payload = type; }

    public: /*重载事件*/

        virtual void recycle_alloc_event(){}
        virtual void recycle_release_event(){}

    public:	/*方法*/

        inline rv_bool is_marker1() { return m_head.marker; }

        //当前SN比输入SN小
        inline bool is_smaller(rtp_packet_block * item)
        {
            if(is_ts_smaller(item))
            {
                return true;
            }
            else
            {
                return is_sn_smaller(item);
            }
        }

        inline bool is_equal_sn(rtp_packet_block * item)
        {
            return item->get_sn() == m_head.sequenceNumber;
        }

        inline int32_t get_distance(rtp_packet_block * item)
        {
            return uint16_t(item->get_sn() - m_head.sequenceNumber) + 1;
        }

        inline bool is_nequal_timestamp(rtp_packet_block * item)
        {
            return item->get_ts() != m_head.timestamp;
        }

        //function
        //数据内容操作
        // 外部内容压入 -- 返回实际压入数量 -- write
        uint32_t write(const uint8_t *src, uint32_t src_size,
            uint32_t offset, const rv_rtp_param& head)
        {
            memcpy(&m_head,&head,sizeof(rv_rtp_param));
            //__super::write(src,src_size,offset);
            return tghelper::byte_block::write(src,src_size,offset);
        }
        uint32_t write(const char *src, uint32_t src_size,
            uint32_t offset, const rv_rtp_param& head)
        {
            return write((char *)src, src_size, offset,head);
        }
        // 内部数据弹出 -- 返回实际弹出数量 -- read
        // offset变量有效则带offset内容弹出，否则仅弹出payload部分
        uint32_t read(uint8_t *dst, uint32_t dst_size, rv_rtp_param& head,uint32_t *offset = 0)
        {
            memcpy(&head,&m_head,sizeof(rv_rtp_param));
            //return __super::read(dst,dst_size,offset);
            return tghelper::byte_block::read(dst,dst_size,offset);
        }
        uint32_t read(char *dst, uint32_t dst_size, rv_rtp_param& head, uint32_t *offset = 0)
        { return read((uint8_t*)dst, dst_size, head, offset); }

        inline void set_head_param(const rv_rtp_param& param)
        {
            memcpy(&m_head,&param,sizeof(rv_rtp_param));
        }

    protected:

        inline bool is_sn_smaller(rtp_packet_block * item)
        {
            if (!item)
            {
                return false;
            }

            if (m_head.sequenceNumber < item->get_sn())
            {
                return true;
            }

            if (m_head.sequenceNumber >= MP_SN_JUDGE_CONDITION)
            {
                if (m_head.sequenceNumber - item->get_sn() >= MP_SN_JUDGE_CONDITION)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
            //return uint16_t(m_head.sequenceNumber - item->get_sn()) > MP_SN_JUDGE_CONDITION;
        }

        inline bool is_ts_smaller(rtp_packet_block * item)
        {
            if (!item)
            {
                return false;
            }

            if (m_head.timestamp < item->get_ts())
            {
                return true;
            }

            if (m_head.timestamp >= MP_TS_JUDGE_CONDITION)
            {
                if (m_head.sequenceNumber - item->get_sn() >= MP_TS_JUDGE_CONDITION)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
            //return uint32_t(m_head.timestamp - item->get_ts()) > MP_SN_JUDGE_CONDITION;
        }

    public:

        rv_rtp_param m_head;

    public:

        bool m_bFrame;
        uint32_t m_frame[3];//12字节私有头(校验+datatype+frametype)
    };
}

#endif
