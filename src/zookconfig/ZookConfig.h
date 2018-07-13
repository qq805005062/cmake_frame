#ifndef __ZOOK_CONFIG_PARAMETER_H__
#define __ZOOK_CONFIG_PARAMETER_H__
#include <stdio.h>

#include <mutex>
#include <functional>
#include <map>
#include <string>
#include <memory>

#include "Singleton.h"
#include "noncopyable.h"

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"

typedef std::map<std::string, std::string> ConfigMapData;
typedef ConfigMapData::iterator ConfigMapDataIter;

//暂时不支持新增节点，删除节点这样的回调
typedef std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)> ConfigChangeCall;

#define ZOOK_CONFIG_INIT_ERROR			-10000
#define ZOOK_CONFIG_PARAMETER_ERROR		-10001
#define ZOOK_CONFIG_ALREADY_INIT		-10002
#define ZOOK_CONFIG_NO_INIT				-10003

#define ZOOK_CONFIG_MALLOC_ERROR		-10004

#ifdef SHOW_PRINTF_MESSAGE

#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)

#else

#define PDEBUG(fmt, args...)

#define PERROR(fmt, args...)

#endif

namespace ZOOKCONFIG
{

class ZookConfig
{
public:
	ZookConfig();

	~ZookConfig();

	int zookConfigInit(const std::string& zookAddr);

	//路径末尾不能带斜线分隔符
	int zookLoadAllConfig(const std::string& path);

	int getConfigKeyValue(const std::string& key, std::string& value);
	
	int createSessionPath(const std::string& path, const std::string& value);

	int setConfigKeyValue(const std::string& key, const std::string& value);


	void configUpdateCallBack(const std::string& key);

	void setConfigChangeCall(const ConfigChangeCall& cb)
	{
		cb_ = cb;
	}

private:
	int initialize_zookeeper(const char* zookeeper, const int debug = 1);

	int loadAllKeyValue(const char* rootPath);

	const char* utilFristConstchar(const char *str,const char c);
	
	zhandle_t *zkHandle;
	std::mutex configLock;
	
	ConfigChangeCall cb_;

	ConfigMapData configMap;
};

typedef std::shared_ptr<ZOOKCONFIG::ZookConfig> ZookConfigPtr;

class ZookConfigSingleton : public noncopyable
{
public:
	ZookConfigSingleton()
		:configPoint(nullptr)
	{
	
}

	~ZookConfigSingleton()
	{
		if(configPoint)
			configPoint.reset();
	}

	static ZookConfigSingleton& instance() { return ZOOKCONFIG::Singleton<ZookConfigSingleton>::instance(); }

	int zookConfigInit(const std::string& zookAddr, const std::string& path);

	int createSessionPath(const std::string& path, const std::string& value);
	
private:
	ZookConfigPtr configPoint;
};

}

#endif
