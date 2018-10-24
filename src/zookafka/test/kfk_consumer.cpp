
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
#include <iostream>
#include <pthread.h>

#include "../ZooKfkTopicsPop.h"


#endif

ZOOKEEPERKAFKA::ZooKfkTopicsPop pop;
void* startConsumerThread(void* obj)
{
	while(1)
	{
		int64_t offsetNum = 0;
		int32_t parationNum = 0;
		std::string topicName, dataStr, keyStr;
		int ret = pop.pop(topicName, dataStr, &keyStr, &offsetNum, &parationNum);
		if(ret < 0)
		{
			printf("pop.pop return ret %d \n", ret);
		}else{
			if(!topicName.empty())
			{
				printf("pop.pop topicName parationNum offset %s %d %lu\n", topicName.c_str(), parationNum, offsetNum);
			}
		}
	}

	return NULL;
}

int main(int argc, char* argv[])
{
	pthread_t pthreadId_;
	//pop.zookInit("192.169.6.66:2181,192.169.6.67:2181,192.169.6.68:2181","pu_cmd_topic","248");
	pop.kfkInit("192.169.6.66:9092,192.169.6.67:9092,192.169.6.68:9092","pu_cmd_topic","adsadsada");
	if (pthread_create(&pthreadId_, NULL, &startConsumerThread, NULL))
	{
		printf("startConsumerThread init error \n");
		return 0;
	}
	while(1)
	{
		sleep(60);
	}
}

#if 0
#include <iostream>
#include <pthread.h>

#include "../ZooKfkTopicsPop.h"

ZOOKEEPERKAFKA::ZooKfkTopicsPop pop;
void* startConsumerThread(void* obj)
{
	while(1)
	{
		int64_t offsetNum = 0;
		int32_t parationNum = 0;
		std::string topicName, dataStr, keyStr;
		int ret = pop.pop(topicName, dataStr, &keyStr, &offsetNum, &parationNum);
		if(ret < 0)
		{
			printf("pop.pop return ret %d \n", ret);
		}else{
			if(!topicName.empty())
				printf("pop.pop topicName parationNum offset %s %d %lu\n", topicName.c_str(), parationNum, offsetNum);
		}
	}

	return NULL;
}

int main(int argc, char* argv[])
{
	pthread_t pthreadId_;
	int switchOne = 1, switchTwo = 1, switchThr = 1, switchFou = 1;
	pop.zookInit("192.169.6.60:2181,192.169.6.61:2181,192.169.6.62:2181","zookeeper_11,zookeeper_22,zookeeper_33,zookeeper_44","248");
	if (pthread_create(&pthreadId_, NULL, &startConsumerThread, NULL))
	{
		printf("startConsumerThread init error \n");
		return 0;
	}

	pop.kfkSubscription();
	std::cout << ">>";
	for (std::string line; std::getline(std::cin, line); line.clear())
	{
		if(!line.empty())
		{
			if(line.compare("1") == 0)
			{
				if(switchOne)
				{
					pop.kfkTopicConsumeStop("zookeeper_11");
					switchOne = 0;
				}else{
					pop.kfkTopicConsumeStart("zookeeper_11");
					switchOne = 1;
				}
				pop.kfkSubscription();
			}

			if(line.compare("2") == 0)
			{
				if(switchTwo)
				{
					pop.kfkTopicConsumeStop("zookeeper_22");
					switchTwo = 0;
				}else{
					pop.kfkTopicConsumeStart("zookeeper_22");
					switchTwo = 1;
				}
				pop.kfkSubscription();
			}

			if(line.compare("3") == 0)
			{
				if(switchThr)
				{
					pop.kfkTopicConsumeStop("zookeeper_33");
					switchThr = 0;
				}else{
					pop.kfkTopicConsumeStart("zookeeper_33");
					switchThr = 1;
				}
				pop.kfkSubscription();
			}

			if(line.compare("4") == 0)
			{
				if(switchFou)
				{
					pop.kfkTopicConsumeStop("zookeeper_44");
					switchFou = 0;
				}else{
					pop.kfkTopicConsumeStart("zookeeper_44");
					switchFou = 1;
				}
				pop.kfkSubscription();
			}
		}
		std::cout << std::endl << ">>";
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


