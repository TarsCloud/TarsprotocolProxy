#include "TarsProxyServer.h"
#include "Common.h"
#include "ShareConnManager.h"
#include "TarsProxyImp.h"
#include "util/tc_network_buffer.h"

using namespace std;

TarsProxyServer g_app;

/**
 * 四层网关，主要两种应用场景: 
 * 1. 作为TARS协议的四层接入；
 * 2. 跨 IDC 的 TARS 请求透明转发
 * 除了支持基本的协议转发，还能够做被调用的服务及接口ip授权
 */

/////////////////////////////////////////////////////////////////
// 框架自动生产的代码
void TarsProxyServer::initialize()
{
    addServant<TarsProxyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".TarsProxyObj");

    if (!loadConf(true))
    {
        TLOG_DEBUG("load config fail!!!" << endl);
        exit(1);
    }

    // 添加协议解析器
    addServantProtocol(ServerConfig::Application + "." + ServerConfig::ServerName + ".TarsProxyObj", &tars::AppProtocol::parse);

    // 添加动态加载配置的命令
    TARS_ADD_ADMIN_CMD_NORMAL("loadProxyConf", TarsProxyServer::cmdLoadProxyConf);
}

bool TarsProxyServer::cmdLoadProxyConf(const string &command, const string &params, string &result)
{
    bool ret = loadConf(false);

    if (ret)
    {
        result = "loadConf succ";
    }
    else
    {
        result = "loadConf fail.";
    }

    return true;
}

// 通过配置获取真实的servant名称
string TarsProxyServer::getProxyObj(const string &servant, bool &hasProxy)
{
    hasProxy = false;
    TC_ThreadRLock r(_confRWLock);
    map<string, string>::iterator it = _proxy.find(servant);

    if (it != _proxy.end())
    {
        hasProxy = true;
        return it->second;
    }

    return servant;
}

// 控制 ip 白名单
bool TarsProxyServer::checkAuth(const string &ip, const string &obj, const string &func)
{
    TC_ThreadRLock r(_confRWLock);
    if (!_checkAuthOnOff)
    {
        return true;
    }

    if (_ipBlackList.find(ip) != _ipBlackList.end())
    {
        FDLOG("auth") << "ip_black_list error|" << ip << "|" << obj << "|" << func << endl;
        return false;
    }
    if (_ipWhiteList.find(ip) != _ipWhiteList.end())
    {
        FDLOG("auth") << "ip_white_list ok|" << ip << "|" << obj << "|" << func << endl;
        return true;
    }
    if (_serverWhiteList.find(obj) != _serverWhiteList.end())
    {
        FDLOG("auth") << "server_white_list ok|" << ip << "|" << obj << "|" << func << endl;
        return true;
    }

    map<string, set<string>>::iterator it = _authList.find(obj);
    if (it != _authList.end())
    {
        if (it->second.find(ip) != it->second.end())
        {
            FDLOG("auth") << "server ok|" << ip << "|" << obj << "|" << func << endl;
            return true;
        }
    }

    // 可以控制到 obj + func 级别
    it = _authList.find(obj + ":" + func);
    if (it != _authList.end())
    {
        if (it->second.find(ip) != it->second.end())
        {
            FDLOG("auth") << "server_func ok|" << ip << "|" << obj << "|" << func << endl;
            return true;
        }
    }

    FDLOG("auth") << "error|" << ip << "|" << obj << "|" << func << endl;
    return false;
}

// 加载配置
bool TarsProxyServer::loadConf(bool isStartMain)
{
    try
    {
        addConfig(ServerConfig::ServerName + ".conf");

        tars::TC_Config conf;
        conf.parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");

        {
            TC_ThreadWLock w(_confRWLock);

            map<string, string> proxy = conf.getDomainMap("/conf/proxy");
            for (map<string, string>::iterator it = proxy.begin(); it != proxy.end(); ++it)
            {
                string str1 = TC_Common::trim(it->first);
                string str2 = TC_Common::trim(it->second);
                if (str1.length() > 0 && str2.length() > 0)
                {
                    _proxy[str1] = str2;
                    TLOG_DEBUG("add proxy:" << str1 << "===>" << str2 << endl);
                }
            }

            string onOff = TC_Common::lower(TC_Common::trim(conf.get("/conf/auth<on_off>", "off")));
            if (onOff == "on")
            {
                _checkAuthOnOff = true;

                vector<string> vsList;
                vsList = conf.getDomainVector("/conf/auth");

                for (size_t i = 0; i < vsList.size(); i++)
                {
                    //struct LimitConf stLimitConf ;
                    string buss = vsList[i];
                    string strDomain = "/conf/auth/" + vsList[i];
                    map<string, string> tmp = conf.getDomainMap(strDomain);
                    for (map<string, string>::iterator it = tmp.begin(); it != tmp.end(); ++it)
                    {
                        vector<string> v = TC_Common::sepstr<string>(it->second, "|");
                        string k = TC_Common::trim(it->first);
                        for (size_t x = 0; x < v.size(); ++x)
                        {
                            _authList[k].insert(TC_Common::trim(v[x]));
                        }
                    }
                }

                TLOG_DEBUG("auth conf list:" << endl);
                for (map<string, set<string>>::iterator it = _authList.begin(); it != _authList.end(); ++it)
                {
                    TLOG_DEBUG(it->first << "===>" << TC_Common::tostr(it->second.begin(), it->second.end(), " ") << endl);
                }

                string sList = conf.get("/conf/auth<ip_white_list>");
                vector<string> vTmp = TC_Common::sepstr<string>(sList, "|");
                for (size_t i = 0; i < vTmp.size(); ++i)
                {
                    _ipWhiteList.insert(TC_Common::trim(vTmp[i]));
                }

                sList = conf.get("/conf/auth<ip_black_list>", "");
                vTmp.clear();
                vTmp = TC_Common::sepstr<string>(sList, "|");
                for (size_t i = 0; i < vTmp.size(); ++i)
                {
                    _ipBlackList.insert(TC_Common::trim(vTmp[i]));
                }

                sList = conf.get("/conf/auth<server_white_list>", "");
                vTmp.clear();
                vTmp = TC_Common::sepstr<string>(sList, "|");
                for (size_t i = 0; i < vTmp.size(); ++i)
                {
                    _serverWhiteList.insert(TC_Common::trim(vTmp[i]));
                }

                TLOG_DEBUG("ip white list:" << TC_Common::tostr(_ipWhiteList.begin(), _ipWhiteList.end(), " ") << endl);
                TLOG_DEBUG("ip black list:" << TC_Common::tostr(_ipBlackList.begin(), _ipBlackList.end(), " ") << endl);
                TLOG_DEBUG("server white list:" << TC_Common::tostr(_serverWhiteList.begin(), _serverWhiteList.end(), " ") << endl);
            }
            else
            {
                _checkAuthOnOff = false;
                TLOG_DEBUG("check auth conf off!" << endl);
            }
        }

        {
            // 跨IDC间 心跳配置
            map<string, string> proxyConn = conf.getDomainMap("/conf/proxy_conn");
            unsigned int interval = TC_Common::strto<unsigned int>(conf.get("/conf<tarsping_interval>", "0"));
            if (ShareConnManager::getInstance()->initShareProxy(proxyConn, interval))
            {
                if (isStartMain)
                {
                    ShareConnManager::getInstance()->start();
                    TLOG_DEBUG("initShareProxy success! start tars_ping thread!" << endl);
                }
                else
                {
                    TLOG_DEBUG("flash proxy_conn succ." << endl);
                }
            }
            else
            {
                TLOG_DEBUG("initShareProxy fail, proxyConn size:" << proxyConn.size() << ", interval:" << interval << endl);
            }
        }

        TLOG_DEBUG("loadAllConf succ." << endl);

        return true;
    }
    catch (exception &ex)
    {
        TLOG_ERROR("loadAllConf exception:" << ex.what() << endl);
    }

    return false;
}

/////////////////////////////////////////////////////////////////
// 框架自动生产的代码
void TarsProxyServer::destroyApp()
{
    // 框架自动生成
    // 该服务这里不需要添加业务逻辑
}

/////////////////////////////////////////////////////////////////
// 框架自动生产的代码
int main(int argc, char *argv[])
{
    try
    {
        g_app.main(argc, argv);
        g_app.waitForShutdown();
    }
    catch (std::exception &e)
    {
        cerr << "std::exception:" << e.what() << std::endl;
    }
    catch (...)
    {
        cerr << "unknown exception." << std::endl;
    }
    return -1;
}
/////////////////////////////////////////////////////////////////
