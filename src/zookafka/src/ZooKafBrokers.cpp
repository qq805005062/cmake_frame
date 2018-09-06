#include <rapidjson/document.h>

#include "../ZooKfkCommon.h"
#include "../ZooKafBrokers.h"

namespace ZOOKEEPERKAFKA
{
static const char KafkaBrokerPath[] = "/brokers/ids";
static int zookeeperColonyNum = 0;

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
			PDEBUG("Zookeeper brokerlist path :: %s",path);
			int len = sizeof(cfg);
			zoo_get(zzh, path, 0, cfg, &len, NULL);

			if(len > 0)
			{
				cfg[len] = '\0';
				rapidjson::Document doc;
				doc.Parse<0>(cfg, len);
				if(doc.HasParseError())
				{
					continue;
				}
				if(doc.IsObject())
				{
					std::string jHost = "";
					int jPort = 0;
					if(doc.HasMember("host") && doc["host"].IsString())
					{
						jHost = doc["host"].GetString();
					}

					if(doc.HasMember("port") && doc["port"].IsInt())
					{
						jPort = doc["code"].GetInt();
					}

					if(jHost.length() && jPort)
					{
						ret++;
						sprintf(brokerptr, "%s:%d", jHost.c_str(), jPort);
						PDEBUG("Zookeeper brokerptr value :: %s",brokerptr);
						brokerptr += strlen(brokerptr);
						if (i < brokerlist.count - 1)
						{
							*brokerptr++ = ',';
						}
					}
				}
			}
#if 0
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
#endif
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
			ZooKafBrokers *pZooKfkBroker = static_cast<ZooKafBrokers *>(watcherCtx);
			pZooKfkBroker->kfkBrokerChange(brokers);
		}else{
			PDEBUG("There is no foun any brokers in zookeeper and this is very dange");
			ZooKafBrokers *pZooKfkBroker = static_cast<ZooKafBrokers *>(watcherCtx);
			pZooKfkBroker->kfkBrokerChange("");
		}
	}
}

ZooKafBrokers::ZooKafBrokers()
	:zKeepers()
	,zookeeph(nullptr)
	,cb_(nullptr)
{
	PDEBUG("ZooKafBrokers init");
}

ZooKafBrokers::~ZooKafBrokers()
{
	PERROR("~ZooKafBrokers exit");
}

std::string ZooKafBrokers::zookInit(const std::string& zookeepers)
{
	int ret = 0;
	char brokers[1024] = {0};
	
	if(zookeeph)
	{
		PERROR("initialize_zookeeper init already");
		return "";
	}
	zKeepers.assign(zookeepers);
	zookeeph = initialize_zookeeper(zookeepers.c_str());
	if(zookeeph == NULL)
	{
		PERROR("initialize_zookeeper new error");
		return "";
	}

	ret = set_brokerlist_from_zookeeper(zookeeph, brokers);
	if(ret <= 0)
	{
		PERROR("set_brokerlist_from_zookeeper error :: %d",ret);
		return "";
	}

	std::string resultBro(brokers);
	return resultBro;
}

void ZooKafBrokers::kfkBrokerChange(const std::string& bros)
{
	if(cb_)
	{
		cb_(bros);
	}
}

zhandle_t* ZooKafBrokers::initialize_zookeeper(const char* zookeeper, int debug)
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

	const char *p = zookeeper;
	do
	{
		p++;
		zookeeperColonyNum++;
		p = utilFristConstchar(p, ',');
	}while(p && *p);
	
	return zh;
}


}

