
#include <strings.h>
#include <unistd.h>

#include "RedisAsync.h"

namespace ASYNCREDIS
{
static volatile int64_t requestNum = 0;

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

static void respondCallback(redisAsyncContext *c, void *r, void *privdata) {
	RedisRequest *pRedisRequest = static_cast<RedisRequest *>(privdata);
    redisReply *reply = static_cast<redisReply *>(r);
	requestNum--;
	if(reply == NULL)
	{
		PERROR("");
		return;
	}

  	switch (reply->type)
	{
		case REDIS_REPLY_STRING:
			{
				if(pRedisRequest->IsResultCallBack())
				{
					if(reply->str && reply->len && strcasecmp(reply->str, "OK") == 0)
					{
						int64_t ret = 0;
						std::string strErr;
						pRedisRequest->RespondCallBack(ret,strErr);
					}else{
						int64_t ret = -1;
						std::string strErr(reply->str, reply->len);
						pRedisRequest->RespondCallBack(ret,strErr);
					}
				}

				if(pRedisRequest->IsStrValueCallBack())
				{
					std::string strValue(reply->str, reply->len);
					std::string strErr;
					pRedisRequest->RespondCallBack(strValue,strErr);
				}

				if(pRedisRequest->IsStrVectorCallBack())
				{
					StrVector strVector;
					std::string strValue(reply->str, reply->len);
					strVector.push_back(strValue);
					std::string strErr;
					pRedisRequest->RespondCallBack(strVector,strErr);
				}
			}
			break;
		case REDIS_REPLY_ARRAY:
			{
				if(pRedisRequest->IsResultCallBack())
				{
					int64_t ret = 0;
					std::string strErr;
					pRedisRequest->RespondCallBack(ret,strErr);
				}

				if(pRedisRequest->IsStrValueCallBack())
				{
					std::string strValue(reply->str, reply->len);
					std::string strErr;
					pRedisRequest->RespondCallBack(strValue,strErr);
				}

				if(pRedisRequest->IsStrVectorCallBack())
				{
					StrVector strVector;
					for (size_t i = 0; i < reply->elements; i++)
					{
						strVector.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
					}
					std::string strErr;
					pRedisRequest->RespondCallBack(strVector,strErr);
				}
			}
			break;
		case REDIS_REPLY_INTEGER:
			{
				if(pRedisRequest->IsResultCallBack())
				{
					int64_t ret = reply->integer;
					std::string strErr;
					pRedisRequest->RespondCallBack(ret,strErr);
				}

				if(pRedisRequest->IsStrValueCallBack())
				{
					std::string strValue;
					std::string strErr;
					pRedisRequest->RespondCallBack(strValue,strErr);
				}

				if(pRedisRequest->IsStrVectorCallBack())
				{
					StrVector strVector;
					std::string strErr;
					pRedisRequest->RespondCallBack(strVector,strErr);
				}
			}
			break;
		case REDIS_REPLY_STATUS:
			{
				if(pRedisRequest->IsResultCallBack())
				{
					int64_t ret = 0;
					std::string strErr;
					pRedisRequest->RespondCallBack(ret,strErr);
				}

				if(pRedisRequest->IsStrValueCallBack())
				{
					std::string strValue;
					std::string strErr;
					pRedisRequest->RespondCallBack(strValue,strErr);
				}

				if(pRedisRequest->IsStrVectorCallBack())
				{
					StrVector strVector;
					std::string strErr;
					pRedisRequest->RespondCallBack(strVector,strErr);
				}
			}
			break;
		case REDIS_REPLY_NIL:
			{
				if(pRedisRequest->IsResultCallBack())
				{
					int64_t ret = 0;
					std::string strErr;
					pRedisRequest->RespondCallBack(ret,strErr);
				}

				if(pRedisRequest->IsStrValueCallBack())
				{
					std::string strValue;
					std::string strErr;
					pRedisRequest->RespondCallBack(strValue,strErr);
				}

				if(pRedisRequest->IsStrVectorCallBack())
				{
					StrVector strVector;
					std::string strErr;
					pRedisRequest->RespondCallBack(strVector,strErr);
				}
			}
			break;
		case REDIS_REPLY_ERROR:
			{
				if(pRedisRequest->IsResultCallBack())
				{
					int64_t ret = -1;
					std::string strErr(reply->str, reply->len);
					pRedisRequest->RespondCallBack(ret,strErr);
				}

				if(pRedisRequest->IsStrValueCallBack())
				{
					std::string strValue;
					std::string strErr(reply->str, reply->len);
					pRedisRequest->RespondCallBack(strValue,strErr);
				}

				if(pRedisRequest->IsStrVectorCallBack())
				{
					StrVector strVector;
					std::string strErr(reply->str, reply->len);
					pRedisRequest->RespondCallBack(strVector,strErr);
				}
			}
			break;
		default:
			break;
	}
	
	delete pRedisRequest;
	return;
}

RedisAsync::RedisAsync(struct event_base* base)
	:IterLock()
	,asyStatus(0)//0 mean no init,1 mean conning host and port,2 mean conn succ,3 mean conn failed,4 mean will be exit
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
		conns_.push_back(c);
	}
	return 0;
}

int RedisAsync::ResetConnect(const std::string host,int port)
{
	host_.clear();
	host_ = host;
	port_ = port;
	
	RedisBlockClear();
	ConstRedisAsyncConn ().swap(conns_);
	
	for(int i = 0; i < conInitNum; i++)
	{
		redisAsyncContext *c = redisAsyncConnect(host.c_str(), port);
		if (c->err)
		{
			PERROR("");
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

	if(conSuccNum == conInitNum && asyStatus == 0)
		asyStatus = 1;
}

void RedisAsync::RedisLoop()
{
	if(libevent)
		event_base_dispatch(libevent);
}

void RedisAsync::RedisBlockClear()
{
	asyStatus = 0;
	while(requestNum > 0)
		usleep(100);

	for(int i = 0; i < conInitNum; i++)
	{
		redisAsyncDisconnect(conns_[lastIndex]);
	}
	
	for(int i = 0; i < conInitNum; i++)
	{
		redisLibeventCleanup(conns_[lastIndex]->ev.data);
		conns_[lastIndex]->ev.data = NULL;
		redisAsyncFree(conns_[lastIndex]);
	}
	return;
}

int RedisAsync::set(const std::string& key, const std::string& value, const CmdResultCallBack& retCb, void *priv)
{
	if(asyStatus == 0)
	{
		PERROR("redis async no ready now\n");
		return -3;
	}
	if(key.empty() || value.empty())
	{
		PERROR("");
		return -1;
	}
	
	int ret = 0;
	common::MutexLockGuard lock(IterLock);
	if(retCb)
	{
		RedisRequest *pRedisRequest = new RedisRequest(priv,retCb);
		ret = redisAsyncCommand(conns_[lastIndex], respondCallback, static_cast<void *>(pRedisRequest), "SET %s %s", key.c_str(), value.c_str());
	}else{
		ret = redisAsyncCommand(conns_[lastIndex], NULL, NULL, "SET %s %s", key.c_str(), value.c_str());
	}
	if(ret != REDIS_OK)
	{
		PERROR("redisAsyncCommand error :: %d\n", ret);
		return -1;
	}
	requestNum++;
	lastIndex++;
	if(lastIndex >= conSuccNum)
		lastIndex = 0;
	return 0;
}

int RedisAsync::get(const std::string& key, CmdStrValueCallBack strCb, void *priv)
{
	return 0;
}


int RedisAsync::hmset(const std::string& key, const HashMap& hashMap, const CmdResultCallBack& retCb, void *priv)
{
	if(asyStatus == 0)
	{
		PERROR("redis async no ready now\n");
		return -3;
	}
	if(key.empty() || hashMap.empty())
	{
		PERROR();
		return -1;
	}
	
	StrVector msetCmd;
	msetCmd.push_back("HMSET");
	msetCmd.push_back(key);
	for(HashMapConstIter iter = hashMap.begin();iter != hashMap.end();iter++)
	{
		msetCmd.push_back(iter->first);
		msetCmd.push_back(iter->second);
	}

	std::vector<const char* > argv(msetCmd.size());
	std::vector<size_t> argvlen(msetCmd.size());

	size_t i = 0;
	for (std::vector<std::string>::const_iterator it = msetCmd.begin();
	        it != msetCmd.end(); ++i, ++it)
	{
		argv[i] = it->c_str();
		argvlen[i] = it->size();
	}
	
	int ret = 0;
	common::MutexLockGuard lock(IterLock);
	if(retCb)
	{
		RedisRequest *pRedisRequest = new RedisRequest(priv,retCb);
		ret = redisAsyncCommandArgv(conns_[lastIndex], respondCallback, static_cast<void *>(pRedisRequest), static_cast<int>(argv.size()), &(argv[0]), &(argvlen[0]));
	}else{
		ret = redisAsyncCommandArgv(conns_[lastIndex], NULL, NULL, static_cast<int>(argv.size()), &(argv[0]), &(argvlen[0]));
	}
	if(ret != REDIS_OK)
	{
		PERROR("redisAsyncCommandArgv error :: %d\n", ret);
		return -1;
	}
	requestNum++;
	lastIndex++;
	if(lastIndex >= conSuccNum)
		lastIndex = 0;
	return 0;
}

}


