
#include "../ZooKfkCommon.h"
#include "../ZooKfkConsumer.h"

namespace ZOOKEEPERKAFKA
{

static const char KafkaBrokerPath[] = "/brokers/ids",KfkConsumerGroup[] = "zookeeper";

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
			ZooKfkConsumer *pZooKfkCon = static_cast<ZooKfkConsumer *>(watcherCtx);
			pZooKfkCon->changeKafkaBrokers(brokers);
		}
	}
}

int ZooKfkConsumer::zookInit(const std::string& zookeepers)
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
	ret = 0;
	return ret;
}

int ZooKfkConsumer::zookInit(const std::string& zookeepers,
		const std::string& topic,
		MsgConsumerCallback ccb,
		MsgErrorCallback ecb,
		int32_t partition,
		int64_t start_offset)
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
	ret = kfkInit(brokers,topic,ccb,ecb,partition,start_offset);
	if(ret < 0)
	{
		PERROR("kfkInit error :: %d\n",ret);
	}
	return ret;
}

int ZooKfkConsumer::kfkInit(const std::string& brokers,
		const std::string& topic,
		MsgConsumerCallback ccb,
		MsgErrorCallback ecb,
		int32_t partition,
		int64_t start_offset)
{
	RdKafka::Conf *m_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
	RdKafka::Conf *m_tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

	if (!m_conf || !m_conf)
	{
		PERROR("kfk conf and topic can't be null\n");
		return enum_err_init_err;
	}

	if (topic.empty() && topic_.empty())
	{
		PERROR("topic name can't be null\n");
		return enum_err_topic_empty;
	}

	if (brokers.empty() && kfkBrokers.empty())
	{
		PERROR("brokers name can't be null\n");
		return enum_err_broker_empty;
	}
	std::string errstr;
	if(brokers.empty())
	{
		if(m_conf->set("metadata.broker.list", kfkBrokers, errstr) != 0)
		{
			PERROR("kfk conf metadata.broker.list set error :: %s\n",errstr.c_str());
			return enum_err_init_err;
		}
	}else{
		if(m_conf->set("metadata.broker.list", brokers, errstr) != 0)
		{
			PERROR("kfk conf metadata.broker.list set error :: %s\n",errstr.c_str());
			return enum_err_init_err;
		}
		kfkBrokers = brokers;
	}
	
	if(m_conf->set("group.id", KfkConsumerGroup, errstr) != 0)
	{
		PERROR("kfk conf group.id set error :: %s\n",errstr.c_str());
		return enum_err_init_err;
	}

	if(m_consumer)
	{
		delete m_consumer;
		m_consumer = NULL;
	}
	m_consumer = RdKafka::Consumer::create(m_conf, errstr);
	if (!m_consumer)
	{
		PERROR("Failed to create consumer :: %s\n",errstr.c_str());
		return enum_err_create_consumer;
	}

	if(m_topic)
	{
		delete m_topic;
		m_topic = NULL;
	}
	if(topic.empty())
	{
		m_topic = RdKafka::Topic::create(m_consumer, topic_, m_tconf, errstr);
		if (!m_topic)
		{
			PERROR("Failed to create topic :: %s\n",errstr.c_str());
			return enum_err_create_toppic;
		}
	}else{
		m_topic = RdKafka::Topic::create(m_consumer, topic, m_tconf, errstr);
		if (!m_topic)
		{
			PERROR("Failed to create topic :: %s\n",errstr.c_str());
			return enum_err_create_toppic;
		}
		topic_ = topic;
	}
	if(ccb)
		MCb_ = ccb;
	if(ecb)
		ECb_ = ecb;
	partition_ = partition;
	offset_ = start_offset;
	return 0;
}

int ZooKfkConsumer::start()
{
	int ret = 0;
	bool initFlag = false;
	
	RdKafka::ErrorCode resp = m_consumer->start(m_topic, partition_, offset_);
	if (resp != RdKafka::ERR_NO_ERROR)
	{
		PERROR("Failed to start consumer :: %s\n",RdKafka::err2str(resp).c_str());
		return enum_err_start_err;
	}
	run = true;
	while(run)
	{
		RdKafka::Message *msg = m_consumer->consume(m_topic, partition_, 1000);
		switch (msg->err())
		{
			case RdKafka::ERR__TIMED_OUT:
				break;
			case RdKafka::ERR_NO_ERROR:
				/* Real message */
				if(MCb_)
				{
					if(msg->key())
					{
						MCb_(static_cast<const char *>(msg->payload()),msg->len(),msg->key()->c_str(),msg->key_len(),msg->offset());
					}else{
						MCb_(static_cast<const char *>(msg->payload()),msg->len(),NULL,0,msg->offset());
					}
				}
				break;
			case RdKafka::ERR__PARTITION_EOF:
				/* Last message */
				break;

			case RdKafka::ERR__UNKNOWN_TOPIC:
			case RdKafka::ERR__UNKNOWN_PARTITION:
				PERROR("Failed to consumer :: %s\n",msg->errstr().c_str());
				if(ECb_)
				{
					ECb_(msg->err(),msg->errstr());
				}
				run = false;
				ret = msg->err();
				break;
			default:
				/* Errors */
				PERROR("Failed to consumer :: %s\n",msg->errstr().c_str());
				if(ECb_)
				{
					ECb_(msg->err(),msg->errstr());
				}
				run = false;
				ret = msg->err();
				break;
		}
		delete msg;
		m_consumer->poll(0);

		while(broChange)
		{
			usleep(100);
			initFlag = true;
		}
		if(initFlag)
		{
			RdKafka::ErrorCode resp = m_consumer->start(m_topic, partition_, offset_);
			if (resp != RdKafka::ERR_NO_ERROR)
			{
				PERROR("Failed to start consumer :: %s\n",RdKafka::err2str(resp).c_str());
				return enum_err_start_err;
			}
			initFlag = false;
		}
	}
	return ret;
}

int ZooKfkConsumer::stop()
{
	run = false;
	m_consumer->stop(m_topic, partition_);
    //m_consumer->poll(1000);
    return 0;
}

void ZooKfkConsumer::changeKafkaBrokers(const std::string& brokers)
{
	broChange = true;
	m_consumer->stop(m_topic, partition_);
    m_consumer->poll(1000);
	delete m_consumer;
	m_consumer = NULL;
	delete m_topic;
	m_topic = NULL;
	std::string top;
	kfkInit(brokers,top);
	broChange = false;
}

zhandle_t* ZooKfkConsumer::initialize_zookeeper(const char * zookeeper, const int debug)
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

