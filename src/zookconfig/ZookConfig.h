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
//������Ϣ���壬������zookeeper�Լ�����Ĵ�������
/*
 *zhaoxiaoxiao
 *ע��ʹ�ó�������������ע����ʱ������Ϣ������������Ϣ����ȡ������Ϣ����������Ϣ����ӿ�
 *
 *�˽ӿڶ���ͬ���ӿ�
 */
namespace ZOOKCONFIG
{
//ע�������Ϣ��Ŀǰֻ�������ͣ������ڵ����ƣ����ر�ţ�����ʱ·������ip��portΪ��ʱ�ڵ�ֵ����ֿ��ģ�
//ip �Ͷ˿ڲ��÷ֿ�·��������ʵ�֣����Ҵ����Ƿֿ��������ص�����һ�λ�ȡ��
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

//��ʱ��֧�������ڵ㣬ɾ���ڵ������Ļص�
typedef std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)> ConfigChangeCall;

//��ȡ������Ϣ�仯�ص�
typedef std::function<void(const TcpServerInfoVector& tcpServerInfo)> TcpServerChangeCall;
//ͳһ����������ӿ�
class ZookConfig
{
public:
		
ZookConfig();

	~ZookConfig();
	//��ʼ�����У��첽��ʼ��zookeeper����
	int zookConfigInit(const std::string& zookAddr);

	//�����������ò�����·��ĩβ���ܴ�б�߷ָ�����
	int zookLoadAllConfig(const std::string& configPath);

	//��ȡһ�Բ������ã�key value
	int getConfigKeyValue(const std::string& key, std::string& value);

	//��ȡע�������Ϣ�������б�
	int getTcpServerListInfo(const std::string& serverPath, TcpServerInfoVector& infoList);
	
	//������ʱ·������Ϣֵ
	int createSessionPath(const std::string& path, const std::string& value);

	//����һ������·������Ϣֵ
	int createForeverPath(const std::string& path, const std::string& value);

	//�޸�key valueֵ���˽ӿڿ��ܻ������ػص�ִ��
	int setConfigKeyValue(const std::string& key, const std::string& value);

	//��ȡһ��·����ֵ��
	int zookGetPathValue(const std::string& getPath, std::string& value);

	//���ò����仯���ص���ʹ���߲��ù��ģ��ڲ�ʹ��
	void configUpdateCallBack(const std::string& key);

	//ע�������Ϣ�����仯�ص���ʹ���߲��ù���
	void serverInfoChangeCallBack(const std::string& path);
	
	//���ò��������ص�
	void setConfigChangeCall(const ConfigChangeCall& cb)
	{
		configCb = cb;
	}

	//���÷�����Ϣ�仯�ص�
	void setServerChangeCall(const TcpServerChangeCall& cb)
	{
		serverCb = cb;
	}
	///////////////////////////////////////////////////////////////////////////////////
	//�ڲ�ʹ�ã�ʹ���߲��ù���
	int getConfigRootPath(std::string& path);
	//�ڲ�ʹ�ã�ʹ���߲��ù���
	int getServerInfoRootPath(TcpServerTypeVector& pathList);

private:
	int initialize_zookeeper(const char* zookeeper, const int debug = 1);

	int loadAllKeyValue(const char* rootPath);

	int LoadTcpServerListInfo(const char* serverPath, TcpServerInfoVector& infoList);

	char* utilFristChar(char *str,const char c);

	int zookeeperPathCheck(const char *str);
	
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

//��ʵ��ʹ�÷�ʽ������ʹ�ã�һ�������н���һ���������
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

	//��ʵ����ȡ����ӿ�
	static ZookConfigSingleton& instance() { return ZOOKCONFIG::Singleton<ZookConfigSingleton>::instance(); }

	//zookAddr zookeeper ��ַ
	//path ��β��Ҫ��б�ܣ����õ�·����ĩβһ�����ܴ�б��
	//·����Ҫ�Լ���Ȩ�ޣ����û�ж���Ȩ����û�а취���ظ������õ�
	//�����ʱû����������Դ���
	int zookConfigInit(const std::string& zookAddr, const std::string& path);

	//����һ����ʱ·����
	//path��·���� ĩβһ�����ܴ�б��
	//value��ֵ��
	int createSessionPath(const std::string& path, const std::string& value);

	//��ȡһ��·����Ӧ��ֵ
	int zookGetPathValue(const std::string& path, std::string& value);

	//����һ�����õ�·��
	//path��·����ĩβһ�����ܴ�б��
	//value��ֵ
	int createForeverPath(const std::string& path, const std::string& value);

	//���ò����仯�ص�����
	void setConfigChangeCall(const ConfigChangeCall& cb);

	//���÷�����Ϣ�仯�ص�����
	void setServerChangeCall(const TcpServerChangeCall& cb);

	//��ȡ���ò�����key valueֵ
	int getConfigKeyValue(const std::string& key, std::string& value);

	//����һ�����ò�����ֵ������п��ܻ�����ص�ִ��
	int setConfigKeyValue(const std::string& key, const std::string& value);

	//��ȡ������Ϣ�ӿڣ�serverPathΪ·����ĩβ���ܴ�б��
	//infoList �᷵�ط�����Ϣ�б���Ϣ�������б����
	int getTcpServerListInfo(const std::string& serverPath, TcpServerInfoVector& infoList);
private:
	ZookConfigPtr configPoint;
};

}

#endif
