
#ifndef __ZOO_KFK_TOPICS_PUSH__
#define __ZOO_KFK_TOPICS_PUSH__

#include <stdio.h>
#include <string.h>

#include <functional>
#include <string>
#include <map>
#include <vector>
//#include <common/MutexLock.h>

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

typedef struct pushErrorMsg
{
	const char *topic;
	const char *msg;
	const char *key;
	const char *errMsg;
	int msgLen;
	int keyLen;
	int errorCode;
}PUSHERRORMSG;

namespace ZOOKEEPERKAFKA
{

typedef std::function<void(PUSHERRORMSG *msgInfo)> MsgPushErrorCallBack;

typedef std::map<std::string,rd_kafka_topic_t*> KfkTopicPtrMap;
typedef KfkTopicPtrMap::iterator KfkTopicPtrMapIter;

/*
 *kafka往多个topic生成数据
 *topic初始化的时候以逗号分开，不可以有多余的符号
 *所有的topic最大消息大小保持一致
 */
class ZooKfkTopicsPush
{
public:
	//构造函数
	ZooKfkTopicsPush();
	//析构函数
	~ZooKfkTopicsPush();

	//仅仅初始化zookeeper，后面自己调用kfkInit，参数brokers可以为空字符串
	int zookInit(const std::string& zookeepers);
	//初始化zookeeper，内部调用kfkInit初始化topics，可以是多个，逗号分开，不要任何多余的符号
	int zookInit(const std::string& zookeepers,
			  const std::string& topics,
			  int maxMsgqueue = 2 * 1024 * 1024);

	//内部使用的回调函数，使用者不用关心
	void msgPushErrorCall(PUSHERRORMSG *msgInfo)
	{
		if(cb_)
			cb_(msgInfo);
	}

	//设置错误回调函数，一旦写入kafka发送错误，调用此回调
	void setMsgPushErrorCall(const MsgPushErrorCallBack& cb)
	{
		cb_ = cb;
	}

	//kfk初始化，可以自己设置brokers，如果已经设置了zookeeper，brokers可以为空。topics可以是多个，逗号分开，不要任何多余的符号
	int kfkInit(const std::string& brokers,
			  const std::string& topics,
			  int maxMsgqueue = 2 * 1024 * 1024);

	//往kfk生成数据，指定topic,如果topic不存在的话，则返回错误，必须在初始化的时候初始化
	int push(const std::string& topic,
			 const std::string& data,
	         std::string* key = NULL,
	         int partition = RD_KAFKA_PARTITION_UA,
	         int msgFlags = RD_KAFKA_MSG_F_COPY);

	//资源释放，释放所有的资源
	void kfkDestroy();
	//kfk中brokers更改的时候，更改，使用者不用关心，内部调用
	void changeKafkaBrokers(const std::string& brokers);

private:
	zhandle_t* initialize_zookeeper(const char * zookeeper, const int debug);

	bool str2Vec(const char* src, std::vector<std::string>& dest, const char* delim);
	
	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	
	rd_kafka_t* kfkt;

	KfkTopicPtrMap topicPtrMap;
	MsgPushErrorCallBack cb_;

};

}

#endif

