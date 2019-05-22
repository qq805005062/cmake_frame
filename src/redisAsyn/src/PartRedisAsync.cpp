#include <stdarg.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#include <async.h>

#include "Incommon.h"
#include "OrderInfo.h"
#include "LibeventIo.h"
#include "../RedisAsync.h"

/*
 * [PartRedisAsync.cpp] 这个文件存在的意义就是RedisAsync.cpp文件里面内容太多了。分担部分代码过来。保证代码可读性
 * @author xiaoxiao 2019-05-22
 * @param
 * @param
 * @param
 *
 * @return
 */
namespace CLUSTER_REDIS_ASYNC
{
void RedisAsync::asyncStateCallBack(int asyFd, int stateCode, const std::string& stateMsg)
{
    if(stateCb_)
    {
        switch(stateCode)
        {
            case REDIS_SVR_INIT_STATE:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "init state %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
            case REDIS_SVR_RUNING_STATE:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "running ok %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
            case REDIS_EXCEPTION_STATE:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "part exception %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
            case REDIS_SVR_ERROR_STATE:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "error state %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
            case REDIS_SVR_INVALID_STATE:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "redis svr invalid state %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
            default:
                {
                    char stateMsgBuf[1024] = {0};
                    sprintf(stateMsgBuf, "unknow state %s", stateMsg.c_str());
                    stateCb_(asyFd, stateCode, stateMsgBuf);
                    break;
                }
        }
    }
}

void RedisAsync::asyncExceCallBack(int asyFd, int exceCode, const std::string& exceMsg)
{
    if(exceCb_)
    {
        switch(exceCode)
        {
            case EXCE_MALLOC_NULL:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "new or malloc nullptr %s", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
            default:
                {
                    char exceMsgBuf[1024] = {0};
                    sprintf(exceMsgBuf, "%s unkown exception", exceMsg.c_str());
                    exceCb_(asyFd, exceCode, exceMsgBuf);
                    break;
                }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//以下可以自定义一些接口、命令函数。
//方法入口可以不做参数校验，redisAsyncCommand方法做了统一的严格的参数校验
int RedisAsync::set(int asyFd, const std::string& key, const std::string& value, int outSecond, const CmdResultCallback& cb, void *priv)
{
    char cmdBuf[1024] = {0};

    sprintf(cmdBuf, "set %s %s", key.c_str(), value.c_str());
    return redisAsyncCommand(asyFd, cb, outSecond, priv, key, cmdBuf);
}

int RedisAsync::redisAsyncCommand(int asyFd, const CmdResultCallback& cb, int outSecond, void *priv, const std::string& key, const char *format, ...)
{
    char *cmd = NULL;
    va_list ap;
    va_start(ap, format);

    int len = redisvFormatCommand(&cmd, format, ap);
    if (len == -1)
    {
     return CMD_PARAMETER_ERROR_CODE;
    }
    std::string cmdStr(cmd);
    if(cmd)
    {
     free(cmd);
    }
    va_end(ap);
    return redisAsyncCommand(asyFd, cb, outSecond, priv, key, cmdStr);;
}

}

