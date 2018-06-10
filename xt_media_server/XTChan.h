#ifndef XTCHAN_H__INCLUDE__
#define XTCHAN_H__INCLUDE__

#include <map>
#include <boost/thread/recursive_mutex.hpp>
using namespace std;

// ͨ����Ϣ
struct  xt_chan
{
    bool			active;		// �Ƿ񼤻�
    int				type;		// 0:˽�� 1:��׼ 2:��һת��
    unsigned int	sink;		// ת����
};

class XTChan
{
private:
    XTChan(void);
    ~XTChan(void);
    static XTChan self;
public:
    static XTChan* instance(){return &self;}
    static XTChan* _(){return &self;}

    bool is_chan_used(const unsigned long chaid);

    // ����ͨ��
    int add_chan(unsigned long chanid, int num = 1);

    // ����ͨ��
    int active_chan(unsigned long chanid, bool active);

    // ����/ɾ��ת��
    int add_sink(unsigned long chanid);
    int del_sink(unsigned long chanid);
    // ���ñ�׼ͨ��
    int set_stdchan(unsigned long chanid);

    // ���ñ���ͨ��
    int set_candchan(unsigned long chanid);
    // ��ÿ���ͨ��
    int get_freechan(unsigned long &chanid);
    int get_freestdchan(unsigned long &chanid);
    int get_freecandchan(unsigned long &chanid);

    // ���ͨ����Ϣ
    int get_chan_info(unsigned long chanid, xt_chan &chan);

private:
    boost::recursive_mutex		m_mutex;		//mutex
    map<unsigned long, xt_chan>		m_chans;		//ͨ����Ϣ
};
#endif//XTCHAN_H__INCLUDE__
