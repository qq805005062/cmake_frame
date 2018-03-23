
#include <signal.h>
#include "ZooKafkaGet.h"

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

static const char KafkaBrokerPath[] = "/brokers/ids",KfkConsumerGroup[] = "zookeeper";

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
			ZooKafkaGet *pZooKafkaGut = static_cast<ZooKafkaGet *>(watcherCtx);
			pZooKafkaGut->changeKafkaBrokers(brokers);
			//rd_kafka_brokers_add(rk, brokers);
			//rd_kafka_poll(rk, 10);
		}
	}
}

ZooKafkaGet::ZooKafkaGet()
	:zKeepers()
	,zookeeph(nullptr)
	,kfkBrokers()
	,groupId(KfkConsumerGroup)
	,kfkt(nullptr)
	,topicMap()
	,kfkconft(nullptr)
	,kfktopiconft(nullptr)
	,topicparlist(nullptr)
	,kMessageMaxSize( 4 * 1024 * 1024)
	,startOffset(static_cast<int64_t>(RD_KAFKA_OFFSET_END))
	,partition(RD_KAFKA_PARTITION_UA)
	,pause_(0)
{
	PDEBUG("ZooKafkaGet struct\n");
}

ZooKafkaGet::~ZooKafkaGet()
{
	kfkDestroy();
	PDEBUG("ZooKafkaGet exit\n");
}

int ZooKafkaGet::zookInit(const std::string& zookeepers)
{
	int ret = 0;
	char brokers[1024] = {0};
	
	if(zookeeph)
	{
		PERROR("zookeeper already init\n");
		return -1;
	}
	
	zookeeph = initialize_zookeeper(zookeepers.c_str(), 1);
	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	if(ret < 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d\n",ret);
		return ret;
	}

	zKeepers = zookeepers;
	kfkBrokers.append(brokers);
	ret = 0;
	return ret;
}

int ZooKafkaGet::zookInit(const std::string& zookeepers, const std::string& topic)
{
	int ret = 0;
	char brokers[1024] = {0};

	if(zookeeph)
	{
		PERROR("zookeeper already init\n");
		return -1;
	}
	
	zookeeph = initialize_zookeeper(zookeepers.c_str(), 1);
	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	if(ret < 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d\n",ret);
		return ret;
	}

	zKeepers = zookeepers;
	kfkBrokers.append(brokers);
	ret = kfkInit(kfkBrokers,topic);
	if(ret < 0)
	{
		PERROR("kfkInit error :: %d\n",ret);
	}
	return ret;
}

#if 0
int ZooKafkaGet::kfkInit(const std::string& brokers, const std::string& topic)
{
	char errStr[512] = { 0 },tmp[16] = { 0 };
	int ret = 0;
	kfkconft = rd_kafka_conf_new();
	rd_kafka_conf_set_log_cb(kfkconft, kfkLogger);

	snprintf(tmp, sizeof tmp, "%i", SIGIO);
	rd_kafka_conf_set(kfkconft, "internal.termination.signal", tmp, NULL, 0);
	rd_kafka_conf_set(kfkconft, "queued.min.messages", "1000000", NULL, 0);
	rd_kafka_conf_set(kfkconft, "session.timeout.ms", "6000", NULL, 0);
	if(brokers.empty() || brokers.length() == 0)
	{
		ret = rd_kafka_conf_set(kfkconft, "metadata.broker.list",kfkBrokers.c_str(), errStr, sizeof errStr);
		
	}else{
		ret = rd_kafka_conf_set(kfkconft, "metadata.broker.list",brokers.c_str(), errStr, sizeof errStr);
	}
	if (ret)
	{
		PERROR("rd_kafka_conf_set error: %s", errStr);
		return -1;
	}
	if (RD_KAFKA_CONF_OK != rd_kafka_conf_set(kfkconft, "group.id", KfkConsumerGroup, errStr, sizeof errStr))
	{
		PERROR("set kafka config value failed, reason: %s", errStr);
		return -1;
	}


	kfktopiconft = rd_kafka_topic_conf_new();
	// rd_kafka_topic_conf_set(kfktopiconft, "auto.commit.enable", "true", errStr, sizeof(errStr));

	// rd_kafka_topic_conf_set(kfktopiconft, "auto.commit.interval.ms", "2000", errStr, sizeof(errStr));

	// rd_kafka_topic_conf_set(kfktopiconft, "offset.store.path", filePath.c_str(), errStr, sizeof(errStr));

	// rd_kafka_topic_conf_set(kfktopiconft, "offset.store.method", "file", errStr, sizeof(errStr));

	// rd_kafka_topic_conf_set(kfktopiconft, "offset.store.sync.interval.ms", "2000", errStr, sizeof(errStr));
	rd_kafka_topic_conf_set(kfktopiconft, "auto.offset.reset", "smallest", NULL, 0);
	if (rd_kafka_topic_conf_set(kfktopiconft, "offset.store.method", "broker", errStr, sizeof errStr) != RD_KAFKA_CONF_OK)
	{
		PERROR("set offset store method failed: %s", errStr);
		return -1;
	}
	rd_kafka_conf_set_default_topic_conf(kfkconft, kfktopiconft);
	
	kfkt = rd_kafka_new(RD_KAFKA_CONSUMER, kfkconft, errStr, sizeof errStr);
	if (NULL == kfkt)
	{
		PERROR("Failed to create new consumer, reason: %s", errStr);
		return -1;
	}
	
	
	if(brokers.empty() || brokers.length() == 0)
	{
		ret = rd_kafka_brokers_add(kfkt, kfkBrokers.c_str());
	}else{
		ret = rd_kafka_brokers_add(kfkt, brokers.c_str());
	}
	if (ret == 0)
	{
		PERROR("no valid brokers specified, brokers: %s", errStr);
		return -1;
	}
	rd_kafka_poll_set_consumer(kfkt);

	std::vector<std::string> topics;
	str2Vec(topic.c_str(), topics, ",");
	if(topics.size() < 1)
	{
		PERROR("topics ERROR :: %s\n",topic.c_str());
		return -1;
	}
	int size = static_cast<int>(topics.size());
	topicparlist = rd_kafka_topic_partition_list_new(size);
	if(!topicparlist)
	{
		PERROR("rd_kafka_topic_partition_list_new ERROR NULL ptr\n");
		return -1;
	}

	for (int i = 0; i < size; i++)
	{
		PDEBUG("rd_kafka_topic_partition_list_add topic :: %s\n",topics[i].c_str());
		rd_kafka_topic_partition_list_add(topicparlist, topics[i].c_str(), partition);
	}

	rd_kafka_resp_err_t err = rd_kafka_subscribe(kfkt, topicparlist);
	if (err)
	{
		PERROR("Failed to start consuming topics: %s", rd_kafka_err2str(err));
		return -1;
	}
	return 0;
}
#else
int ZooKafkaGet::kfkInit(const std::string& brokers, const std::string& topic)
{
	char errStr[512] = { 0 };
	kfkconft = rd_kafka_conf_new();
	if(kfkconft == NULL)
	{
		PERROR("rd_kafka_conf_new ERROR\n");
		return -1;
	}
	
	rd_kafka_conf_set_log_cb(kfkconft, kfkLogger);

	int ret = 0;
	if(brokers.empty() || brokers.length() == 0)
	{
		ret = rd_kafka_conf_set(kfkconft, "metadata.broker.list",kfkBrokers.c_str(), errStr, sizeof errStr);
		
	}else{
		ret = rd_kafka_conf_set(kfkconft, "metadata.broker.list",brokers.c_str(), errStr, sizeof errStr);
	}
	if (ret)
	{
		PERROR("rd_kafka_conf_set error: %s", errStr);
		return -1;
	}
	kfkt = rd_kafka_new(RD_KAFKA_CONSUMER, kfkconft, errStr, sizeof errStr );
	if (NULL == kfkt)
	{
		PERROR("Failed to create new consumer, reason: %s", errStr);
		return -1;
	}
	
	std::vector<std::string> topics;
	str2Vec(topic.c_str(), topics, ",");
	if(topics.size() < 1)
	{
		PERROR("topics ERROR :: %s\n",topic.c_str());
		return -1;
	}
	int size = static_cast<int>(topics.size());
	for (int i = 0; i < size; i++)
	{
		PDEBUG("rd_kafka_topic_new :: %s\n",topics[i].c_str());
		kfktopiconft = rd_kafka_topic_conf_new();
		rd_kafka_topic_t* rkt = rd_kafka_topic_new(kfkt, topics[i].c_str(), kfktopiconft);
		topicMap.insert(KfkTopicPtrMap::value_type(topics[i],rkt));
		rd_kafka_consume_start(rkt, partition, startOffset);
	}
	return 0;
}
#endif
int ZooKafkaGet::kfkAddTopic(const std::string& topic)
{
	KfkTopicPtrMapIter iter = topicMap.find(topic);
	if(iter == topicMap.end())
	{
		rd_kafka_topic_t* rkt = rd_kafka_topic_new(kfkt, topic.c_str(), kfktopiconft);
		if(rkt)
		{
			topicMap.insert(KfkTopicPtrMap::value_type(topic,rkt));
		}else{
			PERROR("rd_kafka_topic_new error\n");
			return -1;
		}
		
	}
	return 0;
}

int ZooKafkaGet::kfkTopicConsumeStart(const std::string& topic)
{
	int ret = 0;
	KfkTopicPtrMapIter iter = topicMap.find(topic);
	if(iter == topicMap.end())
	{
		PERROR("%% Failed find %s rd_kafka_topic_new object\n",topic.c_str());
		ret = -1;
	}else{
		ret = rd_kafka_consume_start(iter->second, partition, startOffset);
		if(ret < 0)
		{
			PERROR("Failed to start consuming: %d\n",errno);
		}
	}
	return ret;
}

int ZooKafkaGet::get(std::string& topic, std::string& data, int64_t* offset, std::string* key)
{
	rd_kafka_message_t* message = NULL;
	while(1)
	{
		message = rd_kafka_consumer_poll(kfkt, 1000);
		if(!message)
			continue;
	}
	PDEBUG("------------------------------------------------------\n");
	if (message->err)
	{
		PERROR("get data error,reason: %s", rd_kafka_err2str(message->err));
		rd_kafka_message_destroy(message);
		return -1;
	}

	const char *top = rd_kafka_topic_name(message->rkt);
	if(top)
	{
		topic.assign(top, strlen(top));
	}else{
		PERROR("message get from kafka no topic name\n");
		return -1;
	}
	data.assign(const_cast<const char* >(static_cast<char* >(message->payload)), message->len);
	if(offset)
	{
		*offset = message->offset;
	}
	if (key)
	{
		key->assign(const_cast<const char* >(static_cast<char* >(message->key)), message->key_len);
	}

	PDEBUG("len: %zu, partition: %d, offset: %ld", message->len, message->partition, message->offset);
	rd_kafka_message_destroy(message);
	return 0;
}

int ZooKafkaGet::getFromTopicPar(const std::string& topic, std::string& data, int64_t* offset,std::string* key)
{
	KfkTopicPtrMapIter iter = topicMap.find(topic);
	if(iter == topicMap.end())
	{
		PERROR("The topic :: %s have't add to kafka read map\n",topic.c_str());
		return -1;
	}

	rd_kafka_message_t *rkmessage = NULL;
	while(1)
	{
		rkmessage = rd_kafka_consume(iter->second, partition, 1000);
		if (!rkmessage) /* timeout */
			continue;
	}
	
	if (rkmessage->err)
	{
		PERROR("getFromTopicPar data error,reason: %s", rd_kafka_err2str(rkmessage->err));
		rd_kafka_message_destroy(rkmessage);
		return -1;
	}
	
	data.assign(const_cast<const char* >(static_cast<char* >(rkmessage->payload)), rkmessage->len);
	if(offset)
	{
		*offset = rkmessage->offset;
	}
	if (key)
	{
		key->assign(const_cast<const char* >(static_cast<char* >(rkmessage->key)), rkmessage->key_len);
	}
	PDEBUG("len: %zu, partition: %d, offset: %ld", rkmessage->len, rkmessage->partition, rkmessage->offset);

	/* Return message to rdkafka */
	rd_kafka_message_destroy(rkmessage);
	return 0;
}

int ZooKafkaGet::kfkTopicConsumeStop(const std::string& topic)
{
	KfkTopicPtrMapIter iter = topicMap.find(topic);
	if(iter == topicMap.end())
	{
		PERROR("The topic :: %s have't add to kafka read map\n",topic.c_str());
		return -1;
	}
	rd_kafka_consume_stop(iter->second, partition);
	return 0;
}

void ZooKafkaGet::kfkDestroy()
{
	rd_kafka_resp_err_t err = rd_kafka_consumer_close(kfkt);
	if (err)
	{
		PERROR("failed to close consumer: %s", rd_kafka_err2str(err));
	}

	for(KfkTopicPtrMapIter iter = topicMap.begin();iter != topicMap.end();iter++)
	{
		rd_kafka_consume_stop(iter->second,RD_KAFKA_PARTITION_UA);
		rd_kafka_topic_destroy(iter->second);
	}
	KfkTopicPtrMap ().swap(topicMap);
	rd_kafka_destroy(kfkt);

	rd_kafka_wait_destroyed(2000);
	zookeeper_close(zookeeph);
}

void ZooKafkaGet::changeKafkaBrokers(const std::string& brokers)
{
	kfkBrokers.clear();
	kfkBrokers = brokers;
	rd_kafka_brokers_add(kfkt, brokers.c_str());
	rd_kafka_poll(kfkt, 10);
	return;
}

zhandle_t* ZooKafkaGet::initialize_zookeeper(const char * zookeeper, const int debug)
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

bool ZooKafkaGet::str2Vec(const char* src, std::vector<std::string>& dest, const char* delim)
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

