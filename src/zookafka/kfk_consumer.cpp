
#include "stdio.h"

#include "ZooKfkTopicsPop.h"

int main(int argc, char* argv[])
{
	int num = 0;
	ZOOKEEPERKAFKA::ZooKfkTopicsPop pop;
	pop.zookInit("192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181","zookeeper,zookeeper11,zookeeper22,zookeeper33");

	while(1)
	{
		std::string topic,kfkdata;
		int64_t off;
		pop.pop(topic,kfkdata,&off);
		PDEBUG("topic :: %s,kfkdata :: %s,offset:: %lu\n",topic.c_str(),kfkdata.c_str(),off);
		num++;

		if(num == 4)
			pop.kfkTopicConsumeStop("zookeeper33");

		if(num == 6)
			pop.kfkTopicConsumeStop("zookeeper22");

		if(num == 8)
			pop.kfkTopicConsumeStart("zookeeper33");
	
	}
	return 0;
}

