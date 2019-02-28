#include "RedisClusterNode.h"
#include "RedisClient.h"


using namespace rediscluster;

const int kMinSize = 1;
const int kMaxSize = 64;

RedisClusterNode::RedisClusterNode()
	: bStatus_(true),
	  minSize_(2),
	  maxSize_(10),
	  currSize_(0),
	  timeout_(0),
	  password_(""),
	  mutex_()
{
}

RedisClusterNode::~RedisClusterNode()
{
	//WARN("host=%s port=%d will disconnect", info_.ipStr.c_str(), info_.port);
	stop();
}

bool RedisClusterNode::init(const NodeInfo& info,
                            int minSize, int maxSize, int timeout,
                            const std::string& password)
{
	info_ = info;
	minSize_ = minSize < kMinSize ? kMinSize : minSize;
	maxSize_ = maxSize > kMaxSize ? kMaxSize : maxSize;
	timeout_ = timeout;
	password_ = password;

	if (minSize_ > maxSize_)
	{
		return false;
	}

	//异常重连的时候 需要清空回收所有上次建立的连接 仅处理空闲的连接 
	//已经被取走的连接会由removeRedisClient处理释放
	//此过程加锁 为了不让继续获取连接
	std::lock_guard<std::mutex> lock(mutex_);
	for ( auto it=idleList_.begin(); it != idleList_.end(); ++it)
	{
		if (*it)
		{
			(*it)->release();
			delete (*it);
			*it = NULL;
		}
	}
	idleList_.clear();


	for (int i = 0; i < minSize_; ++i)
	{
		RedisClient* client = NULL;
		if (!applayNewRedisClient(client))
		{
			return false;
		}

		idleList_.push_back(client);
	}
	return true;
}


void RedisClusterNode::stop()
{
	std::lock_guard<std::mutex> lock(mutex_);
	std::list<RedisClient* >::iterator it = idleList_.begin();
	for ( ; it != idleList_.end(); ++it)
	{
		if (*it)
		{
			(*it)->release();
			delete (*it);
			*it = NULL;
		}

	}
	idleList_.clear();

	std::map<RedisClient* , int>::iterator it2 = useMap_.begin();
	for ( ; it2 != useMap_.end(); ++it2)
	{
		if (it2->first)
		{
			it2->first->release();
			delete it2->first;
		}
	}
	useMap_.clear();
}

bool RedisClusterNode::applayNewRedisClient(RedisClient*& client)
{
	if (client == NULL)
	{
		client = new RedisClient(this);
		if (client)
		{
			client->init(info_.ipStr, info_.port, timeout_, password_);
			bool bRet = client->connect();
			if (!bRet && client)
			{
				//ERROR("conn redis failed! reason: (%s:%d) %s", info_.ipStr.c_str(), info_.port, client->getLastError().c_str());
				delete client;
				client = NULL;
			}
			return bRet;
		}
	}
	return false;
}

RedisClient* RedisClusterNode::getRedisClient()
{
	RedisClient* client = NULL;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!idleList_.empty())
		{
			client = idleList_.front();
			idleList_.pop_front();
			useMap_.insert(std::make_pair(client, 0));
		}
		else if (idleList_.empty() && static_cast<int>(useMap_.size()) < maxSize_)
		{
			if (applayNewRedisClient(client))
			{
				useMap_.insert(std::make_pair(client, 0));
			}
			else
			{
				// 如果申请新的连接不成功则置节点状态为false
				bStatus_ = false;
			}
		}
	}
	return client;
}

void RedisClusterNode::releaseRedisClient(RedisClient* client)
{
	if (client != NULL)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (useMap_.find(client) != useMap_.end())
		{
			idleList_.push_back(client);
			useMap_.erase(client);
			printf("releaseRedisClient success, idleList_.size(): %lu\n", idleList_.size());
		}
		else
		{
			printf("releaseRedisClient failed, idleList_.size(): %lu\n", idleList_.size());
		}
	}
}

void RedisClusterNode::removeRedisClient(RedisClient* client)
{
	if (client != NULL)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (useMap_.find(client) != useMap_.end())
		{
			useMap_.erase(client);
			client->release();
			client = NULL;
		}
	}

	//遇到异常[视为整个节点异常]移除连接
	bStatus_ = false;
}
