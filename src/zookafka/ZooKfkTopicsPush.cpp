
#include <signal.h>

#include "ZooKfkTopicsPush.h"

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
			ZooKfkTopicsPush *pZooKafkaPush = static_cast<ZooKfkTopicsPush *>(watcherCtx);
			pZooKafkaPush->changeKafkaBrokers(brokers);
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
		// 这里可以写失败的处理
		PUSHERRORMSG msg;
		msg.topic = rd_kafka_topic_name(message->rkt);
		msg.msg = const_cast<const char* >(static_cast<char* >(message->payload));
		msg.msgLen = message->len;
		msg.key = const_cast<const char* >(static_cast<char* >(message->key));
		msg.keyLen = message->key_len;
		msg.errorCode = message->err;
		msg.errMsg = rd_kafka_err2str(message->err);
		PERROR("%% Message delivery failed: %s\n", rd_kafka_err2str(message->err));
		ZooKfkTopicsPush *pKfkPush = static_cast<ZooKfkTopicsPush *>(opaque);
		pKfkPush->msgPushErrorCall(&msg);
	}
	else
	{
		// 发送成功处理，这里只是示范，一般可以不处理成功的，只处理失败的就OK
		std::string data, key;
		data.assign(const_cast<const char* >(static_cast<char* >(message->payload)), message->len);
		key.assign(const_cast<const char* >(static_cast<char* >(message->key)), message->key_len);

		PDEBUG("%% success callback (%zd bytes, offset %ld, partition %d) msg: %s, key:%s\n",
		      message->len, message->offset, message->partition, data.c_str(), key.c_str());
	}
}

ZooKfkTopicsPush::ZooKfkTopicsPush()
	:zKeepers()
	,zookeeph(nullptr)
	,kfkBrokers()
	,kfkt(nullptr)
	,topicPtrMap()
	,cb_(nullptr)
{
	PDEBUG("ZooKfkTopicsPush struct\n");
}

ZooKfkTopicsPush::~ZooKfkTopicsPush()
{
	kfkDestroy();
	PDEBUG("ZooKfkTopicsPush exit\n");
}

int ZooKfkTopicsPush::zookInit(const std::string& zookeepers)
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
	ret = 0;
	return ret;
}

int ZooKfkTopicsPush::zookInit(const std::string& zookeepers,
			  const std::string& topics,
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
	ret = kfkInit(kfkBrokers,topics,maxMsgqueue);
	if(ret < 0)
	{
		PERROR("kfkInit error :: %d\n",ret);
	}
	return ret;
}

int ZooKfkTopicsPush::kfkInit(const std::string& brokers,
			  const std::string& topics,
			  int maxMsgqueue)
{
	char tmp[64] = {0},errStr[512] = {0};
	int ret = 0;
	rd_kafka_conf_t* kfkconft = rd_kafka_conf_new();
	rd_kafka_conf_set_log_cb(kfkconft, kfkLogger);
	rd_kafka_conf_set_opaque(kfkconft,this);
	rd_kafka_conf_set_dr_msg_cb(kfkconft, msgDelivered);
	snprintf(tmp, sizeof tmp, "%i", SIGIO);
	rd_kafka_conf_set(kfkconft, "internal.termination.signal", tmp, NULL, 0);
	snprintf(tmp, sizeof(tmp), "%d", maxMsgqueue);
	rd_kafka_conf_set(kfkconft, "queue.buffering.max.messages", tmp, NULL, 0);
	rd_kafka_conf_set(kfkconft, "message.send.max.retries", "3", NULL, 0);
	rd_kafka_conf_set(kfkconft, "retry.backoff.ms", "500", NULL, 0);

	kfkt = rd_kafka_new(RD_KAFKA_PRODUCER, kfkconft, errStr, sizeof errStr);
	if(!kfkt)
	{
		PERROR("***Failed to create new producer: %s***\n", errStr);
		return -1;
	}
	rd_kafka_set_log_level(kfkt, KFK_LOG_DEBUG);

	if(brokers.empty())
	{
		ret = rd_kafka_brokers_add(kfkt, kfkBrokers.c_str());
	}else{
		ret = rd_kafka_brokers_add(kfkt, brokers.c_str());
		if(kfkBrokers.empty())
			kfkBrokers = brokers;
	}
	if (ret == 0)
	{
		PERROR("*** No valid brokers specified: %s ***\n", brokers.c_str());
		return -1;
	}
	
	std::vector<std::string> topics_;
	str2Vec(topics.c_str(), topics_, ",");
	if(topics_.size() < 1)
	{
		PERROR("topics ERROR :: %s\n",topics.c_str());
		return -1;
	}
	int size = static_cast<int>(topics_.size());
	for(int i = 0;i < size;i++)
	{
		rd_kafka_topic_conf_t *pTopiConf = rd_kafka_topic_conf_new();
		if(!pTopiConf)
		{
			PERROR("rd_kafka_topic_conf_new ERROR\n");
			return -1;
		}
		rd_kafka_topic_conf_set(pTopiConf, "produce.offset.report", "true", errStr, sizeof(errStr));
		rd_kafka_topic_conf_set(pTopiConf, "request.required.acks", "1", errStr, sizeof(errStr));

		rd_kafka_topic_t *pTopic = rd_kafka_topic_new(kfkt, topics_[i].c_str(), pTopiConf);
		if(!pTopic)
		{
			PERROR("rd_kafka_topic_new ERROR\n");
			return -1;
		}

		topicPtrMap.insert(KfkTopicPtrMap::value_type(topics_[i],pTopic));
	}
	
	return 0;
}

int ZooKfkTopicsPush::push(const std::string& topic,
			 const std::string& data,
	         std::string* key,
	         int partition,
	         int msgFlags)
{
	if(data.empty() || topic.empty())
	{
		PERROR();
		return -1;
	}

	if(topicPtrMap.empty())
	{
		PERROR();
		return -1;
	}

	KfkTopicPtrMapIter iter = topicPtrMap.find(topic);
	if(iter == topicPtrMap.end())
	{
		PERROR();
		return -1;
	}
	int ret = rd_kafka_produce(iter->second,
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
		      topic.c_str(),
		      partition,
		      rd_kafka_err2str(rd_kafka_last_error()));

		rd_kafka_poll(kfkt, 0);
		return ret;
	}

	PDEBUG("*** Push %lu bytes to topic:%s partition: %i***\n",
	      data.size(),
	      topic.c_str(),
	      partition);
	//rd_kafka_poll(kfkt, 1000);
	rd_kafka_poll(kfkt, 0);
	return 0;
}

void ZooKfkTopicsPush::kfkDestroy()
{
	while(rd_kafka_outq_len(kfkt) > 0)
	{
		rd_kafka_poll(kfkt, 100);
	}

	for(KfkTopicPtrMapIter iter = topicPtrMap.begin();iter != topicPtrMap.end();iter++)
	{
		rd_kafka_topic_destroy(iter->second);
	}

	KfkTopicPtrMap ().swap(topicPtrMap);
	rd_kafka_destroy(kfkt);
}

void ZooKfkTopicsPush::changeKafkaBrokers(const std::string& brokers)
{
	kfkBrokers.clear();
	kfkBrokers = brokers;
	rd_kafka_brokers_add(kfkt, brokers.c_str());
	rd_kafka_poll(kfkt, 10);
	return;
}

zhandle_t* ZooKfkTopicsPush::initialize_zookeeper(const char * zookeeper, const int debug)
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

bool ZooKfkTopicsPush::str2Vec(const char* src, std::vector<std::string>& dest, const char* delim)
{
	if (NULL == src || delim == NULL)
	{
		return false;
	}

	char* tmp  = const_cast<char* >(src);
	char* curr = strtok(tmp, delim);
	while (curr)
	{
		PDEBUG("str2Vec :: curr :: %s\n",curr);
		dest.push_back(curr);
		curr = strtok(NULL, delim);
	}
	return true;
}

}

