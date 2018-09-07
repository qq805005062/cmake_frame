#ifndef __REDIS_SYNCHRONIZATION_CLIENT_H__
#define __REDIS_SYNCHRONIZATION_CLIENT_H__

#include <stdio.h>
#include <string>

#include <hiredis/hiredis.h>

#define REDIS_SYNC_PARAMETER_ERROR			-4000
#define REDIS_SYNC_DISCONNECT_ERROR			-4001
#define REDIS_SYNC_TIMEOUT_ERROR			-4002
#define REDIS_SYNC_PASS_ERROR				-4003


namespace REDIS_SYNC_CLIENT
{

class redisClient
{
public:
	redisClient();

	~redisClient();

	int connect(int port, const std::string& host, int outMs = 1500, const std::string& pass = "");

	void disConnect();

	int ping();

	int set(const std::string& key, const std::string& value);

	int get(const std::string& key, std::string& value);

private:

	int auth();
	
	bool isConn;
	int redisPort;
	int timeoutMs;
	std::string redisHost;
	std::string redisPass;

	redisContext* context_;
	
};

}

#endif
