
#ifndef __ZOO_KFK_CONSUMER__
#define __ZOO_KFK_CONSUMER__
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cstring>

#include <memory>
#include <thread>

#include "librdkafka/rdkafkacpp.h"

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"
#include "jansson/jansson.h"

#define SHOW_DEBUG		1
#define SHOW_ERROR		1
#ifdef SHOW_DEBUG
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#ifdef SHOW_ERROR
#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PERROR(fmt, args...)
#endif

/*
使用：

class consumer_proc : public KFK::consumer_interface
{
	public:
	void on_consumer(const int err_code, const std::string& err_msg,
		const std::string& msg, const int64_t offset)
	{
		if (-191 != err_code)
			std::cout << offset << "=" << msg << std::endl;
	}
};


int main (int argc, char **argv)
{
	std::string broker = "192.169.3.165:9092";
	std::string topic = "test2";
	KFK::consumer consumer;
	consumer.init(broker, topic);

	consumer_proc callback;
	consumer.start(0, 70, (KFK::consumer_interface*)&callback);

	getchar();
	return 0;
}

*/

namespace ZOOKEEPERKAFKA
{

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

class consumer_interface
{
public:
	virtual void on_consumer(const int err_code, const std::string& err_msg,
		const std::string& msg, const int64_t offset) = 0;
};

class consumer_callback 
		: public RdKafka::ConsumeCb
		, public RdKafka::EventCb
{
public:
	consumer_callback() : m_callback(NULL) {}
	void set_callback(consumer_interface* cb)
	{
		m_callback = cb;
	}
	void consume_cb(RdKafka::Message &msg, void *opaque)
	{
		if (m_callback)
			m_callback->on_consumer(msg.err(), msg.errstr(),
				std::string(static_cast<const char *>(msg.payload()),
					static_cast<int>(msg.len())), msg.offset());
		// msg_consume_proc(&msg, opaque);
	}

	void event_cb(RdKafka::Event &event)
	{}

protected:
	consumer_interface* m_callback;
};

class ZooKfkConsumer
{
public:
	ZooKfkConsumer()
		:m_conf(NULL)
		,m_rk(NULL)
		,m_topic_conf(NULL)
		,m_rkqu(NULL)
		,m_topics(NULL)
		,m_bRun(false)
		,m_thread_spr(nullptr)
		,m_cb_consumer()
	{
	}

	~ZooKfkConsumer()
	{
	}

	int init(const std::string brokers, const std::string topic);

	int stop();

	/*
	功能： 接收通道消息
	partition: 文件
	start_offset: 偏移
	返回： 
	*/
	int start(std::string group, int64_t start_offset, consumer_interface* ex_consume_cb);

	/*
	功能： 设置producer或者topic的属性
	*/
	int set(const std::string attr, const std::string sValue, std::string& errstr);


protected:
	rd_kafka_conf_t* m_conf;
	rd_kafka_t* m_rk;
	rd_kafka_topic_conf_t* m_topic_conf;
	rd_kafka_queue_t* m_rkqu;
	rd_kafka_topic_partition_list_t* m_topics;

	std::string m_brokers;
	std::string m_group;

	bool m_bRun;
	std::shared_ptr<std::thread> m_thread_spr;
	consumer_callback m_cb_consumer;

};

}
#endif

