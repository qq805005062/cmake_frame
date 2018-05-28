
#include <pthread.h>
#include "../ZooKfkTopicsPop.h"
#include "../ZooKfkCommon.h"

static volatile uint64_t countNum = 0;

int consuerTest()
{
	return 0;
}

int producerTest()
{
	return 0;
};

int main(int argc, char* argv[])
{
	#if 0
	static volatile uint64_t last_recv_num = 0;
	static volatile int second = 0;
	while(1)
	{
		second++;
		if(last_recv_num == 0 && countNum > 0)
		{
			second = 1;
		}

		uint64_t NowCountNum = countNum;
		printf("recv all num :: %lu,recv speed :: %lu,average speed :: %ld\n",
			NowCountNum, (NowCountNum - last_recv_num), (NowCountNum / second) );
		last_recv_num = NowCountNum;
		sleep(1);
	}
	#endif

	ZOOKEEPERKAFKA::ZooKfkTopicsPop pop;
	pop.zookInit("192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181","zookeeper33,v-topic","xiaoxiao");

	while(1)
	{
		std::string topic,kfkdata;
		int64_t off;
		pop.pop(topic,kfkdata,nullptr,&off);
		PDEBUG("topic :: %s,kfkdata :: %s,offset:: %lu\n",topic.c_str(),kfkdata.c_str(),off);
	}
	return 0;
}

