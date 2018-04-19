
#include <pthread.h>

#include "../ZooKfkCommon.h"

static volatile uint64_t countNum = 0;

int consuerTest()
{
	return 0;
}

int producerTest()
{
	return 0;
};

int main(int argc, char* argv[])
{

	static volatile uint64_t last_recv_num = 0;
	static volatile int second = 0;
	while(1)
	{
		second++;
		if(last_recv_num == 0 && countNum > 0)
		{
			second = 1;
		}

		uint64_t NowCountNum = countNum;
		printf("recv all num :: %lu,recv speed :: %lu,average speed :: %ld\n",
			NowCountNum, (NowCountNum - last_recv_num), (NowCountNum / second) );
		last_recv_num = NowCountNum;
		sleep(1);
	}
	return 0;
}

