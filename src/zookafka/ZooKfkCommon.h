
#ifndef __ZOO_KFK_COMMON_H__
#define __ZOO_KFK_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#ifdef SHOW_DEBUG_MESSAGE
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#ifdef SHOW_ERROR_MESSAGE
#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ## args)
#else
#define PERROR(fmt, args...)
#endif

/////////////////////////////////////////////////////////////////////////////////////////////

#define NO_KAFKA_BROKERS_FOUND				-10000
#define TRANSMIT_PARAMTER_ERROR				-10001
#define PUSH_TOPIC_NAME_NOINIT				-10002
#define MODULE_RECV_EXIT_COMMND				-10003

#define KAFKA_MODULE_NEW_ERROR				-10004
#define KAFKA_BROKERS_ADD_ERROR				-10005
#define KAFKA_NO_TOPIC_NAME_INIT			-10006
#define KAFKA_TOPIC_CONF_NEW_ERROR			-10007
#define KAFKA_TOPIC_NEW_ERROR				-10008
#define KAFKA_INIT_ALREADY					-10009
#define KAFKA_NO_INIT_ALREADY				-10010
#define KAFKA_UNHAPPEN_ERRPR				-10011

#define KAFKA_CONSUMER_CONFSET_ERROR		-10012
#define KAFKA_CONSUMER_ADDTOPIC_ERROR		-10013

////////////////////////////////////////////////////////////////////////////////////////////////

#define KFK_LOG_EMERG   0
#define KFK_LOG_ALERT   1
#define KFK_LOG_CRIT    2
#define KFK_LOG_ERR     3
#define KFK_LOG_WARNING 4
#define KFK_LOG_NOTICE  5
#define KFK_LOG_INFO    6
#define KFK_LOG_DEBUG   7

//////////////////////////////////////////////////////////////////////////////////////////////
inline int strIntLen(const char *str)
{
	int len = 0;
	if(!str)
		return len;
	while(*str)
	{
		str++;
		len++;
	}
	return len;
}

inline char* utilFristchar(char *str,const char c)
{
	char *p = str;
	if(!str)
		return NULL;
	while(*p)
	{
		if(*p == c)
			return p;
		else
			p++;
	}
	return NULL;
}

inline const char* utilFristConstchar(const char *str,const char c)
{
	const char *p = str;
	if(!str)
		return NULL;
	while(*p)
	{
		if(*p == c)
			return p;
		else
			p++;
	}
	return NULL;
}


#endif

