
#include "../ZooKfkCommon.h"
#include "../ZooKfkProducer.h"

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
	zookeeph = initialize_zookeeper(zookeepers.c_str(), 0);
	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	////////////////////////////////////////////////
	if(ret < 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d\n",ret);
		return ret;
	}
	
	//zKeepers.clear();
	zKeepers.assign(zookeepers);
	//kfkBrokers.clear();
	//kfkBrokers.append(brokers);
	kfkBrokers.assign(brokers,strlen(brokers));
	ret = 0;
	return ret;
}

int ZooKfkProducer::zookInit(const std::string& zookeepers,
		const std::string& topics,
		int pNum,
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
	ret = kfkInit(brokers,topics,pNum,errCb,cb);
	if(ret < 0)
	{
		PERROR("kfkInit error :: %d\n",ret);
	}
	return ret;
}


int ZooKfkProducer::kfkInit(const std::string brokers,
		const std::string topic,
		int pNum,
		MsgDeliveryError* errCb,
		MsgDelivery* cb)
{
	RdKafka::Conf *m_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
	RdKafka::Conf *m_tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
	if (!m_conf || !m_tconf)
	{
		PERROR("conf, t_conf init err\n");
		return enum_err_init_err;
	}
	
	if (topic.empty() && topic_.empty())
	{
		PERROR("topic can't be null\n");
		return enum_err_topic_empty;
	}
	if (brokers.empty() && kfkBrokers.empty())
	{
		PERROR("brokers can't be null\n");
		return enum_err_broker_empty;
	}
	
	std::string errstr;
	//HashPartitionerCb hash_partitioner;
	// Set configuration properties
	if(brokers.empty())
	{
		m_conf->set("metadata.broker.list", kfkBrokers, errstr);
	}else{
		m_conf->set("metadata.broker.list", brokers, errstr);
		kfkBrokers = brokers;
	}
	
	if(errCb)
		m_cb.setDeliveryError(errCb);
	if(cb)
		m_cb.setDelivery(cb);

	// 事件回调
	m_conf->set("event_cb", static_cast<RdKafka::EventCb *>(&m_cb), errstr);
	/* Set delivery report callback */
	m_conf->set("dr_cb", static_cast<RdKafka::DeliveryReportCb *>(&m_cb), errstr);

	// Create producer using accumulated global configuration.
	for(int i = 0;i < pNum;i++)
	{
		RdKafka::Producer* m_producer = RdKafka::Producer::create(m_conf, errstr);
		if (!m_producer)
		{
			PERROR("Failed to create producer: %s\n",errstr.c_str());
			return enum_err_create_producer;
		}
		kfkProducerVect.push_back(m_producer);
		PDEBUG("Created producer %s\n",m_producer->name().c_str());
	}
	
	// Create topic handle.
	
	if(topic.empty())
	{
		for(int i = 0;i < pNum;i++)
		{
			RdKafka::Topic* m_topic = RdKafka::Topic::create(kfkProducerVect[i], topic_, m_tconf, errstr);
			if (!m_topic) 
			{
				PERROR("Failed to create topic: %s\n",errstr.c_str());
				return enum_err_create_toppic;
			}
			kfkTopicVevt.push_back(m_topic);
			PDEBUG("Created Topic %s\n",topic_.c_str());
		}
	}else{
		for(int i = 0;i < pNum;i++)
		{
			RdKafka::Topic* m_topic = RdKafka::Topic::create(kfkProducerVect[i], topic, m_tconf, errstr);
			if (!m_topic) 
			{
				PERROR("Failed to create topic: %s\n",errstr.c_str());
				return enum_err_create_toppic;
			}
			kfkTopicVevt.push_back(m_topic);
			PDEBUG("Created Topic %s\n",topic.c_str());
		}
		topic_ = topic;
	}
	producerSize = pNum;

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
	while(isBroChang)
	{
		usleep(100);
	}
	pushNow++;
	int index = lastIndex.incrementAndGet() % producerSize;
	if(!kfkProducerVect[index] || !kfkTopicVevt[index])
	{
		PERROR("RdKafka::Producer RdKafka::Topic empty\n");
		return enum_err_init_err;
	}

	// int32_t partition = RdKafka::Topic::PARTITION_UA;
	// Produce message
	RdKafka::ErrorCode resp = kfkProducerVect[index]->produce(kfkTopicVevt[index], partition,
		RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
		const_cast<char *>(msg.c_str()), msg.size(),
		key, msgOpaque);
	kfkProducerVect[index]->poll(0); // void queue full, trigger callback
	if (resp != RdKafka::ERR_NO_ERROR)
	{
		strerr = RdKafka::err2str(resp);
		PERROR("send failed : %s\n",strerr.c_str());
		return enum_err_send_err;
	}

	// 短暂等待
	int MAC_CNT = send_timeout_times;// 5;
	int cnt = MAC_CNT;
	while (cnt && kfkProducerVect[index]->outq_len() > send_wnd_size)//0) 
	{
		cnt--;
		PDEBUG("Waiting for : %d\n",kfkProducerVect[index]->outq_len());
		kfkProducerVect[index]->poll(10 * (MAC_CNT - cnt));
	}

	// 窗口满了
	if (kfkProducerVect[index]->outq_len() > send_wnd_size)
		return -100;
	pushNow--;
	return 0;
}

void ZooKfkProducer::changeKafkaBrokers(const std::string& brokers)
{
	isBroChang = 1;
	while(pushNow)
	{
		usleep(100);
	}
	for(int i = 0;i < producerSize;i++)
	{
		delete kfkProducerVect[i];
		kfkProducerVect[i] = NULL;
	}

	for(int i = 0;i < producerSize;i++)
	{
		delete kfkTopicVevt[i];
		kfkTopicVevt[i] = NULL;
	}

	std::vector<RdKafka::Producer*> ().swap(kfkProducerVect);
	std::vector<RdKafka::Topic*> ().swap(kfkTopicVevt);
	kfkInit(brokers,topic_,producerSize);
	isBroChang = 0;
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

