#ifndef _TarsCallback_H_
#define _TarsCallback_H_

#include "servant/Application.h"

using namespace std;
using namespace tars;

struct TarsCallbackParam
{
    TarsCallbackParam() : cPacketType(0) {}
    int iRequestId;
    string sServantName;
    string sFuncName;
    string clientIp;
    char cPacketType;
};

/**
 * 异步回调对象
 */
class TarsCallback : public ServantProxyCallback
{
  public:
    TarsCallback(const string &type,
                 TarsCurrentPtr current,
                 shared_ptr<TarsCallbackParam> param,
                 int reqId)
        : _param(param), _current(current), _iNewReqId(reqId)
    {
        setType(type);
        _beginTime = TC_Common::now2ms();
    }

    /**
     * 析构
     */
    virtual ~TarsCallback();

    virtual int onDispatch(ReqMessagePtr msg);

  protected:
    /**
     * 响应
     * 
     * @param buffer 
     */
    int doResponse(shared_ptr<ResponsePacket> tupRsp);
    int doResponse(const vector<char> &buffer);

    int handleResponse(const char *buff, size_t buffSize);

    uint64_t getCost()
    {
        return TC_Common::now2ms() - _beginTime;
    }

  protected:
    /**
     * 所有的回应包
     */

    string _rspBuff;

    uint64_t _beginTime;

    /**
	* 请求相关参数
	*/
    shared_ptr<TarsCallbackParam> _param;

    tars::CurrentPtr _current;
    int _iNewReqId;
};

typedef TC_AutoPtr<TarsCallback> TarsCallbackPtr;

/////////////////////////////////////////////////////
#endif
