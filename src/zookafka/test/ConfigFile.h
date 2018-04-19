
#ifndef __ZOO_KFK_CONFIG_H__
#define __ZOO_KFK_CONFIG_H__

#include <string>

#include <common/Singleton.h>
#include <common/MutexLock.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

class ConfigFile : public common::noncopyable
{

public:
	
	ConfigFile();
	~ConfigFile();
	
	static ConfigFile& instance() { return common::Singleton<ConfigFile>::instance();}

	int producerSwitch();

	int producerMessNum();

	int producerMessSize();
	
	int producerNum();

	int consumerSwitch();

	int consumerNum();
private:
	boost::property_tree::ptree m_pt;
	bool isErr;

};

#endif

