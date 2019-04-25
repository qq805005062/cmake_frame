#ifndef __LIBEVENT_TCPCLI_INCOMMON_H__
#define __LIBEVENT_TCPCLI_INCOMMON_H__

#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

#include <string>
#include <memory>

#define DEBUG(fmt, args...)		fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define INFO(fmt, args...)		fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define WARN(fmt, args...)		fprintf(stderr, "%s :: %s() %d: WARN " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ERROR(fmt, args...)		fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

enum TcpClientState
{
	CONN_FAILED = 0,
	CONN_SUCCESS,
	DIS_CONNECT,
};

inline int64_t microSecondSinceEpoch(int64_t* second = NULL)
{
	 struct timeval tv;
 	 gettimeofday(&tv, NULL);
	 if(second)
	 {
	 	*second = tv.tv_sec;
	 }
	 int64_t microSeconds = tv.tv_sec * 1000000 + tv.tv_usec;
	 return microSeconds;
}

inline int64_t secondSinceEpoch()
{
	 struct timeval tv;
 	 gettimeofday(&tv, NULL);
	 int64_t seconds = tv.tv_sec;
	 return seconds;
}

#endif
