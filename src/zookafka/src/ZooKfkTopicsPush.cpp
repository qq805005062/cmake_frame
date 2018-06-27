
#include "../ZooKfkCommon.h"
#include "../ZooKfkTopicsPush.h"

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
	PDEBUG("rdkafka-%d-%s: %s: %s", level, fac, rdk ? rd_kafka_name(rdk) : NULL, buf);
}

static int set_brokerlist_from_zookeeper(zhandle_t *zzh, char *brokers)
{
	int ret = 0;
	if (zzh)
	{
		struct String_vector brokerlist;
		if (zoo_get_children(zzh, KafkaBrokerPath, 1, &brokerlist) != ZOK)
		{
			PERROR("Zookeeper No brokers found on path %s", KafkaBrokerPath);
			return ret;
		}

		int i;
		char *brokerptr = brokers;
		for (i = 0; i < brokerlist.count; i++)
		{
			char path[255] = {0}, cfg[1024] = {0};
			sprintf(path, "/brokers/ids/%s", brokerlist.data[i]);
			PDEBUG("Zookeeper brokerlist path :: %s",path);
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
						PDEBUG("Zookeeper brokerptr value :: %s",brokerptr);
						
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
		PDEBUG("Zookeeper Found brokers:: %s",brokers);
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
			PDEBUG("Zookeeper Found brokers:: %s",brokers);
			ZooKfkTopicsPush *pZooKafkaPush = static_cast<ZooKfkTopicsPush *>(watcherCtx);
			pZooKafkaPush->changeKafkaBrokers(brokers);
			//rd_kafka_brokers_add(rk, brokers);
			//rd_kafka_poll(rk, 10);
		}
	}
}

static void msgDelivered(rd_kafka_t *rk, const rd_kafka_message_t* message, void *opaque)
{
	PDEBUG("deliver: %s: offset %ld", rd_kafka_err2str(message->err), message->offset);
	ZooKfkTopicsPush *pKfkPush = static_cast<ZooKfkTopicsPush *>(opaque);
	CALLBACKMSG msg;
	msg.offset = message->offset;
	msg.topic = rd_kafka_topic_name(message->rkt);
	msg.msg = const_cast<const char* >(static_cast<char* >(message->payload));
	msg.msgLen = message->len;
	msg.key = const_cast<const char* >(static_cast<char* >(message->key));
	msg.keyLen = message->key_len;
	msg.errorCode = message->err;
	msg.errMsg = rd_kafka_err2str(message->err);
	pKfkPush->msgPushWriteCall(message->_private, &msg);
	if (message->err)
	{
		// 这里可以写失败的处理
		PERROR("%% Message delivery failed error msg: %s", rd_kafka_err2str(message->err));
		pKfkPush->msgPushErrorCall(message->_private, &msg);
	}
	else
	{
		#ifdef SHOW_DEBUG_MESSAGE
		// 发送成功处理，这里只是示范，一般可以不处理成功的，只处理失败的就OK
		std::string data, key;
		data.assign(const_cast<const char* >(static_cast<char* >(message->payload)), message->len);
		key.assign(const_cast<const char* >(static_cast<char* >(message->key)), message->key_len);
		PDEBUG("%% success callback (%zd bytes, offset %ld, partition %d) msg: %s, key:%s",
		      message->len, message->offset, message->partition, data.c_str(), key.c_str());
		#else
		return;
		#endif
	}
}

ZooKfkTopicsPush::ZooKfkTopicsPush()
	:zKeepers()
	,zookeeph(nullptr)
	,kfkBrokers()
	,kfkt(nullptr)
	,topicPtrMap()
	,cb_(nullptr)
	,wcb_(nullptr)
	,kfkErrorCode(RD_KAFKA_RESP_ERR_NO_ERROR)
	,kfkErrorMsg()
	,destroy(0)
	,pushNum(0)
{
	PDEBUG("ZooKfkTopicsPush init");
}

ZooKfkTopicsPush::~ZooKfkTopicsPush()
{
	PDEBUG("~ZooKfkTopicsPush exit");
}

int ZooKfkTopicsPush::zookInit(const std::string& zookeepers)
{
	int ret = 0;
	char brokers[1024] = {0};
	/////////////////////////
	if(zookeeph)
	{
		PERROR("initialize_zookeeper init already");
		return KAFKA_INIT_ALREADY;
	}
	zookeeph = initialize_zookeeper(zookeepers.c_str(), 1);
	if(zookeeph == NULL)
	{
		PERROR("initialize_zookeeper new error");
		return KAFKA_MODULE_NEW_ERROR;
	}
	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	////////////////////////////////////////////////
	if(ret <= 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d",ret);
		return NO_KAFKA_BROKERS_FOUND;
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
			  int queueBuffMaxMs,
			  int queueBuffMaxMess)
{
	int ret = 0;
	char brokers[1024] = {0};
	/////////////////////////
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
	////////////////////////////////////////////////
	if(ret <= 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d",ret);
		return NO_KAFKA_BROKERS_FOUND;
	}
	
	//zKeepers.clear();
	zKeepers.assign(zookeepers);
	//kfkBrokers.clear();
	kfkBrokers.assign(brokers,strlen(brokers));
	ret = kfkInit(kfkBrokers, topics, queueBuffMaxMs, queueBuffMaxMess);
	if(ret < 0)
	{
		PERROR("kfkInit error :: %d",ret);
	}
	return ret;
}

int ZooKfkTopicsPush::kfkInit(const std::string& brokers,
			  const std::string& topics,
			  int queueBuffMaxMs,
			  int queueBuffMaxMess)
{
	char tmp[64] = {0},errStr[512] = {0};
	int ret = 0;
	PDEBUG("librdkafka version:: %s",rd_kafka_version_str());
	if(topicPtrMap.empty() == false)
	{
		PERROR("initialize_zookeeper init already");
		return KAFKA_INIT_ALREADY;
	}
	rd_kafka_conf_t* kfkconft = rd_kafka_conf_new();
	rd_kafka_conf_set_log_cb(kfkconft, kfkLogger);
	rd_kafka_conf_set_opaque(kfkconft,this);
	rd_kafka_conf_set_dr_msg_cb(kfkconft, msgDelivered);
	
	snprintf(tmp, sizeof tmp, "%i", SIGIO);
	rd_kafka_conf_set(kfkconft, "internal.termination.signal", tmp, NULL, 0);
	
	memset(tmp,0,64);
	snprintf(tmp, sizeof(tmp), "%d", queueBuffMaxMess);
	rd_kafka_conf_set(kfkconft, "queue.buffering.max.messages", tmp, NULL, 0);

	memset(tmp,0,64);
	snprintf(tmp, sizeof(tmp), "%d", queueBuffMaxMs);
	rd_kafka_conf_set(kfkconft, "queue.buffering.max.ms", tmp, NULL, 0);
	
	rd_kafka_conf_set(kfkconft, "message.send.max.retries", "3", NULL, 0);
	rd_kafka_conf_set(kfkconft, "retry.backoff.ms", "100", NULL, 0);
	rd_kafka_conf_set(kfkconft, "socket.blocking.max.ms", "50", NULL, 0);
	rd_kafka_conf_set(kfkconft, "queue.buffering.max.kbytes", "4096", NULL, 0);

	kfkt = rd_kafka_new(RD_KAFKA_PRODUCER, kfkconft, errStr, sizeof errStr);
	if(!kfkt)
	{
		PERROR("***Failed to create new producer: %s***", errStr);
		return KAFKA_MODULE_NEW_ERROR;
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
		PERROR("*** No valid brokers specified: %s ***", brokers.c_str());
		return KAFKA_BROKERS_ADD_ERROR;
	}
	
	std::vector<std::string> topics_;
	str2Vec(topics.c_str(), topics_, ',');
	if(topics_.size() < 1)
	{
		PERROR("topics ERROR :: %s",topics.c_str());
		return KAFKA_NO_TOPIC_NAME_INIT;
	}
	int size = static_cast<int>(topics_.size());
	for(int i = 0;i < size;i++)
	{
		rd_kafka_topic_conf_t *pTopiConf = rd_kafka_topic_conf_new();
		if(!pTopiConf)
		{
			PERROR("rd_kafka_topic_conf_new ERROR");
			return KAFKA_TOPIC_CONF_NEW_ERROR;
		}
		rd_kafka_topic_conf_set(pTopiConf, "produce.offset.report", "true", errStr, sizeof(errStr));
		rd_kafka_topic_conf_set(pTopiConf, "request.required.acks", "1", errStr, sizeof(errStr));

		rd_kafka_topic_t *pTopic = rd_kafka_topic_new(kfkt, topics_[i].c_str(), pTopiConf);
		if(!pTopic)
		{
			PERROR("rd_kafka_topic_new ERROR");
			return KAFKA_TOPIC_NEW_ERROR;
		}

		topicPtrMap.insert(KfkTopicPtrMap::value_type(topics_[i],pTopic));
	}
	ret = 0;
	return ret;
}

int ZooKfkTopicsPush::push(const std::string& topic,
			 const std::string& data,
			 std::string* key,
			 void *msgPri,
	         int partition,
	         int msgFlags)
{
	int ret = 0;
	if(data.empty() || topic.empty() || topicPtrMap.empty())
	{
		PERROR("push parameter no enought");
		ret = TRANSMIT_PARAMTER_ERROR;
		return ret;
	}

	KfkTopicPtrMapIter iter = topicPtrMap.find(topic);
	if(iter == topicPtrMap.end())
	{
		PERROR("target topic had't been init any way and can't push any data");
		ret = PUSH_TOPIC_NAME_NOINIT;
		return ret;
	}

	if(destroy)
	{
		ret = MODULE_RECV_EXIT_COMMND;
		return ret;
	}
	pushNum++;
	while(1)
	{
		ret = rd_kafka_produce(iter->second,
		                       partition,
		                       msgFlags,
		                       reinterpret_cast<void* >(const_cast<char* >(data.c_str())),
		                       data.size(),
		                       key == NULL ? NULL : reinterpret_cast<const void* >(key->c_str()),
		                       key == NULL ? 0 : key->size(),
		                       msgPri);

		if (ret < 0)
		{
			PERROR("*** Failed to produce to topic %s partition %d: %s *** %d",
			      topic.c_str(),
			      partition,
			      rd_kafka_err2str(rd_kafka_last_error()), ret);
			if(RD_KAFKA_RESP_ERR__QUEUE_FULL == rd_kafka_last_error())
			{
				bolckFlush();
			}else{
				setKfkErrorMessage(rd_kafka_last_error(),rd_kafka_err2str(rd_kafka_last_error()));
				break;
			}
		}else
		{
			break;
		}
	}
	//rd_kafka_poll(kfkt, 100);
	int waitTimer = 0;
	while(rd_kafka_outq_len(kfkt) > 50)
	{
		rd_kafka_poll(kfkt, 1);
		waitTimer++;
		if(waitTimer >= 3)
			break;
	}
	pushNum--;
	ret = rd_kafka_outq_len(kfkt);
	return ret;
}

int ZooKfkTopicsPush::bolckFlush(int queueSize)
{
	if(queueSize <= 0)
	{
		int queueLen = rd_kafka_outq_len(kfkt);
		if(queueLen > 0)
		{
			queueSize = queueLen / 2;
		}else{
			queueSize = 0;
		}
	}
	while(rd_kafka_outq_len(kfkt) > queueSize)
		rd_kafka_poll(kfkt, 50);
	return 0;
}

void ZooKfkTopicsPush::kfkDestroy()
{
	destroy = 1;
	while(pushNum)
		usleep(500);
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
	
	//rd_kafka_wait_destroyed(2000);
	zookeeper_close(zookeeph);
	zookeeph = NULL;

	KfkTopicPtrMap ().swap(topicPtrMap);
	if(cb_)
		cb_ = nullptr;
	if(wcb_)
		wcb_ = nullptr;

	std::string ().swap(zKeepers);
	std::string ().swap(kfkBrokers);
}

void ZooKfkTopicsPush::changeKafkaBrokers(const std::string& brokers)
{
//	kfkBrokers.clear();
	kfkBrokers.assign(brokers);
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
		PERROR("Zookeeper connection not established");
		return NULL;
	}
	return zh;
}

bool ZooKfkTopicsPush::str2Vec(const char* src, std::vector<std::string>& dest, const char delim)
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

	char *pChar = pSrc, *qChar = utilFristConstchar(pChar,delim);
	while(qChar)
	{
		PDEBUG("str2Vec :: curr :: %s",pChar);
		*qChar = 0;
		dest.push_back(pChar);
		pChar = ++qChar;
		qChar = utilFristConstchar(pChar,delim);
	}
	PDEBUG("str2Vec :: curr :: %s",pChar);
	dest.push_back(pChar);
	delete[] pSrc;
	return true;
}

void ZooKfkTopicsPush::setKfkErrorMessage(rd_kafka_resp_err_t code,const char *msg)
{
	kfkErrorCode = code;
	kfkErrorMsg.assign(msg);
}

int ZooKfkProducers::zooKfkProducersInit(int produceNum, const std::string& zookStr, const std::string& topicStr)
{
	int ret = 0;
	if(kfkProducerNum)
		return KAFKA_INIT_ALREADY;
	kfkProducerNum = produceNum;
	for(int i = 0; i < kfkProducerNum; i++)
	{
		ZooKfkProducerPtr producer(new ZOOKEEPERKAFKA::ZooKfkTopicsPush());
		if(!producer)
		{
			PERROR("New ZooKfkProducerPtr point error");
			ret = KAFKA_MODULE_NEW_ERROR;
			return ret;
		}

		ret = producer->zookInit(zookStr, topicStr);
		if(ret < 0)
		{
			PERROR("producer->zookInit error ret : %d", ret);
			return ret;
		}
		ZooKfkProducerPtrVec.push_back(producer);
	}
	return ret;
}

void ZooKfkProducers::zooKfkProducersDestroy()
{
	for(int i = 0; i < kfkProducerNum; i++)
	{
		if(ZooKfkProducerPtrVec[i])
			ZooKfkProducerPtrVec[i]->kfkDestroy();
	}
}

int ZooKfkProducers::setMsgPushErrorCall(const MsgPushCallBack& cb)
{
	int ret = KAFKA_NO_INIT_ALREADY;
	for(int i = 0; i < kfkProducerNum; i++)
	{
		if(ZooKfkProducerPtrVec[i])
		{
			ret++;
			ZooKfkProducerPtrVec[i]->setMsgPushErrorCall(cb);
		}else{
			ret = KAFKA_UNHAPPEN_ERRPR;
		}
	}
	return ret;
}

int ZooKfkProducers::setMsgPushCallBack(const MsgPushCallBack& cb)
{
	int ret = KAFKA_NO_INIT_ALREADY;
	for(int i = 0; i < kfkProducerNum; i++)
	{
		if(ZooKfkProducerPtrVec[i])
		{
			ret++;
			ZooKfkProducerPtrVec[i]->setMsgPushCallBack(cb);
		}else{
			ret = KAFKA_UNHAPPEN_ERRPR;
		}
	}
	return ret;
}

int ZooKfkProducers::psuhKfkMsg(const std::string& topic, const std::string& msg, std::string& errorMsg, std::string* key)
{
	int ret = KAFKA_UNHAPPEN_ERRPR;
	if(kfkProducerNum == 0)
	{
		ret = KAFKA_NO_INIT_ALREADY;
		return ret;
	}
	unsigned int index = lastIndex++;
	int sunindex = index % kfkProducerNum;
	if(ZooKfkProducerPtrVec[sunindex])
	{
		ret = ZooKfkProducerPtrVec[sunindex]->push(topic, msg, key);
		if(ret < 0)
		{
			ret = ZooKfkProducerPtrVec[sunindex]->getLastErrorMsg(errorMsg);
		}
	}
	return ret;
}


}

