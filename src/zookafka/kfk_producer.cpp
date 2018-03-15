
#include <iostream>
#include <string>

#include <stdio.h>

#include "ZooKafkaPut.h"

int main(int argc, char* argv[])
{
	ZOOKEEPERKAFKA::ZooKafkaPut prducer;
	prducer.zookInit("192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181","zookeeper");

	for (std::string line; std::getline(std::cin, line); line.clear())
	{
		prducer.push(line);
	}
	return 0;
}

