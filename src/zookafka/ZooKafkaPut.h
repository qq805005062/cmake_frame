#ifndef __ZOO_KFK_TOPIC_PUT__
#define __ZOO_KFK_TOPIC_PUT__

#include <stdio.h>
#include <string.h>

#include <functional>
#include <string>

#include <mutex>

#include "librdkafka/rdkafka.h"

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"
#include "jansson/jansson.h"

namespace ZOOKEEPERKAFKA
{

//typedef void (*MsgCallBack)(rd_kafka_t *rk, const rd_kafka_message_t* rkmessage, void *opaque);

//typedef void (*MsgPushErrorCallBack)(int errorCode,const char* errorMsg,void* opaque);

class ZooKafkaPut
{

public:
	typedef std::function<void(const char *msg,int msgLen, int errorCode, const char* errorMsg)> MsgPushErrorCallBack;
	
	ZooKafkaPut();

	~ZooKafkaPut();
	
	int zookInit(const std::string& zookeepers,
			  const std::string& topicName,
			  int partition = RD_KAFKA_PARTITION_UA,
			  int maxMsgqueue = 2 * 1024 * 1024);

	void msgPushErrorCall(const char *msg,int msgLen, int errorCode, const char* errorMsg)
	{
		if(cb_)
			cb_(msg,msgLen,errorCode,errorMsg);
	}

	int kfkInit(const std::string& brokers,
			  const std::string& topicName,
			  int partition = RD_KAFKA_PARTITION_UA,
			  int maxMsgqueue = 2 * 1024 * 1024);

	void setMsgPushErrorCall(const MsgPushErrorCallBack& cb)
	{
		cb_ = cb;
	}

	int push(const std::string& data,
	         std::string* key = NULL,
	         int partition = RD_KAFKA_PARTITION_UA,
	         int msgFlags = RD_KAFKA_MSG_F_COPY);
	
	void kfkDestroy();

	void changeKafkaBrokers(const std::string& brokers);

private:

	zhandle_t* initialize_zookeeper(const char * zookeeper, const int debug);

	std::mutex kfkLock;
	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	rd_kafka_t* kfkt;
	
	rd_kafka_topic_t* kfktopic;
	MsgPushErrorCallBack cb_;
};

}

#endif