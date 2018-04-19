
#include "../ZooKfkCommon.h"
#include "../ZooKafkaGet.h"

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
	,kfkt(nullptr)
	,topicpar(nullptr)
	,kMessageMaxSize(0)
	,partitions_()
{
	PDEBUG("ZooKafkaGet struct\n");
}

ZooKafkaGet::~ZooKafkaGet()
{
	kfkDestroy();
	PDEBUG("ZooKafkaGet exit\n");
}

int ZooKafkaGet::zookInit(const std::string& zookeepers,
			  const std::string& topic,
			  const std::string& groupId,
			  const std::vector<int>& partitions,
	          int64_t startOffset,
	          size_t messageMaxSize)
{
	int ret = 0;
	char brokers[1024] = {0};

	zookeeph = initialize_zookeeper(zookeepers.c_str(), 1);
	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	if(ret < 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d\n",ret);
		return ret;
	}

	zKeepers.assign(zookeepers);
	kfkBrokers.assign(brokers,strlen(brokers));
	ret = kfkInit(kfkBrokers,topic,groupId,partitions,startOffset,messageMaxSize);
	if(ret < 0)
	{
		PERROR("kfkInit error :: %d\n",ret);
	}
	return ret;
}

int ZooKafkaGet::kfkInit(const std::string& brokers,
			  const std::string& topic,
			  const std::string& groupId,
			  const std::vector<int>& partitions,
	          int64_t startOffset,
	          size_t messageMaxSize)
{
	char errStr[512] = { 0 },tmp[16] = { 0 };
	kMessageMaxSize = messageMaxSize;

	rd_kafka_conf_t* kfkconft = rd_kafka_conf_new();

	rd_kafka_conf_set_log_cb(kfkconft, kfkLogger);

	snprintf(tmp, sizeof tmp, "%i", SIGIO);
	rd_kafka_conf_set(kfkconft, "internal.termination.signal", tmp, NULL, 0);
	rd_kafka_conf_set(kfkconft, "queued.min.messages", "1000000", NULL, 0);
	rd_kafka_conf_set(kfkconft, "session.timeout.ms", "6000", NULL, 0);
	if (groupId.empty() || groupId != "")
	{
		if (RD_KAFKA_CONF_OK != rd_kafka_conf_set(kfkconft, "group.id", groupId.c_str(), errStr, sizeof errStr))
		{
			PERROR("set kafka config value failed, reason: %s\n", errStr);
			return -1;
		}
	}

	rd_kafka_topic_conf_t* kfktopiconft = rd_kafka_topic_conf_new();
	// rd_kafka_topic_conf_set(kfktopiconft, "auto.commit.enable", "true", errStr, sizeof(errStr));
	// rd_kafka_topic_conf_set(kfktopiconft, "auto.commit.interval.ms", "2000", errStr, sizeof(errStr));
	// rd_kafka_topic_conf_set(kfktopiconft, "offset.store.path", filePath.c_str(), errStr, sizeof(errStr));
	// rd_kafka_topic_conf_set(kfktopiconft, "offset.store.method", "file", errStr, sizeof(errStr));
	// rd_kafka_topic_conf_set(kfktopiconft, "offset.store.sync.interval.ms", "2000", errStr, sizeof(errStr));
	rd_kafka_topic_conf_set(kfktopiconft, "auto.offset.reset", "smallest", NULL, 0);
	if (rd_kafka_topic_conf_set(kfktopiconft, "offset.store.method", "broker", errStr, sizeof errStr) != RD_KAFKA_CONF_OK)
	{
		PERROR("set offset store method failed: %s\n", errStr);
		return -1;
	}

	rd_kafka_conf_set_default_topic_conf(kfkconft, kfktopiconft);
	kfkt = rd_kafka_new(RD_KAFKA_CONSUMER, kfkconft, errStr, sizeof errStr);
	if (NULL == kfkt)
	{
		PERROR("Failed to create new consumer, reason: %s\n", errStr);
		return -1;
	}

	if (rd_kafka_brokers_add(kfkt, brokers.c_str()) == 0)
	{
		PERROR("no valid brokers specified, brokers: %s\n", errStr);
		return -1;
	}

	rd_kafka_poll_set_consumer(kfkt);

	int size = static_cast<int>(partitions.size());
	topicpar = rd_kafka_topic_partition_list_new(size);

	for (int i = 0; i < size; i++)
	{
		PDEBUG("rd_kafka_topic_partition_list_add :: %d\n",partitions[i]);
		rd_kafka_topic_partition_list_add(topicpar, topic.c_str(), partitions[i]);
	}

	rd_kafka_resp_err_t err = rd_kafka_subscribe(kfkt, topicpar);
	if (err)
	{
		PERROR("Failed to start consuming topics: %s\n", rd_kafka_err2str(err));
		return -1;
	}
	return 0;
}

//rd_kafka_err2str(rd_kafka_errno2err(errno))
int ZooKafkaGet::get(std::string& data, int64_t* offset,std::string* key)
{
	rd_kafka_message_t* message = NULL;
	while(1)
	{
		message = rd_kafka_consumer_poll(kfkt, 1000);
		if(message == NULL)
			continue;
		else if(message->err)
		{
			PERROR("get data error,reason: %d -- %s\n", message->err, rd_kafka_err2str(message->err));
			rd_kafka_message_destroy(message);
		}else
			break;
	}

	if (static_cast<size_t>(message->len) > kMessageMaxSize)
	{
		PERROR("message size too large,discarded!\n");
		rd_kafka_message_destroy(message);
		return -1;
	}
	
	// 取消息体
	data.assign(const_cast<const char* >(static_cast<char* >(message->payload)), message->len);
	if(offset)
		*offset = message->offset;
	if (key)
	{
		key->assign(const_cast<const char* >(static_cast<char* >(message->key)), message->key_len);
	}

	PDEBUG("len: %zu, partition: %d, offset: %ld\n", message->len, message->partition, message->offset);
	rd_kafka_message_destroy(message);
	return 0;
}

void ZooKafkaGet::kfkDestroy()
{
	rd_kafka_resp_err_t err = rd_kafka_consumer_close(kfkt);
	if (err)
	{
		PERROR("failed to close consumer: %s\n", rd_kafka_err2str(err));
	}

	rd_kafka_topic_partition_list_destroy(topicpar);

	// destroy handle
	rd_kafka_destroy(kfkt);

	int run = 5;
	while (run-- > 0 && rd_kafka_wait_destroyed(1000) == -1)
	{
		PDEBUG("Waiting for librdkafka to decommission\n");
	}

	if (run <= 0)
	{
		rd_kafka_dump(stdout, kfkt);
	}
}

void ZooKafkaGet::changeKafkaBrokers(const std::string& brokers)
{
	kfkBrokers.assign(brokers);
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

}

