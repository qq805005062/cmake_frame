#include <unistd.h>

#include "../ZookConfig.h"

#ifdef SHOW_PRINTF_MESSAGE

#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)

#else

#define PDEBUG(fmt, args...)

#define PERROR(fmt, args...)

#endif

void configWatchFun(const std::string& key, const std::string& oldValue, const std::string& newValue)
{
	PDEBUG("configWatchFun key %s oldValue %s newValue %s", key.c_str(), oldValue.c_str(), newValue.c_str());
}

void serverWatchFun(const ZOOKCONFIG::TcpServerInfoVector& tcpServerInfo, const std::string& typeName)
{
	PDEBUG("tcpServerInfo type %s", tcpServerInfo[i].typeKey.c_str());
	for(size_t i = 0; i < tcpServerInfo.size(); i++)
	{
		PDEBUG("ip addr %s port %d process no %d", tcpServerInfo[i].ipAddr.c_str(), tcpServerInfo[i].port, tcpServerInfo[i].gateNo);
	}
}


int main(int argc, char* argv[])
{
	int ret = 0;
	
	std::string zookAddr = "192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181", testPath = "/xiaoxiao/imbizsvr/5857";
	ret = ZOOKCONFIG::ZookConfigSingleton::instance().zookConfigInit(zookAddr,testPath);
	if(ret < 0)
	{
		PERROR("zookConfigInit ret %d", ret);
		return ret;
	}

	std::string configKey = "common_idc_type", configValue;
	ret = ZOOKCONFIG::ZookConfigSingleton::instance().getConfigKeyValue(configKey, configValue);
	if(ret < 0)
	{
		PERROR("Zookeeper config no found config key %s", configKey.c_str());
		//return ret;
	}else{
		PDEBUG("common_idc_type topic= \"%s\"",configValue.c_str());
	}


	ZOOKCONFIG::ZookConfigSingleton::instance().setConfigChangeCall(std::bind(configWatchFun, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	ZOOKCONFIG::ZookConfigSingleton::instance().setServerChangeCall(std::bind(serverWatchFun, std::placeholders::_1, std::placeholders::_2));
	
	ZOOKCONFIG::ZookConfigSingleton::instance().createSessionPath("/im/222/imloginsvr/223/dadad_dasdad","192.169.0.61:9568");

	ZOOKCONFIG::TcpServerInfoVector tcpServerInfo;
	ZOOKCONFIG::ZookConfigSingleton::instance().getTcpServerListInfo("/xiaoxiao/imaccesssvr",tcpServerInfo);
	for(size_t i = 0; i < tcpServerInfo.size(); i++)
	{
		PDEBUG("tcpServerInfo type %s, ip addr %s port %d process no %d", tcpServerInfo[i].typeKey.c_str(), tcpServerInfo[i].ipAddr.c_str(), tcpServerInfo[i].port, tcpServerInfo[i].gateNo);
	}

	while(1)
		sleep(60);
	return 0;
}

