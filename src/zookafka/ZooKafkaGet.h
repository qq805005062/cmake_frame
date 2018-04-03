#ifndef __ZOO_KFK_TOPIC_GET__
#define __ZOO_KFK_TOPIC_GET__

#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

#include "librdkafka/rdkafka.h"

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

namespace ZOOKEEPERKAFKA
{

class ZooKafkaGet
{
public:

	ZooKafkaGet();

	~ZooKafkaGet();
	
	int zookInit(const std::string& zookeepers,
			  const std::string& topic,
			  const std::string& groupId,
			  const std::vector<int>& partitions = { 0 },
	          int64_t startOffset = static_cast<int64_t>(RD_KAFKA_OFFSET_END),//static_cast<int64_t>(RD_KAFKA_OFFSET_STORED)
	          size_t messageMaxSize = 4 * 1024 * 1024);

	int kfkInit(const std::string& brokers,
			  const std::string& topic,
			  const std::string& groupId,
			  const std::vector<int>& partitions = { 0 },
	          int64_t startOffset = static_cast<int64_t>(RD_KAFKA_OFFSET_STORED),
	          size_t messageMaxSize = 4 * 1024 * 1024);

	int get(std::string& data, int64_t* offset,std::string* key = NULL);

	void kfkDestroy();

	void changeKafkaBrokers(const std::string& brokers);

private:

	zhandle_t* initialize_zookeeper(const char * zookeeper, const int debug);

	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;

	rd_kafka_t* kfkt;

	rd_kafka_topic_partition_list_t* topicpar;

	size_t kMessageMaxSize;
	std::vector<int> partitions_;

};

}

#endif