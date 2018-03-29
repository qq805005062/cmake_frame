
#include "RedisAsync.h"

namespace ASYNCREDIS
{

static void connectCallback(const redisAsyncContext *c, int status)
{
	PDEBUG("RedisOnConnection connectCallback success\n");
	RedisAsync *pRedis = static_cast<RedisAsync *>(c->data);
	pRedis->RedisOnConnection(c,status,true);
}

static void disconnectCallback(const redisAsyncContext *c, int status)
{
	PDEBUG("RedisOnConnection disconnectCallback success\n");
	RedisAsync *pRedis = static_cast<RedisAsync *>(c->data);
	pRedis->RedisOnConnection(c,status,false);
}

RedisAsync::RedisAsync(struct event_base* base)
	:IterLock()
	,status(0)//0 mean no init,1 mean conning host and port,2 mean conn succ,3 mean conn failed,4 mean will be exit
	,libevent(base)
	,conInitNum(0)
	,conSuccNum(0)
	,lastIndex(0)
	,host_()
	,port_(0)
	,conns_()
{
}

RedisAsync::~RedisAsync()
{
}

int RedisAsync::RedisConnect(const std::string host,int port,int num)
{
	if(num <= 0 || host.empty() || port <= 0)
	{
		PERROR("RedisConnect paramter error\n");
		return -1;
	}
	
	if(libevent == NULL)
		libevent = event_base_new();
	if(libevent == NULL)
	{
		PERROR("RedisConnect event_base_new error\n");
		return -2;
	}

	host_ = host;
	port_ = port;
	for(int i = 0; i < num; i++)
	{
		PDEBUG("redisAsyncConnect :: %s %d\n",host.c_str(), port);
		redisAsyncContext *c = redisAsyncConnect(host.c_str(), port);
		if (c->err)
		{
			PERROR("RedisConnect redisAsyncConnect :: %s",c->errstr);
			return -2;
		}
		redisLibeventAttach(c,libevent);
		c->data = this;
		redisAsyncSetConnectCallback(c,connectCallback);
    	redisAsyncSetDisconnectCallback(c,disconnectCallback);
		conInitNum++;
	}
	return 0;
}

int RedisAsync::ResetConnect(const std::string host,int port)
{
	host_.clear();
	host_ = host;
	port_ = port;
	ConstRedisAsyncConn ().swap(conns_);
	
	for(int i = 0; i < conInitNum; i++)
	{
		redisAsyncContext *c = redisAsyncConnect(host.c_str(), port);
		if (c->err)
		{
			PERROR();
			return -2;
		}
		redisLibeventAttach(c,libevent);
		c->data = this;
		redisAsyncSetConnectCallback(c,connectCallback);
    	redisAsyncSetDisconnectCallback(c,disconnectCallback);
	}
	return 0;
}

void RedisAsync::RedisOnConnection(const redisAsyncContext *c,int status,bool bConn)
{
	if(bConn)
	{
		if (status == REDIS_OK)
		{
			conns_.push_back(c);
			conSuccNum++;
			PDEBUG("RedisOnConnection conn success\n");
		}else{
			PERROR("connectCallback Error: %s\n", c->errstr);
		}
	}else{
		if (status == REDIS_OK)
		{
			common::MutexLockGuard lock(IterLock);
			for(ConstRedisAsyncConnIter iter = conns_.begin(); iter != conns_.end(); iter++)
			{
				if(*iter == c)
					conns_.erase(iter);
			}
			conSuccNum--;
			PDEBUG("RedisOnConnection disconn success\n");
		}else{
			PERROR("disconnectCallback Error: %s\n", c->errstr);
		}
	}
}

void RedisAsync::RedisLoop()
{
	if(libevent)
		event_base_dispatch(libevent);
}

void RedisAsync::RedisBlockClear()
{
	return;
}

int RedisAsync::set(const std::string& key, const std::string& value, const CmdResultCallBack& retCb)
{
	return 0;
}

int RedisAsync::get(const std::string& key, CmdStrValueCallBack strCb)
{
	return 0;
}


int RedisAsync::hmset(const std::string& key, const HashMap& hashMap, const CmdResultCallBack& retCb)
{
	return 0;
}


}


