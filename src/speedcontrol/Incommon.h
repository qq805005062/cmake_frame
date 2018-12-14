#ifndef __SPEED_CONTROL_INCOMMON_H__
#define __SPEED_CONTROL_INCOMMON_H__

#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

#include <string>
#include <memory>

#define PDEBUG(fmt, args...)		fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)		fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

inline int64_t secondSinceEpoch()
{
	 struct timeval tv;
 	 gettimeofday(&tv, NULL);
	 int64_t seconds = tv.tv_sec;
	 return seconds;
}
#endif