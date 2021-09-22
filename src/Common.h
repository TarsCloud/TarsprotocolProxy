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

const size_t g_rspSizeLimit = 9 * 1024 * 1024 - 5;

#endif
