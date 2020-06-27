#include "ShareConnManager.h"
#include "servant/Application.h"
#include "util/tc_config.h"

ShareConnManager::ShareConnManager()
{
    _terminate = false;
    _tarsPingInterval = 5;
    _prot_total.requestFunc = ProxyProtocol::streamRequest;
    _prot_total.responseFunc = ProxyProtocol::totalResponse;
}

void ShareConnManager::terminate()
{
    _terminate = true;

    TC_ThreadLock::Lock lock(*this);
    notifyAll();
}

bool ShareConnManager::initShareProxy(const map<string, string> &m, unsigned int interval)
{
    _nameMap = m;

    TC_ThreadWLock w(_proxyRWLock);

    // 这里先直接初始化共享连接的proxy
    map<string, string>::iterator it = _nameMap.begin();
    for (; it != _nameMap.end(); ++it)
    {
        ServantPrx proxy = Application::getCommunicator()->stringToProxy<ServantPrx>(it->second);
        proxy->tars_set_protocol(_prot_total);
        _proxyMap[it->first] = proxy;
        _shareProxy.push_back(proxy);

        TLOG_DEBUG("init share proxy, " << it->first << "|" << it->second << endl);
    }

    if (_shareProxy.size() > 0 && interval != 0)
    {
        if (interval > 1800)
        {
            interval = 5;
        }

        _tarsPingInterval = interval;

        TLOG_DEBUG("init share proxy succ! tars_ping interval=" << _tarsPingInterval << "(s)" << endl);
        return true;
    }

    return false;
}

ServantPrx ShareConnManager::getProxy(const string &sServantName)
{
    {
        TC_ThreadRLock r(_proxyRWLock);
        map<string, ServantPrx>::iterator it = _proxyMap.find(sServantName);
        if (it != _proxyMap.end())
        {
            TLOG_DEBUG("rpc-call return exist proxy---------------> " << it->second->tars_name() << " :: " << sServantName << endl);
            return it->second;
        }
    }

    //绝大部分情况下, 运行到上面就直接返回了
    ServantPrx proxy = Application::getCommunicator()->stringToProxy<ServantPrx>(sServantName);
    proxy->tars_set_protocol(_prot_total);

    {
        TC_ThreadWLock w(_proxyRWLock);
        _proxyMap[sServantName] = proxy;
        TLOG_DEBUG("rpc-call create new proxy---------------> " << sServantName << " :: " << sServantName << endl);
    }

    return proxy;
}

void ShareConnManager::run()
{

    while (!_terminate)
    {

        try
        {
            for (size_t i = 0; i < _shareProxy.size(); ++i)
            {
                try
                {
                    _shareProxy[i]->tars_ping();
                    TLOG_DEBUG(_shareProxy[i]->tars_name() << ", do tars_ping finish." << endl);
                }
                catch (exception &ex)
                {
                    TLOG_ERROR("exception:" << ex.what() << endl);
                }
                catch (...)
                {
                    TLOG_ERROR("exception: unkonw error!" << endl);
                }
            }
        }
        catch (exception &ex)
        {
            TLOG_ERROR("[ShareConnManager]:" << ex.what() << endl);
        }
        catch (...)
        {
            TLOG_ERROR("[ShareConnManager] unknown error." << endl);
        }

        TC_ThreadLock::Lock lock(*this);
        timedWait(1000 * _tarsPingInterval);
    }
}
