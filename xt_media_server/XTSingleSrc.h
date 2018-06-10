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

    // 增加删除
    int add_src(int srcno, int srcno_prime);
    int del_src(int srcno_prime);
    int del_src2(int srcno);

    // 使用
    void use_src(int srcno, bool use);

    // 获得空闲转发源
    int get_freesrc(int srcno_prime, int &srcno);

    // 监测空闲转发源
    bool has_freesrc(int srcno_prime);

    // 获取
    int get_src(int srcno_prime, std::vector<int> &vsrc);

    //获取实际使用的srcno
    void get_physical_srcno(const int srcno_prime,std::list<int>& srcnos);

private:
    boost::shared_mutex    m_srcs_mutex;     // mutex
    map<int, src_inf>      m_srcs;      // 关联转发源(转发相同数据)
    static XTSingleSrc     self;
};
#endif//#ifndef XTSINGLESRC_H__INCLUDE__
