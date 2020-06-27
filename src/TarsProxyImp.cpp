#include "TarsProxyImp.h"
#include "Common.h"
#include "ShareConnManager.h"
#include "TarsCallback.h"
#include "TarsProxyServer.h"
#include "servant/Application.h"
#include "util/tc_hash_fun.h"

#include <atomic>

using namespace std;

std::atomic<int> g_iRequestId(0);
const string CTX_ENDPOINT = "CTX_ENDPOINT";
const string CTX_TAFHASH_KEY = "CTX_TAFHASH_KEY";

//////////////////////////////////////////////////////
void TarsProxyImp::initialize()
{
    //initialize servant here:
    //...
}

//////////////////////////////////////////////////////
void TarsProxyImp::destroy()
{
    //destroy servant here:
    //...
}

/*
*
*/

int TarsProxyImp::doClose(tars::CurrentPtr current)
{
    FDLOG("conn") << "doClose|" << current->getUId() << "|" << current->getIp() << "|" << current->getPort() << endl;

    return 0;
}

int TarsProxyImp::doRequest(tars::CurrentPtr current, vector<char> &response)
{
    try
    {
        FDLOG("conn") << "doRequest|" << current->getUId() << "|" << current->getIp() << "|" << current->getPort() << endl;

        const vector<char> &request = current->getRequestBuffer();

        TLOG_DEBUG("request len:" << request.size() << endl);

        if (request.size() < 4)
        {
            TLOG_ERROR("error request size." << request.size() << endl);
            return RTNCODE_REQ_PARAM_ERR;
        }

        tars::TarsInputStream<BufferReader> is;

        is.setBuffer(&request[0], request.size());

        RequestPacket reqPacket;

        reqPacket.readFrom(is);

        TLOG_DEBUG(reqPacket.iRequestId << ", servant:" << reqPacket.sServantName << ", func:" << reqPacket.sFuncName << ", requestPacket size:" << request.size() << endl);

        if (reqPacket.sServantName.empty())
        {
            TLOG_ERROR("error, servant is empty()" << endl);
            return RTNCODE_REQ_PARAM_ERR;
        }

        if (!g_app.checkAuth(current->getIp(), reqPacket.sServantName, reqPacket.sFuncName))
        {
            TLOG_ERROR("check auth fail!" << current->getIp() << ", " << reqPacket.sServantName << ", " << reqPacket.sFuncName << endl);
            current->close();
            return 0;
        }

        current->setResponse(false);

        if (reqPacket.sFuncName == "tars_ping")
        {
            TLOG_DEBUG("tars_ping..." << endl);
            ResponsePacket rsp;
            rsp.iVersion = reqPacket.iVersion;
            rsp.cPacketType = reqPacket.cPacketType;
            rsp.iRequestId = reqPacket.iRequestId;
            rsp.iMessageType = reqPacket.iMessageType;
            rsp.iRet = tars::TARSSERVERSUCCESS;

            tars::TarsOutputStream<BufferWriter> os;
            rsp.writeTo(os);

            //doRSP的时候也要加上头部4个字节
            unsigned int bufferlength = os.getLength() + 4;
            bufferlength = htonl(bufferlength);

            string rspBuff;
            rspBuff.append((char *)&bufferlength, 4);
            rspBuff.append(os.getBuffer(), os.getLength());
            current->sendResponse(rspBuff.c_str(), rspBuff.size());
            return 0;
        }
        else
        {
            bool hasProxy = false;
            string reqObj;
            reqObj = g_app.getProxyObj(reqPacket.sServantName, hasProxy);
            if (!hasProxy && reqPacket.context.find(CTX_ENDPOINT) != reqPacket.context.end() && reqPacket.context[CTX_ENDPOINT].length() > 0)
            {
                reqObj = reqPacket.sServantName + "@" + reqPacket.context[CTX_ENDPOINT];
            }

            return doRequest_tars_async(reqObj, reqPacket, request, current);
        }
    }
    catch (exception &ex)
    {
        TLOG_ERROR("exception:" << ex.what() << endl);
    }
    catch (...)
    {
        TLOG_ERROR("exception: unknown exception." << endl);
    }

    return RTNCODE_PROXY_EXCEPTION;
}

int TarsProxyImp::doRequest_tars_async(const string &sObj, RequestPacket &reqPacket, const vector<char> &request, tars::CurrentPtr current)
{
    try
    {
        ServantPrx proxy = ShareConnManager::getInstance()->getProxy(sObj);

        if (isNullPtr(proxy))
        {
            TLOG_ERROR("proxy null, obj:" << sObj << endl);
            return RTNCODE_OBJ_ERROR;
        }

        TLOG_DEBUG(sObj << ", func:" << reqPacket.sFuncName << endl);

        shared_ptr<TarsCallbackParam> param(new TarsCallbackParam());
        param->iRequestId = reqPacket.iRequestId;
        param->sServantName = reqPacket.sServantName;
        param->sFuncName = reqPacket.sFuncName;
        param->clientIp = current->getIp();
        param->cPacketType = reqPacket.cPacketType;

        //设置新的tup requestid
        reqPacket.iRequestId = ++g_iRequestId;

        //设置TAF服务的ServantName
        reqPacket.sServantName = string(sObj.substr(0, sObj.find("@")));

        reqPacket.cPacketType = tars::TARSNORMAL; // 这里强制要求回包

        if (reqPacket.context.find("REMOTE_IP") == reqPacket.context.end())
        {
            reqPacket.context["REMOTE_IP"] = current->getIp();
        }

        //重新编码
        tars::TarsOutputStream<BufferWriter> buffer;
        reqPacket.writeTo(buffer);

        unsigned int bufferlength = buffer.getLength() + 4;
        bufferlength = htonl(bufferlength);

        string s;
        s.append((char *)&bufferlength, 4);
        s.append(buffer.getBuffer(), buffer.getLength());

        TLOG_DEBUG("req.iVersion:" << reqPacket.iVersion
                                   << ", req.cPacketType:" << (int)reqPacket.cPacketType
                                   << ", req.sFuncName:" << reqPacket.sFuncName
                                   << ", vReqBuff.size():" << reqPacket.sBuffer.size()
                                   << ", servantName:" << reqPacket.sServantName << ", request:" << s.size() << endl);

        string type = (reqPacket.iVersion == TUPVERSION ? "tup" : "tars");
        TarsCallbackPtr cb = new TarsCallback(type, current, param, reqPacket.iRequestId);
        string hash_flag;
        if (reqPacket.context.find(CTX_TAFHASH_KEY) != reqPacket.context.end() && reqPacket.context[CTX_TAFHASH_KEY].length() > 0)
        {
            proxy->tars_hash(tars::hash<string>()(reqPacket.context[CTX_TAFHASH_KEY]))->rpc_call_async(reqPacket.iRequestId, reqPacket.sFuncName, s.c_str(), s.size(), cb);
            hash_flag = " --->hashkey:" + reqPacket.context[CTX_TAFHASH_KEY];
        }
        else
        {
            proxy->rpc_call_async(reqPacket.iRequestId, reqPacket.sFuncName, s.c_str(), s.size(), cb);
        }

        TLOG_DEBUG(sObj << ", Tars Call===>func:" << reqPacket.sFuncName << " do rpc_call_async ok, hashflag:" << hash_flag << endl);

        return 0;
    }
    catch (exception &ex)
    {
        TLOG_ERROR("exception:" << ex.what() << endl);
    }

    return RTNCODE_PROXY_NULL;
}
