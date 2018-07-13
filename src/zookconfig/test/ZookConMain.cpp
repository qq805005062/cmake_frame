#include <unistd.h>

#include "../ZookConfig.h"

int main(int argc, char* argv[])
{
	PDEBUG("Hello world");
	std::string zookAddr = "192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181", testPath = "/imbizsvr/5858";
	ZOOKCONFIG::ZookConfigSingleton::instance().zookConfigInit(zookAddr,testPath);
	ZOOKCONFIG::ZookConfigSingleton::instance().createSessionPath("/imbizsvr/5858/ip","192.169.0.61");
	while(1)
		sleep(60);
	return 0;
}

