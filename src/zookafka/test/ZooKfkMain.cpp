#include <stdio.h>
#include <pthread.h>
#include <mutex>

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
static volatile int targetMsgNum = 0, kafkaProducerCallNum = 0, kafkaConsumerCallNum = 0;
static pthread_mutex_t syncLock,countLock;
static void everyKfkDeliverCall(void *msgPri, CALLBACKMSG *msgInfo)
{
	if(msgInfo)
	{
		if(msgInfo->errorCode && msgInfo->errMsg)
		{
			PERROR("everyKfkDeliverCall error %d error msg %s", msgInfo->errorCode, msgInfo->errMsg);
		}else{
			;//PDEBUG("everyKfkDeliverCall success %d ", kafkaProducerCallNum);
		}
	}
	pthread_mutex_lock(&countLock);
	kafkaProducerCallNum++;
	if(kafkaProducerCallNum == targetMsgNum)
	{
		PDEBUG("pthread_mutex_unlock %d ", kafkaProducerCallNum);
		pthread_mutex_unlock(&syncLock);
	}
	pthread_mutex_unlock(&countLock);
}

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
	while(1)
	{
		std::string topicName, msgData, errMsg;
		int ret = ZOOKEEPERKAFKA::ZooKfkConsumers::instance().consume(index, topicName, msgData, errMsg);
		if(ret < 0)
		{
			PERROR("ZooKfkConsumers error %d %s", ret, errMsg.c_str());
		}else{
			;//PDEBUG("pop from topic %s dataLen %ld ret %d index %d", topicName.c_str(), msgData.length(), ret, index);
			//PDEBUG("pop from topic %s dataLen %ld data %s ret %d index %d", topicName.c_str(), msgData.length(), msgData.c_str(), ret, index);
		}
		pthread_mutex_lock(&countLock);
		kafkaConsumerCallNum++;
		if(kafkaConsumerCallNum == targetMsgNum)
		{
			pthread_mutex_unlock(&syncLock);
			break;
		}else if(kafkaConsumerCallNum > targetMsgNum)
		{
			break;
		}
		pthread_mutex_unlock(&countLock);
	}
}

int consumerTest(int msgNum, int threadNum)
{
	std::string broAdds = ConfigFile::instance().zookeepBrokers();
	std::string topicName = ConfigFile::instance().testTopicName();
	std::string consumerGroup = "testConsumer";
	ZOOKEEPERKAFKA::ZooKfkConsumers::instance().zooKfkConsumerInit(threadNum, broAdds, topicName, consumerGroup);

	targetMsgNum = msgNum;
	PDEBUG("kafka producer test for brokers %s thread num %d msg num %d topic name %s", broAdds.c_str(), threadNum, msgNum, topicName.c_str());
	ZOOKEEPERKAFKA::Timestamp startSecond(ZOOKEEPERKAFKA::Timestamp::now());
	pthread_mutex_lock(&syncLock);
	for(int i = 0; i < threadNum; i++)
	{
		testThreadPool->run(std::bind(consumerKfkMsg, i));
	}
	ZOOKEEPERKAFKA::Timestamp endSecond(ZOOKEEPERKAFKA::Timestamp::now());
	pthread_mutex_lock(&syncLock);
	ZOOKEEPERKAFKA::Timestamp overSecond(ZOOKEEPERKAFKA::Timestamp::now());
	ZOOKEEPERKAFKA::ZooKfkConsumers::instance().zooKfkConsumerDestroy();
	PDEBUG("kafka consumer test begin %lu end %lu over %lu", startSecond.microSecondsSinceEpoch(), endSecond.microSecondsSinceEpoch(), overSecond.microSecondsSinceEpoch());
	
	return 0;
}

int producerTest(const std::string& msg, int msgNum, int threadNum)
{
	int ret = 0;
	std::string broAdds = ConfigFile::instance().zookeepBrokers();
	std::string topicName = ConfigFile::instance().testTopicName();
	
	ret = ZOOKEEPERKAFKA::ZooKfkProducers::instance().zooKfkProducersInit(threadNum,broAdds,topicName);
	if(ret < 0)
	{
		PERROR("ZOOKEEPERKAFKA::ZooKfkProducers::instance().zooKfkProducersInit error");
		return ret;
	}
	ZOOKEEPERKAFKA::ZooKfkProducers::instance().setMsgPushCallBack(std::bind(everyKfkDeliverCall, std::placeholders::_1, std::placeholders::_2));

	targetMsgNum = msgNum;
	PDEBUG("kafka producer test for brokers %s thread num %d msg num %d topic name %s", broAdds.c_str(), threadNum, msgNum, topicName.c_str());
	ZOOKEEPERKAFKA::Timestamp startSecond(ZOOKEEPERKAFKA::Timestamp::now());
	pthread_mutex_lock(&syncLock);
	for(int i = 0; i < msgNum; i++)
	{
		testThreadPool->run(std::bind(produceKfkMsg, topicName, msg));
	}
	do{
		ret = pthread_mutex_trylock(&syncLock);
		//PDEBUG("pthread_mutex_trylock :%d",ret);
		if(ret == 0)
			break;
		for(int i = 0; i < threadNum; i++)
		{
			ZOOKEEPERKAFKA::ZooKfkProducers::instance().produceFlush(i);
		}
	}while(1);
	ZOOKEEPERKAFKA::Timestamp endSecond(ZOOKEEPERKAFKA::Timestamp::now());
	PDEBUG("kafka producer test begin %lu end %lu", startSecond.microSecondsSinceEpoch(), endSecond.microSecondsSinceEpoch());
	ZOOKEEPERKAFKA::ZooKfkProducers::instance().zooKfkProducersDestroy();
	ZOOKEEPERKAFKA::Timestamp overSecond(ZOOKEEPERKAFKA::Timestamp::now());
	PDEBUG("kafkaProducerCallNum :%d",kafkaProducerCallNum);
	PDEBUG("kafka producer test begin %lu end %lu over %lu", startSecond.microSecondsSinceEpoch(), endSecond.microSecondsSinceEpoch(), overSecond.microSecondsSinceEpoch());
	
	return 0;
};

int main(int argc, char* argv[])
{
	int sw = ConfigFile::instance().producerSwitch();
	testThreadPool.reset(new ZOOKEEPERKAFKA::ThreadPool("testPool"));
	int msgNum = ConfigFile::instance().testMsgNum();

	pthread_mutex_init(&syncLock, NULL);
	pthread_mutex_init(&countLock, NULL);
	if(sw)
	{
		PDEBUG("Now begin kafka produce test.........");
		int threadNum = ConfigFile::instance().testThreadNum();
		testThreadPool->start(threadNum);
		int msgLen = ConfigFile::instance().producerMessSize();
		pBuf = new char[msgLen];
		if(pBuf)
		{
			memset(pBuf,'a',msgLen);
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
	pthread_mutex_destroy(&syncLock);
	return 0;
}

