#ifndef __XIAO_INCOMMON_H__
#define __XIAO_INCOMMON_H__

#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <memory>

#define DEBUG(fmt, args...)		fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define INFO(fmt, args...)		fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define WARN(fmt, args...)		fprintf(stderr, "%s :: %s() %d: WARN " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ERROR(fmt, args...)		fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)


#endif
