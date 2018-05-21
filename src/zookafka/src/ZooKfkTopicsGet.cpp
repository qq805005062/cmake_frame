
#include "../ZooKfkCommon.h"
#include "../ZooKfkTopicsGet.h"

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
			ZooKfkTopicsGet *pZooKafkaGet = static_cast<ZooKfkTopicsGet *>(watcherCtx);
			pZooKafkaGet->changeKafkaBrokers(brokers);
			//rd_kafka_brokers_add(rk, brokers);
			//rd_kafka_poll(rk, 10);
		}
	}
}

ZooKfkTopicsGet::ZooKfkTopicsGet()
	:listLock()
	,zKeepers()
	,zookeeph(nullptr)
	,kfkBrokers()
	,kfkt(nullptr)
	,kMessageMaxSize( 4 * 1024 * 1024)
{
	PDEBUG("ZooKfkTopicsPop struct\n");
}

ZooKfkTopicsGet::~ZooKfkTopicsGet()
{
	kfkDestroy();
	PDEBUG("ZooKfkTopicsPop exit\n");
}

int ZooKfkTopicsGet::zookInit(const std::string& zookeepers)
{
	int ret = 0;
	char brokers[1024] = {0};
	
	if(zookeeph)
	{
		PERROR("zookeeper already init\n");
		return -1;
	}
	
	zookeeph = initialize_zookeeper(zookeepers.c_str(), 0);
	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	if(ret < 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d\n",ret);
		return ret;
	}

	zKeepers = zookeepers;
	kfkBrokers.append(brokers);
	ret = kfkInit(brokers);
	return ret;
}

int ZooKfkTopicsGet::kfkInit(const std::string& brokers)
{
	char errStr[512] = { 0 },tmp[16] = { 0 };
	int ret = 0;
	rd_kafka_conf_t* kfkconft = rd_kafka_conf_new();
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

	return 0;
}

int ZooKfkTopicsGet::kfkTopicConsumeStart(const std::string& topic, int partition, int64_t offset)
{
	char topicKeyBuf[64] = {0};
	std::lock_guard<std::mutex> lock(listLock);
	sprintf(topicKeyBuf,"%s,%d",topic.c_str(),partition);
	std::string topicKeyStr(topicKeyBuf);
	PDEBUG("kfkTopicConsumeStart topicKeyStr :: %s\n",topicKeyStr.c_str());
	KfkTopicPtrMapIter iter = topicPtrMap.find(topicKeyStr);
	
	if(iter == topicPtrMap.end())
	{
		rd_kafka_topic_conf_t *topic_conf = rd_kafka_topic_conf_new();
		if(topic_conf == NULL)
		{
			PERROR("rd_kafka_topic_conf_new topic :: %s conf new error\n",topic.c_str());
			return -1;
		}
		
		rd_kafka_topic_t* rkt = rd_kafka_topic_new(kfkt, topic.c_str(), topic_conf);
		if(rkt == NULL)
		{
			PERROR("rd_kafka_topic_new new topic :: %s error\n",topic.c_str());
			return -1;
		}

		if (rd_kafka_consume_start(rkt, partition, offset) == -1){
			PERROR("Failed to start topic :: %s consuming error :: %s\n", topic.c_str(), rd_kafka_err2str(rd_kafka_errno2err(errno)));
			return -1;
		}

		topicPtrMap.insert(KfkTopicPtrMap::value_type(topicKeyStr,rkt));
		return 0;
	}else{
		PDEBUG("kfkTopicConsumeStart start topic :: %s partition :: %d already in start map\n",topic.c_str(),partition);
		return 0;
	}
}

int ZooKfkTopicsGet::get(std::string& topic, std::string& data, int64_t* offset, int partition, std::string* key)
{
	rd_kafka_topic_t* rkt = NULL;
	rd_kafka_message_t *rkmessage = NULL;
	char topicKeyBuf[64] = {0};
	{
		std::lock_guard<std::mutex> lock(listLock);
		sprintf(topicKeyBuf,"%s,%d",topic.c_str(),partition);
		std::string topicKeyStr(topicKeyBuf);
		PDEBUG("kfkTopicConsumeStart topicKeyStr :: %s\n",topicKeyStr.c_str());
		KfkTopicPtrMapIter iter = topicPtrMap.find(topicKeyStr);
		if(iter == topicPtrMap.end())
		{
			PERROR("ZooKfkTopicsGet get topic :: %s partition :: %d no in start map\n",topic.c_str(),partition);
			return -1;
		}
		rkt = iter->second;
	}
	
	while(1)
	{
		rkmessage = rd_kafka_consume(rkt, partition, 100);
		if (!rkmessage) /* timeout */
			continue;
		else if(rkmessage->err)
		{
			PERROR("get data error, %d reason: %s\n", rkmessage->err, rd_kafka_err2str(rkmessage->err));
			rd_kafka_message_destroy(rkmessage);
		}else
			break;
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

	rd_kafka_message_destroy(rkmessage);
	return 0;
}

int ZooKfkTopicsGet::kfkTopicConsumeStop(const std::string& topic, int partition)
{

	char topicKeyBuf[64] = {0};
	std::lock_guard<std::mutex> lock(listLock);
	sprintf(topicKeyBuf,"%s,%d",topic.c_str(),partition);
	std::string topicKeyStr(topicKeyBuf);
	KfkTopicPtrMapIter iter = topicPtrMap.find(topicKeyStr);
	if(iter == topicPtrMap.end())
	{
		PERROR("kfkTopicConsumeStop stop topic :: %s partition :: %d no in start map\n",topic.c_str(),partition);
		return -1;
	}

	rd_kafka_consume_stop(iter->second, partition);
	rd_kafka_topic_destroy(iter->second);
	return 0;
}

void ZooKfkTopicsGet::changeKafkaBrokers(const std::string& brokers)
{
	//kfkBrokers.clear();
	kfkBrokers.assign(brokers);
	rd_kafka_brokers_add(kfkt, brokers.c_str());
	rd_kafka_poll(kfkt, 10);
	return;
}

void ZooKfkTopicsGet::kfkDestroy()
{
	std::lock_guard<std::mutex> lock(listLock);
	for(KfkTopicPtrMapIter iter = topicPtrMap.begin(); iter != topicPtrMap.end(); iter++)
	{
		const char *pChar = strchr(iter->first.c_str(),',');
		if(pChar)
		{
			pChar++;
			int partition = atoi(pChar);
			rd_kafka_consume_stop(iter->second, partition);
			rd_kafka_topic_destroy(iter->second);
		}
	}

	KfkTopicPtrMap ().swap(topicPtrMap);	
	rd_kafka_destroy(kfkt);

	rd_kafka_wait_destroyed(2000);
	zookeeper_close(zookeeph);
}

zhandle_t* ZooKfkTopicsGet::initialize_zookeeper(const char * zookeeper, const int debug)
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

bool ZooKfkTopicsGet::str2Vec(const char* src, std::vector<std::string>& dest, const char* delim)
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

