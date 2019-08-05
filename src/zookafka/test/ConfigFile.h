
#ifndef __ZOO_KFK_CONFIG_H__
#define __ZOO_KFK_CONFIG_H__

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "../Singleton.h"
#include "../noncopyable.h"

class ConfigFile : public ZOOKEEPERKAFKA::noncopyable
{

public:
	
	ConfigFile();
	~ConfigFile();
	
	static ConfigFile& instance() { return ZOOKEEPERKAFKA::Singleton<ConfigFile>::instance();}

	std::string zookeepBrokers();

	std::string kakfaBrokers();
	
	int testMsgNum();
	
	int testThreadNum();

	std::string testTopicName();

	int producerSwitch();

	int producerMessSize();

	int consumerSwitch();

private:
	boost::property_tree::ptree m_pt;
	bool isErr;

};

#endif

