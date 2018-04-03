
#include "ZooKfkProducer.h"

namespace ZOOKEEPERKAFKA
{

static const char KafkaBrokerPath[] = "/brokers/ids";

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
			ZooKfkProducer *pZooKfkProducer = static_cast<ZooKfkProducer *>(watcherCtx);
			pZooKfkProducer->changeKafkaBrokers(brokers);
			//rd_kafka_brokers_add(rk, brokers);
			//rd_kafka_poll(rk, 10);
		}
	}
}

int ZooKfkProducer::zookInit(const std::string& zookeepers)
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

int ZooKfkProducer::zookInit(const std::string& zookeepers,
		const std::string& topics,
		MsgDeliveryError* errCb,
		MsgDelivery* cb)
{
	int ret = 0;
	char brokers[1024] = {0};
	/////////////////////////
	zookeeph = initialize_zookeeper(zookeepers.c_str(), 0);
	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	////////////////////////////////////////////////
	if(ret < 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d\n",ret);
		return ret;
	}
	
	zKeepers = zookeepers;
	ret = kfkInit(brokers,topics,errCb,cb);
	if(ret < 0)
	{
		PERROR("kfkInit error :: %d\n",ret);
	}
	return ret;
}


int ZooKfkProducer::kfkInit(const std::string brokers,
		const std::string topic,
		MsgDeliveryError* errCb,
		MsgDelivery* cb)
{
	m_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
	m_tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

	if (!m_conf || !m_tconf)
	{
		std::cout << "conf, t_conf init err" << std::endl;
		return enum_err_init_err;
	}
	if (topic.empty() && topic_.empty())
	{
		std::cout << "topic cant be null" << std::endl;
		return enum_err_topic_empty;
	}

	if (brokers.empty() && kfkBrokers.empty())
	{
		std::cout << "brokers cant be null" << std::endl;
		return enum_err_broker_empty;
	}
	
	std::string errstr;
	HashPartitionerCb hash_partitioner;

	// Create configuration objects
	RdKafka::Conf*& conf = m_conf;
	RdKafka::Conf*& tconf = m_tconf;

	// Set configuration properties
	if(brokers.empty())
	{
		conf->set("metadata.broker.list", kfkBrokers, errstr);
	}else{
		conf->set("metadata.broker.list", brokers, errstr);
		kfkBrokers = brokers;
	}
	if(errCb)
		m_cb.setDeliveryError(errCb);

	if(cb)
		m_cb.setDelivery(cb);

	// 事件回调
	conf->set("event_cb", static_cast<RdKafka::EventCb *>(&m_cb), errstr);

	/* Set delivery report callback */
	conf->set("dr_cb", static_cast<RdKafka::DeliveryReportCb *>(&m_cb), errstr);

	// Create producer using accumulated global configuration.
	if (!m_producer)
	{
		m_producer = RdKafka::Producer::create(conf, errstr);
		if (!m_producer)
		{
			std::cout << "Failed to create producer: " << errstr.c_str() << std::endl;
			return enum_err_create_producer;
		}
		std::cout << "% Created producer " << m_producer->name() << std::endl;
	}
	
	// Create topic handle.
	if (!m_topic)
	{
		if(topic.empty())
		{
			m_topic = RdKafka::Topic::create(m_producer, topic_, tconf, errstr);
			if (!m_topic) 
			{
				std::cout << "Failed to create topic: " << errstr.c_str() << std::endl;
				return enum_err_create_toppic;
			}
		}else{
			m_topic = RdKafka::Topic::create(m_producer, topic, tconf, errstr);
			if (!m_topic) 
			{
				std::cout << "Failed to create topic: " << errstr.c_str() << std::endl;
				return enum_err_create_toppic;
			}
			topic_ = topic;
		}
	}

	return 0;
}

int ZooKfkProducer::send(const std::string& msg,
		std::string& strerr,
		const std::string* key,
		void *msgOpaque,
		int32_t partition,
		int send_timeout_times,
		int send_wnd_size)
{
	if (!m_topic || !m_producer)
	{
		std::cout << "m_topic, m_producer init err" << std::endl;
		return enum_err_init_err;
	}

	// int32_t partition = RdKafka::Topic::PARTITION_UA;
	// Produce message
	RdKafka::ErrorCode resp = m_producer->produce(m_topic, partition,
		RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
		const_cast<char *>(msg.c_str()), msg.size(),
		key, msgOpaque);
	m_producer->poll(0); // void queue full, trigger callback
	if (resp != RdKafka::ERR_NO_ERROR)
	{
		strerr = RdKafka::err2str(resp);
		std::cout << "send failed: " << strerr.c_str() << std::endl;
		return enum_err_send_err;
	}

	// 短暂等待
	int MAC_CNT = send_timeout_times;// 5;
	int cnt = MAC_CNT;
	while (cnt && m_producer->outq_len() > send_wnd_size)//0) 
	{
		cnt--;
		std::cerr << "Waiting for " << m_producer->outq_len() << std::endl;
		m_producer->poll(10 * (MAC_CNT - cnt));
	}

	// 窗口满了
	if (m_producer->outq_len() > send_wnd_size)
		return -100;
	return 0;
}

int ZooKfkProducer::set(const std::string& attr, const std::string& sValue, std::string& errstr)
{
	if (!m_conf)
		return enum_err_set_INVALID;

	return m_conf->set(attr.c_str(), sValue.c_str(), errstr);
}

void ZooKfkProducer::changeKafkaBrokers(const std::string& brokers)
{
	if (m_producer)
	{
		delete m_producer;
		m_producer = NULL;
	}

	if (m_topic)
	{
		delete m_topic;
		m_topic = NULL;
	}

	std::string top;
	kfkInit(brokers,top);
	return;
}

zhandle_t* ZooKfkProducer::initialize_zookeeper(const char * zookeeper, const int debug)
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

