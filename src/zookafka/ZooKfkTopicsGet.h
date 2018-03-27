
#ifndef __ZOO_KFK_TOPICS_POP__
#define __ZOO_KFK_TOPICS_POP__

#include <errno.h> 
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>
#include <list>
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

typedef std::map<std::string,rd_kafka_topic_t*> KfkTopicPtrMap;
typedef KfkTopicPtrMap::iterator KfkTopicPtrMapIter;

namespace ZOOKEEPERKAFKA
{
/*
 *kfk消费多个topic的情况，指定分区、指定起始位置
 *可以定义分区，定义起始位置，获取指定topic，指定分区，必须先初始化。阻塞式的获取
 *可以对某一个topic、分区开始、停止
 *统一分组
 */
class ZooKfkTopicsGet
{
public:
	//构造函数
	ZooKfkTopicsGet();
	//析构函数
	~ZooKfkTopicsGet();

	//仅仅传入zookeeper的地址信息，逗号分隔多个ip port，内部直接调用kfkInit
	int zookInit(const std::string& zookeepers);
	//kfk初始化，非用zookeeper初始化
	int kfkInit(const std::string& brokers);
	//启动某一个topic读，指定分区，指定偏移量、多个topic，多次调用
	int kfkTopicConsumeStart(const std::string& topic, int partition = 0, int64_t offset = RD_KAFKA_OFFSET_END);	
	//获取kfk消息，获取指定topic的消息，返回数据，偏移量和key，阻塞式，一直到有数据为止
	int get(std::string& topic, std::string& data, int64_t* offset = NULL, int partition = 0, std::string* key = NULL);
	//停止某一个topic读，指定分区，每次只能传入一个topic，多个topic，多次调用
	int kfkTopicConsumeStop(const std::string& topic, int partition = RD_KAFKA_PARTITION_UA);
	//销毁资源信息
	void kfkDestroy();
	//zookeeper发现brokers变化修正brokers
	void changeKafkaBrokers(const std::string& brokers);
private:
	zhandle_t* initialize_zookeeper(const char * zookeeper, const int debug);

	bool str2Vec(const char* src, std::vector<std::string>& dest, const char* delim);

	common::MutexLock listLock;
	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	
	rd_kafka_t* kfkt;
	KfkTopicPtrMap topicPtrMap;
	
	size_t kMessageMaxSize;
};

}

#endif

