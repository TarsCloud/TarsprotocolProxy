#ifndef __COMMON_H__
#define __COMMON_H__
#include "servant/Application.h"
#include "servant/RemoteLogger.h"

#define STAT_LOG FDLOG("stat")

const int RTNCODE_PROXY_NULL = -99;
const int RTNCODE_PROXY_EXCEPTION = -100;
const int RTNCODE_REQ_PARAM_ERR = -101;
const int RTNCODE_OBJ_ERROR = -102;
const int RTNCODE_RECVFROMSRV_ERRLEN = -103;
const int RTNCODE_SENDTOSRV_ERR = -104;
const int RTNCODE_RECVFROMSRV_ERR = -105;
const int RTNCODE_RSP_TOOLONG = -106;
const int RTNCODE_DORSP_EXCEPTION = -199;
const int RTNCODE_ONEWAY_NORSP = 1;

// 出于性能考虑， 这里回包限制不要太大
const size_t g_rspSizeLimit = 9 * 1024 * 1024 - 5;

#define LOG_MSG(level, msg...)                               \
    do                                                       \
    {                                                        \
        if (LOG->isNeedLog(level))                           \
            LOG->log(level) << FILE_FUNC_LINE << "|" << msg; \
    } while (0)

#define TLOG_DEBUG(msg...) LOG_MSG(LocalRollLogger::DEBUG_LOG, msg)
#define TLOG_ERROR(msg...) LOG_MSG(LocalRollLogger::ERROR_LOG, msg)

#endif
