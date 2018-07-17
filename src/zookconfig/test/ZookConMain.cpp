#include <unistd.h>

#include "../ZookConfig.h"

void configWatchFun(const std::string& key, const std::string& oldValue, const std::string& newValue)
{
	PDEBUG("configWatchFun key %s oldValue %s newValue %s", key.c_str(), oldValue.c_str(), newValue.c_str());
}

void serverWatchFun(const ZOOKCONFIG::TcpServerInfoVector& tcpServerInfo)
{
	for(size_t i = 0; i < tcpServerInfo.size(); i++)
	{
		PDEBUG("tcpServerInfo type %s, ip addr %s port %d process no %d", tcpServerInfo[i].typeKey.c_str(), tcpServerInfo[i].ipAddr.c_str(), tcpServerInfo[i].port, tcpServerInfo[i].gateNo);
	}
}


int main(int argc, char* argv[])
{
	int ret = 0;
	
	std::string zookAddr = "192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181", testPath = "/xiaoxiao/imbizsvr/5857";
	ret = ZOOKCONFIG::ZookConfigSingleton::instance().zookConfigInit(zookAddr,testPath);
	if(ret < 0)
	{
		PERROR("");
		return ret;
	}

	ZOOKCONFIG::ZookConfigSingleton::instance().setConfigChangeCall(std::bind(configWatchFun, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	ZOOKCONFIG::ZookConfigSingleton::instance().setServerChangeCall(std::bind(serverWatchFun, std::placeholders::_1));
	
	ZOOKCONFIG::ZookConfigSingleton::instance().createSessionPath("/imbizsvr/5858","192.169.0.61:9568");

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

