
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

typedef std::list<std::string> ListStringTopic;
typedef ListStringTopic::iterator ListStringTopicIter;

namespace ZOOKEEPERKAFKA
{
/*
 *kfk���Ѷ��topic�������������ѡ���������kfk�����ȡ
 *����ͳһ��ʼλ��
 *���Զ�ĳһ��topic��ʼ����ֹͣ
 *ͳһ����
 */
class ZooKfkTopicsPop
{
public:
	//���캯��
	ZooKfkTopicsPop();
	//��������
	~ZooKfkTopicsPop();

	//��������zookeeper�ĵ�ַ��Ϣ�����ŷָ����ip port������Լ�����kfkInit
	int zookInit(const std::string& zookeepers);
	//����zookeeper��ַ��Ϣ�����ŷָ����ip port��������topic�����ŷָ����������ж���ķ��ţ��ڲ�����kfkInit
	int zookInit(const std::string& zookeepers, const std::string& topic);
	//kfk��ʼ����brokers���Դ��룬Ҳ���Դ��գ���ʹ��zookeeper��ȡ�ģ����ŷָ����ip port�����topic�����ŷָ�
	int kfkInit(const std::string& brokers, const std::string& topic);
	//����ĳһ��topic����������ڳ�ʼ����topic�У����򱨴�
	int kfkTopicConsumeStart(const std::string& topic);	
	//��ȡkfkһ����Ϣ�������Ի�ȡ��Ӧ��topic��ƫ������key
	int pop(std::string& topic, std::string& data, int64_t* offset = NULL, std::string* key = NULL);
	//ֹͣĳһ��topic����������ڳ�ʼ����topic
	int kfkTopicConsumeStop(const std::string& topic);
	//������Դ��Ϣ
	void kfkDestroy();
	//zookeeper����brokers�仯����brokers
	void changeKafkaBrokers(const std::string& brokers);
private:
	zhandle_t* initialize_zookeeper(const char * zookeeper, const int debug);

	bool str2Vec(const char* src, std::vector<std::string>& dest, const char* delim);

	common::MutexLock listLock;
	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	ListStringTopic topics_;
	
	rd_kafka_t* kfkt;
	rd_kafka_conf_t* kfkconft;
	
	rd_kafka_topic_conf_t* kfktopiconft;
	rd_kafka_topic_partition_list_t* topicparlist;

	size_t kMessageMaxSize;
	int64_t startOffset;
	int32_t partition;
};

}

#endif

