#ifndef _TarsProxyImp_H_
#define _TarsProxyImp_H_

#include "servant/Application.h"
//#include "TarsProxy.h"

/**
 *
 *
 */
class TarsProxyImp : public tars::Servant
{
  public:
    /**
	 *
	 */
    virtual ~TarsProxyImp() {}

    /**
	 *
	 */
    virtual void initialize();

    /**
	 *
	 */
    virtual void destroy();

    int doClose(tars::CurrentPtr current);
    /**
     * 处理客户端的主动请求
     * @param current 
     * @param response 
     * @return int 
     */
    virtual int doRequest(tars::CurrentPtr current, vector<char> &response);

  protected:
    int doRequest_tars_async(const string &sObj, RequestPacket &tup, const vector<char> &request, tars::CurrentPtr current);

    template <typename T>
    static bool isNullPtr(T ptr)
    {
        return ((long int)0) == ((long int)ptr);
    }
};
/////////////////////////////////////////////////////
#endif
