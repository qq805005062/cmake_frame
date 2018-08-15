
#include <unistd.h>
#include <inttypes.h>
#include "../ZooKfkCommon.h"
#include "../ZooKfkTopicsPop.h"

#pragma GCC diagnostic ignored "-Wconversion"

namespace ZOOKEEPERKAFKA
{
static const char KafkaBrokerPath[] = "/brokers/ids";
static int zookeeperColonyNum = 0;
static int assign_cnt = 0;

static void kfkLogger(const rd_kafka_t* rdk, int level, const char* fac, const char* buf)
{
	PDEBUG("rdkafka-%d-%s: %s: %s", level, fac, rdk ? rd_kafka_name(rdk) : NULL, buf);
}

static void rebalance_cb (rd_kafka_t *rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t *partitions, void *opaque) {

	char *memberid = rd_kafka_memberid(rk);
	PDEBUG("%s: MemberId \"%s\": Consumer group rebalanced: %s\n", rd_kafka_name(rk), memberid, rd_kafka_err2str(err));

	if (memberid)
		free(memberid);

	for (int i = 0 ; i < partitions->cnt ; i++) {
		PDEBUG(" %s [%d] offset %ld %s %s\n",
			 partitions->elems[i].topic,
			 partitions->elems[i].partition,
			 partitions->elems[i].offset,
			 partitions->elems[i].err ? ": " : "",
			 partitions->elems[i].err ?
			 rd_kafka_err2str(partitions->elems[i].err) : "");
		}

	switch (err)
	{
		case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
			assign_cnt++;
			rd_kafka_assign(rk, partitions);
			break;

		case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
			if (assign_cnt == 0)
				PDEBUG("asymetric rebalance_cb\n");
			assign_cnt--;
			rd_kafka_assign(rk, NULL);
			break;

		default:
			PDEBUG("rebalance failed: %s\n",
				  rd_kafka_err2str(err));
			break;
	}
}


static int set_brokerlist_from_zookeeper(zhandle_t *zzh, char *brokers)
{
	int ret = 0,tryTime = 0;
	if (zzh)
	{
		struct String_vector brokerlist;
		do{
			#if 1
			ret = zoo_get_children(zzh, KafkaBrokerPath, 1, &brokerlist);
			#else
			struct Stat nodes;
			ret = zoo_get_children2(zzh, KafkaBrokerPath, 1, &brokerlist, &nodes);
			PDEBUG("zoo_get_children2 %d nodes.czxid %lu", ret, nodes.czxid);
			#endif
			if(ret != ZOK)
			{
				PERROR("Zookeeper No brokers found on path %s error %d %s %d", KafkaBrokerPath, ret, zerror(ret), zoo_state(zzh));
				if(ZCONNECTIONLOSS == ret)
					tryTime++;
				else
					return ret;
			}else{
				break;
			}
		}while(tryTime < zookeeperColonyNum);
		PDEBUG("tryTime %d", tryTime);
		if(ret != ZOK)
		{
			PERROR("Zookeeper No brokers found on path %s error %d %s %d", KafkaBrokerPath, ret, zerror(ret), zoo_state(zzh));
			return ret;
		}

		int i;
		char *brokerptr = brokers;
		for (i = 0; i < brokerlist.count; i++)
		{
			char path[255] = {0}, cfg[1024] = {0};
			sprintf(path, "/brokers/ids/%s", brokerlist.data[i]);
			PDEBUG("brokerlist path :: %s",path);
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
						PDEBUG("brokerptr value :: %s",brokerptr);
						
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
		PDEBUG("Found brokers:: %s",brokers);
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
			PDEBUG("Found brokers:: %s",brokers);
			ZooKfkTopicsPop *pZooKafkaPop = static_cast<ZooKfkTopicsPop *>(watcherCtx);
			pZooKafkaPop->changeKafkaBrokers(brokers);
			//rd_kafka_brokers_add(rk, brokers);
			//rd_kafka_poll(rk, 10);
		}
	}
}

ZooKfkTopicsPop::ZooKfkTopicsPop()
	:listLock()
	,zKeepers()
	,zookeeph(nullptr)
	,kfkBrokers()
	,groupName_()
	,topics_()
	,kfkt(nullptr)
	,kMessageMaxSize( 4 * 1024 * 1024)
	,startOffset(static_cast<int64_t>(RD_KAFKA_OFFSET_INVALID))
	,partition(RD_KAFKA_PARTITION_UA)
	,kfkErrorCode(RD_KAFKA_RESP_ERR_NO_ERROR)
	,kfkErrorMsg()
	,destroy(0)
	,initFlag(0)
	,switchFlag(0)
	,errorFlag(0)
{
	PDEBUG("ZooKfkTopicsPop struct");
}

ZooKfkTopicsPop::~ZooKfkTopicsPop()
{
	PDEBUG("ZooKfkTopicsPop exit");
}

int ZooKfkTopicsPop::zookInit(const std::string& zookeepers)
{
	int ret = 0;
	char brokers[1024] = {0};
	
	if(zookeeph)
	{
		PERROR("initialize_zookeeper init already");
		return KAFKA_INIT_ALREADY;
	}
	zookeeph = initialize_zookeeper(zookeepers.c_str(), 0);
	if(zookeeph == NULL)
	{
		PERROR("initialize_zookeeper new error");
		return KAFKA_MODULE_NEW_ERROR;
	}
	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	if(ret <= 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d",ret);
		return NO_KAFKA_BROKERS_FOUND;
	}

	zKeepers = zookeepers;
	kfkBrokers.assign(brokers);
	ret = 0;
	return ret;
}

int ZooKfkTopicsPop::zookInit(const std::string& zookeepers, const std::string& topic, const std::string& groupName)
{
	int ret = 0;
	char brokers[1024] = {0};

	if(zookeeph)
	{
		PERROR("initialize_zookeeper init already");
		return KAFKA_INIT_ALREADY;
	}
	zookeeph = initialize_zookeeper(zookeepers.c_str(), 0);
	if(zookeeph == NULL)
	{
		PERROR("initialize_zookeeper new error");
		return KAFKA_MODULE_NEW_ERROR;
	}
	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	if(ret <= 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d",ret);
		return NO_KAFKA_BROKERS_FOUND;
	}

	zKeepers = zookeepers;
	kfkBrokers.assign(brokers);
	ret = kfkInit(kfkBrokers, topic, groupName);
	if(ret < 0)
	{
		PERROR("kfkInit error :: %d",ret);
	}
	return ret;
}

int ZooKfkTopicsPop::kfkInit(const std::string& brokers, const std::string& topic, const std::string& groupName)
{
	char errStr[512] = { 0 },tmp[16] = { 0 };
	int ret = 0;
	PDEBUG("librdkafka version:: %s",rd_kafka_version_str());

	std::lock_guard<std::mutex> lock(listLock);
	if(initFlag)
		return KAFKA_INIT_ALREADY;
	initFlag = 1;
	rd_kafka_conf_t* kfkconft = rd_kafka_conf_new();
	if(kfkconft == NULL)
	{
		PERROR("rd_kafka_conf_new NULL");
		return KAFKA_MODULE_NEW_ERROR;
	}
	rd_kafka_conf_set_log_cb(kfkconft, kfkLogger);
	rd_kafka_conf_set_rebalance_cb(kfkconft, rebalance_cb);
	
	snprintf(tmp, sizeof tmp, "%i", SIGIO);
	rd_kafka_conf_set(kfkconft, "internal.termination.signal", tmp, NULL, 0);
	rd_kafka_conf_set(kfkconft, "queued.min.messages", "1000000", NULL, 0);
	rd_kafka_conf_set(kfkconft, "session.timeout.ms", "6000", NULL, 0);
	
	if(brokers.empty() || brokers.length() == 0)
	{
		ret = rd_kafka_conf_set(kfkconft, "metadata.broker.list",kfkBrokers.c_str(), errStr, sizeof errStr);
	}else{
		ret = rd_kafka_conf_set(kfkconft, "metadata.broker.list",brokers.c_str(), errStr, sizeof errStr);
		kfkBrokers.assign(brokers);
	}
	if (ret)
	{
		PERROR("rd_kafka_conf_set error: %s", errStr);
		return KAFKA_CONSUMER_CONFSET_ERROR;
	}
	if(!groupName.empty())
	{
		PERROR("kfkInit groupName: %s", groupName.c_str());
		groupName_.assign(groupName);
		ret = rd_kafka_conf_set(kfkconft, "group.id", groupName.c_str(), errStr, sizeof errStr);
		if(ret != RD_KAFKA_CONF_OK)
		{
			PERROR("set kafka config value failed, reason: %s", errStr);
			return KAFKA_CONSUMER_CONFSET_ERROR;
		}
	}
	rd_kafka_topic_conf_t* kfktopiconft = rd_kafka_topic_conf_new();
	rd_kafka_topic_conf_set(kfktopiconft, "auto.commit.enable", "false", errStr, sizeof(errStr));
	//rd_kafka_topic_conf_set(kfktopiconft, "auto.commit.interval.ms", "200", errStr, sizeof(errStr));
	rd_kafka_topic_conf_set(kfktopiconft, "auto.offset.reset", "largest", errStr, sizeof(errStr));
	// rd_kafka_topic_conf_set(kfktopiconft, "offset.store.path", filePath.c_str(), errStr, sizeof(errStr));
	// rd_kafka_topic_conf_set(kfktopiconft, "offset.store.method", "file", errStr, sizeof(errStr));
	// rd_kafka_topic_conf_set(kfktopiconft, "offset.store.sync.interval.ms", "2000", errStr, sizeof(errStr));
	// rd_kafka_topic_conf_set(kfktopiconft, "auto.offset.reset", "smallest", NULL, 0);
	if (rd_kafka_topic_conf_set(kfktopiconft, "offset.store.method", "broker", errStr, sizeof errStr) != RD_KAFKA_CONF_OK)
	{
		PERROR("set offset store method failed: %s", errStr);
		return KAFKA_CONSUMER_CONFSET_ERROR;
	}
	rd_kafka_conf_set_default_topic_conf(kfkconft, kfktopiconft);
	
	kfkt = rd_kafka_new(RD_KAFKA_CONSUMER, kfkconft, errStr, sizeof errStr);
	if (NULL == kfkt)
	{
		PERROR("Failed to create new consumer, reason: %s", errStr);
		return KAFKA_MODULE_NEW_ERROR;
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
		return KAFKA_BROKERS_ADD_ERROR;
	}
	rd_kafka_poll_set_consumer(kfkt);

	if(!topic.empty())
	{
		std::vector<std::string> topics;
		str2Vec(topic.c_str(), topics, ',');
		if(topics.size() < 1)
		{
			PERROR("topics ERROR :: %s",topic.c_str());
			return TRANSMIT_PARAMTER_ERROR;
		}
		int size = static_cast<int>(topics.size());
		rd_kafka_topic_partition_list_t* topicparlist = rd_kafka_topic_partition_list_new(size);
		if(!topicparlist)
		{
			PERROR("rd_kafka_topic_partition_list_new ERROR NULL ptr");
			return KAFKA_TOPIC_CONF_NEW_ERROR;
		}

		for (int i = 0; i < size; i++)
		{
			PDEBUG("rd_kafka_topic_partition_list_add topic :: %s",topics[i].c_str());
			rd_kafka_topic_partition_list_add(topicparlist, topics[i].c_str(), partition);
			topics_.push_back(topics[i]);
		}

		rd_kafka_resp_err_t err = rd_kafka_subscribe(kfkt, topicparlist);
		if (err)
		{
			PERROR("Failed to start consuming topics: %s", rd_kafka_err2str(err));
			rd_kafka_topic_partition_list_destroy(topicparlist);
			return KAFKA_CONSUMER_ADDTOPIC_ERROR;
		}
		rd_kafka_topic_partition_list_destroy(topicparlist);
	}
	ret = 0;
	return ret;
}

int ZooKfkTopicsPop::kfkTopicConsumeStart(const std::string& topic)
{
	if(errorFlag)
		return errorFlag;
	switchFlag = 1;

	std::lock_guard<std::mutex> lock(listLock);
	for(ListStringTopicIter iter = topics_.begin();iter != topics_.end();iter++)
	{
		int ret = iter->compare(topic);
		if(ret == 0)
		{
			PDEBUG("kfkTopicConsumeStart topic alread in there");
			switchFlag = 0;
			return 0;
		}
	}
	
	int size = static_cast<int>(topics_.size());
	size++;
	rd_kafka_topic_partition_list_t *pList = rd_kafka_topic_partition_list_new(size);
	if(!pList)
	{
		PERROR("rd_kafka_topic_partition_list_new ERROR NULL ptr");
		switchFlag = 0;
		errorFlag = KAFKA_CONSUMER_ADDTOPIC_ERROR;
		return KAFKA_TOPIC_CONF_NEW_ERROR;
	}
	for(ListStringTopicIter iter = topics_.begin();iter != topics_.end();iter++)
	{
		PDEBUG("rd_kafka_topic_partition_list_add topic :: %s",iter->c_str());
		rd_kafka_topic_partition_list_add(pList, iter->c_str(), partition);
		//rd_kafka_topic_partition_list_set_offset(pList, iter->c_str(), partition,startOffset);
	}
	PDEBUG("rd_kafka_topic_partition_list_add topic :: %s",topic.c_str());
	rd_kafka_topic_partition_list_add(pList, topic.c_str(), partition);
	//rd_kafka_topic_partition_list_set_offset(pList, topic.c_str(), partition,startOffset);
	if(size > 1)
	{
		rd_kafka_resp_err_t err = rd_kafka_unsubscribe(kfkt);
		if(err)
		{
			PERROR("Failed rd_kafka_unsubscribe topics: %s", rd_kafka_err2str(err));
			switchFlag = 0;
			errorFlag = KAFKA_CONSUMER_ADDTOPIC_ERROR;
			rd_kafka_topic_partition_list_destroy(pList);
			return KAFKA_CONSUMER_ADDTOPIC_ERROR;
		}
	}
		
	rd_kafka_resp_err_t err = rd_kafka_subscribe(kfkt, pList);
	if (err)
	{
		PERROR("Failed rd_kafka_subscribe topics: %s", rd_kafka_err2str(err));
		ListStringTopic ().swap(topics_);
		switchFlag = 0;
		errorFlag = KAFKA_CONSUMER_ADDTOPIC_ERROR;
		rd_kafka_topic_partition_list_destroy(pList);
		return KAFKA_CONSUMER_ADDTOPIC_ERROR;
	}
	topics_.push_back(topic);
	rd_kafka_topic_partition_list_destroy(pList);
	switchFlag = 0;
	return errorFlag;
}

int ZooKfkTopicsPop::pop(std::string& topic, std::string& data, std::string* key, int64_t* offset, int32_t* parnum)
{
	int ret = 0;
	
	std::lock_guard<std::mutex> lock(listLock);
		
	if(destroy)
		return MODULE_RECV_EXIT_COMMND;
	if(errorFlag)
		return errorFlag;
	if(switchFlag)
		return 0;
	
	rd_kafka_message_t* message = NULL;
	while(1)
	{
		message = rd_kafka_consumer_poll(kfkt, 500);
		if(switchFlag)
		{
			if(message && message->err == RD_KAFKA_RESP_ERR__PARTITION_EOF)
			{
				rd_kafka_message_destroy(message);
				message = NULL;
			}
			break;
		}
		if(destroy)
		{
			if(message && message->err == RD_KAFKA_RESP_ERR__PARTITION_EOF)
			{
				rd_kafka_message_destroy(message);
				message = NULL;
			}
			break;
		}
		if(!message)
			continue;
		else if(message->err == RD_KAFKA_RESP_ERR__PARTITION_EOF)
		{
			rd_kafka_message_destroy(message);
			continue;
		}
		else
			break;
	}
	if(message)
	{
		if(message->err)
		{
			setKfkErrorMessage(message->err,rd_kafka_err2str(message->err));
			rd_kafka_message_destroy(message);
			ret = message->err;
		}else{
			const char *top = rd_kafka_topic_name(message->rkt);
			if(top)
			{
				topic.assign(top, strlen(top));
			}else{
				PERROR("message get from kafka no topic name");
			}
			data.assign(const_cast<const char* >(static_cast<char* >(message->payload)), message->len);
			
			if(offset)
			{
				*offset = message->offset;
			}
			if(parnum)
			{
				*parnum = message->partition;
			}
			if (key)
			{
				key->assign(const_cast<const char* >(static_cast<char* >(message->key)), message->key_len);
			}
			#if 0
			PDEBUG("partition %d , offset %ld", message->partition, message->offset);
			
			if(top)
			{
				PDEBUG("len: %zu,topic :: %s partition: %d, data %s ,offset: %ld", message->len, top, message->partition, static_cast<char *>(message->payload), message->offset);
			}else{
				PDEBUG("len: %zu, partition: %d, data %s ,offset: %ld", message->len, message->partition, static_cast<char *>(message->payload), message->offset);
			}
			#endif
			rd_kafka_message_destroy(message);
		}
	}
	
	return ret;
}

int ZooKfkTopicsPop::tryPop(std::string& topic, std::string& data, int timeout_ms, std::string* key, int64_t* offset, int32_t* parnum)
{
	int ret = 0;
	
	std::lock_guard<std::mutex> lock(listLock);
	if(destroy)
		return MODULE_RECV_EXIT_COMMND;
	if(errorFlag)
		return errorFlag;
	if(switchFlag)
		return 0;
	
	rd_kafka_message_t* message = NULL;
	message = rd_kafka_consumer_poll(kfkt, 500);
	if(message)
	{
		if(message->err)
		{
			if(message->err != RD_KAFKA_RESP_ERR__PARTITION_EOF)
			{
				setKfkErrorMessage(message->err,rd_kafka_err2str(message->err));
				ret = message->err;
			}
			rd_kafka_message_destroy(message);
		}else
		{
			ret++;
			const char *top = rd_kafka_topic_name(message->rkt);
			if(top)
			{
				topic.assign(top, strlen(top));
			}else{
				PERROR("message get from kafka no topic name");
			}
			data.assign(const_cast<const char* >(static_cast<char* >(message->payload)), message->len);
			if(offset)
			{
				*offset = message->offset;
			}
			if(parnum)
			{
				*parnum = message->partition;
			}
			if (key)
			{
				key->assign(const_cast<const char* >(static_cast<char* >(message->key)), message->key_len);
			}
			if(top)
			{
				PDEBUG("len: %zu,topic :: %s partition: %d, offset: %ld", message->len, top, message->partition, message->offset);
			}else{
				PDEBUG("len: %zu, partition: %d, offset: %ld", message->len, message->partition, message->offset);
			}
			rd_kafka_message_destroy(message);
		}
	}
	return ret;
}


int ZooKfkTopicsPop::kfkTopicConsumeStop(const std::string& topic)
{
	int size = static_cast<int>(topics_.size());
	if(size == 0)
	{
		PDEBUG("kfkTopicConsumeStop already stop");
		return 0;
	}
	if(errorFlag)
		return errorFlag;
	switchFlag = 1;

	std::lock_guard<std::mutex> lock(listLock);
	
	ListStringTopicIter iter = topics_.begin();
	for(;iter != topics_.end();iter++)
	{
		size = iter->compare(topic);
		if(size == 0)
		{
			break;
		}
	}
	if(size)
	{
		PERROR("There is no found topic in reading topic");
		switchFlag = 0;
		return TRANSMIT_PARAMTER_ERROR;
	}
	topics_.erase(iter);
	size = static_cast<int>(topics_.size());
	if(size == 0)
	{
		rd_kafka_resp_err_t err = rd_kafka_unsubscribe(kfkt);
		if(err)
		{
			PERROR("Failed rd_kafka_unsubscribe topics: %s", rd_kafka_err2str(err));
			topics_.push_back(topic);
			switchFlag = 0;
			errorFlag = KAFKA_CONSUMER_ADDTOPIC_ERROR;
			return KAFKA_CONSUMER_ADDTOPIC_ERROR;
		}
		switchFlag = 0;
		return size;
	}
	rd_kafka_topic_partition_list_t *pList = rd_kafka_topic_partition_list_new(size);
	if(pList == NULL)
	{
		PERROR("rd_kafka_topic_partition_list_new ERROR NULL ptr");
		topics_.push_back(topic);
		switchFlag = 0;
		errorFlag = KAFKA_CONSUMER_ADDTOPIC_ERROR;
		return KAFKA_MODULE_NEW_ERROR;
	}
	for(iter = topics_.begin();iter != topics_.end();iter++)
	{
		PDEBUG("rd_kafka_topic_partition_list_add topic :: %s",iter->c_str());
		rd_kafka_topic_partition_list_add(pList, iter->c_str(), partition);
		//rd_kafka_topic_partition_list_set_offset(pList, iter->c_str(), partition,startOffset);
	}

	rd_kafka_resp_err_t err = rd_kafka_unsubscribe(kfkt);
	if(err)
	{
		PERROR("Failed rd_kafka_unsubscribe topics: %s", rd_kafka_err2str(err));
		topics_.push_back(topic);
		switchFlag = 0;
		errorFlag = KAFKA_CONSUMER_ADDTOPIC_ERROR;
		rd_kafka_topic_partition_list_destroy(pList);
		return KAFKA_CONSUMER_ADDTOPIC_ERROR;
	}

	err = rd_kafka_subscribe(kfkt, pList);
	if (err)
	{
		PERROR("Failed rd_kafka_subscribe topics: %s", rd_kafka_err2str(err));
		ListStringTopic ().swap(topics_);
		switchFlag = 0;
		errorFlag = KAFKA_CONSUMER_ADDTOPIC_ERROR;
		rd_kafka_topic_partition_list_destroy(pList);
		return KAFKA_CONSUMER_ADDTOPIC_ERROR;
	}

	rd_kafka_topic_partition_list_destroy(pList);
	switchFlag = 0;
	return errorFlag;
}

void ZooKfkTopicsPop::changeKafkaBrokers(const std::string& brokers)
{
	//kfkBrokers.clear();
	kfkBrokers.assign(brokers);
	rd_kafka_brokers_add(kfkt, brokers.c_str());
	rd_kafka_poll(kfkt, 10);
	return;
}

void ZooKfkTopicsPop::kfkDestroy()
{
	destroy = 1;
	std::lock_guard<std::mutex> lock(listLock);
	
	rd_kafka_resp_err_t err = rd_kafka_consumer_close(kfkt);
	if (err)
	{
		PERROR("failed to close consumer: %s", rd_kafka_err2str(err));
	}

	if(kfkt)
	{
		rd_kafka_destroy(kfkt);
		kfkt = NULL;
	}

	//rd_kafka_wait_destroyed(2000);
	zookeeper_close(zookeeph);
	
	zookeeph = NULL;
	std::string ().swap(zKeepers);
	std::string ().swap(kfkBrokers);
	ListStringTopic ().swap(topics_);
	kMessageMaxSize = 4 * 1024 * 1024;
	startOffset = static_cast<int64_t>(RD_KAFKA_OFFSET_INVALID);
	partition = RD_KAFKA_PARTITION_UA;
}

void ZooKfkTopicsPop::kfkSubscription()
{
	rd_kafka_topic_partition_list_t *pList = NULL;
	if(kfkt)
	{
		rd_kafka_resp_err_t err = rd_kafka_subscription(kfkt, &pList);
		if(err)
		{
			setKfkErrorMessage(err, rd_kafka_err2str(err));
			PERROR("rd_kafka_subscription err %d %s", err, rd_kafka_err2str(err));
			return;
		}
		if(pList == NULL)
		{
			PERROR("kfkSubscription list null");
			return;
		}

		rd_kafka_topic_partition_t *pInfo = NULL;
		for(int i = 0; i < pList->cnt; i++)
		{
			if(pInfo)
			{
				pInfo++;
			}else{
				pInfo = pList->elems;
			}
			if(pInfo->topic)
			{
				PDEBUG("index %d topic %s partition %d offset %ld", i, pInfo->topic, pInfo->partition, pInfo->offset);
			}else{
				PDEBUG("index %d topic NULL", i);
			}
		}

		rd_kafka_topic_partition_list_destroy(pList);
	}
}

zhandle_t* ZooKfkTopicsPop::initialize_zookeeper(const char* zookeeper, const int debug)
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
		PERROR("Zookeeper connection not established.");
		return NULL;
	}

	const char *p = zookeeper;
	do
	{
		p++;
		zookeeperColonyNum++;
		p = utilFristConstchar(p, ',');
	}while(p && *p);
	
	return zh;
}

bool ZooKfkTopicsPop::str2Vec(const char* src, std::vector<std::string>& dest, const char delim)
{
	if (NULL == src)
	{
		return false;
	}
	
	int srcLen = strIntLen(src);
	srcLen++;
	char *pSrc = new char[srcLen];
	memset(pSrc,0,srcLen);
	srcLen--;
	memcpy(pSrc,src,srcLen);

	char *pChar = pSrc, *qChar = utilFristchar(pChar,delim);
	while(qChar)
	{
		PDEBUG("str2Vec :: curr :: %s",pChar);
		*qChar = 0;
		dest.push_back(pChar);
		pChar = ++qChar;
		qChar = utilFristchar(pChar,delim);
	}
	PDEBUG("str2Vec :: curr :: %s",pChar);
	dest.push_back(pChar);
	delete[] pSrc;
	return true;
}

void ZooKfkTopicsPop::setKfkErrorMessage(rd_kafka_resp_err_t code,const char *msg)
{
	kfkErrorCode = code;
	kfkErrorMsg.assign(msg);
}

int ZooKfkConsumers::zooKfkConsumerInit(int consumerNum, const std::string& zookStr, const std::string& topicStr,  const std::string& groupName)
{
	int ret = 0;
	if(kfkConsumerNum)
		return KAFKA_INIT_ALREADY;
	kfkConsumerNum = consumerNum;
	for(int i = 0; i < kfkConsumerNum; i++)
	{
		ZooKfkConsumerPtr consumer(new ZOOKEEPERKAFKA::ZooKfkTopicsPop());
		if(!consumer)
		{
			PERROR("New ZooKfkConsumerPtr point error");
			ret = KAFKA_MODULE_NEW_ERROR;
			return ret;
		}
		ret = consumer->zookInit(zookStr, topicStr, groupName);
		if(ret < 0)
		{
			PERROR("producer->zookInit error ret : %d", ret);
			return ret;
		}
		ZooKfkConsumerPtrVec.push_back(consumer);
	}
	
	return ret;
}

void ZooKfkConsumers::zooKfkConsumerDestroy()
{
	for(int i = 0; i < kfkConsumerNum; i++)
	{
		if(ZooKfkConsumerPtrVec.size() > 0 && ZooKfkConsumerPtrVec[i])
			ZooKfkConsumerPtrVec[i]->kfkDestroy();
	}
}

int ZooKfkConsumers::zooKfkConsumerStart(const std::string& topic)
{
	int ret = 0;
	for(int i = 0;i < kfkConsumerNum; i++)
	{
		if(ZooKfkConsumerPtrVec.size() && ZooKfkConsumerPtrVec[i])
			ret = ZooKfkConsumerPtrVec[i]->kfkTopicConsumeStart(topic);
		else
			ret = KAFKA_UNHAPPEN_ERRPR;
	}
	return ret;
}

int ZooKfkConsumers::zooKfkConsumerStop(const std::string& topic)
{
	int ret = 0;
	for(int i = 0;i < kfkConsumerNum; i++)
	{
		if(ZooKfkConsumerPtrVec.size() && ZooKfkConsumerPtrVec[i])
			ret = ZooKfkConsumerPtrVec[i]->kfkTopicConsumeStop(topic);
		else
			ret = KAFKA_UNHAPPEN_ERRPR;
	}
	return ret;
}

int ZooKfkConsumers::consume(int index, std::string& topic, std::string& data, std::string& errorMsg, std::string* key, int64_t* offset)
{
	int ret = 0;
	if(index >= kfkConsumerNum)
		return KAFKA_NO_INIT_ALREADY;
	if(ZooKfkConsumerPtrVec.size() && ZooKfkConsumerPtrVec[index])
	{
		ret = ZooKfkConsumerPtrVec[index]->pop(topic, data, key, offset);
		if(ret < 0)
			ret = ZooKfkConsumerPtrVec[index]->getLastErrorMsg(errorMsg);
	}
	else
		ret = KAFKA_UNHAPPEN_ERRPR;
		
	return ret;
}

int ZooKfkConsumers::tryConsume(int index, std::string& topic, std::string& data, int timeout_ms, std::string& errorMsg, std::string* key, int64_t* offset)
{
	int ret = 0;
	if(index >= kfkConsumerNum)
		return KAFKA_NO_INIT_ALREADY;
	if(ZooKfkConsumerPtrVec.size() && ZooKfkConsumerPtrVec[index])
	{
		ret = ZooKfkConsumerPtrVec[index]->tryPop(topic, data, timeout_ms, key, offset);
		if(ret < 0)
			ret = ZooKfkConsumerPtrVec[index]->getLastErrorMsg(errorMsg);
	}
	else
		ret = KAFKA_UNHAPPEN_ERRPR;
	return ret;
}


}

