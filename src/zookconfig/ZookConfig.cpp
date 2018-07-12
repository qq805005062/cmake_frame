#include <string.h>

#include "ZookConfig.h"

namespace ZookConfig
{
static int zookeeperColonyNum = 0;
static std::string rootDirect = "";

static void watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
	if (type == ZOO_CHILD_EVENT && strncmp(path, rootDirect.c_str(), rootDirect.length()) == 0)
	{
		const char* p_char = path;
		p_char += rootDirect.length() + 1;
		PDEBUG("watcher path key change:: %s %s", path, p_char);
		ZookConfig *pZookConfig = static_cast<ZookConfig *>(watcherCtx);
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

	return ret;
}

int ZookConfig::zookLoadAllConfig(const std::string& path)
{
	rootDirect.assign(path);
	return loadAllKeyValue(path.c_str());
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

}

