
#ifndef __ZOO_KFK_TOPICS_PUSH__
#define __ZOO_KFK_TOPICS_PUSH__

#include <stdio.h>
#include <string.h>
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <map>
#include <vector>
#include <mutex>

#include "Singleton.h"
#include "noncopyable.h"

#pragma GCC diagnostic ignored "-Wold-style-cast"

#include "librdkafka/rdkafka.h"

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"

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

//typedef std::function<void(void *msgPri, CALLBACKMSG *msgInfo)> MsgPushErrorCallBack;

typedef std::function<void(void *msgPri, CALLBACKMSG *msgInfo)> MsgPushCallBack;
typedef std::function<int(const std::string& brokers)> BrokersChangeCallBack;//kafka ��Ⱥ�仯֪ͨ�ص����п����ǿգ�������ش���0.���˳�ģ�鴦��

typedef std::map<std::string,rd_kafka_topic_t*> KfkTopicPtrMap;
typedef KfkTopicPtrMap::iterator KfkTopicPtrMapIter;

namespace ZOOKEEPERKAFKA
{

/*
 *kafka�����topic��������
 *topic��ʼ����ʱ���Զ��ŷֿ����������ж���ķ���
 *���е�topic�����ڳ�ʼ�������ʼ���ã����������������ݵ�ʱ����
 *���е�topic�����Ϣ��С����һ��
 *���з������ش��ڵ���0��ʾ�ɹ��������д���
 *���ش�����ο� ͷ�ļ�ZooKfkCommon.h
 */
class ZooKfkTopicsPush
{
public:
	//���캯��
	ZooKfkTopicsPush();
	//��������
	~ZooKfkTopicsPush();

	//������ʼ��zookeeper�������Լ�����kfkInit������brokers����Ϊ���ַ���
	//int zookInit(const std::string& zookeepers);
	//��ʼ��zookeeper���ڲ�����kfkInit��ʼ��topics�������Ƕ�������ŷֿ�����Ҫ�κζ���ķ���
	int zookInit(const std::string& zookeepers,
			  const std::string& topics,
			  int queueBuffMaxMs = 1000,
			  int queueBuffMaxMess = 100000);

	//���ô���ص�������һ��д��kafka���ʹ��󣬵��ô˻ص�
	void setMsgPushErrorCall(const MsgPushCallBack& cb)
	{
		cb_ = cb;
	}

	//����ÿ����Ϣ�ص�������Ϣ��дkafka�����ô˻ص�
	void setMsgPushCallBack(const MsgPushCallBack& cb)
	{
		wcb_ = cb;
	}

	//����kafka��Ⱥ�仯�ص�����
	void setBrokersChangeCallBack(const BrokersChangeCallBack& cb)
	{
		downcb_ = cb;
	}

	//kfk��ʼ���������Լ�����brokers�����ǰ�����zookInit��brokers����Ϊ�գ�brokers���zookeeper��ȡ
	//brokersҲ�����Լ�����
	//topics�����Ƕ�������ŷֿ�����Ҫ�κζ���ķ��ţ�����һ���Գ�ʼ���꣬���ڲ�����������topic
	int kfkInit(const std::string& brokers,
			  const std::string& topics,
			  int queueBuffMaxMs = 1000,
			  int queueBuffMaxMess = 100000);

	int producerAddTopic(const std::string& topic);

	//��kfk�������ݣ�ָ��topic,���topic�����ڵĻ����򷵻ش��󣬱����ڳ�ʼ����ʱ���ʼ��
	//�ڲ�����������Ĵ������治��Ҫ���� -184//�ڲ��Ѿ�������������ⲿ�Ͳ���Ҫ����bolckFlush
	//data��Ҫ���ɵ����ݣ�msgPri˽�����ݣ��ص���ʱ���ص�����
	//key�Ƕ���������ÿ����Ϣʱ�ģ�����Ϊ��
	//partition����Ĭ�Ϸ�Ƭ
	//-1 ���������޷���ȷ����
	//-2 topicδ����ʼ��
	//-3 �Ѿ�������kfkDestroy
	int push(const std::string& topic,
			 const std::string& data,
			 std::string* key = NULL,
			 void *msgPri = NULL,
	         int partition = RD_KAFKA_PARTITION_UA,
	         int msgFlags = RD_KAFKA_MSG_F_COPY);


	//�ϲ�Ӧ�ÿ��ԶԱ��ض��н���ˢ�²����������ض���С��queueSizeֵ��ʱ��Ż᷵�أ�����
	//��push���ش����ʱ��getLastErrorMsg == -184��ʱ�򣬱���Ҫ��������������������Բ�������Ĭ��ֵ
	//�������Է�������������ò���Ӱ�������Ƿ񵽷���ˣ�ֻ�ǻ�Ӱ�챾�ض��д�С��push�ڲ��Ѿ������ˣ��ⲿ���õ���
	int bolckFlush(bool lockFlag = true, int queueSize = 0);

	//��Դ�ͷţ��ͷ����е���Դ���첽�˳��������Դʹ��,�˷����������ģ��˳�֮��ͱ�ʾ���������Ѿ�����ɾ�
	void kfkDestroy();

	//��ȡ���һ�δ�����Ϣ�����д����ʱ��Ӧ�õ��������������ӡ������־
	int getLastErrorMsg(std::string& msg)
	{
		msg.assign(kfkErrorMsg);
		return static_cast<int>(kfkErrorCode);
	}

	//kfk��brokers���ĵ�ʱ�򣬸��ģ�ʹ���߲��ù��ģ��ڲ�����
	void changeKafkaBrokers(const std::string& brokers);

	//�ڲ�ʹ�õĻص�������ʹ���߲��ù���
	void msgPushErrorCall(void *msgPri, CALLBACKMSG *msgInfo)
	{
		if(cb_)
			cb_(msgPri, msgInfo);
	}

	//�ڲ�ʹ�õĻص�������ʹ���߲��ù���
	void msgPushWriteCall(void *msgPri, CALLBACKMSG *msgInfo)
	{
		if(wcb_)
			wcb_(msgPri, msgInfo);
	}
private:
	int zookInit(const std::string& zookeepers);
	
	zhandle_t* initialize_zookeeper(const char* zookeeper, const int debug);

	bool str2Vec(const char* src, std::vector<std::string>& dest, const char delim);

	void setKfkErrorMessage(rd_kafka_resp_err_t code,const char *msg);

	std::mutex topicMapLock;
	std::string zKeepers;
	zhandle_t *zookeeph;
	std::string kfkBrokers;
	
	rd_kafka_t* kfkt;

	KfkTopicPtrMap topicPtrMap;
	MsgPushCallBack cb_;
	MsgPushCallBack wcb_;
	BrokersChangeCallBack downcb_;

	rd_kafka_resp_err_t kfkErrorCode;
	std::string kfkErrorMsg;
	int destroy;
	int initFlag;
};

typedef std::shared_ptr<ZOOKEEPERKAFKA::ZooKfkTopicsPush> ZooKfkProducerPtr;
//////////////////////////////////////////////////////////////////////////////////////version 1.0
/*
 *��������ߵ�ʵ��ģʽ��������������е����ò�������һ����
 *����ֱ�Ӱ���ͷ�ļ���ʵ��ʹ�ã�����ֱ��ָ����ʼ����������ߣ���߲�����
 *���ش�����ο� ͷ�ļ�ZooKfkCommon.h
 */
class ZooKfkProducers : public noncopyable
{
public:
	ZooKfkProducers()
		:lastIndex(0)
		,kfkProducerNum(0)
		,ZooKfkProducerPtrVec()
	{
	}

	~ZooKfkProducers()
	{
		for(int i = 0; i < kfkProducerNum; i++)
		{
			if(ZooKfkProducerPtrVec.size() > 0 && ZooKfkProducerPtrVec[i])
				ZooKfkProducerPtrVec[i].reset();
		}

		std::vector<ZooKfkProducerPtr> ().swap(ZooKfkProducerPtrVec);
	}

	//��ʵ���ӿ�
	static ZooKfkProducers& instance() { return ZOOKEEPERKAFKA::Singleton<ZooKfkProducers>::instance(); }

	//��ʼ���ӿڣ������߸�����zookeeper��ַ��Ϣ����Ҫ������topic
	int zooKfkProducersInit(int produceNum, const std::string& zookStr, const std::string& topicStr);

	//�����˳�֮ǰ���ýӿڣ���֤���ᶪ���ݣ�����ȫ���־û���
	void zooKfkProducersDestroy();
	
	//���ô���ص��ӿ�
	int setMsgPushErrorCall(const MsgPushCallBack& cb);

	//����kafka��Ⱥ�仯�ص�
	int setBrokersChangeCall(const BrokersChangeCallBack& cb);

	//����ÿ����Ϣ�ص��ӿڣ����������ص�
	int setMsgPushCallBack(const MsgPushCallBack& cb);

	//д��Ϣ��topic���ƣ���Ϣ���ݣ�����д��󣬷��ش�����Ϣ��key��Ĭ�Ͽ�
	int psuhKfkMsg(const std::string& topic, const std::string& msg, std::string& errorMsg, std::string* key = NULL, void *msgPri = NULL);

	int addProducerTopic(const std::string& topic);

	int produceFlush(int index);

private:
	//volatile unsigned int lastIndex;
	unsigned int lastIndex;//ÿ��д���±꣬����д
	int kfkProducerNum;//�����߸���
	std::vector<ZooKfkProducerPtr> ZooKfkProducerPtrVec;
};
//////////////////////////////////////////////////////////////////////////////////////version 2.0

/*
 *��������ߵ�ʵ��ģʽ��������������е����ò�������һ����
 *����ֱ�Ӱ���ͷ�ļ���ʵ��ʹ�ã�����ֱ��ָ����ʼ����������ߣ���߲�����
 *���ش�����ο� ͷ�ļ�ZooKfkCommon.h
 */
class ZooKfkGenerators : public noncopyable
{
public:
	ZooKfkGenerators()
		:lastIndex(0)
		,kfkProducerNum(0)
		,ZooKfkProducerPtrVec()
	{
	}

	~ZooKfkGenerators()
	{
		for(int i = 0; i < kfkProducerNum; i++)
		{
			if(ZooKfkProducerPtrVec.size() > 0 && ZooKfkProducerPtrVec[i])
				ZooKfkProducerPtrVec[i].reset();
		}

		std::vector<ZooKfkProducerPtr> ().swap(ZooKfkProducerPtrVec);
	}

	//��ʵ���ӿ�
	static ZooKfkGenerators& instance() { return ZOOKEEPERKAFKA::Singleton<ZooKfkGenerators>::instance(); }

	//��ʼ���ӿڣ������߸�����zookeeper��ַ��Ϣ����Ҫ������topic
	int zooKfkProducersInit(int produceNum, const std::string& brokerStr, const std::string& topicStr);

	//�����˳�֮ǰ���ýӿڣ���֤���ᶪ���ݣ�����ȫ���־û���
	void zooKfkProducersDestroy();
	
	//���ô���ص��ӿ�
	int setMsgPushErrorCall(const MsgPushCallBack& cb);

	//����kafka��Ⱥ�仯�ص�
	int setBrokersChangeCall(const BrokersChangeCallBack& cb);
	
	//����ÿ����Ϣ�ص��ӿڣ����������ص�
	int setMsgPushCallBack(const MsgPushCallBack& cb);

	//д��Ϣ��topic���ƣ���Ϣ���ݣ�����д��󣬷��ش�����Ϣ��key��Ĭ�Ͽ�
	int psuhKfkMsg(const std::string& topic, const std::string& msg, std::string& errorMsg, std::string* key = NULL, void *msgPri = NULL);

	int addGereratorsTopic(const std::string& topic);
		
	int produceFlush(int index);

private:
	//volatile unsigned int lastIndex;
	unsigned int lastIndex;//ÿ��д���±꣬����д
	int kfkProducerNum;//�����߸���
	std::vector<ZooKfkProducerPtr> ZooKfkProducerPtrVec;
};
//////////////////////////////////////////////////////////////////////////////////////version 3.0


}

#endif

