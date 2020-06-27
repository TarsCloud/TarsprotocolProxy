#ifndef _SHARECONNMANAGER_H_
#define _SHARECONNMANAGER_H_

#include "Common.h"
#include "servant/Application.h"
#include "util/tc_monitor.h"
#include "util/tc_singleton.h"
#include "util/tc_thread_rwlock.h"

using namespace std;
using namespace tars;

/**
 * 管理需要共享TarsProxy连接的代理
 */
class ShareConnManager : public TC_Singleton<ShareConnManager>, public TC_ThreadLock, public TC_Thread
{
  public:
    friend typename TC_Singleton<ShareConnManager>::TCreatePolicy;

    /**
     * 加载配置
     * 
     * @return string 
     */
    bool initShareProxy(const map<string, string> &m, unsigned int interval);

    /**
     * 获取代理
     * 
     * @param tup 
     * 
     * @return ServantPrx 
     */
    ServantPrx getProxy(const string &sServantName);

    /**
     * 结束
     */
    void terminate();

  protected:
    virtual void run();

    /**
     * 构造
     */
    ShareConnManager();

  protected:
    map<string, string> _nameMap;

    map<string, ServantPrx> _proxyMap;
    vector<ServantPrx> _shareProxy;

    bool _terminate;

    TC_ThreadRWLocker _proxyRWLock;

    unsigned int _tarsPingInterval;

    ProxyProtocol _prot_total;
};

/////////////////////////////////////////////////////
#endif
