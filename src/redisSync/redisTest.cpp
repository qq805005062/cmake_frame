
#include <stdio.h>

#include "redisClient.h"

#define PDEBUG(fmt, args...)		fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)		fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

int main(int argc, char **argv)
{
	REDIS_SYNC_CLIENT::redisClient client;

	int ret = client.connect(6379, "127.0.0.1");
	PDEBUG("client.connect ret %d", ret);

	ret = client.ping();
	PDEBUG("client.ping ret %d", ret);

	client.disConnect();
	return ret;
}

