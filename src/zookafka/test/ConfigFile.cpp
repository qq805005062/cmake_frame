
#include "../ZooKfkCommon.h"
#include "ConfigFile.h"

static const char conFileName[] = "./zookfk.ini";

ConfigFile::ConfigFile()
{
	isErr = false;
	try
	{
    	boost::property_tree::ini_parser::read_ini(conFileName, m_pt);	
	}catch (const std::exception & e)
	{
		PERROR("ConfigFile read error :: %s",e.what());
		isErr = true;
	}
}

ConfigFile::~ConfigFile()
{
	PERROR("~ConfigFile exit");
}

std::string ConfigFile::zookeepBrokers()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file");
		return "";
	}
	boost::property_tree::ptree tag_setting = m_pt.get_child("common");
	std::string bro = tag_setting.get<std::string>("zookeeper", "192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181");
	PDEBUG("zookeepBrokers = %s", bro.c_str());
	return bro;
}

int ConfigFile::testMsgNum()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("common");
	int num = tag_setting.get<int>("messnum", 100000000);
	PDEBUG("testMsgNum = %d",num);
	return num;
}

int ConfigFile::testThreadNum()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("common");
	int num = tag_setting.get<int>("threadnum", 4);
	PDEBUG("testThreadNum = %d",num);
	return num;
}

std::string ConfigFile::testTopicName()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file");
		return "";
	}
	boost::property_tree::ptree tag_setting = m_pt.get_child("common");
	std::string top = tag_setting.get<std::string>("topic", "speedTopic");
	PDEBUG("testTopicName = %s",top.c_str());
	return top;
}

int ConfigFile::producerSwitch()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("producer");
	int sw = tag_setting.get<int>("switch", 0);
	PDEBUG("producerSwitch = %d",sw);
	return sw;
}


int ConfigFile::producerMessSize()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("producer");
	int size = tag_setting.get<int>("messize", 500);
	PDEBUG("producerMessSize = %d",size);
	return size;
}

int ConfigFile::consumerSwitch()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("consumer");
	int sw = tag_setting.get<int>("switch", 1);
	PDEBUG("consumerSwitch = %d",sw);
	return sw;
}


