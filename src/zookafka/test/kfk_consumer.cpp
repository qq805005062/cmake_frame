
#include "stdio.h"
#include <unistd.h>

#if 0
#include "ZooKafkaGet.h"

int main(int argc, char* argv[])
{
	ZOOKEEPERKAFKA::ZooKafkaGet get;
	get.zookInit("192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181","zookeeper","zookeeper");

	while(1)
	{
		std::string kfkdata;
		int64_t off;
		get.get(kfkdata,&off);
		PDEBUG("kfkdata :: %s,offset:: %lu\n",kfkdata.c_str(),off);
	}
}

#endif

#if 1
#include <pthread.h>

#include "../ZooKfkTopicsPop.h"

ZOOKEEPERKAFKA::ZooKfkTopicsPop pop;
static int flag = 0,popNum = 0;
void* startConsumerThread(void* obj)
{
	while(1)
	{
		int64_t offsetNum = 0;
		int32_t parationNum = 0;
		std::string topicName, dataStr, keyStr;
		while(flag)
		{
			sleep(1);
		}
		int ret = pop.pop(topicName, dataStr, &keyStr, &offsetNum, &parationNum);
		popNum++;
		if(popNum && popNum % 10 == 0)
		{
			flag = 1;
		}
		if(ret < 0)
		{
			printf("pop.pop return ret %d \n", ret);
		}else{
			printf("pop.pop topicName parationNum parationNum %s %d %lu\n", topicName.c_str(), parationNum, offsetNum);
		}
	}

	return NULL;
}

int main(int argc, char* argv[])
{
	pthread_t pthreadId_;
	int ret = 0;
	pop.zookInit("192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181","zookeeper11,zookeeper22,zookeeper33","248");
	if (pthread_create(&pthreadId_, NULL, &startConsumerThread, NULL))
	{
		printf("startConsumerThread init error \n");
		return 0;
	}
	
	while(1)
	{
		sleep(1);
		if(popNum && popNum % 10 == 0)
		{
			ret++;
			ret = ret % 6;
			switch(ret)
			{
				case 0:
					printf("kfkTopicConsumeStart zookeeper11\n");
					pop.kfkTopicConsumeStart("zookeeper11");
					break;
				case 1:
					printf("kfkTopicConsumeStop zookeeper11\n");
					pop.kfkTopicConsumeStop("zookeeper11");
					break;
				case 2:
					printf("kfkTopicConsumeStop zookeeper22\n");
					pop.kfkTopicConsumeStop("zookeeper22");
					break;
				case 3:
					printf("kfkTopicConsumeStart zookeeper22\n");
					pop.kfkTopicConsumeStart("zookeeper22");
					break;
				case 4:
					printf("kfkTopicConsumeStop zookeeper33\n");
					pop.kfkTopicConsumeStop("zookeeper33");
					break;
				case 5:
					printf("kfkTopicConsumeStart zookeeper33\n");
					pop.kfkTopicConsumeStart("zookeeper33");
					break;
				default:
					printf("default break\n");
					break;
			}
			popNum++;
			flag = 0;
		}
	}
	return 0;
}
#endif

#if 0

#include "ZooKfkTopicsGet.h"

int main(int argc, char* argv[])
{
	ZOOKEEPERKAFKA::ZooKfkTopicsGet get;
	get.zookInit("192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181");
	get.kfkTopicConsumeStart("zookeeper",0);
	get.kfkTopicConsumeStart("zookeeper",1);
	get.kfkTopicConsumeStart("zookeeper",2);
	get.kfkTopicConsumeStart("zookeeper",3);
	//get.kfkTopicConsumeStart("zookeeper11");
	//get.kfkTopicConsumeStart("zookeeper22");
	//get.kfkTopicConsumeStart("zookeeper33");
	while(1)
	{
		std::string topic,kfkdata;
		int64_t off;
		topic = "zookeeper";
		get.get(topic,kfkdata,&off,0);
		PDEBUG("zookeeper ,kfkdata :: %s,offset:: %lu\n", kfkdata.c_str(), off);

		get.get(topic,kfkdata,&off,1);
		PDEBUG("zookeeper ,kfkdata :: %s,offset:: %lu\n", kfkdata.c_str(), off);

		get.get(topic,kfkdata,&off,2);
		PDEBUG("zookeeper ,kfkdata :: %s,offset:: %lu\n", kfkdata.c_str(), off);

/*
		kfkdata.clear();
		topic = "zookeeper11";
		get.get(topic,kfkdata,&off);
		PDEBUG("zookeeper11 ,kfkdata :: %s,offset:: %lu\n", kfkdata.c_str(), off);

		kfkdata.clear();
		topic = "zookeeper22";
		get.get(topic,kfkdata,&off);
		PDEBUG("zookeeper22 ,kfkdata :: %s,offset:: %lu\n", kfkdata.c_str(), off);

		kfkdata.clear();
		topic = "zookeeper33";
		get.get(topic,kfkdata,&off);
		PDEBUG("zookeeper33 ,kfkdata :: %s,offset:: %lu\n", kfkdata.c_str(), off);*/
	}
	return 0;
}
#endif

#if 0

#include "ZooKfkConsumer.h"

void msgConsumerInfo(const char *msg, size_t msgLen, const char *key, size_t keyLen, int64_t offset)
{
	PDEBUG("msg :: %s len :: %ld offset :: %ld\n",msg,msgLen,offset);
	return;
}

void msgConsumerError(int errCode,const std::string& errMsg)
{
	PDEBUG("errCode :: %d errMsg :: %s\n",errCode,errMsg.c_str());
	return;
}

int main(int argc, char* argv[])
{
	ZOOKEEPERKAFKA::ZooKfkConsumer consumer;

	int ret = consumer.zookInit("192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181"
		,"zookeeper"
		,std::bind(msgConsumerInfo,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4,std::placeholders::_5)
		,std::bind(msgConsumerError,std::placeholders::_1,std::placeholders::_2));

	if(ret < 0)
		return ret;
	ret = consumer.start();
	return ret;
	
}

#endif


