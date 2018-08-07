
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

#include <mutex>

#include "Singleton.h"
#include "noncopyable.h"

#pragma GCC diagnostic ignored "-Wold-style-cast"

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
 *必须统一启始位置(初始化构造函数可以修改，默认是队尾）
 *可以对某一个topic开始或者停止（初始化的topic已经默认开始了，不需要重复调用kfkTopicConsumeStart
 *统一分组，分组名可以传递空字符串，这样就不分组
 *所有方法返回大于等于0 表示成功，否则表示有错误
 *返回错误码参考 头文件ZooKfkCommon.h
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
	//不需要自己再次调用kfkInit
	int zookInit(const std::string& zookeepers, const std::string& topic, const std::string& groupName);
	//kfk初始化，如果前面调用了zookInit；brokers可以传入空，brokers可以从zookeeper中获取，
	//brokers也可以不传入空，如果前面没有调用zookInit。多个topic，逗号分隔
	//初始化的topic默认调用了kfkTopicConsumeStart
	//groupName如果为空字符串则不分组
	int kfkInit(const std::string& brokers, const std::string& topic, const std::string& groupName);
	//增加某一个topic读，每次只能传入一个topic，多个分多次调用
	int kfkTopicConsumeStart(const std::string& topic);	
	//获取kfk一个消息，并可以获取对应的topic，偏移量，key；
	//-1表示有错，内部过滤了-191（读到队尾的错误)
	int pop(std::string& topic, std::string& data, std::string* key = NULL, int64_t* offset = NULL, int32_t* parnum = NULL);
	//获取kfk一个消息，timeout_ms超时时间，
	//内部已经过滤-191的错误，不需要外部过滤-191的错误
	int tryPop(std::string& topic, std::string& data, int timeout_ms, std::string* key = NULL, int64_t* offset = NULL, int32_t* parnum = NULL);
	
	//停止某一个topic读，必须存在已经读的topic
	int kfkTopicConsumeStop(const std::string& topic);
	//销毁资源信息、异步退出，清除资源使用
	void kfkDestroy();

	//获取最后一次错误信息，当有错误的时候应该调用这个方法，打印错误日志
	int getLastErrorMsg(std::string& msg)
	{
		msg.assign(kfkErrorMsg);
		return static_cast<int>(kfkErrorCode);
	}
	
	//zookeeper发现brokers变化修正brokers
	void changeKafkaBrokers(const std::string& brokers);
private:
	zhandle_t* initialize_zookeeper(const char* zookeeper, const int debug);

	bool str2Vec(const char* src, std::vector<std::string>& dest, const char delim);

	void setKfkErrorMessage(rd_kafka_resp_err_t code,const char *msg);

	std::mutex listLock;
	std::mutex flushLock;
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
	int destroy;
	volatile int popNum;
	int initFlag;
	int switchFlag;
	int errorFlag;
};

typedef std::shared_ptr<ZOOKEEPERKAFKA::ZooKfkTopicsPop> ZooKfkConsumerPtr;

/*
 *多个消费者单实例模式，多个消费者所有的配置参数都是一样的
 *可以直接包含头文件单实例使用，可以直接指定初始化多个消费者，提高并发量
 *返回错误码参考 头文件ZooKfkCommon.h
 */
class ZooKfkConsumers : public noncopyable
{
public:
	ZooKfkConsumers()
		:lastIndex(0)
		,kfkConsumerNum(0)
		,ZooKfkConsumerPtrVec()
	{
	}

	~ZooKfkConsumers()
	{
		for(int i = 0;i < kfkConsumerNum; i++)
		{
			if(ZooKfkConsumerPtrVec[i])
				ZooKfkConsumerPtrVec[i].reset();
		}

		std::vector<ZooKfkConsumerPtr> ().swap(ZooKfkConsumerPtrVec);
	}

	//单实例接口哦
	static ZooKfkConsumers& instance() { return ZOOKEEPERKAFKA::Singleton<ZooKfkConsumers>::instance(); }

	//初始化接口，消费者个数，zookeeper地址，订阅topic名称，组名称
	int zooKfkConsumerInit(int consumerNum, const std::string& zookStr, const std::string& topicStr,  const std::string& groupName);

	//消费者退出接口，保证消息不丢失，进程退出调用接口
	void zooKfkConsumerDestroy();

	//开启一个topic的读，单次只能配置一个
	int zooKfkConsumerStart(const std::string& topic);

	//关闭一个topic的读，单词只能配置一个
	int zooKfkConsumerStop(const std::string& topic);

	//消费一条消息，如果有错误，会返回错误消息，自动过滤了队尾的错误，下标表示用的下标，不能大于等于初始化的个数
	int consume(int index, std::string& topic, std::string& data, std::string& errorMsg, std::string* key = NULL, int64_t* offset = NULL);

	//超时消费一条消息，如果超时，无消息也会返回，返回1就是有消息，0是超时，错误小于0，下标不能呢个大于等于初始化个数，内部过滤到队尾的错误，不可以当定时器用
	int tryConsume(int index, std::string& topic, std::string& data, int timeout_ms, std::string& errorMsg, std::string* key = NULL, int64_t* offset = NULL);
	
private:
	//volatile unsigned int lastIndex;
	unsigned int lastIndex;
	int kfkConsumerNum;
	std::vector<ZooKfkConsumerPtr> ZooKfkConsumerPtrVec;
};

}

#endif

