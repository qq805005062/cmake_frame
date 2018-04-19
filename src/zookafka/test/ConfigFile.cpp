
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
		PERROR("ConfigFile read error :: %s\n",e.what());
		isErr = true;
	}
}

ConfigFile::~ConfigFile()
{
	PERROR("ConfigFile exit\n");
}

int ConfigFile::producerSwitch()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file\n");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("producer");
	int sw = tag_setting.get<int>("switch", 1);
	PDEBUG("producerSwitch = %d\n",sw);
	return sw;
}

int ConfigFile::producerMessNum()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file\n");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("producer");
	int num = tag_setting.get<int>("messnum", 100000000);
	PDEBUG("producerMessNum = %d\n",num);
	return num;
}

int ConfigFile::producerMessSize()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file\n");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("producer");
	int size = tag_setting.get<int>("messize", 1000);
	PDEBUG("producerMessSize = %d\n",size);
	return size;
}

int ConfigFile::producerNum()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file\n");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("producer");
	int num = tag_setting.get<int>("pronum", 4);
	PDEBUG("producerNum = %d\n",num);
	return num;
}

int ConfigFile::consumerSwitch()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file\n");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("consumer");
	int sw = tag_setting.get<int>("switch", 1);
	PDEBUG("consumerSwitch = %d\n",sw);
	return sw;
}

int ConfigFile::consumerNum()
{
	if(isErr)
	{
		PERROR("There is something wrong about config file\n");
		return -1;
	}

	boost::property_tree::ptree tag_setting = m_pt.get_child("consumer");
	int num = tag_setting.get<int>("cosnum", 4);
	PDEBUG("consumerNum = %d\n",num);
	return num;
}


