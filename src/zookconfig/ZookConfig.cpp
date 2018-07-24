
#include <string.h>

#include "ZookConfig.h"

#ifdef SHOW_PRINTF_MESSAGE

#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)

#else

#define PDEBUG(fmt, args...)

#define PERROR(fmt, args...)

#endif


namespace ZOOKCONFIG
{

static void watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
	int ret = 0;
	ZOOKCONFIG::ZookConfig *pZookConfig = static_cast<ZOOKCONFIG::ZookConfig *>(watcherCtx);
	
	PDEBUG("watcher type %d state %d path %s",type, state, path);
	
	if (type == ZOO_CHILD_EVENT)
	{
		TcpServerTypeVector typeVector;
		ret = pZookConfig->getServerInfoRootPath(typeVector);
		if(ret <= 0)
		{
			PERROR("pZookConfig->getServerInfoRootPath ret %d", ret);
			return;
		}

		for(size_t i = 0;i < typeVector.size(); i++)
		{
			if(strncmp(path, typeVector[i].c_str(), typeVector[i].length()) == 0)
			{
				PDEBUG("watcher ZOO_CHILD_EVENT child change:: %s", path);
				pZookConfig->serverInfoChangeCallBack(path);
			}
		}
	}

	if(type == ZOO_CHANGED_EVENT)
	{
		std::string configPath;
		ret = pZookConfig->getConfigRootPath(configPath);
		if(ret <= 0)
		{
			PERROR("pZookConfig->getConfigRootPath ret %d", ret);
			return;
		}

		if(strncmp(path, configPath.c_str(), configPath.length()) == 0)
		{
			const char* p_char = path;
			p_char += configPath.length() + 1;
			PDEBUG("watcher ZOO_CHANGED_EVENT change:: %s %s", path, p_char);
			pZookConfig->configUpdateCallBack(p_char);
		}
	}
}

ZookConfig::ZookConfig()
	:colonyNum(0)
	,configRootPath()
	,serverPathVector()
	,zkHandle(nullptr)
	,configLock()
	,configCb()
	,serverCb()
	,configMap()
{
}

ZookConfig::~ZookConfig()
{
	if(zkHandle)
	{
		zookeeper_close(zkHandle);
		zkHandle = NULL;
	}
	ConfigMapData ().swap(configMap);
}

int ZookConfig::zookConfigInit(const std::string& zookAddr)
{
	int ret = initialize_zookeeper(zookAddr.c_str());
	PDEBUG("initialize_zookeeper ret %d ", ret);
	return ret;
}

int ZookConfig::zookLoadAllConfig(const std::string& configPath)
{
	if(configPath.empty())
		return ZOOK_CONFIG_PARAMETER_ERROR;

	if(!configRootPath.empty())
		return ZOOK_CONFIG_ALREADY_INIT;
	
	configRootPath.assign(configPath);
	return loadAllKeyValue(configPath.c_str());
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

int ZookConfig::getTcpServerListInfo(const std::string& serverPath, TcpServerInfoVector& infoList)
{
	if(serverPath.empty())
		return ZOOK_CONFIG_PARAMETER_ERROR;

	int pathcheck = zookeeperPathCheck(serverPath.c_str());
	if(pathcheck <= 0)
		return pathcheck;

	if(pathcheck == 1)
		return ZOOK_CONFIG_PARAMETER_ERROR;
	
	if (zkHandle)
	{
		for(size_t i = 0; i < serverPathVector.size(); i++)
		{
			if(serverPath.compare(serverPathVector[i]) == 0)
			{
				return ZOOK_SERVER_INFO_ALREADY_INIT;
			}
		}
		serverPathVector.push_back(serverPath);
		TcpServerInfoVector ().swap(infoList);

		return LoadTcpServerListInfo(serverPath.c_str(), infoList);
	}else{
		PERROR("loadAllKeyValue zkHandle no init");
		return ZOOK_CONFIG_NO_INIT;
	}
}

int ZookConfig::createSessionPath(const std::string& path, const std::string& value)
{
	int valueLen = 0, pathLen = 0, ret = 0, tryTime = 0, pathcheck = 0;
	char *pathArray = NULL, *pBuff = NULL;

	pathcheck = zookeeperPathCheck(path.c_str());
	if(pathcheck <= 0)
		return pathcheck;

	if(pathcheck == 1)
		return ZOOK_CONFIG_PARAMETER_ERROR;
	
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
					if(ret == ZOK)
						break;
					PERROR("zoo_exists %s %d %d", pathArray, ret, zoo_state(zkHandle));
					if(ZCONNECTIONLOSS == ret)
						tryTime++;
					else
						break;
				}while(tryTime < colonyNum);

				if(ZNONODE == ret)
				{
					tryTime = 0;
					do{
						ret = zoo_create(zkHandle, pathArray, NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
						if(ret == ZOK)
							break;
						PERROR("zoo_create %s %d %d", pathArray, ret, zoo_state(zkHandle));
						if(ZCONNECTIONLOSS == ret)
							tryTime++;
						else
							break;
					}while(tryTime < colonyNum);
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
			if(ret == ZOK)
				break;
			PERROR("zoo_exists %s %d %d", pathArray, ret, zoo_state(zkHandle));
			if(ZCONNECTIONLOSS == ret)
				tryTime++;
			else
				break;
		}while(tryTime < colonyNum);

		if(ret == ZOK)
		{
			tryTime = 0;
			do{
				ret = zoo_set(zkHandle, pathArray, value.c_str(), valueLen, -1);
				if(ret == ZOK)
					break;
				PERROR("zoo_set %s %d %d", pathArray, ret, zoo_state(zkHandle));
				if(ZCONNECTIONLOSS == ret)
					tryTime++;
				else
					break;
			}while(tryTime < colonyNum);
		}else if(ret == ZNONODE)
		{
			tryTime = 0;
			do{
				ret = zoo_create(zkHandle, pathArray, value.c_str(), valueLen, &ZOO_READ_ACL_UNSAFE, 1, NULL, 0);
				if(ret == ZOK)
					break;
				PERROR("zoo_create %s %d %d", pathArray, ret, zoo_state(zkHandle));
				if(ZCONNECTIONLOSS == ret)
					tryTime++;
				else
					break;
			}while(tryTime < colonyNum);
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

int ZookConfig::createForeverPath(const std::string& path, const std::string& value)
{
	int valueLen = 0, pathLen = 0, ret = 0, tryTime = 0, pathcheck = 0;
	char *pathArray = NULL, *pBuff = NULL;

	pathcheck = zookeeperPathCheck(path.c_str());
	if(pathcheck <= 0)
		return pathcheck;

	if(pathcheck == 1)
		return ZOOK_CONFIG_PARAMETER_ERROR;
	
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
					if(ret == ZOK)
						break;
					PERROR("zoo_exists %s %d %d", pathArray, ret, zoo_state(zkHandle));
					if(ZCONNECTIONLOSS == ret)
						tryTime++;
					else
						break;
				}while(tryTime < colonyNum);

				if(ZNONODE == ret)
				{
					tryTime = 0;
					do{
						ret = zoo_create(zkHandle, pathArray, NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
						if(ret == ZOK)
							break;
						PERROR("zoo_create %s %d %d", pathArray, ret, zoo_state(zkHandle));
						if(ZCONNECTIONLOSS == ret)
							tryTime++;
						else
							break;
					}while(tryTime < colonyNum);
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
			if(ret == ZOK)
				break;
			PERROR("zoo_exists %s %d %d", pathArray, ret, zoo_state(zkHandle));
			if(ZCONNECTIONLOSS == ret)
				tryTime++;
			else
				break;
		}while(tryTime < colonyNum);

		if(ret == ZOK)
		{
			tryTime = 0;
			do{
				ret = zoo_set(zkHandle, pathArray, value.c_str(), valueLen, -1);
				if(ret == ZOK)
					break;
				PERROR("zoo_set %s %d %d", pathArray, ret, zoo_state(zkHandle));
				if(ZCONNECTIONLOSS == ret)
					tryTime++;
				else
					break;
			}while(tryTime < colonyNum);
		}else if(ret == ZNONODE)
		{
			tryTime = 0;
			do{
				ret = zoo_create(zkHandle, pathArray, value.c_str(), valueLen, &ZOO_READ_ACL_UNSAFE, 0, NULL, 0);
				if(ret == ZOK)
					break;
				PERROR("zoo_create %s %d %d", pathArray, ret, zoo_state(zkHandle));
				if(ZCONNECTIONLOSS == ret)
					tryTime++;
				else
					break;
			}while(tryTime < colonyNum);
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
	char path[512] = {0}, cfg[1024] = {0};
	sprintf(path, "%s/%s", configRootPath.c_str(), key.c_str());

	int len = sizeof(cfg);
	zoo_get(zkHandle, path, 0, cfg, &len, NULL);

	std::string oldValue = "", newValue = "";
	{
		std::lock_guard<std::mutex> lock(configLock);
		ConfigMapDataIter iter = configMap.find(key);
		if(iter == configMap.end())
		{
			PERROR("configUpdateCallBack path :: %s no found in map may be add will be abandon", path);
			return;
		}else{
			oldValue.assign(iter->second);
			if(len > 0)
			{
				cfg[len] = '\0';
				newValue.assign(cfg);
				PDEBUG("configUpdateCallBack path :: %s value %s", path, newValue.c_str());
			}else{
				PDEBUG("configUpdateCallBack path :: %s value empty", path);
			}
			iter->second.assign(newValue);
		}
	}
	
	if(configCb)
		configCb(key, oldValue, newValue);
}

void ZookConfig::serverInfoChangeCallBack(const std::string& path)
{
	if(serverCb)
	{
		TcpServerInfoVector serverInfo;
		LoadTcpServerListInfo(path.c_str(), serverInfo);
		serverCb(serverInfo);
	}
}

int ZookConfig::getConfigRootPath(std::string& path)
{
	path.assign(configRootPath);
	int ret = static_cast<int>(configRootPath.size());
	return ret;
}

int ZookConfig::getServerInfoRootPath(TcpServerTypeVector& pathList)
{	
	pathList.assign(serverPathVector.begin(), serverPathVector.end());
	int ret = static_cast<int>(pathList.size());
	return ret;
}

int ZookConfig::setConfigKeyValue(const std::string& key, const std::string& value)
{
	int ret = 0, tryTime = 0;

	if (zkHandle)
	{
		char path[512] = {0};
		int len = static_cast<int>(value.length());

		sprintf(path, "%s/%s", configRootPath.c_str(), key.c_str());
		do{
			ret = zoo_set(zkHandle, path, value.c_str(), len, -1);
			if(ret == ZOK)
				break;
			PERROR("zoo_set %s %d %d", path, ret, zoo_state(zkHandle));
			if(ZCONNECTIONLOSS == ret)
				tryTime++;
			else
				break;
		}while(tryTime < colonyNum);
	}else{
		PERROR("loadAllKeyValue zkHandle no init");
		ret = ZOOK_CONFIG_NO_INIT;
		return ret;
	}
	
	return ret;
}

int ZookConfig::zookGetPathValue(const std::string& getPath, std::string& value)
{
	int ret = 0, tryTime = 0, pathcheck = 0;

	pathcheck = zookeeperPathCheck(getPath.c_str());
	if(pathcheck <= 0)
		return pathcheck;
	
	if (zkHandle)
	{
		char path[512] = {0}, cfg[1024] = {0};
		int len = 0;
		sprintf(path, "%s", getPath.c_str());
		do{
			len = sizeof(cfg);
			ret = zoo_get(zkHandle, path, 0, cfg, &len, NULL);
			if(ret == ZOK)
				break;
			PERROR("zoo_get on path %s error %d %s %d", path, ret, zerror(ret), zoo_state(zkHandle));
			if(ZCONNECTIONLOSS == ret)
				tryTime++;
			else
				return ret;
		}while(tryTime < colonyNum);

		if(ret == ZOK)
		{
			if(len > 0)
			{
				cfg[len] = '\0';
				value.assign(cfg);
			}else{
				value.assign("");
			}
		}
	}else{
		PERROR("loadAllKeyValue zkHandle no init");
		ret = ZOOK_CONFIG_NO_INIT;
		return ret;
	}
	
	return ret;
}

int ZookConfig::loadAllKeyValue(const char* rootPath)
{
	int ret = 0, tryTime = 0, pathcheck = 0;
	PDEBUG("loadAllKeyValue :: %s", rootPath);

	pathcheck = zookeeperPathCheck(rootPath);
	if(pathcheck <= 0)
		return pathcheck;
	
	if (zkHandle)
	{
		struct String_vector brokerlist;
		tryTime = 0;
		do{
			ret = zoo_get_children(zkHandle, rootPath, 0, &brokerlist);
			if(ret == ZOK)
				break;
			PERROR("zoo_get_children on path %s error %d %s %d", rootPath, ret, zerror(ret), zoo_state(zkHandle));
			if(ZCONNECTIONLOSS == ret)
				tryTime++;
			else
				return ret;
		}while(tryTime < colonyNum);
		if(ret != ZOK)
		{
			PERROR("Zookeeper No found on path %s error %d %s %d", rootPath, ret, zerror(ret), zoo_state(zkHandle));
			return ret;
		}

		for (int i = 0; i < brokerlist.count; i++)
		{
			char path[512] = {0}, cfg[1024] = {0};
			int len = 0;
			if(pathcheck == 1)
				sprintf(path, "%s%s", rootPath, brokerlist.data[i]);
			else
				sprintf(path, "%s/%s", rootPath, brokerlist.data[i]);
			tryTime = 0;
			do{
				len = sizeof(cfg);
				ret = zoo_get(zkHandle, path, 1, cfg, &len, NULL);
				if(ret == ZOK)
					break;
				PERROR("zoo_get on path %s error %d %s %d", rootPath, ret, zerror(ret), zoo_state(zkHandle));
				if(ZCONNECTIONLOSS == ret)
					tryTime++;
				else
					return ret;
			}while(tryTime < colonyNum);

			if(ret == ZOK)
			{
				if (len > 0)
				{
					cfg[len] = '\0';
					std::string configKey(brokerlist.data[i]), configValue(cfg);
					PDEBUG("loadAllKeyValue path :: %s value %s", configKey.c_str(), configValue.c_str());
					std::lock_guard<std::mutex> lock(configLock);
					configMap.insert(ConfigMapData::value_type(configKey, configValue));
				}else{
					std::string configKey(brokerlist.data[i]);
					PDEBUG("loadAllKeyValue path :: %s value empty", configKey.c_str());
					std::lock_guard<std::mutex> lock(configLock);
					configMap.insert(ConfigMapData::value_type(configKey, ""));
				}
			}else{
				PERROR("loadAllKeyValue zoo_get error %d", ret);
				return ret;
			}
		}
	}else{
		PERROR("loadAllKeyValue zkHandle no init");
		ret = ZOOK_CONFIG_NO_INIT;
		return ret;
	}
	ret = 0;
	return ret;
}

int ZookConfig::LoadTcpServerListInfo(const char* serverPath, TcpServerInfoVector& infoList)
{
	int ret = 0,tryTime = 0,result = 0;

	if (zkHandle)
	{
		TcpServerInfoVector ().swap(infoList);
		const char* pServerType = utilLastConstchar(serverPath, '/');
		if(pServerType == NULL)
			return ZOOK_CONFIG_PARAMETER_ERROR;
		pServerType++;
		std::string serverTypeName(pServerType);
		
		struct String_vector brokerlist;
		tryTime = 0;
		do{
			ret = zoo_get_children(zkHandle, serverPath, 1, &brokerlist);
			if(ret == ZOK)
				break;
			PERROR("zoo_get_children No found child on path %s error %d %s %d", serverPath, ret, zerror(ret), zoo_state(zkHandle));
			if(ZCONNECTIONLOSS == ret)
				tryTime++;
			else
				return ret;
		}while(tryTime < colonyNum);
		if(ret != ZOK)
		{
			PERROR("zoo_get_children No found child on path %s error %d %s %d", serverPath, ret, zerror(ret), zoo_state(zkHandle));
			return ret;
		}

		for (int i = 0; i < brokerlist.count; i++)
		{
			char infoPath[512] = {0}, infoCfg[1024] = {0};
			sprintf(infoPath, "%s/%s", serverPath, brokerlist.data[i]);
			int32_t serverNo = atoi(brokerlist.data[i]);
			PDEBUG("serverNo %d",serverNo);

			int infoLen = 0;
			tryTime = 0;
			do{
				infoLen = sizeof(infoCfg);
				ret = zoo_get(zkHandle, infoPath, 0, infoCfg, &infoLen, NULL);
				if(ret == ZOK)
					break;
				PERROR("zoo_get on path %s error %d %s %d", infoPath, ret, zerror(ret), zoo_state(zkHandle));
				if(ZCONNECTIONLOSS == ret)
					tryTime++;
				else
					return ret;
			}while(tryTime < colonyNum);
			
			if(ret != ZOK)
			{
				PERROR("zoo_get No found on path %s error %d %s %d", infoPath, ret, zerror(ret), zoo_state(zkHandle));
				continue;
			}
			
			if (infoLen > 0)
			{
				infoCfg[infoLen] = '\0';
			}else{
				PERROR("zoo_get No found on path %s error %d %s %d infolen 0", infoPath, ret, zerror(ret), zoo_state(zkHandle));
				continue;
			}
			
			char *pServerPort = utilFristChar(infoCfg,':');
			*pServerPort = '\0';
			
			std::string serverIp(infoCfg);
			PDEBUG("serverIp %s",serverIp.c_str());

			pServerPort++;
			uint16_t serverPort = static_cast<uint16_t>(atoi(pServerPort));

			PDEBUG("serverPort %d",serverPort);
			TcpServerInfo tcpServerInfo(serverNo, serverPort, serverIp, serverTypeName);
			result++;
			infoList.push_back(tcpServerInfo);
		}
	}else{
		PERROR("loadAllKeyValue zkHandle no init");
		ret = ZOOK_CONFIG_NO_INIT;
		return ret;
	}
	PDEBUG("LoadTcpServerListInfo result %d", result);
	return result;
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
		colonyNum++;
		p = utilFristConstchar(p, ',');
	}while(p && *p);
	
	return 0;
}

int ZookConfig::zookeeperPathCheck(const char *str)
{
	int len = static_cast<int>(strlen(str));
	if(len == 0)
	{
		len = -ZOOK_CONFIG_PARAMETER_ERROR;
		return len;
	}

	if(*str != '/')
	{
		return ZOOK_CONFIG_PARAMETER_ERROR;
	}

	if(len == 1)
	{
		return len;
	}

	if(len < 0)
		return len;

	len--;
	str += len;

	if(*str == '/')
		return ZOOK_CONFIG_PARAMETER_ERROR;
	
	len++;
	return len;
}

char* ZookConfig::utilFristChar(char *str,const char c)
{
	char *p = str;
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

const char* ZookConfig::utilLastConstchar(const char* str, const char c)
{
	const char *p = NULL;
	if(str == NULL)
		return NULL;
	p = utilEndConstchar(str);
	while(p >= str)
	{
		if(*p == c)
			return p;
		else
			p--;
	}
	return NULL;
}

const char* ZookConfig::utilEndConstchar(const char* str)
{
	const char *p = str;
	if(str == NULL)
		return NULL;
	while(*p != 0)
	{
		p++;
	}
	p--;
	return p;
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

	if(path.empty())
		return ret;
	
	ret = configPoint->zookLoadAllConfig(path);
	return ret;
}

int ZookConfigSingleton::createSessionPath(const std::string& path, const std::string& value)
{
	if(configPoint)
		return configPoint->createSessionPath(path, value);
	else
		return ZOOK_CONFIG_NO_INIT;
}

int ZookConfigSingleton::zookGetPathValue(const std::string& path, std::string& value)
{
	if(configPoint)
		return configPoint->zookGetPathValue(path, value);
	else
		return ZOOK_CONFIG_NO_INIT;
}

int ZookConfigSingleton::createForeverPath(const std::string& path, const std::string& value)
{
	if(configPoint)
		return configPoint->createForeverPath(path,value);
	else
		return ZOOK_CONFIG_NO_INIT;
}

void ZookConfigSingleton::setConfigChangeCall(const ConfigChangeCall& cb)
{
	if(configPoint)
		configPoint->setConfigChangeCall(cb);
}

void ZookConfigSingleton::setServerChangeCall(const TcpServerChangeCall& cb)
{
	if(configPoint)
		configPoint->setServerChangeCall(cb);
}

int ZookConfigSingleton::getConfigKeyValue(const std::string& key, std::string& value)
{
	if(configPoint)
		return configPoint->getConfigKeyValue(key,value);
	else
		return ZOOK_CONFIG_NO_INIT;
}

int ZookConfigSingleton::setConfigKeyValue(const std::string& key, const std::string& value)
{
	if(configPoint)
		return configPoint->setConfigKeyValue(key,value);
	else
		return ZOOK_CONFIG_NO_INIT;
}

int ZookConfigSingleton::getTcpServerListInfo(const std::string& serverPath, TcpServerInfoVector& infoList)
{
	if(configPoint)
		return configPoint->getTcpServerListInfo(serverPath,infoList);
	else
		return ZOOK_CONFIG_NO_INIT;
}

}

