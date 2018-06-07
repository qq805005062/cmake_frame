
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

typedef struct callBackMsg
{
	const char *topic;
	const char *msg;
	const char *key;
	const char *errMsg;
	int64_t offset;
	int msgLen;
	int keyLen;
	int errorCode;
}CALLBACKMSG;

namespace ZOOKEEPERKAFKA
{

//typedef std::function<void(void *msgPri, CALLBACKMSG *msgInfo)> MsgPushErrorCallBack;

typedef std::function<void(void *msgPri, CALLBACKMSG *msgInfo)> MsgPushCallBack;

typedef std::map<std::string,rd_kafka_topic_t*> KfkTopicPtrMap;
typedef KfkTopicPtrMap::iterator KfkTopicPtrMapIter;

/*
 *kafka往多个topic生成数据
 *topic初始化的时候以逗号分开，不可以有多余的符号
 *所有的topic必须在初始化里面初始化好，否则真正生产数据的时候会出
 *所有的topic最大消息大小保持一致
 *所有方法返回大于等于0表示成功，否则有错误
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
			  int queueBuffMaxMs = 500,
			  int queueBuffMaxMess = 2 * 1024 * 1024);

	//设置错误回调函数，一旦写入kafka发送错误，调用此回调
	void setMsgPushErrorCall(const MsgPushCallBack& cb)
	{
		cb_ = cb;
	}

	//设置每条消息回调，当消息回写kafka则会调用此回调
	void setMsgPushCallBack(const MsgPushCallBack& cb)
	{
		wcb_ = cb;
	}

	//kfk初始化，可以自己设置brokers，如果前面调用zookInit则brokers可以为空，brokers会从zookeeper获取
	//brokers也可以自己设置
	//topics可以是多个，逗号分开，不要任何多余的符号，必须一次性初始化完，后期不可以再增加topic
	int kfkInit(const std::string& brokers,
			  const std::string& topics,
			  int queueBuffMaxMs = 500,
			  int queueBuffMaxMess = 2 * 1024 * 1024);

	//往kfk生成数据，指定topic,如果topic不存在的话，则返回错误，必须在初始化的时候初始化
	//data是要生成的数据，msgPri私有数据，回调的时候会回调回来
	//key是对于消费者每个消息时的，可以为空
	//partition分区默认分片
	//-1 参数错误，无法正确处理
	//-2 topic未被初始化
	//-3 已经调用了kfkDestroy
	int push(const std::string& topic,
			 const std::string& data,
			 std::string* key = NULL,
			 void *msgPri = NULL,
	         int partition = RD_KAFKA_PARTITION_UA,
	         int msgFlags = RD_KAFKA_MSG_F_COPY);


	//上层应用可以对本地队列进行刷新操作，当本地队列小于queueSize值的时候才会返回，阻塞
	int bolckFlush(int queueSize);

	//资源释放，释放所有的资源、异步退出，清除资源使用,此方法是阻塞的，退出之后就表示所有数据已经清理干净
	void kfkDestroy();

	//获取最后一次错误信息，当有错误的时候应该调用这个方法，打印错误日志
	int getLastErrorMsg(std::string& msg)
	{
		msg.assign(kfkErrorMsg);
		return static_cast<int>(kfkErrorCode);
	}

	//kfk中brokers更改的时候，更改，使用者不用关心，内部调用
	void changeKafkaBrokers(const std::string& brokers);

	//内部使用的回调函数，使用者不用关心
	void msgPushErrorCall(void *msgPri, CALLBACKMSG *msgInfo)
	{
		if(cb_)
			cb_(msgPri, msgInfo);
	}

	//内部使用的回调函数，使用者不用关心
	void msgPushWriteCall(void *msgPri, CALLBACKMSG *msgInfo)
	{
		if(wcb_)
			wcb_(msgPri, msgInfo);
	}
private:
	zhandle_t* initialize_zookeeper(const char * zookeeper, const int debug);

	bool str2Vec(const char* src, std::vector<std::string>& dest, const char delim);

	void setKfkErrorMessage(rd_kafka_resp_err_t code,const char *msg);
	
	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	
	rd_kafka_t* kfkt;

	KfkTopicPtrMap topicPtrMap;
	MsgPushCallBack cb_;
	MsgPushCallBack wcb_;

	rd_kafka_resp_err_t kfkErrorCode;
	std::string kfkErrorMsg;
	int destroy;
	int pushNum;
};

}

#endif

