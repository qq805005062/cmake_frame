
#include "stdio.h"
#include <unistd.h>

//#define ZOOKEEPER_KAFKA_TOPIC		1
//#define ZOOKEEPER_KAFKA_TOPICS_POP	1
#define ZOOKEEPER_KAFKA_TOPICS_GET	1

#ifdef ZOOKEEPER_KAFKA_TOPIC
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

#ifdef ZOOKEEPER_KAFKA_TOPICS
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

#ifdef ZOOKEEPER_KAFKA_TOPICS_GET

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



