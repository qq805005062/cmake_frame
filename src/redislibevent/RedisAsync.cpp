
#include <strings.h>
#include <unistd.h>

#include "RedisAsync.h"

namespace ASYNCREDIS
{
static volatile int64_t requestNum = 0;

static void connectCallback(const redisAsyncContext *c, int status)
{
	PDEBUG("RedisOnConnection connectCallback\n");
	RedisAsync *pRedis = static_cast<RedisAsync *>(c->data);
	pRedis->RedisOnConnection(c,status,true);
}

static void disconnectCallback(const redisAsyncContext *c, int status)
{
	PDEBUG("RedisOnConnection disconnectCallback\n");
	RedisAsync *pRedis = static_cast<RedisAsync *>(c->data);
	pRedis->RedisOnConnection(c,status,false);
}

static void respondCallback(redisAsyncContext *c, void *r, void *privdata) {
	RedisRequest *pRedisRequest = static_cast<RedisRequest *>(privdata);
    redisReply *reply = static_cast<redisReply *>(r);
	requestNum--;
	if(reply == NULL)
	{
		PERROR("respondCallback redisReply NULL,This suitation will never happen,big problem\n");
		return;
	}

  	switch (reply->type)
	{
		case REDIS_REPLY_STRING:
			{
				PDEBUG("respondCallback REDIS_REPLY_STRING type :: %s -- %lu -- %lld -- %lu\n",reply->str,reply->len,reply->integer,reply->elements);
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
				PDEBUG("respondCallback REDIS_REPLY_ARRAY type :: %s -- %lu -- %lld -- %lu\n",reply->str,reply->len,reply->integer,reply->elements);
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
				PDEBUG("respondCallback REDIS_REPLY_INTEGER type :: %s -- %lu -- %lld -- %lu\n",reply->str,reply->len,reply->integer,reply->elements);
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
				PDEBUG("respondCallback REDIS_REPLY_STATUS type :: %s -- %lu -- %lld -- %lu\n",reply->str,reply->len,reply->integer,reply->elements);
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
				PDEBUG("respondCallback REDIS_REPLY_NIL type :: %s -- %lu -- %lld -- %lu\n",reply->str,reply->len,reply->integer,reply->elements);
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
				PDEBUG("respondCallback REDIS_REPLY_ERROR type :: %s -- %lu -- %lld -- %lu\n",reply->str,reply->len,reply->integer,reply->elements);
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
	PDEBUG("RedisAsync\n");
}

RedisAsync::~RedisAsync()
{
	PDEBUG("RedisAsync exit\n");
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

	PDEBUG("RedisConnect :: host :: %s,port :: %d,connnum :: %d\n",host.c_str(),port,num);
	host_ = host;
	port_ = port;
	conInitNum = num;
	for(int i = 0; i < num; i++)
	{
		redisAsyncContext *c = redisAsyncConnect(host.c_str(), port);
		if (c->err)
		{
			PERROR("RedisConnect redisAsyncConnect :: %s -- %d\n",c->errstr,i);
			return -2;
		}
		redisLibeventAttach(c,libevent);
		c->data = this;
		redisAsyncSetConnectCallback(c,connectCallback);
    	redisAsyncSetDisconnectCallback(c,disconnectCallback);
		conns_.push_back(c);
		PDEBUG("RedisConnect redisAsyncConnect :: %d\n",i);
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
	PDEBUG("RedisAsync :: ResetConnect :: clear before\n");
	for(int i = 0; i < conInitNum; i++)
	{
		redisAsyncContext *c = redisAsyncConnect(host.c_str(), port);
		if (c->err)
		{
			PERROR("redisAsyncConnect error :: %s -- %d\n",c->errstr,i);
			return -2;
		}
		redisLibeventAttach(c,libevent);
		c->data = this;
		redisAsyncSetConnectCallback(c,connectCallback);
    	redisAsyncSetDisconnectCallback(c,disconnectCallback);
		conns_.push_back(c);
		PDEBUG("RedisConnect redisAsyncConnect ResetConnect :: %d\n",i);
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
			PDEBUG("RedisOnConnection conn success num :: %d\n",conSuccNum);
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
				{
					redisLibeventCleanup((*iter)->ev.data);
					(*iter)->ev.data = NULL;
					redisAsyncFree(*iter);
					conns_.erase(iter);
				}
			}
			conSuccNum--;
			PDEBUG("RedisOnConnection disconn now conn num :: %d -- %ld\n",conSuccNum,conns_.size());
		}else{
			PERROR("disconnectCallback Error: %s\n", c->errstr);
		}
	}

	if(conSuccNum == conInitNum && asyStatus == 0)
	{
		PDEBUG("redis connection init num all success\n");
		asyStatus = 1;
	}
}

void RedisAsync::RedisLoop()
{
	if(libevent)
		event_base_dispatch(libevent);
	PERROR("Redis no event loop may be disconnect\n");
}

void RedisAsync::RedisBlockClear()
{
	asyStatus = 0;
	PDEBUG("RedisAsync :: RedisBlockClear :: %ld\n",requestNum);
	while(requestNum > 0)
		usleep(100);
	PDEBUG("RedisAsync :: RedisBlockClear :: %ld\n",requestNum);
	for(int i = 0; i < conSuccNum; i++)
	{
		redisAsyncDisconnect(conns_[i]);
	}
	PDEBUG("RedisAsync :: RedisBlockClear redisAsyncDisconnect :: %d\n",conSuccNum);
	for(int i = 0; i < conInitNum; i++)
	{
		redisLibeventCleanup(conns_[lastIndex]->ev.data);
		conns_[lastIndex]->ev.data = NULL;
		redisAsyncFree(conns_[lastIndex]);
	}
	PDEBUG("RedisAsync :: RedisBlockClear redisLibeventCleanup :: %d\n",conSuccNum);
	return;
}

int RedisAsync::set(const std::string& key, const std::string& value, const CmdResultCallBack& retCb, void *priv)
{
	while(asyStatus == 0)
		usleep(100);
	if(key.empty() || value.empty())
	{
		PERROR("RedisAsync set paramter null\n");
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
	PDEBUG("RedisAsync set %s %s\n",key.c_str(), value.c_str());
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
	while(asyStatus == 0)
		usleep(100);
	if(key.empty() || hashMap.empty())
	{
		PERROR("hmset paramter empty\n");
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
		//PDEBUG("hmset cmd :: %s len :: %ld\n", argv[i], argvlen[i]);
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
	PDEBUG("RedisAsync hmset key %s\n",key.c_str());
	requestNum++;
	lastIndex++;
	if(lastIndex >= conSuccNum)
		lastIndex = 0;
	return 0;
}

}


