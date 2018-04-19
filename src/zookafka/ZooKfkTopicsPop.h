
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
	int zookInit(const std::string& zookeepers, const std::string& topic, const std::string groupName);
	//kfk��ʼ����brokers���Դ��룬Ҳ���Դ��գ���ʹ��zookeeper��ȡ�ģ����ŷָ����ip port�����topic�����ŷָ�
	int kfkInit(const std::string& brokers, const std::string& topic, const std::string groupName);
	//����ĳһ��topic����������ڳ�ʼ����topic�У����򱨴�
	int kfkTopicConsumeStart(const std::string& topic);	
	//��ȡkfkһ����Ϣ�������Ի�ȡ��Ӧ��topic��ƫ������key��
	int pop(std::string& topic, std::string& data, int64_t* offset = NULL, std::string* key = NULL);
	//��ȡkfkһ����Ϣ��timeout_ms��ʱʱ�䣬����-1��ʾ�д��󣬷���0��ʾ����������1��ʾ��ʱ
	int tryPop(std::string& topic, std::string& data, int timeout_ms, int64_t* offset = NULL, std::string* key = NULL);
	
	//ֹͣĳһ��topic����������ڳ�ʼ����topic
	int kfkTopicConsumeStop(const std::string& topic);
	//������Դ��Ϣ
	void kfkDestroy();

	int getLastErrorMsg(std::string& msg)
	{
		msg.assign(kfkErrorMsg);
		return static_cast<int>(kfkErrorCode);
	}
	
	//zookeeper����brokers�仯����brokers
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

