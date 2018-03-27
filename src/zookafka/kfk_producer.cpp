
#include <iostream>
#include <string>

#include <unistd.h>
#include <stdio.h>

//#define ZOOKEEPER_KAFKA_TOPIC	1
#define ZOOKEEPER_KAFKA_TOPICS	1

#ifdef ZOOKEEPER_KAFKA_TOPIC

#include "ZooKafkaPut.h"

int main(int argc, char* argv[])
{
	ZOOKEEPERKAFKA::ZooKafkaPut prducer;
	prducer.zookInit("192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181","zookeeper");

	for (std::string line; std::getline(std::cin, line); line.clear())
	{
		prducer.push(line);
	}
	return 0;
}

#endif

#ifdef ZOOKEEPER_KAFKA_TOPICS

#include "ZooKfkTopicsPush.h"

void MsgPushErrorCallback(PUSHERRORMSG *msgInfo)
{
	PERROR("MsgPushErrorCallback :: len :: %d,msg data :: %s\n",msgInfo->msgLen,msgInfo->msg);
	return;
}

int main(int argc, char* argv[])
{
	ZOOKEEPERKAFKA::ZooKfkTopicsPush push;

	push.setMsgPushErrorCall(std::bind(MsgPushErrorCallback,std::placeholders::_1));
	push.zookInit("192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181","zookeeper,zookeeper11,zookeeper22,zookeeper33");
	while(1)
	{
		push.push("zookeeper","zookeeper");
		sleep(1);
		push.push("zookeeper11","zookeeper11");
		sleep(1);
		push.push("zookeeper22","zookeeper22");
		sleep(1);
		push.push("zookeeper33","zookeeper33");
		sleep(1);
	}
	return 0;
}

#endif

