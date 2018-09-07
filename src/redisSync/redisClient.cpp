#include <strings.h>

#include "redisClient.h"

#define PDEBUG(fmt, args...)		fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)		fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

namespace REDIS_SYNC_CLIENT
{

redisClient::redisClient()
	:isConn(false)
	,redisPort(0)
	,timeoutMs(0)
	,redisHost()
	,redisPass()
	,context_(nullptr)
{
	PDEBUG("redisClient init");
}

redisClient::~redisClient()
{
	PERROR("~redisClient exit");
	disConnect();
}

int redisClient::connect(int port, const std::string& host, int outMs, const std::string& pass)
{
	if(host.empty() || port == 0)
	{
		return REDIS_SYNC_PARAMETER_ERROR;
	}

	timeoutMs = outMs;
	redisPort = port;
	redisHost.assign(host);
	if(pass.length())
	{
		redisPass.assign(pass);
	}

	struct timeval timeout;
	timeout.tv_sec = outMs / 1000;
	timeout.tv_usec = (outMs % 1000) * 1000;

	context_ = redisConnectWithTimeout(host.c_str(), port, timeout);
	if (context_ == NULL || context_->err)
	{
		if(context_)
		{
			PERROR("redisConnectWithTimeout error %s", context_->errstr);
			redisFree(context_);
		}else{
			PERROR("Connection error: can't allocate redis context");
		}
		return REDIS_SYNC_DISCONNECT_ERROR;
	}

	if(pass.length())
	{
		if(auth() < 0)
		{
			disConnect();
			return REDIS_SYNC_PASS_ERROR;
		}
	}
	isConn = true;
	return 0;
}

void redisClient::disConnect()
{
	if(context_)
	{
		redisFree(context_);
		context_ = nullptr;
	}
	isConn = false;
}

int redisClient::ping()
{
	int ret = 0;
	if(isConn == false)
	{
		return REDIS_SYNC_DISCONNECT_ERROR;
	}

	redisReply* reply = static_cast<redisReply* >(redisCommand(context_, "PING"));
	if (reply == NULL)
	{
		PERROR("Connection error: can't allocate redis redisReply");
		return REDIS_SYNC_DISCONNECT_ERROR;
	}

	if(reply->str)
	{
		if(strcasecmp(reply->str, "PONG"))
		{
			if(reply->type == REDIS_REPLY_ERROR)
			{
				PERROR("redisCommand PING error %s", reply->str);
			}
			ret = REDIS_SYNC_DISCONNECT_ERROR;
		}
	}

	freeReplyObject(reply);
	return ret;
}

int redisClient::auth()
{
	int ret = 0;
	redisReply* reply = static_cast<redisReply* >(redisCommand(context_, "AUTH %s", redisPass.c_str()));
	if (reply == NULL)
	{
		PERROR("Connection error: can't allocate redis redisReply");
		return REDIS_SYNC_DISCONNECT_ERROR;
	}

	if(reply->str)
	{
		if(strcasecmp(reply->str, "OK"))
		{
			PERROR("redisCommand auth error %s", reply->str);
			ret = REDIS_SYNC_DISCONNECT_ERROR;
		}
	}
	
	freeReplyObject(reply);
	return ret;
}

}

