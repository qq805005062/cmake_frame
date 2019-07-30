

#include "Incommon.h"
#include "RedisClient.h"

namespace REDIS_ASYNC_CLIENT
{

RedisClient::RedisClient(const RedisNodeAddInfoPtr& clientInfo, const RedisInitParPtr& initPar)
    :clientAddrInfo_(clientInfo)
    ,redisInitPar_(initPar)
    ,masterSockets_()
    ,slaveSockets_()
    ,orderDeque_()
{
    PDEBUG("RedisClient init");
}

//这个类除了new之外所有的操作都应该在一个线程中完成，千万不要跨线程
RedisClient::~RedisClient()
{
    PERROR("~RedisClient exit");
}



void RedisClient::requestCmd(const common::OrderNodePtr& order, uint64_t nowSecond)
{
   
}

void RedisClient::checkOutSecondCmd(uint64_t nowSecond)
{
    
}



}

