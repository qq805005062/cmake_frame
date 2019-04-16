#ifndef __CLUSTER_REDIS_ASYNC_H__
#define __CLUSTER_REDIS_ASYNC_H__

#include <vector>
#include <functional>

#include "src/Singleton.h"
#include "src/noncopyable.h"


namespace CLUSTER_REDIS_ASYNC
{

typedef std::vector<std::string> StdVectorString;
typedef StdVectorString::iterator StdVectorStringIter;


/*
 * [function] 初始化回调函数，异步初始化，当异步初始化完成之后，回调会被调用
 * @author xiaoxiao 2019-04-14
 * @param ret 初始化返回值，大于等于0是成功，其他是错误
 * @param errMsg 当初始化失败的时候，错误信息
 * @param
 *
 * @return 无
 */
typedef std::function<void(int ret, const std::string& errMsg)> InitCallback;

/*
 * [function] 执行命令结果回调函数。
 * @author xiaoxiao 2019-04-14
 * @param ret 初始化返回值，大于等于0是成功，其他是错误
 * @param priv 当初始化失败的时候，错误信息
 * @param resultMsg
 *
 * @return 无
 */
typedef std::function<void(int64_t ret, void* priv, StdVectorString& resultMsg)> CmdResultCallback;

/*
 * [ClusterRedisAsync]异步redis集群或者单点管理类
 * @author xiaoxiao 2019-04-15
 * @param
 * @param
 * @param
 *
 * @return
 */

class ClusterRedisAsync : public common::noncopyable
{
public:
    ClusterRedisAsync();

    ~ClusterRedisAsync();

    static ClusterRedisAsync& instance() { return common::Singleton<ClusterRedisAsync>::instance(); }

    /*
     * [redisAsyncInit] 初始化方法,单线程初始化，只需要初始化一次
     * @author xiaoxiao 2019-04-15
     * @param threadNum 内部线程数，内部会开多少个线程来处理与redis的io。建议参考集群数量，但是也不是要特别多，最少也要是一个
     * @param ipPortInfo redis集群信息，可以配置单点，也可以配置多个点。初始化如果连不上会失败，内部支持主从切换、支持自动重连
     * @param initcb 初始化完成回调函数，无论成功与失败，都会回调
     * @param connNum 连接数量，因为是异步，所以基本上1个也就够用了，当然也可以初始化多个
     *
     * @return 大于等于0是成功，其他是错误，内部异步初始化
     */
    int redisAsyncInit(int threadNum, const std::string& ipPortInfo, InitCallback& initcb, int connNum = 1);

    int set(const std::string& key, const std::string& value, int outSecond, const CmdResultCallback& cb, void *priv);

    /*
     * [redisAppendCommand]异步
     * @author
     * @param
     * @param
     * @param
     *
     * @return
     */
    int redisAsyncCommand(const CmdResultCallback& cb, int outSecond, void *priv, const char *format, ...);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void redisAsyncConnect();

    void libeventIoThread(int index);
private:

    void asyncInitCallBack(int ret);

    int connNum_;
    int ioThreadNum_;
    
    InitCallback initCb_;
};

}
#endif
