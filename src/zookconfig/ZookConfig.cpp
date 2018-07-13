#include <string.h>

#include "ZookConfig.h"

namespace ZOOKCONFIG
{
static int zookeeperColonyNum = 0;
static std::string rootDirect;

static void watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
	PDEBUG("watcher type %d",type);
	if (type == ZOO_CHILD_EVENT && strncmp(path, rootDirect.c_str(), rootDirect.length()) == 0)
	{
		const char* p_char = path;
		p_char += rootDirect.length() + 1;
		PDEBUG("watcher path key change:: %s %s", path, p_char);
		ZOOKCONFIG::ZookConfig *pZookConfig = static_cast<ZOOKCONFIG::ZookConfig *>(watcherCtx);
		pZookConfig->configUpdateCallBack(p_char);
	}
}

ZookConfig::ZookConfig()
	:zkHandle(nullptr)
	,configLock()
	,cb_()
	,configMap()
{
}

ZookConfig::~ZookConfig()
{
	zookeeper_close(zkHandle);
	zkHandle = NULL;
}

int ZookConfig::zookConfigInit(const std::string& zookAddr)
{
	int ret = initialize_zookeeper(zookAddr.c_str());
	PDEBUG("initialize_zookeeper ret %d ", ret);
	return ret;
}

int ZookConfig::zookLoadAllConfig(const std::string& path)
{
	rootDirect.assign(path);
	return loadAllKeyValue(path.c_str());
}

int ZookConfig::getConfigKeyValue(const std::string& key, std::string& value)
{
	int ret = -1;
	std::lock_guard<std::mutex> lock(configLock);
	ConfigMapDataIter iter = configMap.find(key);
	if(iter == configMap.end())
	{
		return ret;
	}
	value.assign(iter->second);
	ret = 0;
	return ret;
}

int ZookConfig::createSessionPath(const std::string& path, const std::string& value)
{
	int valueLen = 0, pathLen = 0, ret = 0, tryTime = 0;
	char *pathArray = NULL, *pBuff = NULL;
	valueLen = static_cast<int>(value.length());
	pathLen = static_cast<int>(path.length());
	pathLen++;
	pathArray = new char[pathLen];
	if(pathArray)
	{
		pBuff = pathArray;
		memset(pathArray, 0, pathLen);
		pathLen--;
		strncpy(pathArray, path.c_str(), pathLen);
		for(int i = 1; i < pathLen; i++)
		{
			pBuff++;
			if( *pBuff == '/' )
			{
				*pBuff = '\0';
				tryTime = 0;
				do{
					struct Stat stat;
					ret = zoo_exists(zkHandle, pathArray, 0, &stat);
					PDEBUG("zoo_exists %s %d %d", pathArray, ret, zoo_state(zkHandle));
					if(ZCONNECTIONLOSS == ret)
						tryTime++;
					else
						break;
				}while(tryTime < zookeeperColonyNum);

				if(ZNONODE == ret)
				{
					tryTime = 0;
					do{
						ret = zoo_create(zkHandle, pathArray, NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
						PDEBUG("zoo_create %s %d %d", pathArray, ret, zoo_state(zkHandle));
						if(ZCONNECTIONLOSS == ret)
							tryTime++;
						else
							break;
					}while(tryTime < zookeeperColonyNum);
				}else if(ret != ZOK)
				{
					PERROR("zoo_exists %s %d %s %d", pathArray, ret, zerror(ret), zoo_state(zkHandle));
					delete[] pathArray;
					return ret;
				}

				if(ret != ZOK)
				{
					PERROR("zoo_create %s %d %s %d", pathArray, ret, zerror(ret), zoo_state(zkHandle));
					delete[] pathArray;
					return ret;
				}

				*pBuff = '/';
			}
		}

		tryTime = 0;
		do{
			struct Stat stat;
			ret = zoo_exists(zkHandle, pathArray, 0, &stat);
			PDEBUG("zoo_exists %s %d %d", pathArray, ret, zoo_state(zkHandle));
			if(ZCONNECTIONLOSS == ret)
				tryTime++;
			else
				break;
		}while(tryTime < zookeeperColonyNum);

		if(ret == ZOK)
		{
			tryTime = 0;
			do{
				ret = zoo_set(zkHandle, pathArray, value.c_str(), valueLen, -1);
				PDEBUG("zoo_set %s %d %d", pathArray, ret, zoo_state(zkHandle));
				if(ZCONNECTIONLOSS == ret)
					tryTime++;
				else
					break;
			}while(tryTime < zookeeperColonyNum);
		}else if(ret == ZNONODE)
		{
			tryTime = 0;
			do{
				ret = zoo_create(zkHandle, pathArray, value.c_str(), valueLen, &ZOO_READ_ACL_UNSAFE, 1, NULL, 0);
				PDEBUG("zoo_create %s %d %d", pathArray, ret, zoo_state(zkHandle));
				if(ZCONNECTIONLOSS == ret)
					tryTime++;
				else
					break;
			}while(tryTime < zookeeperColonyNum);
		}else{
			PERROR("zoo_exists %s %d %s %d", pathArray, ret, zerror(ret), zoo_state(zkHandle));
			delete[] pathArray;
			return ret;
		}

		delete[] pathArray;
		return ret;
	}else{
		PDEBUG("createSessionPath %d", ZOOK_CONFIG_MALLOC_ERROR);
		return ZOOK_CONFIG_MALLOC_ERROR;
	}
}

void ZookConfig::configUpdateCallBack(const std::string& key)
{
	char path[255] = {0}, cfg[1024] = {0};
	sprintf(path, "%s/%s", rootDirect.c_str(), key.c_str());

	int len = sizeof(cfg);
	zoo_get(zkHandle, path, 0, cfg, &len, NULL);

	if (len > 0)
	{
		std::string oldValue, newValue(cfg);
		PDEBUG("configUpdateCallBack path :: %s value %s", path, cfg);
		{
			std::lock_guard<std::mutex> lock(configLock);
			ConfigMapDataIter iter = configMap.find(key);
			if(iter == configMap.end())
			{
				PDEBUG("configUpdateCallBack path :: %s value %s may be add will be abandon", path, cfg);
				return;
			}else{
				oldValue.assign(iter->second);
				iter->second.assign(cfg);
			}
		}
		cb_(key, oldValue, newValue);
	}else{
		PDEBUG("configUpdateCallBack path :: %s value empty", path);
	}
}

int ZookConfig::loadAllKeyValue(const char* rootPath)
{
	int ret = 0,tryTime = 0;
	if (zkHandle)
	{
		struct String_vector brokerlist;
		do{
			ret = zoo_get_children(zkHandle, rootPath, 1, &brokerlist);
			if(ret != ZOK)
			{
				PERROR("Zookeeper No found on path %s error %d %s %d", rootPath, ret, zerror(ret), zoo_state(zkHandle));
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
			PERROR("Zookeeper No found on path %s error %d %s %d", rootPath, ret, zerror(ret), zoo_state(zkHandle));
			return ret;
		}

		for (int i = 0; i < brokerlist.count; i++)
		{
			char path[255] = {0}, cfg[1024] = {0};
			sprintf(path, "%s/%s", path, brokerlist.data[i]);
			
			int len = sizeof(cfg);
			zoo_get(zkHandle, path, 0, cfg, &len, NULL);

			if (len > 0)
			{
				PDEBUG("loadAllKeyValue path :: %s value %s", path, cfg);
				std::string configKey(brokerlist.data[i]), configValue(cfg);
				std::lock_guard<std::mutex> lock(configLock);
				configMap.insert(ConfigMapData::value_type(configKey, configValue));
			}else{
				PDEBUG("loadAllKeyValue path :: %s value empty", path);
			}
		}
	}else{
		PDEBUG("loadAllKeyValue zkHandle no init");
		ret = ZOOK_CONFIG_NO_INIT;
	}

	return ret;
}

int ZookConfig::initialize_zookeeper(const char* zookeeper, const int debug)
{
	if(zkHandle)
		return ZOOK_CONFIG_ALREADY_INIT;
	
	if (debug)
	{
		zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
	}
	
	zkHandle = zookeeper_init(zookeeper,
		watcher,
		10000, NULL, this, 0);
	if (zkHandle == NULL)
	{
		PERROR("Zookeeper connection not established");
		return ZOOK_CONFIG_INIT_ERROR;
	}

	const char *p = zookeeper;
	do
	{
		p++;
		zookeeperColonyNum++;
		p = utilFristConstchar(p, ',');
	}while(p && *p);
	
	PDEBUG("initialize_zookeeper conn num %d", zoo_state(zkHandle));
	return 0;
}

const char* ZookConfig::utilFristConstchar(const char *str,const char c)
{
	const char *p = str;
	if(!str)
		return NULL;
	while(*p)
	{
		if(*p == c)
			return p;
		else
			p++;
	}
	return NULL;
}

int ZookConfigSingleton::zookConfigInit(const std::string& zookAddr, const std::string& path)
{
	if(configPoint)
		return ZOOK_CONFIG_ALREADY_INIT;

	configPoint.reset(new ZookConfig());
	if(configPoint == nullptr)
	{
		return ZOOK_CONFIG_MALLOC_ERROR;
	}

	int ret = configPoint->zookConfigInit(zookAddr);
	if(ret < 0)
		return ret;

	return ret;
}

int ZookConfigSingleton::createSessionPath(const std::string& path, const std::string& value)
{
	if(configPoint)
		return configPoint->createSessionPath(path,value);
	else
		return ZOOK_CONFIG_NO_INIT;
}

void ZookConfigSingleton::setConfigChangeCall(const ConfigChangeCall& cb)
{
	if(configPoint)
		configPoint->setConfigChangeCall(cb);
}

int ZookConfigSingleton::getConfigKeyValue(const std::string& key, std::string& value)
{
	if(configPoint)
		return configPoint->getConfigKeyValue(key,value);
	else
		return ZOOK_CONFIG_NO_INIT;
}

}

