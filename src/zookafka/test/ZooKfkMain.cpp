#include <stdio.h>
#include <pthread.h>

#include <string>
#include <memory>

#include "Timestamp.h"
#include "ThreadPool.h"
#include "ConfigFile.h"

#include "../ZooKfkCommon.h"
#include "../ZooKfkTopicsPop.h"
#include "../ZooKfkTopicsPush.h"

static volatile uint64_t countNum = 0;

std::unique_ptr<ZOOKEEPERKAFKA::ThreadPool> testThreadPool;
static char *pBuf = NULL;

static void produceKfkMsg(const std::string& top, const std::string& msg)
{
	std::string errMsg;
	int ret = ZOOKEEPERKAFKA::ZooKfkProducers::instance().psuhKfkMsg(top, msg, errMsg);
	if(ret < 0)
	{
		PERROR("produceKfkMsg error %d %s", ret, errMsg.c_str());
	}
}

static void consumerKfkMsg(int index)
{
	std::string topicName, msgData, errMsg;
	int ret = ZOOKEEPERKAFKA::ZooKfkConsumers::instance().consume(index, topicName, msgData, errMsg);
	if(ret < 0)
	{
		PERROR("produceKfkMsg error %d %s", ret, errMsg.c_str());
	}
}

int consumerTest(int msgNum, int threadNum)
{
	int index = 0;
	std::string broAdds = ConfigFile::instance().zookeepBrokers();
	std::string topicName = ConfigFile::instance().testTopicName();
	std::string consumerGroup = "testConsumer";
	ZOOKEEPERKAFKA::ZooKfkConsumers::instance().zooKfkConsumerInit(threadNum, broAdds, topicName, consumerGroup);

	PDEBUG("kafka producer test for brokers %s thread num %d msg num %d topic name %s", broAdds.c_str(), threadNum, msgNum, topicName.c_str());
	ZOOKEEPERKAFKA::Timestamp startSecond(ZOOKEEPERKAFKA::Timestamp::now());
	for(int i = 0; i < msgNum; i++)
	{
		index = i % threadNum;
		testThreadPool->run(std::bind(consumerKfkMsg, index));
	}
	ZOOKEEPERKAFKA::Timestamp endSecond(ZOOKEEPERKAFKA::Timestamp::now());
	
	ZOOKEEPERKAFKA::ZooKfkConsumers::instance().zooKfkConsumerDestroy();
	ZOOKEEPERKAFKA::Timestamp overSecond(ZOOKEEPERKAFKA::Timestamp::now());
	PDEBUG("kafka consumer test begin %lu end %lu over %lu", startSecond.microSecondsSinceEpoch(), endSecond.microSecondsSinceEpoch(), overSecond.microSecondsSinceEpoch());
	
	return 0;
}

int producerTest(const std::string& msg, int msgNum, int threadNum)
{
	std::string broAdds = ConfigFile::instance().zookeepBrokers();
	std::string topicName = ConfigFile::instance().testTopicName();
	
	ZOOKEEPERKAFKA::ZooKfkProducers::instance().zooKfkProducersInit(threadNum,broAdds,topicName);

	PDEBUG("kafka producer test for brokers %s thread num %d msg num %d topic name %s", broAdds.c_str(), threadNum, msgNum, topicName.c_str());
	ZOOKEEPERKAFKA::Timestamp startSecond(ZOOKEEPERKAFKA::Timestamp::now());
	for(int i = 0; i < msgNum; i++)
	{
		testThreadPool->run(std::bind(produceKfkMsg, topicName, msg));
	}
	ZOOKEEPERKAFKA::Timestamp endSecond(ZOOKEEPERKAFKA::Timestamp::now());

	ZOOKEEPERKAFKA::ZooKfkProducers::instance().zooKfkProducersDestroy();
	ZOOKEEPERKAFKA::Timestamp overSecond(ZOOKEEPERKAFKA::Timestamp::now());
	PDEBUG("kafka producer test begin %lu end %lu over %lu", startSecond.microSecondsSinceEpoch(), endSecond.microSecondsSinceEpoch(), overSecond.microSecondsSinceEpoch());
	
	return 0;
};

int main(int argc, char* argv[])
{
	int sw = ConfigFile::instance().producerSwitch();
	testThreadPool.reset(new ZOOKEEPERKAFKA::ThreadPool("testPool"));
	int msgNum = ConfigFile::instance().testMsgNum();
	if(sw)
	{
		PDEBUG("Now begin kafka produce test.........");
		int threadNum = ConfigFile::instance().testThreadNum();
		testThreadPool->start(threadNum);
		int msgLen = ConfigFile::instance().producerMessSize();
		pBuf = new char[msgLen];
		if(pBuf)
		{
			memset(pBuf,1,msgLen);
		}else{
			PERROR("kafka produce prepare test data new error");
			return 0;
		}
		std::string testMsg(pBuf,msgLen);
		producerTest(testMsg, msgNum, threadNum);

		if(pBuf)
			delete[] pBuf;
		return 0;
	}

	sw = ConfigFile::instance().consumerSwitch();
	if(sw)
	{
		PDEBUG("Now begin kafka consumer test.........");
		int threadNum = ConfigFile::instance().testThreadNum();
		testThreadPool->start(threadNum);
		consumerTest(msgNum,threadNum);
		return 0;
	}

	return 0;
}

