
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>

#include <common/Timestamp.h>

#include "RedisAsync.h"

static ASYNCREDIS::RedisAsync hha;
static int respond = 0;
static int64_t beginS = 0,endS = 0;

#define TESTNUM		400

void hMsetCallBack(int64_t ret, void *privdata, const std::string& err)
{
	respond++;
	if(!err.empty())
	{
		PERROR("hMsetCallBack :: error :: %s\n",err.c_str());
		return;
	}
	if(respond == TESTNUM)
	{
		endS = common::Timestamp::now().microSecondsSinceEpoch();
		printf("Time spend %ld   %ld\n",beginS,endS);
	}
	PDEBUG("hMsetCallBack :: ret :: %ld :: %d\n",ret,respond);
}

void* RedisInitThread(void* obj)
{
	hha.RedisConnect("127.0.0.1", 6379,5);
	hha.RedisLoop();
	return NULL;
}

int main (int argc, char **argv)
{
	pthread_t pthreadId_;
	uint64_t msgId = 4161478067814942313;
	char redis_key[32] = {0};
	//char redis_key[32] = "msg:4161478067814942313";

	if (pthread_create(&pthreadId_, NULL, &RedisInitThread, NULL))
	{
		printf("RedisInitThread init error \n");
		return -1;
	}
	
	ASYNCREDIS::HashMap hmsetMap;
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("spno","10655999666456151"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("dnVolume","0"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("tmplparams","15800000000"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("title","15800000000"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("smsgwno","4427"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("userid","iris03"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("ecid","100317"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("exdata","15800000000"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("tmplID","0"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("dldTimes","0"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("dldState","0"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("flag","1"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("upVolume","0"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("smsptcode","PTGW2"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("smsgwno","4427"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("Phone","15800000000"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("genTime","1521447030088"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("volume","0"));
	hmsetMap.insert(ASYNCREDIS::HashMap::value_type("validtm","24"));

	beginS = common::Timestamp::now().microSecondsSinceEpoch();
	for(int i = 0;i < TESTNUM;i++)
	{
		msgId++;
		memset(redis_key,0,32);
		sprintf(redis_key,"msg:%lu",msgId);
		hha.hmset(redis_key,hmsetMap,std::bind(hMsetCallBack,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3),NULL);
		PDEBUG("hmset :::: %d\n",i);
	}

	while(1)
		sleep(60);
	
    return 0;
}

