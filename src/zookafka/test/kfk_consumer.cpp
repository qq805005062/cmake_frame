
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

#if 0
#include "ZooKfkTopicsPop.h"

int main(int argc, char* argv[])
{
	ZOOKEEPERKAFKA::ZooKfkTopicsPop pop;
	pop.zookInit("192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181","zookeeper,zookeeper11,zookeeper22,zookeeper33");

	while(1)
	{
#if 1
		pop.kfkTopicConsumeStop("zookeeper33");
		sleep(10);
		pop.kfkTopicConsumeStop("zookeeper22");
		sleep(10);
		pop.kfkTopicConsumeStop("zookeeper11");
		sleep(10);
		pop.kfkTopicConsumeStart("zookeeper33");
		sleep(10);
		pop.kfkTopicConsumeStart("zookeeper22");
		sleep(10);
		pop.kfkTopicConsumeStart("zookeeper11");
#else
		std::string topic,kfkdata;
		int64_t off;
		pop.pop(topic,kfkdata,&off);
		PDEBUG("topic :: %s,kfkdata :: %s,offset:: %lu\n",topic.c_str(),kfkdata.c_str(),off);
#endif
	
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

#if 1

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


