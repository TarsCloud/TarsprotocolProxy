#include "TarsCallback.h"
#include "Common.h"
#include "TarsProxyImp.h"
#include "servant/Application.h"
#include "tup/tup.h"
#include "util/tc_config.h"
#include "util/tc_hash_fun.h"
using namespace std;
using namespace tup;

// 处理回调逻辑
//////////////////////////////////////////////////////

TarsCallback::~TarsCallback()
{
}

int TarsCallback::onDispatch(ReqMessagePtr msg)
{
    if (msg->response->iRet == 0)
    {
        TLOG_DEBUG("================ getType:" << getType() << ", reqId:" << _iNewReqId << ", server ret succ."
                                               << ", " << _param->sServantName << ":" << _param->sFuncName << ", " << _param->clientIp << endl);
    }
    else
    {
        TLOG_ERROR("================ getType:" << getType() << ", reqId:" << _iNewReqId << ", server ret fail, iRet:" << msg->response->iRet
                                               << ", " << _param->sServantName << ":" << _param->sFuncName << ", " << _param->clientIp << endl);
    }

    int doRspRet = 0;
    if (getType() == "tup")
    {
        doRspRet = doResponse(msg->response->sBuffer);
        FDLOG("tupcall") << _param->clientIp << "|" << _param->sServantName << "|" << _param->sFuncName << "|" << msg->response->iRet << "|" << getCost() << "|" << msg->response->sBuffer.size() << "|" << doRspRet << endl;
    }
    else if (getType() == "tars")
    {
        doRspRet = doResponse(msg->response);
        FDLOG("tarscall") << _param->clientIp << "|" << _param->sServantName << "|" << _param->sFuncName << "|" << msg->response->iRet << "|" << getCost() << "|" << msg->response->sBuffer.size() << "|" << doRspRet << endl;
    }

    return 0;
}

int TarsCallback::doResponse(const vector<char> &buffer)
{
    try
    {
        TLOG_DEBUG("rsp buffer size:" << buffer.size() << endl);

        tars::TarsInputStream<BufferReader> is;
        if (buffer.size() <= 4)
        {
            TLOG_ERROR("buffer.size = " << buffer.size() << endl);
            return RTNCODE_RECVFROMSRV_ERRLEN;
        }
        else
        {
            is.setBuffer(&buffer[0] + 4, buffer.size() - 4);
        }

        // tup 协议
        RequestPacket req;
        req.readFrom(is);
        TLOGDEBUG("read tup RequestPacket succ." << endl);

        req.iRequestId = _param->iRequestId;
        req.sServantName = _param->sServantName;
        req.sFuncName = _param->sFuncName;

        tars::TarsOutputStream<BufferWriter> os;
        req.writeTo(os);
        return handleResponse(os.getBuffer(), os.getLength());
    }
    catch (exception &ex)
    {
        TLOG_ERROR("error:" << ex.what() << endl);
    }
    catch (...)
    {
        TLOG_ERROR("unknown error." << endl);
    }
    return RTNCODE_DORSP_EXCEPTION;
}

int TarsCallback::doResponse(shared_ptr<ResponsePacket> rsp)
{
    try
    {
        // 恢复回包相关参数
        rsp->iRequestId = _param->iRequestId;
        tars::TarsOutputStream<BufferWriter> os;
        rsp->writeTo(os);

        return handleResponse(os.getBuffer(), os.getLength());
    }
    catch (exception &ex)
    {
        TLOG_ERROR("error:" << ex.what() << endl);
    }
    catch (...)
    {
        TLOG_ERROR("unknown error." << endl);
    }
    return RTNCODE_DORSP_EXCEPTION;
}

int TarsCallback::handleResponse(const char *buff, size_t buffSize)
{
    try
    {
        //doRSP的时候也要加上头部4个字节
        size_t bufferlength = buffSize + 4;
        bufferlength = (size_t)(htonl(bufferlength));

        string rspBuff;
        rspBuff.append((char *)&bufferlength, 4);
        rspBuff.append(buff, buffSize);

        TLOG_DEBUG(_param->sServantName << "::" << _param->sFuncName << ", requestid:"
                                        << _param->iRequestId << ", rsp length:"
                                        << bufferlength << endl);

        if (rspBuff.length() > g_rspSizeLimit)
        {
            TLOG_ERROR("packet is too big|" << rspBuff.length()
                                                << "|" << _param->sServantName
                                                << "|" << _param->sFuncName
                                                << endl);
            return RTNCODE_RSP_TOOLONG;
        }

        // 如果客户端请求类型是单向调用， 就不用回包 
        if (_param->cPacketType != tars::TARSONEWAY)
        {
            _current->sendResponse(rspBuff.c_str(), rspBuff.size());
            TLOG_DEBUG("send rsp ok, size:" << rspBuff.size() << endl);
        }
        else
        {
            TLOG_DEBUG("request is TARSONEWAY, donot send rsp." << endl);
            return RTNCODE_ONEWAY_NORSP;
        }

        return 0;
    }
    catch (exception &ex)
    {
        TLOG_ERROR("error:" << ex.what() << endl);
    }
    catch (...)
    {
        TLOG_ERROR("unknown error." << endl);
    }

    return RTNCODE_DORSP_EXCEPTION;
}

//////////////////////////////////////////////////////
