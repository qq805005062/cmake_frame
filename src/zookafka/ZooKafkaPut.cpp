
#include <signal.h>

#include "ZooKafkaPut.h"

#define KFK_LOG_EMERG   0
#define KFK_LOG_ALERT   1
#define KFK_LOG_CRIT    2
#define KFK_LOG_ERR     3
#define KFK_LOG_WARNING 4
#define KFK_LOG_NOTICE  5
#define KFK_LOG_INFO    6
#define KFK_LOG_DEBUG   7

namespace ZOOKEEPERKAFKA
{

static const char KafkaBrokerPath[] = "/brokers/ids";

static void kfkLogger(const rd_kafka_t* rdk, int level, const char* fac, const char* buf)
{
	PDEBUG("rdkafka-%d-%s: %s: %s\n", level, fac, rdk ? rd_kafka_name(rdk) : NULL, buf);
}

static int set_brokerlist_from_zookeeper(zhandle_t *zzh, char *brokers)
{
	int ret = 0;
	if (zzh)
	{
		struct String_vector brokerlist;
		if (zoo_get_children(zzh, KafkaBrokerPath, 1, &brokerlist) != ZOK)
		{
			PERROR("No brokers found on path %s\n", KafkaBrokerPath);
			return ret;
		}

		int i;
		char *brokerptr = brokers;
		for (i = 0; i < brokerlist.count; i++)
		{
			char path[255] = {0}, cfg[1024] = {0};
			sprintf(path, "/brokers/ids/%s", brokerlist.data[i]);
			PDEBUG("brokerlist path :: %s\n",path);
			int len = sizeof(cfg);
			zoo_get(zzh, path, 0, cfg, &len, NULL);

			if (len > 0)
			{
				cfg[len] = '\0';
				json_error_t jerror;
				json_t *jobj = json_loads(cfg, 0, &jerror);
				if (jobj)
				{
					json_t *jhost = json_object_get(jobj, "host");
					json_t *jport = json_object_get(jobj, "port");

					if (jhost && jport)
					{
						const char *host = json_string_value(jhost);
						const int   port = json_integer_value(jport);
						ret++;
						sprintf(brokerptr, "%s:%d", host, port);
						PDEBUG("brokerptr value :: %s\n",brokerptr);
						
						brokerptr += strlen(brokerptr);
						if (i < brokerlist.count - 1)
						{
							*brokerptr++ = ',';
						}
					}
					json_decref(jobj);
				}
			}
		}
		deallocate_String_vector(&brokerlist);
		PDEBUG("Found brokers:: %s\n",brokers);
	}
	
	return ret;
}

static void watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
	int ret = 0;
	char brokers[1024] = {0};
	if (type == ZOO_CHILD_EVENT && strncmp(path, KafkaBrokerPath, strlen(KafkaBrokerPath)) == 0)
	{
		ret = set_brokerlist_from_zookeeper(zh, brokers);
		if( ret > 0 )
		{
			PDEBUG("Found brokers:: %s\n",brokers);
			ZooKafkaPut *pZooKafkaPut = static_cast<ZooKafkaPut *>(watcherCtx);
			pZooKafkaPut->changeKafkaBrokers(brokers);
			//rd_kafka_brokers_add(rk, brokers);
			//rd_kafka_poll(rk, 10);
		}
	}
}

static void msgDelivered(rd_kafka_t *rk, const rd_kafka_message_t* message, void *opaque)
{
	PDEBUG("deliver: %s: offset %ld\n", rd_kafka_err2str(message->err), message->offset);
	if (message->err)
	{
		// �������дʧ�ܵĴ���
		PERROR("%% Message delivery failed: %s\n", rd_kafka_err2str(message->err));
		ZooKafkaPut *pKfkPut = static_cast<ZooKafkaPut *>(opaque);
		pKfkPut->msgPushErrorCall(message->err,rd_kafka_err2str(message->err));
	}
	else
	{
		// ���ͳɹ���������ֻ��ʾ����һ����Բ�����ɹ��ģ�ֻ����ʧ�ܵľ�OK
		std::string data, key;
		data.assign(const_cast<const char* >(static_cast<char* >(message->payload)), message->len);
		key.assign(const_cast<const char* >(static_cast<char* >(message->key)), message->key_len);

		PDEBUG("%% success callback (%zd bytes, offset %ld, partition %d) msg: %s, key:%s\n",
		      message->len, message->offset, message->partition, data.c_str(), key.c_str());
	}
}

ZooKafkaPut::ZooKafkaPut()
	:kfkLock()
	,zKeepers()
	,zookeeph(nullptr)
	,kfkBrokers()
	,kfkt(nullptr)
	,kfkconft(nullptr)
	,kfktopic(nullptr)
	,kfktopiconft(nullptr)
	,cb_(nullptr)
{
	PDEBUG("ZooKafkaPut struct\n");
}

ZooKafkaPut::~ZooKafkaPut()
{
	kfkDestroy();
	PDEBUG("ZooKafkaPut exit\n");
}

int ZooKafkaPut::zookInit(const std::string& zookeepers,
			  const std::string& topicName,
			  int partition,
			  int maxMsgqueue)
{
	int ret = 0;
	char brokers[1024] = {0};
	/////////////////////////
	zookeeph = initialize_zookeeper(zookeepers.c_str(), 1);
	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	////////////////////////////////////////////////
	if(ret < 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d\n",ret);
		return ret;
	}
	
	zKeepers.clear();
	zKeepers = zookeepers;
	kfkBrokers.clear();
	kfkBrokers.append(brokers);
	ret = kfkInit(kfkBrokers,topicName,partition,maxMsgqueue);
	if(ret < 0)
	{
		PERROR("kfkInit error :: %d\n",ret);
	}
	return ret;
}

void ZooKafkaPut::msgPushErrorCall(int errorCode,const char* errorMsg)
{
	if(cb_)
		cb_(errorCode,errorMsg);
}

int ZooKafkaPut::kfkInit(const std::string& brokers,
			  const std::string& topicName,
			  int partition,
			  int maxMsgqueue)
{
	char tmp[64] = {0},errStr[512] = {0};
	kfkconft = rd_kafka_conf_new();
	rd_kafka_conf_set_log_cb(kfkconft, kfkLogger);
	rd_kafka_conf_set_opaque(kfkconft,this);
	rd_kafka_conf_set_dr_msg_cb(kfkconft, msgDelivered);
	snprintf(tmp, sizeof tmp, "%i", SIGIO);
	rd_kafka_conf_set(kfkconft, "internal.termination.signal", tmp, NULL, 0);
	snprintf(tmp, sizeof(tmp), "%d", maxMsgqueue);
	rd_kafka_conf_set(kfkconft, "queue.buffering.max.messages", tmp, NULL, 0);
	rd_kafka_conf_set(kfkconft, "message.send.max.retries", "3", NULL, 0);
	rd_kafka_conf_set(kfkconft, "retry.backoff.ms", "500", NULL, 0);

	kfktopiconft = rd_kafka_topic_conf_new();
	rd_kafka_topic_conf_set(kfktopiconft, "produce.offset.report", "true", errStr, sizeof(errStr));
	rd_kafka_topic_conf_set(kfktopiconft, "request.required.acks", "1", errStr, sizeof(errStr));

	kfkt = rd_kafka_new(RD_KAFKA_PRODUCER, kfkconft, errStr, sizeof errStr);
	if(!kfkt)
	{
		PERROR("***Failed to create new producer: %s***\n", errStr);
		return -1;
	}
	rd_kafka_set_log_level(kfkt, KFK_LOG_DEBUG);

	if (rd_kafka_brokers_add(kfkt, brokers.c_str()) == 0)
	{
		PERROR("*** No valid brokers specified: %s ***\n", brokers.c_str());
		return -1;
	}

	kfktopic = rd_kafka_topic_new(kfkt, topicName.c_str(), kfktopiconft);
	if(kfkBrokers.empty())
		kfkBrokers = brokers;

	return 0;
}

int ZooKafkaPut::push(const std::string& data,
	         std::string* key,
	         int partition,
	         int msgFlags)
{
	int ret = -1;
	common::MutexLockGuard lock(kfkLock);
	if (data.empty())
	{
		PERROR("push value is null\n");
		return ret;
	}

	// �����ӵ���Ϣ��Ŀ���������õ�"queue.buffering.max.messages"���õ���Ŀ
	// ��rd_kafka_produce()����������-1����errno����ΪENOBUFS
	ret = rd_kafka_produce(kfktopic,
	                       partition,
	                       msgFlags,
	                       reinterpret_cast<void* >(const_cast<char* >(data.c_str())),
	                       data.size(),
	                       key == NULL ? NULL : reinterpret_cast<const void* >(key->c_str()),
	                       key == NULL ? 0 : key->size(),
	                       key == NULL ? NULL : reinterpret_cast<void* >(const_cast<char* >(key->c_str())));
	if (ret == -1)
	{
		PERROR("*** Failed to produce to topic %s partition %d: %s ***\n",
		      rd_kafka_topic_name(kfktopic),
		      partition,
		      rd_kafka_err2str(rd_kafka_last_error()));

		rd_kafka_poll(kfkt, 0);
		return ret;
	}

	PDEBUG("*** Push %lu bytes to topic:%s partition: %i***\n",
	      data.size(),
	      rd_kafka_topic_name(kfktopic),
	      partition);
	rd_kafka_poll(kfkt, 1000);
	return 0;
}

void ZooKafkaPut::kfkDestroy()
{
	rd_kafka_topic_destroy(kfktopic);
	rd_kafka_destroy(kfkt);
}

void ZooKafkaPut::changeKafkaBrokers(const std::string& brokers)
{
	common::MutexLockGuard lock(kfkLock);
	kfkBrokers.clear();
	kfkBrokers = brokers;
	rd_kafka_brokers_add(kfkt, brokers.c_str());
	rd_kafka_poll(kfkt, 10);
	return;
}

zhandle_t* ZooKafkaPut::initialize_zookeeper(const char * zookeeper, const int debug)
{
	zhandle_t *zh = NULL;
	if (debug)
	{
		zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
	}
	
	zh = zookeeper_init(zookeeper,
		watcher,
		10000, NULL, this, 0);
	if (zh == NULL)
	{
		PERROR("Zookeeper connection not established.\n");
		return NULL;
	}
	return zh;
}
	
}

