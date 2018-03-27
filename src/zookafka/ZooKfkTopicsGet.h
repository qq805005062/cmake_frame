
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
 *kfk���Ѷ��topic�������ָ��������ָ����ʼλ��
 *���Զ��������������ʼλ�ã���ȡָ��topic��ָ�������������ȳ�ʼ��������ʽ�Ļ�ȡ
 *���Զ�ĳһ��topic��������ʼ��ֹͣ
 *ͳһ����
 */
class ZooKfkTopicsGet
{
public:
	//���캯��
	ZooKfkTopicsGet();
	//��������
	~ZooKfkTopicsGet();

	//��������zookeeper�ĵ�ַ��Ϣ�����ŷָ����ip port���ڲ�ֱ�ӵ���kfkInit
	int zookInit(const std::string& zookeepers);
	//kfk��ʼ��������zookeeper��ʼ��
	int kfkInit(const std::string& brokers);
	//����ĳһ��topic����ָ��������ָ��ƫ���������topic����ε���
	int kfkTopicConsumeStart(const std::string& topic, int partition = 0, int64_t offset = RD_KAFKA_OFFSET_END);	
	//��ȡkfk��Ϣ����ȡָ��topic����Ϣ���������ݣ�ƫ������key������ʽ��һֱ��������Ϊֹ
	int get(std::string& topic, std::string& data, int64_t* offset = NULL, int partition = 0, std::string* key = NULL);
	//ֹͣĳһ��topic����ָ��������ÿ��ֻ�ܴ���һ��topic�����topic����ε���
	int kfkTopicConsumeStop(const std::string& topic, int partition = RD_KAFKA_PARTITION_UA);
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
	
	rd_kafka_t* kfkt;
	KfkTopicPtrMap topicPtrMap;
	
	size_t kMessageMaxSize;
};

}

#endif

