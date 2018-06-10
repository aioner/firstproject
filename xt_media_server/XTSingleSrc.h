#ifndef XTSINGLESRC_H__INCLUDE__
#define XTSINGLESRC_H__INCLUDE__
#include "XTSrc.h"
#include <boost/thread/shared_mutex.hpp>

using namespace std;

struct src_inf 
{
    int src_prime;
    bool use;
};

class XTSingleSrc
{
private:
    XTSingleSrc(void);
    ~XTSingleSrc(void);

public:
    static XTSingleSrc* inst() {  return &self; }
    static XTSingleSrc* _() {  return &self; }

    // ����ɾ��
    int add_src(int srcno, int srcno_prime);
    int del_src(int srcno_prime);
    int del_src2(int srcno);

    // ʹ��
    void use_src(int srcno, bool use);

    // ��ÿ���ת��Դ
    int get_freesrc(int srcno_prime, int &srcno);

    // ������ת��Դ
    bool has_freesrc(int srcno_prime);

    // ��ȡ
    int get_src(int srcno_prime, std::vector<int> &vsrc);

    //��ȡʵ��ʹ�õ�srcno
    void get_physical_srcno(const int srcno_prime,std::list<int>& srcnos);

private:
    boost::shared_mutex    m_srcs_mutex;     // mutex
    map<int, src_inf>      m_srcs;      // ����ת��Դ(ת����ͬ����)
    static XTSingleSrc     self;
};
#endif//#ifndef XTSINGLESRC_H__INCLUDE__
