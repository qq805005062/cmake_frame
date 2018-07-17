#ifndef __ZOOK_CONFIG_PARAMETER_H__
#define __ZOOK_CONFIG_PARAMETER_H__
#include <stdio.h>

#include <mutex>
#include <functional>
#include <map>
#include <vector>
#include <string>
#include <memory>

#include "Singleton.h"
#include "noncopyable.h"

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"

#define ZOOK_CONFIG_INIT_ERROR			-10000
#define ZOOK_CONFIG_PARAMETER_ERROR		-10001
#define ZOOK_CONFIG_ALREADY_INIT		-10002
#define ZOOK_CONFIG_NO_INIT				-10003

#define ZOOK_CONFIG_MALLOC_ERROR		-10004
#define ZOOK_SERVER_INFO_ALREADY_INIT	-10005
#define ZOOK_SERVER_INFO_ERROR			-10006

#ifdef SHOW_PRINTF_MESSAGE

#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)

#else

#define PDEBUG(fmt, args...)

#define PERROR(fmt, args...)

#endif

namespace ZOOKCONFIG
{
//注册服务信息，目前只会有类型，（根节点名称）网关编号，（临时路径），ip：port为临时节点值，拆分开的，
//ip 和端口不好分开路径，不好实现，而且创建是分开创建，回调很难一次获取到
typedef struct __TcpServerInfo{
	__TcpServerInfo(int32_t no, int16_t po, const std::string& ip, const std::string& typeName)
		:gateNo(no)
		,port(po)
		,ipAddr(ip)
		,typeKey(typeName)
	{
	}

	~__TcpServerInfo() {}

	__TcpServerInfo(const __TcpServerInfo& that)
		:gateNo(0)
		,port(0)
		,ipAddr()
		,typeKey()
	{
		*this = that;
	}

	__TcpServerInfo& operator=(const __TcpServerInfo& that)
	{
		if (this == &that) return *this;

		gateNo = that.gateNo;
		port = that.port;
		ipAddr = that.ipAddr;
		typeKey = that.typeKey;
		return *this;
	}
	
	int32_t gateNo;
	int16_t port;
	std::string ipAddr;
	std::string typeKey;
}TcpServerInfo;

typedef std::vector<TcpServerInfo> TcpServerInfoVector;
typedef std::vector<std::string> TcpServerTypeVector;

typedef std::map<std::string, std::string> ConfigMapData;
typedef ConfigMapData::iterator ConfigMapDataIter;

//暂时不支持新增节点，删除节点这样的回调
typedef std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)> ConfigChangeCall;

//获取服务信息变化回调
typedef std::function<void(const TcpServerInfoVector& tcpServerInfo)> TcpServerChangeCall;
//统一参数配置类接口
class ZookConfig
{
public:
	ZookConfig();

	~ZookConfig();
	//初始化队列，异步初始化zookeeper连接
	int zookConfigInit(const std::string& zookAddr);

	//加载所有配置参数，路径末尾不能带斜线分隔符，
	int zookLoadAllConfig(const std::string& configPath);

	//获取一对参数配置，key value
	int getConfigKeyValue(const std::string& key, std::string& value);

	//获取注册服务信息，返回列表
	int getTcpServerListInfo(const std::string& serverPath, TcpServerInfoVector& infoList);
	
	//创建临时路径、消息值
	int createSessionPath(const std::string& path, const std::string& value);

	//修改key value值，暂时未实现
	int setConfigKeyValue(const std::string& key, const std::string& value);

	//配置参数变化，回调，使用者不用关心，内部使用
	void configUpdateCallBack(const std::string& key);

	//注册服务信息参数变化回调，使用者不用关系
	void serverInfoChangeCallBack(const std::string& path);
	
	//设置参数监听回调
	void setConfigChangeCall(const ConfigChangeCall& cb)
	{
		configCb = cb;
	}

	//设置服务信息变化回调
	void setServerChangeCall(const TcpServerChangeCall& cb)
	{
		serverCb = cb;
	}
	///////////////////////////////////////////////////////////////////////////////////
	//内部使用，
	int getConfigRootPath(std::string& path);
	//内部使用
	int getServerInfoRootPath(TcpServerTypeVector& pathList);

private:
	int initialize_zookeeper(const char* zookeeper, const int debug = 1);

	int loadAllKeyValue(const char* rootPath);

	int LoadTcpServerListInfo(const char* serverPath, TcpServerInfoVector& infoList);

	char* utilFristChar(char *str,const char c);
	
	const char* utilFristConstchar(const char *str,const char c);

	const char* utilLastConstchar(const char* str, const char c);

	const char* utilEndConstchar(const char* str);
	
	int colonyNum;
	std::string configRootPath;
	TcpServerTypeVector serverPathVector;
	
	zhandle_t *zkHandle;
	std::mutex configLock;
	
	ConfigChangeCall configCb;
	TcpServerChangeCall serverCb;
	ConfigMapData configMap;
};

typedef std::shared_ptr<ZOOKCONFIG::ZookConfig> ZookConfigPtr;

//单实例使用方式，方便使用，一个进程中仅仅一个对象存在
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

	//单实例获取对象接口
	static ZookConfigSingleton& instance() { return ZOOKCONFIG::Singleton<ZookConfigSingleton>::instance(); }

	//zookAddr zookeeper 地址
	//path 结尾不要带斜杠，配置的路径，末尾一定不能带斜杠
	int zookConfigInit(const std::string& zookAddr, const std::string& path);
	//创建一个临时路径，
	//path是路径， 末尾一定不能带斜杠，value是值，
	int createSessionPath(const std::string& path, const std::string& value);
	//设置参数变化回调函数
	void setConfigChangeCall(const ConfigChangeCall& cb);
	//设置服务信息变化回调函数
	void setServerChangeCall(const TcpServerChangeCall& cb);
	//获取配置参数中key value值
	int getConfigKeyValue(const std::string& key, std::string& value);
	//获取服务信息接口，serverPath为路径，末尾不能带斜杠
	//infoList 会返回服务信息列表信息。返回列表个数
	int getTcpServerListInfo(const std::string& serverPath, TcpServerInfoVector& infoList);
private:
	ZookConfigPtr configPoint;
};

}

#endif
