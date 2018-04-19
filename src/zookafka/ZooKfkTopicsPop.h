
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


typedef std::list<std::string> ListStringTopic;
typedef ListStringTopic::iterator ListStringTopicIter;

namespace ZOOKEEPERKAFKA
{
/*
 *kfk消费多个topic的情况，不可以选择分区，由kfk均衡读取
 *必须统一启始位置
 *可以对某一个topic开始或者停止
 *统一分组
 */
class ZooKfkTopicsPop
{
public:
	//构造函数
	ZooKfkTopicsPop();
	//析构函数
	~ZooKfkTopicsPop();

	//仅仅传入zookeeper的地址信息，逗号分隔多个ip port；其后自己调用kfkInit
	int zookInit(const std::string& zookeepers);
	//传入zookeeper地址信息，逗号分隔多个ip port，传入多个topic，逗号分隔，不可以有多余的符号，内部调用kfkInit
	int zookInit(const std::string& zookeepers, const std::string& topic, const std::string groupName);
	//kfk初始化，brokers可以传入，也可以传空，则使用zookeeper获取的，逗号分隔多个ip port。多个topic，逗号分隔
	int kfkInit(const std::string& brokers, const std::string& topic, const std::string groupName);
	//启动某一个topic读，必须存在初始化的topic中，否则报错
	int kfkTopicConsumeStart(const std::string& topic);	
	//获取kfk一个消息，并可以获取对应的topic，偏移量，key；
	int pop(std::string& topic, std::string& data, int64_t* offset = NULL, std::string* key = NULL);
	//获取kfk一个消息，timeout_ms超时时间，返回-1表示有错误，返回0表示正常，返回1表示超时
	int tryPop(std::string& topic, std::string& data, int timeout_ms, int64_t* offset = NULL, std::string* key = NULL);
	
	//停止某一个topic读，必须存在初始化的topic
	int kfkTopicConsumeStop(const std::string& topic);
	//销毁资源信息
	void kfkDestroy();

	int getLastErrorMsg(std::string& msg)
	{
		msg.assign(kfkErrorMsg);
		return static_cast<int>(kfkErrorCode);
	}
	
	//zookeeper发现brokers变化修正brokers
	void changeKafkaBrokers(const std::string& brokers);
private:
	zhandle_t* initialize_zookeeper(const char * zookeeper, const int debug);

	bool str2Vec(const char* src, std::vector<std::string>& dest, const char delim);

	void setKfkErrorMessage(rd_kafka_resp_err_t code,const char *msg);

	common::MutexLock listLock;
	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	ListStringTopic topics_;
	
	rd_kafka_t* kfkt;
	
	rd_kafka_topic_partition_list_t* topicparlist;

	size_t kMessageMaxSize;
	int64_t startOffset;
	int32_t partition;

	rd_kafka_resp_err_t kfkErrorCode;
	std::string kfkErrorMsg;
};

}

#endif

