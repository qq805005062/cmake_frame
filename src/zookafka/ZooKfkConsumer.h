
#ifndef __ZOO_KFK_CONSUMER__
#define __ZOO_KFK_CONSUMER__

#include <errno.h> 
#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <functional>

#include <getopt.h>

#include "librdkafka/rdkafkacpp.h"

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"
#include "jansson/jansson.h"


namespace ZOOKEEPERKAFKA
{

typedef std::function<void(const char *msg, size_t msgLen, const char *key, size_t keyLen, int64_t offset)> MsgConsumerCallback;

typedef std::function<void(int errCode,const std::string& errMsg)> MsgErrorCallback;

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

class ZooKfkConsumer
{
public:
	ZooKfkConsumer()
		:m_consumer(NULL)
		,m_topic(NULL)
		,broChange(false)
		,run(false)
		,zKeepers()
		,zookeeph(nullptr)
		,kfkBrokers()
		,topic_()
		,partition_(-1)
		,offset_(-1)
		,MCb_(nullptr)
		,ECb_(nullptr)
	{
	}

	~ZooKfkConsumer()
	{
		if (m_consumer)
		{
			delete m_consumer;
			m_consumer = NULL;
		}

		if (m_topic)
		{
			delete m_topic;
			m_topic = NULL;
		}
	}

	int zookInit(const std::string& zookeepers);

	int zookInit(const std::string& zookeepers,
		const std::string& topic,
		MsgConsumerCallback ccb = nullptr,
		MsgErrorCallback ecb = nullptr,
		int32_t partition = 0,
		int64_t start_offset = -1);

	int kfkInit(const std::string& brokers,
		const std::string& topic,
		MsgConsumerCallback ccb = nullptr,
		MsgErrorCallback ecb = nullptr,
		int32_t partition = -1,
		int64_t start_offset = -1);

	//block never return until stop
	int start();

	int stop();

	void changeKafkaBrokers(const std::string& brokers);
protected:
	RdKafka::Consumer* m_consumer;
	RdKafka::Topic* m_topic;

private:
	zhandle_t* initialize_zookeeper(const char * zookeeper, const int debug);

	bool broChange;
	bool run;
	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	std::string topic_;
	int32_t partition_;
	int32_t offset_;

	MsgConsumerCallback MCb_;
	MsgErrorCallback ECb_;
};

}
#endif

