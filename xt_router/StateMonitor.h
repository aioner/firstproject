#ifndef STATEMONITEOR_H
#define STATEMONITEOR_H

#include <boost/noncopyable.hpp>
#include "xtXml.h"
#include <vector>
#include "InfoMgr.h"

class StateMonitor : boost::noncopyable
{
public:
    StateMonitor(void);
    ~StateMonitor(void);

public:
    static StateMonitor* instance()
    {
        return &m_Obj;
    }

private:

    static StateMonitor m_Obj;
    std::string strOldStatusMessage;

public:
    void        Monitor(std::string &strRet , int &playedFlag);
    void        setOldStatusMessage(std::string strStatusMessage);
    std::string getOldStatusMessage();
};

#endif //END STATEMONITEOR_H
