#ifndef __ZOO_KFK_BROKERS_H__
#define __ZOO_KFK_BROKERS_H__

#include <functional>

//#include "ZooKafBrokers.h"

#include "Singleton.h"
#include "noncopyable.h"

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"

typedef std::function<void(const std::string& newBrokers)> KfkBrokersChangeCall;

namespace ZOOKEEPERKAFKA
{

class ZooKafBrokers : public noncopyable
{
public:
	
	ZooKafBrokers();
	
	~ZooKafBrokers();

	static ZooKafBrokers& instance() { return ZOOKEEPERKAFKA::Singleton<ZooKafBrokers>::instance(); }

	std::string zookInit(const std::string& zookeepers);

	void setBrokerNoticeCall(const KfkBrokersChangeCall& cb)
	{
		cb_ = cb;
	}

	void kfkBrokerChange(const std::string& bros);
	
private:

	zhandle_t* initialize_zookeeper(const char* zookeeper, int debug = 1);

	std::string zKeepers;
	zhandle_t *zookeeph;
	
	KfkBrokersChangeCall cb_;
};

}
#endif

