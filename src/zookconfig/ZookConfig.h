#ifndef __ZOOK_CONFIG_PARAMETER_H__
#define __ZOOK_CONFIG_PARAMETER_H__
#include <stdio.h>

#include <mutex>
#include <functional>
#include <map>
#include <string>

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"

typedef std::map<std::string, std::string> ConfigMapData;
typedef ConfigMapData::iterator ConfigMapDataIter;

//��ʱ��֧�������ڵ㣬ɾ���ڵ������Ļص�
typedef std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)> ConfigChangeCall;

#define ZOOK_CONFIG_INIT_ERROR			-10000
#define ZOOK_CONFIG_PARAMETER_ERROR		-10001
#define ZOOK_CONFIG_ALREADY_INIT		-10002
#define ZOOK_CONFIG_NO_INIT				-10003

#ifdef SHOW_PRINTF_MESSAGE

#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)

#else

#define PDEBUG(fmt, args...)

#define PERROR(fmt, args...)

#endif

namespace ZookConfig
{

class ZookConfig
{
public:
	ZookConfig();

	~ZookConfig();

	int zookConfigInit(const std::string& zookAddr);

	//·��ĩβ���ܴ�б�߷ָ���
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

}

#endif
