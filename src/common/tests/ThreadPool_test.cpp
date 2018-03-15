#include <common/ThreadPool.h>
#include <common/CurrentThread.h>
#include <common/CountDownLatch.h>

#include <unistd.h>
#include <iostream>

using namespace std;

void print()
{
	cout << "tid = " << common::CurrentThread::tid() << endl;
}

void printString(const string& str)
{
	cout << str << endl;
	usleep(100*1000);
}

void test(int maxSize)
{
	cout << "The ThreadPool with max queue size = " << maxSize << endl;
	common::ThreadPool pool("MainThreadPool");
	pool.setMaxQueueSize(maxSize);
	pool.start(5);

	cout << "Adding " << endl;
	pool.run(print);
	pool.run(print);

	for (int i = 0; i < 100; i++)
	{
		char buf[32];
		snprintf(buf, sizeof buf, "task %d", i);
		pool.run(std::bind(printString, std::string(buf)));
	}

	cout << "Done" << endl;

	common::CountDownLatch latch(1);
	pool.run(std::bind(&common::CountDownLatch::countDown, &latch));
	latch.wait();
	pool.stop();
}

int main()
{
	test(0);
	test(1);
	test(5);
	test(10);
	test(50);
}