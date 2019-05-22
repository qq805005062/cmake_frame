#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <string.h>
#include <pthread.h>
#include <iostream>

#include "RedisAsync.h"

#define PTRACE(fmt, args...)        fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PDEBUG(fmt, args...)        fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)        fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

static int singleFd = 0, clusterFd = 0;

static void stateChangeCallback(int asyFd, int stateCode, const std::string& stateMsg)
{
    PDEBUG("asyFd %d stateCode %d stateMsg %s", asyFd, stateCode, stateMsg.c_str());
}

static void exceptionRedisMsg(int asyFd, int exceCode, const std::string& exceMsg)
{
    PDEBUG("asyFd %d exceCode %d exceMsg %s", asyFd, exceCode, exceMsg.c_str());
}

int main(int argc, char* argv[])
{

    CLUSTER_REDIS_ASYNC::RedisAsync::instance().redisAsyncInit(2, 2, 3, 10,
        std::bind(stateChangeCallback, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3),
        std::bind(exceptionRedisMsg, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3));
    PDEBUG("RedisAsync init");

    singleFd = CLUSTER_REDIS_ASYNC::RedisAsync::instance().addSigleRedisInfo("127.0.0.1:6800");
    PDEBUG("RedisAsync addSigleRedisInfo singleFd %d", singleFd);
    if(singleFd < 0)
    {
        return -1;
    }

    clusterFd = CLUSTER_REDIS_ASYNC::RedisAsync::instance().addClusterInfo("192.169.6.234:6790,192.169.6.234:6791");
    PDEBUG("RedisAsync addSigleRedisInfo clusterFd %d", clusterFd);
    if(clusterFd < 0)
    {
        return -1;
    }

    while(1)
    {
        sleep(60);
    }
    return 0;
}

