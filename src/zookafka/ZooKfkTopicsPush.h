
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
 *kafka�����topic��������
 *topic��ʼ����ʱ���Զ��ŷֿ����������ж���ķ���
 *���е�topic�����Ϣ��С����һ��
 */
class ZooKfkTopicsPush
{
public:
	//���캯��
	ZooKfkTopicsPush();
	//��������
	~ZooKfkTopicsPush();

	//������ʼ��zookeeper�������Լ�����kfkInit������brokers����Ϊ���ַ���
	int zookInit(const std::string& zookeepers);
	//��ʼ��zookeeper���ڲ�����kfkInit��ʼ��topics�������Ƕ�������ŷֿ�����Ҫ�κζ���ķ���
	int zookInit(const std::string& zookeepers,
			  const std::string& topics,
			  int maxMsgqueue = 2 * 1024 * 1024);

	//�ڲ�ʹ�õĻص�������ʹ���߲��ù���
	void msgPushErrorCall(PUSHERRORMSG *msgInfo)
	{
		if(cb_)
			cb_(msgInfo);
	}

	//���ô���ص�������һ��д��kafka���ʹ��󣬵��ô˻ص�
	void setMsgPushErrorCall(const MsgPushErrorCallBack& cb)
	{
		cb_ = cb;
	}

	//kfk��ʼ���������Լ�����brokers������Ѿ�������zookeeper��brokers����Ϊ�ա�topics�����Ƕ�������ŷֿ�����Ҫ�κζ���ķ���
	int kfkInit(const std::string& brokers,
			  const std::string& topics,
			  int maxMsgqueue = 2 * 1024 * 1024);

	//��kfk�������ݣ�ָ��topic,���topic�����ڵĻ����򷵻ش��󣬱����ڳ�ʼ����ʱ���ʼ��
	int push(const std::string& topic,
			 const std::string& data,
	         std::string* key = NULL,
	         int partition = RD_KAFKA_PARTITION_UA,
	         int msgFlags = RD_KAFKA_MSG_F_COPY);

	//��Դ�ͷţ��ͷ����е���Դ
	void kfkDestroy();
	//kfk��brokers���ĵ�ʱ�򣬸��ģ�ʹ���߲��ù��ģ��ڲ�����
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

