
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


typedef std::list<std::string> ListStringTopic;
typedef ListStringTopic::iterator ListStringTopicIter;

typedef std::function<int(const std::string& brokers)> BrokersChangeCallBack;//kafka ��Ⱥ�仯֪ͨ�ص����п����ǿգ�������ش���0.���˳�ģ�鴦��

namespace ZOOKEEPERKAFKA
{
/*
 *kfk���Ѷ��topic�������������ѡ���������kfk�����ȡ
 *����ͳһ��ʼλ��(��ʼ�����캯�������޸ģ�Ĭ���Ƕ�β��
 *���Զ�ĳһ��topic��ʼ����ֹͣ����ʼ����topic�Ѿ�Ĭ�Ͽ�ʼ�ˣ�����Ҫ�ظ�����kfkTopicConsumeStart
 *ͳһ���飬���������Դ��ݿ��ַ����������Ͳ�����
 *���з������ش��ڵ���0 ��ʾ�ɹ��������ʾ�д���
 *���ش�����ο� ͷ�ļ�ZooKfkCommon.h
 */
class ZooKfkTopicsPop
{
public:
	//���캯��
	ZooKfkTopicsPop();
	//��������
	~ZooKfkTopicsPop();

	//��������zookeeper�ĵ�ַ��Ϣ�����ŷָ����ip port������Լ�����kfkInit
	//int zookInit(const std::string& zookeepers);
	//����zookeeper��ַ��Ϣ�����ŷָ����ip port��������topic�����ŷָ����������ж���ķ��ţ��ڲ�����kfkInit
	//����Ҫ�Լ��ٴε���kfkInit
	int zookInit(const std::string& zookeepers, const std::string& topic, const std::string& groupName);

	//����kafka��Ⱥ�仯�ص�����
	void setBrokersChangeCallBack(const BrokersChangeCallBack& cb)
	{
		downcb_ = cb;
	}
	
	//kfk��ʼ�������ǰ�������zookInit��brokers���Դ���գ�brokers���Դ�zookeeper�л�ȡ��
	//brokersҲ���Բ�����գ����ǰ��û�е���zookInit�����topic�����ŷָ�
	//��ʼ����topicĬ�ϵ�����kfkTopicConsumeStart
	//groupName���Ϊ���ַ����򲻷���
	int kfkInit(const std::string& brokers, const std::string& topic, const std::string& groupName);
	//����ĳһ��topic����ÿ��ֻ�ܴ���һ��topic������ֶ�ε���
	int kfkTopicConsumeStart(const std::string& topic);	
	//��ȡkfkһ����Ϣ�������Ի�ȡ��Ӧ��topic��ƫ������key��
	//-1��ʾ�д��ڲ�������-191��������β�Ĵ���)
	//��Ҫ�����ؿ����ݴ���
	int pop(std::string& topic, std::string& data, std::string* key = NULL, int64_t* offset = NULL, int32_t* parnum = NULL);
	//��ȡkfkһ����Ϣ��timeout_ms��ʱʱ�䣬
	//�ڲ��Ѿ�����-191�Ĵ��󣬲���Ҫ�ⲿ����-191�Ĵ���
	//��Ҫ�����ؿ����ݴ���
	int tryPop(std::string& topic, std::string& data, int timeout_ms, std::string* key = NULL, int64_t* offset = NULL, int32_t* parnum = NULL);
	
	//ֹͣĳһ��topic������������Ѿ�����topic
	int kfkTopicConsumeStop(const std::string& topic);
	//������Դ��Ϣ���첽�˳��������Դʹ��
	void kfkDestroy();

	//��ȡ���һ�δ�����Ϣ�����д����ʱ��Ӧ�õ��������������ӡ������־
	int getLastErrorMsg(std::string& msg)
	{
		msg.assign(kfkErrorMsg);
		return static_cast<int>(kfkErrorCode);
	}

	void kfkSubscription();
	
	//zookeeper����brokers�仯����brokers
	void changeKafkaBrokers(const std::string& brokers);
private:
	int zookInit(const std::string& zookeepers);
	
	zhandle_t* initialize_zookeeper(const char* zookeeper, const int debug);

	bool str2Vec(const char* src, std::vector<std::string>& dest, const char delim);

	void setKfkErrorMessage(rd_kafka_resp_err_t code,const char *msg);

	std::mutex listLock;
	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	std::string groupName_;
	ListStringTopic topics_;
	
	rd_kafka_t* kfkt;

	size_t kMessageMaxSize;
	int64_t startOffset;
	int32_t partition;

	rd_kafka_resp_err_t kfkErrorCode;
	std::string kfkErrorMsg;
	int destroy;
	int initFlag;
	int switchFlag;
	int errorFlag;

	BrokersChangeCallBack downcb_;
};

typedef std::shared_ptr<ZOOKEEPERKAFKA::ZooKfkTopicsPop> ZooKfkConsumerPtr;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////version 1.0

/*
 *��������ߵ�ʵ��ģʽ��������������е����ò�������һ����
 *����ֱ�Ӱ���ͷ�ļ���ʵ��ʹ�ã�����ֱ��ָ����ʼ����������ߣ���߲�����
 *���ش�����ο� ͷ�ļ�ZooKfkCommon.h
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

	//��ʵ���ӿ�Ŷ
	static ZooKfkConsumers& instance() { return ZOOKEEPERKAFKA::Singleton<ZooKfkConsumers>::instance(); }

	//��ʼ���ӿڣ������߸�����zookeeper��ַ������topic���ƣ�������
	int zooKfkConsumerInit(int consumerNum, const std::string& zookStr, const std::string& topicStr,  const std::string& groupName);

	int kfkBorsConsumerInit(int consumerNum, const std::string& borsStr, const std::string& topicStr,  const std::string& groupName);

	//�������˳��ӿڣ���֤��Ϣ����ʧ�������˳����ýӿ�
	void zooKfkConsumerDestroy();

	//����һ��topic�Ķ�������ֻ������һ��
	int zooKfkConsumerStart(const std::string& topic);

	//�ر�һ��topic�Ķ�������ֻ������һ��
	int zooKfkConsumerStop(const std::string& topic);

	//����kafka��Ⱥ�仯�ص�
	int setBrokersChangeCall(const BrokersChangeCallBack& cb);
	
	//����һ����Ϣ������д��󣬻᷵�ش�����Ϣ���Զ������˶�β�Ĵ����±��ʾ�õ��±꣬���ܴ��ڵ��ڳ�ʼ���ĸ���
	int consume(int index, std::string& topic, std::string& data, std::string& errorMsg, std::string* key = NULL, int64_t* offset = NULL);

	//��ʱ����һ����Ϣ�������ʱ������ϢҲ�᷵�أ�����1��������Ϣ��0�ǳ�ʱ������С��0���±겻���ظ����ڵ��ڳ�ʼ���������ڲ����˵���β�Ĵ��󣬲����Ե���ʱ����
	int tryConsume(int index, std::string& topic, std::string& data, int timeout_ms, std::string& errorMsg, std::string* key = NULL, int64_t* offset = NULL);
	
private:
	//volatile unsigned int lastIndex;
	unsigned int lastIndex;
	int kfkConsumerNum;
	std::vector<ZooKfkConsumerPtr> ZooKfkConsumerPtrVec;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////version 2.0

/*
 *��������ߵ�ʵ��ģʽ��������������е����ò�������һ����
 *����ֱ�Ӱ���ͷ�ļ���ʵ��ʹ�ã�����ֱ��ָ����ʼ����������ߣ���߲�����
 *���ش�����ο� ͷ�ļ�ZooKfkCommon.h
 */
class ZooKfkSubscribes : public noncopyable
{
public:
	ZooKfkSubscribes()
		:lastIndex(0)
		,kfkConsumerNum(0)
		,ZooKfkConsumerPtrVec()
	{
	}

	~ZooKfkSubscribes()
	{
		for(int i = 0;i < kfkConsumerNum; i++)
		{
			if(ZooKfkConsumerPtrVec[i])
				ZooKfkConsumerPtrVec[i].reset();
		}

		std::vector<ZooKfkConsumerPtr> ().swap(ZooKfkConsumerPtrVec);
	}

	//��ʵ���ӿ�Ŷ
	static ZooKfkSubscribes& instance() { return ZOOKEEPERKAFKA::Singleton<ZooKfkSubscribes>::instance(); }

	//��ʼ���ӿڣ������߸�����zookeeper��ַ������topic���ƣ�������
	int zooKfkConsumerInit(int consumerNum, const std::string& brokerStr, const std::string& topicStr,  const std::string& groupName);

	//�������˳��ӿڣ���֤��Ϣ����ʧ�������˳����ýӿ�
	void zooKfkConsumerDestroy();

	//����һ��topic�Ķ�������ֻ������һ��
	int zooKfkConsumerStart(const std::string& topic);

	//�ر�һ��topic�Ķ�������ֻ������һ��
	int zooKfkConsumerStop(const std::string& topic);

	//����kafka��Ⱥ�仯�ص�
	int setBrokersChangeCall(const BrokersChangeCallBack& cb);
	
	//����һ����Ϣ������д��󣬻᷵�ش�����Ϣ���Զ������˶�β�Ĵ����±��ʾ�õ��±꣬���ܴ��ڵ��ڳ�ʼ���ĸ���
	int consume(int index, std::string& topic, std::string& data, std::string& errorMsg, std::string* key = NULL, int64_t* offset = NULL);

	//��ʱ����һ����Ϣ�������ʱ������ϢҲ�᷵�أ�����1��������Ϣ��0�ǳ�ʱ������С��0���±겻���ظ����ڵ��ڳ�ʼ���������ڲ����˵���β�Ĵ��󣬲����Ե���ʱ����
	int tryConsume(int index, std::string& topic, std::string& data, int timeout_ms, std::string& errorMsg, std::string* key = NULL, int64_t* offset = NULL);
	
private:
	//volatile unsigned int lastIndex;
	unsigned int lastIndex;
	int kfkConsumerNum;
	std::vector<ZooKfkConsumerPtr> ZooKfkConsumerPtrVec;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////version 3.0

}

#endif

