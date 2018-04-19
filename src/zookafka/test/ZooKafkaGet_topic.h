#include <errno.h> 
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <common/MutexLock.h>

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


typedef std::shared_ptr<struct rd_kafka_topic_s> KfkTopicPtr;
typedef std::map<std::string,rd_kafka_topic_t *> KfkTopicPtrMap;
typedef KfkTopicPtrMap::iterator KfkTopicPtrMapIter;

namespace ZOOKEEPERKAFKA
{

class ZooKafkaGet
{
public:

	ZooKafkaGet();

	~ZooKafkaGet();

	int zookInit(const std::string& zookeepers);
	
	int zookInit(const std::string& zookeepers, const std::string& topic);

	int kfkInit(const std::string& brokers, const std::string& topic);


	//这个方法必须预先初始化好，因为涉及map迭代器的使用，所以在初始化之后再次调用，可能会崩溃
	int kfkAddTopic(const std::string& topic);

	int kfkTopicConsumeStart(const std::string& topic);	
	
	int get(std::string& topic, std::string& data, int64_t* offset = NULL, std::string* key = NULL);

	int getFromTopicPar(const std::string& topic, std::string& data, int64_t* offset = NULL,std::string* key = NULL);

	int kfkTopicConsumeStop(const std::string& topic);
	
	void kfkDestroy();

	void changeKafkaBrokers(const std::string& brokers);

private:

	zhandle_t* initialize_zookeeper(const char * zookeeper, const int debug);

	bool str2Vec(const char* src, std::vector<std::string>& dest, const char* delim);
		
	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	std::string groupId;

	rd_kafka_t* kfkt;
	KfkTopicPtrMap topicMap;
	rd_kafka_conf_t* kfkconft;
	
	rd_kafka_topic_conf_t* kfktopiconft;
	rd_kafka_topic_partition_list_t* topicparlist;

	size_t kMessageMaxSize;

	int64_t startOffset;
	int32_t partition;
	volatile int pause_;
};

}