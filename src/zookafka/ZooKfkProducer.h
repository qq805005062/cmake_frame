
#ifndef __ZOO_KFK_PRODUCER__
#define __ZOO_KFK_PRODUCER__

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cstring>

#include <getopt.h>

#include <vector>

#include <common/Atomic.h>
#include "librdkafka/rdkafkacpp.h"

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"
#include "jansson/jansson.h"

/*
 π”√£∫
class KfkSendError : public ZOOKEEPERKAFKA::MsgDeliveryError
{
	public:
	void onDeliveryError(const int err_code, const std::string& err_msg,
		const std::string& msg, const std::string* key, void *msgOpaque)
	{
		PERROR("ERROR code :: %d  info :: %s\n", err_code, err_msg.c_str());
	}
};

int main(int argc, char* argv[])
{
	std::string broker = "192.169.3.165:9092";
	std::string topic = "test2";

	KFK::KfkSendError callback;

	KFK::producer pro;
	pro.init(broker, topic, (ZOOKEEPERKAFKA::MsgDeliveryError*)callback);

	for (std::string line; run && std::getline(std::cin, line);)
	{
		if (line == "quit")
			break;
		std::string msg = line;
		std::string errmsg;
		pro.send(msg, errmsg);
		std::cout << errmsg.c_str() << std::endl;
	}
	getchar();
	return 0;
}
*/

namespace ZOOKEEPERKAFKA
{

typedef common::AtomicIntegerT<uint32_t> AtomicUInt32;

enum enum_err
{
	enum_err_OK = 0,			/**< Configuration property was succesfully set */
	enum_err_set_UNKNOWN = -1,  /**< Unknown configuration property */
	enum_err_set_INVALID = -2,  /**< Invalid configuration value */

	enum_err_broker_empty = -3,
	enum_err_topic_empty = -4,
	enum_err_init_err = -5,
	enum_err_create_producer = -6,
	enum_err_create_toppic = -7,
	enum_err_create_consumer = -8,
	enum_err_start_err = -9,
	enum_err_send_err = -10,
};

class MsgDeliveryError
{
public:
	virtual void onDeliveryError(const int err_code, const std::string& err_msg,
		const std::string& msg, const std::string* key, void *msgOpaque) = 0;
};

class MsgDelivery
{
public:
	virtual void onDelivery(const int err_code, const std::string& err_msg,
		const std::string& msg, const int64_t offset, const std::string* key, void *msgOpaque) = 0;
};

class MsgDeliveryCallback 
		: public RdKafka::DeliveryReportCb
		, public RdKafka::EventCb
{
public:
	MsgDeliveryCallback()
		:deliveCb_(NULL)
		,errCb_(NULL)
		{
		}

	void setDelivery(MsgDelivery *deliveCb)
	{
		deliveCb_ = deliveCb;
	}

	void setDeliveryError(MsgDeliveryError *errCb)
	{
		errCb_ = errCb;
	}
	
	void dr_cb(RdKafka::Message &msg)
	{
		if(deliveCb_)//every msg callback
		{
			deliveCb_->onDelivery(msg.err(), msg.errstr(), std::string(static_cast<const char *>(msg.payload()),static_cast<int>(msg.len()))
				, msg.offset(), msg.key(), msg.msg_opaque());
		}

		if(errCb_ && msg.err())// ERROR callback
		{
			errCb_->onDeliveryError(msg.err(), msg.errstr(), std::string(static_cast<const char *>(msg.payload()),static_cast<int>(msg.len()))
				, msg.key(), msg.msg_opaque());
		}
	}

	void event_cb(RdKafka::Event &event)
	{}

protected:
	MsgDelivery* deliveCb_; 
	MsgDeliveryError* errCb_;
};

class ZooKfkProducer
{
public:
	ZooKfkProducer()
		:kfkProducerVect()
		,kfkTopicVevt()
		,m_cb()
		,zKeepers()
		,zookeeph(nullptr)
		,kfkBrokers()
		,topic_()
		,producerSize(0)
		,lastIndex()
		,isBroChang(0)
		,pushNow(0)
	{	
	}

	~ZooKfkProducer()
	{
		for(int i = 0;i < producerSize;i++)
		{
			delete kfkProducerVect[i];
			kfkProducerVect[i] = NULL;
		}

		for(int i = 0;i < producerSize;i++)
		{
			delete kfkTopicVevt[i];
			kfkTopicVevt[i] = NULL;
		}

		std::vector<RdKafka::Producer*> ().swap(kfkProducerVect);
		std::vector<RdKafka::Topic*> ().swap(kfkTopicVevt);
	}

	int zookInit(const std::string& zookeepers);

	int zookInit(const std::string& zookeepers,
		const std::string& topics,
		int pNum,
		MsgDeliveryError* errCb = nullptr,
		MsgDelivery* cb = nullptr);
	
	int kfkInit(const std::string brokers,
		const std::string topic,
		int pNum,
		MsgDeliveryError* errCb = nullptr,
		MsgDelivery* cb = nullptr);

	int send(const std::string& msg,
		std::string& strerr,
		const std::string* key = nullptr,
		void *msgOpaque = nullptr,
		int32_t partition = -1,
		int send_timeout_times = 5,
		int send_wnd_size = 100);

	void changeKafkaBrokers(const std::string& brokers);
protected:
	
	std::vector<RdKafka::Producer*> kfkProducerVect;
	std::vector<RdKafka::Topic*> kfkTopicVevt;
	MsgDeliveryCallback m_cb;
private:
	zhandle_t* initialize_zookeeper(const char * zookeeper, const int debug);

	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	std::string topic_;
	int producerSize;
	AtomicUInt32 lastIndex;
	volatile int isBroChang;
	volatile int pushNow;
	
};

class HashPartitionerCb : public RdKafka::PartitionerCb 
{
public:
	int32_t partitioner_cb(const RdKafka::Topic *topic,
		const std::string *key,
		int32_t partition_cnt,
		void *msg_opaque)
	{
		return djb_hash(key->c_str(), key->size()) % partition_cnt;
	}
private:
	static inline unsigned int djb_hash(const char *str, size_t len)
	{
		unsigned int hash = 5381;
		for (size_t i = 0; i < len; i++)
			hash = ((hash << 5) + hash) + str[i];
		return hash;
	}
};

}
#endif
